/**
 * @file GlobalBlackboard_Tests.cpp
 * @brief Unit tests for GlobalBlackboard singleton.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   a) Set and Get a variable.
 *   b) Dirty flag is set on write and cleared with ClearDirty().
 *   c) Clear() removes all variables.
 *   d) LocalBlackboard::GetValueScoped / SetValueScoped with "global:" prefix.
 *   e) Scoped access with "local:" prefix stays local.
 *   f) Scoped access with no prefix stays local.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "../../Source/NodeGraphCore/GlobalBlackboard.h"
#include "../../Source/TaskSystem/LocalBlackboard.h"
#include "../../Source/TaskSystem/TaskGraphTypes.h"
#include "../../Source/TaskSystem/TaskGraphTemplate.h"

#include <iostream>
#include <cassert>
#include <string>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(condition, message)                                     \
    do {                                                                    \
        if (!(condition)) {                                                 \
            std::cout << "  FAIL: " << (message) << std::endl;             \
            ++s_failCount;                                                  \
        }                                                                   \
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
// Test A: Set/Get a variable
// ---------------------------------------------------------------------------

static void TestA_SetAndGet()
{
    std::cout << "Test A: GlobalBlackboard Set/Get..." << std::endl;

    GlobalBlackboard::Get().Clear();

    GlobalBlackboard::Get().SetVar("TestInt",    TaskValue(42));
    GlobalBlackboard::Get().SetVar("TestFloat",  TaskValue(3.14f));
    GlobalBlackboard::Get().SetVar("TestBool",   TaskValue(true));
    GlobalBlackboard::Get().SetVar("TestString", TaskValue(std::string("hello")));

    bool passed = true;

    TaskValue vi = GlobalBlackboard::Get().GetVar("TestInt");
    TEST_ASSERT(vi.GetType()   == VariableType::Int, "TestInt should be Int type");
    TEST_ASSERT(vi.AsInt()     == 42,                "TestInt should equal 42");
    if (vi.GetType() != VariableType::Int || vi.AsInt() != 42) passed = false;

    TaskValue vf = GlobalBlackboard::Get().GetVar("TestFloat");
    TEST_ASSERT(vf.GetType()   == VariableType::Float, "TestFloat should be Float type");
    if (vf.GetType() != VariableType::Float) passed = false;

    TaskValue vb = GlobalBlackboard::Get().GetVar("TestBool");
    TEST_ASSERT(vb.GetType()   == VariableType::Bool, "TestBool should be Bool type");
    TEST_ASSERT(vb.AsBool()    == true,               "TestBool should be true");
    if (vb.GetType() != VariableType::Bool || !vb.AsBool()) passed = false;

    TaskValue vs = GlobalBlackboard::Get().GetVar("TestString");
    TEST_ASSERT(vs.GetType()   == VariableType::String, "TestString should be String type");
    TEST_ASSERT(vs.AsString()  == "hello",              "TestString should equal 'hello'");
    if (vs.GetType() != VariableType::String || vs.AsString() != "hello") passed = false;

    ReportTest("TestA_SetAndGet", passed);
}

// ---------------------------------------------------------------------------
// Test B: Dirty flag
// ---------------------------------------------------------------------------

static void TestB_DirtyFlag()
{
    std::cout << "Test B: Dirty flag..." << std::endl;

    GlobalBlackboard::Get().Clear();
    GlobalBlackboard::Get().ClearDirty();

    bool passed = true;

    TEST_ASSERT(!GlobalBlackboard::Get().IsDirty(), "Should not be dirty before any write");
    if (GlobalBlackboard::Get().IsDirty()) passed = false;

    GlobalBlackboard::Get().SetVar("X", TaskValue(1));
    TEST_ASSERT(GlobalBlackboard::Get().IsDirty(), "Should be dirty after SetVar");
    if (!GlobalBlackboard::Get().IsDirty()) passed = false;

    GlobalBlackboard::Get().ClearDirty();
    TEST_ASSERT(!GlobalBlackboard::Get().IsDirty(), "Should not be dirty after ClearDirty");
    if (GlobalBlackboard::Get().IsDirty()) passed = false;

    ReportTest("TestB_DirtyFlag", passed);
}

// ---------------------------------------------------------------------------
// Test C: Clear removes all variables
// ---------------------------------------------------------------------------

static void TestC_Clear()
{
    std::cout << "Test C: Clear removes all variables..." << std::endl;

    GlobalBlackboard::Get().SetVar("Key1", TaskValue(1));
    GlobalBlackboard::Get().SetVar("Key2", TaskValue(2));
    GlobalBlackboard::Get().Clear();

    bool passed = true;

    TaskValue v = GlobalBlackboard::Get().GetVar("Key1");
    TEST_ASSERT(v.IsNone(), "After Clear(), Key1 should return None");
    if (!v.IsNone()) passed = false;

    v = GlobalBlackboard::Get().GetVar("Key2");
    TEST_ASSERT(v.IsNone(), "After Clear(), Key2 should return None");
    if (!v.IsNone()) passed = false;

    ReportTest("TestC_Clear", passed);
}

// ---------------------------------------------------------------------------
// Test D: LocalBlackboard::GetValueScoped with "global:" prefix
// ---------------------------------------------------------------------------

static void TestD_GlobalPrefixReadWrite()
{
    std::cout << "Test D: GlobalBlackboard scoped access via LocalBlackboard..." << std::endl;

    GlobalBlackboard::Get().Clear();
    GlobalBlackboard::Get().SetVar("SharedScore", TaskValue(100));

    // Build a minimal template with one BB entry "LocalScore"
    TaskGraphTemplate tmpl;
    BlackboardEntry entry;
    entry.Key     = "LocalScore";
    entry.Type    = VariableType::Int;
    entry.Default = TaskValue(0);
    tmpl.Blackboard.push_back(entry);

    LocalBlackboard lb;
    lb.InitializeFromEntries(tmpl.Blackboard);
    lb.SetValue("LocalScore", TaskValue(5));

    bool passed = true;

    // GetValueScoped("global:SharedScore") should return 100
    TaskValue gv = lb.GetValueScoped("global:SharedScore");
    TEST_ASSERT(gv.GetType() == VariableType::Int, "global:SharedScore should be Int");
    TEST_ASSERT(gv.AsInt()   == 100,               "global:SharedScore should equal 100");
    if (gv.GetType() != VariableType::Int || gv.AsInt() != 100) passed = false;

    // SetValueScoped("global:SharedScore", 200) should update GlobalBlackboard
    lb.SetValueScoped("global:SharedScore", TaskValue(200));
    TaskValue gv2 = GlobalBlackboard::Get().GetVar("SharedScore");
    TEST_ASSERT(gv2.AsInt() == 200, "GlobalBlackboard should reflect SetValueScoped");
    if (gv2.AsInt() != 200) passed = false;

    // GetValueScoped("local:LocalScore") should return 5 (local)
    TaskValue lv = lb.GetValueScoped("local:LocalScore");
    TEST_ASSERT(lv.AsInt() == 5, "local:LocalScore should equal 5");
    if (lv.AsInt() != 5) passed = false;

    // GetValueScoped("LocalScore") (no prefix) should also return 5
    TaskValue uv = lb.GetValueScoped("LocalScore");
    TEST_ASSERT(uv.AsInt() == 5, "LocalScore (no prefix) should equal 5");
    if (uv.AsInt() != 5) passed = false;

    ReportTest("TestD_GlobalPrefixReadWrite", passed);
}

// ---------------------------------------------------------------------------
// Test E: SetValueScoped with "local:" prefix stays local
// ---------------------------------------------------------------------------

static void TestE_LocalPrefixStaysLocal()
{
    std::cout << "Test E: local: prefix does not leak to GlobalBlackboard..." << std::endl;

    GlobalBlackboard::Get().Clear();

    TaskGraphTemplate tmpl;
    BlackboardEntry entry;
    entry.Key     = "Counter";
    entry.Type    = VariableType::Int;
    entry.Default = TaskValue(0);
    tmpl.Blackboard.push_back(entry);

    LocalBlackboard lb;
    lb.InitializeFromEntries(tmpl.Blackboard);

    lb.SetValueScoped("local:Counter", TaskValue(99));

    bool passed = true;

    // Local value should be updated
    TEST_ASSERT(lb.GetValue("Counter").AsInt() == 99, "local Counter should be 99");
    if (lb.GetValue("Counter").AsInt() != 99) passed = false;

    // GlobalBlackboard should NOT have "Counter"
    TaskValue gv = GlobalBlackboard::Get().GetVar("Counter");
    TEST_ASSERT(gv.IsNone(), "GlobalBlackboard should not have Counter set by local: prefix");
    if (!gv.IsNone()) passed = false;

    ReportTest("TestE_LocalPrefixStaysLocal", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== GlobalBlackboard_Tests ===" << std::endl;

    TestA_SetAndGet();
    TestB_DirtyFlag();
    TestC_Clear();
    TestD_GlobalPrefixReadWrite();
    TestE_LocalPrefixStaysLocal();

    std::cout << std::endl
              << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
