/**
 * @file Task_ChangeState.cpp
 * @brief Atomic AI task: change the entity's current state in the LocalBlackboard.
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

static const char* BB_KEY_CURRENT_STATE = "local:current_state";

Task_ChangeState::Task_ChangeState() {}

void Task_ChangeState::Abort()
{
    SYSTEM_LOG << "[Task_ChangeState(AI)] Abort()\n";
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
        SYSTEM_LOG << "[Task_ChangeState(AI)] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    // --- Resolve StateName parameter ---
    std::string stateName;
    {
        auto it = params.find("StateName");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_ChangeState(AI)] Missing or invalid 'StateName' parameter\n";
            return TaskStatus::Failure;
        }
        stateName = it->second.AsString();
    }

    // Log the state transition.
    const TaskValue currentVal = ctx.LocalBB->GetValueScoped(BB_KEY_CURRENT_STATE);
    if (currentVal.GetType() == VariableType::String)
    {
        SYSTEM_LOG << "[Task_ChangeState(AI)] Entity " << ctx.Entity
                   << " state: '" << currentVal.AsString()
                   << "' -> '" << stateName << "'\n";
    }
    else
    {
        SYSTEM_LOG << "[Task_ChangeState(AI)] Entity " << ctx.Entity
                   << " setting state -> '" << stateName << "'\n";
    }

    ctx.LocalBB->SetValueScoped(BB_KEY_CURRENT_STATE, TaskValue(stateName));

    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_ChangeState, "Task_ChangeState")

} // namespace Olympe
