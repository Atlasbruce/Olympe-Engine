/**
 * @file Task_Wait.cpp
 * @brief Implementation of Task_Wait atomic task.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_Wait uses ctx.StateTimer (accumulated by TaskSystem each tick) to
 * measure elapsed time.  It returns Running until StateTimer >= Duration.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_Wait.h"
#include "../../AtomicTaskRegistry.h"
#include "../../../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Execute (legacy path - returns Failure without a context)
// ============================================================================

TaskStatus Task_Wait::Execute(const ParameterMap& /*params*/)
{
    // StateTimer is only available via AtomicTaskContext.  Callers must use
    // ExecuteWithContext().
    return TaskStatus::Failure;
}

// ============================================================================
// ExecuteWithContext
// ============================================================================

TaskStatus Task_Wait::ExecuteWithContext(const AtomicTaskContext& ctx,
                                          const ParameterMap& params)
{
    float duration = 1.0f;
    {
        auto it = params.find("Duration");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
            duration = it->second.AsFloat();
    }

    SYSTEM_LOG << "[Task_Wait] Entity " << ctx.Entity
               << " timer=" << ctx.StateTimer << " / " << duration << "\n";

    if (ctx.StateTimer >= duration)
    {
        SYSTEM_LOG << "[Task_Wait] Entity " << ctx.Entity << " wait complete - Success\n";
        return TaskStatus::Success;
    }

    return TaskStatus::Running;
}

// ============================================================================
// Abort
// ============================================================================

void Task_Wait::Abort()
{
    SYSTEM_LOG << "[Task_Wait] Abort() called\n";
}

REGISTER_ATOMIC_TASK(Task_Wait, "Task_Wait")

} // namespace Olympe
