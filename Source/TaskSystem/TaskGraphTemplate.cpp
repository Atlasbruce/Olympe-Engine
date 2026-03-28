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
    // EXCEPTION: Allow RootNodeID = -1 for legacy/template files (schema v2)
    // These can still be loaded and edited, just not executed
    if (RootNodeID != -1 && validIds.find(RootNodeID) == validIds.end())
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

// ============================================================================
// Phase 24.3 - Poka-Yoke: Sanitize exec connections
// ============================================================================

int TaskGraphTemplate::SanitizeExecConnections()
{
    int removedCount = 0;

    // Helper: determine if a node type is data-pure (no exec pins)
    auto IsDataPureNode = [](TaskNodeType type) -> bool {
        return type == TaskNodeType::GetBBValue ||
               type == TaskNodeType::MathOp;
    };

    // Helper: find node by ID
    auto FindNode = [this](int32_t nodeID) -> TaskNodeDefinition* {
        for (size_t i = 0; i < Nodes.size(); ++i)
        {
            if (Nodes[i].NodeID == nodeID)
                return &Nodes[i];
        }
        return nullptr;
    };

    // Scan ExecConnections and remove invalid ones
    std::vector<ExecPinConnection> validConnections;

    for (size_t i = 0; i < ExecConnections.size(); ++i)
    {
        const ExecPinConnection& conn = ExecConnections[i];

        TaskNodeDefinition* srcNode = FindNode(conn.SourceNodeID);
        TaskNodeDefinition* dstNode = FindNode(conn.TargetNodeID);

        // Check 1: Source node must exist
        if (srcNode == nullptr)
        {
            SYSTEM_LOG << "[TaskGraphTemplate] SanitizeExecConnections: Removing connection with invalid source node #"
                       << conn.SourceNodeID << "\n";
            ++removedCount;
            continue;
        }

        // Check 2: Destination node must exist
        if (dstNode == nullptr)
        {
            SYSTEM_LOG << "[TaskGraphTemplate] SanitizeExecConnections: Removing connection with invalid destination node #"
                       << conn.TargetNodeID << "\n";
            ++removedCount;
            continue;
        }

        // Check 3: Source node cannot be data-pure (data-pure nodes have no exec-out)
        if (IsDataPureNode(srcNode->Type))
        {
            SYSTEM_LOG << "[TaskGraphTemplate] SanitizeExecConnections: Removing exec-out connection from data-pure node #"
                       << conn.SourceNodeID << " (type=" << static_cast<int>(srcNode->Type)
                       << "." << conn.SourcePinName << " -> node #" << conn.TargetNodeID << ")\n";
            ++removedCount;
            continue;
        }

        // Check 4: Destination node cannot be data-pure (data-pure nodes have no exec-in)
        if (IsDataPureNode(dstNode->Type))
        {
            SYSTEM_LOG << "[TaskGraphTemplate] SanitizeExecConnections: Removing exec-in connection to data-pure node #"
                       << conn.TargetNodeID << " (type=" << static_cast<int>(dstNode->Type)
                       << " <- node #" << conn.SourceNodeID << "." << conn.SourcePinName << ")\n";
            ++removedCount;
            continue;
        }

        // Connection is valid, keep it
        validConnections.push_back(conn);
    }

    // Replace with sanitized list
    ExecConnections = validConnections;

    if (removedCount > 0)
    {
        SYSTEM_LOG << "[TaskGraphTemplate] SanitizeExecConnections: Removed " << removedCount
                   << " invalid exec connection(s) - graph is now clean\n";
    }

    return removedCount;
}

} // namespace Olympe
