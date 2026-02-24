/**
 * @file Task_RequestPathfinding.h
 * @brief Atomic task that asynchronously requests a path via PathfindingManager.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * Task_RequestPathfinding submits an async pathfinding request on the first
 * tick and polls for completion on subsequent ticks.  When the request
 * completes the path string is written to the "Path" LocalBlackboard key.
 *
 * Parameters (ParameterMap):
 *   "Target"     (Vector) - destination position (required)
 *   "AsyncDelay" (Float)  - simulated delay in seconds (optional, default 0.0)
 *
 * LocalBlackboard reads:
 *   "Position" (Vector) - current entity position (required)
 *
 * LocalBlackboard output:
 *   "Path" (String) - straight-line path string "(sx,sy,sz)->(tx,ty,tz)"
 *
 * Returns:
 *   Running  on the first tick (request submitted) and while waiting.
 *   Success  once the path is written to "Path".
 *   Failure  if required parameters or BB keys are missing.
 *
 * Abort() cancels the in-flight request.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <cstdint>

#include "../../IAtomicTask.h"
#include "../../Pathfinding/PathfindingManager.h"

namespace Olympe {

/**
 * @class Task_RequestPathfinding
 * @brief Async pathfinding task that polls PathfindingManager for completion.
 */
class Task_RequestPathfinding : public IAtomicTask {
public:
    Task_RequestPathfinding();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;

private:
    PathfindingManager::RequestID m_requestID;  ///< Active request ID (0 = none)
    bool                          m_hasRequest; ///< True after first-tick submission
};

} // namespace Olympe
