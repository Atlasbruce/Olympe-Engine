/**
 * @file Task_ChangeState.cpp
 * @brief Atomic task: change the AI state in the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_ChangeState.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_ChangeState::Task_ChangeState() {}

void Task_ChangeState::Abort()
{
    SYSTEM_LOG << "[Task_ChangeState] Abort()\n";
}

TaskStatus Task_ChangeState::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_ChangeState::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                 const ParameterMap& params)
{
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_ChangeState] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    // --- Resolve NewState parameter ---
    std::string newState;
    {
        auto it = params.find("NewState");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_ChangeState] Missing or invalid 'NewState' parameter\n";
            return TaskStatus::Failure;
        }
        newState = it->second.AsString();
    }

    // --- Log the current state for diagnostics ---
    const TaskValue currentVal = ctx.LocalBB->GetValueScoped("local:CurrentState");
    if (currentVal.GetType() == VariableType::String)
    {
        SYSTEM_LOG << "[Task_ChangeState] Entity " << ctx.Entity
                   << " state: '" << currentVal.AsString()
                   << "' -> '" << newState << "'\n";
    }
    else
    {
        SYSTEM_LOG << "[Task_ChangeState] Entity " << ctx.Entity
                   << " setting state to '" << newState << "'\n";
    }

    // --- Write new state to LocalBlackboard ---
    ctx.LocalBB->SetValueScoped("local:CurrentState", TaskValue(newState));

    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_ChangeState, "Task_ChangeState")

} // namespace Olympe
