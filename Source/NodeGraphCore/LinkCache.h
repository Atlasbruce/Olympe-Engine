/**
 * @file LinkCache.h
 * @brief Incoming/outgoing link cache for efficient node-graph traversal (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * LinkCache builds two hash-maps (source-node -> outgoing links, target-node ->
 * incoming links) from a flat vector of ExecPinConnections.  The cache is
 * invalidated whenever the topology changes and lazily rebuilt on next access.
 *
 * No ImGui dependency.  No external dependencies beyond the standard library
 * and TaskGraphTemplate.h.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

// ============================================================================
// LinkCache
// ============================================================================

/**
 * @class LinkCache
 * @brief Singleton that caches per-node incoming/outgoing ExecPinConnection lists.
 *
 * Typical usage:
 * @code
 *   auto& lc = LinkCache::Get();
 *   lc.Rebuild(myTemplate.ExecConnections);
 *   const auto& out = lc.GetOutgoing(nodeId);
 *   const auto& in  = lc.GetIncoming(nodeId);
 * @endcode
 */
class LinkCache {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /** @brief Returns the single shared instance. */
    static LinkCache& Get();

    // -----------------------------------------------------------------------
    // Cache management
    // -----------------------------------------------------------------------

    /**
     * @brief Rebuilds both outgoing and incoming maps from the supplied list.
     *
     * @param connections  Flat list of exec-pin connections (e.g. from a
     *                     TaskGraphTemplate).
     */
    void Rebuild(const std::vector<ExecPinConnection>& connections);

    /**
     * @brief Marks the cache as stale.  The next Rebuild() call refreshes it.
     */
    void Invalidate();

    /**
     * @brief Returns true if the cache has been built and is not stale.
     */
    bool IsValid() const;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the list of connections that originate from @p nodeId.
     *        Returns an empty vector if the node has no outgoing connections.
     */
    const std::vector<ExecPinConnection>& GetOutgoing(int nodeId) const;

    /**
     * @brief Returns the list of connections that terminate at @p nodeId.
     *        Returns an empty vector if the node has no incoming connections.
     */
    const std::vector<ExecPinConnection>& GetIncoming(int nodeId) const;

    /**
     * @brief Returns the total number of connections stored in the cache.
     */
    int GetConnectionCount() const;

private:

    LinkCache();

    std::unordered_map<int, std::vector<ExecPinConnection>> m_Outgoing;
    std::unordered_map<int, std::vector<ExecPinConnection>> m_Incoming;
    std::vector<ExecPinConnection>                          m_EmptyList; ///< Returned for misses
    bool                                                    m_IsValid;
    int                                                     m_ConnectionCount;
};

} // namespace Olympe
