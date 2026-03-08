/**
 * @file Task_PlayAnimation.cpp
 * @brief Atomic task: request an animation to play on an entity.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_PlayAnimation.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_PlayAnimation::Task_PlayAnimation() {}

void Task_PlayAnimation::Abort()
{
    SYSTEM_LOG << "[Task_PlayAnimation] Abort()\n";
}

TaskStatus Task_PlayAnimation::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_PlayAnimation::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                   const ParameterMap& params)
{
    // --- Resolve AnimationName parameter ---
    std::string animName;
    {
        auto it = params.find("AnimationName");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_PlayAnimation] Missing or invalid 'AnimationName' parameter\n";
            return TaskStatus::Failure;
        }
        animName = it->second.AsString();
    }

    // --- Resolve Loop parameter (optional, default false) ---
    bool loop = false;
    {
        auto it = params.find("Loop");
        if (it != params.end() && it->second.GetType() == VariableType::Bool)
        {
            loop = it->second.AsBool();
        }
    }

    SYSTEM_LOG << "[Task_PlayAnimation] Entity " << ctx.Entity
               << " animation='" << animName << "' loop=" << (loop ? "true" : "false") << "\n";

    // -----------------------------------------------------------------------
    // Headless / fallback mode: write to "local:CurrentAnimation" in LocalBB
    // -----------------------------------------------------------------------
    // (Future: when an AnimationComponent pointer is added to TaskWorldFacade,
    // call ctx.ComponentFacade->Animation->Play(animName, loop) here.)
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_PlayAnimation] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    ctx.LocalBB->SetValueScoped("local:CurrentAnimation", TaskValue(animName));

    SYSTEM_LOG << "[Task_PlayAnimation] Entity " << ctx.Entity
               << " animation set - Success\n";
    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_PlayAnimation, "Task_PlayAnimation")

} // namespace Olympe
