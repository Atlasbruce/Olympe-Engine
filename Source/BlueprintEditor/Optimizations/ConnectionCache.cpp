/**
 * @file ConnectionCache.cpp
 * @brief Bezier control-point cache implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "ConnectionCache.h"

#include <cstring>

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

ConnectionCache& ConnectionCache::Get()
{
    static ConnectionCache s_Instance;
    return s_Instance;
}

// ============================================================================
// Construction
// ============================================================================

ConnectionCache::ConnectionCache()
{
    std::memset(&m_DefaultBezier, 0, sizeof(m_DefaultBezier));
}

// ============================================================================
// Cache operations
// ============================================================================

bool ConnectionCache::HasCached(int connectionId) const
{
    return m_Cache.find(connectionId) != m_Cache.end();
}

const BezierPoints& ConnectionCache::GetBezier(int connectionId) const
{
    std::map<int, BezierPoints>::const_iterator it = m_Cache.find(connectionId);
    if (it == m_Cache.end())
        return m_DefaultBezier;
    return it->second;
}

void ConnectionCache::SetBezier(int connectionId, const BezierPoints& pts)
{
    m_Cache[connectionId] = pts;
}

void ConnectionCache::Invalidate(int connectionId)
{
    m_Cache.erase(connectionId);
}

void ConnectionCache::InvalidateAll()
{
    m_Cache.clear();
}

size_t ConnectionCache::GetCacheSize() const
{
    return m_Cache.size();
}

} // namespace Olympe
