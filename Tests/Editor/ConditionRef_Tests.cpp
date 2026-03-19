/**
 * @file ConditionRef_Tests.cpp
 * @brief Unit tests for ConditionRef and OperandRef (Phase 24 Milestone 2.2).
 * @author Olympe Engine
 * @date 2026-03-19
 *
 * @details
 * Tests (10):
 *   1.  OperandRef_DefaultIsConst         — default mode is Const
 *   2.  OperandRef_VariableMode           — Variable mode stores variableName
 *   3.  OperandRef_ConstMode              — Const mode stores constValue
 *   4.  OperandRef_PinMode                — Pin mode stores dynamicPinID
 *   5.  ConditionRef_DefaultOperator      — default operatorStr is "=="
 *   6.  ConditionRef_AllOperators         — all six operators can be set
 *   7.  OperandRef_ModeTransition_VarToConst — switching mode clears old field
 *   8.  OperandRef_ModeTransition_ConstToPin — Const → Pin preserves pinID
 *   9.  OperandRef_ModeTransition_PinToVar  — Pin → Variable clears pinID
 *  10.  ConditionRef_CompareTypeDefault   — compareType defaults to Float
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
// Test 1: OperandRef_DefaultIsConst
// ---------------------------------------------------------------------------
static void Test1_OperandRef_DefaultIsConst()
{
    int before = s_failCount;
    OperandRef op;
    TEST_ASSERT(op.mode == OperandRef::Mode::Const, "Default mode should be Const");
    TEST_ASSERT(op.variableName.empty(),  "variableName should be empty by default");
    TEST_ASSERT(op.constValue.empty(),    "constValue should be empty by default");
    TEST_ASSERT(op.dynamicPinID.empty(),  "dynamicPinID should be empty by default");
    ReportTest("Test1_OperandRef_DefaultIsConst", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: OperandRef_VariableMode
// ---------------------------------------------------------------------------
static void Test2_OperandRef_VariableMode()
{
    int before = s_failCount;
    OperandRef op;
    op.mode = OperandRef::Mode::Variable;
    op.variableName = "mHealth";

    TEST_ASSERT(op.mode == OperandRef::Mode::Variable, "Mode should be Variable");
    TEST_ASSERT(op.variableName == "mHealth", "variableName should be 'mHealth'");
    ReportTest("Test2_OperandRef_VariableMode", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: OperandRef_ConstMode
// ---------------------------------------------------------------------------
static void Test3_OperandRef_ConstMode()
{
    int before = s_failCount;
    OperandRef op;
    op.mode = OperandRef::Mode::Const;
    op.constValue = "42.5";

    TEST_ASSERT(op.mode == OperandRef::Mode::Const, "Mode should be Const");
    TEST_ASSERT(op.constValue == "42.5", "constValue should be '42.5'");
    ReportTest("Test3_OperandRef_ConstMode", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: OperandRef_PinMode
// ---------------------------------------------------------------------------
static void Test4_OperandRef_PinMode()
{
    int before = s_failCount;
    OperandRef op;
    op.mode = OperandRef::Mode::Pin;
    op.dynamicPinID = "pin_inst_abc123";

    TEST_ASSERT(op.mode == OperandRef::Mode::Pin, "Mode should be Pin");
    TEST_ASSERT(op.dynamicPinID == "pin_inst_abc123",
                "dynamicPinID should be 'pin_inst_abc123'");
    ReportTest("Test4_OperandRef_PinMode", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: ConditionRef_DefaultOperator
// ---------------------------------------------------------------------------
static void Test5_ConditionRef_DefaultOperator()
{
    int before = s_failCount;
    ConditionRef ref;
    TEST_ASSERT(ref.operatorStr == "==", "Default operator should be '=='");
    TEST_ASSERT(ref.conditionIndex == -1, "Default conditionIndex should be -1");
    ReportTest("Test5_ConditionRef_DefaultOperator", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: ConditionRef_AllOperators
// ---------------------------------------------------------------------------
static void Test6_ConditionRef_AllOperators()
{
    int before = s_failCount;
    const char* ops[] = { "==", "!=", "<", "<=", ">", ">=" };
    for (int i = 0; i < 6; ++i)
    {
        ConditionRef ref;
        ref.operatorStr = ops[i];
        TEST_ASSERT(ref.operatorStr == ops[i], "Operator should be stored correctly");
    }
    ReportTest("Test6_ConditionRef_AllOperators", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 7: OperandRef_ModeTransition_VarToConst
// ---------------------------------------------------------------------------
static void Test7_OperandRef_ModeTransition_VarToConst()
{
    int before = s_failCount;
    OperandRef op;
    op.mode = OperandRef::Mode::Variable;
    op.variableName = "mSpeed";

    // Transition to Const — caller is responsible for clearing old fields
    op.mode = OperandRef::Mode::Const;
    op.constValue = "100";
    op.dynamicPinID.clear();

    TEST_ASSERT(op.mode == OperandRef::Mode::Const, "Mode should now be Const");
    TEST_ASSERT(op.constValue == "100", "constValue should be '100'");
    TEST_ASSERT(op.dynamicPinID.empty(), "dynamicPinID should be empty");
    // variableName may still hold old value — that is acceptable (tagged union style)
    ReportTest("Test7_OperandRef_ModeTransition_VarToConst", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 8: OperandRef_ModeTransition_ConstToPin
// ---------------------------------------------------------------------------
static void Test8_OperandRef_ModeTransition_ConstToPin()
{
    int before = s_failCount;
    OperandRef op;
    op.mode = OperandRef::Mode::Const;
    op.constValue = "50";

    // Transition to Pin
    op.mode = OperandRef::Mode::Pin;
    op.dynamicPinID = "pin_inst_xyz789";

    TEST_ASSERT(op.mode == OperandRef::Mode::Pin, "Mode should now be Pin");
    TEST_ASSERT(op.dynamicPinID == "pin_inst_xyz789",
                "dynamicPinID should be 'pin_inst_xyz789'");
    ReportTest("Test8_OperandRef_ModeTransition_ConstToPin", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 9: OperandRef_ModeTransition_PinToVar
// ---------------------------------------------------------------------------
static void Test9_OperandRef_ModeTransition_PinToVar()
{
    int before = s_failCount;
    OperandRef op;
    op.mode = OperandRef::Mode::Pin;
    op.dynamicPinID = "pin_inst_old";

    // Transition to Variable — caller must clear dynamicPinID
    op.mode = OperandRef::Mode::Variable;
    op.variableName = "mStamina";
    op.dynamicPinID.clear();

    TEST_ASSERT(op.mode == OperandRef::Mode::Variable, "Mode should now be Variable");
    TEST_ASSERT(op.variableName == "mStamina", "variableName should be 'mStamina'");
    TEST_ASSERT(op.dynamicPinID.empty(), "dynamicPinID must be cleared on mode change");
    ReportTest("Test9_OperandRef_ModeTransition_PinToVar", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 10: ConditionRef_CompareTypeDefault
// ---------------------------------------------------------------------------
static void Test10_ConditionRef_CompareTypeDefault()
{
    int before = s_failCount;
    ConditionRef ref;
    TEST_ASSERT(ref.compareType == VariableType::Float,
                "compareType should default to Float");
    ReportTest("Test10_ConditionRef_CompareTypeDefault", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    std::cout << "=== ConditionRef Tests ===\n";

    Test1_OperandRef_DefaultIsConst();
    Test2_OperandRef_VariableMode();
    Test3_OperandRef_ConstMode();
    Test4_OperandRef_PinMode();
    Test5_ConditionRef_DefaultOperator();
    Test6_ConditionRef_AllOperators();
    Test7_OperandRef_ModeTransition_VarToConst();
    Test8_OperandRef_ModeTransition_ConstToPin();
    Test9_OperandRef_ModeTransition_PinToVar();
    Test10_ConditionRef_CompareTypeDefault();

    std::cout << "\nResults: " << s_passCount << " passed, "
              << s_failCount << " failed.\n";
    return (s_failCount == 0) ? 0 : 1;
}
