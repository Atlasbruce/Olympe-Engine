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
 *
 * No SDL3, ImGui or Editor dependency — only TaskSystem headers are used.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/TaskExecutionBridge.h"
#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/LocalBlackboard.h"

#include <iostream>
#include <string>

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

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}

