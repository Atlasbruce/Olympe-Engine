/**
 * @file Task_SetSprite.h
 * @brief Atomic task that sets the current sprite reference for an entity.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_SetSprite updates the sprite displayed by an entity.  In World mode
 * (when a render component facade is available) it delegates to the render
 * system.  In headless mode it stores the sprite reference in the LocalBB.
 *
 * Parameters (ParameterMap):
 *   "SpriteRef" (String) — sprite asset path or identifier (required).
 *
 * Returns:
 *   Success  after writing the sprite reference.
 *   Failure  if "SpriteRef" is missing.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_SetSprite
 * @brief Sets the current sprite reference via ECS or LocalBlackboard.
 */
class Task_SetSprite : public IAtomicTask {
public:
    Task_SetSprite();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
