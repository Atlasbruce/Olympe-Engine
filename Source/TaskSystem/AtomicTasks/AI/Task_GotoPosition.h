/**
 * @file Task_GotoPosition.h
 * @brief Atomic task that moves an entity toward a 2D target position.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_GotoPosition moves the entity toward (TargetX, TargetY) each frame.
 * In headless mode it integrates the entity's "Position" key in the
 * LocalBlackboard using ctx.DeltaTime.  In World mode it writes Velocity
 * to the MovementComponent via the ComponentFacade.
 *
 * Parameters (ParameterMap):
 *   "TargetX" (Float) — X coordinate of the destination (required).
 *   "TargetY" (Float) — Y coordinate of the destination (required).
 *   "Speed"   (Float) — Movement speed in units/s (optional, default 300.0).
 *
 * Returns:
 *   Running  while the entity is more than 5 units from the target.
 *   Success  once the entity arrives (distance <= 5 units).
 *   Success  immediately when ctx.World is nullptr (headless/test mode).
 *   Failure  if "TargetX" or "TargetY" parameters are missing.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_GotoPosition
 * @brief Moves an entity toward a 2D (TargetX, TargetY) destination.
 */
class Task_GotoPosition : public IAtomicTask {
public:
    Task_GotoPosition();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
