/**
 * @file Task_PlayAnimation.h
 * @brief Atomic task that starts an animation on an entity.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_PlayAnimation requests that an animation be played on the entity.
 * In headless mode it stores the animation name in the LocalBlackboard.
 *
 * Parameters (ParameterMap):
 *   "AnimationName" (String) — animation identifier (required).
 *   "Loop"          (Bool)   — whether to loop the animation (optional, default=false).
 *
 * Returns:
 *   Success  after writing the animation request.
 *   Failure  if "AnimationName" is missing.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_PlayAnimation
 * @brief Requests an animation to play via ECS or LocalBlackboard.
 */
class Task_PlayAnimation : public IAtomicTask {
public:
    Task_PlayAnimation();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
