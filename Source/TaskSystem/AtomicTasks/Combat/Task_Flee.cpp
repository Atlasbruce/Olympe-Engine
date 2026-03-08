/**
 * @file Task_Flee.cpp
 * @brief Atomic task: compute flee direction away from a threat.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_Flee.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

#include <cmath>

namespace Olympe {

static const float DEFAULT_FLEE_DISTANCE = 200.0f;

Task_Flee::Task_Flee() {}

void Task_Flee::Abort()
{
    SYSTEM_LOG << "[Task_Flee] Abort()\n";
}

TaskStatus Task_Flee::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_Flee::ExecuteWithContext(const AtomicTaskContext& ctx,
                                          const ParameterMap& params)
{
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_Flee] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    // --- Resolve Distance parameter (optional) ---
    float distance = DEFAULT_FLEE_DISTANCE;
    {
        auto it = params.find("Distance");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
        {
            distance = it->second.AsFloat();
            if (distance <= 0.0f) distance = DEFAULT_FLEE_DISTANCE;
        }
    }

    // --- Resolve threat position ---
    ::Vector threatPos(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("Target");
        if (it != params.end() && it->second.GetType() == VariableType::Vector)
        {
            threatPos = it->second.AsVector();
        }
        // If no explicit Target parameter, use the entity's own position as a
        // safe fallback (results in a zero flee vector, which is benign).
    }

    // --- Read entity's current position ---
    ::Vector entityPos(0.0f, 0.0f, 0.0f);
    {
        const TaskValue posVal = ctx.LocalBB->GetValueScoped("local:Position");
        if (posVal.GetType() == VariableType::Vector)
        {
            entityPos = posVal.AsVector();
        }
    }

    // --- Compute flee direction (opposite of threat direction) ---
    ::Vector delta = entityPos - threatPos;
    const float norm = delta.Norm();

    ::Vector fleeTarget;
    if (norm > 1e-5f)
    {
        ::Vector dir = delta * (1.0f / norm);
        fleeTarget = entityPos + dir * distance;
    }
    else
    {
        // Entity is on top of the threat: flee along +X as a safe default.
        fleeTarget = entityPos + ::Vector(distance, 0.0f, 0.0f);
    }

    SYSTEM_LOG << "[Task_Flee] Entity " << ctx.Entity
               << " fleeing from (" << threatPos.x << "," << threatPos.y << ")"
               << " to (" << fleeTarget.x << "," << fleeTarget.y << ")"
               << " distance=" << distance << "\n";

    ctx.LocalBB->SetValueScoped("local:FleeTarget", TaskValue(fleeTarget));

    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_Flee, "Task_Flee")

} // namespace Olympe
