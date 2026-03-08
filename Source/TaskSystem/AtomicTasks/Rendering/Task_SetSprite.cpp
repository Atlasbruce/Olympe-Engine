/**
 * @file Task_SetSprite.cpp
 * @brief Atomic task: set the current sprite reference for an entity.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_SetSprite.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_SetSprite::Task_SetSprite() {}

void Task_SetSprite::Abort()
{
    SYSTEM_LOG << "[Task_SetSprite] Abort()\n";
}

TaskStatus Task_SetSprite::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_SetSprite::ExecuteWithContext(const AtomicTaskContext& ctx,
                                               const ParameterMap& params)
{
    // --- Resolve SpriteRef parameter ---
    std::string spriteRef;
    {
        auto it = params.find("SpriteRef");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_SetSprite] Missing or invalid 'SpriteRef' parameter\n";
            return TaskStatus::Failure;
        }
        spriteRef = it->second.AsString();
    }

    SYSTEM_LOG << "[Task_SetSprite] Entity " << ctx.Entity
               << " sprite='" << spriteRef << "'\n";

    // -----------------------------------------------------------------------
    // World mode: delegate to render component (stub — render facade not yet
    // added to TaskWorldFacade; falls through to headless path)
    // -----------------------------------------------------------------------
    // (Future: when a RenderComponent pointer is added to TaskWorldFacade,
    // call ctx.ComponentFacade->Render->SetSprite(spriteRef) here.)

    // -----------------------------------------------------------------------
    // Headless / fallback mode: write to "local:CurrentSprite" in LocalBB
    // -----------------------------------------------------------------------
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_SetSprite] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    ctx.LocalBB->SetValueScoped("local:CurrentSprite", TaskValue(spriteRef));

    SYSTEM_LOG << "[Task_SetSprite] Entity " << ctx.Entity
               << " sprite set - Success\n";
    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_SetSprite, "Task_SetSprite")

} // namespace Olympe
