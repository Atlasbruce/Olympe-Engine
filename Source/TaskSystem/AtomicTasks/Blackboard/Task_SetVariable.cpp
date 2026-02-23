/**
 * @file Task_SetVariable.cpp
 * @brief Atomic task: write a value into the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_SetVariable.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_SetVariable::Task_SetVariable() {}

void Task_SetVariable::Abort()
{
    SYSTEM_LOG << "[Task_SetVariable] Abort()\n";
}

TaskStatus Task_SetVariable::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_SetVariable::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                 const ParameterMap& params)
{
    // --- Resolve VarName parameter ---
    std::string varName;
    {
        auto it = params.find("VarName");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_SetVariable] Missing or invalid 'VarName' parameter\n";
            return TaskStatus::Failure;
        }
        varName = it->second.AsString();
    }

    // --- Resolve Value parameter ---
    auto valueIt = params.find("Value");
    if (valueIt == params.end() || valueIt->second.IsNone())
    {
        SYSTEM_LOG << "[Task_SetVariable] Missing 'Value' parameter\n";
        return TaskStatus::Failure;
    }

    // --- Write to LocalBlackboard ---
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_SetVariable] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    try
    {
        ctx.LocalBB->SetValue(varName, valueIt->second);
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[Task_SetVariable] Failed to set '" << varName
                   << "': " << e.what() << "\n";
        return TaskStatus::Failure;
    }

    SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity
               << " set '" << varName << "' - Success\n";
    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_SetVariable, "Task_SetVariable")

} // namespace Olympe
