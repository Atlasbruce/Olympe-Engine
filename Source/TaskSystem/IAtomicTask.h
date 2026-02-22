/**
 * @file IAtomicTask.h
 * @brief Interface for atomic tasks in the Atomic Task System.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * IAtomicTask is the base interface that every concrete atomic task must
 * implement.  An atomic task receives a parameter map (string -> TaskValue)
 * at Execute() time and returns a TaskStatus indicating success, failure,
 * or running (for multi-frame tasks).  If the system needs to interrupt
 * a running task it calls Abort() so the task can clean up any state.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <unordered_map>

#include "TaskGraphTypes.h"

namespace Olympe {

/**
 * @enum TaskStatus
 * @brief Result code returned by IAtomicTask::Execute().
 */
enum class TaskStatus : uint8_t {
    Success,    ///< Task completed successfully
    Failure,    ///< Task failed
    Running     ///< Task is still in progress (multi-frame tasks)
};

/**
 * @class IAtomicTask
 * @brief Abstract interface for a single atomic unit of work.
 *
 * @details
 * Implement Execute() to perform the task logic and Abort() to release
 * any resources or state when the task is interrupted before completion.
 * Tasks receive their parameters as a map of TaskValue instances keyed
 * by parameter name.
 *
 * Example:
 * @code
 *   class Task_MyTask : public IAtomicTask {
 *   public:
 *       TaskStatus Execute(const ParameterMap& params) override { ... }
 *       void       Abort()                             override { ... }
 *   };
 * @endcode
 */
class IAtomicTask {
public:

    /// Convenience alias for the parameter map passed to Execute().
    using ParameterMap = std::unordered_map<std::string, TaskValue>;

    virtual ~IAtomicTask() = default;

    /**
     * @brief Executes the atomic task for one frame.
     *
     * @param params  Named parameters provided by the task graph node.
     * @return TaskStatus::Success, Failure, or Running.
     *         Returning Running causes the task to be ticked again next frame.
     */
    virtual TaskStatus Execute(const ParameterMap& params) = 0;

    /**
     * @brief Aborts the task, releasing any in-progress state.
     *
     * Called by the TaskSystem when execution is interrupted (e.g. a parent
     * node is aborted or a new graph is bound while a task is Running).
     * Concrete tasks must implement this to clean up timers, reservations,
     * animations, or any other side-effects started in Execute().
     */
    virtual void Abort() = 0;
};

} // namespace Olympe
