/**
 * @file GraphExecutionSimulator.cpp
 * @brief Implementation of graph execution simulator.
 * @author Olympe Engine
 * @date 2026-03-24
 */

#include "GraphExecutionSimulator.h"
#include "BlueprintValidator.h"
#include "../TaskSystem/TaskGraphTypes.h"
#include "../system/system_utils.h"
#include <algorithm>
#include <set>

namespace Olympe {

GraphExecutionSimulator::GraphExecutionSimulator()
{
}

GraphExecutionSimulator::~GraphExecutionSimulator()
{
}

std::vector<ValidationError> GraphExecutionSimulator::SimulateExecution(const TaskGraphTemplate& tmpl,
                                                                       const SimulationOptions& options,
                                                                       GraphExecutionTracer& outTracer)
{
    std::vector<ValidationError> errors;
    outTracer.Reset();

    // Initialize blackboard
    LocalBlackboard blackboard;
    if (!options.initialBlackboardJson.empty())
    {
        // TODO: Parse JSON and initialize blackboard
    }

    // Get entry point
    if (tmpl.EntryPointID < 0)
    {
        errors.push_back(ValidationError(-1, "", "No entry point defined", 
                                        ErrorSeverity::Critical, "Simulation"));
        outTracer.RecordError(-1, "", "No entry point defined", "Critical");
        outTracer.RecordExecutionCompleted(false, "Graph has no entry point");
        return errors;
    }

    // Simulate execution
    int32_t currentNodeId = tmpl.EntryPointID;
    m_visitCount.clear();
    m_pathStack.clear();
    int32_t stepCount = 0;

    outTracer.RecordNodeEntered(currentNodeId, "EntryPoint", "EntryPoint");

    while (currentNodeId != NODE_INDEX_NONE && stepCount < options.maxStepsPerFrame)
    {
        ++stepCount;

        // Get node definition
        const TaskNodeDefinition* nodeDef = tmpl.GetNode(currentNodeId);
        if (!nodeDef)
        {
            std::string msg = "Node " + std::to_string(currentNodeId) + " not found";
            errors.push_back(ValidationError(currentNodeId, "", msg, 
                                            ErrorSeverity::Error, "Simulation"));
            outTracer.RecordError(currentNodeId, "", msg);
            break;
        }

        // Track visit count for cycle detection
        m_visitCount[currentNodeId]++;
        if (m_visitCount[currentNodeId] > 2)
        {
            std::string msg = "Potential infinite loop detected at node " + 
                            (nodeDef->NodeName.empty() ? std::to_string(currentNodeId) : nodeDef->NodeName);
            errors.push_back(ValidationError(currentNodeId, nodeDef->NodeName, msg,
                                            ErrorSeverity::Warning, "Logic"));
            outTracer.RecordError(currentNodeId, nodeDef->NodeName, msg, "Warning");
            break;
        }

        // Simulate node execution
        int32_t nextNodeId = NODE_INDEX_NONE;

        switch (nodeDef->Type)
        {
            case TaskNodeType::EntryPoint:
            {
                outTracer.RecordNodeEntered(currentNodeId, "EntryPoint", "EntryPoint");
                // EntryPoint always goes to the "Then" pin
                nextNodeId = GetNextNodeId(tmpl, currentNodeId, "Then");
                if (nextNodeId == NODE_INDEX_NONE)
                {
                    outTracer.RecordExecutionBlocked(currentNodeId, "EntryPoint has no outgoing connection");
                    errors.push_back(ValidationError(currentNodeId, "EntryPoint",
                                                    "EntryPoint has no outgoing execution link",
                                                    ErrorSeverity::Error, "Link"));
                }
                break;
            }

            case TaskNodeType::Branch:
                nextNodeId = HandleBranchSimulation(tmpl, currentNodeId, blackboard, outTracer);
                break;

            case TaskNodeType::Switch:
                nextNodeId = HandleSwitchSimulation(tmpl, currentNodeId, blackboard, outTracer);
                break;

            case TaskNodeType::VSSequence:
                nextNodeId = HandleSequenceSimulation(tmpl, currentNodeId, blackboard, outTracer);
                break;

            case TaskNodeType::While:
                nextNodeId = HandleWhileSimulation(tmpl, currentNodeId, blackboard, outTracer);
                break;

            case TaskNodeType::AtomicTask:
            {
                outTracer.RecordNodeEntered(currentNodeId, nodeDef->NodeName, "AtomicTask");
                nextNodeId = GetNextNodeId(tmpl, currentNodeId, "Completed");
                if (nextNodeId == NODE_INDEX_NONE)
                {
                    outTracer.RecordExecutionBlocked(currentNodeId, "No outgoing link from AtomicTask");
                }
                break;
            }

            case TaskNodeType::GetBBValue:
            case TaskNodeType::SetBBValue:
            case TaskNodeType::MathOp:
            {
                outTracer.RecordNodeEntered(currentNodeId, nodeDef->NodeName, "DataNode");

                // Trace all incoming data pins recursively
                m_tracedDataNodes.clear();
                TraceDataPinEvaluation(currentNodeId, tmpl, outTracer);

                nextNodeId = GetNextNodeId(tmpl, currentNodeId, "Then");
                break;
            }

            case TaskNodeType::Delay:
            {
                outTracer.RecordNodeEntered(currentNodeId, nodeDef->NodeName, "Delay");
                nextNodeId = GetNextNodeId(tmpl, currentNodeId, "Completed");
                break;
            }

            case TaskNodeType::DoOnce:
            {
                outTracer.RecordNodeEntered(currentNodeId, nodeDef->NodeName, "DoOnce");
                nextNodeId = GetNextNodeId(tmpl, currentNodeId, "Completed");
                break;
            }

            case TaskNodeType::SubGraph:
            {
                outTracer.RecordNodeEntered(currentNodeId, nodeDef->NodeName, "SubGraph");

                // Get SubGraphPath from either field or Parameters
                std::string subGraphPath = nodeDef->SubGraphPath;
                if (subGraphPath.empty())
                {
                    auto it = nodeDef->Parameters.find("subgraph_path");
                    if (it != nodeDef->Parameters.end() &&
                        it->second.Type == ParameterBindingType::Literal)
                    {
                        subGraphPath = it->second.LiteralValue.to_string();
                    }
                }

                if (subGraphPath.empty())
                {
                    outTracer.RecordError(currentNodeId, nodeDef->NodeName, "SubGraph path is empty");
                }

                nextNodeId = GetNextNodeId(tmpl, currentNodeId, "Completed");
                break;
            }

            default:
                nextNodeId = NODE_INDEX_NONE;
                break;
        }

        currentNodeId = nextNodeId;
    }

    if (stepCount >= options.maxStepsPerFrame)
    {
        std::string msg = "Maximum simulation steps exceeded - possible infinite loop";
        errors.push_back(ValidationError(-1, "", msg, ErrorSeverity::Warning, "Logic"));
        outTracer.RecordError(-1, "", msg, "Warning");
    }

    // Perform additional validation checks
    if (options.validateBranchPaths)
    {
        ValidateAllBranches(tmpl, errors);
    }

    if (options.validateDataFlow)
    {
        ValidateDataConnections(tmpl, errors);
    }

    // Check for unreachable nodes
    std::vector<int32_t> unreachable = FindUnreachableNodes(tmpl, errors);

    outTracer.RecordExecutionCompleted(errors.empty(), 
        std::to_string(stepCount) + " steps executed, " + 
        std::to_string(errors.size()) + " validation issues found");

    return errors;
}

int32_t GraphExecutionSimulator::SimulateStep(const TaskGraphTemplate& tmpl,
                                             int32_t currentNodeId,
                                             LocalBlackboard& blackboard,
                                             const SimulationOptions& options,
                                             GraphExecutionTracer& tracer)
{
    const TaskNodeDefinition* node = tmpl.GetNode(currentNodeId);
    if (!node)
        return NODE_INDEX_NONE;

    tracer.RecordNodeEntered(currentNodeId, node->NodeName, "");
    return NODE_INDEX_NONE;
}

int32_t GraphExecutionSimulator::HandleBranchSimulation(const TaskGraphTemplate& tmpl,
                                                       int32_t nodeId,
                                                       LocalBlackboard& blackboard,
                                                       GraphExecutionTracer& tracer)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeId);
    if (!node)
        return NODE_INDEX_NONE;

    tracer.RecordNodeEntered(nodeId, node->NodeName, "Branch");

    // For simulation, we'll try the true branch by default
    // In a real scenario, we'd evaluate the condition
    bool conditionResult = true;
    tracer.RecordConditionEvaluated(nodeId, "condition", conditionResult);

    if (conditionResult)
    {
        tracer.RecordBranchTaken(nodeId, "True", -1);
        return GetNextNodeId(tmpl, nodeId, "Then");
    }
    else
    {
        tracer.RecordBranchTaken(nodeId, "False", -1);
        return GetNextNodeId(tmpl, nodeId, "Else");
    }
}

