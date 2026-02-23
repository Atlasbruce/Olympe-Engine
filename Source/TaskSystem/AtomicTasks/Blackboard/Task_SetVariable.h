/**
 * @file Task_SetVariable.h
 * @brief Atomic task that writes a value into the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_SetVariable writes the "Value" parameter into the LocalBlackboard
 * variable named by the "VarName" parameter.
 *
 * Parameters (ParameterMap):
 *   "VarName" (String) - name of the blackboard variable to set (required)
 *   "Value"   (any)    - the value to write; type must match the variable's
 *                        declared type (required)
 *
 * Returns:
 *   Success  on successful write.
 *   Failure  if VarName is missing, Value is missing, or a type mismatch
 *            occurs.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"

namespace Olympe {

/**
 * @class Task_SetVariable
 * @brief Atomic task that writes a value into the LocalBlackboard.
 */
class Task_SetVariable : public IAtomicTask {
public:
    Task_SetVariable();

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    TaskStatus Execute(const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
