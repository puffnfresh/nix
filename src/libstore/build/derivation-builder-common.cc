#include "build/derivation-builder-common.hh"
#include "nix/util/logging.hh"
#include "nix/util/strings.hh"
#include "nix/util/environment-variables.hh"
#include "nix/util/file-system.hh"
#include "nix/util/processes.hh"
#include "nix/store/local-store.hh"
#include "nix/store/globals.hh"

#ifndef _WIN32
#  include <unistd.h>
#endif

namespace nix {

std::tuple<OutputPathMap, StringMap, std::map<StorePath, StorePath>>
computeScratchOutputs(LocalStore & store, const DerivationBuilderParams & params, bool needsHashRewrite)
{
    OutputPathMap scratchOutputs;
    StringMap inputRewrites;
    std::map<StorePath, StorePath> redirectedOutputs;
    for (auto & [outputName, status] : params.initialOutputs) {
        auto makeFallbackPath = [&](const std::string & suffix, std::string_view name) {
            return store.makeStorePath(
                "rewrite:" + std::string(params.drvPath.to_string()) + ":" + suffix, Hash(HashAlgorithm::SHA256), name);
        };
        auto scratchPath =
            !status.known
                ? makeFallbackPath("name:" + std::string(outputName), outputPathName(params.drv.name, outputName))
            : !needsHashRewrite          ? status.known->path
            : !status.known->isPresent() ? status.known->path
            : params.buildMode != bmRepair && !status.known->isValid()
                ? status.known->path
                : makeFallbackPath(std::string(status.known->path.to_string()), status.known->path.name());
        scratchOutputs.insert_or_assign(outputName, scratchPath);

        inputRewrites[hashPlaceholder(outputName)] = store.printStorePath(scratchPath);

        if (!status.known)
            continue;
        auto fixedFinalPath = status.known->path;

        if (fixedFinalPath == scratchPath)
            continue;

        deletePath(store.printStorePath(scratchPath));

        {
            std::string h1{fixedFinalPath.hashPart()};
            std::string h2{scratchPath.hashPart()};
            inputRewrites[h1] = h2;
        }

        redirectedOutputs.insert_or_assign(std::move(fixedFinalPath), std::move(scratchPath));
    }

    return {std::move(scratchOutputs), std::move(inputRewrites), std::move(redirectedOutputs)};
}

void logBuilderInfo(const BasicDerivation & drv)
{
    printMsg(lvlChatty, "executing builder '%1%'", drv.builder);
    printMsg(lvlChatty, "using builder args '%1%'", concatStringsSep(" ", drv.args));
    for (auto & i : drv.env)
        printMsg(lvlVomit, "setting builder env variable '%1%'='%2%'", i.first, i.second);
}

StringMap initBaseEnv(
    const std::string & storeDir,
    const DerivationBuilderParams & params,
    const StringMap & inputRewrites,
    const DerivationType & derivationType,
    const std::filesystem::path & tmpDirInSandbox,
    std::function<void(const std::string &, std::string_view)> writeExtraFile,
    std::optional<std::filesystem::path> homeDir)
{
    StringMap env;

    env["PATH"] = "/path-not-set";
    if (homeDir)
        env["HOME"] = homeDir->string();
    env["NIX_STORE"] = storeDir;
    env["NIX_BUILD_CORES"] = fmt(
        "%d",
        settings.getLocalSettings().buildCores ? settings.getLocalSettings().buildCores : settings.getDefaultCores());

    for (const auto & [name, info] : params.desugaredEnv.variables) {
        env[name] = info.prependBuildDirectory ? (tmpDirInSandbox / info.value).string() : info.value;
    }

    for (const auto & [fileName, value] : params.desugaredEnv.extraFiles) {
        writeExtraFile(fileName, rewriteStrings(value, inputRewrites));
    }

    env["NIX_BUILD_TOP"] = tmpDirInSandbox.string();
    env["TMPDIR"] = env["TEMPDIR"] = env["TMP"] = env["TEMP"] = tmpDirInSandbox.string();
    env["PWD"] = tmpDirInSandbox.string();

    if (derivationType.isFixed())
        env["NIX_OUTPUT_CHECKED"] = "1";

    if (!derivationType.isSandboxed()) {
        for (auto & i : params.drvOptions.impureEnvVars) {
            env[i] = getEnv(i).value_or("");
        }
    }

    env["NIX_LOG_FD"] = "2";
    env["TERM"] = "xterm-256color";

    return env;
}

int commonUnprepare(
    Pid & pid,
    const Store & store,
    const StorePath & drvPath,
    BuildResult & buildResult,
    DerivationBuilderCallbacks & miscMethods,
    AutoCloseFD & builderOut)
{
    int status = pid.kill();

    debug("builder process for '%s' finished", store.printStorePath(drvPath));

    buildResult.timesBuilt++;
    buildResult.stopTime = time(0);

    miscMethods.childTerminated();

    builderOut.close();

    miscMethods.closeLogFile();

    return status;
}

void cleanupBuildCore(
    bool force,
    LocalStore & store,
    const std::map<StorePath, StorePath> & redirectedOutputs,
    const BasicDerivation & drv,
    std::filesystem::path & topTmpDir,
    std::filesystem::path & tmpDir)
{
    if (force) {
        for (auto & i : redirectedOutputs)
            deletePath(store.toRealPath(i.second));
    }

    if (topTmpDir != "") {
#ifndef _WIN32
        chmod(topTmpDir, 0000);
#endif

        if (settings.keepFailed && !force && !drv.isBuiltin()) {
            printError("note: keeping build directory %s", PathFmt(tmpDir));
#ifndef _WIN32
            chmod(topTmpDir, 0755);
            chmod(tmpDir, 0755);
#endif
        } else
            deletePath(topTmpDir);
        topTmpDir = "";
        tmpDir = "";
    }
}

} // namespace nix
