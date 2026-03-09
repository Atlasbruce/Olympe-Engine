/**
 * @file LinkCache.cpp
 * @brief LinkCache implementation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "LinkCache.h"

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

LinkCache& LinkCache::Get()
{
    static LinkCache s_Instance;
    return s_Instance;
}

LinkCache::LinkCache()
    : m_IsValid(false)
    , m_ConnectionCount(0)
{
}

// ============================================================================
// Cache management
// ============================================================================

void LinkCache::Rebuild(const std::vector<ExecPinConnection>& connections)
{
    m_Outgoing.clear();
    m_Incoming.clear();
    m_ConnectionCount = 0;

    for (size_t i = 0; i < connections.size(); ++i)
    {
        const ExecPinConnection& conn = connections[i];
        m_Outgoing[static_cast<int>(conn.SourceNodeID)].push_back(conn);
        m_Incoming[static_cast<int>(conn.TargetNodeID)].push_back(conn);
        ++m_ConnectionCount;
    }

    m_IsValid = true;
}

void LinkCache::Invalidate()
{
    m_IsValid = false;
}

bool LinkCache::IsValid() const
{
    return m_IsValid;
}

// ============================================================================
// Accessors
// ============================================================================

const std::vector<ExecPinConnection>& LinkCache::GetOutgoing(int nodeId) const
{
    auto it = m_Outgoing.find(nodeId);
    if (it != m_Outgoing.end())
        return it->second;
    return m_EmptyList;
}

const std::vector<ExecPinConnection>& LinkCache::GetIncoming(int nodeId) const
{
    auto it = m_Incoming.find(nodeId);
    if (it != m_Incoming.end())
        return it->second;
    return m_EmptyList;
}

int LinkCache::GetConnectionCount() const
{
    return m_ConnectionCount;
}

} // namespace Olympe
