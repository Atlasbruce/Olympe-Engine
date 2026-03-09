/**
 * @file DebuggerTest.cpp
 * @brief Unit tests for DebugController (Phase 5 ATS VS debugger).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   1.  DebugController_InitialState_IsNotDebugging
 *   2.  DebugController_StartDebugging_TransitionsToRunning
 *   3.  DebugController_StopDebugging_TransitionsToNotDebugging
 *   4.  DebugController_PauseAndContinue_StatesCorrect
 *   5.  DebugController_SetAndClearBreakpoint
 *   6.  DebugController_ToggleBreakpoint_AddsAndRemoves
 *   7.  DebugController_StepNext_PausesAfterOneNode
 *   8.  DebugController_CallStack_PushAndPop
 *   9.  DebugController_OnNodeExecuting_BreakpointPausesInHeadless
 *  10.  DebugController_ClearAllBreakpoints_RemovesAll
 *
 * No SDL3, ImGui, or Editor dependency — only DebugController headers are used.
 * Headless mode is enabled so OnNodeExecuting does not busy-wait.
 *
 * C++14 compliant — no C++17/20 features.
 */

#include "BlueprintEditor/DebugController.h"
#include "TaskSystem/LocalBlackboard.h"
#include "TaskSystem/TaskGraphTemplate.h"

#include <iostream>
#include <string>

using Olympe::DebugController;
using Olympe::DebugState;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                      \
    do {                                                            \
        if (!(cond)) {                                              \
            std::cout << "  FAIL: " << (msg) << std::endl;         \
            ++s_failCount;                                          \
        }                                                           \
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

// ---------------------------------------------------------------------------
// Fixture: reset DebugController to a clean state before each test
// ---------------------------------------------------------------------------

static void ResetController()
{
    DebugController& dc = DebugController::Get();
    dc.StopDebugging();
    dc.ClearAllBreakpoints();
    dc.SetHeadlessMode(true);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void Test_InitialState_IsNotDebugging()
{
    ResetController();
    DebugController& dc = DebugController::Get();

    bool passed = true;
    TEST_ASSERT(dc.GetState() == DebugState::NotDebugging,
                "Initial state should be NotDebugging");
    TEST_ASSERT(!dc.IsDebugging(),
                "IsDebugging() should be false initially");
    TEST_ASSERT(dc.GetCurrentGraphID() == -1,
                "GetCurrentGraphID() should be -1 initially");
    TEST_ASSERT(dc.GetCurrentNodeID() == -1,
                "GetCurrentNodeID() should be -1 initially");

    ReportTest("DebugController_InitialState_IsNotDebugging",
               s_failCount == 0);
    passed = (s_failCount == 0);
    (void)passed;
}

static void Test_StartDebugging_TransitionsToRunning()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.StartDebugging(5);
    TEST_ASSERT(dc.GetState() == DebugState::Running,
                "StartDebugging should transition to Running");
    TEST_ASSERT(dc.IsDebugging(),
                "IsDebugging() should be true after StartDebugging");
    TEST_ASSERT(dc.GetCurrentGraphID() == 5,
                "GetCurrentGraphID() should return 5 after StartDebugging(5)");

    ReportTest("DebugController_StartDebugging_TransitionsToRunning",
               s_failCount == prevFail);
}

static void Test_StopDebugging_TransitionsToNotDebugging()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.StartDebugging(1);
    dc.StopDebugging();
    TEST_ASSERT(dc.GetState() == DebugState::NotDebugging,
                "StopDebugging should transition to NotDebugging");
    TEST_ASSERT(!dc.IsDebugging(),
                "IsDebugging() should be false after StopDebugging");
    TEST_ASSERT(dc.GetCurrentGraphID() == -1,
                "GetCurrentGraphID() should be -1 after StopDebugging");

    ReportTest("DebugController_StopDebugging_TransitionsToNotDebugging",
               s_failCount == prevFail);
}

static void Test_PauseAndContinue_StatesCorrect()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.StartDebugging(0);
    TEST_ASSERT(dc.GetState() == DebugState::Running, "Should be Running after start");

    dc.Pause();
    TEST_ASSERT(dc.GetState() == DebugState::Paused, "Should be Paused after Pause()");

    dc.Continue();
    TEST_ASSERT(dc.GetState() == DebugState::Running, "Should be Running after Continue()");

    ReportTest("DebugController_PauseAndContinue_StatesCorrect",
               s_failCount == prevFail);
}

static void Test_SetAndClearBreakpoint()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    TEST_ASSERT(!dc.HasBreakpoint(0, 3), "No breakpoint initially at (0,3)");

    dc.SetBreakpoint(0, 3, "graph.json", "NodeA");
    TEST_ASSERT(dc.HasBreakpoint(0, 3), "Breakpoint should exist at (0,3)");
    TEST_ASSERT(!dc.HasBreakpoint(0, 4), "No breakpoint at (0,4)");

    dc.ClearBreakpoint(0, 3);
    TEST_ASSERT(!dc.HasBreakpoint(0, 3), "Breakpoint should be gone after ClearBreakpoint");

    ReportTest("DebugController_SetAndClearBreakpoint",
               s_failCount == prevFail);
}

