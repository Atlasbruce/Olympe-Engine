/**
 * @file Task_SetAnimation.cpp
 * @brief Atomic AI task: trigger an animation on an entity.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_SetAnimation.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_SetAnimation::Task_SetAnimation() {}

void Task_SetAnimation::Abort()
{
    SYSTEM_LOG << "[Task_SetAnimation] Abort()\n";
}

TaskStatus Task_SetAnimation::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_SetAnimation::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                  const ParameterMap& params)
{
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_SetAnimation] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    // --- Resolve AnimRef parameter ---
    std::string animRef;
    {
        auto it = params.find("AnimRef");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_SetAnimation] Missing or invalid 'AnimRef' parameter\n";
            return TaskStatus::Failure;
        }
        animRef = it->second.AsString();
    }

    // --- Resolve Loop parameter (optional, default true) ---
    bool loop = true;
    {
        auto it = params.find("Loop");
        if (it != params.end() && it->second.GetType() == VariableType::Bool)
        {
            loop = it->second.AsBool();
        }
    }

    SYSTEM_LOG << "[Task_SetAnimation] Entity " << ctx.Entity
               << " anim='" << animRef << "' loop=" << (loop ? "true" : "false") << "\n";

    // Write animation info to LocalBlackboard.
    // Future: when ComponentFacade exposes an AnimationComponent, call
    // ctx.ComponentFacade->Animation->Play(animRef, loop) here.
    ctx.LocalBB->SetValueScoped("local:current_animation", TaskValue(animRef));

    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_SetAnimation, "Task_SetAnimation")

} // namespace Olympe
