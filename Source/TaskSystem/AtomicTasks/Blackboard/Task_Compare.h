/**
 * @file Task_Compare.h
 * @brief Atomic task that compares two values and returns Success or Failure.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_Compare evaluates a comparison between "LHS" and "RHS" parameters
 * using the "Operator" parameter and returns Success if the comparison is
 * true, Failure otherwise.
 *
 * Parameters (ParameterMap):
 *   "LHS"      (Int|Float|String) - left-hand side value (required)
 *   "RHS"      (Int|Float|String) - right-hand side value (required, same type)
 *   "Operator" (String)           - one of: "==", "!=", "<", ">", "<=", ">="
 *                                   (required; "<","<=",">",">=" numeric only)
 *
 * Returns:
 *   Success  if the comparison is true.
 *   Failure  if the comparison is false, or if parameters are invalid.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_Compare
 * @brief Atomic task that returns Success if a comparison holds, Failure otherwise.
 */
class Task_Compare : public IAtomicTask {
public:
    Task_Compare();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