static void Test_ToggleBreakpoint_AddsAndRemoves()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    TEST_ASSERT(!dc.HasBreakpoint(1, 7), "No breakpoint initially");

    dc.ToggleBreakpoint(1, 7, "g.json", "N7");
    TEST_ASSERT(dc.HasBreakpoint(1, 7), "Breakpoint added by ToggleBreakpoint");

    dc.ToggleBreakpoint(1, 7);
    TEST_ASSERT(!dc.HasBreakpoint(1, 7), "Breakpoint removed by second ToggleBreakpoint");

    ReportTest("DebugController_ToggleBreakpoint_AddsAndRemoves",
               s_failCount == prevFail);
}

static void Test_StepNext_PausesAfterOneNode()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.StartDebugging(0);
    dc.Pause();
    TEST_ASSERT(dc.GetState() == DebugState::Paused, "Should be Paused");

    dc.StepNext();
    TEST_ASSERT(dc.GetState() == DebugState::StepNext, "Should be StepNext after StepNext()");

    // Simulate OnNodeExecuting in headless mode — should transition to Paused
    dc.OnNodeExecuting(0, 2, nullptr);
    TEST_ASSERT(dc.GetState() == DebugState::Paused,
                "Should be Paused after OnNodeExecuting in StepNext mode");
    TEST_ASSERT(dc.GetCurrentNodeID() == 2,
                "CurrentNodeID should be 2 after OnNodeExecuting");

    ReportTest("DebugController_StepNext_PausesAfterOneNode",
               s_failCount == prevFail);
}

static void Test_CallStack_PushAndPop()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.StartDebugging(0);

    TEST_ASSERT(dc.GetCallStack().empty(), "Call stack should be empty initially");

    dc.PushCallFrame(0, 1, "root.json", "EntryPoint");
    TEST_ASSERT(dc.GetCallStack().size() == 1, "Call stack should have 1 frame");
    TEST_ASSERT(dc.GetCallStack().back().isCurrent, "Top frame should be current");

    dc.PushCallFrame(10, 3, "sub.json", "AtomicTask");
    TEST_ASSERT(dc.GetCallStack().size() == 2, "Call stack should have 2 frames");
    TEST_ASSERT(dc.GetCallStack().back().graphID == 10,
                "Top frame graphID should be 10");

    dc.PopCallFrame();
    TEST_ASSERT(dc.GetCallStack().size() == 1, "Call stack should have 1 frame after pop");
    TEST_ASSERT(dc.GetCallStack().back().isCurrent,
                "Remaining frame should be current after pop");

    dc.PopCallFrame();
    TEST_ASSERT(dc.GetCallStack().empty(), "Call stack should be empty after popping all");

    ReportTest("DebugController_CallStack_PushAndPop",
               s_failCount == prevFail);
}

static void Test_OnNodeExecuting_BreakpointPausesInHeadless()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.StartDebugging(0);
    dc.SetBreakpoint(0, 5, "graph.json", "Node5");

    // Should be Running before hitting the breakpoint
    TEST_ASSERT(dc.GetState() == DebugState::Running, "Should be Running");

    // Simulate execution reaching node 5 (breakpoint)
    dc.OnNodeExecuting(0, 5, nullptr);

    // In headless mode, should pause immediately without waiting
    TEST_ASSERT(dc.GetState() == DebugState::Paused,
                "Should be Paused after hitting breakpoint in headless mode");
    TEST_ASSERT(dc.GetCurrentNodeID() == 5,
                "CurrentNodeID should be 5");

    ReportTest("DebugController_OnNodeExecuting_BreakpointPausesInHeadless",
               s_failCount == prevFail);
}

static void Test_ClearAllBreakpoints_RemovesAll()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.SetBreakpoint(0, 1);
    dc.SetBreakpoint(0, 2);
    dc.SetBreakpoint(1, 3);
    TEST_ASSERT(dc.GetAllBreakpoints().size() == 3, "Should have 3 breakpoints");

    dc.ClearAllBreakpoints();
    TEST_ASSERT(dc.GetAllBreakpoints().empty(),
                "Should have no breakpoints after ClearAllBreakpoints");

    ReportTest("DebugController_ClearAllBreakpoints_RemovesAll",
               s_failCount == prevFail);
}

static void Test_BreakpointEnabled_Toggle()
{
    ResetController();
    DebugController& dc = DebugController::Get();
    int prevFail = s_failCount;

    dc.SetBreakpoint(0, 10, "g.json", "N10");
    TEST_ASSERT(dc.HasBreakpoint(0, 10), "Breakpoint enabled by default");

    dc.SetBreakpointEnabled(0, 10, false);
    TEST_ASSERT(!dc.HasBreakpoint(0, 10),
                "HasBreakpoint returns false for disabled breakpoint");

    dc.SetBreakpointEnabled(0, 10, true);
    TEST_ASSERT(dc.HasBreakpoint(0, 10),
                "HasBreakpoint returns true after re-enabling");

    ReportTest("DebugController_BreakpointEnabled_Toggle",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=============================" << std::endl;
    std::cout << " DebugController Unit Tests  " << std::endl;
    std::cout << "=============================" << std::endl;

    Test_InitialState_IsNotDebugging();
    Test_StartDebugging_TransitionsToRunning();
    Test_StopDebugging_TransitionsToNotDebugging();
    Test_PauseAndContinue_StatesCorrect();
    Test_SetAndClearBreakpoint();
    Test_ToggleBreakpoint_AddsAndRemoves();
    Test_StepNext_PausesAfterOneNode();
    Test_CallStack_PushAndPop();
    Test_OnNodeExecuting_BreakpointPausesInHeadless();
    Test_ClearAllBreakpoints_RemovesAll();
    Test_BreakpointEnabled_Toggle();

    std::cout << "=============================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
