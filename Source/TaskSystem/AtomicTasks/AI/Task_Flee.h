/**
 * @file Task_Flee.h
 * @brief Atomic AI task that moves an entity away from a 2D threat position.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_Flee computes a flee direction opposite to the threat (TargetX, TargetY)
 * and moves the entity away from it.  Returns Running while the entity is
 * closer than Distance to the threat; returns Success once the entity is
 * far enough away.
 *
 * Parameters (ParameterMap):
 *   "TargetX"  (Float) — X coordinate of the threat (required).
 *   "TargetY"  (Float) — Y coordinate of the threat (required).
 *   "Distance" (Float) — safe flee distance in units (optional, default 200.0).
 *
 * Returns:
 *   Running  while the entity's distance to the threat < Distance.
 *   Success  once the entity's distance to the threat >= Distance.
 *   Failure  if "TargetX" or "TargetY" are missing.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_Flee
 * @brief Moves an entity away from a 2D threat position until safe distance is reached.
 */
class Task_Flee : public IAtomicTask {
public:
    Task_Flee();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
