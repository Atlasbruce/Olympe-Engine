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
 * Phase 1.4 provides a minimal skeleton.  Full atomic-task dispatch and
 * World-integrated component retrieval will be added in Phase 1.5+.
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
     *  1. Retrieves the TaskRunnerComponent.
     *  2. Looks up the bound TaskGraphTemplate via AssetManager::Get().
     *  3. Skips the entity if no valid template is found.
     *  4. Calls ExecuteNode() to advance task graph execution.
     *
     * @note Phase 1.4: component retrieval from World is deferred (TODO Phase 1.5).
     */
    virtual void Process() override;

private:

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Advances execution of one node in the task graph for the given entity.
     *
     * @param entity  The entity being processed.
     * @param runner  Reference to the entity's TaskRunnerComponent.
     * @param tmpl    Non-null pointer to the resolved TaskGraphTemplate.
     * @param dt      Delta-time in seconds for the current frame.
     *
     * @note Phase 1.4 skeleton: logs the current node and returns immediately.
     *       Full atomic-task dispatch will be implemented in Phase 1.5+.
     */
    void ExecuteNode(EntityID entity,
                     TaskRunnerComponent& runner,
                     const TaskGraphTemplate* tmpl,
                     float dt);
};

} // namespace Olympe
