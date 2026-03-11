#pragma once
///@file

#include <optional>
#include <string>
#include <windows.h>

namespace nix {

/**
 * Flags for BfSetupFilter, matching the undocumented bindfltapi.h definitions.
 */
enum BindFltFlags : ULONG {
    BINDFLT_FLAG_READ_ONLY_MAPPING = 0x00000001,
    BINDFLT_FLAG_MERGED_BIND_MAPPING = 0x00000002,
    BINDFLT_FLAG_USE_CURRENT_SILO_MAPPING = 0x00000004,
    BINDFLT_FLAG_REPARSE_ON_FILES = 0x00000008,
    BINDFLT_FLAG_SKIP_SHARING_CHECK = 0x00000010,
    BINDFLT_FLAG_IMMUTABLE_BACKING = 0x00000020,
};

/**
 * Dynamic loading wrapper around bindfltapi.dll.
 *
 * The BindFlt mini-filter driver provides kernel-level filesystem path
 * virtualization scoped to a job object. We use it to sandbox builds:
 * hide the entire Nix store, then overlay only declared inputs (read-only)
 * and output paths (read-write).
 *
 * Uses the undocumented BfSetupFilter API (Win10 1903+) which is the only
 * API that supports job-object scoping. Loaded dynamically so there is no
 * link-time dependency.
 */
class BindFltApi
{
public:
    /**
     * Get the singleton instance, or std::nullopt if bindfltapi.dll
     * is not available on this system.
     */
    static std::optional<BindFltApi> & instance();

    /**
     * Try to load bindfltapi.dll. Returns true if successful.
     * Called automatically by instance().
     */
    static bool tryLoad();

    /**
     * Create a bind filter mapping on a job object.
     *
     * @param jobHandle Handle to the job object to scope the mapping to.
     * @param flags Combination of BindFltFlags.
     * @param virtualPath The path that the process will see.
     * @param backingPath The real path that virtualPath maps to.
     * @returns HRESULT from BfSetupFilter.
     */
    HRESULT setupFilter(
        HANDLE jobHandle, ULONG flags, const std::wstring & virtualPath, const std::wstring & backingPath) const;

    /**
     * Remove a bind filter mapping from a job object.
     *
     * @param jobHandle Handle to the job object.
     * @param virtualPath The virtual path to unmap.
     * @returns HRESULT from BfRemoveMapping.
     */
    HRESULT removeMapping(HANDLE jobHandle, const std::wstring & virtualPath) const;

private:
    using BfSetupFilterFn = HRESULT(WINAPI *)(
        HANDLE jobHandle,
        ULONG flags,
        LPCWSTR virtualizationRootPath,
        LPCWSTR virtualizationTargetPath,
        LPCWSTR * virtualizationExceptionPaths,
        ULONG numExceptionPaths);

    using BfRemoveMappingFn = HRESULT(WINAPI *)(HANDLE jobHandle, LPCWSTR virtualizationRootPath);

    HMODULE hModule = nullptr;
    BfSetupFilterFn pfnSetupFilter = nullptr;
    BfRemoveMappingFn pfnRemoveMapping = nullptr;

    BindFltApi(HMODULE hModule, BfSetupFilterFn setupFilter, BfRemoveMappingFn removeMapping)
        : hModule{hModule}
        , pfnSetupFilter{setupFilter}
        , pfnRemoveMapping{removeMapping}
    {
    }
};

} // namespace nix
