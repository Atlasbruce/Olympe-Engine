/**
 * @file TaskGraphTemplate.cpp
 * @brief Implementation of TaskGraphTemplate validation and lookup for the Atomic Task System
 * @author Olympe Engine
 * @date 2026-02-20
 */

#include "TaskGraphTemplate.h"

#include <stdexcept>
#include <string>

#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Validation
// ============================================================================

bool TaskGraphTemplate::Validate() const
{
    // Rule 1: Nodes must not be empty
    if (Nodes.empty())
    {
        SYSTEM_LOG << "[TaskGraphTemplate] Validate failed: Nodes is empty" << std::endl;
        return false;
    }

    // Build a temporary set of valid IDs for O(n) lookups
    std::unordered_map<int32_t, bool> validIds;
    for (size_t i = 0; i < Nodes.size(); ++i)
    {
        validIds[Nodes[i].NodeID] = true;
    }

    // Rule 2: RootNodeID must reference an existing node
    if (validIds.find(RootNodeID) == validIds.end())
    {
        SYSTEM_LOG << "[TaskGraphTemplate] Validate failed: RootNodeID " << RootNodeID
                   << " does not reference a known node" << std::endl;
        return false;
    }

    // Rule 3: All ChildrenIDs must reference existing nodes
    for (size_t i = 0; i < Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = Nodes[i];
        for (size_t c = 0; c < node.ChildrenIDs.size(); ++c)
        {
            int32_t childId = node.ChildrenIDs[c];
            if (validIds.find(childId) == validIds.end())
            {
                SYSTEM_LOG << "[TaskGraphTemplate] Validate failed: Node " << node.NodeID
                           << " references unknown child " << childId << std::endl;
                return false;
            }
        }
    }

    SYSTEM_LOG << "[TaskGraphTemplate] Validate passed for template '" << Name << "'" << std::endl;
    return true;
}

// ============================================================================
// Node lookup
// ============================================================================

const TaskNodeDefinition* TaskGraphTemplate::GetNode(int32_t nodeId) const
{
    auto it = m_nodeLookup.find(nodeId);
    if (it == m_nodeLookup.end())
    {
        return nullptr;
    }
    return it->second;
}

void TaskGraphTemplate::BuildLookupCache()
{
    m_nodeLookup.clear();

    for (size_t i = 0; i < Nodes.size(); ++i)
    {
        m_nodeLookup[Nodes[i].NodeID] = &Nodes[i];
    }

    SYSTEM_LOG << "[TaskGraphTemplate] BuildLookupCache: " << m_nodeLookup.size()
               << " entries for template '" << Name << "'" << std::endl;
}

} // namespace Olympe
