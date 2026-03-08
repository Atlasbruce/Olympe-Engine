/**
 * @file Task_Flee.h
 * @brief Atomic task that computes a flee direction away from a target.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Task_Flee reads the entity's current position from "local:Position" and
 * an optional threat target from "local:Target" (or the "Target" parameter).
 * It computes the direction away from the threat, scales it by Distance, and
 * writes the resulting flee target position to "local:FleeTarget" in the
 * LocalBlackboard.  A movement task (e.g. Task_MoveToLocation) can then be
 * used to move the entity toward the computed flee target.
 *
 * Parameters (ParameterMap):
 *   "Target"   (Vector)  — threat position override (optional; falls back to
 *                          "local:Target" EntityID in LocalBB, then origin).
 *   "Distance" (Float)   — flee distance in units (optional, default 200.0).
 *
 * Returns:
 *   Success  after writing the flee target.
 *   Failure  if ctx.LocalBB is null.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_Flee
 * @brief Computes a flee direction and stores the target position in LocalBB.
 */
class Task_Flee : public IAtomicTask {
public:
    Task_Flee();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
