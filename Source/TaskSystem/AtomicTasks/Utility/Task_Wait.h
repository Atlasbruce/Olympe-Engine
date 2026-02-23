/**
 * @file Task_Wait.h
 * @brief Atomic task that waits for a configurable duration before succeeding.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_Wait reads the "Duration" (Float, seconds) parameter and returns
 * TaskStatus::Running on every tick until ctx.StateTimer >= Duration,
 * then returns TaskStatus::Success.
 *
 * Parameters:
 *   "Duration" (Float) - time to wait in seconds.  Default: 1.0.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"
#include "../../AtomicTaskContext.h"

namespace Olympe {

/**
 * @class Task_Wait
 * @brief Multi-frame wait task driven by ctx.StateTimer.
 */
class Task_Wait : public IAtomicTask {
public:

    TaskStatus Execute(const ParameterMap& params) override;

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
