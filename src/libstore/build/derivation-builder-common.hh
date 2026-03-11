#pragma once

#include "nix/store/build/derivation-builder.hh"

#include <filesystem>
#include <functional>
#include <optional>
#include <tuple>

namespace nix {

class LocalStore;
class Pid;

/**
 * Compute scratch output paths and set up hash rewrites for each output.
 */
std::tuple<OutputPathMap, StringMap, std::map<StorePath, StorePath>>
computeScratchOutputs(LocalStore & store, const DerivationBuilderParams & params, bool needsHashRewrite);

/**
 * Log chatty builder info.
 */
void logBuilderInfo(const BasicDerivation & drv);

/**
 * Set up the base build environment variables (portable).
 *
 * @param writeExtraFile Callback to write extra files from desugared env.
 *   On Unix this calls writeBuilderFile (with chown); on Windows it just writes the file.
 * @param homeDir If set, HOME is set to this. On Unix this is /homeless-shelter.
 */
StringMap initBaseEnv(
    const std::string & storeDir,
    const DerivationBuilderParams & params,
    const StringMap & inputRewrites,
    const DerivationType & derivationType,
    const std::filesystem::path & tmpDirInSandbox,
    std::function<void(const std::string &, std::string_view)> writeExtraFile,
    std::optional<std::filesystem::path> homeDir = std::nullopt);

/**
 * Common first part of `unprepareBuild()`: kill/wait for the child,
 * update build result, close log file. Returns the exit status.
 */
int commonUnprepare(
    Pid & pid,
    const Store & store,
    const StorePath & drvPath,
    BuildResult & buildResult,
    DerivationBuilderCallbacks & miscMethods,
    AutoCloseFD & builderOut);

/**
 * Common core of `cleanupBuild()`: delete redirected outputs if forced,
 * handle keepFailed, clean up tmpDir.
 */
void cleanupBuildCore(
    bool force,
    LocalStore & store,
    const std::map<StorePath, StorePath> & redirectedOutputs,
    const BasicDerivation & drv,
    std::filesystem::path & topTmpDir,
    std::filesystem::path & tmpDir);

} // namespace nix
