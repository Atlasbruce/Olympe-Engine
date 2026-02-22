/**
 * @file TaskSystemRunningTest.cpp
 * @brief Unit tests for TaskSystem AtomicTask lifecycle (Phase 2.C).
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * Tests cover:
 *   a) A multi-frame task (Task_CountToN) persists across ticks while Running,
 *      and transitions to Success after N calls to Execute().
 *   b) Setting runner.CurrentNodeIndex to NODE_INDEX_NONE while a task is
 *      Running causes the next ExecuteNode() call to invoke Abort() and reset
 *      activeTask.
 *
 * No SDL3 or World dependency - tests call ExecuteNode() directly with a
 * caller-owned TaskRunnerComponent so the lifecycle can be verified without
 * a full ECS World.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/AtomicTaskRegistry.h"
#include "TaskSystem/IAtomicTask.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "ECS/Components/TaskRunnerComponent.h"

#include <iostream>
#include <string>
#include <memory>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(condition, message)                             \
    do {                                                            \
        if (!(condition)) {                                         \
            std::cout << "  FAIL: " << (message) << std::endl;     \
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
// Task_CountToN - test-only atomic task
//
// Counts Execute() calls and returns Running for the first (N-1) calls,
// then returns Success on the Nth call.  Abort() sets a global flag so
// tests can verify it was called.
// ---------------------------------------------------------------------------

/// Global counters reset before each test function.
static int  g_executeCount = 0;
static bool g_abortCalled  = false;

/// Number of ticks Task_CountToN executes before returning Success.
static const int TASK_N = 3;

namespace {

class Task_CountToN : public Olympe::IAtomicTask {
    int m_ticks; ///< Execute() calls accumulated for this instance.
public:
    Task_CountToN() : m_ticks(0) {}

    Olympe::TaskStatus Execute(const ParameterMap& /*params*/) override
    {
        ++m_ticks;
        ++g_executeCount;
        return (m_ticks >= TASK_N) ? Olympe::TaskStatus::Success
                                   : Olympe::TaskStatus::Running;
    }

    void Abort() override
    {
        g_abortCalled = true;
    }
};

} // anonymous namespace

/// ID used when registering Task_CountToN with the registry.
static const char* TASK_COUNT_TO_N_ID = "Task_CountToN_RunningTest";

// ---------------------------------------------------------------------------
// Helper: build a minimal in-memory TaskGraphTemplate with one AtomicTask node
//
//   Node 0 (AtomicTask, AtomicTaskID = TASK_COUNT_TO_N_ID)
//     NextOnSuccess = NODE_INDEX_NONE  (graph ends on success)
//     NextOnFailure = NODE_INDEX_NONE  (graph ends on failure)
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakeMinimalTemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "RunningTestTemplate";
    tmpl.RootNodeID = 0;

    Olympe::TaskNodeDefinition node;
    node.NodeID        = 0;
    node.NodeName      = "CountNode";
    node.Type          = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID  = TASK_COUNT_TO_N_ID;
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;

    tmpl.Nodes.push_back(node);
    tmpl.BuildLookupCache();

    return tmpl;
}

// ---------------------------------------------------------------------------
// Helper: register Task_CountToN factory (safe to call multiple times -
// Registry::Register replaces any existing entry with the same ID).
// ---------------------------------------------------------------------------

static void RegisterCountToN()
{
    Olympe::AtomicTaskRegistry::Get().Register(
        TASK_COUNT_TO_N_ID,
        []() -> std::unique_ptr<Olympe::IAtomicTask> {
            return std::unique_ptr<Olympe::IAtomicTask>(new Task_CountToN());
        });
}

// ---------------------------------------------------------------------------
// Test A: Task_CountToN persists across ticks and succeeds on the Nth tick
// ---------------------------------------------------------------------------