int32_t GraphExecutionSimulator::HandleSwitchSimulation(const TaskGraphTemplate& tmpl,
                                                       int32_t nodeId,
                                                       LocalBlackboard& blackboard,
                                                       GraphExecutionTracer& tracer)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeId);
    if (!node)
        return NODE_INDEX_NONE;

    tracer.RecordNodeEntered(nodeId, node->NodeName, "Switch");

    // Get the default (first) case
    return GetNextNodeId(tmpl, nodeId, "Default");
}

int32_t GraphExecutionSimulator::HandleSequenceSimulation(const TaskGraphTemplate& tmpl,
                                                         int32_t nodeId,
                                                         LocalBlackboard& blackboard,
                                                         GraphExecutionTracer& tracer)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeId);
    if (!node)
        return NODE_INDEX_NONE;

    tracer.RecordNodeEntered(nodeId, node->NodeName, "VSSequence");
    return GetNextNodeId(tmpl, nodeId, "Then");
}

int32_t GraphExecutionSimulator::HandleWhileSimulation(const TaskGraphTemplate& tmpl,
                                                      int32_t nodeId,
                                                      LocalBlackboard& blackboard,
                                                      GraphExecutionTracer& tracer)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeId);
    if (!node)
        return NODE_INDEX_NONE;

    tracer.RecordNodeEntered(nodeId, node->NodeName, "While");
    tracer.RecordBranchTaken(nodeId, "Loop", -1);

    return GetNextNodeId(tmpl, nodeId, "Loop");
}

