/**
 * @file VSGraphExecutor.cpp
 * @brief Implémentation de VSGraphExecutor pour les graphes ATS Visual Scripting.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * C++14 compliant - no C++17/20 features.
 */

#include "VSGraphExecutor.h"
#include "AtomicTaskRegistry.h"
#include "IAtomicTask.h"
#include "../Core/AssetManager.h"
#include "../system/system_utils.h"

#include <sstream>

namespace Olympe {

// ============================================================================
// Constants
// ============================================================================

/// Maximum steps per frame to prevent infinite loops.
static const int MAX_STEPS_PER_FRAME = 64;

/// Out-of-class definition required in C++14 for ODR-used static const members.
const int VSGraphExecutor::MAX_SUBGRAPH_DEPTH;

// ============================================================================
// ExecuteFrame (public)
// ============================================================================

void VSGraphExecutor::ExecuteFrame(EntityID entity,
                                   TaskRunnerComponent& runner,
                                   const TaskGraphTemplate& tmpl,
                                   LocalBlackboard& localBB,
                                   World* worldPtr,
                                   float dt)
{
    // If no active node, reset to EntryPoint and return — execution starts next frame.
    if (runner.CurrentNodeID == NODE_INDEX_NONE)
    {
        runner.CurrentNodeID = tmpl.EntryPointID;
        return;
    }

    // Main execution loop — bounded to MAX_STEPS_PER_FRAME to prevent infinite loops.
    for (int step = 0; step < MAX_STEPS_PER_FRAME; ++step)
    {
        const int32_t currentID = runner.CurrentNodeID;

        const TaskNodeDefinition* node = tmpl.GetNode(currentID);
        if (node == nullptr)
        {
            SYSTEM_LOG << "[VSGraphExecutor] Entity " << entity
                       << ": node ID " << currentID
                       << " not found in template '" << tmpl.Name << "'\n";
            runner.CurrentNodeID = NODE_INDEX_NONE;
            return;
        }

        // Resolve incoming data pins before executing the node.
        ResolveDataPins(currentID, runner, tmpl, localBB);

        int32_t nextID = NODE_INDEX_NONE;

        switch (node->Type)
        {
            case TaskNodeType::EntryPoint:
                nextID = HandleEntryPoint(currentID, tmpl);
                break;

            case TaskNodeType::Branch:
                nextID = HandleBranch(currentID, runner, tmpl, localBB);
                break;

            case TaskNodeType::Switch:
                nextID = HandleSwitch(currentID, runner, tmpl, localBB);
                break;

            case TaskNodeType::VSSequence:
                nextID = HandleVSSequence(currentID, runner, tmpl);
                break;

            case TaskNodeType::While:
                nextID = HandleWhile(currentID, runner, tmpl, localBB);
                break;

            case TaskNodeType::DoOnce:
                nextID = HandleDoOnce(currentID, runner, tmpl);
                break;

            case TaskNodeType::Delay:
                nextID = HandleDelay(currentID, runner, tmpl, dt);
                break;

            case TaskNodeType::GetBBValue:
                nextID = HandleGetBBValue(currentID, runner, tmpl, localBB);
                break;

            case TaskNodeType::SetBBValue:
                nextID = HandleSetBBValue(currentID, runner, tmpl, localBB);
                break;

            case TaskNodeType::MathOp:
                nextID = HandleMathOp(currentID, runner, tmpl);
                break;

            case TaskNodeType::AtomicTask:
                nextID = HandleAtomicTask(entity, currentID, runner, tmpl, localBB, worldPtr, dt);
                break;

            case TaskNodeType::SubGraph:
            {
                static thread_local SubGraphCallStack s_callStack;
                nextID = HandleSubGraph(entity, currentID, runner, tmpl, localBB, worldPtr, dt, s_callStack);
                break;
            }

            default:
                // Unhandled node type: log and end frame.
                SYSTEM_LOG << "[VSGraphExecutor] Entity " << entity
                           << ": unhandled node type "
                           << static_cast<int>(node->Type)
                           << " at node " << currentID << "\n";
                return;
        }

        // End of frame: node requests to stay active (multi-frame task, delay, etc.).
        if (nextID == NODE_INDEX_NONE)
        {
            return;
        }

        // Same node returned: multi-frame node that needs to tick again next frame.
        if (nextID == runner.CurrentNodeID)
        {
            return;
        }

        // Advance to next node.
        runner.CurrentNodeID = nextID;
    }

    // Reached step limit — log a warning (likely an infinite loop in the graph).
    SYSTEM_LOG << "[VSGraphExecutor] Entity " << entity
               << ": reached MAX_STEPS_PER_FRAME (" << MAX_STEPS_PER_FRAME
               << ") in template '" << tmpl.Name << "' — possible infinite loop\n";
}

// ============================================================================
// FindExecTarget (private)
// ============================================================================

int32_t VSGraphExecutor::FindExecTarget(int32_t sourceNodeID,
                                        const std::string& pinName,
                                        const TaskGraphTemplate& tmpl)
{
    for (size_t i = 0; i < tmpl.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& conn = tmpl.ExecConnections[i];
        if (conn.SourceNodeID == sourceNodeID && conn.SourcePinName == pinName)
        {
            return conn.TargetNodeID;
        }
    }
    return NODE_INDEX_NONE;
}

// ============================================================================
// ResolveDataPins (private)
// ============================================================================

void VSGraphExecutor::ResolveDataPins(int32_t nodeID,
                                      TaskRunnerComponent& runner,
                                      const TaskGraphTemplate& tmpl,
                                      LocalBlackboard& localBB)
{
    for (size_t i = 0; i < tmpl.DataConnections.size(); ++i)
    {
        const DataPinConnection& conn = tmpl.DataConnections[i];
        if (conn.TargetNodeID != nodeID)
        {
            continue;
        }

        // Build cache key for the source output pin.
        std::ostringstream srcKey;
        srcKey << conn.SourceNodeID << ":" << conn.SourcePinName;

        const std::string srcKeyStr = srcKey.str();

        // Build cache key for the target input pin.
        std::ostringstream dstKey;
        dstKey << nodeID << ":" << conn.TargetPinName;

        const std::string dstKeyStr = dstKey.str();

        // Try to get value from source node's output pin cache.
        auto it = runner.DataPinCache.find(srcKeyStr);
        if (it != runner.DataPinCache.end())
        {
            runner.DataPinCache[dstKeyStr] = it->second;
        }
        else
        {
            // Fallback: look for the default value in the target node's data pin definition.
            const TaskNodeDefinition* targetNode = tmpl.GetNode(nodeID);
            if (targetNode != nullptr)
            {
                for (size_t p = 0; p < targetNode->DataPins.size(); ++p)
                {
                    const DataPinDefinition& pin = targetNode->DataPins[p];
                    if (pin.PinName == conn.TargetPinName && pin.Dir == DataPinDir::Input)
                    {
                        runner.DataPinCache[dstKeyStr] = pin.Default;
                        break;
                    }
                }
            }
        }
    }
}

// ============================================================================
// ReadBBValue / WriteBBValue (private)
// ============================================================================

TaskValue VSGraphExecutor::ReadBBValue(const std::string& scopedKey,
                                       LocalBlackboard& localBB)
{
    return localBB.GetValueScoped(scopedKey);
}

void VSGraphExecutor::WriteBBValue(const std::string& scopedKey,
                                   const TaskValue& value,
                                   LocalBlackboard& localBB)
{
    localBB.SetValueScoped(scopedKey, value);
}

// ============================================================================
// HandleEntryPoint (private)
// ============================================================================

int32_t VSGraphExecutor::HandleEntryPoint(int32_t nodeID,
                                          const TaskGraphTemplate& tmpl)
{
    // Immediately advance to the first exec successor.
    return FindExecTarget(nodeID, "Out", tmpl);
}

// ============================================================================
// HandleBranch (private)
// ============================================================================

int32_t VSGraphExecutor::HandleBranch(int32_t nodeID,
                                      TaskRunnerComponent& runner,
                                      const TaskGraphTemplate& tmpl,
                                      LocalBlackboard& localBB)
{
    // Build cache key for the Condition data pin.
    std::ostringstream condKey;
    condKey << nodeID << ":Condition";
    const std::string condKeyStr = condKey.str();

    bool condition = false;

    auto it = runner.DataPinCache.find(condKeyStr);
    if (it != runner.DataPinCache.end())
    {
        const TaskValue& val = it->second;
        if (val.GetType() == VariableType::Bool)
        {
            try
            {
                condition = val.AsBool();
            }
            catch (...)
            {
                condition = false;
            }
        }
        else if (val.GetType() == VariableType::Int)
        {
            try
            {
                condition = (val.AsInt() != 0);
            }
            catch (...)
            {
                condition = false;
            }
        }
    }
    else
    {
        // No data pin connected: use ConditionID as fallback (not implemented here, default false).
        const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
        if (node != nullptr && !node->ConditionID.empty())
        {
            SYSTEM_LOG << "[VSGraphExecutor] Branch node " << nodeID
                       << ": ConditionID '" << node->ConditionID
                       << "' evaluation not implemented — defaulting to Else\n";
        }
        condition = false;
    }

    if (condition)
    {
        return FindExecTarget(nodeID, "Then", tmpl);
    }
    else
    {
        return FindExecTarget(nodeID, "Else", tmpl);
    }
}

// ============================================================================
// HandleSwitch (private)
// ============================================================================

int32_t VSGraphExecutor::HandleSwitch(int32_t nodeID,
                                      TaskRunnerComponent& runner,
                                      const TaskGraphTemplate& tmpl,
                                      LocalBlackboard& localBB)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return NODE_INDEX_NONE;
    }

    // Read the value to switch on from the "Value" data pin.
    std::ostringstream valKey;
    valKey << nodeID << ":Value";
    const std::string valKeyStr = valKey.str();

    std::string caseLabel;

    auto it = runner.DataPinCache.find(valKeyStr);
    if (it != runner.DataPinCache.end())
    {
        const TaskValue& val = it->second;
        if (val.GetType() == VariableType::String)
        {
            try { caseLabel = val.AsString(); } catch (...) {}
        }
        else if (val.GetType() == VariableType::Int)
        {
            try
            {
                std::ostringstream oss;
                oss << val.AsInt();
                caseLabel = oss.str();
            }
            catch (...) {}
        }
    }

    // Try each case label.
    for (size_t i = 0; i < node->SwitchCases.size(); ++i)
    {
        if (node->SwitchCases[i] == caseLabel)
        {
            return FindExecTarget(nodeID, node->SwitchCases[i], tmpl);
        }
    }

    // No matching case: follow Default exec output if connected.
    return FindExecTarget(nodeID, "Default", tmpl);
}

