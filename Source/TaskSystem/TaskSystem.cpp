/**
 * @file TaskSystem.cpp
 * @brief Implementation of TaskSystem: Phase 2.C AtomicTask lifecycle.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * Implements the AtomicTask lifecycle in TaskSystem so that multi-frame tasks
 * (returning TaskStatus::Running) are handled correctly across frames using
 * runner.activeTask, and Abort() is invoked when a running task is cancelled.
 *
 * ### Lifecycle summary
 *  - At node entry, if runner.activeTask == nullptr, a task instance is created
 *    via AtomicTaskRegistry::Create(node.AtomicTaskID) and stored in activeTask.
 *  - Each tick calls activeTask->Execute(params).
 *  - Running  => keep activeTask; accumulate StateTimer; return.
 *  - Success/Failure => reset activeTask; set LastStatus; reset StateTimer;
 *    advance CurrentNodeIndex via TransitionToNextNode().
 *  - If CurrentNodeIndex is NODE_INDEX_NONE when ExecuteNode() is called and
 *    activeTask is non-null, activeTask->Abort() is called before resetting.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem.h"
#include "AtomicTaskRegistry.h"
#include "IAtomicTask.h"
#include "LocalBlackboard.h"
#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

TaskSystem::TaskSystem()
{
    // TODO(Phase 1.5): Set requiredSignature bits for TaskRunnerComponent once
    // the component registration API is integrated with World:
    //
    //   requiredSignature.set(GetComponentTypeID_Static<TaskRunnerComponent>());
}

// ============================================================================
// Process
// ============================================================================

void TaskSystem::Process()
{
    // TODO(Phase 1.5): Retrieve delta-time from GameEngine::fDt when this system
    // runs inside the runtime engine loop.
    const float dt = 0.016f; // placeholder: ~60 fps

    for (const EntityID& entity : m_entities)
    {
        // TODO(Phase 1.5): Retrieve TaskRunnerComponent from World:
        //
        //   TaskRunnerComponent& runner =
        //       World::GetInstance().GetComponent<TaskRunnerComponent>(entity);
        //
        // For the Phase 2.C skeleton, a default-constructed runner is used so
        // that the code path can be exercised without a full World dependency.
        TaskRunnerComponent runner;

        // Skip entities with no bound template.
        const TaskGraphTemplate* tmpl =
            AssetManager::Get().GetTaskGraph(runner.GraphTemplateID);

        if (tmpl == nullptr)
        {
            SYSTEM_LOG << "[TaskSystem] Entity " << entity
                       << " skipped: no valid TaskGraphTemplate bound.\n";
            continue;
        }

        ExecuteNode(entity, runner, tmpl, dt);
    }
}

// ============================================================================
// ExecuteNode
// ============================================================================

void TaskSystem::ExecuteNode(EntityID entity,
                              TaskRunnerComponent& runner,
                              const TaskGraphTemplate* tmpl,
                              float dt)
{
    // NODE_INDEX_NONE signals that there is no active node (graph finished or
    // externally interrupted).  Abort any lingering task and return.
    if (runner.CurrentNodeIndex == NODE_INDEX_NONE)
    {
        if (runner.activeTask)
        {
            SYSTEM_LOG << "[TaskSystem] Entity " << entity
                       << ": node index is NODE_INDEX_NONE with active task"
                       << " - calling Abort()\n";
            runner.activeTask->Abort();
            runner.activeTask.reset();
        }
        return;
    }

    // Look up the current node by its NodeID.
    const TaskNodeDefinition* node = tmpl->GetNode(runner.CurrentNodeIndex);
    if (node == nullptr)
    {
        SYSTEM_LOG << "[TaskSystem] Entity " << entity
                   << ": node ID " << runner.CurrentNodeIndex
                   << " not found in template '" << tmpl->Name << "'\n";
        // Abort any active task associated with the missing node.
        if (runner.activeTask)
        {
            runner.activeTask->Abort();
            runner.activeTask.reset();
        }
        return;
    }

    // Dispatch to the appropriate node type.
    switch (node->Type)
    {
        case TaskNodeType::AtomicTask:
            ExecuteAtomicTask(entity, runner, *node, tmpl, dt);
            break;

        default:
            // Sequence / Selector / Parallel control nodes are not fully
            // implemented in Phase 2.C.  Log and skip.
            SYSTEM_LOG << "[TaskSystem] Entity " << entity
                       << ": control-flow node type "
                       << static_cast<int>(node->Type)
                       << " not yet supported in Phase 2.C\n";
            break;
    }
}

// ============================================================================
// ExecuteAtomicTask (private)
// ============================================================================

void TaskSystem::ExecuteAtomicTask(EntityID entity,
                                    TaskRunnerComponent& runner,
                                    const TaskNodeDefinition& node,
                                    const TaskGraphTemplate* tmpl,
                                    float dt)
{
    // Create the task instance on first entry to this node.
    if (!runner.activeTask)
    {
        runner.activeTask = AtomicTaskRegistry::Get().Create(node.AtomicTaskID);
        if (!runner.activeTask)
        {
            SYSTEM_LOG << "[TaskSystem] Entity " << entity
                       << ": unknown AtomicTaskID '" << node.AtomicTaskID << "'\n";
            runner.LastStatus = TaskRunnerComponent::TaskStatus::Failure;
            TransitionToNextNode(runner, node, false);
            return;
        }
    }

    // Initialize LocalBlackboard: restore persisted state or seed from template defaults.
    LocalBlackboard bb;
    if (!runner.LocalBlackboardData.empty())
    {
        bb.Initialize(*tmpl);
        bb.Deserialize(runner.LocalBlackboardData);
    }
    else
    {
        bb.Initialize(*tmpl);
    }

    // Build parameter map from the node's literal and LocalVariable bindings.
    IAtomicTask::ParameterMap params;

    for (const auto& kv : node.Parameters)
    {
        if (kv.second.Type == ParameterBindingType::Literal)
        {
            params[kv.first] = kv.second.LiteralValue;
        }
        else if (kv.second.Type == ParameterBindingType::LocalVariable)
        {
            if (bb.HasVariable(kv.second.VariableName))
            {
                params[kv.first] = bb.GetValue(kv.second.VariableName);
            }
            else
            {
                SYSTEM_LOG << "[TaskSystem] Entity " << entity
                           << ": LocalVariable '" << kv.second.VariableName
                           << "' not found in template '" << tmpl->Name << "' - skipping binding\n";
            }
        }
    }

    // Tick the task for this frame.
    TaskStatus status = runner.activeTask->Execute(params);

    // Persist LocalBlackboard state so values survive across frames.
    bb.Serialize(runner.LocalBlackboardData);

    // Accumulate time spent in this node on every tick.
    runner.StateTimer += dt;

    if (status == TaskStatus::Running)
    {
        // Task is still in progress: keep activeTask for the next frame.
        return;
    }

    // Task completed (Success or Failure): clean up and transition.
    runner.activeTask.reset();

    runner.LastStatus = (status == TaskStatus::Success)
                            ? TaskRunnerComponent::TaskStatus::Success
                            : TaskRunnerComponent::TaskStatus::Failure;

    TransitionToNextNode(runner, node, status == TaskStatus::Success);
}

// ============================================================================
// TransitionToNextNode (private)
// ============================================================================

void TaskSystem::TransitionToNextNode(TaskRunnerComponent& runner,
                                       const TaskNodeDefinition& node,
                                       bool success)
{
    // Advance to the next NodeID.  NODE_INDEX_NONE means the graph is complete.
    runner.CurrentNodeIndex = success ? node.NextOnSuccess : node.NextOnFailure;

    // Reset the per-node timer on every transition.
    runner.StateTimer = 0.0f;
}

// ============================================================================
// AbortActiveTask (public)
// ============================================================================

void TaskSystem::AbortActiveTask(TaskRunnerComponent& runner)
{
    if (runner.activeTask)
    {
        runner.activeTask->Abort();
        runner.activeTask.reset();
        runner.LastStatus = TaskRunnerComponent::TaskStatus::Aborted;
    }
}

} // namespace Olympe
