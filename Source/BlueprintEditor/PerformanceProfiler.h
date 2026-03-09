/**
 * @file PerformanceProfiler.h
 * @brief Performance profiler for ATS Visual Scripting node execution (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * PerformanceProfiler is a singleton that collects per-node execution
 * metrics across frames.  It is opt-in: profiling only starts when
 * BeginProfiling() is called, and stops with StopProfiling().
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <chrono>

namespace Olympe {

// ============================================================================
// Supporting structures
// ============================================================================

/**
 * @struct NodeExecutionMetrics
 * @brief Accumulated per-node execution statistics.
 */
struct NodeExecutionMetrics {
    int         nodeID         = -1;
    std::string nodeName;
    float       executionTimeMs = 0.0f;   ///< Last recorded execution time (ms)
    uint64_t    executionCount  = 0;      ///< Total number of executions
    float       avgTimeMs       = 0.0f;   ///< Running average execution time (ms)
    float       maxTimeMs       = 0.0f;   ///< Maximum observed execution time (ms)
    float       totalTimeMs     = 0.0f;   ///< Total accumulated time (ms)
};

/**
 * @struct FrameProfile
 * @brief Snapshot of all node metrics for a single frame.
 */
struct FrameProfile {
    uint64_t                    frameNumber    = 0;
    float                       totalFrameTimeMs = 0.0f;
    std::vector<NodeExecutionMetrics> nodeMetrics;
};

// ============================================================================
// PerformanceProfiler
// ============================================================================

/**
 * @class PerformanceProfiler
 * @brief Singleton performance profiler for VS graph node execution.
 *
 * @details
 * Usage pattern (from VSGraphExecutor):
 * @code
 *   PerformanceProfiler::Get().BeginNodeExecution(node->nodeID, node->nodeName);
 *   // ... execute node ...
 *   PerformanceProfiler::Get().EndNodeExecution(node->nodeID);
 * @endcode
 *
 * Usage pattern (from TaskSystem / game loop):
 * @code
 *   PerformanceProfiler::Get().BeginFrame();
 *   // ... process all task runners ...
 *   PerformanceProfiler::Get().EndFrame();
 * @endcode
 *
 * Meyers singleton pattern (local static in Get()).
 * Thread-safe via std::mutex.
 */
class PerformanceProfiler {
public:

    /**
     * @brief Returns the singleton instance (Meyers pattern).
     */
    static PerformanceProfiler& Get();

    // -----------------------------------------------------------------------
    // Profiling lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Enables profiling.  Must be called before BeginFrame().
     */
    void BeginProfiling();

    /**
     * @brief Disables profiling and optionally clears accumulated data.
     */
    void StopProfiling();

    /**
     * @brief Returns true if profiling is currently enabled.
     */
    bool IsProfiling() const;

    // -----------------------------------------------------------------------
    // Frame lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Marks the start of a new frame.
     * Clears the per-frame node timing accumulators.
     */
    void BeginFrame();

    /**
     * @brief Marks the end of the current frame.
     * Commits the frame data to history.
     */
    void EndFrame();

    // -----------------------------------------------------------------------
    // Node instrumentation
    // -----------------------------------------------------------------------

    /**
     * @brief Records the start time for a node execution.
     * @param nodeID   ID of the node.
     * @param nodeName Human-readable node name (used on first call).
     */
    void BeginNodeExecution(int nodeID, const std::string& nodeName);

    /**
     * @brief Records the end time for a node execution and updates metrics.
     * @param nodeID   ID of the node (must match the last BeginNodeExecution call).
     */
    void EndNodeExecution(int nodeID);

    // -----------------------------------------------------------------------
    // Data access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the current (in-progress) frame profile.
     */
    FrameProfile GetCurrentFrame() const;

    /**
     * @brief Returns up to @p count most recent completed frame profiles.
     * @param count  Maximum number of frames to return (0 = all).
     */
    std::vector<FrameProfile> GetFrameHistory(int count = 0) const;

    /**
     * @brief Returns accumulated lifetime metrics sorted by avgTimeMs descending.
     */
    std::vector<NodeExecutionMetrics> GetHotspots() const;

    /**
     * @brief Clears all accumulated data and history.
     */
    void Clear();

    /**
     * @brief Exports the frame history to a CSV file.
     * @param path  Destination file path (must be writable).
     * @return true on success, false on file open error.
     */
    bool SaveToFile(const std::string& path) const;

    // -----------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------

    /// Number of frames kept in the rolling history buffer.
    static const int MAX_FRAME_HISTORY = 120;

    /// Hotspot threshold (ms) — nodes above this are flagged.
    static constexpr float HOTSPOT_THRESHOLD_MS = 1.0f;

private:

    PerformanceProfiler();
    ~PerformanceProfiler();

    // Non-copyable, non-movable
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

    // -----------------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------------

    mutable std::mutex m_mutex;

    bool     m_profiling    = false;
    uint64_t m_frameNumber  = 0;

    /// Per-frame timing: node ID → start time
    std::unordered_map<int, std::chrono::steady_clock::time_point> m_nodeStartTimes;

    /// Accumulated per-frame node metrics (reset each BeginFrame)
    std::unordered_map<int, NodeExecutionMetrics> m_frameNodeMetrics;

    /// Lifetime metrics per node
    std::unordered_map<int, NodeExecutionMetrics> m_lifetimeMetrics;

    /// Rolling frame history buffer
    std::vector<FrameProfile> m_frameHistory;

    /// Frame start time
    std::chrono::steady_clock::time_point m_frameStartTime;
};

} // namespace Olympe
