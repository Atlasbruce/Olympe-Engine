/**
 * @file EditorRuntimeBridgeTest.cpp
 * @brief Integration test for TaskExecutionBridge + TaskSystem callback mechanism.
 * @author Olympe Engine
 * @date 2026-02-25
 *
 * @details
 * Tests cover:
 *   a) TaskExecutionBridge::Install() registers hooks and marks the bridge as installed.
 *   b) TaskExecutionBridge::IsInstalled() returns true after Install().
 *   c) TaskExecutionBridge::Uninstall() clears hooks and marks bridge as uninstalled.
 *   d) Double Uninstall() is safe (no crash).
 *   e) Install() with null hooks is accepted.
 *   f) Re-install replaces hooks.
 *   g) SetEditorPublishCallback is invoked and the BB hook receives data when
 *      ExecuteAtomicTask causes a Running status.
 *
 * No SDL3, ImGui or Editor dependency — only TaskSystem headers are used.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/TaskExecutionBridge.h"
#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/LocalBlackboard.h"
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

#define TEST_ASSERT(condition, message)                              \
    do {                                                             \
        if (!(condition)) {                                          \
            std::cout << "  FAIL: " << (message) << std::endl;      \
            ++s_failCount;                                           \
        }                                                            \
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
// Stub hooks that record what was received
// ---------------------------------------------------------------------------

static int  g_receivedNodeIndex = -999;
static bool g_nodeFnCalled      = false;
static bool g_bbFnCalled        = false;

static void StubNodeHook(int nodeIndex)
{
    g_receivedNodeIndex = nodeIndex;
    g_nodeFnCalled      = true;
}

static void StubBBHook(const Olympe::LocalBlackboard* /*bb*/)
{
    g_bbFnCalled = true;
}

static void ResetStubs()
{
    g_receivedNodeIndex = -999;
    g_nodeFnCalled      = false;
    g_bbFnCalled        = false;
}

// ---------------------------------------------------------------------------
// Test A: Install sets IsInstalled to true
// ---------------------------------------------------------------------------

static void TestA_InstallState()
{
    std::cout << "Test A: Install/Uninstall state..." << std::endl;

    bool passed = true;

    // Ensure clean starting state
    Olympe::TaskExecutionBridge::Uninstall();

    TEST_ASSERT(!Olympe::TaskExecutionBridge::IsInstalled(),
                "Should not be installed before Install()");
    if (Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; }

    Olympe::TaskExecutionBridge::Install(&StubNodeHook, &StubBBHook);

    TEST_ASSERT(Olympe::TaskExecutionBridge::IsInstalled(),
                "Should be installed after Install()");
    if (!Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; }

    // Cleanup
    Olympe::TaskExecutionBridge::Uninstall();

    TEST_ASSERT(!Olympe::TaskExecutionBridge::IsInstalled(),
                "Should not be installed after Uninstall()");
    if (Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; }

    ReportTest("TestA_InstallState", passed);
}

// ---------------------------------------------------------------------------
// Test B: Double uninstall is safe
// ---------------------------------------------------------------------------

static void TestB_DoubleUninstall()
{
    std::cout << "Test B: Double Uninstall() is safe..." << std::endl;

    Olympe::TaskExecutionBridge::Uninstall();
    Olympe::TaskExecutionBridge::Uninstall(); // must not crash

    ReportTest("TestB_DoubleUninstall", true);
}

// ---------------------------------------------------------------------------
// Test C: Null hooks are accepted (no crash)
// ---------------------------------------------------------------------------

static void TestC_NullHooks()
{
    std::cout << "Test C: null hooks do not crash..." << std::endl;

    bool passed = true;

    // Install with null hooks: must not crash
    Olympe::TaskExecutionBridge::Install(nullptr, nullptr);

    TEST_ASSERT(Olympe::TaskExecutionBridge::IsInstalled(),
                "Bridge should be installed with null hooks");
    if (!Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; }

    Olympe::TaskExecutionBridge::Uninstall();

    ReportTest("TestC_NullHooks", passed);
}

// ---------------------------------------------------------------------------
// Test D: Re-install replaces hooks, bridge remains installed
// ---------------------------------------------------------------------------

static void TestD_Reinstall()
{
    std::cout << "Test D: Reinstall replaces hooks..." << std::endl;

    bool passed = true;

    Olympe::TaskExecutionBridge::Install(&StubNodeHook, &StubBBHook);

    TEST_ASSERT(Olympe::TaskExecutionBridge::IsInstalled(),
                "Should be installed after first Install");
    if (!Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; }

    // Install again with different hooks
    Olympe::TaskExecutionBridge::Install(nullptr, &StubBBHook);

    TEST_ASSERT(Olympe::TaskExecutionBridge::IsInstalled(),
                "Should still be installed after re-Install");
    if (!Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; }

    Olympe::TaskExecutionBridge::Uninstall();

    ReportTest("TestD_Reinstall", passed);
}

