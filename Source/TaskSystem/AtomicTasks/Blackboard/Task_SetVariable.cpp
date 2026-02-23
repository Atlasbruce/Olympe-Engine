/**
 * @file Task_SetVariable.cpp
 * @brief Implementation of Task_SetVariable atomic task.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Writes the "Value" parameter into ctx.LocalBB["VariableName"].  Returns
 * Success if the write succeeds, Failure on missing variable or type mismatch.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_SetVariable.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

#include <stdexcept>

namespace Olympe {

// ============================================================================
// Execute (legacy path)
// ============================================================================

TaskStatus Task_SetVariable::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure;
}

// ============================================================================
// ExecuteWithContext
// ============================================================================

TaskStatus Task_SetVariable::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                 const ParameterMap& params)
{
    // Read variable name.
    std::string varName;
    {
        auto it = params.find("VariableName");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity
                       << ": missing or non-String 'VariableName' parameter\n";
            return TaskStatus::Failure;
        }
        varName = it->second.AsString();
    }

    // Read value to set.
    auto valIt = params.find("Value");
    if (valIt == params.end())
    {
        SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity
                   << ": missing 'Value' parameter\n";
        return TaskStatus::Failure;
    }

    const TaskValue& value = valIt->second;

    // Write into LocalBlackboard.
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity
                   << ": LocalBB is null\n";
        return TaskStatus::Failure;
    }

    if (!ctx.LocalBB->HasVariable(varName))
    {
        SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity
                   << ": variable '" << varName << "' not declared in template\n";
        return TaskStatus::Failure;
    }

    try
    {
        ctx.LocalBB->SetValue(varName, value);
    }
    catch (const std::runtime_error& e)
    {
        SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity
                   << ": " << e.what() << "\n";
        return TaskStatus::Failure;
    }

    SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity
               << ": set '" << varName << "' - Success\n";
    return TaskStatus::Success;
}

// ============================================================================
// Abort
// ============================================================================

void Task_SetVariable::Abort()
{
    SYSTEM_LOG << "[Task_SetVariable] Abort() called\n";
}

REGISTER_ATOMIC_TASK(Task_SetVariable, "Task_SetVariable")

} // namespace Olympe
