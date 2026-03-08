/**
 * @file Task_GetDeltaTime.cpp
 * @brief Atomic task: write the current frame delta-time to the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_GetDeltaTime.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_GetDeltaTime::Task_GetDeltaTime() {}

void Task_GetDeltaTime::Abort()
{
    SYSTEM_LOG << "[Task_GetDeltaTime] Abort()\n";
}

TaskStatus Task_GetDeltaTime::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_GetDeltaTime::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                  const ParameterMap& /*params*/)
{
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_GetDeltaTime] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    ctx.LocalBB->SetValueScoped("local:DeltaTime", TaskValue(ctx.DeltaTime));

    SYSTEM_LOG << "[Task_GetDeltaTime] Entity " << ctx.Entity
               << " dt=" << ctx.DeltaTime << " written to 'local:DeltaTime'\n";
    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_GetDeltaTime, "Task_GetDeltaTime")

} // namespace Olympe
