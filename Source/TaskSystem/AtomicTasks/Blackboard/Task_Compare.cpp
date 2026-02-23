/**
 * @file Task_Compare.cpp
 * @brief Implementation of Task_Compare atomic task.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Compares operands A and B using the operator in "Op".  Numeric promotions
 * allow mixing Float and Int operands.  Returns Success if the comparison
 * holds, Failure otherwise.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_Compare.h"
#include "../../AtomicTaskRegistry.h"
#include "../../../system/system_utils.h"

#include <cmath>

namespace Olympe {

// ============================================================================
// Execute (legacy path)
// ============================================================================

TaskStatus Task_Compare::Execute(const ParameterMap& params)
{
    // Context is not required for compare, so delegate to the no-context path.
    AtomicTaskContext dummy;
    return ExecuteWithContext(dummy, params);
}

// ============================================================================
// ExecuteWithContext
// ============================================================================

TaskStatus Task_Compare::ExecuteWithContext(const AtomicTaskContext& ctx,
                                             const ParameterMap& params)
{
    (void)ctx;

    // Read operator.
    std::string op = "==";
    {
        auto it = params.find("Op");
        if (it != params.end() && it->second.GetType() == VariableType::String)
            op = it->second.AsString();
    }

    // Read A.
    auto itA = params.find("A");
    if (itA == params.end())
    {
        SYSTEM_LOG << "[Task_Compare] Missing parameter 'A'\n";
        return TaskStatus::Failure;
    }

    // Read B.
    auto itB = params.find("B");
    if (itB == params.end())
    {
        SYSTEM_LOG << "[Task_Compare] Missing parameter 'B'\n";
        return TaskStatus::Failure;
    }

    const TaskValue& a = itA->second;
    const TaskValue& b = itB->second;

    // ------------------------------------------------------------------
    // String comparison
    // ------------------------------------------------------------------

    if (a.GetType() == VariableType::String && b.GetType() == VariableType::String)
    {
        const std::string sa = a.AsString();
        const std::string sb = b.AsString();

        bool result = false;
        if (op == "==")       result = (sa == sb);
        else if (op == "<")   result = (sa <  sb);
        else if (op == ">")   result = (sa >  sb);
        else
        {
            SYSTEM_LOG << "[Task_Compare] Unknown operator '" << op << "'\n";
            return TaskStatus::Failure;
        }

        return result ? TaskStatus::Success : TaskStatus::Failure;
    }

    // ------------------------------------------------------------------
    // Numeric comparison (Float or Int, with promotion)
    // ------------------------------------------------------------------

    float fa = 0.0f;
    float fb = 0.0f;
    bool numericA = false;
    bool numericB = false;

    if (a.GetType() == VariableType::Float)  { fa = a.AsFloat(); numericA = true; }
    else if (a.GetType() == VariableType::Int) { fa = static_cast<float>(a.AsInt()); numericA = true; }

    if (b.GetType() == VariableType::Float)  { fb = b.AsFloat(); numericB = true; }
    else if (b.GetType() == VariableType::Int) { fb = static_cast<float>(b.AsInt()); numericB = true; }

    if (numericA && numericB)
    {
        static const float kEpsilon = 1e-6f;
        bool result = false;
        if (op == "==")       result = (std::abs(fa - fb) <= kEpsilon);
        else if (op == "<")   result = (fa <  fb);
        else if (op == ">")   result = (fa >  fb);
        else
        {
            SYSTEM_LOG << "[Task_Compare] Unknown operator '" << op << "'\n";
            return TaskStatus::Failure;
        }

        return result ? TaskStatus::Success : TaskStatus::Failure;
    }

    SYSTEM_LOG << "[Task_Compare] Incompatible or unsupported types for comparison\n";
    return TaskStatus::Failure;
}

// ============================================================================
// Abort
// ============================================================================

void Task_Compare::Abort()
{
    SYSTEM_LOG << "[Task_Compare] Abort() called\n";
}

REGISTER_ATOMIC_TASK(Task_Compare, "Task_Compare")

} // namespace Olympe
