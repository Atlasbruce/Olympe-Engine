/**
 * @file Task_GotoPosition.cpp
 * @brief Atomic task: move entity toward a 2D target position.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_GotoPosition.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../TaskWorldFacade.h"
#include "../../../system/system_utils.h"

#include <cmath>

namespace Olympe {

static const float DEFAULT_GOTO_SPEED     = 300.0f;
static const float ARRIVAL_TOLERANCE      = 5.0f;
static const char* BB_KEY_POSITION        = "Position";

Task_GotoPosition::Task_GotoPosition() {}

void Task_GotoPosition::Abort()
{
    SYSTEM_LOG << "[Task_GotoPosition] Abort()\n";
}

TaskStatus Task_GotoPosition::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_GotoPosition::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                  const ParameterMap& params)
{
    // --- Resolve TargetX parameter ---
    float targetX = 0.0f;
    {
        auto it = params.find("TargetX");
        if (it == params.end())
        {
            SYSTEM_LOG << "[Task_GotoPosition] Missing 'TargetX' parameter\n";
            return TaskStatus::Failure;
        }
        if (it->second.GetType() == VariableType::Float)
        {
            targetX = it->second.AsFloat();
        }
        else if (it->second.GetType() == VariableType::Int)
        {
            targetX = static_cast<float>(it->second.AsInt());
        }
    }

    // --- Resolve TargetY parameter ---
    float targetY = 0.0f;
    {
        auto it = params.find("TargetY");
        if (it == params.end())
        {
            SYSTEM_LOG << "[Task_GotoPosition] Missing 'TargetY' parameter\n";
            return TaskStatus::Failure;
        }
        if (it->second.GetType() == VariableType::Float)
        {
            targetY = it->second.AsFloat();
        }
        else if (it->second.GetType() == VariableType::Int)
        {
            targetY = static_cast<float>(it->second.AsInt());
        }
    }

    // --- Resolve Speed parameter (optional) ---
    float speed = DEFAULT_GOTO_SPEED;
    {
        auto it = params.find("Speed");
        if (it != params.end())
        {
            if (it->second.GetType() == VariableType::Float)
                speed = it->second.AsFloat();
            else if (it->second.GetType() == VariableType::Int)
                speed = static_cast<float>(it->second.AsInt());
            if (speed <= 0.0f) speed = DEFAULT_GOTO_SPEED;
        }
    }

    ::Vector target(targetX, targetY, 0.0f);

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
        delta.z        = 0.0f; // 2D movement
        const float dist = delta.Norm();

        SYSTEM_LOG << "[Task_GotoPosition] (WorldMode) Entity " << ctx.Entity
                   << " pos=(" << pos.x << "," << pos.y << ")"
                   << " target=(" << targetX << "," << targetY << ")"
                   << " dist=" << dist << "\n";

        if (dist <= ARRIVAL_TOLERANCE)
        {
            velocity = ::Vector(0.0f, 0.0f, 0.0f);
            return TaskStatus::Success;
        }

        ::Vector dir = delta * (1.0f / dist);
        velocity     = dir * speed;
        return TaskStatus::Running;
    }

    // -----------------------------------------------------------------------
    // Headless mode: read/write "Position" via LocalBlackboard
    // -----------------------------------------------------------------------
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_GotoPosition] No LocalBlackboard and no ComponentFacade\n";
        return TaskStatus::Success; // safe default in test mode
    }

    // Read current position from blackboard.
    ::Vector pos(0.0f, 0.0f, 0.0f);
    {
        const TaskValue posVal = ctx.LocalBB->GetValueScoped("local:Position");
        if (posVal.GetType() == VariableType::Vector)
        {
            pos = posVal.AsVector();
        }
        else if (ctx.LocalBB->HasVariable(BB_KEY_POSITION))
        {
            try { pos = ctx.LocalBB->GetValue(BB_KEY_POSITION).AsVector(); }
            catch (...) {}
        }
    }

    ::Vector delta = target - pos;
    delta.z        = 0.0f;
    const float dist = delta.Norm();

    SYSTEM_LOG << "[Task_GotoPosition] Entity " << ctx.Entity
               << " pos=(" << pos.x << "," << pos.y << ")"
               << " target=(" << targetX << "," << targetY << ")"
               << " dist=" << dist << "\n";

    if (dist <= ARRIVAL_TOLERANCE)
    {
        return TaskStatus::Success;
    }

    const float step   = speed * ctx.DeltaTime;
    ::Vector    newPos = (dist <= step)
                         ? target
                         : pos + delta * (step / dist);

    // Write updated position back.
    try
    {
        ctx.LocalBB->SetValueScoped("local:Position", TaskValue(newPos));
    }
    catch (...)
    {
        try { ctx.LocalBB->SetValue(BB_KEY_POSITION, TaskValue(newPos)); }
        catch (...) {}
    }

    return TaskStatus::Running;
}

REGISTER_ATOMIC_TASK(Task_GotoPosition, "Task_GotoPosition")

} // namespace Olympe
