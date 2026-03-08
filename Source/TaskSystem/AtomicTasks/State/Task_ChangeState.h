/**
 * @file Task_ChangeState.h
 * @brief Atomic task that changes the AI state stored in the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_ChangeState writes a new state string to the "local:CurrentState" key
 * in the entity's LocalBlackboard.  It is the standard way for VS graphs to
 * transition between logical AI states (e.g. "Patrolling" → "Alert" → "Guard").
 *
 * Parameters (ParameterMap):
 *   "NewState" (String) — target state name (required).
 *
 * Returns:
 *   Success  after writing the new state.
 *   Failure  if "NewState" is missing, or if ctx.LocalBB is null.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_ChangeState
 * @brief Sets "local:CurrentState" in the entity's LocalBlackboard.
 */
class Task_ChangeState : public IAtomicTask {
public:
    Task_ChangeState();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
