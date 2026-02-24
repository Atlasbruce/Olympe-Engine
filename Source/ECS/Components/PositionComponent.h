/**
 * @file PositionComponent.h
 * @brief ECS component storing an entity's world-space position.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * PositionComponent is a plain-data ECS component that stores the 3-D
 * position of an entity in world space.  It is used by movement systems
 * (e.g. Task_MoveToLocation in World mode) to read and write entity positions.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "../../vector.h"

namespace Olympe {

/**
 * @struct PositionComponent
 * @brief ECS component: entity world-space position.
 */
struct PositionComponent
{
    /// World-space position of the entity.
    ::Vector Position;
};

} // namespace Olympe
