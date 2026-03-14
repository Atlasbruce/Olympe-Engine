/**
 * @file VSGraphVerifier.cpp
 * @brief Global graph verifier for ATS Visual Script graphs (Phase 21-A).
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * Implements 17 verification rules (E001-E012, W001-W004, I001) on a
 * TaskGraphTemplate. Fully stateless — no ImGui/ImNodes dependency.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VSGraphVerifier.h"
#include "../system/system_utils.h"

#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace Olympe {

// ============================================================================
// VSVerificationResult — helpers
// ============================================================================

bool VSVerificationResult::HasErrors() const
{
    for (size_t i = 0; i < issues.size(); ++i)
    {
        if (issues[i].severity == VSVerificationSeverity::Error)
            return true;
    }
    return false;
}

bool VSVerificationResult::HasWarnings() const
{
    for (size_t i = 0; i < issues.size(); ++i)
    {
        if (issues[i].severity == VSVerificationSeverity::Warning)
            return true;
    }
    return false;
}

bool VSVerificationResult::IsValid() const
{
    return !HasErrors();
}

// ============================================================================
// VSGraphVerifier — private helpers
// ============================================================================

void VSGraphVerifier::AddIssue(VSVerificationResult& r,
                                VSVerificationSeverity sev,
                                int nodeID,
                                const std::string& ruleID,
                                const std::string& message)
{
    VSVerificationIssue issue;
    issue.severity = sev;
    issue.nodeID   = nodeID;
    issue.ruleID   = ruleID;
    issue.message  = message;
    r.issues.push_back(issue);
}

// ============================================================================
// VSGraphVerifier::Verify
// ============================================================================

VSVerificationResult VSGraphVerifier::Verify(const TaskGraphTemplate& graph)
{
    SYSTEM_LOG << "[VSGraphVerifier] Starting verification of graph '"
               << graph.Name << "' (" << graph.Nodes.size() << " nodes)\n";

    VSVerificationResult result;

    // Structural rules
    CheckEntryPoint(graph, result);
    CheckDanglingNodes(graph, result);
    CheckExecCycles(graph, result);
    CheckSubGraphCircular(graph, result);

    // Type-safety rules
    CheckExecPinTypes(graph, result);
    CheckDataPinTypes(graph, result);
    CheckPinDirections(graph, result);

    // Blackboard rules
    CheckBlackboardKeys(graph, result);
    CheckBlackboardTypes(graph, result);

    // Switch rules (Phase 22-A)
    CheckSwitchNodes(graph, result);

    // Warning rules
    CheckNodeParameterWarnings(graph, result);

    // Info rules
    CheckReachability(graph, result);

    SYSTEM_LOG << "[VSGraphVerifier] Verification complete: "
               << result.issues.size() << " issue(s) found"
               << " (errors=" << (result.HasErrors() ? "yes" : "no")
               << " warnings=" << (result.HasWarnings() ? "yes" : "no") << ")\n";

    return result;
}

// ============================================================================
// E001 — Exactly one EntryPoint node required
// ============================================================================

void VSGraphVerifier::CheckEntryPoint(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    int count = 0;
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        if (g.Nodes[i].Type == TaskNodeType::EntryPoint)
            ++count;
    }

    if (count == 0)
    {
        AddIssue(r, VSVerificationSeverity::Error, -1,
                 "E001_NoEntryPoint",
                 "Graph has no EntryPoint node. Exactly one EntryPoint is required.");
    }
    else if (count > 1)
    {
        std::ostringstream oss;
        oss << "Graph has " << count
            << " EntryPoint nodes. Exactly one EntryPoint is required.";
        AddIssue(r, VSVerificationSeverity::Error, -1,
                 "E001_MultipleEntryPoints",
                 oss.str());
    }
}

// ============================================================================
// E002 — Dangling node (no exec in or out, except EntryPoint)
// ============================================================================

void VSGraphVerifier::CheckDanglingNodes(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];

        // EntryPoint has no exec-in by design — skip
        if (node.Type == TaskNodeType::EntryPoint)
            continue;

        bool hasExecIn  = false;
        bool hasExecOut = false;

        for (size_t j = 0; j < g.ExecConnections.size(); ++j)
        {
            const ExecPinConnection& c = g.ExecConnections[j];
            if (c.TargetNodeID == node.NodeID)
                hasExecIn = true;
            if (c.SourceNodeID == node.NodeID)
                hasExecOut = true;
            if (hasExecIn && hasExecOut)
                break;
        }

        if (!hasExecIn && !hasExecOut)
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') has no exec connections (dangling node).";
            AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                     "E002_DanglingNode",
                     oss.str());
        }
    }
}

// ============================================================================
// E003 — Exec cycle detected (iterative DFS on adjacency list)
// ============================================================================

void VSGraphVerifier::CheckExecCycles(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    // Build adjacency list: nodeID -> list of successor nodeIDs
    std::map<int, std::vector<int> > adj;
    for (size_t i = 0; i < g.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& c = g.ExecConnections[i];
        adj[c.SourceNodeID].push_back(c.TargetNodeID);
    }

    // Collect all node IDs that appear in ExecConnections
    std::unordered_set<int> nodeIDs;
    for (size_t i = 0; i < g.ExecConnections.size(); ++i)
    {
        nodeIDs.insert(g.ExecConnections[i].SourceNodeID);
        nodeIDs.insert(g.ExecConnections[i].TargetNodeID);
    }

    // Run iterative DFS cycle detection from each unvisited node
    // visited: permanently finished; inStack: currently in DFS path
    std::unordered_set<int> visited;
    bool cycleReported = false;

    for (std::unordered_set<int>::const_iterator startIt = nodeIDs.begin();
         startIt != nodeIDs.end() && !cycleReported; ++startIt)
    {
        int startNode = *startIt;
        if (visited.count(startNode) > 0)
            continue;

        // Iterative DFS using explicit stack of (node, iterator-into-adj)
        // We simulate the recursive call stack.
        std::vector<int> path;
        std::unordered_set<int> inStack;
        std::vector<std::pair<int, size_t> > dfsStack;

        dfsStack.push_back(std::make_pair(startNode, static_cast<size_t>(0)));
        inStack.insert(startNode);
        path.push_back(startNode);

        while (!dfsStack.empty() && !cycleReported)
        {
            std::pair<int, size_t>& top = dfsStack.back();
            int cur  = top.first;
            size_t&  idx = top.second;

            std::map<int, std::vector<int> >::const_iterator adjIt = adj.find(cur);

            if (adjIt != adj.end() && idx < adjIt->second.size())
            {
                int next = adjIt->second[idx];
                ++idx;

                if (inStack.count(next) > 0)
                {
                    // Cycle found
                    std::ostringstream oss;
                    oss << "Exec cycle detected involving node #" << next << ".";
                    AddIssue(r, VSVerificationSeverity::Error, next,
                             "E003_ExecCycle",
                             oss.str());
                    cycleReported = true;
                }
                else if (visited.count(next) == 0)
                {
                    inStack.insert(next);
                    path.push_back(next);
                    dfsStack.push_back(std::make_pair(next, static_cast<size_t>(0)));
                }
            }
            else
            {
                // Finished with cur
                visited.insert(cur);
                inStack.erase(cur);
                dfsStack.pop_back();
                if (!path.empty())
                    path.pop_back();
            }
        }

        // Mark all nodes in this DFS tree as visited
        for (size_t p = 0; p < path.size(); ++p)
            visited.insert(path[p]);
    }
}

// ============================================================================
// E004 — Circular SubGraph reference (self-reference check)
// ============================================================================

void VSGraphVerifier::CheckSubGraphCircular(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (node.Type != TaskNodeType::SubGraph)
            continue;
        if (node.SubGraphPath.empty())
            continue;

        // Simple self-reference check: SubGraphPath matches graph name
        if (node.SubGraphPath == g.Name)
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') references itself as a SubGraph (SubGraphPath == graph.Name '"
                << g.Name << "'). This would cause infinite recursion.";
            AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                     "E004_CircularSubGraph",
                     oss.str());
        }
    }
}

// ============================================================================
// E005 — Exec connection references unknown node
// ============================================================================

void VSGraphVerifier::CheckExecPinTypes(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    // Build fast lookup set of valid node IDs
    std::unordered_set<int> validIDs;
    for (size_t i = 0; i < g.Nodes.size(); ++i)
        validIDs.insert(g.Nodes[i].NodeID);

    for (size_t i = 0; i < g.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& c = g.ExecConnections[i];

        if (validIDs.count(c.SourceNodeID) == 0)
        {
            std::ostringstream oss;
            oss << "Exec connection references unknown source node #" << c.SourceNodeID << ".";
            AddIssue(r, VSVerificationSeverity::Error, c.SourceNodeID,
                     "E005_UnknownExecSourceNode",
                     oss.str());
        }

        if (validIDs.count(c.TargetNodeID) == 0)
        {
            std::ostringstream oss;
            oss << "Exec connection references unknown target node #" << c.TargetNodeID << ".";
            AddIssue(r, VSVerificationSeverity::Error, c.TargetNodeID,
                     "E005_UnknownExecTargetNode",
                     oss.str());
        }
    }
}

// ============================================================================
// E006 — Incompatible data pin types
// ============================================================================

void VSGraphVerifier::CheckDataPinTypes(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.DataConnections.size(); ++i)
    {
        const DataPinConnection& c = g.DataConnections[i];

        // Find source node and pin
        const DataPinDefinition* srcPin = NULL;
        const DataPinDefinition* dstPin = NULL;

        for (size_t ni = 0; ni < g.Nodes.size(); ++ni)
        {
            const TaskNodeDefinition& node = g.Nodes[ni];
            if (node.NodeID == c.SourceNodeID)
            {
                for (size_t pi = 0; pi < node.DataPins.size(); ++pi)
                {
                    if (node.DataPins[pi].PinName == c.SourcePinName)
                    {
                        srcPin = &node.DataPins[pi];
                        break;
                    }
                }
            }
            if (node.NodeID == c.TargetNodeID)
            {
                for (size_t pi = 0; pi < node.DataPins.size(); ++pi)
                {
                    if (node.DataPins[pi].PinName == c.TargetPinName)
                    {
                        dstPin = &node.DataPins[pi];
                        break;
                    }
                }
            }
        }

        if (srcPin == NULL || dstPin == NULL)
            continue; // Can't validate without pin definitions

        // VariableType::None is treated as "any" — skip type check
        if (srcPin->PinType == VariableType::None || dstPin->PinType == VariableType::None)
            continue;

        if (srcPin->PinType != dstPin->PinType)
        {
            std::ostringstream oss;
            oss << "Data pin type mismatch: node #" << c.SourceNodeID
                << " pin '" << c.SourcePinName << "' -> node #" << c.TargetNodeID
                << " pin '" << c.TargetPinName << "'. Types are incompatible.";
            AddIssue(r, VSVerificationSeverity::Error, c.SourceNodeID,
                     "E006_DataPinTypeMismatch",
                     oss.str());
        }
    }
}

// ============================================================================
// E007 — Inverted pin direction
// ============================================================================

void VSGraphVerifier::CheckPinDirections(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.DataConnections.size(); ++i)
    {
        const DataPinConnection& c = g.DataConnections[i];

        const DataPinDefinition* srcPin = NULL;
        const DataPinDefinition* dstPin = NULL;

        for (size_t ni = 0; ni < g.Nodes.size(); ++ni)
        {
            const TaskNodeDefinition& node = g.Nodes[ni];
            if (node.NodeID == c.SourceNodeID)
            {
                for (size_t pi = 0; pi < node.DataPins.size(); ++pi)
                {
                    if (node.DataPins[pi].PinName == c.SourcePinName)
                    {
                        srcPin = &node.DataPins[pi];
                        break;
                    }
                }
            }
            if (node.NodeID == c.TargetNodeID)
            {
                for (size_t pi = 0; pi < node.DataPins.size(); ++pi)
                {
                    if (node.DataPins[pi].PinName == c.TargetPinName)
                    {
                        dstPin = &node.DataPins[pi];
                        break;
                    }
                }
            }
        }

        if (srcPin == NULL || dstPin == NULL)
            continue;

        if (srcPin->Dir != DataPinDir::Output)
        {
            std::ostringstream oss;
            oss << "Data connection from node #" << c.SourceNodeID
                << " pin '" << c.SourcePinName
                << "': source pin must be Output direction.";
            AddIssue(r, VSVerificationSeverity::Error, c.SourceNodeID,
                     "E007_InvertedPinDirection",
                     oss.str());
        }

        if (dstPin->Dir != DataPinDir::Input)
        {
            std::ostringstream oss;
            oss << "Data connection to node #" << c.TargetNodeID
                << " pin '" << c.TargetPinName
                << "': destination pin must be Input direction.";
            AddIssue(r, VSVerificationSeverity::Error, c.TargetNodeID,
                     "E007_InvertedPinDirection",
                     oss.str());
        }
    }
}

// ============================================================================
// E008 — Unknown Blackboard key in GetBBValue/SetBBValue
// ============================================================================

void VSGraphVerifier::CheckBlackboardKeys(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    // Skip if no blackboard schema declared
    if (g.Blackboard.empty())
        return;

    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];

        if (node.Type != TaskNodeType::GetBBValue && node.Type != TaskNodeType::SetBBValue)
            continue;

        if (node.BBKey.empty())
            continue; // Will be caught by W001-style rules if needed

        bool found = false;
        for (size_t j = 0; j < g.Blackboard.size(); ++j)
        {
            if (g.Blackboard[j].Key == node.BBKey)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') references unknown blackboard key '" << node.BBKey << "'.";
            AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                     "E008_UnknownBBKey",
                     oss.str());
        }
    }
}

// ============================================================================
// E009 — Blackboard type mismatch
// ============================================================================

void VSGraphVerifier::CheckBlackboardTypes(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    if (g.Blackboard.empty())
        return;

    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];

        if (node.Type != TaskNodeType::GetBBValue && node.Type != TaskNodeType::SetBBValue)
            continue;

        if (node.BBKey.empty())
            continue;

        // Find the BB entry
        const BlackboardEntry* entry = NULL;
        for (size_t j = 0; j < g.Blackboard.size(); ++j)
        {
            if (g.Blackboard[j].Key == node.BBKey)
            {
                entry = &g.Blackboard[j];
                break;
            }
        }

        if (entry == NULL)
            continue; // Unknown key already reported by E008

        // Find the data pin on the node (first data pin with a declared type)
        for (size_t pi = 0; pi < node.DataPins.size(); ++pi)
        {
            const DataPinDefinition& pin = node.DataPins[pi];
            if (pin.PinType == VariableType::None)
                continue;

            if (pin.PinType != entry->Type)
            {
                std::ostringstream oss;
                oss << "Node #" << node.NodeID << " ('" << node.NodeName
                    << "') data pin '" << pin.PinName
                    << "' type does not match blackboard key '"
                    << node.BBKey << "' declared type.";
                AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                         "E009_BBTypeMismatch",
                         oss.str());
                break; // Report once per node
            }
        }
    }
}

// ============================================================================
// Warning rules — W001–W004
// ============================================================================

// ============================================================================
// E010–E012 — Switch node validation (Phase 22-A)
// ============================================================================

void VSGraphVerifier::CheckSwitchNodes(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (node.Type != TaskNodeType::Switch)
            continue;

        // E010 — Switch node missing switchVariable
        if (node.switchVariable.empty())
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') is a Switch node with no switchVariable assigned."
                   " Assign a Blackboard key to switch on.";
            AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                     "E010_SwitchMissingVariable",
                     oss.str());
        }

        // E011 — duplicate case values
        std::unordered_map<std::string, size_t> seenValues;
        for (size_t ci = 0; ci < node.switchCases.size(); ++ci)
        {
            const SwitchCaseDefinition& sc = node.switchCases[ci];
            if (sc.value.empty())
                continue;
            auto it = seenValues.find(sc.value);
            if (it != seenValues.end())
            {
                std::ostringstream oss;
                oss << "Node #" << node.NodeID << " ('" << node.NodeName
                    << "') has duplicate switch case value '" << sc.value
                    << "' at index " << ci
                    << " (first seen at index " << it->second << ").";
                AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                         "E011_SwitchDuplicateCaseValue",
                         oss.str());
            }
            else
            {
                seenValues[sc.value] = ci;
            }
        }

        // E012 — case with empty pin name
        for (size_t ci = 0; ci < node.switchCases.size(); ++ci)
        {
            const SwitchCaseDefinition& sc = node.switchCases[ci];
            if (sc.pinName.empty())
            {
                std::ostringstream oss;
                oss << "Node #" << node.NodeID << " ('" << node.NodeName
                    << "') has a switch case at index " << ci
                    << " with an empty pin name.";
                AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                         "E012_SwitchEmptyPinName",
                         oss.str());
            }
        }
    }
}

void VSGraphVerifier::CheckNodeParameterWarnings(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];

        // W001 — AtomicTask with empty AtomicTaskID
        if (node.Type == TaskNodeType::AtomicTask && node.AtomicTaskID.empty())
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') is an AtomicTask with no AtomicTaskID assigned.";
            AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                     "W001_EmptyAtomicTaskID",
                     oss.str());
        }

        // W002 — Delay with DelaySeconds <= 0
        if (node.Type == TaskNodeType::Delay && node.DelaySeconds <= 0.0f)
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') Delay node has DelaySeconds=" << node.DelaySeconds
                << " (<= 0). The delay will complete immediately.";
            AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                     "W002_NonPositiveDelay",
                     oss.str());
        }

        // W003 — SubGraph with empty SubGraphPath
        if (node.Type == TaskNodeType::SubGraph && node.SubGraphPath.empty())
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') is a SubGraph node with no SubGraphPath assigned.";
            AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                     "W003_EmptySubGraphPath",
                     oss.str());
        }

        // W004 — MathOp with empty MathOperator
        if (node.Type == TaskNodeType::MathOp && node.MathOperator.empty())
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') is a MathOp node with no MathOperator assigned.";
            AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                     "W004_EmptyMathOperator",
                     oss.str());
        }
    }
}

// ============================================================================
// I001 — Node not reachable from EntryPoint
// ============================================================================

void VSGraphVerifier::CheckReachability(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    // Find the EntryPoint node ID
    int entryID = -1;
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        if (g.Nodes[i].Type == TaskNodeType::EntryPoint)
        {
            entryID = g.Nodes[i].NodeID;
            break;
        }
    }

    // No EntryPoint — cannot do reachability (E001 already reported this)
    if (entryID == -1)
        return;

    // Build adjacency list from exec connections
    std::map<int, std::vector<int> > adj;
    for (size_t i = 0; i < g.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& c = g.ExecConnections[i];
        adj[c.SourceNodeID].push_back(c.TargetNodeID);
    }

    // BFS/DFS from entryID
    std::unordered_set<int> visited;
    std::vector<int> stack;
    stack.push_back(entryID);

    while (!stack.empty())
    {
        int cur = stack.back();
        stack.pop_back();

        if (visited.count(cur) > 0)
            continue;
        visited.insert(cur);

        std::map<int, std::vector<int> >::const_iterator it = adj.find(cur);
        if (it != adj.end())
        {
            for (size_t j = 0; j < it->second.size(); ++j)
                stack.push_back(it->second[j]);
        }
    }

    // Any node not visited is unreachable
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (visited.count(node.NodeID) == 0)
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "') is not reachable from the EntryPoint.";
            AddIssue(r, VSVerificationSeverity::Info, node.NodeID,
                     "I001_UnreachableNode",
                     oss.str());
        }
    }
}

} // namespace Olympe