// ============================================================================
// HandleVSSequence (private)
// ============================================================================

int32_t VSGraphExecutor::HandleVSSequence(int32_t nodeID,
                                          TaskRunnerComponent& runner,
                                          const TaskGraphTemplate& tmpl)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return NODE_INDEX_NONE;
    }

    if (runner.SequenceChildIndex < static_cast<int32_t>(node->ChildrenIDs.size()))
    {
        const int32_t childID = node->ChildrenIDs[static_cast<size_t>(runner.SequenceChildIndex)];
        runner.SequenceChildIndex++;
        return childID;
    }

    // All children executed: reset index and follow Out.
    runner.SequenceChildIndex = 0;
    return FindExecTarget(nodeID, "Out", tmpl);
}

// ============================================================================
// HandleWhile (private)
// ============================================================================

int32_t VSGraphExecutor::HandleWhile(int32_t nodeID,
                                     TaskRunnerComponent& runner,
                                     const TaskGraphTemplate& tmpl,
                                     LocalBlackboard& localBB)
{
    // Read condition from "Condition" data pin.
    std::ostringstream condKey;
    condKey << nodeID << ":Condition";
    const std::string condKeyStr = condKey.str();

    bool condition = false;

    auto it = runner.DataPinCache.find(condKeyStr);
    if (it != runner.DataPinCache.end())
    {
        const TaskValue& val = it->second;
        if (val.GetType() == VariableType::Bool)
        {
            try { condition = val.AsBool(); } catch (...) {}
        }
        else if (val.GetType() == VariableType::Int)
        {
            try { condition = (val.AsInt() != 0); } catch (...) {}
        }
    }
    else
    {
        const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
        if (node != nullptr && !node->ConditionID.empty())
        {
            SYSTEM_LOG << "[VSGraphExecutor] While node " << nodeID
                       << ": ConditionID '" << node->ConditionID
                       << "' evaluation not implemented — exiting loop\n";
        }
        condition = false;
    }

    if (condition)
    {
        return FindExecTarget(nodeID, "Loop", tmpl);
    }
    else
    {
        return FindExecTarget(nodeID, "Completed", tmpl);
    }
}

