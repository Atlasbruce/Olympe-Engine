/**
 * @file Task_SetPosition.cpp
 * @brief Atomic task: set entity position via ECS or LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_SetPosition.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../TaskWorldFacade.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_SetPosition::Task_SetPosition() {}

void Task_SetPosition::Abort()
{
    SYSTEM_LOG << "[Task_SetPosition] Abort()\n";
}

TaskStatus Task_SetPosition::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_SetPosition::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                 const ParameterMap& params)
{
    // --- Resolve Target parameter ---
    ::Vector target(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("Target");
        if (it == params.end() || it->second.GetType() != VariableType::Vector)
        {
            SYSTEM_LOG << "[Task_SetPosition] Missing or invalid 'Target' parameter\n";
            return TaskStatus::Failure;
        }
        target = it->second.AsVector();
    }

    SYSTEM_LOG << "[Task_SetPosition] Entity " << ctx.Entity
               << " -> (" << target.x << "," << target.y << ")\n";

    // -----------------------------------------------------------------------
    // World mode: write directly to PositionComponent when available
    // -----------------------------------------------------------------------
    if (ctx.ComponentFacade && ctx.ComponentFacade->Position)
    {
        ctx.ComponentFacade->Position->Position = target;
        SYSTEM_LOG << "[Task_SetPosition] (WorldMode) Entity " << ctx.Entity
                   << " position set - Success\n";
        return TaskStatus::Success;
    }

    // -----------------------------------------------------------------------
    // Headless mode: write to "local:Position" in LocalBlackboard
    // -----------------------------------------------------------------------
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_SetPosition] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    ctx.LocalBB->SetValueScoped("local:Position", TaskValue(target));

    SYSTEM_LOG << "[Task_SetPosition] Entity " << ctx.Entity
               << " position written to LocalBB - Success\n";
    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_SetPosition, "Task_SetPosition")

} // namespace Olympe
