/**
 * @file ProfilerTest.cpp
 * @brief Unit tests for PerformanceProfiler (Phase 5 ATS VS profiler).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   1.  Profiler_InitialState_NotProfiling
 *   2.  Profiler_BeginProfiling_EnablesProfiling
 *   3.  Profiler_StopProfiling_DisablesProfiling
 *   4.  Profiler_BeginEndFrame_IncreasesHistory
 *   5.  Profiler_NodeExecution_MetricsCollected
 *   6.  Profiler_MultipleFrames_HistoryRolls
 *   7.  Profiler_GetHotspots_SortedDescending
 *   8.  Profiler_Clear_ResetsAllData
 *   9.  Profiler_SaveToFile_WritesCSV
 *  10.  Profiler_NodeExecution_WhenNotProfiling_NoOp
 *
 * No SDL3, ImGui, or Editor dependency — only PerformanceProfiler is used.
 *
 * C++14 compliant — no C++17/20 features.
 */

#include "BlueprintEditor/PerformanceProfiler.h"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

using Olympe::PerformanceProfiler;
using Olympe::FrameProfile;
using Olympe::NodeExecutionMetrics;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                          \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cout << "  FAIL: " << (msg) << std::endl;             \
            ++s_failCount;                                              \
        }                                                               \
    } while (false)

static void ReportTest(const std::string& name, bool passed)
{
    if (passed)
    {
        std::cout << "[PASS] " << name << std::endl;
        ++s_passCount;
    }
    else
    {
        std::cout << "[FAIL] " << name << std::endl;
        ++s_failCount;
    }
}

static void ResetProfiler()
{
    PerformanceProfiler& p = PerformanceProfiler::Get();
    p.StopProfiling();
    p.Clear();
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void Test_InitialState_NotProfiling()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    TEST_ASSERT(!p.IsProfiling(), "Profiler should not be profiling initially");
    TEST_ASSERT(p.GetFrameHistory().empty(), "Frame history should be empty initially");
    TEST_ASSERT(p.GetHotspots().empty(), "Hotspots should be empty initially");

    ReportTest("Profiler_InitialState_NotProfiling", s_failCount == prevFail);
}

static void Test_BeginProfiling_EnablesProfiling()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();
    TEST_ASSERT(p.IsProfiling(), "IsProfiling() should be true after BeginProfiling()");

    p.StopProfiling();
    ReportTest("Profiler_BeginProfiling_EnablesProfiling", s_failCount == prevFail);
}

static void Test_StopProfiling_DisablesProfiling()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();
    p.StopProfiling();
    TEST_ASSERT(!p.IsProfiling(), "IsProfiling() should be false after StopProfiling()");

    ReportTest("Profiler_StopProfiling_DisablesProfiling", s_failCount == prevFail);
}

static void Test_BeginEndFrame_IncreasesHistory()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();

    TEST_ASSERT(p.GetFrameHistory().empty(), "History empty before any frame");

    p.BeginFrame();
    p.EndFrame();
    TEST_ASSERT(p.GetFrameHistory().size() == 1, "History should have 1 frame");

    p.BeginFrame();
    p.EndFrame();
    TEST_ASSERT(p.GetFrameHistory().size() == 2, "History should have 2 frames");

    p.StopProfiling();
    ReportTest("Profiler_BeginEndFrame_IncreasesHistory", s_failCount == prevFail);
}

static void Test_NodeExecution_MetricsCollected()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();
    p.BeginFrame();

    p.BeginNodeExecution(1, "NodeA");
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    p.EndNodeExecution(1);

    p.EndFrame();

    auto history = p.GetFrameHistory();
    TEST_ASSERT(!history.empty(), "Frame history should have at least 1 entry");

    bool foundNode = false;
    for (size_t i = 0; i < history.back().nodeMetrics.size(); ++i)
    {
        if (history.back().nodeMetrics[i].nodeID == 1)
        {
            foundNode = true;
            TEST_ASSERT(history.back().nodeMetrics[i].executionTimeMs >= 0.0f,
                        "Execution time should be >= 0");
            TEST_ASSERT(history.back().nodeMetrics[i].executionCount >= 1,
                        "Execution count should be >= 1");
            TEST_ASSERT(history.back().nodeMetrics[i].nodeName == "NodeA",
                        "Node name should be NodeA");
        }
    }
    TEST_ASSERT(foundNode, "Node 1 should appear in frame metrics");

    p.StopProfiling();
    ReportTest("Profiler_NodeExecution_MetricsCollected", s_failCount == prevFail);
}

