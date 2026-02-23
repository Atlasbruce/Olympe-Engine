/**
 * @file TaskSystem.h
 * @brief ECS system that iterates TaskRunnerComponent entities and drives
 *        task graph execution each frame.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * TaskSystem inherits from ECS_System and is registered in the World alongside
 * all other runtime systems.  Each frame, Process() iterates the set of
 * entities that own a TaskRunnerComponent, retrieves the bound
 * TaskGraphTemplate via AssetManager, and delegates to ExecuteNode() to
 * advance execution.
 *
 * ### AtomicTask lifecycle (Phase 2.C)
 * ExecuteNode() implements the following lifecycle for AtomicTask nodes:
 *
 *  1. On first entry to a node, create the IAtomicTask instance via
 *     AtomicTaskRegistry::Create() and store it in runner.activeTask.
 *  2. Each tick, call runner.activeTask->Execute(params).
 *  3. If Execute returns TaskStatus::Running, keep activeTask and return -
 *     the task will be ticked again on the next frame.
 *  4. If Execute returns Success or Failure, destroy activeTask (reset the
 *     unique_ptr), set runner.LastStatus, reset runner.StateTimer, and
 *     advance runner.CurrentNodeIndex to NextOnSuccess / NextOnFailure.
 *     NODE_INDEX_NONE is used as the sentinel "graph complete" value.
 *  5. If runner.CurrentNodeIndex is set to NODE_INDEX_NONE externally while
 *     a task is Running (e.g. the entity is removed or the graph is
 *     interrupted), the next call to ExecuteNode() calls activeTask->Abort()
 *     before releasing the instance.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../ECS_Systems.h"
#include "../ECS/Components/TaskRunnerComponent.h"
#include "TaskGraphTemplate.h"
#include "../Core/AssetManager.h"

namespace Olympe {

/**
 * @class TaskSystem
 * @brief ECS system responsible for advancing task graph execution each frame.
 *
 * @details
 * Usage:
 * @code
 *   TaskSystem* taskSystem = world.RegisterSystem<TaskSystem>();
 *   // TaskSystem sets its own requiredSignature in the constructor.
 * @endcode
 *
 * Inheritance: TaskSystem : ECS_System
 */
class TaskSystem : public ECS_System
{
public:

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Constructs the system and configures the required component signature.
     *
     * @note Signature configuration is left as a TODO until the component
     *       registration API is fully wired up in Phase 1.5.
     */
    TaskSystem();

    // -----------------------------------------------------------------------
    // ECS_System interface
    // -----------------------------------------------------------------------

    /**
     * @brief Processes all entities registered with this system for one frame.
     *
     * For each entity in m_entities:
     *  1. Retrieves the TaskRunnerComponent (TODO Phase 1.5: from World).
     *  2. Looks up the bound TaskGraphTemplate via AssetManager::Get().
     *  3. Skips the entity if no valid template is found.
     *  4. Calls ExecuteNode() to advance task graph execution.
     */
    virtual void Process() override;

    // -----------------------------------------------------------------------
    // Core execution (public to allow direct invocation in unit tests)
    // -----------------------------------------------------------------------

    /**
     * @brief Advances execution of one node in the task graph for the given entity.
     *
     * @details
     * Implements the AtomicTask lifecycle described in the class documentation.
     * runner.CurrentNodeIndex is treated as a NodeID (looked up via
     * TaskGraphTemplate::GetNode()).  NODE_INDEX_NONE (-1) means there is no
     * active node; any lingering activeTask is Abort()ed and released.
     *
     * @param entity  The entity being processed.
     * @param runner  Reference to the entity's TaskRunnerComponent.
     * @param tmpl    Non-null pointer to the resolved TaskGraphTemplate.
     * @param dt      Delta-time in seconds for the current frame.
     */
    void ExecuteNode(EntityID entity,
                     TaskRunnerComponent& runner,
                     const TaskGraphTemplate* tmpl,
                     float dt);

    /**
     * @brief Aborts the active atomic task on a runner, if any.
     *
     * @details
     * Calls Abort() on runner.activeTask (if non-null), resets the unique_ptr,
     * and sets runner.LastStatus to TaskRunnerComponent::TaskStatus::Aborted.
     * Safe to call when runner.activeTask is nullptr (no-op).
     *
     * @param runner  Reference to the entity's TaskRunnerComponent.
     */
    void AbortActiveTask(TaskRunnerComponent& runner);

private:

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Executes one tick of an AtomicTask node.
     *
     * Creates runner.activeTask on first call for this node, ticks it, and
     * handles Running / completion transitions.
     *
     * @param entity  The entity being processed.
     * @param runner  Reference to the entity's TaskRunnerComponent.
     * @param node    The current AtomicTask node definition.
     * @param tmpl    Non-null pointer to the resolved TaskGraphTemplate (used
     *                to resolve LocalVariable bindings).
     * @param dt      Delta-time in seconds for the current frame.
     */
    void ExecuteAtomicTask(EntityID entity,
                           TaskRunnerComponent& runner,
                           const TaskNodeDefinition& node,
                           const TaskGraphTemplate* tmpl,
                           float dt);

    /**
     * @brief Advances runner.CurrentNodeIndex after a node completes.
     *
     * Sets runner.CurrentNodeIndex to node.NextOnSuccess if @p success is
     * true, or node.NextOnFailure otherwise.  NODE_INDEX_NONE signals that
     * the graph has finished.  Also resets runner.StateTimer to 0.
     *
     * @param runner   The runner component to update.
     * @param node     The node that just completed.
     * @param success  true if the node succeeded, false if it failed.
     */
    void TransitionToNextNode(TaskRunnerComponent& runner,
                              const TaskNodeDefinition& node,
                              bool success);
};

} // namespace Olympe
