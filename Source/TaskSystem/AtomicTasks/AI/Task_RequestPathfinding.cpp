/**
 * @file Task_RequestPathfinding.cpp
 * @brief Implementation of Task_RequestPathfinding atomic task.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Builds a trivial two-waypoint path (start -> goal) and stores it as a
 * comma/colon-separated String in ctx.LocalBB["Path"].  Returns Success.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_RequestPathfinding.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

#include <sstream>
#include <iomanip>

namespace Olympe {

// ============================================================================
// Execute (legacy path)
// ============================================================================

TaskStatus Task_RequestPathfinding::Execute(const ParameterMap& params)
{
    AtomicTaskContext dummy;
    return ExecuteWithContext(dummy, params);
}

// ============================================================================
// ExecuteWithContext
// ============================================================================

TaskStatus Task_RequestPathfinding::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                        const ParameterMap& params)
{
    // ------------------------------------------------------------------
    // Read Start and Goal parameters
    // ------------------------------------------------------------------

    Vector start(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("Start");
        if (it != params.end() && it->second.GetType() == VariableType::Vector)
            start = it->second.AsVector();
    }

    Vector goal(0.0f, 0.0f, 0.0f);
    {
        auto it = params.find("Goal");
        if (it != params.end() && it->second.GetType() == VariableType::Vector)
            goal = it->second.AsVector();
    }

    // ------------------------------------------------------------------
    // Build path string: "sx,sy:gx,gy"
    // ------------------------------------------------------------------

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2)
        << start.x << "," << start.y
        << ":" << goal.x << "," << goal.y;

    const std::string pathStr = oss.str();

    // ------------------------------------------------------------------
    // Write to LocalBlackboard["Path"] if declared
    // ------------------------------------------------------------------

    if (ctx.LocalBB && ctx.LocalBB->HasVariable("Path"))
    {
        try
        {
            ctx.LocalBB->SetValue("Path", TaskValue(pathStr));
        }
        catch (...)
        {
            SYSTEM_LOG << "[Task_RequestPathfinding] Entity " << ctx.Entity
                       << ": could not write 'Path' to LocalBB (type mismatch?)\n";
        }
    }

    SYSTEM_LOG << "[Task_RequestPathfinding] Entity " << ctx.Entity
               << " path='" << pathStr << "' - Success\n";

    return TaskStatus::Success;
}

// ============================================================================
// Abort
// ============================================================================

void Task_RequestPathfinding::Abort()
{
    SYSTEM_LOG << "[Task_RequestPathfinding] Abort() called\n";
}

REGISTER_ATOMIC_TASK(Task_RequestPathfinding, "Task_RequestPathfinding")

} // namespace Olympe
