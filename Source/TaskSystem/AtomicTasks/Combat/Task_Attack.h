/**
 * @file Task_Attack.h
 * @brief Atomic task that performs a melee/ranged attack on a target entity.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_Attack reads the current target EntityID from "local:Target" in
 * the LocalBlackboard, then logs the attack with the resolved damage and
 * range values.  The actual damage application is handled by the combat
 * system; this task only initiates and logs the action.
 *
 * Parameters (ParameterMap):
 *   "Damage" (Float) — damage amount (optional, default 10.0).
 *   "Range"  (Float) — maximum attack range in units (optional, default 50.0).
 *
 * Returns:
 *   Success  after performing the attack action.
 *   Failure  if ctx.LocalBB is null.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_Attack
 * @brief Initiates an attack action on the current target entity.
 */
class Task_Attack : public IAtomicTask {
public:
    Task_Attack();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
