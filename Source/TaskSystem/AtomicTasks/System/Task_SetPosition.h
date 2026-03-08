/**
 * @file Task_SetPosition.h
 * @brief Atomic task that sets the entity's position.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_SetPosition teleports an entity to a specified position.
 *
 * Parameters (ParameterMap):
 *   "Target" (Vector) — destination position (required).
 *
 * World mode  (ctx.ComponentFacade with Position set):
 *   Writes directly to the entity's PositionComponent.
 *
 * Headless mode (ComponentFacade is nullptr or Position is null):
 *   Writes to "local:Position" in ctx.LocalBB.
 *
 * Returns:
 *   Success  after writing the position.
 *   Failure  if "Target" is missing.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_SetPosition
 * @brief Sets entity position via ECS or LocalBlackboard.
 */
class Task_SetPosition : public IAtomicTask {
public:
    Task_SetPosition();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
