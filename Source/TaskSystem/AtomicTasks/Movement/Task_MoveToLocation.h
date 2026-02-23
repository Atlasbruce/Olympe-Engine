/**
 * @file Task_MoveToLocation.h
 * @brief Atomic task that moves an entity toward a target location.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_MoveToLocation moves an entity toward a specified target position.
 * It operates in dual mode:
 *   - World mode  : if ctx.WorldPtr is set and the entity has a position
 *                   component, that component is read and updated directly.
 *   - Headless mode: reads and writes the "Position" key in ctx.LocalBB,
 *                    integrating position analytically using ctx.DeltaTime.
 *
 * Parameters (ParameterMap):
 *   "Target" (Vector) - destination position (required)
 *   "Speed"  (Float)  - units per second (optional, default 100.0)
 *
 * Returns:
 *   Running  while the entity is more than a small tolerance from Target.
 *   Success  once the entity reaches the target (or snaps to it on the
 *            final step).
 *   Failure  if the "Position" BB key is missing in headless mode.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_MoveToLocation
 * @brief Moves an entity toward a target location each tick.
 */
class Task_MoveToLocation : public IAtomicTask {
public:
    Task_MoveToLocation();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