// ---------------------------------------------------------------------------
// Test E: Uninstall clears IsInstalled
// ---------------------------------------------------------------------------

static void TestE_UninstallClearsState()
{
    std::cout << "Test E: Uninstall clears state..." << std::endl;

    bool passed = true;

    ResetStubs();

    Olympe::TaskExecutionBridge::Install(&StubNodeHook, &StubBBHook);
    Olympe::TaskExecutionBridge::Uninstall();

    TEST_ASSERT(!Olympe::TaskExecutionBridge::IsInstalled(),
                "Bridge should not be installed after Uninstall");
    if (Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; }

    ReportTest("TestE_UninstallClearsState", passed);
}

// ---------------------------------------------------------------------------
// Test F: TaskSystem::SetEditorPublishCallback is non-null after Install
//         and null after Uninstall (verified via TaskSystem's getter not
//         available, so this is a smoke test of the public API chain).
// ---------------------------------------------------------------------------

static void TestF_InstallUninstallCycle()
{
    std::cout << "Test F: Install/Uninstall cycle..." << std::endl;

    bool passed = true;

    for (int i = 0; i < 5; ++i)
    {
        Olympe::TaskExecutionBridge::Install(&StubNodeHook, &StubBBHook);
        TEST_ASSERT(Olympe::TaskExecutionBridge::IsInstalled(),
                    "Should be installed in cycle");
        if (!Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; break; }

        Olympe::TaskExecutionBridge::Uninstall();
        TEST_ASSERT(!Olympe::TaskExecutionBridge::IsInstalled(),
                    "Should be uninstalled in cycle");
        if (Olympe::TaskExecutionBridge::IsInstalled()) { passed = false; break; }
    }

    ReportTest("TestF_InstallUninstallCycle", passed);
}

// ---------------------------------------------------------------------------
// Test G: SetEditorPublishCallback is invoked and hooks fire when an
//         AtomicTask causes a Running status (end-to-end bridge test).
// ---------------------------------------------------------------------------

namespace {

/// AtomicTask that always returns Running.
class Task_AlwaysRunning : public Olympe::IAtomicTask
{
public:
    Olympe::TaskStatus Execute(const ParameterMap& /*params*/) override
    {
        return Olympe::TaskStatus::Running;
    }
    void Abort() override {}
};

} // anonymous namespace

static const char* TASK_ALWAYS_RUNNING_ID = "Task_AlwaysRunning_BridgeTest_G";

static Olympe::TaskGraphTemplate MakeSingleNodeTemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "BridgeTestTemplate";
    tmpl.RootNodeID = 0;

    Olympe::TaskNodeDefinition node;
    node.NodeID        = 0;
    node.NodeName      = "AlwaysRunningNode";
    node.Type          = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID  = TASK_ALWAYS_RUNNING_ID;
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;

    tmpl.Nodes.push_back(node);
    tmpl.BuildLookupCache();
    return tmpl;
}

static void TestG_CallbackFiredOnRunningTask()
{
    std::cout << "Test G: SetEditorPublishCallback invoked and hooks fire on Running task..."
              << std::endl;

    ResetStubs();

    // Register the always-running task.
    Olympe::AtomicTaskRegistry::Get().Register(
        TASK_ALWAYS_RUNNING_ID,
        []() -> std::unique_ptr<Olympe::IAtomicTask> {
            return std::unique_ptr<Olympe::IAtomicTask>(new Task_AlwaysRunning());
        });

    // Install bridge with our stub hooks.
    Olympe::TaskExecutionBridge::Install(&StubNodeHook, &StubBBHook);

    // Execute one tick of the graph — task returns Running so the publish callback fires.
    Olympe::TaskGraphTemplate   tmpl   = MakeSingleNodeTemplate();
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    system.ExecuteNode(1u, runner, &tmpl, 0.016f);

    bool passed = true;

    TEST_ASSERT(g_nodeFnCalled,
                "Node hook should be called when task is Running");
    if (!g_nodeFnCalled) { passed = false; }

    TEST_ASSERT(g_bbFnCalled,
                "BB hook should be called with live blackboard when task is Running");
    if (!g_bbFnCalled) { passed = false; }

    TEST_ASSERT(g_receivedNodeIndex == 0,
                "Node hook should receive the current node index (0)");
    if (g_receivedNodeIndex != 0) { passed = false; }

    // Clean up.
    Olympe::TaskExecutionBridge::Uninstall();

    ReportTest("TestG_CallbackFiredOnRunningTask", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== EditorRuntimeBridgeTest ===" << std::endl;

    TestA_InstallState();
    TestB_DoubleUninstall();
    TestC_NullHooks();
    TestD_Reinstall();
    TestE_UninstallClearsState();
    TestF_InstallUninstallCycle();
    TestG_CallbackFiredOnRunningTask();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}

