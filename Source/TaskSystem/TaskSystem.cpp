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
 *    advance CurrentNodeID via TransitionToNextNode().
 *  - If CurrentNodeID is NODE_INDEX_NONE when ExecuteNode() is called and
 *    activeTask is non-null, activeTask->Abort() is called before resetting.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem.h"
#include "AtomicTaskRegistry.h"
#include "IAtomicTask.h"
#include "LocalBlackboard.h"
#include "../TaskSystem/AtomicTaskContext.h"
#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Static member definitions
// ============================================================================

TaskEditorPublishFn TaskSystem::s_EditorPublishFn = nullptr;

// ============================================================================
// SetEditorPublishCallback
// ============================================================================

void TaskSystem::SetEditorPublishCallback(TaskEditorPublishFn fn)
{
    s_EditorPublishFn = fn;
}

// ============================================================================
// Constructor
// ============================================================================

TaskSystem::TaskSystem()
{
    // Component signature is set when registering with World in the runtime.
    // In runtime builds: requiredSignature.set(GetComponentTypeID_Static<TaskRunnerComponent>());
}

// ============================================================================
// Process
// ============================================================================

void TaskSystem::Process()
{
    // Delta-time placeholder for headless / test contexts.
    // In the runtime engine, use GameEngine::fDt passed from the game loop.
    const float dt = 0.016f;

    for (const EntityID& entity : m_entities)
    {
        // In the runtime engine, retrieve the actual component via:
        //   TaskRunnerComponent& runner =
        //       World::Get().GetComponent<TaskRunnerComponent>(entity);
        //
        // In test contexts (m_entities populated manually), tasks are driven
        // directly via ExecuteNode() / ExecuteVSFrame(), so the default runner
        // below is only reached when the entity set is populated by the ECS.
        TaskRunnerComponent runner;

        // Resolve the template.  If graphAssetPath is set and no template is
        // bound by AssetID, attempt to load from file.
        const TaskGraphTemplate* tmpl =
            AssetManager::Get().GetTaskGraph(runner.GraphTemplateID);

        if (tmpl == nullptr)
        {
            SYSTEM_LOG << "[TaskSystem] Entity " << entity
                       << " skipped: no valid TaskGraphTemplate bound.\n";
            continue;
        }

        // Primary execution path: ATS Visual Script.
        ExecuteVSFrame(entity, runner, tmpl, dt);
    }
}

// ============================================================================
// ExecuteVSFrame (public)
// ============================================================================

void TaskSystem::ExecuteVSFrame(EntityID entity,
                                 TaskRunnerComponent& runner,
                                 const TaskGraphTemplate* tmpl,
                                 float dt)
{
    // Initialize LocalBlackboard from Blackboard entries if not yet populated.
    // Heuristic: if runner.LocalBlackboard is empty AND tmpl->Blackboard is not empty.
    if (runner.LocalBlackboard.empty() && !tmpl->Blackboard.empty())
    {
        for (size_t i = 0; i < tmpl->Blackboard.size(); ++i)
        {
            const BlackboardEntry& entry = tmpl->Blackboard[i];
            runner.LocalBlackboard[entry.Key] = entry.Default;
        }
    }

    // Build a LocalBlackboard instance for VSGraphExecutor, bridging from
    // runner.LocalBlackboard (unordered_map) to the typed LocalBlackboard class.
    LocalBlackboard localBB;
    localBB.InitializeFromEntries(tmpl->Blackboard);

    // Populate localBB with current runner values (override defaults).
    for (auto it = runner.LocalBlackboard.begin();
         it != runner.LocalBlackboard.end(); ++it)
    {
        if (localBB.HasVariable(it->first))
        {
            try
            {
                localBB.SetValue(it->first, it->second);
            }
            catch (...)
            {
                // Type mismatch or unknown variable — silently skip.
            }
        }
    }

    World* worldPtr = nullptr; // TODO Phase 3: inject real World

    VSGraphExecutor::ExecuteFrame(entity, runner, *tmpl, localBB, worldPtr, dt);

    // Resync runner.LocalBlackboard from localBB after execution.
    const std::vector<std::string>& keys = localBB.GetVariableNames();
    for (size_t i = 0; i < keys.size(); ++i)
    {
        try
        {
            runner.LocalBlackboard[keys[i]] = localBB.GetValue(keys[i]);
        }
        catch (...)
        {
            // Skip variables that fail to read.
        }
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
    if (runner.CurrentNodeID == NODE_INDEX_NONE)
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
    const TaskNodeDefinition* node = tmpl->GetNode(runner.CurrentNodeID);
    if (node == nullptr)
    {
        SYSTEM_LOG << "[TaskSystem] Entity " << entity
                   << ": node ID " << runner.CurrentNodeID
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

    // Initialize LocalBlackboard from template defaults.
    LocalBlackboard bb;
    bb.Initialize(*tmpl);

    // Populate from runner.LocalBlackboard (typed per-entity state).
    for (auto it = runner.LocalBlackboard.begin(); it != runner.LocalBlackboard.end(); ++it)
    {
        if (bb.HasVariable(it->first))
        {
            try { bb.SetValue(it->first, it->second); }
            catch (...) { /* type mismatch — skip */ }
        }
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
    AtomicTaskContext ctx;
    ctx.Entity     = entity;
    ctx.WorldPtr   = nullptr; // World integration deferred to Phase 1.5
    ctx.LocalBB    = &bb;
    ctx.DeltaTime  = dt;
    ctx.StateTimer = runner.StateTimer;

    TaskStatus status = runner.activeTask->ExecuteWithContext(ctx, params);

    // Persist LocalBlackboard state so values survive across frames.
    const std::vector<std::string>& keys = bb.GetVariableNames();
    for (size_t i = 0; i < keys.size(); ++i)
    {
        try { runner.LocalBlackboard[keys[i]] = bb.GetValue(keys[i]); }
        catch (...) {}
    }

    // Accumulate time spent in this node on every tick.
    runner.StateTimer += dt;

    if (status == TaskStatus::Running)
    {
        // Task is still in progress: keep activeTask for the next frame.
        // Publish live runtime info to the editor if a callback is registered.
        if (s_EditorPublishFn != nullptr)
        {
            s_EditorPublishFn(entity, runner.CurrentNodeID, &bb);
        }
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
    runner.CurrentNodeID = success ? node.NextOnSuccess : node.NextOnFailure;

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