static void TestA_RunningTaskPersistsAndSucceeds()
{
    std::cout << "Test A: Running task persists across ticks and succeeds on tick N..."
              << std::endl;

    // Reset global counters.
    g_executeCount = 0;
    g_abortCalled  = false;

    RegisterCountToN();

    Olympe::TaskGraphTemplate tmpl = MakeMinimalTemplate();
    Olympe::TaskSystem        system;
    Olympe::TaskRunnerComponent runner;

    // runner.CurrentNodeIndex defaults to 0, which matches node.NodeID = 0.

    bool passed = true;
    const float dt = 0.016f;

    // --- Ticks 1 .. (N-1): task should be Running, activeTask non-null ---
    for (int tick = 1; tick < TASK_N; ++tick)
    {
        system.ExecuteNode(1u, runner, &tmpl, dt);

        TEST_ASSERT(runner.activeTask != nullptr,
                    "activeTask should be non-null while Running");
        if (!runner.activeTask) { passed = false; }

        TEST_ASSERT(runner.CurrentNodeIndex == 0,
                    "CurrentNodeIndex should stay at 0 while Running");
        if (runner.CurrentNodeIndex != 0) { passed = false; }
    }

    // --- Tick N: task should return Success ---
    system.ExecuteNode(1u, runner, &tmpl, dt);

    TEST_ASSERT(runner.activeTask == nullptr,
                "activeTask should be null after task completes");
    if (runner.activeTask) { passed = false; }

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success,
                "LastStatus should be Success after task completes");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Success)
    { passed = false; }

    // NextOnSuccess was NODE_INDEX_NONE, so CurrentNodeIndex should be NODE_INDEX_NONE.
    TEST_ASSERT(runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE,
                "CurrentNodeIndex should be NODE_INDEX_NONE after final transition");
    if (runner.CurrentNodeIndex != Olympe::NODE_INDEX_NONE) { passed = false; }

    TEST_ASSERT(g_executeCount == TASK_N,
                "Execute() should have been called exactly N times");
    if (g_executeCount != TASK_N) { passed = false; }

    TEST_ASSERT(!g_abortCalled,
                "Abort() should NOT have been called on a task that completed normally");
    if (g_abortCalled) { passed = false; }

    ReportTest("TestA_RunningTaskPersistsAndSucceeds", passed);
}

// ---------------------------------------------------------------------------
// Test B: Abort() is called when CurrentNodeIndex is set to NODE_INDEX_NONE
//         while a task is Running
// ---------------------------------------------------------------------------

static void TestB_AbortCalledOnExternalInterrupt()
{
    std::cout << "Test B: Abort() is called when task is interrupted externally..."
              << std::endl;

    // Reset global counters.
    g_executeCount = 0;
    g_abortCalled  = false;

    RegisterCountToN();

    Olympe::TaskGraphTemplate   tmpl = MakeMinimalTemplate();
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    bool passed = true;
    const float dt = 0.016f;

    // --- Run for (N-1) ticks so the task is Running but not yet complete ---
    for (int tick = 0; tick < TASK_N - 1; ++tick)
    {
        system.ExecuteNode(2u, runner, &tmpl, dt);
    }

    TEST_ASSERT(runner.activeTask != nullptr,
                "activeTask should be non-null before interrupt");
    if (!runner.activeTask) { passed = false; }

    // --- External interrupt: set CurrentNodeIndex to NODE_INDEX_NONE ---
    runner.CurrentNodeIndex = Olympe::NODE_INDEX_NONE;

    // --- Next tick: ExecuteNode should detect NODE_INDEX_NONE and call Abort() ---
    system.ExecuteNode(2u, runner, &tmpl, dt);

    TEST_ASSERT(g_abortCalled,
                "Abort() should have been called after external interrupt");
    if (!g_abortCalled) { passed = false; }

    TEST_ASSERT(runner.activeTask == nullptr,
                "activeTask should be null after Abort()");
    if (runner.activeTask) { passed = false; }

    ReportTest("TestB_AbortCalledOnExternalInterrupt", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== TaskSystemRunningTest ===" << std::endl;

    TestA_RunningTaskPersistsAndSucceeds();
    TestB_AbortCalledOnExternalInterrupt();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
