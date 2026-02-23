/**
 * @file Task_Wait.h
 * @brief Atomic task that waits for a specified duration.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_Wait returns Running until ctx.StateTimer reaches the "Duration"
 * parameter, then returns Success.
 *
 * Parameters (ParameterMap):
 *   "Duration" (Float) - seconds to wait (required, > 0)
 *
 * Returns:
 *   Running  while StateTimer < Duration.
 *   Success  once StateTimer >= Duration.
 *   Failure  if "Duration" parameter is missing or non-positive.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_Wait
 * @brief Atomic task that idles for a fixed duration then succeeds.
 */
class Task_Wait : public IAtomicTask {
public:
    Task_Wait();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
