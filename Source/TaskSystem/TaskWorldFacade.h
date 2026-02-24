/**
 * @file TaskWorldFacade.h
 * @brief Lightweight ECS component accessor bridge for the Task System.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * TaskWorldFacade provides a simple, SDL-free bridge between the Task System
 * and the ECS layer.  It holds raw pointers to the PositionComponent and
 * MovementComponent that belong to the entity currently executing a task.
 *
 * In production, the ECS system responsible for driving task execution
 * (e.g. a TaskExecutionSystem) populates a TaskWorldFacade and places it in
 * the AtomicTaskContext before calling TaskSystem::ExecuteNode().
 *
 * In unit tests, a TaskWorldFacade is built directly by the test, pointing at
 * stack-allocated component instances.  This allows the World mode branch of
 * Task_MoveToLocation (and similar tasks) to be exercised without requiring a
 * live World / SDL context.
 *
 * Design notes
 * ------------
 * - Raw (non-owning) pointers are intentional: the lifetime of the pointed-to
 *   components is managed by the caller (ECS system or test fixture).
 * - nullptr members are safe: tasks MUST check for nullptr before use and fall
 *   back to the LocalBlackboard path when components are absent.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../ECS/Components/PositionComponent.h"
#include "../ECS/Components/MovementComponent.h"

namespace Olympe {

/**
 * @struct TaskWorldFacade
 * @brief Lightweight ECS component accessor passed through AtomicTaskContext.
 *
 * @details
 * Both pointers default to nullptr.  A task that supports World mode checks
 * both pointers before using them; if either is nullptr the task falls back
 * to its LocalBlackboard (headless) code path.
 */
struct TaskWorldFacade
{
    /// Non-owning pointer to the entity's PositionComponent.  May be nullptr.
    PositionComponent* Position = nullptr;

    /// Non-owning pointer to the entity's MovementComponent.  May be nullptr.
    MovementComponent* Movement = nullptr;
};

} // namespace Olympe
