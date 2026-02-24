/**
 * @file Task_MoveToLocation.cpp
 * @brief Atomic task: move entity toward a target location.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Dual-mode movement task.
 *
 * World mode (ctx.ComponentFacade with Position and Movement set):
 *   Reads the entity's PositionComponent and writes to its MovementComponent.
 *   Sets Velocity = dir * speed while the entity is farther than
 *   ARRIVAL_TOLERANCE from the target; zeroes Velocity and returns Success
 *   once it arrives.  A physics/movement ECS system is responsible for
 *   integrating the velocity.
 *
 * Headless mode (ctx.ComponentFacade is nullptr or components are absent):
 *   Reads and writes the "Position" key in ctx.LocalBB, integrating position
 *   analytically using ctx.DeltaTime.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_MoveToLocation.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../TaskWorldFacade.h"
#include "../../../system/system_utils.h"

#include <cmath>

namespace Olympe {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static const float DEFAULT_SPEED      = 100.0f;
static const float ARRIVAL_TOLERANCE  = 0.5f;
static const char* BB_KEY_POSITION    = "Position";

// ---------------------------------------------------------------------------
// Constructor / Abort
// ---------------------------------------------------------------------------

Task_MoveToLocation::Task_MoveToLocation() {}

void Task_MoveToLocation::Abort()
{
    SYSTEM_LOG << "[Task_MoveToLocation] Abort()\n";
}

// ---------------------------------------------------------------------------
// Execute (legacy fallback - not used when ExecuteWithContext is available)
// ---------------------------------------------------------------------------

TaskStatus Task_MoveToLocation::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

// ---------------------------------------------------------------------------
// ExecuteWithContext
// ---------------------------------------------------------------------------

TaskStatus Task_MoveToLocation::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                    const ParameterMap& params)
{
    // --- Resolve Target parameter ---
    ::Vector target(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("Target");
        if (it != params.end() && it->second.GetType() == VariableType::Vector)
        {
            target = it->second.AsVector();
        }
        else
        {
            SYSTEM_LOG << "[Task_MoveToLocation] Missing or invalid 'Target' parameter\n";
            return TaskStatus::Failure;
        }
    }

    // --- Resolve Speed parameter (optional) ---
    float speed = DEFAULT_SPEED;
    {
        auto it = params.find("Speed");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
        {
            speed = it->second.AsFloat();
            if (speed <= 0.0f) speed = DEFAULT_SPEED;
        }
    }

    // -----------------------------------------------------------------------
    // World mode: use ECS PositionComponent + MovementComponent when available
    // -----------------------------------------------------------------------
    if (ctx.ComponentFacade
        && ctx.ComponentFacade->Position
        && ctx.ComponentFacade->Movement)
    {
        ::Vector& pos      = ctx.ComponentFacade->Position->Position;
        ::Vector& velocity = ctx.ComponentFacade->Movement->Velocity;

        ::Vector delta = target - pos;
        float dist     = delta.Norm();

        SYSTEM_LOG << "[Task_MoveToLocation] (WorldMode) Entity " << ctx.Entity
                   << " pos=(" << pos.x << "," << pos.y << ")"
                   << " target=(" << target.x << "," << target.y << ")"
                   << " dist=" << dist << "\n";

        if (dist <= ARRIVAL_TOLERANCE)
        {
            velocity = ::Vector(0.0f, 0.0f, 0.0f);
            SYSTEM_LOG << "[Task_MoveToLocation] (WorldMode) Entity " << ctx.Entity
                       << " reached target - Success\n";
            return TaskStatus::Success;
        }

        ::Vector dir = delta * (1.0f / dist);
        velocity     = dir * speed;
        return TaskStatus::Running;
    }

    // -----------------------------------------------------------------------
    // Headless mode: read / write "Position" via LocalBlackboard
    // -----------------------------------------------------------------------

    // --- Read current position from LocalBlackboard ---
    if (!ctx.LocalBB || !ctx.LocalBB->HasVariable(BB_KEY_POSITION))
    {
        SYSTEM_LOG << "[Task_MoveToLocation] 'Position' key not found in LocalBlackboard\n";
        return TaskStatus::Failure;
    }

    ::Vector pos;
    try
    {
        pos = ctx.LocalBB->GetValue(BB_KEY_POSITION).AsVector();
    }
    catch (...)
    {
        SYSTEM_LOG << "[Task_MoveToLocation] Failed to read 'Position' from LocalBlackboard\n";
        return TaskStatus::Failure;
    }

    // --- Compute direction and step ---
    ::Vector delta = target - pos;
    float dist = delta.Norm();

    SYSTEM_LOG << "[Task_MoveToLocation] Entity " << ctx.Entity
               << " pos=(" << pos.x << "," << pos.y << ")"
               << " target=(" << target.x << "," << target.y << ")"
               << " dist=" << dist << "\n";

    float step = speed * ctx.DeltaTime;

    if (dist <= ARRIVAL_TOLERANCE || dist <= step)
    {
        // Snap to target and report success.
        try
        {
            ctx.LocalBB->SetValue(BB_KEY_POSITION, TaskValue(target));
        }
        catch (...) {}

        SYSTEM_LOG << "[Task_MoveToLocation] Entity " << ctx.Entity
                   << " reached target - Success\n";
        return TaskStatus::Success;
    }

    // Move step units toward target.
    ::Vector dir = delta * (step / dist);
    ::Vector newPos = pos + dir;

    try
    {
        ctx.LocalBB->SetValue(BB_KEY_POSITION, TaskValue(newPos));
    }
    catch (...)
    {
        SYSTEM_LOG << "[Task_MoveToLocation] Failed to write 'Position' to LocalBlackboard\n";
        return TaskStatus::Failure;
    }

    return TaskStatus::Running;
}

REGISTER_ATOMIC_TASK(Task_MoveToLocation, "Task_MoveToLocation")

} // namespace Olympe