// ============================================================================
// HandleDoOnce (private)
// ============================================================================

int32_t VSGraphExecutor::HandleDoOnce(int32_t nodeID,
                                      TaskRunnerComponent& runner,
                                      const TaskGraphTemplate& tmpl)
{
    auto it = runner.DoOnceFlags.find(nodeID);
    if (it != runner.DoOnceFlags.end() && it->second)
    {
        // Already fired — block execution.
        return NODE_INDEX_NONE;
    }

    // Mark as fired and execute.
    runner.DoOnceFlags[nodeID] = true;
    return FindExecTarget(nodeID, "Out", tmpl);
}

// ============================================================================
// HandleDelay (private)
// ============================================================================

int32_t VSGraphExecutor::HandleDelay(int32_t nodeID,
                                     TaskRunnerComponent& runner,
                                     const TaskGraphTemplate& tmpl,
                                     float dt)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return NODE_INDEX_NONE;
    }

    runner.StateTimer += dt;

    if (runner.StateTimer >= node->DelaySeconds)
    {
        runner.StateTimer = 0.0f;
        return FindExecTarget(nodeID, "Completed", tmpl);
    }

    // Still waiting — remain on this node, end frame.
    return runner.CurrentNodeID;
}

// ============================================================================
// HandleGetBBValue (private)
// ============================================================================

