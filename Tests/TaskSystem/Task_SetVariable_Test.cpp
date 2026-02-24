/**
 * @file Task_SetVariable_Test.cpp
 * @brief Unit tests for Task_SetVariable error paths and success path.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * Tests cover:
 *   a) Returns Failure when 'VarName' parameter is missing.
 *   b) Returns Failure when 'VarName' parameter has wrong type (non-String).
 *   c) Returns Failure when 'Value' parameter is missing.
 *   d) Returns Failure when ctx.LocalBB is null.
 *   e) Returns Success and writes the value to LocalBlackboard.
 *   f) Short ID "SetVariable" is accepted by the registry (ID migration).
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/AtomicTasks/Blackboard/Task_SetVariable.h"
#include "TaskSystem/AtomicTaskContext.h"
#include "TaskSystem/AtomicTaskRegistry.h"
#include "TaskSystem/LocalBlackboard.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <memory>

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
// Helper: build a minimal TaskGraphTemplate with a "Done" bool variable
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakeTemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "SetVarTest";
    tmpl.RootNodeID = 0;

    Olympe::VariableDefinition v;
    v.Name         = "Done";
    v.Type         = Olympe::VariableType::Bool;
    v.DefaultValue = Olympe::TaskValue(false);
    v.IsLocal      = true;
    tmpl.LocalVariables.push_back(v);

    tmpl.BuildLookupCache();
    return tmpl;
}

// ---------------------------------------------------------------------------
// Test a: Failure when 'VarName' is absent
// ---------------------------------------------------------------------------

static void TestA_MissingVarName()
{
    std::cout << "SetVariable: Failure when VarName is missing..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard   bb;
    bb.Initialize(tmpl);

    Olympe::AtomicTaskContext ctx;
    ctx.Entity   = 1u;
    ctx.LocalBB  = &bb;
    ctx.DeltaTime = 0.016f;

    Olympe::IAtomicTask::ParameterMap params;
    // VarName intentionally omitted
    params["Value"] = Olympe::TaskValue(true);

    Olympe::Task_SetVariable task;
    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Failure,
                "Should Fail when VarName is absent");
    if (status != Olympe::TaskStatus::Failure) passed = false;

    ReportTest("TestA_MissingVarName", passed);
}

// ---------------------------------------------------------------------------
// Test b: Failure when 'VarName' has wrong type (non-String)
// ---------------------------------------------------------------------------

static void TestB_InvalidVarNameType()
{
    std::cout << "SetVariable: Failure when VarName has wrong type..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard   bb;
    bb.Initialize(tmpl);

    Olympe::AtomicTaskContext ctx;
    ctx.Entity   = 1u;
    ctx.LocalBB  = &bb;
    ctx.DeltaTime = 0.016f;

    Olympe::IAtomicTask::ParameterMap params;
    params["VarName"] = Olympe::TaskValue(42);  // Int instead of String
    params["Value"]   = Olympe::TaskValue(true);

    Olympe::Task_SetVariable task;
    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Failure,
                "Should Fail when VarName is not a String");
    if (status != Olympe::TaskStatus::Failure) passed = false;

    ReportTest("TestB_InvalidVarNameType", passed);
}

// ---------------------------------------------------------------------------
// Test c: Failure when 'Value' parameter is missing
// ---------------------------------------------------------------------------

static void TestC_MissingValue()
{
    std::cout << "SetVariable: Failure when Value is missing..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard   bb;
    bb.Initialize(tmpl);

    Olympe::AtomicTaskContext ctx;
    ctx.Entity   = 1u;
    ctx.LocalBB  = &bb;
    ctx.DeltaTime = 0.016f;

    Olympe::IAtomicTask::ParameterMap params;
    params["VarName"] = Olympe::TaskValue(std::string("Done"));
    // Value intentionally omitted

    Olympe::Task_SetVariable task;
    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Failure,
                "Should Fail when Value is absent");
    if (status != Olympe::TaskStatus::Failure) passed = false;

    ReportTest("TestC_MissingValue", passed);
}

// ---------------------------------------------------------------------------
// Test d: Failure when ctx.LocalBB is null
// ---------------------------------------------------------------------------

static void TestD_NullLocalBB()
{
    std::cout << "SetVariable: Failure when LocalBB is null..." << std::endl;

    bool passed = true;

    Olympe::AtomicTaskContext ctx;
    ctx.Entity   = 1u;
    ctx.LocalBB  = nullptr; // no blackboard
    ctx.DeltaTime = 0.016f;

    Olympe::IAtomicTask::ParameterMap params;
    params["VarName"] = Olympe::TaskValue(std::string("Done"));
    params["Value"]   = Olympe::TaskValue(true);

    Olympe::Task_SetVariable task;
    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Failure,
                "Should Fail when LocalBB is null");
    if (status != Olympe::TaskStatus::Failure) passed = false;

    ReportTest("TestD_NullLocalBB", passed);
}

// ---------------------------------------------------------------------------
// Test e: Success - value written to LocalBlackboard
// ---------------------------------------------------------------------------

static void TestE_SuccessWritesToBB()
{
    std::cout << "SetVariable: Success writes value to LocalBlackboard..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard   bb;
    bb.Initialize(tmpl);

    Olympe::AtomicTaskContext ctx;
    ctx.Entity   = 1u;
    ctx.LocalBB  = &bb;
    ctx.DeltaTime = 0.016f;

    Olympe::IAtomicTask::ParameterMap params;
    params["VarName"] = Olympe::TaskValue(std::string("Done"));
    params["Value"]   = Olympe::TaskValue(true);

    Olympe::Task_SetVariable task;
    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Success,
                "Should succeed when parameters and LocalBB are valid");
    if (status != Olympe::TaskStatus::Success) passed = false;

    // Verify the value was written
    TEST_ASSERT(bb.HasVariable("Done"),
                "LocalBB should still have the 'Done' variable");
    try
    {
        bool val = bb.GetValue("Done").AsBool();
        TEST_ASSERT(val == true,
                    "LocalBB 'Done' should now be true");
        if (!val) passed = false;
    }
    catch (...)
    {
        std::cout << "  FAIL: Exception reading 'Done' from LocalBB" << std::endl;
        passed = false;
        ++s_failCount;
    }

    ReportTest("TestE_SuccessWritesToBB", passed);
}

// ---------------------------------------------------------------------------
// Test f: Short ID "SetVariable" accepted by the registry
// ---------------------------------------------------------------------------

static void TestF_ShortIdAccepted()
{
    std::cout << "SetVariable: Short ID 'SetVariable' accepted by registry..." << std::endl;

    bool passed = true;

    // The registry should resolve "SetVariable" -> "Task_SetVariable"
    auto task = Olympe::AtomicTaskRegistry::Get().Create("SetVariable");

    TEST_ASSERT(task != nullptr,
                "Registry should return a task for short ID 'SetVariable'");
    if (task == nullptr) passed = false;

    ReportTest("TestF_ShortIdAccepted", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Task_SetVariable_Test ===" << std::endl;

    TestA_MissingVarName();
    TestB_InvalidVarNameType();
    TestC_MissingValue();
    TestD_NullLocalBB();
    TestE_SuccessWritesToBB();
    TestF_ShortIdAccepted();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
