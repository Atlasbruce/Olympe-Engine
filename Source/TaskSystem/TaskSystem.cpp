/**
 * @file TaskSystem.cpp
 * @brief Implementation of TaskSystem: Phase 1.4 skeleton.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * This file provides the minimal skeleton required by Phase 1.4.
 * Full implementation (World-integrated component retrieval, atomic-task
 * dispatch, blackboard management) is deferred to Phase 1.5+.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/TaskSystem.h"
#include "system/system_utils.h"

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
        // For the Phase 1.4 skeleton, a default-constructed runner is used so
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
// ExecuteNode (stub)
// ============================================================================

void TaskSystem::ExecuteNode(EntityID entity,
                              TaskRunnerComponent& runner,
                              const TaskGraphTemplate* tmpl,
                              float dt)
{
    // TODO(Phase 1.5): Dispatch to atomic tasks, update runner.LastStatus,
    // advance runner.CurrentNodeIndex, and manage runner.StateTimer.
    SYSTEM_LOG << "[TaskSystem] ExecuteNode entity=" << entity
               << " node=" << runner.CurrentNodeIndex
               << " template=" << tmpl->Name
               << "\n";

    // Suppress unused-parameter warning for dt in the skeleton.
    (void)dt;
}

} // namespace Olympe
