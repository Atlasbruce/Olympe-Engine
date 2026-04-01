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
#include "AtomicTaskUIRegistry.h"
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"
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

    // Registry rules (Phase 22-C)
    CheckAtomicTaskIDs(graph, result);
    CheckConditionIDs(graph, result);
    CheckMathOperators(graph, result);
    CheckSubGraphPaths(graph, result);
    CheckConditionParams(graph, result);
    CheckBBKeyCompatibility(graph, result);

    // Condition structure rules (Phase 23-B.4)
    CheckConditionStructure(graph, result);

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

        // Phase 24.3 - CRITICAL: Skip data-pure nodes (GetBBValue, MathOp)
        // These nodes have no exec pins and are connected via data pins instead
        // They are NOT dangling — they are pure data computation nodes
        if (node.Type == TaskNodeType::GetBBValue || 
            node.Type == TaskNodeType::MathOp)
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
        if (node.Type == TaskNodeType::SubGraph)
        {
            // Check both SubGraphPath field AND Parameters["subgraph_path"]
            // (UI edits Parameters, but we sync to SubGraphPath only before save)
            std::string resolvedPath = node.SubGraphPath;

            // Fallback: check Parameters["subgraph_path"] if SubGraphPath is empty
            if (resolvedPath.empty())
            {
                auto it = node.Parameters.find("subgraph_path");
                if (it != node.Parameters.end() && 
                    it->second.Type == ParameterBindingType::Literal)
                {
                    resolvedPath = it->second.LiteralValue.to_string();
                }
            }

            if (resolvedPath.empty())
            {
                std::ostringstream oss;
                oss << "Node #" << node.NodeID << " ('" << node.NodeName
                    << "') is a SubGraph node with no SubGraphPath assigned.";
                AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                         "W003_EmptySubGraphPath",
                         oss.str());
            }
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

        // Phase 24.3 - CRITICAL: Skip data-pure nodes (GetBBValue, MathOp)
        // These nodes are connected via data pins, not exec connections
        // They don't need to be reachable from EntryPoint via exec flow
        if (node.Type == TaskNodeType::GetBBValue || node.Type == TaskNodeType::MathOp)
            continue;

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

// ============================================================================
// E020 — AtomicTask: taskType not registered in AtomicTaskUIRegistry
// ============================================================================

void VSGraphVerifier::CheckAtomicTaskIDs(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    const AtomicTaskUIRegistry& reg = AtomicTaskUIRegistry::Get();

    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (node.Type != TaskNodeType::AtomicTask)
            continue;

        if (node.AtomicTaskID.empty())
            continue; // W001 handles empty IDs separately

        if (reg.GetTaskSpec(node.AtomicTaskID) == nullptr)
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "'): AtomicTaskID '" << node.AtomicTaskID
                << "' is not registered in AtomicTaskUIRegistry (no display metadata).";
            AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                     "W005_UnknownAtomicTaskID", oss.str());
        }
    }
}

// ============================================================================
// E021 — Branch/While: conditionType not registered in ConditionRegistry
// ============================================================================

void VSGraphVerifier::CheckConditionIDs(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    const ConditionRegistry& reg = ConditionRegistry::Get();

    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (node.Type != TaskNodeType::Branch && node.Type != TaskNodeType::While)
            continue;

        if (node.ConditionID.empty())
            continue; // W003 handles empty condition IDs

        if (reg.GetConditionSpec(node.ConditionID) == nullptr)
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "'): ConditionID '" << node.ConditionID
                << "' is not registered in ConditionRegistry.";
            AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                     "E021_UnknownConditionID", oss.str());
        }
    }
}

// ============================================================================
// E023 — MathOp: operation is not a valid math operator
// ============================================================================

void VSGraphVerifier::CheckMathOperators(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (node.Type != TaskNodeType::MathOp)
            continue;

        if (node.MathOperator.empty())
            continue; // W004 handles empty MathOperator

        if (!OperatorRegistry::IsValidMathOperator(node.MathOperator))
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "'): MathOperator '" << node.MathOperator
                << "' is not a valid operator. Expected one of: +, -, *, /, %.";
            AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                     "E023_InvalidMathOperator", oss.str());
        }
    }
}

// ============================================================================
// E024 — SubGraph: subGraphPath is empty
// ============================================================================

