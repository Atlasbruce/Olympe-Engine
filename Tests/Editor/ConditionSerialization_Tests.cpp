/**
 * @file ConditionSerialization_Tests.cpp
 * @brief Unit tests for ConditionRef + OperandRef JSON serialization round-trips (Phase 24 M2.2).
 * @author Olympe Engine
 * @date 2026-03-19
 *
 * @details
 * Tests (8):
 *   1.  OperandRef_Variable_RoundTrip   — Variable mode: save→load preserves variableName
 *   2.  OperandRef_Const_RoundTrip      — Const mode: save→load preserves constValue
 *   3.  OperandRef_Pin_RoundTrip        — Pin mode: save→load preserves dynamicPinID exactly
 *   4.  ConditionRef_Full_RoundTrip     — ConditionRef: all fields survive save→load
 *   5.  ConditionRef_CompareType_Int    — compareType Int round-trips correctly
 *   6.  ConditionRef_CompareType_Bool   — compareType Bool round-trips correctly
 *   7.  ConditionRef_Unknown_Mode       — unrecognised mode defaults to Const (backward compat)
 *   8.  OperandRef_Empty_PinID_Preserved— empty dynamicPinID round-trips as empty string
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/ConditionRef.h"

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
            std::cout << "  FAIL: " << (msg) << "\n";                  \
            ++s_failCount;                                              \
        }                                                               \
    } while (false)

static void ReportTest(const std::string& name, bool passed)
{
    if (passed) {
        std::cout << "[PASS] " << name << "\n";
        ++s_passCount;
    } else {
        std::cout << "[FAIL] " << name << "\n";
    }
}

// ---------------------------------------------------------------------------
// Test 1: OperandRef_Variable_RoundTrip
// ---------------------------------------------------------------------------
static void Test1_OperandRef_Variable_RoundTrip()
{
    int before = s_failCount;
    OperandRef original;
    original.mode = OperandRef::Mode::Variable;
    original.variableName = "mHealth";

    const auto json = original.ToJson();
    const OperandRef loaded = OperandRef::FromJson(json);

    TEST_ASSERT(loaded.mode == OperandRef::Mode::Variable,
                "Mode should be Variable after round-trip");
    TEST_ASSERT(loaded.variableName == "mHealth",
                "variableName should survive round-trip");

    ReportTest("Test1_OperandRef_Variable_RoundTrip", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: OperandRef_Const_RoundTrip
// ---------------------------------------------------------------------------
static void Test2_OperandRef_Const_RoundTrip()
{
    int before = s_failCount;
    OperandRef original;
    original.mode = OperandRef::Mode::Const;
    original.constValue = "3.14";

    const auto json = original.ToJson();
    const OperandRef loaded = OperandRef::FromJson(json);

    TEST_ASSERT(loaded.mode == OperandRef::Mode::Const,
                "Mode should be Const after round-trip");
    TEST_ASSERT(loaded.constValue == "3.14",
                "constValue should survive round-trip");

    ReportTest("Test2_OperandRef_Const_RoundTrip", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: OperandRef_Pin_RoundTrip (CRITICAL — dynamicPinID must be exact)
// ---------------------------------------------------------------------------
static void Test3_OperandRef_Pin_RoundTrip()
{
    int before = s_failCount;
    const std::string expectedPinID = "pin_inst_abc123def456xyz";

    OperandRef original;
    original.mode = OperandRef::Mode::Pin;
    original.dynamicPinID = expectedPinID;

    const auto json = original.ToJson();
    const OperandRef loaded = OperandRef::FromJson(json);

    TEST_ASSERT(loaded.mode == OperandRef::Mode::Pin,
                "Mode should be Pin after round-trip");
    TEST_ASSERT(loaded.dynamicPinID == expectedPinID,
                "dynamicPinID must be preserved exactly");

    ReportTest("Test3_OperandRef_Pin_RoundTrip", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: ConditionRef_Full_RoundTrip
// ---------------------------------------------------------------------------
static void Test4_ConditionRef_Full_RoundTrip()
{
    int before = s_failCount;
    ConditionRef original;
    original.conditionIndex = 2;

    original.leftOperand.mode = OperandRef::Mode::Variable;
    original.leftOperand.variableName = "mSpeed";

    original.operatorStr = "<=";

    original.rightOperand.mode = OperandRef::Mode::Pin;
    original.rightOperand.dynamicPinID = "pin_inst_rightXYZ";

    original.compareType = VariableType::Float;

    const auto json = original.ToJson();
    const ConditionRef loaded = ConditionRef::FromJson(json);

    TEST_ASSERT(loaded.conditionIndex == 2,           "conditionIndex preserved");
    TEST_ASSERT(loaded.leftOperand.mode == OperandRef::Mode::Variable,
                "left mode preserved");
    TEST_ASSERT(loaded.leftOperand.variableName == "mSpeed",
                "leftOperand.variableName preserved");
    TEST_ASSERT(loaded.operatorStr == "<=",           "operatorStr preserved");
    TEST_ASSERT(loaded.rightOperand.mode == OperandRef::Mode::Pin,
                "right mode preserved");
    TEST_ASSERT(loaded.rightOperand.dynamicPinID == "pin_inst_rightXYZ",
                "rightOperand.dynamicPinID preserved exactly");
    TEST_ASSERT(loaded.compareType == VariableType::Float,
                "compareType preserved");

    ReportTest("Test4_ConditionRef_Full_RoundTrip", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: ConditionRef_CompareType_Int
// ---------------------------------------------------------------------------
static void Test5_ConditionRef_CompareType_Int()
{
    int before = s_failCount;
    ConditionRef original;
    original.compareType = VariableType::Int;

    const auto json = original.ToJson();
    const ConditionRef loaded = ConditionRef::FromJson(json);

    TEST_ASSERT(loaded.compareType == VariableType::Int,
                "compareType Int should round-trip");

    ReportTest("Test5_ConditionRef_CompareType_Int", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: ConditionRef_CompareType_Bool
// ---------------------------------------------------------------------------
static void Test6_ConditionRef_CompareType_Bool()
{
    int before = s_failCount;
    ConditionRef original;
    original.compareType = VariableType::Bool;

    const auto json = original.ToJson();
    const ConditionRef loaded = ConditionRef::FromJson(json);

    TEST_ASSERT(loaded.compareType == VariableType::Bool,
                "compareType Bool should round-trip");

    ReportTest("Test6_ConditionRef_CompareType_Bool", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 7: OperandRef_Unknown_Mode_DefaultsToConst
// ---------------------------------------------------------------------------
static void Test7_OperandRef_Unknown_Mode_DefaultsToConst()
{
    int before = s_failCount;
    // Build JSON with an unrecognized mode string (backward-compat test)
    nlohmann::json j;
    j["mode"] = "Unknown";
    j["variableName"] = "";
    j["constValue"] = "";
    j["dynamicPinID"] = "";

    const OperandRef loaded = OperandRef::FromJson(j);

    TEST_ASSERT(loaded.mode == OperandRef::Mode::Const,
                "Unknown mode should fall back to Const");

    ReportTest("Test7_OperandRef_Unknown_Mode_DefaultsToConst", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 8: OperandRef_Empty_PinID_Preserved
// ---------------------------------------------------------------------------
static void Test8_OperandRef_Empty_PinID_Preserved()
{
    int before = s_failCount;
    OperandRef original;
    original.mode = OperandRef::Mode::Pin;
    original.dynamicPinID = "";  // not yet assigned

    const auto json = original.ToJson();
    const OperandRef loaded = OperandRef::FromJson(json);

    TEST_ASSERT(loaded.mode == OperandRef::Mode::Pin,
                "Mode should remain Pin even with empty pinID");
    TEST_ASSERT(loaded.dynamicPinID.empty(),
                "Empty dynamicPinID should round-trip as empty");

    ReportTest("Test8_OperandRef_Empty_PinID_Preserved", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    std::cout << "=== ConditionSerialization Tests ===\n";

    Test1_OperandRef_Variable_RoundTrip();
    Test2_OperandRef_Const_RoundTrip();
    Test3_OperandRef_Pin_RoundTrip();
    Test4_ConditionRef_Full_RoundTrip();
    Test5_ConditionRef_CompareType_Int();
    Test6_ConditionRef_CompareType_Bool();
    Test7_OperandRef_Unknown_Mode_DefaultsToConst();
    Test8_OperandRef_Empty_PinID_Preserved();

    std::cout << "\nResults: " << s_passCount << " passed, "
              << s_failCount << " failed.\n";
    return (s_failCount == 0) ? 0 : 1;
}
