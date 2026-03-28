/**
 * @file DataPinEvaluator_inline.h
 * @brief Inline implementation of DataPinEvaluator.
 * @author Olympe Engine
 * @date 2026-03-20
 */

#pragma once

#include "TaskGraphTemplate.h"
#include "LocalBlackboard.h"
#include "../ECS/Components/TaskRunnerComponent.h"
#include "../BlueprintEditor/MathOpOperand.h"
#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Helper inline functions
// ============================================================================

inline float DataPinEvaluator_TaskValueToFloat(const TaskValue& val)
{
    VariableType type = val.GetType();

    if (type == VariableType::Float)
    {
        return val.AsFloat();
    }
    else if (type == VariableType::Int)
    {
        return static_cast<float>(val.AsInt());
    }
    else if (type == VariableType::Bool)
    {
        return val.AsBool() ? 1.0f : 0.0f;
    }
    else
    {
        throw std::runtime_error("Cannot convert TaskValue type to float for math operation");
    }
}

inline TaskValue DataPinEvaluator_ComputeMath(
    const std::string& op,
    float left,
    float right)
{
    float result = 0.0f;

    if (op == "+")
    {
        result = left + right;
    }
    else if (op == "-")
    {
        result = left - right;
    }
    else if (op == "*")
    {
        result = left * right;
    }
    else if (op == "/")
    {
        if (std::fabs(right) < std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("Division by zero in MathOp");
        }
        result = left / right;
    }
    else if (op == "%")
    {
        if (std::fabs(right) < std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("Modulo by zero in MathOp");
        }
        result = std::fmod(left, right);
    }
    else if (op == "^")
    {
        result = std::pow(left, right);
    }
    else
    {
        throw std::runtime_error("Unknown math operator: " + op);
    }

    return TaskValue(result);
}

// Forward declaration for recursion
inline DataPinEvaluationResult DataPinEvaluator_EvaluateRecursive(
    int32_t nodeID,
    const std::string& pinName,
    const TaskGraphTemplate& tmpl,
    TaskRunnerComponent& runner,
    LocalBlackboard& localBB,
    std::unordered_set<int32_t>& visitedNodes,
    int32_t recursionDepth);

// ============================================================================
// Main public API
// ============================================================================

inline bool DataPinEvaluator::EvaluateNodeInputPins(
    int32_t nodeID,
    const TaskGraphTemplate& tmpl,
    TaskRunnerComponent& runner,
    LocalBlackboard& localBB)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        return false;
    }

    // Iterate through all data connections targeting this node
    for (size_t i = 0; i < tmpl.DataConnections.size(); ++i)
    {
        const DataPinConnection& conn = tmpl.DataConnections[i];
        if (conn.TargetNodeID != nodeID)
        {
            continue;
        }

        // Evaluate the source node's output pin
        std::unordered_set<int32_t> visitedNodes;
        DataPinEvaluationResult result = DataPinEvaluator_EvaluateRecursive(
            conn.SourceNodeID,
            conn.SourcePinName,
            tmpl,
            runner,
            localBB,
            visitedNodes,
            0);

        if (result.Status != DataPinEvalStatus::Success)
        {
            SYSTEM_LOG << "[DataPinEvaluator] Failed to evaluate pin: "
                       << conn.SourceNodeID << ":" << conn.SourcePinName
                       << " -> " << nodeID << ":" << conn.TargetPinName
                       << " (Status: " << static_cast<int>(result.Status) << ")\n";
            return false;
        }

        // Cache the result for the target input pin
        std::ostringstream dstKey;
        dstKey << nodeID << ":" << conn.TargetPinName;
        runner.DataPinCache[dstKey.str()] = result.Value;
    }

    return true;
}

// ============================================================================
// Recursive evaluation implementation
// ============================================================================

