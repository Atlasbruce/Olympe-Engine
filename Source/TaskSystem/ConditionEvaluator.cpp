/**
 * @file ConditionEvaluator.cpp
 * @brief Implements structured Condition evaluation for Branch/While nodes.
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionEvaluator.h"
#include "../system/system_utils.h"

#include <sstream>
#include <cstring>   // std::sscanf
#include <cmath>     // std::fabs

namespace Olympe {

// ============================================================================
// EvaluateAll
// ============================================================================

bool ConditionEvaluator::EvaluateAll(
    const std::vector<Condition>& conditions,
    const LocalBlackboard& localBB,
    const std::unordered_map<std::string, TaskValue>& dataPinCache,
    int32_t nodeID)
{
    // Empty list -> implicit true (no conditions = always pass)
    if (conditions.empty())
        return true;

    for (size_t i = 0; i < conditions.size(); ++i)
    {
        if (!EvaluateCondition(conditions[i], localBB, dataPinCache, nodeID))
            return false;
    }
    return true;
}

// ============================================================================
// EvaluateCondition
// ============================================================================

bool ConditionEvaluator::EvaluateCondition(
    const Condition& cond,
    const LocalBlackboard& localBB,
    const std::unordered_map<std::string, TaskValue>& dataPinCache,
    int32_t nodeID)
{
    TaskValue leftVal;
    TaskValue rightVal;

    if (!GetConditionValue(cond.leftMode, cond.leftPin, cond.leftVariable,
                           cond.leftConstValue, localBB, dataPinCache, nodeID, leftVal))
    {
        return false;
    }

    if (!GetConditionValue(cond.rightMode, cond.rightPin, cond.rightVariable,
                           cond.rightConstValue, localBB, dataPinCache, nodeID, rightVal))
    {
        return false;
    }

    if (cond.operatorStr.empty())
    {
        SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                   << ": operator is empty — defaulting to false\n";
        return false;
    }

    return CompareValues(leftVal, rightVal, cond.operatorStr, nodeID);
}

// ============================================================================
// GetConditionValue
// ============================================================================

bool ConditionEvaluator::GetConditionValue(
    const std::string& mode,
    const std::string& pin,
    const std::string& variable,
    const TaskValue& constValue,
    const LocalBlackboard& localBB,
    const std::unordered_map<std::string, TaskValue>& dataPinCache,
    int32_t nodeID,
    TaskValue& outValue)
{
    if (mode == "Const")
    {
        // Const is always available — return the literal value directly.
        outValue = constValue;
        return true;
    }
    else if (mode == "Variable")
    {
        if (variable.empty())
        {
            SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                       << ": WARNING — Variable mode with empty variable name\n";
            return false;
        }
        if (!localBB.HasVariable(variable))
        {
            SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                       << ": WARNING — Variable '" << variable
                       << "' not found in blackboard\n";
            return false;
        }
        try
        {
            outValue = localBB.GetValue(variable);
            return true;
        }
        catch (...)
        {
            SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                       << ": WARNING — Failed to read variable '" << variable << "'\n";
            return false;
        }
    }
    else if (mode == "Pin")
    {
        if (pin.empty())
        {
            SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                       << ": WARNING — Pin mode with empty pin reference\n";
            return false;
        }

        // Parse "Node#42.Out" -> cacheKey = "42:Out"
        int32_t srcNodeID = -1;
        std::string pinName;
        if (!ParsePinRef(pin, srcNodeID, pinName))
        {
            SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                       << ": WARNING — Could not parse pin reference '" << pin << "'\n";
            return false;
        }

        std::ostringstream key;
        key << srcNodeID << ":" << pinName;
        const std::string cacheKey = key.str();

        auto it = dataPinCache.find(cacheKey);
        if (it == dataPinCache.end())
        {
            SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                       << ": WARNING — Pin '" << pin
                       << "' (cache key '" << cacheKey << "') not found in DataPinCache\n";
            return false;
        }

        outValue = it->second;
        return true;
    }

    // Unknown mode
    SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
               << ": WARNING — Unknown condition mode '" << mode << "'\n";
    return false;
}

// ============================================================================
// CompareValues
// ============================================================================

bool ConditionEvaluator::CompareValues(const TaskValue& left,
                                       const TaskValue& right,
                                       const std::string& op,
                                       int32_t nodeID)
{
    const VariableType lt = left.GetType();
    const VariableType rt = right.GetType();

    // --- Bool comparisons ---
    if (lt == VariableType::Bool && rt == VariableType::Bool)
    {
        const bool lv = left.AsBool();
        const bool rv = right.AsBool();
        if (op == "==") return lv == rv;
        if (op == "!=") return lv != rv;
        if (op == "<")  return (!lv && rv);   // false < true
        if (op == ">")  return (lv && !rv);
        if (op == "<=") return lv <= rv;
        if (op == ">=") return lv >= rv;
        SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                   << ": WARNING — Unknown operator '" << op << "' for Bool\n";
        return false;
    }

    // --- String comparisons ---
    if (lt == VariableType::String && rt == VariableType::String)
    {
        const std::string lv = left.AsString();
        const std::string rv = right.AsString();
        if (op == "==") return lv == rv;
        if (op == "!=") return lv != rv;
        if (op == "<")  return lv < rv;
        if (op == ">")  return lv > rv;
        if (op == "<=") return lv <= rv;
        if (op == ">=") return lv >= rv;
        SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                   << ": WARNING — Unknown operator '" << op << "' for String\n";
        return false;
    }

    // --- Float comparisons (or Int promoted to Float) ---
    bool leftIsNumeric  = (lt == VariableType::Int  || lt == VariableType::Float);
    bool rightIsNumeric = (rt == VariableType::Int  || rt == VariableType::Float);

    if (leftIsNumeric && rightIsNumeric)
    {
        float lv = 0.0f;
        float rv = 0.0f;

        if (lt == VariableType::Int)   lv = static_cast<float>(left.AsInt());
        else                            lv = left.AsFloat();

        if (rt == VariableType::Int)   rv = static_cast<float>(right.AsInt());
        else                            rv = right.AsFloat();

        if (op == "==") return std::fabs(lv - rv) < 1e-6f;
        if (op == "!=") return std::fabs(lv - rv) >= 1e-6f;
        if (op == "<")  return lv < rv;
        if (op == ">")  return lv > rv;
        if (op == "<=") return lv <= rv;
        if (op == ">=") return lv >= rv;
        SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                   << ": WARNING — Unknown operator '" << op << "' for numeric\n";
        return false;
    }

    // --- Vector comparisons (== and != only) ---
    if (lt == VariableType::Vector && rt == VariableType::Vector)
    {
        const ::Vector lv = left.AsVector();
        const ::Vector rv = right.AsVector();
        const bool eq = (std::fabs(lv.x - rv.x) < 1e-6f &&
                         std::fabs(lv.y - rv.y) < 1e-6f &&
                         std::fabs(lv.z - rv.z) < 1e-6f);
        if (op == "==") return eq;
        if (op == "!=") return !eq;
        SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
                   << ": WARNING — Operator '" << op
                   << "' not supported for Vector (only == and != are valid)\n";
        return false;
    }

    // --- Type mismatch ---
    SYSTEM_LOG << "[ConditionEvaluator] Node #" << nodeID
               << ": WARNING — Type mismatch: cannot compare type "
               << static_cast<int>(lt) << " with type "
               << static_cast<int>(rt)
               << " using operator '" << op << "' (E042)\n";
    return false;
}

// ============================================================================
// ParsePinRef (private helper)
// ============================================================================

bool ConditionEvaluator::ParsePinRef(const std::string& pinRef,
                                     int32_t& outNodeID,
                                     std::string& outPinName)
{
    // Expected format: "Node#<id>.<pinName>"
    // e.g. "Node#42.Out"

    const std::string prefix = "Node#";
    if (pinRef.size() <= prefix.size())
        return false;

    if (pinRef.substr(0, prefix.size()) != prefix)
        return false;

    const std::string rest = pinRef.substr(prefix.size()); // "42.Out"
    const size_t dotPos = rest.find('.');
    if (dotPos == std::string::npos || dotPos == 0 || dotPos + 1 >= rest.size())
        return false;

    const std::string idStr   = rest.substr(0, dotPos);
    const std::string pinName = rest.substr(dotPos + 1);

    // Convert id string to integer
    int id = 0;
    for (size_t i = 0; i < idStr.size(); ++i)
    {
        if (idStr[i] < '0' || idStr[i] > '9')
            return false;
        id = id * 10 + (idStr[i] - '0');
    }

    outNodeID  = static_cast<int32_t>(id);
    outPinName = pinName;
    return true;
}

} // namespace Olympe