int32_t GraphExecutionSimulator::GetNextNodeId(const TaskGraphTemplate& tmpl,
                                              int32_t nodeId,
                                              const std::string& pinName)
{
    // Find execution link from nodeId with pin name
    const TaskNodeDefinition* node = tmpl.GetNode(nodeId);
    if (!node)
        return NODE_INDEX_NONE;

    for (const auto& link : tmpl.ExecConnections)
    {
        if (link.SourceNodeID == nodeId && link.SourcePinName == pinName)
        {
            return link.TargetNodeID;
        }
    }

    return NODE_INDEX_NONE;
}

bool GraphExecutionSimulator::ValidateAllBranches(const TaskGraphTemplate& tmpl,
                                                  std::vector<ValidationError>& outErrors)
{
    bool allValid = true;

    for (const auto& node : tmpl.Nodes)
    {
        if (node.Type == TaskNodeType::Branch)
        {
            // Check that both "Then" and "Else" pins have outgoing connections
            int32_t thenTarget = GetNextNodeId(tmpl, node.NodeID, "Then");
            int32_t elseTarget = GetNextNodeId(tmpl, node.NodeID, "Else");

            if (thenTarget == NODE_INDEX_NONE)
            {
                outErrors.push_back(ValidationError(node.NodeID, node.NodeName,
                    "Branch node missing 'Then' connection",
                    ErrorSeverity::Error, "Link"));
                allValid = false;
            }

            if (elseTarget == NODE_INDEX_NONE)
            {
                outErrors.push_back(ValidationError(node.NodeID, node.NodeName,
                    "Branch node missing 'Else' connection",
                    ErrorSeverity::Error, "Link"));
                allValid = false;
            }
        }
    }

    return allValid;
}

