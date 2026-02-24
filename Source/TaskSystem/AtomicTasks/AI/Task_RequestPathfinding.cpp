/**
 * @file Task_RequestPathfinding.cpp
 * @brief Atomic task: async path request via PathfindingManager.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * On the first ExecuteWithContext() call the task reads "Position" from the
 * LocalBlackboard and the "Target" parameter, then submits an async request
 * to PathfindingManager and returns Running.  On subsequent ticks it polls
 * IsComplete(); when the path is ready the string is written to the "Path"
 * LocalBlackboard key and the task returns Success.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_RequestPathfinding.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

static const char* BB_KEY_POSITION = "Position";
static const char* BB_KEY_PATH     = "Path";

// ---------------------------------------------------------------------------
// Construction / Abort
// ---------------------------------------------------------------------------

Task_RequestPathfinding::Task_RequestPathfinding()
    : m_requestID(PathfindingManager::INVALID_REQUEST_ID)
    , m_hasRequest(false)
{
}

void Task_RequestPathfinding::Abort()
{
    if (m_hasRequest && m_requestID != PathfindingManager::INVALID_REQUEST_ID)
    {
        SYSTEM_LOG << "[Task_RequestPathfinding] Abort() - cancelling request "
                   << m_requestID << "\n";
        PathfindingManager::Get().Cancel(m_requestID);
    }
    m_requestID  = PathfindingManager::INVALID_REQUEST_ID;
    m_hasRequest = false;
}

// ---------------------------------------------------------------------------
// Execute (legacy fallback)
// ---------------------------------------------------------------------------

TaskStatus Task_RequestPathfinding::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

// ---------------------------------------------------------------------------
// ExecuteWithContext
// ---------------------------------------------------------------------------

TaskStatus Task_RequestPathfinding::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                        const ParameterMap& params)
{
    if (!m_hasRequest)
    {
        // --- First tick: resolve parameters and submit async request ---

        // Read current position from LocalBlackboard.
        if (!ctx.LocalBB || !ctx.LocalBB->HasVariable(BB_KEY_POSITION))
        {
            SYSTEM_LOG << "[Task_RequestPathfinding] 'Position' key not found in LocalBlackboard\n";
            return TaskStatus::Failure;
        }

        ::Vector start(0.0f, 0.0f, 0.0f);
        try
        {
            start = ctx.LocalBB->GetValue(BB_KEY_POSITION).AsVector();
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[Task_RequestPathfinding] Failed to read 'Position': " << e.what() << "\n";
            return TaskStatus::Failure;
        }

        // Resolve Target parameter.
        ::Vector target(0.0f, 0.0f, 0.0f);
        {
            auto it = params.find("Target");
            if (it == params.end() || it->second.GetType() != VariableType::Vector)
            {
                SYSTEM_LOG << "[Task_RequestPathfinding] Missing or invalid 'Target' parameter\n";
                return TaskStatus::Failure;
            }
            target = it->second.AsVector();
        }

        // Resolve optional AsyncDelay parameter.
        float asyncDelay = 0.0f;
        {
            auto it = params.find("AsyncDelay");
            if (it != params.end() && it->second.GetType() == VariableType::Float)
            {
                asyncDelay = it->second.AsFloat();
                if (asyncDelay < 0.0f) asyncDelay = 0.0f;
            }
        }

        // Ensure "Path" key exists in LocalBlackboard before writing later.
        if (!ctx.LocalBB->HasVariable(BB_KEY_PATH))
        {
            SYSTEM_LOG << "[Task_RequestPathfinding] 'Path' key not found in LocalBlackboard\n";
            return TaskStatus::Failure;
        }

        // Submit async request.
        m_requestID  = PathfindingManager::Get().Request(start, target, asyncDelay);
        m_hasRequest = true;

        SYSTEM_LOG << "[Task_RequestPathfinding] Entity " << ctx.Entity
                   << " submitted request " << m_requestID
                   << " from (" << start.x << "," << start.y << ")"
                   << " to (" << target.x << "," << target.y << ")\n";

        return TaskStatus::Running;
    }

    // --- Subsequent ticks: poll for completion ---

    if (!PathfindingManager::Get().IsComplete(m_requestID))
    {
        SYSTEM_LOG << "[Task_RequestPathfinding] Entity " << ctx.Entity
                   << " waiting for request " << m_requestID << "\n";
        return TaskStatus::Running;
    }

    // Request completed - retrieve path string.
    std::string path = PathfindingManager::Get().GetPathString(m_requestID);
    PathfindingManager::Get().Cancel(m_requestID); // release entry

    m_requestID  = PathfindingManager::INVALID_REQUEST_ID;
    m_hasRequest = false;

    // Write path to LocalBlackboard.
    try
    {
        ctx.LocalBB->SetValue(BB_KEY_PATH, TaskValue(path));
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[Task_RequestPathfinding] Failed to write 'Path': " << e.what() << "\n";
        return TaskStatus::Failure;
    }

    SYSTEM_LOG << "[Task_RequestPathfinding] Entity " << ctx.Entity
               << " path ready: " << path << " - Success\n";

    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_RequestPathfinding, "Task_RequestPathfinding")

} // namespace Olympe
