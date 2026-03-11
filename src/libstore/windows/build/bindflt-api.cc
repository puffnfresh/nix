#include "bindflt-api.hh"

#include "nix/util/logging.hh"

namespace nix {

bool BindFltApi::tryLoad()
{
    auto & inst = instance();
    return inst.has_value();
}

std::optional<BindFltApi> & BindFltApi::instance()
{
    static std::optional<BindFltApi> inst = []() -> std::optional<BindFltApi> {
        HMODULE hMod = LoadLibraryW(L"bindfltapi.dll");
        if (!hMod) {
            debug("bindfltapi.dll not available (error %d)", GetLastError());
            return std::nullopt;
        }

        auto setupFilter = reinterpret_cast<BfSetupFilterFn>(GetProcAddress(hMod, "BfSetupFilter"));
        auto removeMapping = reinterpret_cast<BfRemoveMappingFn>(GetProcAddress(hMod, "BfRemoveMapping"));

        if (!setupFilter || !removeMapping) {
            debug("bindfltapi.dll loaded but required functions not found");
            FreeLibrary(hMod);
            return std::nullopt;
        }

        debug("bindfltapi.dll loaded successfully");
        return BindFltApi{hMod, setupFilter, removeMapping};
    }();
    return inst;
}

HRESULT BindFltApi::setupFilter(
    HANDLE jobHandle, ULONG flags, const std::wstring & virtualPath, const std::wstring & backingPath) const
{
    return pfnSetupFilter(
        jobHandle,
        flags,
        virtualPath.c_str(),
        backingPath.c_str(),
        nullptr, /* no exception paths */
        0);
}

HRESULT BindFltApi::removeMapping(HANDLE jobHandle, const std::wstring & virtualPath) const
{
    return pfnRemoveMapping(jobHandle, virtualPath.c_str());
}

} // namespace nix
