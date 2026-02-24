/**
 * @file MovementComponent.h
 * @brief ECS component storing an entity's velocity and maximum speed.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * MovementComponent is a plain-data ECS component used by movement tasks
 * (e.g. Task_MoveToLocation in World mode).  The task sets Velocity each
 * frame; a physics/movement ECS system is expected to integrate the velocity
 * and update the PositionComponent.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../vector.h"

namespace Olympe {

/**
 * @struct MovementComponent
 * @brief ECS component: entity velocity and movement constraints.
 */
struct MovementComponent
{
    /// Current velocity vector (units per second) set by movement tasks.
    ::Vector Velocity;

    /// Maximum movement speed (units per second).  0.0 means unconstrained.
    float MaxSpeed = 0.0f;
};

} // namespace Olympe