bool GraphExecutionSimulator::ValidateDataConnections(const TaskGraphTemplate& tmpl,
                                                     std::vector<ValidationError>& outErrors)
{
    bool allValid = true;

    for (const auto& link : tmpl.DataConnections)
    {
        const TaskNodeDefinition* srcNode = tmpl.GetNode(link.SourceNodeID);
        const TaskNodeDefinition* dstNode = tmpl.GetNode(link.TargetNodeID);

        if (!srcNode || !dstNode)
        {
            outErrors.push_back(ValidationError(-1, "",
                "Data connection references non-existent node",
                ErrorSeverity::Error, "Link"));
            allValid = false;
        }
    }

    return allValid;
}

std::vector<int32_t> GraphExecutionSimulator::FindUnreachableNodes(const TaskGraphTemplate& tmpl,
                                                                  std::vector<ValidationError>& outErrors)
{
    std::map<int32_t, bool> reachable;
    for (const auto& node : tmpl.Nodes)
    {
        reachable[node.NodeID] = false;
    }

    // Mark entry point and all reachable nodes
    if (tmpl.EntryPointID >= 0)
    {
        MarkReachableNodes(tmpl, tmpl.EntryPointID, reachable);
    }

    std::vector<int32_t> unreachable;
    for (const auto& pair : reachable)
    {
        if (!pair.second && pair.first != -1)
        {
            unreachable.push_back(pair.first);

            const TaskNodeDefinition* node = tmpl.GetNode(pair.first);
            if (node)
            {
                outErrors.push_back(ValidationError(pair.first, node->NodeName,
                    "Node is unreachable from entry point",
                    ErrorSeverity::Warning, "Logic"));
            }
        }
    }

    return unreachable;
}

void GraphExecutionSimulator::MarkReachableNodes(const TaskGraphTemplate& tmpl,
                                                int32_t nodeId,
                                                std::map<int32_t, bool>& reachable)
{
    if (reachable.find(nodeId) != reachable.end() && reachable[nodeId])
        return; // Already marked

    reachable[nodeId] = true;

    // Find all nodes this one connects to
    for (const auto& link : tmpl.ExecConnections)
    {
        if (link.SourceNodeID == nodeId)
        {
            MarkReachableNodes(tmpl, link.TargetNodeID, reachable);
        }
    }
}

bool GraphExecutionSimulator::DetectPotentialInfiniteLoops(const TaskGraphTemplate& tmpl,
                                                          std::vector<ValidationError>& outErrors)
{
    bool found = false;

    for (const auto& node : tmpl.Nodes)
    {
        if (node.Type == TaskNodeType::While)
        {
            // While loops without proper exit conditions can be infinite
            // This is a heuristic - we'd need condition analysis for certainty
            int32_t exitTarget = GetNextNodeId(tmpl, node.NodeID, "Completed");
            if (exitTarget == NODE_INDEX_NONE)
            {
                outErrors.push_back(ValidationError(node.NodeID, node.NodeName,
                    "While loop has no 'Completed' exit path",
                    ErrorSeverity::Warning, "Logic"));
                found = true;
            }
        }
    }

    return found;
}

bool GraphExecutionSimulator::ValidateConditionExpression(int32_t nodeId,
                                                         const std::string& expression)
{
    // Basic validation - check for empty or invalid syntax
    if (expression.empty())
        return false;

    return true;
}

void GraphExecutionSimulator::BuildNodeReachabilityMap(const TaskGraphTemplate& tmpl,
                                                      std::map<int32_t, bool>& reachable)
{
    for (const auto& node : tmpl.Nodes)
    {
        reachable[node.NodeID] = false;
    }

    if (tmpl.EntryPointID >= 0)
    {
        MarkReachableNodes(tmpl, tmpl.EntryPointID, reachable);
    }
}

