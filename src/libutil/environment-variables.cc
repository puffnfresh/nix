#include "util.hh"
#include "environment-variables.hh"

extern char ** environ __attribute__((weak));

namespace nix {

std::optional<std::string> getEnv(const std::string & key)
{
#ifndef _WIN32
    char * value = getenv(key.c_str());
    if (!value)
        return {};
    return std::string(value);
#else
    char buffer[255] = {0};
    DWORD resultSize = GetEnvironmentVariableA(key.c_str(), buffer, 255);
    if (!resultSize)
        return {};
    std::string value = buffer;
    value.resize(resultSize);
    return value;
#endif
}

std::optional<std::string> getEnvNonEmpty(const std::string & key)
{
    auto value = getEnv(key);
    if (value == "")
        return {};
    return value;
}

std::map<std::string, std::string> getEnv()
{
    std::map<std::string, std::string> env;
    for (size_t i = 0; environ[i]; ++i) {
        auto s = environ[i];
        auto eq = strchr(s, '=');
        if (!eq)
            // invalid env, just keep going
            continue;
        env.emplace(std::string(s, eq), std::string(eq + 1));
    }
    return env;
}

void clearEnv()
{
    for (auto & name : getEnv())
        unsetenv(name.first.c_str());
}

void replaceEnv(const std::map<std::string, std::string> & newEnv)
{
    clearEnv();
    for (auto & newEnvVar : newEnv)
        setEnv(newEnvVar.first.c_str(), newEnvVar.second.c_str());
}

}
