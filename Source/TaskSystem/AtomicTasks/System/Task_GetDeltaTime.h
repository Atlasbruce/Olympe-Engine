/**
 * @file Task_GetDeltaTime.h
 * @brief Atomic task that writes the current delta-time into the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_GetDeltaTime captures ctx.DeltaTime and stores it under the key
 * "local:DeltaTime" in the entity's LocalBlackboard.  Useful for tasks that
 * need to read dt from the blackboard rather than from the context directly.
 *
 * No required parameters.
 *
 * Returns:
 *   Success  always.
 *   Failure  if ctx.LocalBB is null.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_GetDeltaTime
 * @brief Writes ctx.DeltaTime to "local:DeltaTime" in the LocalBlackboard.
 */
class Task_GetDeltaTime : public IAtomicTask {
public:
    Task_GetDeltaTime();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
