/**
 * @file PathfindingManager.h
 * @brief Async pathfinding request manager for the Atomic Task System.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * PathfindingManager provides a simple async API for submitting pathfinding
 * requests and polling for completion.  Each request is processed on a
 * dedicated detached std::thread that writes a straight-line path string into
 * the result entry once the optional delay has elapsed.
 *
 * Usage:
 * @code
 *   auto id = PathfindingManager::Get().Request(start, target, 0.0f);
 *   // ... later ticks ...
 *   if (PathfindingManager::Get().IsComplete(id)) {
 *       std::string path = PathfindingManager::Get().GetPathString(id);
 *       PathfindingManager::Get().Cancel(id); // release entry
 *   }
 * @endcode
 *
 * Thread-safety: all public methods are guarded by m_mutex, except m_nextID
 * which is std::atomic.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../../vector.h"

namespace Olympe {

/**
 * @class PathfindingManager
 * @brief Singleton async pathfinding request manager.
 *
 * @details
 * Requests are identified by a uint64 RequestID.  Each request spawns a
 * detached thread that computes a straight-line path string and marks the
 * entry as completed.  Callers poll IsComplete() and retrieve the result via
 * GetPathString(), then call Cancel() to release the entry.
 */
class PathfindingManager {
public:

    /// Unique identifier for a pathfinding request.
    using RequestID = uint64_t;

    /// Sentinel value for an invalid / unsubmitted request.
    static constexpr RequestID INVALID_REQUEST_ID = 0u;

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the singleton PathfindingManager instance.
     */
    static PathfindingManager& Get();

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * @brief Submits an async pathfinding request.
     *
     * Launches a detached thread that sleeps for @p delaySeconds (simulating
     * pathfinding work) and then writes a straight-line path string into the
     * result entry unless the request has been cancelled.
     *
     * @param start        Start position.
     * @param target       Target position.
     * @param delaySeconds Simulated computation delay in seconds (0.0 = instant).
     * @return A unique RequestID identifying this request.
     */
    RequestID Request(const ::Vector& start,
                      const ::Vector& target,
                      float           delaySeconds = 0.0f);

    /**
     * @brief Returns true if the request identified by @p id has completed.
     *
     * Returns false for unknown or cancelled requests.
     *
     * @param id  RequestID returned by Request().
     */
    bool IsComplete(RequestID id);

    /**
     * @brief Returns the path string computed for @p id.
     *
     * Returns an empty string if the request is not complete or unknown.
     *
     * @param id  RequestID returned by Request().
     */
    std::string GetPathString(RequestID id);

    /**
     * @brief Cancels a pending request and removes its entry.
     *
     * If the request is still in-flight the worker thread will see the
     * cancelled flag and skip writing the result.  The entry is removed from
     * the map immediately so subsequent calls to IsComplete()/GetPathString()
     * return the default empty/false values.
     *
     * Safe to call on an already-completed request (releases the entry).
     *
     * @param id  RequestID returned by Request().
     */
    void Cancel(RequestID id);

private:

    // -----------------------------------------------------------------------
    // Internal types
    // -----------------------------------------------------------------------

    struct RequestEntry {
        ::Vector    start;
        ::Vector    target;
        std::string result;
        bool        completed = false;
        bool        cancelled = false;
    };

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    std::mutex                                    m_mutex;
    std::unordered_map<RequestID, RequestEntry>   m_requests;
    std::atomic<uint64_t>                         m_nextID;

    // -----------------------------------------------------------------------
    // Construction (private - singleton only)
    // -----------------------------------------------------------------------

    PathfindingManager();
    ~PathfindingManager() = default;

    // Non-copyable, non-movable
    PathfindingManager(const PathfindingManager&)            = delete;
    PathfindingManager& operator=(const PathfindingManager&) = delete;
};

} // namespace Olympe
