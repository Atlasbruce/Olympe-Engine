/**
 * @file Task_Flee.cpp
 * @brief Atomic AI task: move entity away from a 2D threat position.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_Flee.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../TaskWorldFacade.h"
#include "../../../system/system_utils.h"

#include <cmath>

namespace Olympe {

static const float DEFAULT_FLEE_DISTANCE = 200.0f;
static const float FLEE_MOVE_SPEED       = 250.0f;

Task_Flee::Task_Flee() {}

void Task_Flee::Abort()
{
    SYSTEM_LOG << "[Task_Flee(AI)] Abort()\n";
}

TaskStatus Task_Flee::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_Flee::ExecuteWithContext(const AtomicTaskContext& ctx,
                                          const ParameterMap& params)
{
    // --- Resolve TargetX parameter ---
    float threatX = 0.0f;
    {
        auto it = params.find("TargetX");
        if (it == params.end())
        {
            SYSTEM_LOG << "[Task_Flee(AI)] Missing 'TargetX' parameter\n";
            return TaskStatus::Failure;
        }
        if (it->second.GetType() == VariableType::Float)
            threatX = it->second.AsFloat();
        else if (it->second.GetType() == VariableType::Int)
            threatX = static_cast<float>(it->second.AsInt());
    }

    // --- Resolve TargetY parameter ---
    float threatY = 0.0f;
    {
        auto it = params.find("TargetY");
        if (it == params.end())
        {
            SYSTEM_LOG << "[Task_Flee(AI)] Missing 'TargetY' parameter\n";
            return TaskStatus::Failure;
        }
        if (it->second.GetType() == VariableType::Float)
            threatY = it->second.AsFloat();
        else if (it->second.GetType() == VariableType::Int)
            threatY = static_cast<float>(it->second.AsInt());
    }

    // --- Resolve Distance parameter (optional) ---
    float safeDistance = DEFAULT_FLEE_DISTANCE;
    {
        auto it = params.find("Distance");
        if (it != params.end())
        {
            if (it->second.GetType() == VariableType::Float)
                safeDistance = it->second.AsFloat();
            else if (it->second.GetType() == VariableType::Int)
                safeDistance = static_cast<float>(it->second.AsInt());
            if (safeDistance <= 0.0f) safeDistance = DEFAULT_FLEE_DISTANCE;
        }
    }

    ::Vector threat(threatX, threatY, 0.0f);

    // -----------------------------------------------------------------------
    // World mode
    // -----------------------------------------------------------------------
    if (ctx.ComponentFacade
        && ctx.ComponentFacade->Position
        && ctx.ComponentFacade->Movement)
    {
        ::Vector& pos      = ctx.ComponentFacade->Position->Position;
        ::Vector& velocity = ctx.ComponentFacade->Movement->Velocity;

        ::Vector delta = pos - threat;
        delta.z        = 0.0f;
        const float dist = delta.Norm();

        SYSTEM_LOG << "[Task_Flee(AI)] (WorldMode) Entity " << ctx.Entity
                   << " pos=(" << pos.x << "," << pos.y << ")"
                   << " threat=(" << threatX << "," << threatY << ")"
                   << " dist=" << dist << " safe=" << safeDistance << "\n";

        if (dist >= safeDistance)
        {
            velocity = ::Vector(0.0f, 0.0f, 0.0f);
            return TaskStatus::Success;
        }

        // Move away from threat.
        if (dist > 1e-5f)
        {
            velocity = delta * (FLEE_MOVE_SPEED / dist);
        }
        else
        {
            // Entity is on top of threat; flee along +X.
            velocity = ::Vector(FLEE_MOVE_SPEED, 0.0f, 0.0f);
        }
        return TaskStatus::Running;
    }

    // -----------------------------------------------------------------------
    // Headless mode: read/write "Position" via LocalBlackboard
    // -----------------------------------------------------------------------
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_Flee(AI)] No LocalBlackboard and no ComponentFacade\n";
        return TaskStatus::Success;
    }

    ::Vector pos(0.0f, 0.0f, 0.0f);
    {
        const TaskValue posVal = ctx.LocalBB->GetValueScoped("local:Position");
        if (posVal.GetType() == VariableType::Vector)
        {
            pos = posVal.AsVector();
        }
        else if (ctx.LocalBB->HasVariable("Position"))
        {
            try { pos = ctx.LocalBB->GetValue("Position").AsVector(); }
            catch (...) {}
        }
    }

    ::Vector delta = pos - threat;
    delta.z        = 0.0f;
    const float dist = delta.Norm();

    SYSTEM_LOG << "[Task_Flee(AI)] Entity " << ctx.Entity
               << " pos=(" << pos.x << "," << pos.y << ")"
               << " threat=(" << threatX << "," << threatY << ")"
               << " dist=" << dist << " safe=" << safeDistance << "\n";

    if (dist >= safeDistance)
    {
        return TaskStatus::Success;
    }

    // Compute flee direction and move.
    const float step = FLEE_MOVE_SPEED * ctx.DeltaTime;
    ::Vector newPos;
    if (dist > 1e-5f)
    {
        newPos = pos + delta * (step / dist);
    }
    else
    {
        newPos = pos + ::Vector(step, 0.0f, 0.0f);
    }

    try
    {
        ctx.LocalBB->SetValueScoped("local:Position", TaskValue(newPos));
    }
    catch (...)
    {
        try { ctx.LocalBB->SetValue("Position", TaskValue(newPos)); }
        catch (...) {}
    }

    return TaskStatus::Running;
}

REGISTER_ATOMIC_TASK(Task_Flee, "Task_Flee_AI")

} // namespace Olympe
