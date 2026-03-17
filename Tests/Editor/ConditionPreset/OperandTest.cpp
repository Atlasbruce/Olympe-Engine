/**
 * @file OperandTest.cpp
 * @brief Unit tests for the Operand struct (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * Tests (5):
 *   1. CreateVariable — factory produces correct mode and value
 *   2. CreateConst    — factory produces correct mode and value
 *   3. CreatePin      — factory produces correct mode and value
 *   4. GetDisplayString — correct bracketed string for each mode
 *   5. SerializationRoundTrip — ToJson / FromJson preserves all fields
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/ConditionPreset/Operand.h"

#include <iostream>
#include <string>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure
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

// ---------------------------------------------------------------------------
// Test 1: CreateVariable
// ---------------------------------------------------------------------------

static void Test1_CreateVariable()
{
    bool ok = true;

    Operand op = Operand::CreateVariable("mHealth");

    TEST_ASSERT(op.IsVariable(),               "mode should be Variable");
    TEST_ASSERT(!op.IsConst(),                 "IsConst should be false");
    TEST_ASSERT(!op.IsPin(),                   "IsPin should be false");
    TEST_ASSERT(op.stringValue == "mHealth",   "stringValue should be 'mHealth'");
    TEST_ASSERT(op.constValue  == 0.0,         "constValue should be 0");

    ok = op.IsVariable() && op.stringValue == "mHealth";
    ReportTest("CreateVariable", ok);
}

// ---------------------------------------------------------------------------
// Test 2: CreateConst
// ---------------------------------------------------------------------------

static void Test2_CreateConst()
{
    bool ok = true;

    Operand op = Operand::CreateConst(42.5);

    TEST_ASSERT(!op.IsVariable(),  "mode should not be Variable");
    TEST_ASSERT(op.IsConst(),      "mode should be Const");
    TEST_ASSERT(!op.IsPin(),       "IsPin should be false");
    TEST_ASSERT(op.constValue == 42.5, "constValue should be 42.5");
    TEST_ASSERT(op.stringValue.empty(), "stringValue should be empty");

    ok = op.IsConst() && op.constValue == 42.5;
    ReportTest("CreateConst", ok);
}

// ---------------------------------------------------------------------------
// Test 3: CreatePin
// ---------------------------------------------------------------------------

static void Test3_CreatePin()
{
    bool ok = true;

    Operand op = Operand::CreatePin("Pin:1");

    TEST_ASSERT(!op.IsVariable(),              "mode should not be Variable");
    TEST_ASSERT(!op.IsConst(),                 "mode should not be Const");
    TEST_ASSERT(op.IsPin(),                    "mode should be Pin");
    TEST_ASSERT(op.stringValue == "Pin:1",     "stringValue should be 'Pin:1'");

    ok = op.IsPin() && op.stringValue == "Pin:1";
    ReportTest("CreatePin", ok);
}

// ---------------------------------------------------------------------------
// Test 4: GetDisplayString
// ---------------------------------------------------------------------------

static void Test4_GetDisplayString()
{
    bool ok = true;

    Operand varOp  = Operand::CreateVariable("mHealth");
    Operand cstOp  = Operand::CreateConst(2.0);
    Operand pinOp  = Operand::CreatePin("Pin:1");

    TEST_ASSERT(varOp.GetDisplayString() == "[mHealth]", "Variable display should be '[mHealth]'");
    TEST_ASSERT(cstOp.GetDisplayString() == "[2]",       "Const 2.0 display should be '[2]'");
    TEST_ASSERT(pinOp.GetDisplayString() == "[Pin:1]",   "Pin display should be '[Pin:1]'");

    // Fractional constant
    Operand fracOp = Operand::CreateConst(2.5);
    TEST_ASSERT(fracOp.GetDisplayString() == "[2.5]",    "Fractional const display should be '[2.5]'");

    ok = (varOp.GetDisplayString() == "[mHealth]")
      && (cstOp.GetDisplayString() == "[2]")
      && (pinOp.GetDisplayString() == "[Pin:1]")
      && (fracOp.GetDisplayString() == "[2.5]");

    ReportTest("GetDisplayString", ok);
}

// ---------------------------------------------------------------------------
// Test 5: SerializationRoundTrip
// ---------------------------------------------------------------------------

static void Test5_SerializationRoundTrip()
{
    bool ok = true;

    // Variable round-trip
    {
        Operand original = Operand::CreateVariable("mSpeed");
        nlohmann::json j = original.ToJson();
        Operand loaded   = Operand::FromJson(j);

        TEST_ASSERT(loaded.IsVariable(),                 "RT-Variable: mode should be Variable");
        TEST_ASSERT(loaded.stringValue == "mSpeed",      "RT-Variable: stringValue should be 'mSpeed'");
        if (!loaded.IsVariable() || loaded.stringValue != "mSpeed") ok = false;
    }

    // Const round-trip
    {
        Operand original = Operand::CreateConst(99.0);
        nlohmann::json j = original.ToJson();
        Operand loaded   = Operand::FromJson(j);

        TEST_ASSERT(loaded.IsConst(),         "RT-Const: mode should be Const");
        TEST_ASSERT(loaded.constValue == 99.0, "RT-Const: constValue should be 99.0");
        if (!loaded.IsConst() || loaded.constValue != 99.0) ok = false;
    }

    // Pin round-trip
    {
        Operand original = Operand::CreatePin("Pin:2");
        nlohmann::json j = original.ToJson();
        Operand loaded   = Operand::FromJson(j);

        TEST_ASSERT(loaded.IsPin(),                   "RT-Pin: mode should be Pin");
        TEST_ASSERT(loaded.stringValue == "Pin:2",    "RT-Pin: stringValue should be 'Pin:2'");
        if (!loaded.IsPin() || loaded.stringValue != "Pin:2") ok = false;
    }

    ReportTest("SerializationRoundTrip", ok);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== OperandTest (Phase 24.0) ===" << std::endl;

    Test1_CreateVariable();
    Test2_CreateConst();
    Test3_CreatePin();
    Test4_GetDisplayString();
    Test5_SerializationRoundTrip();

    std::cout << std::endl
              << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
