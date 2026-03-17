/**
 * @file ConditionPresetEditDialogTest.cpp
 * @brief Unit tests for Phase 24.1 — ConditionPresetEditDialog.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * Tests operate directly on ConditionPresetEditDialog logic methods —
 * no SDL3, ImGui, or ImNodes dependency required.
 *
 * Test cases:
 *   1.  Test_Dialog_CreateMode           — Create mode opens with blank/default form
 *   2.  Test_Dialog_EditMode             — Edit mode loads existing preset data
 *   3.  Test_Dialog_OperandSelectorVariable — SetLeftMode("Variable") + SetLeftVariable()
 *   4.  Test_Dialog_OperandSelectorConst    — SetRightMode("Const") + SetRightConst()
 *   5.  Test_Dialog_OperandSelectorPin      — SetLeftMode("Pin") + SetLeftPin()
 *   6.  Test_Dialog_OperatorSelector        — all 6 operators accepted; invalid rejected
 *   7.  Test_Dialog_PreviewUpdates          — GetPreview() reflects current state
 *   8.  Test_Dialog_SaveValidCondition      — Confirm() succeeds on valid condition
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/ConditionPreset.h"
#include "Editor/Dialogs/ConditionPresetEditDialog.h"

#include <iostream>
#include <string>
#include <vector>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure
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
// Helper: build a complete valid preset
// ---------------------------------------------------------------------------

static ConditionPreset MakeValidPreset()
{
    ConditionPreset p;
    p.id   = "test_preset_001";
    p.name = "Health Check";
    p.condition.leftMode      = "Variable";
    p.condition.leftVariable  = "mHealth";
    p.condition.operatorStr   = "<=";
    p.condition.rightMode     = "Const";
    p.condition.rightConstValue = TaskValue(2);
    return p;
}

// ---------------------------------------------------------------------------
// Test 1: Create mode opens blank form
// ---------------------------------------------------------------------------

static void Test_Dialog_CreateMode()
{
    const std::string name = "Dialog_CreateMode";
    int prevFail = s_failCount;

    ConditionPresetEditDialog dlg(ConditionPresetEditDialog::Mode::Create);

    TEST_ASSERT(dlg.GetMode() == ConditionPresetEditDialog::Mode::Create,
                "Mode should be Create");
    TEST_ASSERT(!dlg.IsOpen(),      "Dialog should be closed initially");
    TEST_ASSERT(!dlg.IsConfirmed(), "Dialog should not be confirmed initially");

    // Default form state
    TEST_ASSERT(dlg.GetLeftMode()  == "Variable", "Default left mode is Variable");
    TEST_ASSERT(dlg.GetOperator()  == "==",       "Default operator is ==");
    TEST_ASSERT(dlg.GetRightMode() == "Const",    "Default right mode is Const");

    // Working copy name is empty by default
    const ConditionPreset result = dlg.GetResult();
    TEST_ASSERT(result.id.empty(),   "Create mode should start with empty ID");

    dlg.Open();
    TEST_ASSERT(dlg.IsOpen(),       "Dialog should be open after Open()");
    TEST_ASSERT(!dlg.IsConfirmed(), "IsConfirmed reset on Open()");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2: Edit mode loads existing preset
// ---------------------------------------------------------------------------

static void Test_Dialog_EditMode()
{
    const std::string name = "Dialog_EditMode";
    int prevFail = s_failCount;

    const ConditionPreset existing = MakeValidPreset();
    ConditionPresetEditDialog dlg(ConditionPresetEditDialog::Mode::Edit, &existing);

    TEST_ASSERT(dlg.GetMode() == ConditionPresetEditDialog::Mode::Edit,
                "Mode should be Edit");

    // Should pre-populate from existing preset
    TEST_ASSERT(dlg.GetLeftMode()  == "Variable", "Left mode loaded from preset");
    TEST_ASSERT(dlg.GetOperator()  == "<=",       "Operator loaded from preset");
    TEST_ASSERT(dlg.GetRightMode() == "Const",    "Right mode loaded from preset");

    const ConditionPreset result = dlg.GetResult();
    TEST_ASSERT(result.id   == "test_preset_001", "ID preserved from existing preset");
    TEST_ASSERT(result.name == "Health Check",    "Name preserved from existing preset");
    TEST_ASSERT(result.condition.leftVariable == "mHealth",
                "Left variable preserved from existing preset");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: Operand selector — Variable mode
// ---------------------------------------------------------------------------

static void Test_Dialog_OperandSelectorVariable()
{
    const std::string name = "Dialog_OperandSelectorVariable";
    int prevFail = s_failCount;

    ConditionPresetEditDialog dlg;

    dlg.SetLeftMode("Variable");
    dlg.SetLeftVariable("mSpeed");

    TEST_ASSERT(dlg.GetLeftMode() == "Variable",    "Left mode is Variable");
    TEST_ASSERT(dlg.GetResult().condition.leftVariable == "mSpeed",
                "Left variable set correctly");

    // Preview should contain [mSpeed]
    dlg.SetOperator(">=");
    dlg.SetRightMode("Const");
    dlg.SetRightConst(TaskValue(100));
    const std::string preview = dlg.GetPreview();
    TEST_ASSERT(preview.find("[mSpeed]") != std::string::npos,
                "Preview should contain [mSpeed]");
    TEST_ASSERT(preview.find(">=") != std::string::npos,
                "Preview should contain >=");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: Operand selector — Const mode
// ---------------------------------------------------------------------------

static void Test_Dialog_OperandSelectorConst()
{
    const std::string name = "Dialog_OperandSelectorConst";
    int prevFail = s_failCount;

    ConditionPresetEditDialog dlg;

    dlg.SetRightMode("Const");
    dlg.SetRightConst(TaskValue(42));

    TEST_ASSERT(dlg.GetRightMode() == "Const", "Right mode is Const");

    // The const value should appear in the preview
    dlg.SetLeftMode("Variable");
    dlg.SetLeftVariable("AI_Mode");
    dlg.SetOperator("==");

    const std::string preview = dlg.GetPreview();
    TEST_ASSERT(preview.find("[AI_Mode]") != std::string::npos,
                "Preview should contain variable name");
    TEST_ASSERT(preview.find("[42]") != std::string::npos ||
                preview.find("42") != std::string::npos,
                "Preview should contain const value 42");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: Operand selector — Pin mode
// ---------------------------------------------------------------------------

static void Test_Dialog_OperandSelectorPin()
{
    const std::string name = "Dialog_OperandSelectorPin";
    int prevFail = s_failCount;

    ConditionPresetEditDialog dlg;

    dlg.SetLeftMode("Pin");
    dlg.SetLeftPin("Node#1.Out");
    dlg.SetOperator("!=");
    dlg.SetRightMode("Pin");
    dlg.SetRightPin("Node#2.Out");

    TEST_ASSERT(dlg.GetLeftMode()  == "Pin", "Left mode is Pin");
    TEST_ASSERT(dlg.GetRightMode() == "Pin", "Right mode is Pin");

    const ConditionPreset result = dlg.GetResult();
    TEST_ASSERT(result.condition.leftPin  == "Node#1.Out", "Left pin set correctly");
    TEST_ASSERT(result.condition.rightPin == "Node#2.Out", "Right pin set correctly");

    const std::string preview = dlg.GetPreview();
    TEST_ASSERT(preview.find("Node#1.Out") != std::string::npos,
                "Preview should contain left pin reference");
    TEST_ASSERT(preview.find("Node#2.Out") != std::string::npos,
                "Preview should contain right pin reference");
    TEST_ASSERT(preview.find("!=") != std::string::npos,
                "Preview should contain the != operator");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6: Operator selector — all 6 operators
// ---------------------------------------------------------------------------

static void Test_Dialog_OperatorSelector()
{
    const std::string name = "Dialog_OperatorSelector";
    int prevFail = s_failCount;

    const std::vector<std::string> validOps = { "==", "!=", "<", "<=", ">", ">=" };

    for (const auto& op : validOps)
    {
        ConditionPresetEditDialog dlg;
        dlg.SetLeftMode("Variable");
        dlg.SetLeftVariable("x");
        dlg.SetOperator(op);
        dlg.SetRightMode("Const");
        dlg.SetRightConst(TaskValue(0));

        TEST_ASSERT(dlg.GetOperator() == op,
                    ("Operator '" + op + "' should be stored correctly").c_str());
        TEST_ASSERT(dlg.IsValid(),
                    ("Condition with operator '" + op + "' should be valid").c_str());

        const std::string preview = dlg.GetPreview();
        TEST_ASSERT(preview.find(op) != std::string::npos,
                    ("Preview should contain operator '" + op + "'").c_str());
    }

    // Invalid operator → IsValid() returns false
    ConditionPresetEditDialog dlgBad;
    dlgBad.SetLeftMode("Variable");
    dlgBad.SetLeftVariable("x");
    dlgBad.SetOperator("??");
    dlgBad.SetRightMode("Const");
    dlgBad.SetRightConst(TaskValue(0));
    TEST_ASSERT(!dlgBad.IsValid(),
                "Condition with invalid operator should not be valid");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7: Preview updates dynamically
// ---------------------------------------------------------------------------

static void Test_Dialog_PreviewUpdates()
{
    const std::string name = "Dialog_PreviewUpdates";
    int prevFail = s_failCount;

    ConditionPresetEditDialog dlg;

    // Initial empty state
    dlg.SetLeftMode("Variable");
    dlg.SetOperator("==");
    dlg.SetRightMode("Const");

    // No variable set → preview should still return something (not crash)
    const std::string empty = dlg.GetPreview();
    TEST_ASSERT(!empty.empty(), "Preview should be non-empty even with blank operands");

    // Set values
    dlg.SetLeftVariable("mHealth");
    dlg.SetRightConst(TaskValue(2));
    const std::string full = dlg.GetPreview();

    TEST_ASSERT(full.find("[mHealth]") != std::string::npos,
                "Preview should update when variable is set");
    TEST_ASSERT(full.find("==") != std::string::npos,
                "Preview should contain operator");

    // Change operator → preview updates
    dlg.SetOperator("<=");
    const std::string updated = dlg.GetPreview();
    TEST_ASSERT(updated.find("<=") != std::string::npos,
                "Preview should update when operator changes");
    TEST_ASSERT(updated.find("==") == std::string::npos ||
                updated.find("<=") != std::string::npos,
                "Operator update should be reflected");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8: Confirm() succeeds on valid condition
// ---------------------------------------------------------------------------

static void Test_Dialog_SaveValidCondition()
{
    const std::string name = "Dialog_SaveValidCondition";
    int prevFail = s_failCount;

    ConditionPresetEditDialog dlg(ConditionPresetEditDialog::Mode::Create);
    dlg.Open();

    // Build a valid condition
    dlg.SetName("My Preset");
    dlg.SetLeftMode("Variable");
    dlg.SetLeftVariable("mSpeed");
    dlg.SetOperator(">=");
    dlg.SetRightMode("Const");
    dlg.SetRightConst(TaskValue(100.0f));

    TEST_ASSERT(dlg.IsValid(),   "Condition should be valid before Confirm");
    TEST_ASSERT(dlg.IsOpen(),    "Dialog should still be open");
    TEST_ASSERT(!dlg.IsConfirmed(), "Not confirmed yet");

    const bool ok = dlg.Confirm();

    TEST_ASSERT(ok,                 "Confirm() should return true for valid condition");
    TEST_ASSERT(dlg.IsConfirmed(),  "IsConfirmed() should be true after Confirm()");
    TEST_ASSERT(!dlg.IsOpen(),      "Dialog should be closed after Confirm()");

    const ConditionPreset result = dlg.GetResult();
    TEST_ASSERT(result.name == "My Preset",        "Name preserved in result");
    TEST_ASSERT(result.condition.leftVariable == "mSpeed",
                "Left variable preserved in result");
    TEST_ASSERT(result.condition.operatorStr == ">=",
                "Operator preserved in result");

    // Invalid condition → Confirm() returns false
    ConditionPresetEditDialog dlgInvalid;
    dlgInvalid.Open();
    dlgInvalid.SetLeftMode("Variable");
    // leftVariable not set → invalid
    dlgInvalid.SetOperator("==");
    dlgInvalid.SetRightMode("Const");
    dlgInvalid.SetRightConst(TaskValue(1));

    TEST_ASSERT(!dlgInvalid.IsValid(),        "Empty variable → invalid");
    TEST_ASSERT(!dlgInvalid.Confirm(),         "Confirm() should fail on invalid condition");
    TEST_ASSERT(!dlgInvalid.IsConfirmed(),     "IsConfirmed should remain false");
    TEST_ASSERT(dlgInvalid.IsOpen(),           "Dialog should remain open after failed Confirm");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 24.1 Tests — ConditionPresetEditDialog ===" << std::endl;

    Test_Dialog_CreateMode();
    Test_Dialog_EditMode();
    Test_Dialog_OperandSelectorVariable();
    Test_Dialog_OperandSelectorConst();
    Test_Dialog_OperandSelectorPin();
    Test_Dialog_OperatorSelector();
    Test_Dialog_PreviewUpdates();
    Test_Dialog_SaveValidCondition();

    std::cout << "\n=== Results: "
              << s_passCount << " passed, "
              << s_failCount << " failed ===" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
