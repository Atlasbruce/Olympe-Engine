/**
 * @file Task_RequestPathfinding.cpp
 * @brief Atomic task: compute a straight-line path and write to LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * Synchronous straight-line pathfinding for deterministic testing.
 * Stores the path destination (End) in the "Path" LocalBlackboard key.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_RequestPathfinding.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

static const char* BB_KEY_PATH = "Path";

Task_RequestPathfinding::Task_RequestPathfinding() {}

void Task_RequestPathfinding::Abort()
{
    SYSTEM_LOG << "[Task_RequestPathfinding] Abort()\n";
}

TaskStatus Task_RequestPathfinding::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_RequestPathfinding::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                        const ParameterMap& params)
{
    // --- Resolve Start parameter ---
    ::Vector start(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("Start");
        if (it != params.end() && it->second.GetType() == VariableType::Vector)
        {
            start = it->second.AsVector();
        }
        else
        {
            SYSTEM_LOG << "[Task_RequestPathfinding] Missing or invalid 'Start' parameter\n";
            return TaskStatus::Failure;
        }
    }

    // --- Resolve End parameter ---
    ::Vector end(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("End");
        if (it != params.end() && it->second.GetType() == VariableType::Vector)
        {
            end = it->second.AsVector();
        }
        else
        {
            SYSTEM_LOG << "[Task_RequestPathfinding] Missing or invalid 'End' parameter\n";
            return TaskStatus::Failure;
        }
    }

    // --- Write path destination to LocalBlackboard ---
    if (!ctx.LocalBB || !ctx.LocalBB->HasVariable(BB_KEY_PATH))
    {
        SYSTEM_LOG << "[Task_RequestPathfinding] 'Path' key not found in LocalBlackboard\n";
        return TaskStatus::Failure;
    }

    try
    {
        ctx.LocalBB->SetValue(BB_KEY_PATH, TaskValue(end));
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[Task_RequestPathfinding] Failed to write 'Path': " << e.what() << "\n";
        return TaskStatus::Failure;
    }

    SYSTEM_LOG << "[Task_RequestPathfinding] Entity " << ctx.Entity
               << " path from (" << start.x << "," << start.y << ")"
               << " to (" << end.x << "," << end.y << ") - Success\n";
    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_RequestPathfinding, "Task_RequestPathfinding")

} // namespace Olympe
