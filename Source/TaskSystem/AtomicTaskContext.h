/**
 * @file AtomicTaskContext.h
 * @brief Runtime context passed to IAtomicTask::ExecuteWithContext().
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * AtomicTaskContext bundles the per-frame runtime data that a concrete atomic
 * task may need without having to query singletons directly.  It is built by
 * TaskSystem::ExecuteAtomicTask() and forwarded to
 * IAtomicTask::ExecuteWithContext() each tick.
 *
 * Passing nullptr for WorldPtr is valid; callers that do not have access to a
 * live World (e.g. unit tests) simply leave it null and tasks that need the
 * World must guard against a null pointer.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../ECS_Entity.h"

namespace Olympe {

// Forward declarations - avoids pulling in heavy headers from task headers.
class World;
class LocalBlackboard;

/**
 * @struct AtomicTaskContext
 * @brief Lightweight context bundle passed to IAtomicTask::ExecuteWithContext().
 */
struct AtomicTaskContext
{
    /// The entity whose task graph is being executed.
    EntityID        Entity     = INVALID_ENTITY_ID;

    /// Pointer to the active World.  May be nullptr in headless / test contexts.
    World*          WorldPtr   = nullptr;

    /// Pointer to the task node's LocalBlackboard for this tick.  Never nullptr.
    LocalBlackboard* LocalBB   = nullptr;

    /// Delta-time in seconds for the current frame.
    float           DeltaTime  = 0.0f;

    /// Time in seconds that the current task node has been running (accumulated).
    float           StateTimer = 0.0f;
};

} // namespace Olympe
