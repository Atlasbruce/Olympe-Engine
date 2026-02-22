/**
 * @file LocalBlackboardTest.cpp
 * @brief Unit tests for LocalBlackboard (Phase 2.A).
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * Tests cover:
 *   a) Initialize() registers variables and sets default values.
 *   b) GetValue() returns the current value; unknown variable throws.
 *   c) SetValue() updates the value; type mismatch throws.
 *   d) Reset() restores default values.
 *   e) HasVariable() returns true/false correctly.
 *   f) GetVariableNames() returns all registered variable names.
 *
 * Run from the repository root.
 * No SDL3 or World dependency - tests exercise the standalone blackboard only.
 */

#include "TaskSystem/LocalBlackboard.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>

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
// Helper: build a simple TaskGraphTemplate with a few local variables
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakeTemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name = "TestTemplate";

    // Bool variable
    {
        Olympe::VariableDefinition def;
        def.Name         = "IsAlerted";
        def.Type         = Olympe::VariableType::Bool;
        def.DefaultValue = Olympe::TaskValue(false);
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    // Int variable
    {
        Olympe::VariableDefinition def;
        def.Name         = "PatrolIndex";
        def.Type         = Olympe::VariableType::Int;
        def.DefaultValue = Olympe::TaskValue(0);
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    // Float variable
    {
        Olympe::VariableDefinition def;
        def.Name         = "Speed";
        def.Type         = Olympe::VariableType::Float;
        def.DefaultValue = Olympe::TaskValue(3.5f);
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    // String variable
    {
        Olympe::VariableDefinition def;
        def.Name         = "State";
        def.Type         = Olympe::VariableType::String;
        def.DefaultValue = Olympe::TaskValue(std::string("idle"));
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }

    return tmpl;
}

// ---------------------------------------------------------------------------
// Test a: Initialize() registers variables with correct defaults
// ---------------------------------------------------------------------------

static void TestA_Initialize()
{
    std::cout << "Test A: Initialize() registers variables..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard bb;
    bb.Initialize(tmpl);

    bool passed = true;

    TEST_ASSERT(bb.HasVariable("IsAlerted"),   "IsAlerted should be registered");
    if (!bb.HasVariable("IsAlerted"))   { passed = false; }

    TEST_ASSERT(bb.HasVariable("PatrolIndex"), "PatrolIndex should be registered");
    if (!bb.HasVariable("PatrolIndex")) { passed = false; }

    TEST_ASSERT(bb.HasVariable("Speed"),       "Speed should be registered");
    if (!bb.HasVariable("Speed"))       { passed = false; }

    TEST_ASSERT(bb.HasVariable("State"),       "State should be registered");
    if (!bb.HasVariable("State"))       { passed = false; }

    TEST_ASSERT(!bb.HasVariable("Nonexistent"), "Nonexistent should not be registered");
    if (bb.HasVariable("Nonexistent"))  { passed = false; }

    ReportTest("TestA_Initialize", passed);
}

// ---------------------------------------------------------------------------
// Test b: GetValue() returns default values; unknown variable throws
// ---------------------------------------------------------------------------

static void TestB_GetValue()
{
    std::cout << "Test B: GetValue() returns defaults and throws on unknown..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard bb;
    bb.Initialize(tmpl);

    bool passed = true;

    // Check default bool
    Olympe::TaskValue v = bb.GetValue("IsAlerted");
    TEST_ASSERT(v.GetType() == Olympe::VariableType::Bool, "IsAlerted should be Bool");
    TEST_ASSERT(v.AsBool() == false, "IsAlerted default should be false");
    if (v.GetType() != Olympe::VariableType::Bool || v.AsBool() != false) { passed = false; }

    // Check default int
    v = bb.GetValue("PatrolIndex");
    TEST_ASSERT(v.GetType() == Olympe::VariableType::Int, "PatrolIndex should be Int");
    TEST_ASSERT(v.AsInt() == 0, "PatrolIndex default should be 0");
    if (v.GetType() != Olympe::VariableType::Int || v.AsInt() != 0) { passed = false; }

    // Check default float
    v = bb.GetValue("Speed");
    TEST_ASSERT(v.GetType() == Olympe::VariableType::Float, "Speed should be Float");
    TEST_ASSERT(v.AsFloat() == 3.5f, "Speed default should be 3.5");
    if (v.GetType() != Olympe::VariableType::Float || v.AsFloat() != 3.5f) { passed = false; }

    // Check default string
    v = bb.GetValue("State");
    TEST_ASSERT(v.GetType() == Olympe::VariableType::String, "State should be String");
    TEST_ASSERT(v.AsString() == "idle", "State default should be 'idle'");
    if (v.GetType() != Olympe::VariableType::String || v.AsString() != "idle") { passed = false; }

    // Unknown variable must throw
    bool threw = false;
    try { bb.GetValue("UnknownVar"); }
    catch (const std::runtime_error&) { threw = true; }
    TEST_ASSERT(threw, "GetValue on unknown variable should throw std::runtime_error");
    if (!threw) { passed = false; }

    ReportTest("TestB_GetValue", passed);
}

// ---------------------------------------------------------------------------
// Test c: SetValue() updates the value; type mismatch throws
// ---------------------------------------------------------------------------

static void TestC_SetValue()
{
    std::cout << "Test C: SetValue() updates value and throws on type mismatch..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard bb;
    bb.Initialize(tmpl);

    bool passed = true;

    // Set bool
    bb.SetValue("IsAlerted", Olympe::TaskValue(true));
    TEST_ASSERT(bb.GetValue("IsAlerted").AsBool() == true, "IsAlerted should be true after SetValue");
    if (bb.GetValue("IsAlerted").AsBool() != true) { passed = false; }

    // Set int
    bb.SetValue("PatrolIndex", Olympe::TaskValue(5));
    TEST_ASSERT(bb.GetValue("PatrolIndex").AsInt() == 5, "PatrolIndex should be 5 after SetValue");
    if (bb.GetValue("PatrolIndex").AsInt() != 5) { passed = false; }

    // Set float
    bb.SetValue("Speed", Olympe::TaskValue(7.0f));
    TEST_ASSERT(bb.GetValue("Speed").AsFloat() == 7.0f, "Speed should be 7.0 after SetValue");
    if (bb.GetValue("Speed").AsFloat() != 7.0f) { passed = false; }

    // Set string
    bb.SetValue("State", Olympe::TaskValue(std::string("patrol")));
    TEST_ASSERT(bb.GetValue("State").AsString() == "patrol", "State should be 'patrol' after SetValue");
    if (bb.GetValue("State").AsString() != "patrol") { passed = false; }

    // Type mismatch must throw (set Bool with Int value)
    bool threw = false;
    try { bb.SetValue("IsAlerted", Olympe::TaskValue(42)); }
    catch (const std::runtime_error&) { threw = true; }
    TEST_ASSERT(threw, "SetValue with wrong type should throw std::runtime_error");
    if (!threw) { passed = false; }

    // Unknown variable must throw
    threw = false;
    try { bb.SetValue("NoSuchVar", Olympe::TaskValue(1)); }
    catch (const std::runtime_error&) { threw = true; }
    TEST_ASSERT(threw, "SetValue on unknown variable should throw std::runtime_error");
    if (!threw) { passed = false; }

    ReportTest("TestC_SetValue", passed);
}

// ---------------------------------------------------------------------------
// Test d: Reset() restores default values
// ---------------------------------------------------------------------------

static void TestD_Reset()
{
    std::cout << "Test D: Reset() restores defaults..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard bb;
    bb.Initialize(tmpl);

    bool passed = true;

    // Mutate values
    bb.SetValue("IsAlerted",   Olympe::TaskValue(true));
    bb.SetValue("PatrolIndex", Olympe::TaskValue(7));
    bb.SetValue("Speed",       Olympe::TaskValue(99.0f));
    bb.SetValue("State",       Olympe::TaskValue(std::string("combat")));

    // Reset
    bb.Reset();

    TEST_ASSERT(bb.GetValue("IsAlerted").AsBool()   == false,  "IsAlerted should be false after Reset");
    if (bb.GetValue("IsAlerted").AsBool()   != false)  { passed = false; }

    TEST_ASSERT(bb.GetValue("PatrolIndex").AsInt()  == 0,      "PatrolIndex should be 0 after Reset");
    if (bb.GetValue("PatrolIndex").AsInt()  != 0)      { passed = false; }

    TEST_ASSERT(bb.GetValue("Speed").AsFloat()      == 3.5f,   "Speed should be 3.5 after Reset");
    if (bb.GetValue("Speed").AsFloat()      != 3.5f)   { passed = false; }

    TEST_ASSERT(bb.GetValue("State").AsString()     == "idle", "State should be 'idle' after Reset");
    if (bb.GetValue("State").AsString()     != "idle") { passed = false; }

    // Variables should still be registered after Reset
    TEST_ASSERT(bb.HasVariable("IsAlerted"),   "IsAlerted should still exist after Reset");
    if (!bb.HasVariable("IsAlerted"))   { passed = false; }

    ReportTest("TestD_Reset", passed);
}

// ---------------------------------------------------------------------------
// Test e: HasVariable() returns correct result
// ---------------------------------------------------------------------------

static void TestE_HasVariable()
{
    std::cout << "Test E: HasVariable() returns correct result..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard bb;
    bb.Initialize(tmpl);

    bool passed = true;

    TEST_ASSERT(bb.HasVariable("IsAlerted"),    "HasVariable('IsAlerted') should be true");
    if (!bb.HasVariable("IsAlerted"))    { passed = false; }

    TEST_ASSERT(!bb.HasVariable(""),            "HasVariable('') should be false");
    if (bb.HasVariable(""))              { passed = false; }

    TEST_ASSERT(!bb.HasVariable("isalerted"),   "HasVariable is case-sensitive");
    if (bb.HasVariable("isalerted"))     { passed = false; }

    ReportTest("TestE_HasVariable", passed);
}

// ---------------------------------------------------------------------------
// Test f: GetVariableNames() returns all registered names
// ---------------------------------------------------------------------------

static void TestF_GetVariableNames()
{
    std::cout << "Test F: GetVariableNames() returns all registered names..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeTemplate();
    Olympe::LocalBlackboard bb;
    bb.Initialize(tmpl);

    bool passed = true;

    std::vector<std::string> names = bb.GetVariableNames();

    TEST_ASSERT(names.size() == 4, "Should return 4 variable names");
    if (names.size() != 4) { passed = false; }

    // Each expected name must appear exactly once
    const std::string expected[] = { "IsAlerted", "PatrolIndex", "Speed", "State" };
    for (size_t i = 0; i < 4; ++i)
    {
        bool found = (std::find(names.begin(), names.end(), expected[i]) != names.end());
        TEST_ASSERT(found, std::string("Expected variable '") + expected[i] + "' in GetVariableNames()");
        if (!found) { passed = false; }
    }

    // Empty blackboard should return empty list
    Olympe::LocalBlackboard emptyBb;
    std::vector<std::string> emptyNames = emptyBb.GetVariableNames();
    TEST_ASSERT(emptyNames.empty(), "Uninitialized blackboard should return empty name list");
    if (!emptyNames.empty()) { passed = false; }

    ReportTest("TestF_GetVariableNames", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== LocalBlackboardTest ===" << std::endl;

    TestA_Initialize();
    TestB_GetValue();
    TestC_SetValue();
    TestD_Reset();
    TestE_HasVariable();
    TestF_GetVariableNames();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
