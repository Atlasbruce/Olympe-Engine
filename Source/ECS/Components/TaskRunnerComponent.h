/**
 * @file TaskRunnerComponent.h
 * @brief ECS component that drives task graph execution at runtime.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * TaskRunnerComponent holds the per-entity runtime state required to execute
 * a TaskGraphTemplate.  One component instance is attached to each entity
 * that participates in the Atomic Task System.
 *
 * The TaskSystem reads and updates this component every frame to advance
 * execution through the bound task graph.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "../Core/AssetManager.h"  ///< Provides AssetID and INVALID_ASSET_ID

namespace Olympe {

/**
 * @struct TaskRunnerComponent
 * @brief Per-entity runtime state for task graph execution.
 *
 * @details
 * Fields:
 *   - GraphTemplateID     : Identifies which TaskGraphTemplate drives this runner.
 *   - CurrentNodeIndex    : Index into the template's Nodes vector of the active node.
 *   - StateTimer          : Accumulated time (seconds) spent in the current node.
 *   - LocalBlackboardData : Raw byte buffer for per-entity blackboard state.
 *   - LastStatus          : Result of the most recently completed node execution.
 */
struct TaskRunnerComponent
{
    // -----------------------------------------------------------------------
    // Status enumeration
    // -----------------------------------------------------------------------

    /**
     * @enum TaskStatus
     * @brief Possible outcomes of a task node execution.
     */
    enum class TaskStatus : uint8_t
    {
        Success, ///< The node completed successfully.
        Failure, ///< The node failed.
        Running, ///< The node is still executing (will be called again next frame).
        Aborted  ///< Execution was interrupted externally.
    };

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------

    /// @brief AssetID of the TaskGraphTemplate driving this runner.
    ///        Set to INVALID_ASSET_ID when no template is bound.
    AssetID GraphTemplateID = INVALID_ASSET_ID;

    /// @brief Index of the currently active node in the template's Nodes vector.
    int32_t CurrentNodeIndex = 0;

    /// @brief Accumulated time (in seconds) spent in the current node state.
    float StateTimer = 0.0f;

    /// @brief Raw byte buffer for per-entity local blackboard data.
    ///        The schema and layout are defined by the bound TaskGraphTemplate.
    std::vector<uint8_t> LocalBlackboardData;

    /// @brief Status returned by the last completed node execution.
    TaskStatus LastStatus = TaskStatus::Success;

    // -----------------------------------------------------------------------
    // Constructors (rule of zero - all members self-initialise)
    // -----------------------------------------------------------------------

    TaskRunnerComponent() = default;
    TaskRunnerComponent(const TaskRunnerComponent&) = default;
    TaskRunnerComponent& operator=(const TaskRunnerComponent&) = default;
};

} // namespace Olympe
