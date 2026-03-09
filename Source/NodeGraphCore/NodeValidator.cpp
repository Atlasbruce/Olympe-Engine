/**
 * @file NodeValidator.cpp
 * @brief NodeValidator implementation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "NodeValidator.h"

#include <unordered_map>
#include <unordered_set>

namespace Olympe {

// ============================================================================
// Public — ValidateGraph
// ============================================================================

std::vector<ValidationMessage> NodeValidator::ValidateGraph(const TaskGraphTemplate* graph)
{
    std::vector<ValidationMessage> messages;

    if (!graph)
    {
        AddMessage(messages, -1, NVSeverity::Error,
                   "Graph pointer is null.",
                   "Ensure a valid graph is loaded before validation.");
        return messages;
    }

    CheckUnconnectedNodes(graph, messages);
    CheckMissingSubGraphPaths(graph, messages);
    CheckInfiniteLoops(graph, messages);

    return messages;
}

// ============================================================================
// Public — ValidateNode
// ============================================================================

std::vector<ValidationMessage> NodeValidator::ValidateNode(const TaskNodeDefinition* node)
{
    std::vector<ValidationMessage> messages;

    if (!node)
    {
        AddMessage(messages, -1, NVSeverity::Error,
                   "Node pointer is null.", "");
        return messages;
    }

    // SubGraph nodes must have a non-empty SubGraphPath
    if (node->Type == TaskNodeType::SubGraph && node->SubGraphPath.empty())
    {
        AddMessage(messages, static_cast<int>(node->NodeID), NVSeverity::Error,
                   "SubGraph node '" + node->NodeName + "' has no SubGraphPath.",
                   "Set the SubGraphPath field to a valid .json blueprint file.");
    }

    return messages;
}

// ============================================================================
// Private — CheckUnconnectedNodes
// ============================================================================

void NodeValidator::CheckUnconnectedNodes(const TaskGraphTemplate*        graph,
                                          std::vector<ValidationMessage>& messages)
{
    // Build a set of source-node IDs that appear in ExecConnections
    std::unordered_set<int32_t> connectedSources;
    for (size_t i = 0; i < graph->ExecConnections.size(); ++i)
        connectedSources.insert(graph->ExecConnections[i].SourceNodeID);

    for (size_t i = 0; i < graph->Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = graph->Nodes[i];

        // Skip entry-point and root node types — they have no output requirement
        if (node.Type == TaskNodeType::EntryPoint || node.Type == TaskNodeType::Root)
            continue;

        if (connectedSources.find(node.NodeID) == connectedSources.end())
        {
            AddMessage(messages, static_cast<int>(node.NodeID), NVSeverity::Warning,
                       "Node '" + node.NodeName + "' has no outgoing exec connection.",
                       "Connect the node's output pin or remove it if unused.");
        }
    }
}

// ============================================================================
// Private — CheckMissingSubGraphPaths
// ============================================================================

void NodeValidator::CheckMissingSubGraphPaths(const TaskGraphTemplate*        graph,
                                              std::vector<ValidationMessage>& messages)
{
    for (size_t i = 0; i < graph->Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = graph->Nodes[i];

        if (node.Type != TaskNodeType::SubGraph)
            continue;

        if (node.SubGraphPath.empty())
        {
            AddMessage(messages, static_cast<int>(node.NodeID), NVSeverity::Error,
                       "SubGraph node '" + node.NodeName + "' has an empty SubGraphPath.",
                       "Set the SubGraphPath field to a valid .json blueprint file.");
        }
    }
}

// ============================================================================
// Private — CheckInfiniteLoops (DFS cycle detection)
// ============================================================================

namespace {

/// DFS helper: returns true if a cycle is found starting from startId.
bool DFSHasCycle(int32_t                                               current,
                 const std::unordered_map<int32_t, std::vector<int32_t>>& adj,
                 std::unordered_set<int32_t>&                          visited,
                 std::unordered_set<int32_t>&                          inStack)
{
    visited.insert(current);
    inStack.insert(current);

    auto it = adj.find(current);
    if (it != adj.end())
    {
        const std::vector<int32_t>& neighbours = it->second;
        for (size_t i = 0; i < neighbours.size(); ++i)
        {
            int32_t next = neighbours[i];
            if (inStack.find(next) != inStack.end())
                return true;  // Back-edge = cycle
            if (visited.find(next) == visited.end())
            {
                if (DFSHasCycle(next, adj, visited, inStack))
                    return true;
            }
        }
    }

    inStack.erase(current);
    return false;
}

} // anonymous namespace

void NodeValidator::CheckInfiniteLoops(const TaskGraphTemplate*        graph,
                                       std::vector<ValidationMessage>& messages)
{
    // Build adjacency list from ExecConnections
    std::unordered_map<int32_t, std::vector<int32_t>> adj;
    for (size_t i = 0; i < graph->ExecConnections.size(); ++i)
    {
        const ExecPinConnection& conn = graph->ExecConnections[i];
        adj[conn.SourceNodeID].push_back(conn.TargetNodeID);
    }

    std::unordered_set<int32_t> visited;
    std::unordered_set<int32_t> inStack;

    for (size_t i = 0; i < graph->Nodes.size(); ++i)
    {
        int32_t nid = graph->Nodes[i].NodeID;
        if (visited.find(nid) == visited.end())
        {
            if (DFSHasCycle(nid, adj, visited, inStack))
            {
                AddMessage(messages, static_cast<int>(nid), NVSeverity::Error,
                           "Cycle detected in exec flow involving node '"
                               + graph->Nodes[i].NodeName + "'.",
                           "Break the circular connection to allow the graph to terminate.");
                // Report once per cycle; a full cycle-member report would require
                // additional path tracking and is deferred to future enhancements.
                break;
            }
        }
    }
}

// ============================================================================
// Utility
// ============================================================================

void NodeValidator::AddMessage(std::vector<ValidationMessage>& messages,
                               int                             nodeId,
                               NVSeverity                      severity,
                               const std::string&              msg,
                               const std::string&              hint)
{
    ValidationMessage m;
    m.nodeId   = nodeId;
    m.severity = severity;
    m.message  = msg;
    m.hint     = hint;
    messages.push_back(m);
}

} // namespace Olympe

