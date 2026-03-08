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
 * execution through the bound task graph.  activeTask stores the IAtomicTask
 * instance currently executing so that multi-frame Running tasks can be
 * re-ticked and cleanly Abort()ed when needed.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../Core/AssetManager.h"             ///< Provides AssetID and INVALID_ASSET_ID
#include "../../TaskSystem/TaskGraphTypes.h"     ///< Provides TaskValue and NODE_INDEX_NONE

namespace Olympe {

// Forward declaration: full type is only required in TaskRunnerComponent.cpp
// where the destructor is defined (unique_ptr delete requires the complete type).
class IAtomicTask;

/**
 * @struct TaskRunnerComponent
 * @brief Per-entity runtime state for task graph execution.
 *
 * @details
 * Fields:
 *   - GraphTemplateID     : Identifies which TaskGraphTemplate drives this runner.
 *   - CurrentNodeID       : ID of the currently active node (replaces CurrentNodeIndex).
 *   - CurrentNodeIndex    : @deprecated Index into Nodes vector; use CurrentNodeID instead.
 *   - StateTimer          : Accumulated time (seconds) spent in the current node.
 *   - LocalBlackboard     : Typed per-entity blackboard state (replaces LocalBlackboardData).
 *   - LocalBlackboardData : @deprecated Raw byte buffer; kept for transitional compatibility.
 *   - LastStatus          : Result of the most recently completed node execution.
 *   - activeTask          : Owning pointer to the IAtomicTask instance currently executing.
 *   - ActiveExecPinName   : Name of the active exec pin on the current node.
 *   - SequenceChildIndex  : For VSSequence: index of the next child to execute.
 *   - DoOnceFlags         : Per-node "already fired" flag (for DoOnce nodes).
 *   - DataPinCache        : Frame-local cache of computed data pin values.
 *
 * @note TaskRunnerComponent is move-only because activeTask is a unique_ptr.
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

    /// @brief Path to the ATS Visual Script JSON asset file (.json).
    ///        When set, TaskSystem can load the template from this path via
    ///        TaskGraphLoader::LoadFromFile() if GraphTemplateID is not yet
    ///        resolved by AssetManager.
    std::string graphAssetPath;

    /// @brief ID of the currently active node (replaces CurrentNodeIndex).
    int32_t CurrentNodeID    = NODE_INDEX_NONE;

    /// @deprecated Use CurrentNodeID instead.
    int32_t CurrentNodeIndex = 0;

    /// @brief Accumulated time (in seconds) spent in the current node state.
    float StateTimer = 0.0f;

    /// @brief Typed local blackboard for per-entity state (replaces LocalBlackboardData raw bytes).
    std::unordered_map<std::string, Olympe::TaskValue> LocalBlackboard;

    /// @deprecated Raw byte buffer – kept for transitional compatibility, will be removed in Phase 2.
    std::vector<uint8_t> LocalBlackboardData;

    /// @brief Status returned by the last completed node execution.
    TaskStatus LastStatus = TaskStatus::Success;

    /// @brief Owning pointer to the IAtomicTask instance that is currently
    ///        executing for this runner.  nullptr when no task is in flight.
    ///        Persisted across frames so that tasks returning Running are
    ///        re-ticked on the next Process() call.  Replaced (old task
    ///        Abort()ed) when the system advances to a different node.
    std::unique_ptr<IAtomicTask> activeTask;

    /// @brief Name of the active exec pin on the current node (e.g. "Then", "Else", "Loop").
    std::string ActiveExecPinName;

    /// @brief For VSSequence: index of the next child to execute.
    int32_t SequenceChildIndex = 0;

    /// @brief Per-node "already fired" flag for DoOnce nodes (key = nodeID).
    std::unordered_map<int32_t, bool> DoOnceFlags;

    /// @brief Frame-local cache of computed data pin values (key = "nodeID:pinName").
    std::unordered_map<std::string, Olympe::TaskValue> DataPinCache;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    TaskRunnerComponent();
    ~TaskRunnerComponent();

    // Move only (unique_ptr member makes copy ill-formed).
    TaskRunnerComponent(TaskRunnerComponent&&) = default;
    TaskRunnerComponent& operator=(TaskRunnerComponent&&) = default;

    TaskRunnerComponent(const TaskRunnerComponent&) = delete;
    TaskRunnerComponent& operator=(const TaskRunnerComponent&) = delete;
};

} // namespace Olympe
