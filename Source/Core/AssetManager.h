/**
 * @file AssetManager.h
 * @brief Central cache for loaded engine assets, including TaskGraphTemplate instances.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * AssetManager is a singleton that loads, caches, and manages the lifetime of
 * shared engine assets.  Phase 1.3 adds support for TaskGraphTemplate assets:
 *
 *   - LoadTaskGraph()   : load from file (or return cached entry) -> AssetID
 *   - GetTaskGraph()    : retrieve a cached template by AssetID
 *   - UnloadTaskGraph() : release a cached template from memory
 *
 * Asset IDs are 32-bit FNV-1a hashes of the normalised file path so that
 * the same path always yields the same ID without requiring a central registry.
 *
 * Ownership:
 *   AssetManager owns all loaded TaskGraphTemplate instances via unique_ptr.
 *   Callers receive a raw pointer (non-owning) from GetTaskGraph().
 *   The pointer remains valid until UnloadTaskGraph() or ~AssetManager() is called.
 *
 * C++14 compliant - no std::variant, std::optional, or C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

// ============================================================================
// Type aliases
// ============================================================================

/// Opaque asset identifier: 32-bit FNV-1a hash of the asset file path.
using AssetID = uint32_t;

/// Sentinel value indicating an invalid / unloaded asset.
static const AssetID INVALID_ASSET_ID = 0u;

// ============================================================================
// AssetManager
// ============================================================================

/**
 * @class AssetManager
 * @brief Singleton cache for engine assets.
 *
 * @details
 * Call Get() to obtain the singleton instance.  All TaskGraphTemplate assets
 * loaded through LoadTaskGraph() are owned by AssetManager; callers must not
 * delete the pointers returned by GetTaskGraph().
 */
class AssetManager {
public:

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the singleton AssetManager instance.
     */
    static AssetManager& Get();

    // -----------------------------------------------------------------------
    // TaskGraph asset API
    // -----------------------------------------------------------------------

    /**
     * @brief Loads a TaskGraphTemplate from @p path and caches it.
     *
     * If the asset has already been loaded (same path hash), the cached
     * AssetID is returned immediately without re-reading the file.
     *
     * @param path      Absolute or relative path to the JSON task graph file.
     * @param outErrors Receives human-readable error messages on failure.
     * @return A non-zero AssetID on success, or INVALID_ASSET_ID on failure.
     *
     * @note AssetManager owns the loaded template.  Do NOT delete the pointer
     *       returned by GetTaskGraph().
     */
    AssetID LoadTaskGraph(const std::string& path,
                          std::vector<std::string>& outErrors);

    /**
     * @brief Returns a non-owning pointer to the cached TaskGraphTemplate.
     *
     * @param id  AssetID previously returned by LoadTaskGraph().
     * @return Pointer to the template, or nullptr if @p id is unknown.
     *
     * @note The returned pointer is valid until UnloadTaskGraph() or the
     *       AssetManager is destroyed.  Do NOT delete it.
     */
    const TaskGraphTemplate* GetTaskGraph(AssetID id) const;

    /**
     * @brief Releases the cached TaskGraphTemplate for @p id.
     *
     * After this call, any raw pointers previously obtained from
     * GetTaskGraph() for the same @p id are dangling and must not be used.
     *
     * @param id  AssetID returned by LoadTaskGraph().
     */
    void UnloadTaskGraph(AssetID id);

    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    /**
     * @brief Computes the 32-bit FNV-1a hash of a file path string.
     *
     * The hash is used as the AssetID so that the same path always maps to
     * the same identifier.  Exposed publicly so callers can pre-compute IDs.
     *
     * @param path  Any non-empty string.
     * @return 32-bit FNV-1a hash.  Returns INVALID_ASSET_ID (0) only if the
     *         input is empty.
     */
    static AssetID ComputeAssetID(const std::string& path);

private:

    // Prevent external construction / copying
    AssetManager()  = default;
    ~AssetManager() = default;
    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------

    /// Cached task graph templates: AssetID -> owned template.
    std::unordered_map<AssetID, std::unique_ptr<TaskGraphTemplate>> m_taskGraphs;
};

} // namespace Olympe
