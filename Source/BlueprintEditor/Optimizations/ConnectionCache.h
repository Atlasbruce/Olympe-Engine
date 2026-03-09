/**
 * @file ConnectionCache.h
 * @brief Bezier control-point cache for VS graph connections (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * ConnectionCache avoids recomputing cubic Bezier control points for every
 * rendered connection each frame.  When a connection's endpoints change,
 * the caller calls Invalidate() so the renderer recomputes and re-stores
 * the points via SetBezier().
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <cstddef>
#include <map>

namespace Olympe {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @struct BezierPoints
 * @brief Four cubic Bezier control points (P0..P3).
 */
struct BezierPoints {
    float p0x, p0y;  ///< Start point
    float p1x, p1y;  ///< First control point
    float p2x, p2y;  ///< Second control point
    float p3x, p3y;  ///< End point
};

// ============================================================================
// ConnectionCache
// ============================================================================

/**
 * @class ConnectionCache
 * @brief Singleton cache mapping connection IDs to Bezier control points.
 *
 * Typical usage:
 * @code
 *   auto& cc = ConnectionCache::Get();
 *   if (!cc.HasCached(connId))
 *   {
 *       BezierPoints pts = ComputeBezier(src, dst);
 *       cc.SetBezier(connId, pts);
 *   }
 *   const BezierPoints& pts = cc.GetBezier(connId);
 *   DrawBezier(pts);
 * @endcode
 */
class ConnectionCache {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the single shared instance.
     */
    static ConnectionCache& Get();

    // -----------------------------------------------------------------------
    // Cache operations
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if Bezier points are stored for @p connectionId.
     */
    bool HasCached(int connectionId) const;

    /**
     * @brief Returns the cached Bezier points for @p connectionId.
     *        Returns a default (zero-initialised) BezierPoints if not cached.
     */
    const BezierPoints& GetBezier(int connectionId) const;

    /**
     * @brief Stores or overwrites the Bezier points for @p connectionId.
     */
    void SetBezier(int connectionId, const BezierPoints& pts);

    /**
     * @brief Removes the cached entry for @p connectionId.
     */
    void Invalidate(int connectionId);

    /**
     * @brief Clears all cached entries.
     */
    void InvalidateAll();

    /**
     * @brief Returns the number of entries currently in the cache.
     */
    size_t GetCacheSize() const;

private:

    ConnectionCache();

    std::map<int, BezierPoints> m_Cache;
    BezierPoints                m_DefaultBezier;  ///< Returned when ID is not in cache
};

} // namespace Olympe
