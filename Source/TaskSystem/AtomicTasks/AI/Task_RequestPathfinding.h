/**
 * @file Task_RequestPathfinding.h
 * @brief Atomic task that computes a path and writes it to the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_RequestPathfinding synchronously computes a straight-line path from
 * "Start" to "End" and stores the destination in the "Path" LocalBlackboard
 * key.  The synchronous approach is intentional for deterministic CI testing.
 *
 * Parameters (ParameterMap):
 *   "Start" (Vector) - path start position (required)
 *   "End"   (Vector) - path end position (required)
 *
 * LocalBlackboard output:
 *   "Path" (Vector) - the destination vector (straight-line path endpoint)
 *
 * Returns:
 *   Success  once the path is written.
 *   Failure  if parameters are missing or the "Path" BB key does not exist.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_RequestPathfinding
 * @brief Computes a straight-line path and writes it to the LocalBlackboard.
 */
class Task_RequestPathfinding : public IAtomicTask {
public:
    Task_RequestPathfinding();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
