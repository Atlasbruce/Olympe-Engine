/**
 * @file Task_MoveToLocation.cpp
 * @brief Implementation of Task_MoveToLocation atomic task.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Moves the entity's LocalBlackboard "Position" toward the "Target" parameter
 * at the given "Speed" (units/s) each tick.  Returns Running until within
 * "AcceptanceRadius", then Success.  Abort() is a no-op because there is no
 * external resource to release.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_MoveToLocation.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

#include <cmath>

namespace Olympe {

// ============================================================================
// Execute (legacy path - context required for movement, return Failure)
// ============================================================================

TaskStatus Task_MoveToLocation::Execute(const ParameterMap& /*params*/)
{
    // This task requires a full AtomicTaskContext to operate.  Callers must
    // use ExecuteWithContext().  Returning Failure here guards against
    // accidental use of the context-free path.
    return TaskStatus::Failure;
}

// ============================================================================
// ExecuteWithContext
// ============================================================================

TaskStatus Task_MoveToLocation::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                    const ParameterMap& params)
{
    // ------------------------------------------------------------------
    // Read parameters
    // ------------------------------------------------------------------

    Vector target(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("Target");
        if (it != params.end() && it->second.GetType() == VariableType::Vector)
            target = it->second.AsVector();
    }

    float speed = 100.0f;
    {
        auto it = params.find("Speed");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
            speed = it->second.AsFloat();
    }

    float acceptanceRadius = 5.0f;
    {
        auto it = params.find("AcceptanceRadius");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
            acceptanceRadius = it->second.AsFloat();
    }

    // ------------------------------------------------------------------
    // Read current position from LocalBlackboard
    // ------------------------------------------------------------------

    Vector position(0.0f, 0.0f, 0.0f);
    if (ctx.LocalBB && ctx.LocalBB->HasVariable("Position"))
    {
        const TaskValue& posVal = ctx.LocalBB->GetValue("Position");
        if (posVal.GetType() == VariableType::Vector)
            position = posVal.AsVector();
    }

    // ------------------------------------------------------------------
    // Compute distance to target
    // ------------------------------------------------------------------

    const float dx   = target.x - position.x;
    const float dy   = target.y - position.y;
    const float dz   = target.z - position.z;
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    SYSTEM_LOG << "[Task_MoveToLocation] Entity " << ctx.Entity
               << " pos=(" << position.x << "," << position.y << ")"
               << " dist=" << dist << " acceptance=" << acceptanceRadius << "\n";

    // ------------------------------------------------------------------
    // Arrival check
    // ------------------------------------------------------------------

    if (dist <= acceptanceRadius)
    {
        // Clear velocity on arrival.
        if (ctx.LocalBB && ctx.LocalBB->HasVariable("Velocity"))
        {
            ctx.LocalBB->SetValue("Velocity", TaskValue(Vector(0.0f, 0.0f, 0.0f)));
        }

        SYSTEM_LOG << "[Task_MoveToLocation] Entity " << ctx.Entity
                   << " arrived at target - Success\n";
        return TaskStatus::Success;
    }

    // ------------------------------------------------------------------
    // Move toward target
    // ------------------------------------------------------------------

    const float invDist  = 1.0f / dist;
    const float dirX     = dx * invDist;
    const float dirY     = dy * invDist;
    const float dirZ     = dz * invDist;

    float moveStep = speed * ctx.DeltaTime;
    if (moveStep > dist)
        moveStep = dist;

    const Vector newPos(position.x + dirX * moveStep,
                        position.y + dirY * moveStep,
                        position.z + dirZ * moveStep);
    const Vector velocity(dirX * speed, dirY * speed, dirZ * speed);

    if (ctx.LocalBB && ctx.LocalBB->HasVariable("Position"))
        ctx.LocalBB->SetValue("Position", TaskValue(newPos));

    if (ctx.LocalBB && ctx.LocalBB->HasVariable("Velocity"))
        ctx.LocalBB->SetValue("Velocity", TaskValue(velocity));

    return TaskStatus::Running;
}

// ============================================================================
// Abort
// ============================================================================

void Task_MoveToLocation::Abort()
{
    // No external resource to release.  The LocalBlackboard is not accessible
    // here; Velocity will remain at its last written value until the next task
    // resets it or the runner is re-initialized.
    SYSTEM_LOG << "[Task_MoveToLocation] Abort() called\n";
}

REGISTER_ATOMIC_TASK(Task_MoveToLocation, "Task_MoveToLocation")

} // namespace Olympe