void VSGraphVerifier::CheckSubGraphPaths(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (node.Type != TaskNodeType::SubGraph)
            continue;

        // Check both SubGraphPath field AND Parameters["subgraph_path"] for Path
        // (UI edits Parameters, but we sync to SubGraphPath only before save)
        std::string resolvedPath = node.SubGraphPath;

        SYSTEM_LOG << "[VSGraphVerifier::CheckSubGraphPaths] Node #" << node.NodeID 
                   << ": SubGraphPath='" << resolvedPath << "'\n";

        // Fallback: check Parameters["subgraph_path"] if SubGraphPath is empty
        if (resolvedPath.empty())
        {
            auto it = node.Parameters.find("subgraph_path");
            if (it != node.Parameters.end())
            {
                SYSTEM_LOG << "[VSGraphVerifier::CheckSubGraphPaths] Found Parameters[subgraph_path], Type=" 
                           << static_cast<int>(it->second.Type) << "\n";

                if (it->second.Type == ParameterBindingType::Literal)
                {
                    resolvedPath = it->second.LiteralValue.to_string();
                    SYSTEM_LOG << "[VSGraphVerifier::CheckSubGraphPaths] Resolved from Literal: '" 
                               << resolvedPath << "'\n";
                }
            }
            else
            {
                SYSTEM_LOG << "[VSGraphVerifier::CheckSubGraphPaths] No Parameters[subgraph_path] found\n";
            }
        }

        if (resolvedPath.empty())
        {
            std::ostringstream oss;
            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                << "'): SubGraphPath is empty. Set a valid .ats file path.";
            AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                     "E024_EmptySubGraphPath", oss.str());
        }
    }
}

// ============================================================================
// E025 — Condition: required parameter missing on Branch/While node
// ============================================================================

void VSGraphVerifier::CheckConditionParams(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    const ConditionRegistry& reg = ConditionRegistry::Get();

    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];
        if (node.Type != TaskNodeType::Branch && node.Type != TaskNodeType::While)
            continue;

        if (node.ConditionID.empty())
            continue;

        const ConditionSpec* spec = reg.GetConditionSpec(node.ConditionID);
        if (!spec)
            continue; // E021 already reported this

        for (size_t p = 0; p < spec->parameters.size(); ++p)
        {
            const ConditionParamSpec& param = spec->parameters[p];
            if (!param.required)
                continue;

            // Check if the parameter is present in the node's Parameters map
            auto it = node.Parameters.find(param.name);
            bool present = (it != node.Parameters.end());

            if (!present)
            {
                std::ostringstream oss;
                oss << "Node #" << node.NodeID << " ('" << node.NodeName
                    << "'): Required parameter '" << param.name
                    << "' is missing for condition '" << node.ConditionID << "'.";
                AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                         "E025_MissingConditionParam", oss.str());
            }
        }
    }
}

// ============================================================================
// W010 — BBKey type incompatible with node expectations
// ============================================================================

void VSGraphVerifier::CheckBBKeyCompatibility(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    // Build a map of BB key -> declared type for fast lookup
    std::unordered_map<std::string, VariableType> bbTypes;
    for (size_t i = 0; i < g.Blackboard.size(); ++i)
    {
        bbTypes[g.Blackboard[i].Key] = g.Blackboard[i].Type;
    }

    for (size_t i = 0; i < g.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& node = g.Nodes[i];

        // ForEach: bbKey should be a List type
        if (node.Type == TaskNodeType::ForEach && !node.BBKey.empty())
        {
            auto it = bbTypes.find(node.BBKey);
            if (it != bbTypes.end() && it->second != VariableType::List)
            {
                std::ostringstream oss;
                oss << "Node #" << node.NodeID << " ('" << node.NodeName
                    << "'): ForEach expects a List variable, but '"
                    << node.BBKey << "' is declared as "
                    << static_cast<int>(it->second) << ".";
                AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                         "W010_BBKeyTypeIncompatible", oss.str());
            }
        }
    }
}

// ============================================================================
// CheckConditionStructure (Phase 23-B.4)
// E040, E041, E042, W015, W016
// ============================================================================

