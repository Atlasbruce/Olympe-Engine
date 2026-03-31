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

#include <sstream>
#include <unordered_set>

namespace Olympe {

// Phase 24 — Helper macro to log simulation traces to both UI and system log
#define ADD_TRACE(trace_str) \
    do { \
        m_simulationTraces.push_back(trace_str); \
        SYSTEM_LOG << "[SimTrace] " << trace_str << "\n"; \
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

    // Clear previous simulation traces
    m_simulationTraces.clear();

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
                ADD_TRACE("  |  Key: '" + nodePtr->BBKey + "'");
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
                ADD_TRACE("  |  Operator: '" + nodePtr->MathOperator + "'");

                // Display operands from mathOpRef if available
                std::ostringstream leftOp, rightOp;

                if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable)
                    leftOp << "Variable: " << nodePtr->mathOpRef.leftOperand.variableName;
                else if (nodePtr->mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const)
                    leftOp << "Const: " << nodePtr->mathOpRef.leftOperand.constValue;
                else
                    leftOp << "Pin: [input]";

                if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable)
                    rightOp << "Variable: " << nodePtr->mathOpRef.rightOperand.variableName;
                else if (nodePtr->mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const)
                    rightOp << "Const: " << nodePtr->mathOpRef.rightOperand.constValue;
                else
                    rightOp << "Pin: [input]";

                ADD_TRACE("  |  Left operand: " + leftOp.str());
                ADD_TRACE("  |  Right operand: " + rightOp.str());
                ADD_TRACE("  |  Result: [computed value]");

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
                ADD_TRACE("  |  Task type: '" + nodePtr->AtomicTaskID + "'");
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
                ADD_TRACE("  |  Evaluating condition...");
                ADD_TRACE("  |  Condition result: TRUE (Loop continues)");

                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && conn.SourcePinName == "Loop")
                    {
                        nextNodeID = conn.TargetNodeID;
                        break;
                    }
                }
                ADD_TRACE("  +- [RESULT] Loop iteration");
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

} // namespace Olympe
