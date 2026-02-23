/**
 * @file Task_Compare.cpp
 * @brief Atomic task: compare two values and return Success or Failure.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * Supports Int, Float, and String comparisons.  Numeric operators
 * (<, <=, >, >=) are only valid for Int and Float.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_Compare.h"
#include "../../AtomicTaskRegistry.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_Compare::Task_Compare() {}

void Task_Compare::Abort()
{
    SYSTEM_LOG << "[Task_Compare] Abort()\n";
}

TaskStatus Task_Compare::Execute(const ParameterMap& params)
{
    // Stateless: no context needed, so the legacy path works too.
    AtomicTaskContext ctx;
    return ExecuteWithContext(ctx, params);
}

TaskStatus Task_Compare::ExecuteWithContext(const AtomicTaskContext& ctx,
                                             const ParameterMap& params)
{
    (void)ctx;

    // --- Resolve Operator ---
    std::string op;
    {
        auto it = params.find("Operator");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_Compare] Missing or invalid 'Operator' parameter\n";
            return TaskStatus::Failure;
        }
        op = it->second.AsString();
    }

    // --- Resolve LHS and RHS ---
    auto lhsIt = params.find("LHS");
    auto rhsIt = params.find("RHS");

    if (lhsIt == params.end() || rhsIt == params.end())
    {
        SYSTEM_LOG << "[Task_Compare] Missing 'LHS' or 'RHS' parameter\n";
        return TaskStatus::Failure;
    }

    const TaskValue& lhs = lhsIt->second;
    const TaskValue& rhs = rhsIt->second;

    if (lhs.GetType() != rhs.GetType())
    {
        SYSTEM_LOG << "[Task_Compare] Type mismatch between LHS and RHS\n";
        return TaskStatus::Failure;
    }

    bool result = false;

    switch (lhs.GetType())
    {
        case VariableType::Int:
        {
            int l = lhs.AsInt();
            int r = rhs.AsInt();
            if      (op == "==") result = (l == r);
            else if (op == "!=") result = (l != r);
            else if (op == "<")  result = (l < r);
            else if (op == "<=") result = (l <= r);
            else if (op == ">")  result = (l > r);
            else if (op == ">=") result = (l >= r);
            else
            {
                SYSTEM_LOG << "[Task_Compare] Unknown operator '" << op << "'\n";
                return TaskStatus::Failure;
            }
            break;
        }
        case VariableType::Float:
        {
            float l = lhs.AsFloat();
            float r = rhs.AsFloat();
            // Note: == and != perform exact bitwise float comparison.
            // Callers using == or != for computed floats should consider
            // using epsilon-tolerant comparisons via LocalBB pre-processing.
            if      (op == "==") result = (l == r);
            else if (op == "!=") result = (l != r);
            else if (op == "<")  result = (l < r);
            else if (op == "<=") result = (l <= r);
            else if (op == ">")  result = (l > r);
            else if (op == ">=") result = (l >= r);
            else
            {
                SYSTEM_LOG << "[Task_Compare] Unknown operator '" << op << "'\n";
                return TaskStatus::Failure;
            }
            break;
        }
        case VariableType::String:
        {
            const std::string& l = lhs.AsString();
            const std::string& r = rhs.AsString();
            if      (op == "==") result = (l == r);
            else if (op == "!=") result = (l != r);
            else if (op == "<")  result = (l < r);
            else if (op == "<=") result = (l <= r);
            else if (op == ">")  result = (l > r);
            else if (op == ">=") result = (l >= r);
            else
            {
                SYSTEM_LOG << "[Task_Compare] Unknown operator '" << op << "'\n";
                return TaskStatus::Failure;
            }
            break;
        }
        default:
            SYSTEM_LOG << "[Task_Compare] Unsupported type for comparison\n";
            return TaskStatus::Failure;
    }

    SYSTEM_LOG << "[Task_Compare] Comparison result: " << (result ? "true" : "false") << "\n";
    return result ? TaskStatus::Success : TaskStatus::Failure;
}

REGISTER_ATOMIC_TASK(Task_Compare, "Task_Compare")

} // namespace Olympe
