/**
 * @file VisualScriptEditorPanel_Verification.cpp
 * @brief Verification, validation, and preset panel rendering (Phase 12).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * Extracted methods:
        -	RunVerification()	~50
        -	RenderVerificationPanel()	~80
        -	RenderVerificationLogsPanel()	~50
        -	RenderValidationOverlay()	~30
        -	RenderPresetBankPanel()	~25
        -	RenderPresetItemCompact()	~15
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"
#include "../third_party/imgui/imgui.h"
#include "../TaskSystem/TaskGraphLoader.h"

#include <sstream>
#include <unordered_set>
#include <fstream>

namespace Olympe {

// Phase 24 — Helper macro to log simulation traces to both UI and system log
#define ADD_TRACE(trace_str) \
    do { \
        const std::string& _trace_temp = (trace_str); \
        m_verificationLogs.push_back(_trace_temp); \
        SYSTEM_LOG << "[SimTrace] " << _trace_temp << "\n"; \
    } while(0)

void VisualScriptEditorPanel::RunVerification()
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() called for graph '"
        << m_template.Name << "'\n";
    m_verificationResult = VSGraphVerifier::Verify(m_template);
    m_verificationDone = true;

    // Phase 24.3 — Populate verification logs for display in the output panel
    m_verificationLogs.clear();
    for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
    {
        const VSVerificationIssue& issue = m_verificationResult.issues[i];
        std::string logEntry;

        // Format: "[SEVERITY] message (Node: nodeID)"
        if (issue.severity == VSVerificationSeverity::Error)
            logEntry = "[ERROR] ";
        else if (issue.severity == VSVerificationSeverity::Warning)
            logEntry = "[WARN] ";
        else
            logEntry = "[INFO] ";

        logEntry += issue.message;
        if (issue.nodeID >= 0)
            logEntry += " (Node: " + std::to_string(issue.nodeID) + ")";

        m_verificationLogs.push_back(logEntry);
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() done: "
        << m_verificationResult.issues.size() << " issue(s), "
        << "errors=" << (m_verificationResult.HasErrors() ? "yes" : "no") << ", "
        << "warnings=" << (m_verificationResult.HasWarnings() ? "yes" : "no") << "\n";
}

void VisualScriptEditorPanel::RunGraphSimulation()
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] RunGraphSimulation() called for graph '"
        << m_template.Name << "'\n";

    // Clear previous simulation traces and initialize verification logs for display
    m_simulationTraces.clear();
    m_verificationLogs.clear();

    ADD_TRACE("[SIMULATION] Graph execution simulation started");
    ADD_TRACE("[SIMULATION] Graph: " + m_template.Name);
    ADD_TRACE("[SIMULATION] Total nodes: " + std::to_string(m_template.Nodes.size()));
    ADD_TRACE("[SIMULATION] Total connections: " + std::to_string(m_template.ExecConnections.size()));
    ADD_TRACE("[SIMULATION] Blackboard entries: " + std::to_string(m_template.Blackboard.size()));

    ADD_TRACE("");
    ADD_TRACE("=== EXECUTION TRACE ===");

    // Initialize blackboard with default values
    std::map<std::string, TaskValue> blackboard;
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        blackboard[m_template.Blackboard[i].Key] = m_template.Blackboard[i].Default;
    }

    // Phase 25 — Initialize visited graphs set for cycle detection
    std::unordered_set<std::string> visitedGraphs;

    // Find entry point
    int32_t currentNodeID = m_template.EntryPointID != NODE_INDEX_NONE ? 
                           m_template.EntryPointID : m_template.RootNodeID;

    if (currentNodeID == NODE_INDEX_NONE)
    {
        ADD_TRACE("[ERROR] No entry point or root node found!");
        SYSTEM_LOG << "[VisualScriptEditorPanel] Simulation FAILED: No entry point\n";
        m_simulationDone = true;
        return;
    }

    ADD_TRACE("[START] Entry point: Node #" + std::to_string(currentNodeID));

    // Phase 24.4 — Token-based execution for multi-branch support
    // Initialize execution token stack with entry point
    m_executionTokenStack.clear();
    m_executionTokenStack.push_back(ExecutionToken(currentNodeID, 0));

    // Simulate graph flow
    int stepCount = 0;
    int maxSteps = 100;
    std::unordered_set<int> visitedInPath;
    int lastTokenDepth = 0;  // Track depth to detect branch changes

    while (!m_executionTokenStack.empty() && stepCount < maxSteps)
    {
        // Pop execution token from stack
        ExecutionToken currentToken = m_executionTokenStack.back();
        m_executionTokenStack.pop_back();
        int32_t currentNodeID = currentToken.nodeID;
        int currentDepth = currentToken.depth; 

        // Phase 24.5 — Clear visited set when entering a new branch (depth decreased)
        // This allows the same node to be visited in different branches
        if (currentDepth < lastTokenDepth)
        {
            visitedInPath.clear();
            ADD_TRACE("[BRANCH] Entering new execution branch - resetting cycle detection");
        }
        lastTokenDepth = currentDepth;

        // Find node definition
        const TaskNodeDefinition* nodePtr = nullptr;
        for (size_t i = 0; i < m_template.Nodes.size(); ++i)
        {
            if (m_template.Nodes[i].NodeID == currentNodeID)
            {
                nodePtr = &m_template.Nodes[i];
                break;
            }
        }

        if (!nodePtr)
        {
            ADD_TRACE("[ERROR] Node #" + std::to_string(currentNodeID) + " not found in template!");
            break;
        }

        // Detect cycles
        if (visitedInPath.count(currentNodeID) > 0)
        {
            ADD_TRACE("[CYCLE] WARNING: Cycle detected! Node #" + std::to_string(currentNodeID) + 
                      " has already been visited in this path");
            break;
        }
        visitedInPath.insert(currentNodeID);

        // Trace node entry
        std::ostringstream nodeEntry;
        nodeEntry << "[ENTER] Step " << (stepCount + 1) << ": Node #" << nodePtr->NodeID;
        if (!nodePtr->NodeName.empty())
            nodeEntry << " '" << nodePtr->NodeName << "'";
        ADD_TRACE(nodeEntry.str());

        // Handle each node type with detailed traces
        int32_t nextNodeID = NODE_INDEX_NONE;

        switch (nodePtr->Type)
        {
            case TaskNodeType::EntryPoint:
            {
                ADD_TRACE("  +- [EVAL] EntryPoint - start of graph");
                // Find next connection
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = m_template.ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] EntryPoint completed");
                break;
            }

            case TaskNodeType::GetBBValue:
            {
                ADD_TRACE("  +- [EVAL] GetBBValue node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));
                ADD_TRACE("  |  Key: '" + nodePtr->BBKey + "'");

                // Find and display the value
                auto it = blackboard.find(nodePtr->BBKey);
                if (it != blackboard.end())
                {
                    std::ostringstream valTrace;
                    valTrace << "  |  Value: " << it->second.AsString();
                    ADD_TRACE(valTrace.str());
                }
                else
                {
                    ADD_TRACE("  |  Value: [NOT FOUND]");
                }

                // Phase 24.7 — Trace outgoing data connections
                ADD_TRACE("  |  Outgoing data connections:");
                bool hasDataOutput = false;
                for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
                {
                    if (m_template.DataConnections[i].SourceNodeID == currentNodeID)
                    {
                        hasDataOutput = true;
                        const DataPinConnection& dataConn = m_template.DataConnections[i];
                        const TaskNodeDefinition* targetNodePtr = nullptr;

                        // Find target node
                        for (size_t j = 0; j < m_template.Nodes.size(); ++j)
                        {
                            if (m_template.Nodes[j].NodeID == dataConn.TargetNodeID)
                            {
                                targetNodePtr = &m_template.Nodes[j];
                                break;
                            }
                        }

                        ADD_TRACE("    +- Out -> Node #" + std::to_string(dataConn.TargetNodeID) + 
                                  " pin '" + dataConn.TargetPinName + "'");
                        if (targetNodePtr && !targetNodePtr->NodeName.empty())
                            ADD_TRACE("       '" + targetNodePtr->NodeName + "'");
                    }
                }

                if (!hasDataOutput)
                {
                    ADD_TRACE("    +- (no data outputs)");
                }

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = m_template.ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Read value from blackboard");
                break;
            }

            case TaskNodeType::SetBBValue:
            {
                ADD_TRACE("  +- [EVAL] SetBBValue node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));
                ADD_TRACE("  |  Key: '" + nodePtr->BBKey + "'");

                // Phase 24.7 — Trace incoming data pins (e.g., Value input from MathOp or GetBBValue)
                // Find all data connections that target this node on a data pin
                ADD_TRACE("  |  Incoming data pins:");
                bool hasDataInput = false;

                // Phase 24.8 — For each incoming data connection, trace its upstream chain
                std::unordered_set<int> visitedDataNodes;

                for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
                {
                    if (m_template.DataConnections[i].TargetNodeID == currentNodeID)
                    {
                        hasDataInput = true;
                        const DataPinConnection& dataConn = m_template.DataConnections[i];
                        const TaskNodeDefinition* sourceNodePtr = nullptr;

                        // Find source node
                        for (size_t j = 0; j < m_template.Nodes.size(); ++j)
                        {
                            if (m_template.Nodes[j].NodeID == dataConn.SourceNodeID)
                            {
                                sourceNodePtr = &m_template.Nodes[j];
                                break;
                            }
                        }

                        ADD_TRACE("    +- Pin: '" + dataConn.TargetPinName + "' from Node #" + 
                                  std::to_string(dataConn.SourceNodeID));
                        if (sourceNodePtr && !sourceNodePtr->NodeName.empty())
                            ADD_TRACE("       '" + sourceNodePtr->NodeName + "'");

                        // Phase 24.8 — Recursively trace this source node's upstream data chain
                        TraceUpstreamDataNodes(dataConn.SourceNodeID, "       ", visitedDataNodes);
                    }
                }

                if (!hasDataInput)
                {
                    ADD_TRACE("    +- (no data inputs)");
                }

                ADD_TRACE("  |  Setting value in blackboard (simulated)");

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = m_template.ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Value written to blackboard");
                break;
            }

            case TaskNodeType::MathOp:
            {
                ADD_TRACE("  +- [EVAL] MathOp node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));

                // Search DataConnections first (independent of mathOpRef which may be out of date)
                std::ostringstream leftOp, rightOp;
                int32_t leftSourceNode = NODE_INDEX_NONE;
                int32_t rightSourceNode = NODE_INDEX_NONE;

                // Look for actual data connections to pins A and B
                for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
                {
                    if (m_template.DataConnections[i].TargetNodeID == currentNodeID)
                    {
                        if (m_template.DataConnections[i].TargetPinName == "A")
                        {
                            leftSourceNode = m_template.DataConnections[i].SourceNodeID;
                            leftOp << "Pin A: [from Node #" << leftSourceNode << "]";
                        }
                        else if (m_template.DataConnections[i].TargetPinName == "B")
                        {
                            rightSourceNode = m_template.DataConnections[i].SourceNodeID;
                            rightOp << "Pin B: [from Node #" << rightSourceNode << "]";
                        }
                    }
                }

                // If no data connections found, fall back to mathOpRef
                if (leftSourceNode == NODE_INDEX_NONE)
                {
                    if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable)
                        leftOp << "Variable: " << nodePtr->mathOpRef.leftOperand.variableName;
                    else if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const)
                        leftOp << "Const: " << nodePtr->mathOpRef.leftOperand.constValue;
                }

                if (rightSourceNode == NODE_INDEX_NONE)
                {
                    if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable)
                        rightOp << "Variable: " << nodePtr->mathOpRef.rightOperand.variableName;
                    else if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const)
                        rightOp << "Const: " << nodePtr->mathOpRef.rightOperand.constValue;
                }

                ADD_TRACE("  |  Operator: " + nodePtr->MathOperator);
                ADD_TRACE("  |  Input A: " + leftOp.str());
                ADD_TRACE("  |  Input B: " + rightOp.str());
                ADD_TRACE("  |  Result: [computed value]");

                // Phase 24.8 — Recursively trace upstream data nodes for pin inputs
                std::unordered_set<int> visitedDataNodes;

                // Trace left input (Pin mode)
                if (leftSourceNode != NODE_INDEX_NONE)
                {
                    ADD_TRACE("  |  [Upstream Pin A]:");
                    TraceUpstreamDataNodes(leftSourceNode, "  |    ", visitedDataNodes);
                }

                // Trace right input (Pin mode)
                if (rightSourceNode != NODE_INDEX_NONE)
                {
                    ADD_TRACE("  |  [Upstream Pin B]:");
                    TraceUpstreamDataNodes(rightSourceNode, "  |    ", visitedDataNodes);
                }

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = m_template.ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Math operation executed");
                break;
            }

            case TaskNodeType::Branch:
            {
                ADD_TRACE("  +- [EVAL] Branch node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));
                ADD_TRACE("  |  Evaluating condition...");

                // Simplified: always assume condition is true for simulation
                bool conditionResult = true;
                ADD_TRACE("  |  Condition result: " + std::string(conditionResult ? "TRUE" : "FALSE"));

                // Phase 24.6 — Push BOTH branches to stack (Then and Else paths)
                // This way the simulation explores both possible execution paths
                std::vector<ExecPinConnection> branchOutputs;
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        branchOutputs.push_back(m_template.ExecConnections[i]);
                    }
                }

                ADD_TRACE("  |  Found " + std::to_string(branchOutputs.size()) + " branch connection(s)");

                // Find Then and Else connections
                int32_t thenNodeID = NODE_INDEX_NONE;
                int32_t elseNodeID = NODE_INDEX_NONE;

                for (size_t i = 0; i < branchOutputs.size(); ++i)
                {
                    const ExecPinConnection& conn = branchOutputs[i];
                    std::ostringstream connTrace;
                    connTrace << "  |  Connection: SourcePin='" << conn.SourcePinName << "' -> Node #" << conn.TargetNodeID;
                    ADD_TRACE(connTrace.str());

                    // Match pin names: "Then", "Else", "Out", "OutElse", or empty
                    if (conn.SourcePinName == "Then" || conn.SourcePinName == "Out" || conn.SourcePinName.empty())
                    {
                        thenNodeID = conn.TargetNodeID;
                        ADD_TRACE("  |    -> Assigned to THEN branch (Node #" + std::to_string(thenNodeID) + ")");
                    }
                    else if (conn.SourcePinName == "Else" || conn.SourcePinName == "OutElse")
                    {
                        elseNodeID = conn.TargetNodeID;
                        ADD_TRACE("  |    -> Assigned to ELSE branch (Node #" + std::to_string(elseNodeID) + ")");
                    }
                }

                // Push both branches to stack in reverse order (LIFO): Else first, then Then
                // This ensures Then executes first (LIFO order), followed by Else
                if (elseNodeID != NODE_INDEX_NONE)
                {
                    m_executionTokenStack.push_back(ExecutionToken(elseNodeID, currentDepth + 1));
                    ADD_TRACE("  |  Pushed ELSE branch to stack: Node #" + std::to_string(elseNodeID));
                }

                if (thenNodeID != NODE_INDEX_NONE)
                {
                    m_executionTokenStack.push_back(ExecutionToken(thenNodeID, currentDepth + 1));
                    ADD_TRACE("  |  Pushed THEN branch to stack: Node #" + std::to_string(thenNodeID));
                }

                if (thenNodeID == NODE_INDEX_NONE && elseNodeID == NODE_INDEX_NONE)
                {
                    ADD_TRACE("  |  WARNING: No Then/Else branches found!");
                }

                std::string branchPath = !conditionResult ? "Else" : "Then";
                ADD_TRACE("  +- [RESULT] Branch explored: " + branchPath + " (both queued for exploration)");

                // Don't set nextNodeID - branches are already on the stack
                nextNodeID = NODE_INDEX_NONE;
                break;
            }

            case TaskNodeType::Switch:
            {
                ADD_TRACE("  +- [EVAL] Switch node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));
                ADD_TRACE("  |  Variable: '" + nodePtr->switchVariable + "'");
                ADD_TRACE("  |  Cases: " + std::to_string(nodePtr->switchCases.size()));

                // Find first case connection for simulation
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = m_template.ExecConnections[i].TargetNodeID;
                        ADD_TRACE("  |  Case selected: (first available)");
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Switch case executed");
                break;
            }

            case TaskNodeType::Delay:
            {
                ADD_TRACE("  +- [EVAL] Delay node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));
                std::ostringstream delayTrace;
                delayTrace << "  |  Duration: " << nodePtr->DelaySeconds << " seconds";
                ADD_TRACE(delayTrace.str());
                ADD_TRACE("  |  (Delay simulated)");

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = m_template.ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Delay completed");
                break;
            }

            case TaskNodeType::AtomicTask:
            {
                ADD_TRACE("  +- [EVAL] AtomicTask node");

                // Special simplified display for Log Message task
                if (nodePtr->AtomicTaskID == "log_message")
                {
                    ADD_TRACE("  |  Log Message");
                    // Extract and display only the message parameter
                    auto msgIt = nodePtr->Parameters.find("message");
                    if (msgIt != nodePtr->Parameters.end())
                    {
                        const std::string& msgValue = msgIt->second.LiteralValue.to_string();
                        ADD_TRACE("  |  Message: " + msgValue);
                    }
                }
                else
                {
                    // Standard display for other AtomicTasks
                    ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));
                    ADD_TRACE("  |  Task type: '" + nodePtr->AtomicTaskID + "'");

                    // Display only relevant parameters (exclude position and internal params)
                    if (!nodePtr->Parameters.empty())
                    {
                        ADD_TRACE("  |  Parameters:");
                        for (const auto& param : nodePtr->Parameters)
                        {
                            const std::string& paramName = param.first;
                            // Skip internal/position parameters
                            if (paramName.find("_pos") == 0 || paramName.find("__") == 0)
                                continue;

                            const ParameterBinding& binding = param.second;
                            std::string paramValue;
                            if (binding.Type == ParameterBindingType::Literal)
                            {
                                paramValue = binding.LiteralValue.to_string();
                            }
                            else if (binding.Type == ParameterBindingType::LocalVariable)
                            {
                                paramValue = "[Variable: " + binding.VariableName + "]";
                            }
                            else
                            {
                                paramValue = "[unknown binding]";
                            }

                            ADD_TRACE("  |    " + paramName + " = " + paramValue);
                        }
                    }
                }

                ADD_TRACE("  |  (Task execution simulated)");

                // Find Completed connection
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && conn.SourcePinName == "Completed")
                    {
                        nextNodeID = conn.TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Task completed");
                break;
            }

            case TaskNodeType::VSSequence:
            {
                ADD_TRACE("  +- [EVAL] Sequence node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));

                // Phase 24 Enhancement: Collect ALL outgoing exec connections from this Sequence
                std::vector<ExecPinConnection> sequenceOutputs;
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        sequenceOutputs.push_back(m_template.ExecConnections[i]);
                    }
                }

                ADD_TRACE("  |  Output pins: " + std::to_string(sequenceOutputs.size()));
                ADD_TRACE("  |  Executing sequence outputs:");

                // Execute each output branch in order
                for (size_t oi = 0; oi < sequenceOutputs.size(); ++oi)
                {
                    const ExecPinConnection& outConn = sequenceOutputs[oi];

                    // Find output node
                    const TaskNodeDefinition* outNodePtr = nullptr;
                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == outConn.TargetNodeID)
                        {
                            outNodePtr = &m_template.Nodes[i];
                            break;
                        }
                    }

                    std::ostringstream outTrace;
                    outTrace << "    +- Output [" << (oi + 1) << "] -> Node #" << outConn.TargetNodeID;
                    if (outNodePtr && !outNodePtr->NodeName.empty())
                        outTrace << " '" << outNodePtr->NodeName << "'";
                    ADD_TRACE(outTrace.str());
                }

                // Phase 24.4 — Push all branch tokens to stack in reverse order (LIFO)
                for (int oi = static_cast<int>(sequenceOutputs.size()) - 1; oi >= 0; --oi)
                {
                    const ExecPinConnection& outConn = sequenceOutputs[oi];
                    m_executionTokenStack.push_back(ExecutionToken(outConn.TargetNodeID, currentDepth + 1));
                }

                ADD_TRACE("  +- [RESULT] Sequence with " + std::to_string(sequenceOutputs.size()) + " branches");
                break;
            }

            case TaskNodeType::While:
            {
                ADD_TRACE("  +- [EVAL] While loop node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));

                // Trace condition evaluation
                ADD_TRACE("  |  Conditions:");
                if (!nodePtr->conditions.empty())
                {
                    for (size_t ci = 0; ci < nodePtr->conditions.size(); ++ci)
                    {
                        const Condition& cond = nodePtr->conditions[ci];
                        ADD_TRACE("  |    Condition #" + std::to_string(ci + 1) + 
                                  ": " + cond.leftVariable + " " + cond.operatorStr + " " + cond.rightVariable);
                    }
                }
                else
                {
                    ADD_TRACE("  |    (no conditions defined)");
                }

                // Simplified: assume condition is true for simulation
                ADD_TRACE("  |  Evaluating condition... TRUE (Loop continues)");

                // Find Loop and Completed output pins
                int32_t loopNodeID = NODE_INDEX_NONE;
                int32_t completedNodeID = NODE_INDEX_NONE;

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID)
                    {
                        if (conn.SourcePinName == "Loop" || conn.SourcePinName == "OutLoop")
                            loopNodeID = conn.TargetNodeID;
                        else if (conn.SourcePinName == "Completed" || conn.SourcePinName == "OutCompleted")
                            completedNodeID = conn.TargetNodeID;
                    }
                }

                if (loopNodeID != NODE_INDEX_NONE)
                {
                    ADD_TRACE("  |  Loop output -> Node #" + std::to_string(loopNodeID));
                    nextNodeID = loopNodeID;
                }
                else if (completedNodeID != NODE_INDEX_NONE)
                {
                    ADD_TRACE("  |  (Loop condition false - would exit)");
                    nextNodeID = completedNodeID;
                }

                ADD_TRACE("  +- [RESULT] Loop iteration executed");
                break;
            }

            case TaskNodeType::ForEach:
            {
                ADD_TRACE("  +- [EVAL] ForEach loop node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));
                ADD_TRACE("  |  (ForEach iteration parameters pending implementation)");

                // Find Loop Body and Completed output pins
                int32_t loopBodyNodeID = NODE_INDEX_NONE;
                int32_t completedNodeID = NODE_INDEX_NONE;

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID)
                    {
                        if (conn.SourcePinName == "Loop Body" || conn.SourcePinName == "OutLoop")
                            loopBodyNodeID = conn.TargetNodeID;
                        else if (conn.SourcePinName == "Completed" || conn.SourcePinName == "OutCompleted")
                            completedNodeID = conn.TargetNodeID;
                    }
                }

                // Simulate iteration
                ADD_TRACE("  |  Iterating list (simulated)");
                if (loopBodyNodeID != NODE_INDEX_NONE)
                {
                    ADD_TRACE("  |  Loop Body -> Node #" + std::to_string(loopBodyNodeID));
                    nextNodeID = loopBodyNodeID;
                }
                else if (completedNodeID != NODE_INDEX_NONE)
                {
                    ADD_TRACE("  |  List empty or iterations completed");
                    nextNodeID = completedNodeID;
                }

                ADD_TRACE("  +- [RESULT] ForEach loop executed");
                break;
            }

            case TaskNodeType::SubGraph:
            {
                ADD_TRACE("  +- [EVAL] SubGraph node");
                ADD_TRACE("  |  " + GetNodePropertyString(*nodePtr));

                // Phase 25 — Get SubGraphPath from either field or Parameters
                std::string subGraphPath = nodePtr->SubGraphPath;
                if (subGraphPath.empty())
                {
                    auto it = nodePtr->Parameters.find("subgraph_path");
                    if (it != nodePtr->Parameters.end() &&
                        it->second.Type == ParameterBindingType::Literal)
                    {
                        subGraphPath = it->second.LiteralValue.to_string();
                    }
                }

                ADD_TRACE("  |  Path: '" + subGraphPath + "'");

                // Trace input parameters if available
                if (!nodePtr->InputParams.empty())
                {
                    ADD_TRACE("  |  Input Parameters:");
                    for (const auto& param : nodePtr->InputParams)
                    {
                        const std::string& paramName = param.first;
                        const ParameterBinding& binding = param.second;

                        std::string paramValue;
                        if (binding.Type == ParameterBindingType::Literal)
                        {
                            paramValue = binding.LiteralValue.to_string();
                        }
                        else if (binding.Type == ParameterBindingType::LocalVariable)
                        {
                            paramValue = "[Variable: " + binding.VariableName + "]";
                        }
                        else
                        {
                            paramValue = "[unknown binding]";
                        }

                        ADD_TRACE("  |    " + paramName + " = " + paramValue);
                    }
                }

                // Phase 25 — Recursive SubGraph loading and execution
                if (!subGraphPath.empty())
                {
                    // Try to resolve and load the SubGraph
                    std::string resolvedPath;
                    std::string searchDirs[] = {
                        "./Blueprints/",
                        "./OlympeBlueprintEditor/Blueprints/",
                        "./Gamedata/"
                    };

                    for (size_t dirIdx = 0; dirIdx < 3; ++dirIdx)
                    {
                        std::string candidate = searchDirs[dirIdx] + subGraphPath;
                        std::ifstream testFile(candidate);
                        if (testFile.good())
                        {
                            resolvedPath = candidate;
                            break;
                        }
                    }

                    if (!resolvedPath.empty())
                    {
                        std::vector<std::string> loadErrors;
                        const TaskGraphTemplate* subGraphTemplate = TaskGraphLoader::LoadFromFile(resolvedPath, loadErrors);

                        if (subGraphTemplate)
                        {
                            ADD_TRACE("  |  === ENTERING SUBGRAPH ===");
                            ADD_TRACE("  |  File: " + resolvedPath);

                            // Phase 25 — Create isolated blackboard for this SubGraph
                            // Start with copy of current blackboard (parent values available as inputs)
                            std::map<std::string, TaskValue> isolatedBlackboard = blackboard;

                            // Phase 25 — Call recursive simulation with cycle detection
                            RunGraphSimulationRecursive(
                                subGraphTemplate,
                                isolatedBlackboard,
                                visitedGraphs,
                                0,  // Recursion depth: 0 for root SubGraph call
                                "  |  ");

                            // Phase 25 — Map output parameters back to parent blackboard
                            if (!nodePtr->OutputParams.empty())
                            {
                                ADD_TRACE("  |  Output Parameter Mapping:");
                                for (const auto& output : nodePtr->OutputParams)
                                {
                                    const std::string& outputName = output.first;
                                    const std::string& bbKey = output.second;

                                    auto it = isolatedBlackboard.find(outputName);
                                    if (it != isolatedBlackboard.end())
                                    {
                                        blackboard[bbKey] = it->second;
                                        ADD_TRACE("  |    " + outputName + " -> [" + bbKey + "]");
                                    }
                                    else
                                    {
                                        ADD_TRACE("  |    [WARNING] Output '" + outputName + "' not found in SubGraph context");
                                    }
                                }
                            }

                            ADD_TRACE("  |  === EXITING SUBGRAPH ===");

                            // Clean up
                            delete subGraphTemplate;
                            subGraphTemplate = nullptr;
                        }
                        else
                        {
                            ADD_TRACE("  |  [ERROR] Failed to load SubGraph: " + resolvedPath);
                            for (size_t errIdx = 0; errIdx < loadErrors.size(); ++errIdx)
                            {
                                ADD_TRACE("  |    - " + loadErrors[errIdx]);
                            }
                        }
                    }
                    else
                    {
                        ADD_TRACE("  |  [ERROR] SubGraph file not found in any search directory: " + subGraphPath);
                    }
                }
                else
                {
                    ADD_TRACE("  |  [ERROR] SubGraph path is empty!");
                }

                // Trace output parameter mappings if available
                if (!nodePtr->OutputParams.empty())
                {
                    ADD_TRACE("  |  Output Parameters (return values):");
                    for (const auto& output : nodePtr->OutputParams)
                    {
                        const std::string& outputName = output.first;
                        const std::string& bbKey = output.second;
                        ADD_TRACE("  |    " + outputName + " -> Blackboard['" + bbKey + "']");
                    }
                }

                // Find Completed output pin
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && conn.SourcePinName == "Completed")
                    {
                        nextNodeID = conn.TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] SubGraph completed");
                break;
            }

            default:
            {
                std::ostringstream unknownNode;
                unknownNode << "  +- [EVAL] Node type: " << static_cast<int>(nodePtr->Type);
                ADD_TRACE(unknownNode.str());

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = m_template.ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Node processed");
                break;
            }
        }

        // Phase 24.4 — Token-based exit trace
        // Trace node exit and token stack status
        // Always push nextNodeID if set (for single-branch nodes like Branch, Delay, etc.)
        if (nextNodeID != NODE_INDEX_NONE)
        {
            m_executionTokenStack.push_back(ExecutionToken(nextNodeID, currentDepth));
            ADD_TRACE("[EXIT] -> Pushed: Node #" + std::to_string(nextNodeID));
        }
        // If stack still has tokens, show the next one that will execute
        else if (!m_executionTokenStack.empty())
        {
            const ExecutionToken& nextToken = m_executionTokenStack.back();
            ADD_TRACE("[EXIT] -> Next token from stack: Node #" + std::to_string(nextToken.nodeID) + 
                      " (stack depth: " + std::to_string(m_executionTokenStack.size()) + ")");
        }
        else
        {
            ADD_TRACE("[EXIT] -> Graph complete (all tokens consumed)");
        }

        ADD_TRACE("");  // Blank line for readability

        ++stepCount;
    }

    // Final summary
    ADD_TRACE("=== EXECUTION SUMMARY ===");
    if (stepCount >= maxSteps)
    {
        ADD_TRACE("[WARNING] Maximum steps reached (" + std::to_string(maxSteps) + ") - possible infinite loop");
    }
    else if (m_executionTokenStack.empty())
    {
        ADD_TRACE("[SUCCESS] Graph execution completed - all branches finished");
    }
    else
    {
        ADD_TRACE("[WARNING] Execution incomplete - " + std::to_string(m_executionTokenStack.size()) + " token(s) remaining on stack");
    }
    ADD_TRACE("Total steps executed: " + std::to_string(stepCount));
    ADD_TRACE("Blackboard entries evaluated: " + std::to_string(blackboard.size()));

    m_simulationDone = true;

    SYSTEM_LOG << "[VisualScriptEditorPanel] Simulation completed: " << stepCount << " steps\n";

    // Append simulation traces to verification logs
    m_verificationLogs.push_back("");  // Separator
    m_verificationLogs.push_back("--- SIMULATION EXECUTION TRACE ---");
    m_verificationLogs.insert(m_verificationLogs.end(),
                              m_simulationTraces.begin(),
                              m_simulationTraces.end());
}

void VisualScriptEditorPanel::RenderVerificationPanel()
{
    ImGui::Separator();
    ImGui::TextDisabled("Graph Verification");

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("Click 'Verify' in toolbar to run verification.");
        return;
    }

    // Global status line
    if (m_verificationResult.HasErrors())
    {
        int errorCount = 0;
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            if (m_verificationResult.issues[i].severity == VSVerificationSeverity::Error)
                ++errorCount;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "Errors found: %d", errorCount);
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "OK — warnings present");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "OK — no issues");
    }

    if (m_verificationResult.issues.empty())
        return;

    // List issues grouped: Errors first, then Warnings, then Info
    const VSVerificationSeverity orderedSev[3] = {
        VSVerificationSeverity::Error,
        VSVerificationSeverity::Warning,
        VSVerificationSeverity::Info
    };

    for (int s = 0; s < 3; ++s)
    {
        VSVerificationSeverity sev = orderedSev[s];
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            const VSVerificationIssue& issue = m_verificationResult.issues[i];
            if (issue.severity != sev)
                continue;

            ImGui::PushID(static_cast<int>(i));

            if (sev == VSVerificationSeverity::Error)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[E]");
            else if (sev == VSVerificationSeverity::Warning)
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[W]");
            else
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[I]");

            ImGui::SameLine();
            ImGui::Text("%s: %s", issue.ruleID.c_str(), issue.message.c_str());

            if (issue.nodeID >= 0)
            {
                ImGui::SameLine();
                std::string btnLabel = "Go##go" + std::to_string(i);
                if (ImGui::SmallButton(btnLabel.c_str()))
                {
                    m_focusNodeID = issue.nodeID;
                    m_selectedNodeID = issue.nodeID;
                }
            }

            ImGui::PopID();
        }
    }
}

void VisualScriptEditorPanel::RenderVerificationLogsPanel()
{
    // Note: The header "Verification Output" is rendered by the container (BlueprintEditorGUI),
    // so we only render the content here (status + logs).

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("(Click 'Verify' or 'Run Graph' button to generate output)");
        return;
    }

    // Display verification result summary
    ImGui::Spacing();

    // Status line with color coding
    if (m_verificationResult.HasErrors())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "[ERROR] Graph has %d error(s)",
            (int)m_verificationResult.issues.size());
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f),
            "[WARNING] Graph is valid but has warnings");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
            "[OK] Graph is valid - no issues found");
    }

    ImGui::Separator();

    // Display all logs (verification issues + simulation traces)
    ImGui::BeginChild("VerificationLogsChild", ImVec2(0, 0), true);

    // If we have logs (from simulation or verification), display them
    if (!m_verificationLogs.empty())
    {
        for (size_t i = 0; i < m_verificationLogs.size(); ++i)
        {
            const std::string& logEntry = m_verificationLogs[i];

            // Color-code based on log entry content
            ImVec4 logColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Default: white

            if (logEntry.find("[ERROR]") != std::string::npos)
                logColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
            else if (logEntry.find("[WARN]") != std::string::npos)
                logColor = ImVec4(1.0f, 0.85f, 0.0f, 1.0f); // Yellow
            else if (logEntry.find("[INFO]") != std::string::npos)
                logColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);  // Light blue
            else if (logEntry.find("[SIMULATION]") != std::string::npos)
                logColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);  // Green
            else if (logEntry.find("[ENTER]") != std::string::npos)
                logColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);  // Light blue
            else if (logEntry.find("[EXIT]") != std::string::npos)
                logColor = ImVec4(0.8f, 0.8f, 1.0f, 1.0f);  // Very light blue
            else if (logEntry.find("[EVAL]") != std::string::npos)
                logColor = ImVec4(0.9f, 0.7f, 0.9f, 1.0f);  // Light purple
            else if (logEntry.find("[RESULT]") != std::string::npos)
                logColor = ImVec4(0.7f, 1.0f, 0.7f, 1.0f);  // Light green
            else if (logEntry.find("[CYCLE]") != std::string::npos)
                logColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange
            else if (logEntry.find("[WARNING]") != std::string::npos)
                logColor = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);  // Bright yellow
            else if (logEntry.find("[SUCCESS]") != std::string::npos)
                logColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);  // Bright green
            else if (logEntry.find("===") != std::string::npos)
                logColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White for separators

            ImGui::TextColored(logColor, "%s", logEntry.c_str());
        }
    }
    else if (!m_verificationResult.issues.empty())
    {
        // Fallback: Display verification issues if no simulation logs
        const VSVerificationSeverity orderedSev[3] = {
            VSVerificationSeverity::Error,
            VSVerificationSeverity::Warning,
            VSVerificationSeverity::Info
        };

        const char* sevLabels[3] = { "[ERROR]", "[WARN]", "[INFO]" };
        ImVec4      sevColors[3] = {
            ImVec4(1.0f, 0.3f, 0.3f, 1.0f),  // Error: red
            ImVec4(1.0f, 0.85f, 0.0f, 1.0f), // Warning: yellow
            ImVec4(0.5f, 0.8f, 1.0f, 1.0f)   // Info: light blue
        };

        for (int s = 0; s < 3; ++s)
        {
            VSVerificationSeverity sev = orderedSev[s];

            // Display all issues with this severity
            for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
            {
                const VSVerificationIssue& issue = m_verificationResult.issues[i];
                if (issue.severity != sev)
                    continue;

                // Format message: "[SEVERITY] message (NodeID: xxx)"
                std::string message = issue.message;
                if (issue.nodeID >= 0)
                {
                    message += " (Node: " + std::to_string(issue.nodeID) + ")";
                }

                ImGui::TextColored(sevColors[s], "%s: %s", sevLabels[s], message.c_str());
            }
        }
    }
    else
    {
        ImGui::TextDisabled("(No logs to display)");
    }

    ImGui::EndChild();
}

void VisualScriptEditorPanel::RenderValidationOverlay()
{
    m_validationWarnings.clear();
    m_validationErrors.clear();

    // Check: every non-EntryPoint node should have at least one exec-in connection
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        const VSEditorNode& eNode = m_editorNodes[i];
        if (eNode.def.Type == TaskNodeType::EntryPoint)
            continue;

        bool hasExecIn = false;
        for (size_t c = 0; c < m_template.ExecConnections.size(); ++c)
        {
            if (m_template.ExecConnections[c].TargetNodeID == eNode.nodeID)
            {
                hasExecIn = true;
                break;
            }
        }
        if (!hasExecIn)
        {
            m_validationErrors.push_back(
                "Node " + std::to_string(eNode.nodeID) + " (" +
                eNode.def.NodeName + "): no exec-in connection");
        }

        // SubGraph path validation
        if (eNode.def.Type == TaskNodeType::SubGraph &&
            eNode.def.SubGraphPath.empty())
        {
            m_validationWarnings.push_back(
                "Node " + std::to_string(eNode.nodeID) +
                " (SubGraph): SubGraphPath is empty");
        }
    }
}

void VisualScriptEditorPanel::RenderPresetBankPanel()
{
    ImGui::TextDisabled("Preset Bank (Global)");
    ImGui::Separator();

    if (!m_libraryPanel)
        return;

    size_t presetCount = m_presetRegistry.GetPresetCount();

    // Toolbar: Add preset button
    if (ImGui::Button("+##addpreset", ImVec2(25, 0)))
    {
        m_libraryPanel->OnAddPresetClicked();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("New Preset");

    ImGui::Separator();
    ImGui::TextDisabled("Total: %zu preset(s)", presetCount);
    ImGui::Separator();

    // List all presets in compact horizontal format
    std::vector<ConditionPreset> allPresets = m_presetRegistry.GetFilteredPresets("");

    if (allPresets.empty())
    {
        ImGui::TextDisabled("(no presets - create one to get started)");
    }

    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        const ConditionPreset& preset = allPresets[i];
        ImGui::PushID(preset.id.c_str());
        RenderPresetItemCompact(preset, i + 1);  // 1-indexed for display
        ImGui::PopID();
    }
}

void VisualScriptEditorPanel::RenderPresetItemCompact(const ConditionPreset& preset, size_t index)
{
#ifndef OLYMPE_HEADLESS
    // Single-line horizontal layout matching mockup:
    // [Index: Name (yellow)] [Left▼ mode] [value] [Op▼] [Right▼ mode] [value] [Edit] [Dup] [X]

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

    // Get a mutable copy of the preset for editing
    ConditionPreset editablePreset = preset;
    bool presetModified = false;

    // Condition name display with index (yellow)
    // Use PushID for unique identification, don't add UUID to visible text
    ImGui::PushID(editablePreset.id.c_str());
    ImGui::TextColored(ImVec4(1.0f, 0.843f, 0.0f, 1.0f), "Condition #%zu", index);
    ImGui::PopID();
    ImGui::SameLine(0.0f, 12.0f);

    // Left operand with unified dropdown (mode + value combined)
    if (RenderOperandEditor(editablePreset.left, "##left_op"))
    {
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 6.0f);

    // Operator dropdown
    std::string opStr;
    switch (editablePreset.op)
    {
    case ComparisonOp::Equal:       opStr = "=="; break;
    case ComparisonOp::NotEqual:    opStr = "!="; break;
    case ComparisonOp::Less:        opStr = "<"; break;
    case ComparisonOp::LessEqual:   opStr = "<="; break;
    case ComparisonOp::Greater:     opStr = ">"; break;
    case ComparisonOp::GreaterEqual: opStr = ">="; break;
    default: opStr = "?"; break;
    }

    const char* opNames[] = { "==", "!=", "<", "<=", ">", ">=" };
    const ComparisonOp opValues[] = {
        ComparisonOp::Equal, ComparisonOp::NotEqual,
        ComparisonOp::Less, ComparisonOp::LessEqual,
        ComparisonOp::Greater, ComparisonOp::GreaterEqual
    };
    int curOpIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (editablePreset.op == opValues[i])
        {
            curOpIdx = i;
            break;
        }
    }

    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::Combo("##op_type", &curOpIdx, opNames, 6))
    {
        editablePreset.op = opValues[curOpIdx];
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 6.0f);

    // Right operand with unified dropdown (mode + value combined)
    if (RenderOperandEditor(editablePreset.right, "##right_op"))
    {
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 12.0f);

    // Save modified preset if changed
    if (presetModified)
    {
        m_presetRegistry.UpdatePreset(editablePreset.id, editablePreset);

        // Phase 24 — Sync to template presets for graph serialization
        // Update the preset in m_template.Presets so it gets saved with the graph
        for (size_t pi = 0; pi < m_template.Presets.size(); ++pi)
        {
            if (m_template.Presets[pi].id == editablePreset.id)
            {
                m_template.Presets[pi] = editablePreset;
                break;
            }
        }

        m_dirty = true;
    }

    // Duplicate button
    if (ImGui::Button("Dup##dup_preset", ImVec2(40, 0)))
    {
        std::string newPresetID = m_presetRegistry.DuplicatePreset(editablePreset.id);

        // Phase 24 — Add the duplicate to template presets as well
        if (!newPresetID.empty())
        {
            const ConditionPreset* newPreset = m_presetRegistry.GetPreset(newPresetID);
            if (newPreset)
            {
                m_template.Presets.push_back(*newPreset);
            }
        }

        m_dirty = true;
    }
    ImGui::SameLine(0.0f, 4.0f);

    // Delete button
    if (ImGui::Button("X##del_preset", ImVec2(25, 0)))
    {
        m_presetRegistry.DeletePreset(editablePreset.id);
        m_pinManager->InvalidatePreset(editablePreset.id);

        // Phase 24 — Remove from template presets as well
        for (size_t pi = 0; pi < m_template.Presets.size(); ++pi)
        {
            if (m_template.Presets[pi].id == editablePreset.id)
            {
                m_template.Presets.erase(m_template.Presets.begin() + pi);
                break;
            }
        }
        // Persist the deletion to disk
        m_presetRegistry.Save("Blueprints/Presets/condition_presets.json");
    }

    ImGui::PopStyleColor(3);

    // Add visual separator between presets
    ImGui::Separator();
#endif
}

// ============================================================================
// Phase 24 — Graph Simulation Helper Methods
// ============================================================================

void VisualScriptEditorPanel::EvaluateDataNode(int32_t nodeID, int depth, const std::string& indent)
{
    // Find node definition
    const TaskNodeDefinition* nodePtr = nullptr;
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].NodeID == nodeID)
        {
            nodePtr = &m_template.Nodes[i];
            break;
        }
    }

    if (!nodePtr)
        return;

    // Only evaluate pure data nodes (non-execution nodes)
    switch (nodePtr->Type)
    {
        case TaskNodeType::GetBBValue:
        {
            ADD_TRACE(indent + "+- [DATA] GetBBValue #" + std::to_string(nodeID));
            if (!nodePtr->NodeName.empty())
                ADD_TRACE(indent + "|  Name: '" + nodePtr->NodeName + "'");
            ADD_TRACE(indent + "|  Key: '" + nodePtr->BBKey + "'");
            ADD_TRACE(indent + "+- -> Returns value from blackboard");
            break;
        }

        case TaskNodeType::MathOp:
        {
            ADD_TRACE(indent + "+- [DATA] MathOp #" + std::to_string(nodeID));
            if (!nodePtr->NodeName.empty())
                ADD_TRACE(indent + "|  Name: '" + nodePtr->NodeName + "'");
            ADD_TRACE(indent + "|  Op: " + nodePtr->MathOperator);

            // Trace left operand recursively if it's a data node reference
            std::string leftDesc = "[const/var]";
            if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable)
                leftDesc = "Var: " + nodePtr->mathOpRef.leftOperand.variableName;
            else if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const)
                leftDesc = "Const: " + nodePtr->mathOpRef.leftOperand.constValue;
            else if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Pin)
                leftDesc = "Pin input";

            ADD_TRACE(indent + "|  Left: " + leftDesc);

            // Trace right operand recursively if it's a data node reference
            std::string rightDesc = "[const/var]";
            if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable)
                rightDesc = "Var: " + nodePtr->mathOpRef.rightOperand.variableName;
            else if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const)
                rightDesc = "Const: " + nodePtr->mathOpRef.rightOperand.constValue;
            else if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Pin)
                rightDesc = "Pin input";

            ADD_TRACE(indent + "|  Right: " + rightDesc);
            ADD_TRACE(indent + "+- -> Result value");
            break;
        }

        default:
            // Other data node types can be added here
            break;
    }
}

// Phase 24.10 — Format task parameters for display in traces
// Extracts and formats all parameters from a ParameterBinding map
std::string VisualScriptEditorPanel::FormatTaskParameters(const std::unordered_map<std::string, ParameterBinding>& parameters,
                                                           const std::string& indent)
{
    if (parameters.empty())
        return "";

    std::ostringstream ss;
    ss << "\n" << indent << "Parameters:";

    for (const auto& param : parameters)
    {
        const std::string& paramName = param.first;
        const ParameterBinding& binding = param.second;

        ss << "\n" << indent << "  " << paramName << " = ";

        if (binding.Type == ParameterBindingType::Literal)
        {
            ss << binding.LiteralValue.to_string();
        }
        else if (binding.Type == ParameterBindingType::LocalVariable)
        {
            ss << "[Variable: " << binding.VariableName << "]";
        }
    }

    return ss.str();
}

// Phase 24.8 — Recursive tracing of upstream data nodes (pure data chains)
// Traces all upstream pure data nodes (GetBBValue, MathOp, etc.) with recursive depth
void VisualScriptEditorPanel::TraceUpstreamDataNodes(int32_t sourceNodeID,
                                                      const std::string& indent,
                                                      std::unordered_set<int>& visitedDataNodes)
{
    // Prevent infinite loops in data graphs
    if (visitedDataNodes.count(sourceNodeID) > 0)
    {
        ADD_TRACE(indent + "+- [CYCLE] Already traced: Node #" + std::to_string(sourceNodeID));
        return;
    }
    visitedDataNodes.insert(sourceNodeID);

    // Find node definition
    const TaskNodeDefinition* nodePtr = nullptr;
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].NodeID == sourceNodeID)
        {
            nodePtr = &m_template.Nodes[i];
            break;
        }
    }

    if (!nodePtr)
    {
        ADD_TRACE(indent + "+- [ERROR] Node #" + std::to_string(sourceNodeID) + " not found");
        return;
    }

    // Determine if this is a pure data node (non-execution)
    bool isPureDataNode = false;
    std::string nodeTypeStr = "";

    switch (nodePtr->Type)
    {
        case TaskNodeType::GetBBValue:
            isPureDataNode = true;
            nodeTypeStr = "GetBBValue";
            break;
        case TaskNodeType::MathOp:
            isPureDataNode = true;
            nodeTypeStr = "MathOp";
            break;
        default:
            break;
    }

    if (!isPureDataNode)
    {
        // Non-data node: trace once and stop recursion
        ADD_TRACE(indent + "+- [NON-DATA] Node #" + std::to_string(sourceNodeID) + 
                  " (" + nodeTypeStr + ")");
        return;
    }

    // Trace this data node with its properties
    ADD_TRACE(indent + "+- [DATA] " + nodeTypeStr + " #" + std::to_string(sourceNodeID));
    if (!nodePtr->NodeName.empty())
        ADD_TRACE(indent + "|  Name: '" + nodePtr->NodeName + "'");

    // Add type-specific properties
    if (nodePtr->Type == TaskNodeType::GetBBValue)
    {
        ADD_TRACE(indent + "|  Key: '" + nodePtr->BBKey + "'");
        ADD_TRACE(indent + "|  (Source: blackboard)");
    }
     else if (nodePtr->Type == TaskNodeType::MathOp)
    {
        ADD_TRACE(indent + "|  Operator: " + nodePtr->MathOperator);

        // Trace left and right operands by searching DataConnections directly
        // (instead of relying on mathOpRef which may be out of date)
        int32_t leftSourceNode = NODE_INDEX_NONE;
        int32_t rightSourceNode = NODE_INDEX_NONE;
        std::string leftDesc = "";
        std::string rightDesc = "";

        // Search for data connections where this node's input pins are targeted
        for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
        {
            if (m_template.DataConnections[i].TargetNodeID == sourceNodeID)
            {
                if (m_template.DataConnections[i].TargetPinName == "A")
                {
                    leftSourceNode = m_template.DataConnections[i].SourceNodeID;
                    leftDesc = "[Pin A from Node #" + std::to_string(leftSourceNode) + "]";
                }
                else if (m_template.DataConnections[i].TargetPinName == "B")
                {
                    rightSourceNode = m_template.DataConnections[i].SourceNodeID;
                    rightDesc = "[Pin B from Node #" + std::to_string(rightSourceNode) + "]";
                }
            }
        }

        // If no data connections found for pins, fall back to mathOpRef
        if (leftSourceNode == NODE_INDEX_NONE)
        {
            if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable)
                leftDesc = "Variable: " + nodePtr->mathOpRef.leftOperand.variableName;
            else if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const)
                leftDesc = "Const: " + nodePtr->mathOpRef.leftOperand.constValue;
        }

        if (rightSourceNode == NODE_INDEX_NONE)
        {
            if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable)
                rightDesc = "Variable: " + nodePtr->mathOpRef.rightOperand.variableName;
            else if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const)
                rightDesc = "Const: " + nodePtr->mathOpRef.rightOperand.constValue;
        }

        ADD_TRACE(indent + "|  Left: " + leftDesc);
        ADD_TRACE(indent + "|  Right: " + rightDesc);

        // Recursively trace left pin input if it's a data node
        if (leftSourceNode != NODE_INDEX_NONE)
        {
            ADD_TRACE(indent + "|  [Upstream Left (Pin A)]:");
            TraceUpstreamDataNodes(leftSourceNode, indent + "|  ", visitedDataNodes);
        }

        // Recursively trace right pin input if it's a data node
        if (rightSourceNode != NODE_INDEX_NONE)
        {
            ADD_TRACE(indent + "|  [Upstream Right (Pin B)]:");
            TraceUpstreamDataNodes(rightSourceNode, indent + "|  ", visitedDataNodes);
        }
    }
}

// Phase 24.9 — Get comprehensive property string for any node type
// Returns a formatted string with all relevant properties for display
std::string VisualScriptEditorPanel::GetNodePropertyString(const TaskNodeDefinition& node)
{
    std::ostringstream ss;

    switch (node.Type)
    {
        case TaskNodeType::EntryPoint:
            ss << "EntryPoint";
            break;

        case TaskNodeType::GetBBValue:
            ss << "GetBBValue | Key: '" << node.BBKey << "'";
            break;

        case TaskNodeType::SetBBValue:
            ss << "SetBBValue | Key: '" << node.BBKey << "'";
            break;

        case TaskNodeType::MathOp:
            ss << "MathOp | Op: " << node.MathOperator;
            break;

        case TaskNodeType::Branch:
            ss << "Branch | Type: Conditional";
            break;

        case TaskNodeType::Switch:
            ss << "Switch | Var: '" << node.switchVariable << "' | Cases: " << node.switchCases.size();
            break;

        case TaskNodeType::Delay:
            ss << "Delay | Duration: " << node.DelaySeconds << "s";
            break;

        case TaskNodeType::AtomicTask:
        {
            ss << "AtomicTask | TaskID: '" << node.AtomicTaskID << "'";
            // Add first parameter or message for display
            if (!node.Parameters.empty())
            {
                auto it = node.Parameters.begin();
                ss << " | " << it->first << ": " << it->second.LiteralValue.to_string();
            }
            break;
        }

        case TaskNodeType::VSSequence:
            ss << "Sequence";
            break;

        case TaskNodeType::While:
            ss << "While loop";
            break;

        case TaskNodeType::SubGraph:
            ss << "SubGraph | Path: '" << node.SubGraphPath << "'";
            break;

        case TaskNodeType::ForEach:
            ss << "ForEach";
            break;

        case TaskNodeType::DoOnce:
            ss << "DoOnce";
            break;

        default:
            ss << "Unknown node type";
            break;
    }

    return ss.str();
}

// Phase 25 — Recursive SubGraph execution with cycle detection
void VisualScriptEditorPanel::RunGraphSimulationRecursive(
    const TaskGraphTemplate* tmpl,
    std::map<std::string, TaskValue>& blackboard,
    std::unordered_set<std::string>& visitedGraphs,
    int recursionDepth,
    const std::string& traceIndent)
{
    // Phase 25 — Depth validation
    if (recursionDepth >= 20)
    {
        ADD_TRACE(traceIndent + "[ERROR] SubGraph recursion depth limit exceeded (>= 20)");
        ADD_TRACE(traceIndent + "[ERROR] Possible infinite loop detected - stopping execution");
        return;
    }

    if (recursionDepth > 10)
    {
        ADD_TRACE(traceIndent + "[WARNING] SubGraph recursion depth > 10 - possible infinite loop");
    }

    // Phase 25 — Find entry point
    int32_t currentNodeID = tmpl->EntryPointID != NODE_INDEX_NONE ? 
                           tmpl->EntryPointID : tmpl->RootNodeID;

    if (currentNodeID == NODE_INDEX_NONE)
    {
        ADD_TRACE(traceIndent + "[ERROR] No entry point in SubGraph");
        return;
    }

    ADD_TRACE(traceIndent + "[SUBGRAPH] Starting execution at Node #" + std::to_string(currentNodeID));

    // Phase 25 — Token-based execution (same as main simulation)
    std::vector<ExecutionToken> tokenStack;
    tokenStack.push_back(ExecutionToken(currentNodeID, 0));

    int stepCount = 0;
    int maxSteps = 100;
    std::unordered_set<int> visitedInPath;

    while (!tokenStack.empty() && stepCount < maxSteps)
    {
        ExecutionToken currentToken = tokenStack.back();
        tokenStack.pop_back();
        currentNodeID = currentToken.nodeID;

        // Find node definition
        const TaskNodeDefinition* nodePtr = nullptr;
        for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
        {
            if (tmpl->Nodes[i].NodeID == currentNodeID)
            {
                nodePtr = &tmpl->Nodes[i];
                break;
            }
        }

        if (!nodePtr)
        {
            ADD_TRACE(traceIndent + "[ERROR] Node #" + std::to_string(currentNodeID) + " not found");
            break;
        }

        // Detect cycles in this SubGraph context
        if (visitedInPath.count(currentNodeID) > 0)
        {
            ADD_TRACE(traceIndent + "[CYCLE] Node #" + std::to_string(currentNodeID) + " already visited");
            break;
        }
        visitedInPath.insert(currentNodeID);

        // Trace node entry
        ADD_TRACE(traceIndent + "[NODE] #" + std::to_string(nodePtr->NodeID) + " - " + nodePtr->NodeName);

        int32_t nextNodeID = NODE_INDEX_NONE;

        // Phase 25 — Handle key node types (simplified for SubGraph context)
        switch (nodePtr->Type)
        {
            case TaskNodeType::EntryPoint:
            {
                // Find next connection
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    if (tmpl->ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = tmpl->ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                break;
            }

            case TaskNodeType::GetBBValue:
            {
                ADD_TRACE(traceIndent + "  [EVAL] GetBBValue - Key: '" + nodePtr->BBKey + "'");
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    if (tmpl->ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = tmpl->ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                break;
            }

            case TaskNodeType::SetBBValue:
            {
                ADD_TRACE(traceIndent + "  [EVAL] SetBBValue - Key: '" + nodePtr->BBKey + "'");
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    if (tmpl->ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = tmpl->ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                break;
            }

            case TaskNodeType::AtomicTask:
            {
                ADD_TRACE(traceIndent + "  [TASK] " + nodePtr->AtomicTaskID);
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = tmpl->ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && conn.SourcePinName == "Completed")
                    {
                        nextNodeID = conn.TargetNodeID;
                        break;
                    }
                }
                break;
            }

            case TaskNodeType::VSSequence:
            {
                ADD_TRACE(traceIndent + "  [SEQUENCE] executing branches");
                std::vector<ExecPinConnection> outputs;
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    if (tmpl->ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        outputs.push_back(tmpl->ExecConnections[i]);
                    }
                }

                // Push branches in reverse order (LIFO)
                for (int oi = static_cast<int>(outputs.size()) - 1; oi >= 0; --oi)
                {
                    tokenStack.push_back(ExecutionToken(outputs[oi].TargetNodeID, currentToken.depth + 1));
                }
                break;
            }

            case TaskNodeType::Branch:
            {
                ADD_TRACE(traceIndent + "  [BRANCH] evaluating condition");
                std::vector<ExecPinConnection> outputs;
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    if (tmpl->ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        outputs.push_back(tmpl->ExecConnections[i]);
                    }
                }

                // Push both branches (simplified: always explore both)
                for (int oi = static_cast<int>(outputs.size()) - 1; oi >= 0; --oi)
                {
                    tokenStack.push_back(ExecutionToken(outputs[oi].TargetNodeID, currentToken.depth + 1));
                }
                break;
            }

            // Phase 25 — RECURSIVE SubGraph call within SubGraph
            case TaskNodeType::SubGraph:
            {
                std::string subGraphPath = nodePtr->SubGraphPath;
                if (subGraphPath.empty())
                {
                    auto it = nodePtr->Parameters.find("subgraph_path");
                    if (it != nodePtr->Parameters.end() &&
                        it->second.Type == ParameterBindingType::Literal)
                    {
                        subGraphPath = it->second.LiteralValue.to_string();
                    }
                }

                ADD_TRACE(traceIndent + "  [SUBGRAPH] Path: '" + subGraphPath + "'");

                if (!subGraphPath.empty())
                {
                    // Phase 25 — Check if already visited (cycle detection by file name)
                    if (visitedGraphs.count(subGraphPath) > 0)
                    {
                        ADD_TRACE(traceIndent + "  [CYCLE] SubGraph '" + subGraphPath + "' already in execution chain");
                        break;
                    }

                    // Resolve file path
                    std::string resolvedPath;
                    std::string searchDirs[] = {
                        "./Blueprints/",
                        "./OlympeBlueprintEditor/Blueprints/",
                        "./Gamedata/"
                    };

                    for (size_t dirIdx = 0; dirIdx < 3; ++dirIdx)
                    {
                        std::string candidate = searchDirs[dirIdx] + subGraphPath;
                        std::ifstream testFile(candidate);
                        if (testFile.good())
                        {
                            resolvedPath = candidate;
                            break;
                        }
                    }

                    if (!resolvedPath.empty())
                    {
                        // Load SubGraph
                        std::vector<std::string> loadErrors;
                        const TaskGraphTemplate* subGraphTemplate = TaskGraphLoader::LoadFromFile(resolvedPath, loadErrors);

                        if (subGraphTemplate)
                        {
                            // Phase 25 — Track this graph in visited set
                            visitedGraphs.insert(subGraphPath);

                            // Phase 25 — Create isolated blackboard for SubGraph (copy of current state)
                            std::map<std::string, TaskValue> isolatedBlackboard = blackboard;

                            // Phase 25 — Recursively execute SubGraph with new depth
                            ADD_TRACE(traceIndent + "    [ENTER] Recursion depth: " + std::to_string(recursionDepth + 1));
                            RunGraphSimulationRecursive(
                                subGraphTemplate,
                                isolatedBlackboard,
                                visitedGraphs,
                                recursionDepth + 1,
                                traceIndent + "    ");
                            ADD_TRACE(traceIndent + "    [EXIT] Returning from SubGraph");

                            // Phase 25 — Merge output parameters from SubGraph back to parent blackboard
                            if (!nodePtr->OutputParams.empty())
                            {
                                for (const auto& output : nodePtr->OutputParams)
                                {
                                    const std::string& outputName = output.first;
                                    const std::string& bbKey = output.second;

                                    auto it = isolatedBlackboard.find(outputName);
                                    if (it != isolatedBlackboard.end())
                                    {
                                        blackboard[bbKey] = it->second;
                                        ADD_TRACE(traceIndent + "    [OUTPUT] " + outputName + " -> [" + bbKey + "]");
                                    }
                                }
                            }

                            // Remove from visited set (allow revisit at different path)
                            visitedGraphs.erase(subGraphPath);

                            delete subGraphTemplate;
                            subGraphTemplate = nullptr;
                        }
                        else
                        {
                            ADD_TRACE(traceIndent + "  [ERROR] Failed to load SubGraph: " + resolvedPath);
                            for (size_t errIdx = 0; errIdx < loadErrors.size(); ++errIdx)
                            {
                                ADD_TRACE(traceIndent + "    - " + loadErrors[errIdx]);
                            }
                        }
                    }
                    else
                    {
                        ADD_TRACE(traceIndent + "  [ERROR] SubGraph file not found: " + subGraphPath);
                    }
                }
                else
                {
                    ADD_TRACE(traceIndent + "  [ERROR] SubGraph path is empty");
                }

                // Find Completed pin
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = tmpl->ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && conn.SourcePinName == "Completed")
                    {
                        nextNodeID = conn.TargetNodeID;
                        break;
                    }
                }
                break;
            }

            default:
            {
                // Other node types: just find next execution node
                for (size_t i = 0; i < tmpl->ExecConnections.size(); ++i)
                {
                    if (tmpl->ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeID = tmpl->ExecConnections[i].TargetNodeID;
                        break;
                    }
                }
                break;
            }
        }

        // Push next node if set
        if (nextNodeID != NODE_INDEX_NONE)
        {
            tokenStack.push_back(ExecutionToken(nextNodeID, currentToken.depth));
        }

        ++stepCount;
    }

    if (stepCount >= maxSteps)
    {
        ADD_TRACE(traceIndent + "[WARNING] SubGraph max steps reached - possible infinite loop");
    }
}

} // namespace Olympe
