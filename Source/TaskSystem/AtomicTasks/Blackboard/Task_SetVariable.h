/**
 * @file Task_SetVariable.h
 * @brief Atomic task that writes a value into the LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Task_SetVariable reads "VariableName" (String) and "Value" (any type) from
 * the parameter map and stores the value into ctx.LocalBB under the given
 * name.  Returns TaskStatus::Success immediately.
 *
 * Parameters:
 *   "VariableName" (String) - key to write in the LocalBlackboard.
 *   "Value"        (any)    - value to store.
 *
 * The variable must already be declared (with a matching type) in the
 * TaskGraphTemplate's LocalVariables list.  If the variable is absent or the
 * types do not match, the task logs a warning and returns Failure.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../IAtomicTask.h"
#include "../../AtomicTaskContext.h"

namespace Olympe {

/**
 * @class Task_SetVariable
 * @brief Writes a named value into the LocalBlackboard and returns Success.
 */
class Task_SetVariable : public IAtomicTask {
public:

    TaskStatus Execute(const ParameterMap& params) override;

    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override;

    void Abort() override;
};

} // namespace Olympe
