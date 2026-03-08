/**
 * @file Task_SetAnimation.h
 * @brief Atomic AI task that triggers an animation on an entity.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_SetAnimation requests that a named animation be played on the entity.
 * In headless mode it stores the animation reference in the LocalBlackboard
 * under "local:current_animation".  In World mode it can be extended to
 * call an AnimationComponent via the ComponentFacade.
 *
 * Parameters (ParameterMap):
 *   "AnimRef" (String) — animation identifier / clip name (required).
 *   "Loop"    (Bool)   — whether to loop (optional, default true).
 *
 * Returns:
 *   Success  after writing the animation request.
 *   Failure  if "AnimRef" is missing or if ctx.LocalBB is null.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_SetAnimation
 * @brief Requests an animation clip to play on the entity.
 */
class Task_SetAnimation : public IAtomicTask {
public:
    Task_SetAnimation();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
