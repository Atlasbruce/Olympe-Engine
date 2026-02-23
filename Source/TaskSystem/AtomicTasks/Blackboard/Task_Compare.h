/**
 * @file Task_Compare.h
 * @brief Atomic task that compares two parameter values and returns Success/Failure.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_Compare reads parameters "A", "B" and "Op" (String: "==", "<", ">")
 * and returns TaskStatus::Success if the comparison holds, otherwise
 * TaskStatus::Failure.
 *
 * Supported type pairs:
 *   - Float vs Float  : numeric comparison.
 *   - Int vs Int      : numeric comparison.
 *   - Float vs Int    : A is promoted to float for comparison.
 *   - String vs String: lexicographic comparison.
 *
 * Parameters:
 *   "A"  (Float | Int | String) - left-hand operand.
 *   "B"  (Float | Int | String) - right-hand operand.
 *   "Op" (String)               - operator: "==" | "<" | ">".
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"
#include "../../AtomicTaskContext.h"

namespace Olympe {

/**
 * @class Task_Compare
 * @brief Compares two values with the given operator and returns Success/Failure.
 */
class Task_Compare : public IAtomicTask {
public:

    TaskStatus Execute(const ParameterMap& params) override;

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
