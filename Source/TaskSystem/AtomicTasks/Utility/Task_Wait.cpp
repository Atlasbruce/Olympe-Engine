/**
 * @file Task_Wait.cpp
 * @brief Atomic task: wait for a specified duration.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Uses ctx.StateTimer (accumulated per-node time managed by TaskSystem) to
 * determine when the wait duration has elapsed.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_Wait.h"
#include "../../AtomicTaskRegistry.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_Wait::Task_Wait() {}

void Task_Wait::Abort()
{
    SYSTEM_LOG << "[Task_Wait] Abort()\n";
}

TaskStatus Task_Wait::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_Wait::ExecuteWithContext(const AtomicTaskContext& ctx,
                                          const ParameterMap& params)
{
    // --- Resolve Duration parameter ---
    float duration = 0.0f;
    {
        auto it = params.find("Duration");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
        {
            duration = it->second.AsFloat();
        }
        else
        {
            SYSTEM_LOG << "[Task_Wait] Missing or invalid 'Duration' parameter\n";
            return TaskStatus::Failure;
        }

        if (duration <= 0.0f)
        {
            SYSTEM_LOG << "[Task_Wait] Duration must be positive (got " << duration << ")\n";
            return TaskStatus::Failure;
        }
    }

    SYSTEM_LOG << "[Task_Wait] Entity " << ctx.Entity
               << " timer=" << ctx.StateTimer
               << " duration=" << duration << "\n";

    if (ctx.StateTimer >= duration)
    {
        SYSTEM_LOG << "[Task_Wait] Entity " << ctx.Entity
                   << " wait complete - Success\n";
        return TaskStatus::Success;
    }

    return TaskStatus::Running;
}

REGISTER_ATOMIC_TASK(Task_Wait, "Task_Wait")

} // namespace Olympe