static void Test_MultipleFrames_HistoryRolls()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();

    // Submit MAX_FRAME_HISTORY + 10 frames
    int target = PerformanceProfiler::MAX_FRAME_HISTORY + 10;
    for (int i = 0; i < target; ++i)
    {
        p.BeginFrame();
        p.EndFrame();
    }

    auto history = p.GetFrameHistory();
    TEST_ASSERT(static_cast<int>(history.size()) == PerformanceProfiler::MAX_FRAME_HISTORY,
                "History should be capped at MAX_FRAME_HISTORY");

    p.StopProfiling();
    ReportTest("Profiler_MultipleFrames_HistoryRolls", s_failCount == prevFail);
}

static void Test_GetHotspots_SortedDescending()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();
    p.BeginFrame();

    // Insert nodes with different execution times by controlling sleep
    p.BeginNodeExecution(10, "Fast");
    // No sleep — very short
    p.EndNodeExecution(10);

    p.BeginNodeExecution(11, "Slow");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    p.EndNodeExecution(11);

    p.EndFrame();

    auto hotspots = p.GetHotspots();
    TEST_ASSERT(!hotspots.empty(), "Hotspots should not be empty");

    // Verify sorted descending by avgTimeMs
    bool sortedOk = true;
    for (size_t i = 1; i < hotspots.size(); ++i)
    {
        if (hotspots[i].avgTimeMs > hotspots[i-1].avgTimeMs)
        {
            sortedOk = false;
            break;
        }
    }
    TEST_ASSERT(sortedOk, "Hotspots should be sorted descending by avgTimeMs");

    p.StopProfiling();
    ReportTest("Profiler_GetHotspots_SortedDescending", s_failCount == prevFail);
}

static void Test_Clear_ResetsAllData()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();
    p.BeginFrame();
    p.BeginNodeExecution(1, "N1");
    p.EndNodeExecution(1);
    p.EndFrame();

    p.Clear();

    TEST_ASSERT(p.GetFrameHistory().empty(), "History should be empty after Clear()");
    TEST_ASSERT(p.GetHotspots().empty(), "Hotspots should be empty after Clear()");

    p.StopProfiling();
    ReportTest("Profiler_Clear_ResetsAllData", s_failCount == prevFail);
}

static void Test_SaveToFile_WritesCSV()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    p.BeginProfiling();
    p.BeginFrame();
    p.BeginNodeExecution(5, "TestNode");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    p.EndNodeExecution(5);
    p.EndFrame();
    p.StopProfiling();

    const std::string csvPath = "/tmp/profiler_test_output.csv";
    bool saved = p.SaveToFile(csvPath);
    TEST_ASSERT(saved, "SaveToFile should return true");

    // Verify the file exists and contains CSV header
    std::ifstream ifs(csvPath);
    TEST_ASSERT(ifs.is_open(), "CSV file should exist after SaveToFile");
    if (ifs.is_open())
    {
        std::string firstLine;
        std::getline(ifs, firstLine);
        TEST_ASSERT(firstLine.find("FrameNumber") != std::string::npos,
                    "CSV should have FrameNumber column");
        TEST_ASSERT(firstLine.find("NodeName") != std::string::npos,
                    "CSV should have NodeName column");
    }

    ReportTest("Profiler_SaveToFile_WritesCSV", s_failCount == prevFail);
}

static void Test_NodeExecution_WhenNotProfiling_NoOp()
{
    ResetProfiler();
    PerformanceProfiler& p = PerformanceProfiler::Get();
    int prevFail = s_failCount;

    // Do NOT call BeginProfiling()
    p.BeginFrame();
    p.BeginNodeExecution(99, "Ignored");
    p.EndNodeExecution(99);
    p.EndFrame();

    TEST_ASSERT(p.GetFrameHistory().empty(),
                "Metrics should not be recorded when not profiling");
    TEST_ASSERT(p.GetHotspots().empty(),
                "Hotspots should be empty when not profiling");

    ReportTest("Profiler_NodeExecution_WhenNotProfiling_NoOp",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "===============================" << std::endl;
    std::cout << " PerformanceProfiler Unit Tests " << std::endl;
    std::cout << "===============================" << std::endl;

    Test_InitialState_NotProfiling();
    Test_BeginProfiling_EnablesProfiling();
    Test_StopProfiling_DisablesProfiling();
    Test_BeginEndFrame_IncreasesHistory();
    Test_NodeExecution_MetricsCollected();
    Test_MultipleFrames_HistoryRolls();
    Test_GetHotspots_SortedDescending();
    Test_Clear_ResetsAllData();
    Test_SaveToFile_WritesCSV();
    Test_NodeExecution_WhenNotProfiling_NoOp();

    std::cout << "===============================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