int32_t VSGraphExecutor::HandleGetBBValue(int32_t nodeID,
                                          TaskRunnerComponent& runner,
                                          const TaskGraphTemplate& tmpl,
                                          LocalBlackboard& localBB)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return NODE_INDEX_NONE;
    }

    const TaskValue val = ReadBBValue(node->BBKey, localBB);

    // Store result in output pin cache.
    std::ostringstream outKey;
    outKey << nodeID << ":Value";
    runner.DataPinCache[outKey.str()] = val;

    return FindExecTarget(nodeID, "Out", tmpl);
}

// ============================================================================
// HandleSetBBValue (private)
// ============================================================================

int32_t VSGraphExecutor::HandleSetBBValue(int32_t nodeID,
                                          TaskRunnerComponent& runner,
                                          const TaskGraphTemplate& tmpl,
                                          LocalBlackboard& localBB)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return NODE_INDEX_NONE;
    }

    // Read input value from data pin cache.
    std::ostringstream inKey;
    inKey << nodeID << ":Value";
    const std::string inKeyStr = inKey.str();

    auto it = runner.DataPinCache.find(inKeyStr);
    if (it != runner.DataPinCache.end())
    {
        WriteBBValue(node->BBKey, it->second, localBB);
    }
    else
    {
        SYSTEM_LOG << "[VSGraphExecutor] SetBBValue node " << nodeID
                   << ": no value in DataPinCache for 'Value' pin — skipping write\n";
    }

    return FindExecTarget(nodeID, "Out", tmpl);
}

// ============================================================================
// HandleMathOp (private)
// ============================================================================

int32_t VSGraphExecutor::HandleMathOp(int32_t nodeID,
                                      TaskRunnerComponent& runner,
                                      const TaskGraphTemplate& tmpl)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return NODE_INDEX_NONE;
    }

    // Read operands A and B from data pin cache.
    std::ostringstream keyA;
    keyA << nodeID << ":A";
    std::ostringstream keyB;
    keyB << nodeID << ":B";

    float a = 0.0f;
    float b = 0.0f;

    auto itA = runner.DataPinCache.find(keyA.str());
    if (itA != runner.DataPinCache.end())
    {
        const TaskValue& v = itA->second;
        if (v.GetType() == VariableType::Float)
        {
            try { a = v.AsFloat(); } catch (...) {}
        }
        else if (v.GetType() == VariableType::Int)
        {
            try { a = static_cast<float>(v.AsInt()); } catch (...) {}
        }
    }

    auto itB = runner.DataPinCache.find(keyB.str());
    if (itB != runner.DataPinCache.end())
    {
        const TaskValue& v = itB->second;
        if (v.GetType() == VariableType::Float)
        {
            try { b = v.AsFloat(); } catch (...) {}
        }
        else if (v.GetType() == VariableType::Int)
        {
            try { b = static_cast<float>(v.AsInt()); } catch (...) {}
        }
    }

    float result = 0.0f;
    const std::string& op = node->MathOperator;

    if (op == "+")
    {
        result = a + b;
    }
    else if (op == "-")
    {
        result = a - b;
    }
    else if (op == "*")
    {
        result = a * b;
    }
    else if (op == "/")
    {
        if (b != 0.0f)
        {
            result = a / b;
        }
        else
        {
            SYSTEM_LOG << "[VSGraphExecutor] MathOp node " << nodeID
                       << ": division by zero — result is 0\n";
            result = 0.0f;
        }
    }
    else
    {
        SYSTEM_LOG << "[VSGraphExecutor] MathOp node " << nodeID
                   << ": unknown operator '" << op << "'\n";
    }

    // Store result in output pin cache.
    std::ostringstream resultKey;
    resultKey << nodeID << ":Result";
    runner.DataPinCache[resultKey.str()] = TaskValue(result);

    return FindExecTarget(nodeID, "Out", tmpl);
}

