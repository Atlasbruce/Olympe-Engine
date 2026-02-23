/**
 * @file Task_MoveToLocation.h
 * @brief Atomic task that moves an entity toward a target location.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_MoveToLocation drives an entity toward a target Vector at a configurable
 * Speed.  It returns TaskStatus::Running each frame until the entity is within
 * AcceptanceRadius of the target, then returns TaskStatus::Success.
 *
 * Dual-mode operation:
 *   - If ctx.WorldPtr is non-null the task can be extended to use World
 *     Position/Movement components (stub left for Phase 1.5 integration).
 *   - Otherwise, the task reads "Position" (Vector) and writes "Position" and
 *     "Velocity" (Vector) into ctx.LocalBB to simulate movement in headless /
 *     unit-test mode.
 *
 * Parameters (all passed via ParameterMap):
 *   "Target"           (Vector) - destination position.
 *   "Speed"            (Float)  - movement speed in units/s.  Default: 100.
 *   "AcceptanceRadius" (Float)  - success radius in units.   Default: 5.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"
#include "../../AtomicTaskContext.h"

namespace Olympe {

/**
 * @class Task_MoveToLocation
 * @brief Moves an entity toward a target each tick using LocalBlackboard state.
 */
class Task_MoveToLocation : public IAtomicTask {
public:

    TaskStatus Execute(const ParameterMap& params) override;

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