void GraphExecutionSimulator::TraceDataPinEvaluation(int32_t nodeId,
                                                   const TaskGraphTemplate& tmpl,
                                                   GraphExecutionTracer& tracer,
                                                   int32_t depth)
{
    // Prevent infinite recursion on cyclic data dependencies
    if (depth > 10)
    {
        return;
    }

    if (m_tracedDataNodes.find(nodeId) != m_tracedDataNodes.end())
    {
        return;
    }

    m_tracedDataNodes.insert(nodeId);

    const TaskNodeDefinition* node = tmpl.GetNode(nodeId);
    if (!node)
    {
        return;
    }

    // Log the data node being evaluated with depth indentation
    std::string indent(depth * 2, ' ');
    std::string depthMarker = (depth > 0) ? ("<- [EVAL] ") : "";

    switch (node->Type)
    {
        case TaskNodeType::GetBBValue:
        {
            tracer.RecordDataPinResolved(nodeId, "Value", 
                indent + depthMarker + "[GetBBValue] " + node->NodeName + 
                " (Key: " + node->BBKey + ")");
            break;
        }

        case TaskNodeType::MathOp:
        {
            // Log the MathOp node
            tracer.RecordDataPinResolved(nodeId, "Result", 
                indent + depthMarker + "[MathOp] " + node->NodeName + 
                " (Operator: " + node->MathOperator + ")");

            // Evaluate and trace both operands
            int32_t leftSourceNodeId = NODE_INDEX_NONE;
            int32_t rightSourceNodeId = NODE_INDEX_NONE;

            for (const auto& conn : tmpl.DataConnections)
            {
                if (conn.TargetNodeID == nodeId)
                {
                    if (conn.TargetPinName == "A")
                    {
                        leftSourceNodeId = conn.SourceNodeID;
                        // Trace left operand source
                        TraceDataConnection(conn.SourceNodeID, conn.SourcePinName,
                                          nodeId, "A", tmpl, tracer, depth + 1);
                    }
                    else if (conn.TargetPinName == "B")
                    {
                        rightSourceNodeId = conn.SourceNodeID;
                        // Trace right operand source
                        TraceDataConnection(conn.SourceNodeID, conn.SourcePinName,
                                          nodeId, "B", tmpl, tracer, depth + 1);
                    }
                }
            }
            break;
        }

        case TaskNodeType::SetBBValue:
        {
            tracer.RecordDataPinResolved(nodeId, "Input", 
                indent + depthMarker + "[SetBBValue] " + node->NodeName + 
                " (Key: " + node->BBKey + ")");

            // Find incoming data connection
            for (const auto& conn : tmpl.DataConnections)
            {
                if (conn.TargetNodeID == nodeId && conn.TargetPinName == "Value")
                {
                    TraceDataConnection(conn.SourceNodeID, conn.SourcePinName,
                                      nodeId, "Value", tmpl, tracer, depth + 1);
                }
            }
            break;
        }

        default:
            break;
    }

    // Find all data connections targeting this node (for non-MathOp nodes)
    if (node->Type != TaskNodeType::MathOp && node->Type != TaskNodeType::SetBBValue)
    {
        for (const auto& conn : tmpl.DataConnections)
        {
            if (conn.TargetNodeID == nodeId)
            {
                TraceDataConnection(conn.SourceNodeID, conn.SourcePinName,
                                  conn.TargetNodeID, conn.TargetPinName,
                                  tmpl, tracer, depth);
            }
        }
    }
}

void GraphExecutionSimulator::TraceDataConnection(int32_t sourceNodeId,
                                                const std::string& sourcePinName,
                                                int32_t targetNodeId,
                                                const std::string& targetPinName,
                                                const TaskGraphTemplate& tmpl,
                                                GraphExecutionTracer& tracer,
                                                int32_t depth)
{
    const TaskNodeDefinition* sourceNode = tmpl.GetNode(sourceNodeId);
    if (!sourceNode)
    {
        return;
    }

    // Recursively trace dependencies of this node
    TraceDataPinEvaluation(sourceNodeId, tmpl, tracer, depth);
}

} // namespace Olympe
