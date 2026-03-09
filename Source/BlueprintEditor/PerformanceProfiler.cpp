/**
 * @file PerformanceProfiler.cpp
 * @brief Implementation of the ATS VS performance profiler (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "PerformanceProfiler.h"

#include <fstream>
#include <algorithm>
#include <iostream>

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

PerformanceProfiler& PerformanceProfiler::Get()
{
    static PerformanceProfiler instance;
    return instance;
}

PerformanceProfiler::PerformanceProfiler()
{
}

PerformanceProfiler::~PerformanceProfiler()
{
}

// ============================================================================
// Profiling lifecycle
// ============================================================================

void PerformanceProfiler::BeginProfiling()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profiling   = true;
    m_frameNumber = 0;
    m_frameNodeMetrics.clear();
    m_nodeStartTimes.clear();
    m_frameHistory.clear();
    // Keep lifetime metrics — they accumulate across sessions
}

void PerformanceProfiler::StopProfiling()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profiling = false;
    m_nodeStartTimes.clear();
    m_frameNodeMetrics.clear();
}

bool PerformanceProfiler::IsProfiling() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_profiling;
}

// ============================================================================
// Frame lifecycle
// ============================================================================

void PerformanceProfiler::BeginFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_profiling)
        return;

    ++m_frameNumber;
    m_frameNodeMetrics.clear();
    m_nodeStartTimes.clear();
    m_frameStartTime = std::chrono::steady_clock::now();
}

void PerformanceProfiler::EndFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_profiling)
        return;

    auto frameEnd = std::chrono::steady_clock::now();
    float totalMs = std::chrono::duration<float, std::milli>(
                        frameEnd - m_frameStartTime).count();

    FrameProfile frame;
    frame.frameNumber      = m_frameNumber;
    frame.totalFrameTimeMs = totalMs;

    for (auto& kv : m_frameNodeMetrics)
    {
        frame.nodeMetrics.push_back(kv.second);
    }

    // Sort by execution time descending
    std::sort(frame.nodeMetrics.begin(), frame.nodeMetrics.end(),
              [](const NodeExecutionMetrics& a, const NodeExecutionMetrics& b) {
                  return a.executionTimeMs > b.executionTimeMs;
              });

    // Add to rolling history
    m_frameHistory.push_back(frame);
    if (static_cast<int>(m_frameHistory.size()) > MAX_FRAME_HISTORY)
    {
        m_frameHistory.erase(m_frameHistory.begin());
    }
}

// ============================================================================
// Node instrumentation
// ============================================================================

void PerformanceProfiler::BeginNodeExecution(int nodeID,
                                             const std::string& nodeName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_profiling)
        return;

    m_nodeStartTimes[nodeID] = std::chrono::steady_clock::now();

    // Ensure frame metric entry exists with node name
    if (m_frameNodeMetrics.find(nodeID) == m_frameNodeMetrics.end())
    {
        NodeExecutionMetrics m;
        m.nodeID   = nodeID;
        m.nodeName = nodeName;
        m_frameNodeMetrics[nodeID] = m;
    }
}

void PerformanceProfiler::EndNodeExecution(int nodeID)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_profiling)
        return;

    auto startIt = m_nodeStartTimes.find(nodeID);
    if (startIt == m_nodeStartTimes.end())
        return;

    auto now     = std::chrono::steady_clock::now();
    float elapsedMs = std::chrono::duration<float, std::milli>(
                          now - startIt->second).count();

    m_nodeStartTimes.erase(startIt);

    // Update per-frame metrics
    auto& fMetric = m_frameNodeMetrics[nodeID];
    fMetric.nodeID          = nodeID;
    fMetric.executionTimeMs = elapsedMs;
    fMetric.executionCount  += 1;
    fMetric.totalTimeMs     += elapsedMs;
    fMetric.avgTimeMs        = fMetric.totalTimeMs / static_cast<float>(fMetric.executionCount);
    if (elapsedMs > fMetric.maxTimeMs)
        fMetric.maxTimeMs = elapsedMs;

    // Update lifetime metrics
    auto& lMetric = m_lifetimeMetrics[nodeID];
    if (lMetric.nodeID == -1 || lMetric.nodeName.empty())
        lMetric.nodeName = fMetric.nodeName;
    lMetric.nodeID          = nodeID;
    lMetric.executionTimeMs = elapsedMs;
    lMetric.executionCount  += 1;
    lMetric.totalTimeMs     += elapsedMs;
    lMetric.avgTimeMs        = lMetric.totalTimeMs / static_cast<float>(lMetric.executionCount);
    if (elapsedMs > lMetric.maxTimeMs)
        lMetric.maxTimeMs = elapsedMs;
}

// ============================================================================
// Data access
// ============================================================================

FrameProfile PerformanceProfiler::GetCurrentFrame() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    FrameProfile frame;
    frame.frameNumber = m_frameNumber;
    frame.totalFrameTimeMs = 0.0f;
    for (auto& kv : m_frameNodeMetrics)
        frame.nodeMetrics.push_back(kv.second);
    return frame;
}

std::vector<FrameProfile> PerformanceProfiler::GetFrameHistory(int count) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (count <= 0 || count >= static_cast<int>(m_frameHistory.size()))
        return m_frameHistory;

    // Return last `count` frames
    auto start = m_frameHistory.end() - count;
    return std::vector<FrameProfile>(start, m_frameHistory.end());
}

std::vector<NodeExecutionMetrics> PerformanceProfiler::GetHotspots() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<NodeExecutionMetrics> result;
    for (auto& kv : m_lifetimeMetrics)
        result.push_back(kv.second);

    std::sort(result.begin(), result.end(),
              [](const NodeExecutionMetrics& a, const NodeExecutionMetrics& b) {
                  return a.avgTimeMs > b.avgTimeMs;
              });
    return result;
}

void PerformanceProfiler::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frameNodeMetrics.clear();
    m_nodeStartTimes.clear();
    m_lifetimeMetrics.clear();
    m_frameHistory.clear();
    m_frameNumber = 0;
}

bool PerformanceProfiler::SaveToFile(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        std::cerr << "[PerformanceProfiler] Failed to open file: " << path << std::endl;
        return false;
    }

    // CSV header
    ofs << "FrameNumber,TotalFrameTimeMs,NodeID,NodeName,ExecTimeMs,"
           "AvgTimeMs,MaxTimeMs,ExecCount\n";

    for (size_t f = 0; f < m_frameHistory.size(); ++f)
    {
        const FrameProfile& frame = m_frameHistory[f];
        for (size_t n = 0; n < frame.nodeMetrics.size(); ++n)
        {
            const NodeExecutionMetrics& m = frame.nodeMetrics[n];
            ofs << frame.frameNumber << ","
                << frame.totalFrameTimeMs << ","
                << m.nodeID << ","
                << m.nodeName << ","
                << m.executionTimeMs << ","
                << m.avgTimeMs << ","
                << m.maxTimeMs << ","
                << m.executionCount << "\n";
        }
    }

    ofs.close();
    return true;
}

} // namespace Olympe
