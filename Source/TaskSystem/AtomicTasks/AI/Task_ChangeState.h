/**
 * @file Task_ChangeState.h
 * @brief Atomic AI task that changes the entity's current state.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_ChangeState writes a new state name to the "local:current_state" key
 * in the entity's LocalBlackboard.  This is the standard way for VS graphs
 * to transition AI entities between logical states (e.g. "Idle" → "Patrolling").
 *
 * Parameters (ParameterMap):
 *   "StateName" (String) — target state name (required).
 *
 * Returns:
 *   Success  after writing the new state name.
 *   Failure  if "StateName" is missing or if ctx.LocalBB is null.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_ChangeState
 * @brief Writes a new AI state name to "local:current_state" in the LocalBlackboard.
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