// ============================================================================
// HandleAtomicTask (private)
// ============================================================================

int32_t VSGraphExecutor::HandleAtomicTask(EntityID entity,
                                          int32_t nodeID,
                                          TaskRunnerComponent& runner,
                                          const TaskGraphTemplate& tmpl,
                                          LocalBlackboard& localBB,
                                          World* worldPtr,
                                          float dt)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return NODE_INDEX_NONE;
    }

    // Create task instance on first entry.
    if (!runner.activeTask)
    {
        runner.activeTask = AtomicTaskRegistry::Get().Create(node->AtomicTaskID);
        if (!runner.activeTask)
        {
            SYSTEM_LOG << "[VSGraphExecutor] Entity " << entity
                       << ": unknown AtomicTaskID '" << node->AtomicTaskID
                       << "' at node " << nodeID << " — skipping\n";
            return FindExecTarget(nodeID, "Out", tmpl);
        }
    }

    // Build parameter map from data pin cache and node parameter bindings.
    IAtomicTask::ParameterMap params;

    for (auto it = node->Parameters.begin(); it != node->Parameters.end(); ++it)
    {
        if (it->second.Type == ParameterBindingType::Literal)
        {
            params[it->first] = it->second.LiteralValue;
        }
        else if (it->second.Type == ParameterBindingType::LocalVariable)
        {
            if (localBB.HasVariable(it->second.VariableName))
            {
                try
                {
                    params[it->first] = localBB.GetValue(it->second.VariableName);
                }
                catch (...)
                {
                    SYSTEM_LOG << "[VSGraphExecutor] Entity " << entity
                               << ": failed to read variable '"
                               << it->second.VariableName << "'\n";
                }
            }
        }
    }

    // Also inject data pin cache values for input pins.
    for (size_t p = 0; p < node->DataPins.size(); ++p)
    {
        const DataPinDefinition& pin = node->DataPins[p];
        if (pin.Dir == DataPinDir::Input)
        {
            std::ostringstream key;
            key << nodeID << ":" << pin.PinName;
            auto it = runner.DataPinCache.find(key.str());
            if (it != runner.DataPinCache.end())
            {
                params[pin.PinName] = it->second;
            }
        }
    }

    // Build context and tick.
    // Note: worldPtr may be nullptr (valid in tests and Phase 2).
    // AtomicTaskContext documents that WorldPtr may be null; concrete tasks
    // that need World access must guard against null themselves.
    AtomicTaskContext ctx;
    ctx.Entity     = entity;
    ctx.WorldPtr   = worldPtr;
    ctx.LocalBB    = &localBB;
    ctx.DeltaTime  = dt;
    ctx.StateTimer = runner.StateTimer;

    TaskStatus status = runner.activeTask->ExecuteWithContext(ctx, params);

    if (status == TaskStatus::Running)
    {
        runner.StateTimer += dt;
        // Remain on this node — end frame.
        return runner.CurrentNodeID;
    }

    // Task completed: clean up.
    runner.activeTask.reset();
    runner.StateTimer = 0.0f;

    if (status == TaskStatus::Success)
    {
        runner.LastStatus = TaskRunnerComponent::TaskStatus::Success;
        return FindExecTarget(nodeID, "Out", tmpl);
    }
    else
    {
        runner.LastStatus = TaskRunnerComponent::TaskStatus::Failure;
        const int32_t failTarget = FindExecTarget(nodeID, "Failure", tmpl);
        return failTarget;  // NODE_INDEX_NONE if not connected is intentional
    }
}

// ============================================================================
// HandleSubGraph (private)
// ============================================================================

