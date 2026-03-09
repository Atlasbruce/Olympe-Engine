/**
 * @file GraphValidationPanel.cpp
 * @brief VS graph validation implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "GraphValidationPanel.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

GraphValidationPanel& GraphValidationPanel::Get()
{
    static GraphValidationPanel s_Instance;
    return s_Instance;
}

// ============================================================================
// Construction
// ============================================================================

GraphValidationPanel::GraphValidationPanel()
    : m_SelectedNodeId(-1)
{
}

// ============================================================================
// Helpers
// ============================================================================

void GraphValidationPanel::AddError(int nodeId,
                                    const std::string& message,
                                    ValidationSeverity severity)
{
    GraphValidationError e;
    e.nodeId   = nodeId;
    e.message  = message;
    e.severity = severity;
    m_Errors.push_back(e);
}

// ============================================================================
// Validate
// ============================================================================

void GraphValidationPanel::Validate(const TaskGraphTemplate& graph)
{
    Clear();

    if (graph.Nodes.empty())
    {
        AddError(-1, "Graph contains no nodes.", ValidationSeverity::Critical);
        return;
    }

    CheckDeadEnds(graph);
    CheckMissingConnections(graph);
    CheckCycles(graph);

    SYSTEM_LOG << "[GraphValidationPanel] Validation complete: "
               << static_cast<int>(m_Errors.size()) << " finding(s)."
               << std::endl;
}

// ============================================================================
// CheckDeadEnds
// ============================================================================

void GraphValidationPanel::CheckDeadEnds(const TaskGraphTemplate& graph)
{
    // Build set of node IDs that have at least one outgoing exec connection.
    std::unordered_set<int> hasExecOut;
    for (size_t i = 0; i < graph.ExecConnections.size(); ++i)
        hasExecOut.insert(static_cast<int>(graph.ExecConnections[i].SourceNodeID));

    // Pure data nodes are not expected to have exec outputs.
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = graph.Nodes[i];

        bool isDataOnly = (node.Type == TaskNodeType::GetBBValue
                        || node.Type == TaskNodeType::SetBBValue
                        || node.Type == TaskNodeType::MathOp);

        if (isDataOnly)
            continue;

        if (hasExecOut.find(static_cast<int>(node.NodeID)) == hasExecOut.end())
        {
            // EntryPoint with no output is critical; others are errors.
            ValidationSeverity sev = (node.Type == TaskNodeType::EntryPoint)
                ? ValidationSeverity::Critical
                : ValidationSeverity::Error;

            AddError(static_cast<int>(node.NodeID),
                     "Node '" + node.NodeName + "' has no outgoing exec connection (dead end).",
                     sev);
        }
    }
}

// ============================================================================
// CheckMissingConnections
// ============================================================================

void GraphValidationPanel::CheckMissingConnections(const TaskGraphTemplate& graph)
{
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = graph.Nodes[i];

        // SubGraph nodes must have a non-empty SubGraphPath.
        if (node.Type == TaskNodeType::SubGraph && node.SubGraphPath.empty())
        {
            AddError(static_cast<int>(node.NodeID),
                     "SubGraph node '" + node.NodeName + "' has no SubGraphPath set.",
                     ValidationSeverity::Error);
        }
    }

    // Check that EntryPoint exists.
    if (graph.EntryPointID == NODE_INDEX_NONE)
    {
        AddError(-1, "Graph has no EntryPoint node.", ValidationSeverity::Critical);
    }
}

// ============================================================================
// CheckCycles
// ============================================================================

void GraphValidationPanel::CheckCycles(const TaskGraphTemplate& graph)
{
    // Build adjacency list: nodeId -> list of target nodeIds
    std::unordered_map<int, std::vector<int>> adj;

    for (size_t i = 0; i < graph.Nodes.size(); ++i)
        adj[static_cast<int>(graph.Nodes[i].NodeID)] = std::vector<int>();

    for (size_t i = 0; i < graph.ExecConnections.size(); ++i)
    {
        int src = static_cast<int>(graph.ExecConnections[i].SourceNodeID);
        int dst = static_cast<int>(graph.ExecConnections[i].TargetNodeID);
        adj[src].push_back(dst);
    }

    // Iterative DFS with colour marking:
    //   0 = white (unvisited), 1 = grey (on stack), 2 = black (done)
    std::unordered_map<int, int> colour;
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
        colour[static_cast<int>(graph.Nodes[i].NodeID)] = 0;

    std::unordered_set<int> cycleReported;

    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        int startId = static_cast<int>(graph.Nodes[i].NodeID);

        if (colour[startId] != 0)
            continue;

        // DFS stack: pair<nodeId, index into adj[nodeId]>
        std::vector<std::pair<int,int>> stack;
        stack.push_back(std::make_pair(startId, 0));
        colour[startId] = 1;

        while (!stack.empty())
        {
            std::pair<int,int>& top  = stack.back();
            int                 node = top.first;
            int&                idx  = top.second;

            const std::vector<int>& neighbours = adj[node];

            if (idx < static_cast<int>(neighbours.size()))
            {
                int next = neighbours[idx];
                ++idx;

                if (colour.find(next) == colour.end())
                    continue; // unknown node — skip

                if (colour[next] == 1)
                {
                    // Back edge — cycle detected
                    if (cycleReported.find(next) == cycleReported.end())
                    {
                        cycleReported.insert(next);
                        AddError(next,
                                 "Cycle detected involving node ID " + std::to_string(next) + ".",
                                 ValidationSeverity::Critical);
                    }
                }
                else if (colour[next] == 0)
                {
                    colour[next] = 1;
                    stack.push_back(std::make_pair(next, 0));
                }
            }
            else
            {
                colour[node] = 2;
                stack.pop_back();
            }
        }
    }
}

// ============================================================================
// Accessors
// ============================================================================

const std::vector<GraphValidationError>& GraphValidationPanel::GetErrors() const
{
    return m_Errors;
}

bool GraphValidationPanel::HasErrors() const
{
    return !m_Errors.empty();
}

bool GraphValidationPanel::HasCriticalErrors() const
{
    for (size_t i = 0; i < m_Errors.size(); ++i)
    {
        if (m_Errors[i].severity == ValidationSeverity::Critical)
            return true;
    }
    return false;
}

void GraphValidationPanel::Clear()
{
    m_Errors.clear();
    m_SelectedNodeId = -1;
}

void GraphValidationPanel::OnErrorClick(int nodeId)
{
    m_SelectedNodeId = nodeId;
}

int GraphValidationPanel::GetSelectedNodeId() const
{
    return m_SelectedNodeId;
}

} // namespace Olympe
