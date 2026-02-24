/**
 * @file PathfindingManager.cpp
 * @brief Async pathfinding request manager implementation.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * Workers are spawned as detached std::threads.  Each worker holds a raw
 * pointer to the PathfindingManager instance; because the manager is a
 * Meyer's singleton (static local) that lives until program exit, the
 * pointer remains valid for the entire application lifetime.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "PathfindingManager.h"

#include <sstream>
#include <thread>

#include "../../system/system_utils.h"

namespace Olympe {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

PathfindingManager& PathfindingManager::Get()
{
    static PathfindingManager s_instance;
    return s_instance;
}

PathfindingManager::PathfindingManager()
{
    m_nextID.store(1u);
}

// ---------------------------------------------------------------------------
// Request
// ---------------------------------------------------------------------------

PathfindingManager::RequestID
PathfindingManager::Request(const ::Vector& start,
                             const ::Vector& target,
                             float           delaySeconds)
{
    RequestID id = m_nextID.fetch_add(1u);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        RequestEntry entry;
        entry.start     = start;
        entry.target    = target;
        entry.completed = false;
        entry.cancelled = false;
        m_requests[id]  = entry;
    }

    SYSTEM_LOG << "[PathfindingManager] Submitted request " << id
               << " from (" << start.x << "," << start.y << ")"
               << " to (" << target.x << "," << target.y << ")"
               << " delay=" << delaySeconds << "s\n";

    // Spawn a detached worker thread.  The lambda captures the manager pointer
    // (safe: singleton lives until program exit) plus the request parameters
    // by value to avoid data races.
    PathfindingManager* mgr = this;
    std::thread worker([mgr, id, start, target, delaySeconds]()
    {
        // Simulate pathfinding computation delay.
        if (delaySeconds > 0.0f)
        {
            using namespace std::chrono;
            auto ms = static_cast<long long>(delaySeconds * 1000.0f);
            std::this_thread::sleep_for(milliseconds(ms));
        }

        std::lock_guard<std::mutex> lock(mgr->m_mutex);

        auto it = mgr->m_requests.find(id);
        if (it == mgr->m_requests.end() || it->second.cancelled)
        {
            SYSTEM_LOG << "[PathfindingManager] Request " << id
                       << " was cancelled - discarding result\n";
            return;
        }

        // Build straight-line path string: "(sx,sy,sz)->(tx,ty,tz)"
        std::ostringstream oss;
        oss << "(" << start.x << "," << start.y << "," << start.z << ")"
            << "->"
            << "(" << target.x << "," << target.y << "," << target.z << ")";

        it->second.result    = oss.str();
        it->second.completed = true;

        SYSTEM_LOG << "[PathfindingManager] Request " << id
                   << " completed: " << it->second.result << "\n";
    });
    worker.detach();

    return id;
}

// ---------------------------------------------------------------------------
// IsComplete
// ---------------------------------------------------------------------------

bool PathfindingManager::IsComplete(RequestID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(id);
    if (it == m_requests.end())
    {
        return false;
    }
    return it->second.completed;
}

// ---------------------------------------------------------------------------
// GetPathString
// ---------------------------------------------------------------------------

std::string PathfindingManager::GetPathString(RequestID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(id);
    if (it == m_requests.end() || !it->second.completed)
    {
        return std::string();
    }
    return it->second.result;
}

// ---------------------------------------------------------------------------
// Cancel
// ---------------------------------------------------------------------------

void PathfindingManager::Cancel(RequestID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(id);
    if (it == m_requests.end())
    {
        return;
    }
    it->second.cancelled = true;
    m_requests.erase(it);

    SYSTEM_LOG << "[PathfindingManager] Request " << id << " cancelled\n";
}

} // namespace Olympe
