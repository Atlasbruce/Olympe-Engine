/**
 * @file Task_RequestPathfinding.h
 * @brief Atomic task that computes a straight-line path and stores it in LocalBB.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_RequestPathfinding performs a simplified synchronous path computation.
 * It builds a straight-line path from "Start" to "Goal" and writes the result
 * as a String into ctx.LocalBB["Path"] (format: "x0,y0:x1,y1").
 *
 * For deterministic CI/headless testing the computation is synchronous.  An
 * asynchronous implementation using navmesh queries can be substituted later.
 *
 * Parameters:
 *   "Start" (Vector) - path start.  If absent, (0,0,0) is used.
 *   "Goal"  (Vector) - path destination.
 *
 * LocalBlackboard output:
 *   "Path" (String)  - serialized waypoint list (written if the variable is
 *                      declared in the template).
 *
 * Returns TaskStatus::Success unconditionally.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"
#include "../../AtomicTaskContext.h"

namespace Olympe {

/**
 * @class Task_RequestPathfinding
 * @brief Synchronous straight-line pathfinding stored in LocalBlackboard["Path"].
 */
class Task_RequestPathfinding : public IAtomicTask {
public:

    TaskStatus Execute(const ParameterMap& params) override;

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