void VSGraphVerifier::CheckConditionStructure(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    // Build BB key set for fast E041 lookup
    std::unordered_map<std::string, VariableType> bbTypes;
    for (size_t i = 0; i < g.Blackboard.size(); ++i)
        bbTypes[g.Blackboard[i].Key] = g.Blackboard[i].Type;

    // Build set of data connections (targetNode:pinName) for W016
    // We use a simple set represented as a sorted vector of strings.
    std::vector<std::string> dataConnectionKeys;
    for (size_t i = 0; i < g.DataConnections.size(); ++i)
    {
        std::ostringstream k;
        k << g.DataConnections[i].TargetNodeID << ":" << g.DataConnections[i].TargetPinName;
        dataConnectionKeys.push_back(k.str());
    }

    for (size_t ni = 0; ni < g.Nodes.size(); ++ni)
    {
        const TaskNodeDefinition& node = g.Nodes[ni];
        if (node.Type != TaskNodeType::Branch && node.Type != TaskNodeType::While)
            continue;
        if (node.conditions.empty())
            continue;

        for (size_t ci = 0; ci < node.conditions.size(); ++ci)
        {
            const Condition& cond = node.conditions[ci];
            const std::string condIdx = std::to_string(ci);

            // Helper lambda-equivalent: check one side (left or right)
            // We repeat the logic for left and right using a small inline helper.

            // Check LEFT side
            {
                const std::string& mode = cond.leftMode;
                const std::string& pin  = cond.leftPin;
                const std::string& var  = cond.leftVariable;

                if (mode == "Pin")
                {
                    if (pin.empty())
                    {
                        // E040: Pin mode but empty reference
                        std::ostringstream oss;
                        oss << "Node #" << node.NodeID << " ('" << node.NodeName
                            << "'): condition[" << condIdx
                            << "] left side: Pin mode selected but pin reference is empty.";
                        AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                                 "E040_ConditionPinEmpty", oss.str());
                    }
                    else
                    {
                        // W016: Pin mode but no DataConnection found for that pin
                        // Parse "Node#<id>.<pinName>" -> check DataConnections
                        // We check source side: is there a DataConnection whose
                        // source == "<id>:<pinName>"?
                        const std::string prefix = "Node#";
                        bool hasConnection = false;
                        if (pin.substr(0, prefix.size()) == prefix)
                        {
                            const std::string rest = pin.substr(prefix.size());
                            const size_t dotPos = rest.find('.');
                            if (dotPos != std::string::npos)
                            {
                                const std::string idStr = rest.substr(0, dotPos);
                                const std::string pName = rest.substr(dotPos + 1);
                                for (size_t dc = 0; dc < g.DataConnections.size(); ++dc)
                                {
                                    std::ostringstream key;
                                    key << g.DataConnections[dc].SourceNodeID
                                        << ":" << g.DataConnections[dc].SourcePinName;
                                    // Also check target side
                                    std::ostringstream tkey;
                                    tkey << idStr << ":" << pName;
                                    if (key.str() == tkey.str())
                                    {
                                        hasConnection = true;
                                        break;
                                    }
                                }
                            }
                        }
                        (void)hasConnection; // W016 is informational; only warn if no connection
                        // Note: we emit W016 only when no data connection is wired to that pin
                        if (!hasConnection)
                        {
                            std::ostringstream oss;
                            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                                << "'): condition[" << condIdx
                                << "] left side: Pin mode references '" << pin
                                << "' but no DataConnection is wired from that pin.";
                            AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                                     "W016_ConditionPinNotWired", oss.str());
                        }
                    }
                }
                else if (mode == "Variable")
                {
                    if (var.empty())
                    {
                        std::ostringstream oss;
                        oss << "Node #" << node.NodeID << " ('" << node.NodeName
                            << "'): condition[" << condIdx
                            << "] left side: Variable mode but variable name is empty (E041).";
                        AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                                 "E041_ConditionVariableNotFound", oss.str());
                    }
                    else if (bbTypes.find(var) == bbTypes.end())
                    {
                        // E041: variable not in blackboard
                        std::ostringstream oss;
                        oss << "Node #" << node.NodeID << " ('" << node.NodeName
                            << "'): condition[" << condIdx
                            << "] left side: Variable '" << var
                            << "' not declared in blackboard.";
                        AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                                 "E041_ConditionVariableNotFound", oss.str());
                    }
                }
                // Const mode: always valid — no check needed
            }

            // Check RIGHT side
            {
                const std::string& mode = cond.rightMode;
                const std::string& pin  = cond.rightPin;
                const std::string& var  = cond.rightVariable;

                if (mode == "Pin")
                {
                    if (pin.empty())
                    {
                        std::ostringstream oss;
                        oss << "Node #" << node.NodeID << " ('" << node.NodeName
                            << "'): condition[" << condIdx
                            << "] right side: Pin mode selected but pin reference is empty.";
                        AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                                 "E040_ConditionPinEmpty", oss.str());
                    }
                    else
                    {
                        const std::string prefix = "Node#";
                        bool hasConnection = false;
                        if (pin.substr(0, prefix.size()) == prefix)
                        {
                            const std::string rest = pin.substr(prefix.size());
                            const size_t dotPos = rest.find('.');
                            if (dotPos != std::string::npos)
                            {
                                const std::string idStr = rest.substr(0, dotPos);
                                const std::string pName = rest.substr(dotPos + 1);
                                for (size_t dc = 0; dc < g.DataConnections.size(); ++dc)
                                {
                                    std::ostringstream key;
                                    key << g.DataConnections[dc].SourceNodeID
                                        << ":" << g.DataConnections[dc].SourcePinName;
                                    std::ostringstream tkey;
                                    tkey << idStr << ":" << pName;
                                    if (key.str() == tkey.str())
                                    {
                                        hasConnection = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (!hasConnection)
                        {
                            std::ostringstream oss;
                            oss << "Node #" << node.NodeID << " ('" << node.NodeName
                                << "'): condition[" << condIdx
                                << "] right side: Pin mode references '" << pin
                                << "' but no DataConnection is wired from that pin.";
                            AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                                     "W016_ConditionPinNotWired", oss.str());
                        }
                    }
                }
                else if (mode == "Variable")
                {
                    if (var.empty())
                    {
                        std::ostringstream oss;
                        oss << "Node #" << node.NodeID << " ('" << node.NodeName
                            << "'): condition[" << condIdx
                            << "] right side: Variable mode but variable name is empty (E041).";
                        AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                                 "E041_ConditionVariableNotFound", oss.str());
                    }
                    else if (bbTypes.find(var) == bbTypes.end())
                    {
                        std::ostringstream oss;
                        oss << "Node #" << node.NodeID << " ('" << node.NodeName
                            << "'): condition[" << condIdx
                            << "] right side: Variable '" << var
                            << "' not declared in blackboard.";
                        AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                                 "E041_ConditionVariableNotFound", oss.str());
                    }
                }
            }

            // W015: Const vs Const — always true/false (optimisation hint)
            if (cond.leftMode == "Const" && cond.rightMode == "Const")
            {
                std::ostringstream oss;
                oss << "Node #" << node.NodeID << " ('" << node.NodeName
                    << "'): condition[" << condIdx
                    << "] is Const vs Const — this will always evaluate to the same"
                       " boolean value. Consider simplifying.";
                AddIssue(r, VSVerificationSeverity::Warning, node.NodeID,
                         "W015_ConstVsConstCondition", oss.str());
            }

            // E042: Type mismatch between operands (when both types are known)
            {
                VariableType leftType  = VariableType::None;
                VariableType rightType = VariableType::None;

                // Resolve left type
                if (cond.leftMode == "Variable")
                {
                    auto it = bbTypes.find(cond.leftVariable);
                    if (it != bbTypes.end()) leftType = it->second;
                }
                else if (cond.leftMode == "Const")
                {
                    leftType = cond.leftConstValue.GetType();
                }

                // Resolve right type
                if (cond.rightMode == "Variable")
                {
                    auto it = bbTypes.find(cond.rightVariable);
                    if (it != bbTypes.end()) rightType = it->second;
                }
                else if (cond.rightMode == "Const")
                {
                    rightType = cond.rightConstValue.GetType();
                }

                // Check for type mismatch (excluding None which means "unknown/pin")
                if (leftType  != VariableType::None &&
                    rightType != VariableType::None &&
                    leftType  != rightType)
                {
                    // Int↔Float promotion is allowed
                    const bool leftNumeric  = (leftType  == VariableType::Int  || leftType  == VariableType::Float);
                    const bool rightNumeric = (rightType == VariableType::Int  || rightType == VariableType::Float);
                    if (!(leftNumeric && rightNumeric))
                    {
                        std::ostringstream oss;
                        oss << "Node #" << node.NodeID << " ('" << node.NodeName
                            << "'): condition[" << condIdx
                            << "] type mismatch: left type="
                            << static_cast<int>(leftType)
                            << " right type=" << static_cast<int>(rightType)
                            << " (E042). Comparison will fail at runtime.";
                        AddIssue(r, VSVerificationSeverity::Error, node.NodeID,
                                 "E042_ConditionTypeMismatch", oss.str());
                    }
                }
            }
        }
    }
}

} // namespace Olympe
