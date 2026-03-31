void VisualScriptEditorPanel::RunGraphSimulation()
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] RunGraphSimulation() called for graph '"
        << m_template.Name << "'\n";

    // Clear previous simulation traces and token stack
    m_simulationTraces.clear();
    m_executionTokenStack.clear();

    m_simulationTraces.push_back("[SIMULATION] Graph execution simulation started");
    m_simulationTraces.push_back("[SIMULATION] Graph: " + m_template.Name);
    m_simulationTraces.push_back("[SIMULATION] Total nodes: " + std::to_string(m_template.Nodes.size()));
    m_simulationTraces.push_back("[SIMULATION] Total connections: " + std::to_string(m_template.ExecConnections.size()));
    m_simulationTraces.push_back("[SIMULATION] Blackboard entries: " + std::to_string(m_template.Blackboard.size()));

    m_simulationTraces.push_back("");
    m_simulationTraces.push_back("=== EXECUTION TRACE ===");

    // Initialize blackboard with default values
    std::map<std::string, TaskValue> blackboard;
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        blackboard[m_template.Blackboard[i].Key] = m_template.Blackboard[i].Default;
    }

    // Find entry point and push initial token
    int32_t entryPointID = m_template.EntryPointID != NODE_INDEX_NONE ? 
                           m_template.EntryPointID : m_template.RootNodeID;

    if (entryPointID == NODE_INDEX_NONE)
    {
        m_simulationTraces.push_back("[ERROR] No entry point or root node found!");
        SYSTEM_LOG << "[VisualScriptEditorPanel] Simulation FAILED: No entry point\n";
        m_simulationDone = true;
        return;
    }

    m_simulationTraces.push_back("[START] Entry point: Node #" + std::to_string(entryPointID));

    // Phase 24.4 — Token-based execution stack
    m_executionTokenStack.push_back(ExecutionToken(entryPointID, 0));

    int stepCount = 0;
    int maxSteps = 200;  // Increased for multi-branch scenarios
    std::unordered_set<std::string> executionPaths;  // Track (nodeID, depth) to detect cycles

    while (!m_executionTokenStack.empty() && stepCount < maxSteps)
    {
        // Pop the next token from the stack
        ExecutionToken currentToken = m_executionTokenStack.back();
        m_executionTokenStack.pop_back();

        int32_t currentNodeID = currentToken.nodeID;
        int depth = currentToken.depth;

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
            m_simulationTraces.push_back("[ERROR] Node #" + std::to_string(currentNodeID) + " not found in template!");
            continue;
        }

        // Detect infinite loops at same depth
        std::string pathKey = std::to_string(currentNodeID) + ":" + std::to_string(depth);
        if (executionPaths.count(pathKey) > 0)
        {
            m_simulationTraces.push_back("[CYCLE] WARNING: Cycle detected at Node #" + std::to_string(currentNodeID));
            continue;
        }
        executionPaths.insert(pathKey);

        // Trace node entry
        std::ostringstream nodeEntry;
        nodeEntry << "[ENTER] Step " << (stepCount + 1) << ": Node #" << nodePtr->NodeID;
        if (!nodePtr->NodeName.empty())
            nodeEntry << " '" << nodePtr->NodeName << "'";
        if (depth > 0)
            nodeEntry << " [depth:" << depth << "]";
        m_simulationTraces.push_back(nodeEntry.str());

        // Handle each node type and push next tokens
        std::vector<int32_t> nextNodeIDs;  // Multiple outputs for branching

        switch (nodePtr->Type)
        {
            case TaskNodeType::EntryPoint:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] EntryPoint - start of graph");
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeIDs.push_back(m_template.ExecConnections[i].TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] EntryPoint completed");
                break;
            }

            case TaskNodeType::GetBBValue:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] GetBBValue node");
                m_simulationTraces.push_back("  │  Key: '" + nodePtr->BBKey + "'");
                auto it = blackboard.find(nodePtr->BBKey);
                if (it != blackboard.end())
                {
                    m_simulationTraces.push_back("  │  Value: " + it->second.AsString());
                }
                else
                {
                    m_simulationTraces.push_back("  │  Value: [NOT FOUND]");
                }
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeIDs.push_back(m_template.ExecConnections[i].TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Read value from blackboard");
                break;
            }

            case TaskNodeType::SetBBValue:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] SetBBValue node");
                m_simulationTraces.push_back("  │  Key: '" + nodePtr->BBKey + "'");
                m_simulationTraces.push_back("  │  Setting value in blackboard (simulated)");
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeIDs.push_back(m_template.ExecConnections[i].TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Value written to blackboard");
                break;
            }

            case TaskNodeType::MathOp:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] MathOp node");
                m_simulationTraces.push_back("  │  Operator: '" + nodePtr->MathOperator + "'");
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
                m_simulationTraces.push_back("  │  Left: " + leftOp.str());
                m_simulationTraces.push_back("  │  Right: " + rightOp.str());
                m_simulationTraces.push_back("  │  Result: [computed]");
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeIDs.push_back(m_template.ExecConnections[i].TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Math operation executed");
                break;
            }

            case TaskNodeType::Branch:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] Branch node");
                m_simulationTraces.push_back("  │  Evaluating condition...");
                bool conditionResult = true;  // Simplified: always true
                m_simulationTraces.push_back("  │  Condition result: " + std::string(conditionResult ? "TRUE" : "FALSE"));
                const std::string targetPin = conditionResult ? "Out" : "OutElse";
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && 
                        (conn.SourcePinName == targetPin || conn.SourcePinName.empty()))
                    {
                        nextNodeIDs.push_back(conn.TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Branch taken: " + targetPin);
                break;
            }

            case TaskNodeType::Switch:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] Switch node");
                m_simulationTraces.push_back("  │  Variable: '" + nodePtr->switchVariable + "'");
                m_simulationTraces.push_back("  │  Cases: " + std::to_string(nodePtr->switchCases.size()));
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeIDs.push_back(m_template.ExecConnections[i].TargetNodeID);
                        m_simulationTraces.push_back("  │  Case selected: (first available)");
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Switch case executed");
                break;
            }

            case TaskNodeType::Delay:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] Delay node");
                m_simulationTraces.push_back("  │  Duration: " + std::to_string(nodePtr->DelaySeconds) + " seconds");
                m_simulationTraces.push_back("  │  (Delay simulated)");
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeIDs.push_back(m_template.ExecConnections[i].TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Delay completed");
                break;
            }

            case TaskNodeType::AtomicTask:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] AtomicTask node");
                m_simulationTraces.push_back("  │  Task type: '" + nodePtr->AtomicTaskID + "'");
                m_simulationTraces.push_back("  │  (Task execution simulated)");
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && conn.SourcePinName == "Completed")
                    {
                        nextNodeIDs.push_back(conn.TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Task completed");
                break;
            }

            case TaskNodeType::VSSequence:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] Sequence node");

                // Phase 24.4 — Collect ALL outgoing connections and push them as tokens (FIFO order, so reverse push)
                std::vector<ExecPinConnection> sequenceOutputs;
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        sequenceOutputs.push_back(m_template.ExecConnections[i]);
                    }
                }

                m_simulationTraces.push_back("  │  Output pins: " + std::to_string(sequenceOutputs.size()));
                m_simulationTraces.push_back("  │  Queueing " + std::to_string(sequenceOutputs.size()) + " branches:");

                // Push tokens in REVERSE order so first output is executed first (LIFO stack semantics)
                for (int oi = (int)sequenceOutputs.size() - 1; oi >= 0; --oi)
                {
                    const ExecPinConnection& outConn = sequenceOutputs[oi];
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
                    outTrace << "    ├─ Output [" << (oi + 1) << "] -> Node #" << outConn.TargetNodeID;
                    if (outNodePtr && !outNodePtr->NodeName.empty())
                        outTrace << " '" << outNodePtr->NodeName << "'";
                    m_simulationTraces.push_back(outTrace.str());

                    // Push token for this branch (with increased depth)
                    m_executionTokenStack.push_back(ExecutionToken(outConn.TargetNodeID, depth + 1));
                }

                m_simulationTraces.push_back("  └─ [RESULT] Queued " + std::to_string(sequenceOutputs.size()) + " branch tokens");
                break;
            }

            case TaskNodeType::While:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] While loop node");
                m_simulationTraces.push_back("  │  Evaluating condition...");
                m_simulationTraces.push_back("  │  Condition result: TRUE (Loop continues)");
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    const ExecPinConnection& conn = m_template.ExecConnections[i];
                    if (conn.SourceNodeID == currentNodeID && conn.SourcePinName == "Loop")
                    {
                        nextNodeIDs.push_back(conn.TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Loop iteration");
                break;
            }

            default:
            {
                m_simulationTraces.push_back("  ├─ [EVAL] Node type: " + std::to_string(static_cast<int>(nodePtr->Type)));
                for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
                {
                    if (m_template.ExecConnections[i].SourceNodeID == currentNodeID)
                    {
                        nextNodeIDs.push_back(m_template.ExecConnections[i].TargetNodeID);
                        break;
                    }
                }
                m_simulationTraces.push_back("  └─ [RESULT] Node processed");
                break;
            }
        }

        // Trace node exit and push next tokens
        if (!nextNodeIDs.empty())
        {
            for (size_t ni = 0; ni < nextNodeIDs.size(); ++ni)
            {
                m_simulationTraces.push_back("[EXIT] -> Next: Node #" + std::to_string(nextNodeIDs[ni]));
                // Push in REVERSE order (LIFO) so first is executed first
                m_executionTokenStack.push_back(ExecutionToken(nextNodeIDs[ni], depth));
            }
        }
        else
        {
            m_simulationTraces.push_back("[EXIT] -> Branch ends (no next node)");
        }

        m_simulationTraces.push_back("");  // Blank line for readability
        ++stepCount;
    }

    // Final summary
    m_simulationTraces.push_back("=== EXECUTION SUMMARY ===");
    if (stepCount >= maxSteps)
    {
        m_simulationTraces.push_back("[WARNING] Maximum steps reached (" + std::to_string(maxSteps) + ") - possible infinite loop");
    }
    else if (m_executionTokenStack.empty())
    {
        m_simulationTraces.push_back("[SUCCESS] All execution branches completed");
    }
    m_simulationTraces.push_back("Total steps executed: " + std::to_string(stepCount));
    m_simulationTraces.push_back("Blackboard entries: " + std::to_string(blackboard.size()));
    m_simulationTraces.push_back("Remaining tokens in stack: " + std::to_string(m_executionTokenStack.size()));

    m_simulationDone = true;

    SYSTEM_LOG << "[VisualScriptEditorPanel] Simulation completed: " << stepCount << " steps\n";

    // Append simulation traces to verification logs
    m_verificationLogs.push_back("");  // Separator
    m_verificationLogs.push_back("--- SIMULATION EXECUTION TRACE (Token-based) ---");
    m_verificationLogs.insert(m_verificationLogs.end(),
                              m_simulationTraces.begin(),
                              m_simulationTraces.end());
}
