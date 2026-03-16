/**
 * @file Phase23B6Test.cpp
 * @brief Regression tests for BUG-030 (ImGui conflicting ID in Condition Builder).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * BUG-030: When two operands share the same mode (Pin/Pin, Var/Var, or
 *          Const/Const), the right-side mode buttons were rendered with active
 *          state labels "[PIN]", "[VAR]", "[CST]" that were identical to the
 *          left-side labels inside the same ImGui::PushID(conditionIndex) scope.
 *          This triggered the "2 visible items with conflicting ID" ImGui error.
 *
 *          Fix: append "##r" to the right-side active-state labels so that the
 *          display text is the same ("[PIN]", "[VAR]", "[CST]") but the ImGui
 *          ID string is unique ("[PIN]##r" vs "[PIN]").
 *
 *          The fix is in VisualScriptEditorPanel::RenderConditionEditor().
 *          These tests validate the underlying Condition data-layer behaviour
 *          (the ImGui rendering itself requires a graphics context and is not
 *          unit-testable here).
 *
 * Test cases (5):
 *   1.  ConditionModes_PinPin       — both operands can be Pin mode simultaneously
 *   2.  ConditionModes_VarVar       — both operands can be Variable mode simultaneously
 *   3.  ConditionModes_ConstConst   — both operands can be Const mode simultaneously
 *   4.  ConditionModes_Mixed        — left Pin / right Variable (and variants)
 *   5.  MultipleConditions_UniqueIndex — 3 conditions each hold their distinct modes
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <vector>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure (mirrors Phase23B5Test.cpp)
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                              \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::cout << "  FAIL: " << (msg) << std::endl;                 \
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
// BUG-030 Tests: ImGui ID conflict — Condition struct data layer
// ---------------------------------------------------------------------------

// Test 1: Both operands set to Pin mode (the primary trigger of the original bug).
// After the fix the right-side button uses "[PIN]##r" so there is no duplicate
// ImGui ID.  Here we verify the Condition data is consistent when both sides
// use Pin mode.
static void Test1_ConditionModes_PinPin()
{
    bool ok = true;

    Condition cond;
    cond.leftMode  = "Pin";
    cond.leftPin   = "Node#1.Out";
    cond.operatorStr = "==";
    cond.rightMode = "Pin";
    cond.rightPin  = "Node#2.Out";

    TEST_ASSERT(cond.leftMode  == "Pin",         "leftMode should be Pin");
    TEST_ASSERT(cond.rightMode == "Pin",         "rightMode should be Pin");
    TEST_ASSERT(cond.leftPin   == "Node#1.Out",  "leftPin should be set");
    TEST_ASSERT(cond.rightPin  == "Node#2.Out",  "rightPin should be set");
    // Modes are the same but the data fields are independent — no conflict
    TEST_ASSERT(cond.leftPin != cond.rightPin,   "left/right pins should differ");

    ok = (cond.leftMode == "Pin") && (cond.rightMode == "Pin")
      && (cond.leftPin != cond.rightPin);

    ReportTest("ConditionModes_PinPin", ok);
}

// Test 2: Both operands set to Variable mode.
static void Test2_ConditionModes_VarVar()
{
    bool ok = true;

    Condition cond;
    cond.leftMode      = "Variable";
    cond.leftVariable  = "mSpeed";
    cond.operatorStr   = ">";
    cond.rightMode     = "Variable";
    cond.rightVariable = "mHealth";

    TEST_ASSERT(cond.leftMode      == "Variable", "leftMode should be Variable");
    TEST_ASSERT(cond.rightMode     == "Variable", "rightMode should be Variable");
    TEST_ASSERT(cond.leftVariable  == "mSpeed",   "leftVariable should be set");
    TEST_ASSERT(cond.rightVariable == "mHealth",  "rightVariable should be set");
    TEST_ASSERT(cond.leftVariable  != cond.rightVariable,
                "left/right variables should differ");

    ok = (cond.leftMode == "Variable") && (cond.rightMode == "Variable")
      && (cond.leftVariable != cond.rightVariable);

    ReportTest("ConditionModes_VarVar", ok);
}

// Test 3: Both operands set to Const mode.
static void Test3_ConditionModes_ConstConst()
{
    bool ok = true;

    Condition cond;
    cond.leftMode        = "Const";
    cond.leftConstValue  = TaskValue(10);
    cond.operatorStr     = "<";
    cond.rightMode       = "Const";
    cond.rightConstValue = TaskValue(20);

    TEST_ASSERT(cond.leftMode  == "Const",    "leftMode should be Const");
    TEST_ASSERT(cond.rightMode == "Const",    "rightMode should be Const");
    TEST_ASSERT(!cond.leftConstValue.IsNone(),  "leftConstValue should not be None");
    TEST_ASSERT(!cond.rightConstValue.IsNone(), "rightConstValue should not be None");
    TEST_ASSERT(cond.leftConstValue.AsInt()  == 10, "leftConst should be 10");
    TEST_ASSERT(cond.rightConstValue.AsInt() == 20, "rightConst should be 20");

    ok = (cond.leftMode == "Const") && (cond.rightMode == "Const")
      && (cond.leftConstValue.AsInt() == 10)
      && (cond.rightConstValue.AsInt() == 20);

    ReportTest("ConditionModes_ConstConst", ok);
}

// Test 4: Mixed left Pin / right Variable — tests that mode fields are
// independent even when they differ.
static void Test4_ConditionModes_Mixed()
{
    bool ok = true;

    // Variant A: left = Pin, right = Variable
    {
        Condition cond;
        cond.leftMode      = "Pin";
        cond.leftPin       = "Node#3.Speed";
        cond.operatorStr   = ">=";
        cond.rightMode     = "Variable";
        cond.rightVariable = "mHealth";

        TEST_ASSERT(cond.leftMode  == "Pin",      "A: leftMode should be Pin");
        TEST_ASSERT(cond.rightMode == "Variable", "A: rightMode should be Variable");
        TEST_ASSERT(cond.leftPin   == "Node#3.Speed", "A: leftPin should be set");
        TEST_ASSERT(cond.rightVariable == "mHealth",  "A: rightVariable should be set");
        ok = ok && (cond.leftMode == "Pin") && (cond.rightMode == "Variable");
    }

    // Variant B: left = Variable, right = Const
    {
        Condition cond;
        cond.leftMode        = "Variable";
        cond.leftVariable    = "AI_Mode";
        cond.operatorStr     = "==";
        cond.rightMode       = "Const";
        cond.rightConstValue = TaskValue(2);

        TEST_ASSERT(cond.leftMode  == "Variable", "B: leftMode should be Variable");
        TEST_ASSERT(cond.rightMode == "Const",    "B: rightMode should be Const");
        TEST_ASSERT(cond.leftVariable == "AI_Mode",       "B: leftVariable should be set");
        TEST_ASSERT(cond.rightConstValue.AsInt() == 2,    "B: rightConst should be 2");
        ok = ok && (cond.leftMode == "Variable") && (cond.rightMode == "Const");
    }

    ReportTest("ConditionModes_Mixed", ok);
}

// Test 5: Three conditions, each with a distinct index.
// Validates that condition data is kept independent (mirrors the
// RenderConditionEditor loop where PushID(condIndex) is called per condition).
static void Test5_MultipleConditions_UniqueIndex()
{
    bool ok = true;

    std::vector<Condition> conditions(3);

    // Condition 0 — Pin/Variable
    conditions[0].leftMode      = "Pin";
    conditions[0].leftPin       = "Node#1.Out";
    conditions[0].operatorStr   = "==";
    conditions[0].rightMode     = "Variable";
    conditions[0].rightVariable = "mSpeed";

    // Condition 1 — Variable/Variable (the classic trigger)
    conditions[1].leftMode      = "Variable";
    conditions[1].leftVariable  = "AI_Mode";
    conditions[1].operatorStr   = "!=";
    conditions[1].rightMode     = "Variable";
    conditions[1].rightVariable = "mHealth";

    // Condition 2 — Pin/Pin (the other classic trigger)
    conditions[2].leftMode  = "Pin";
    conditions[2].leftPin   = "Node#4.X";
    conditions[2].operatorStr = "<";
    conditions[2].rightMode = "Pin";
    conditions[2].rightPin  = "Node#5.Y";

    // Verify each condition is independent
    TEST_ASSERT(conditions[0].leftMode  == "Pin",      "Cond0 leftMode should be Pin");
    TEST_ASSERT(conditions[0].rightMode == "Variable", "Cond0 rightMode should be Variable");

    TEST_ASSERT(conditions[1].leftMode  == "Variable", "Cond1 leftMode should be Variable");
    TEST_ASSERT(conditions[1].rightMode == "Variable", "Cond1 rightMode should be Variable");
    TEST_ASSERT(conditions[1].leftVariable  != conditions[1].rightVariable,
                "Cond1 left/right variables must differ");

    TEST_ASSERT(conditions[2].leftMode  == "Pin",      "Cond2 leftMode should be Pin");
    TEST_ASSERT(conditions[2].rightMode == "Pin",      "Cond2 rightMode should be Pin");
    TEST_ASSERT(conditions[2].leftPin   != conditions[2].rightPin,
                "Cond2 left/right pins must differ");

    // Verify cross-condition independence
    TEST_ASSERT(conditions[0].leftMode != conditions[2].rightMode ||
                conditions[0].leftPin  != conditions[2].rightPin,
                "Conditions 0 and 2 are distinct");

    ok = (conditions[0].leftMode == "Pin")
      && (conditions[1].leftMode == "Variable")
      && (conditions[1].rightMode == "Variable")
      && (conditions[2].leftMode == "Pin")
      && (conditions[2].rightMode == "Pin")
      && (conditions[1].leftVariable  != conditions[1].rightVariable)
      && (conditions[2].leftPin       != conditions[2].rightPin);

    ReportTest("MultipleConditions_UniqueIndex", ok);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 23-B.6 Tests — BUG-030 ImGui Conflicting ID in Condition Builder ==="
              << std::endl;

    Test1_ConditionModes_PinPin();
    Test2_ConditionModes_VarVar();
    Test3_ConditionModes_ConstConst();
    Test4_ConditionModes_Mixed();
    Test5_MultipleConditions_UniqueIndex();

    std::cout << "\n=== Results: " << s_passCount << " passed, "
              << s_failCount << " failed ===" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
