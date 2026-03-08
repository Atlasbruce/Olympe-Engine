/**
 * @file Task_GetWorldInstance.h
 * @brief Atomic task that resolves a named World entity and stores its ID.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_GetWorldInstance looks up an entity by name in the live World and
 * stores the resulting EntityID under "local:TargetInstance" in the
 * LocalBlackboard.  When ctx.WorldPtr is null (headless / test mode) the
 * task logs a warning and returns Failure.
 *
 * Parameters (ParameterMap):
 *   "InstanceName" (String) — name of the entity to locate (required).
 *
 * Returns:
 *   Success  if the entity was found (EntityID written to LocalBB).
 *   Failure  if InstanceName is missing, WorldPtr is null, or entity not found.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_GetWorldInstance
 * @brief Finds an entity by name in World and stores its EntityID in LocalBB.
 */
class Task_GetWorldInstance : public IAtomicTask {
public:
    Task_GetWorldInstance();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