inline DataPinEvaluationResult DataPinEvaluator_EvaluateRecursive(
    int32_t nodeID,
    const std::string& pinName,
    const TaskGraphTemplate& tmpl,
    TaskRunnerComponent& runner,
    LocalBlackboard& localBB,
    std::unordered_set<int32_t>& visitedNodes,
    int32_t recursionDepth)
{
    // Check recursion depth limit
    if (recursionDepth > DataPinEvaluator::MAX_RECURSION_DEPTH)
    {
        DataPinEvaluationResult result;
        result.Status = DataPinEvalStatus::EvaluationError;
        result.ErrorMessage = "Maximum recursion depth exceeded in data pin network";
        return result;
    }

    // Check visited nodes for cycle detection
    if (visitedNodes.find(nodeID) != visitedNodes.end())
    {
        DataPinEvaluationResult result;
        result.Status = DataPinEvalStatus::CycleDetected;
        result.ErrorMessage = "Circular dependency detected in data pin network";
        return result;
    }

    // Mark node as visited
    visitedNodes.insert(nodeID);

    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (node == nullptr)
    {
        DataPinEvaluationResult result;
        result.Status = DataPinEvalStatus::InvalidNode;
        result.ErrorMessage = "Node ID not found in template";
        visitedNodes.erase(nodeID);
        return result;
    }

    // Check cache first
    std::ostringstream cacheKey;
    cacheKey << nodeID << ":" << pinName;
    const std::string cacheKeyStr = cacheKey.str();

    auto cacheIt = runner.DataPinCache.find(cacheKeyStr);
    if (cacheIt != runner.DataPinCache.end())
    {
        DataPinEvaluationResult result;
        result.Status = DataPinEvalStatus::Success;
        result.Value = cacheIt->second;
        visitedNodes.erase(nodeID);
        return result;
    }

    // Dispatch to node-specific handler
    DataPinEvaluationResult result;

    switch (node->Type)
    {
        case TaskNodeType::GetBBValue:
        {
            if (node->BBKey.empty())
            {
                result.Status = DataPinEvalStatus::InvalidNode;
                result.ErrorMessage = "GetBBValue node missing BBKey";
            }
            else
            {
                try
                {
                    result.Value = localBB.GetValueScoped(node->BBKey);
                    result.Status = DataPinEvalStatus::Success;
                }
                catch (const std::exception& e)
                {
                    result.Status = DataPinEvalStatus::EvaluationError;
                    result.ErrorMessage = std::string("Failed to read blackboard value: ") + e.what();
                }
            }
            break;
        }

        case TaskNodeType::MathOp:
        {
            if (node->MathOperator.empty())
            {
                result.Status = DataPinEvalStatus::InvalidNode;
                result.ErrorMessage = "MathOp node missing MathOperator";
            }
            else
            {
                try
                {
                    const MathOpRef& mathOpRef = node->mathOpRef;
                    TaskValue leftOperand;
                    TaskValue rightOperand;

                    // Resolve left operand
                    if (mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const)
                    {
                        try {
                            float constVal = std::stof(mathOpRef.leftOperand.constValue);
                            leftOperand = TaskValue(constVal);
                        } catch (...) {
                            leftOperand = TaskValue(0.0f);
                        }
                    }
                    else if (mathOpRef.leftOperand.mode == MathOpOperand::Mode::Pin)
                    {
                        bool found = false;
                        for (size_t i = 0; i < tmpl.DataConnections.size(); ++i)
                        {
                            const DataPinConnection& conn = tmpl.DataConnections[i];
                            if (conn.TargetNodeID == nodeID && conn.TargetPinName == "A")
                            {
                                DataPinEvaluationResult srcResult = DataPinEvaluator_EvaluateRecursive(
                                    conn.SourceNodeID,
                                    conn.SourcePinName,
                                    tmpl,
                                    runner,
                                    localBB,
                                    visitedNodes,
                                    recursionDepth + 1);

                                if (srcResult.Status != DataPinEvalStatus::Success)
                                {
                                    result = srcResult;
                                    result.ErrorMessage = "Failed to evaluate left operand: " + srcResult.ErrorMessage;
                                    visitedNodes.erase(nodeID);
                                    return result;
                                }
                                leftOperand = srcResult.Value;
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            result.Status = DataPinEvalStatus::EvaluationError;
                            result.ErrorMessage = "No data connection found for MathOp left operand (Pin mode)";
                            visitedNodes.erase(nodeID);
                            return result;
                        }
                    }
                    else if (mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable)
                    {
                        leftOperand = localBB.GetValueScoped(mathOpRef.leftOperand.variableName);
                    }

                    // Resolve right operand
                    if (mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const)
                    {
                        try {
                            float constVal = std::stof(mathOpRef.rightOperand.constValue);
                            rightOperand = TaskValue(constVal);
                        } catch (...) {
                            rightOperand = TaskValue(0.0f);
                        }
                    }
                    else if (mathOpRef.rightOperand.mode == MathOpOperand::Mode::Pin)
                    {
                        bool found = false;
                        for (size_t i = 0; i < tmpl.DataConnections.size(); ++i)
                        {
                            const DataPinConnection& conn = tmpl.DataConnections[i];
                            if (conn.TargetNodeID == nodeID && conn.TargetPinName == "B")
                            {
                                DataPinEvaluationResult srcResult = DataPinEvaluator_EvaluateRecursive(
                                    conn.SourceNodeID,
                                    conn.SourcePinName,
                                    tmpl,
                                    runner,
                                    localBB,
                                    visitedNodes,
                                    recursionDepth + 1);

                                if (srcResult.Status != DataPinEvalStatus::Success)
                                {
                                    result = srcResult;
                                    result.ErrorMessage = "Failed to evaluate right operand: " + srcResult.ErrorMessage;
                                    visitedNodes.erase(nodeID);
                                    return result;
                                }
                                rightOperand = srcResult.Value;
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            result.Status = DataPinEvalStatus::EvaluationError;
                            result.ErrorMessage = "No data connection found for MathOp right operand (Pin mode)";
                            visitedNodes.erase(nodeID);
                            return result;
                        }
                    }
                    else if (mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable)
                    {
                        rightOperand = localBB.GetValueScoped(mathOpRef.rightOperand.variableName);
                    }

                    // Compute result
                    float leftVal = DataPinEvaluator_TaskValueToFloat(leftOperand);
                    float rightVal = DataPinEvaluator_TaskValueToFloat(rightOperand);
                    result.Value = DataPinEvaluator_ComputeMath(node->MathOperator, leftVal, rightVal);
                    result.Status = DataPinEvalStatus::Success;
                }
                catch (const std::exception& e)
                {
                    result.Status = DataPinEvalStatus::EvaluationError;
                    result.ErrorMessage = std::string("Math operation failed: ") + e.what();
                }
            }
            break;
        }

        default:
            result.Status = DataPinEvalStatus::EvaluationError;
            result.ErrorMessage = "Node type does not support data pin evaluation";
            break;
    }

    // Cache the result if successful
    if (result.Status == DataPinEvalStatus::Success)
    {
        runner.DataPinCache[cacheKeyStr] = result.Value;
    }

    visitedNodes.erase(nodeID);
    return result;
}

} // namespace Olympe