int32_t VSGraphExecutor::HandleSubGraph(EntityID entity,
                                        int32_t nodeID,
                                        TaskRunnerComponent& runner,
                                        const TaskGraphTemplate& tmpl,
                                        LocalBlackboard& localBB,
                                        World* worldPtr,
                                        float dt,
                                        SubGraphCallStack& callStack)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        SYSTEM_LOG << "[VSGraphExecutor][ERROR] HandleSubGraph: node " << nodeID
                   << " not found in template '" << tmpl.Name << "'\n";
        return NODE_INDEX_NONE;
    }

    if (node->SubGraphPath.empty())
    {
        SYSTEM_LOG << "[VSGraphExecutor] Entity " << entity
                   << ": SubGraph node " << nodeID
                   << " has no SubGraphPath — skipping\n";
        return FindExecTarget(nodeID, "Out", tmpl);
    }

    // 1. Check depth limit
    if (callStack.Depth >= MAX_SUBGRAPH_DEPTH)
    {
        SYSTEM_LOG << "[VSGraphExecutor][ERROR] SubGraph recursion depth exceeded (max "
                   << MAX_SUBGRAPH_DEPTH << "): '" << node->SubGraphPath << "'\n";
        return FindExecTarget(nodeID, "Out", tmpl);
    }

    // 2. Check for cycles (self-reference or circular dependency)
    if (callStack.Contains(node->SubGraphPath))
    {
        SYSTEM_LOG << "[VSGraphExecutor][ERROR] SubGraph cycle detected: '";
        for (size_t i = 0; i < callStack.PathStack.size(); ++i)
        {
            SYSTEM_LOG << callStack.PathStack[i];
            if (i + 1 < callStack.PathStack.size()) SYSTEM_LOG << " -> ";
        }
        SYSTEM_LOG << " -> " << node->SubGraphPath << "'\n";
        return FindExecTarget(nodeID, "Out", tmpl);
    }

    // 3. Load SubGraph template
    std::vector<std::string> loadErrors;
    const TaskGraphTemplate* subGraphTmpl =
        AssetManager::Get().LoadTaskGraphFromFile(node->SubGraphPath, loadErrors);

    if (subGraphTmpl == nullptr)
    {
        SYSTEM_LOG << "[VSGraphExecutor][ERROR] SubGraph file not found or invalid: '"
                   << node->SubGraphPath << "'\n";
        for (size_t i = 0; i < loadErrors.size(); ++i)
        {
            SYSTEM_LOG << "  - " << loadErrors[i] << "\n";
        }
        return FindExecTarget(nodeID, "Out", tmpl);
    }

    // 4. Create child LocalBlackboard and bind input parameters
    LocalBlackboard childBB;
    childBB.InitializeFromEntries(subGraphTmpl->Blackboard);

    for (auto it = node->InputParams.begin(); it != node->InputParams.end(); ++it)
    {
        const std::string& paramName = it->first;
        const ParameterBinding& binding = it->second;

        TaskValue value;
        if (binding.Type == ParameterBindingType::Literal)
        {
            value = binding.LiteralValue;
        }
        else if (binding.Type == ParameterBindingType::LocalVariable)
        {
            if (localBB.HasVariable(binding.VariableName))
            {
                value = localBB.GetValue(binding.VariableName);
            }
            else
            {
                SYSTEM_LOG << "[VSGraphExecutor] SubGraph input param '" << paramName
                           << "': LocalVariable '" << binding.VariableName
                           << "' not found in parent BB — using default\n";
            }
        }

        if (childBB.HasVariable(paramName))
        {
            try { childBB.SetValue(paramName, value); }
            catch (...) { /* type mismatch — skip */ }
        }
    }

    // 5. Push call stack
    callStack.Push(node->SubGraphPath);

    // 6. Create temporary TaskRunnerComponent for subgraph execution
    TaskRunnerComponent childRunner;
    childRunner.CurrentNodeID = subGraphTmpl->EntryPointID;

    // 7. Execute SubGraph (single frame step)
    ExecuteFrame(entity, childRunner, *subGraphTmpl, childBB, worldPtr, dt);

    // 8. Pop call stack
    callStack.Pop();

    // 9. Extract output parameters back to parent blackboard
    for (auto it = node->OutputParams.begin(); it != node->OutputParams.end(); ++it)
    {
        const std::string& paramName = it->first;
        const std::string& targetBBKey = it->second;

        if (childBB.HasVariable(paramName))
        {
            try
            {
                TaskValue outputValue = childBB.GetValue(paramName);
                localBB.SetValueScoped(targetBBKey, outputValue);
            }
            catch (...)
            {
                SYSTEM_LOG << "[VSGraphExecutor] SubGraph output param '" << paramName
                           << "': failed to write to '" << targetBBKey << "'\n";
            }
        }
    }

    // 10. Continue execution in parent graph
    return FindExecTarget(nodeID, "Out", tmpl);
}

} // namespace Olympe
