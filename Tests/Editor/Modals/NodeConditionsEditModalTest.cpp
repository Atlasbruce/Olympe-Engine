/**
 * @file NodeConditionsEditModalTest.cpp
 * @brief Unit tests for NodeConditionsEditModal (Phase 24-REFONTE).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * Tests (12):
 *   1.  DefaultState              — modal is closed, not confirmed on construction
 *   2.  Open                      — Open() copies refs and opens the modal
 *   3.  Close                     — Close() sets IsOpen() to false
 *   4.  AddCondition              — adds a condition to the working copy
 *   5.  RemoveCondition           — removes a condition by index
 *   6.  SetLogicalOp              — changes op; index 0 always stays Start
 *   7.  MoveConditionUp           — swaps condition with predecessor
 *   8.  MoveConditionDown         — swaps condition with successor
 *   9.  Confirm                   — Apply sets IsConfirmed() and closes modal
 *  10.  GetConditionRefs_AfterConfirm — GetConditionRefs() returns edited list
 *  11.  FilteredDropdown          — GetFilteredPresetsForDropdown respects filter
 *  12.  NormalizeFirstOpStart     — first condition always gets LogicalOp::Start
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/Modals/NodeConditionsEditModal.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/ConditionPresetRegistry.h"
#include "Editor/ConditionPreset/Operand.h"
#include "Editor/ConditionPreset/NodeConditionRef.h"

#include <iostream>
#include <string>
#include <vector>

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

/// Helper: builds a simple Variable<=Const preset.
static ConditionPreset MakePreset(const std::string& id,
                                   const std::string& name)
{
    ConditionPreset p(id,
                      Operand::CreateVariable("mHealth"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(2.0));
    p.name = name;
    return p;
}

// ---------------------------------------------------------------------------
// Test 1: DefaultState
// ---------------------------------------------------------------------------

static void Test1_DefaultState()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsEditModal modal(reg);

    TEST_ASSERT(!modal.IsOpen(),      "Modal should be closed on construction");
    TEST_ASSERT(!modal.IsConfirmed(), "Modal should not be confirmed on construction");
    TEST_ASSERT(modal.GetConditionCount() == 0u, "No conditions on construction");

    ReportTest("Test1_DefaultState", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: Open
// ---------------------------------------------------------------------------

static void Test2_Open()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "Cond A"));

    std::vector<NodeConditionRef> refs;
    refs.emplace_back("p1", LogicalOp::Start);

    NodeConditionsEditModal modal(reg);
    modal.Open(refs);

    TEST_ASSERT(modal.IsOpen(),             "Modal should be open after Open()");
    TEST_ASSERT(!modal.IsConfirmed(),       "Not confirmed after Open()");
    TEST_ASSERT(modal.GetConditionCount() == 1u, "Should have 1 condition from refs");
    TEST_ASSERT(modal.GetConditionRefs()[0].presetID == "p1",
                "Preset ID should match");

    ReportTest("Test2_Open", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: Close
// ---------------------------------------------------------------------------

static void Test3_Close()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsEditModal modal(reg);
    modal.Open({});

    TEST_ASSERT(modal.IsOpen(), "Should be open");
    modal.Close();
    TEST_ASSERT(!modal.IsOpen(), "Should be closed after Close()");

    ReportTest("Test3_Close", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: AddCondition
// ---------------------------------------------------------------------------

static void Test4_AddCondition()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));

    NodeConditionsEditModal modal(reg);
    modal.Open({});

    modal.AddCondition("p1");
    TEST_ASSERT(modal.GetConditionCount() == 1u, "Expected 1 condition");
    TEST_ASSERT(modal.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "First condition should be Start");

    modal.AddCondition("p2");
    TEST_ASSERT(modal.GetConditionCount() == 2u, "Expected 2 conditions");
    TEST_ASSERT(modal.GetConditionRefs()[1].logicalOp == LogicalOp::And,
                "Second condition should default to And");

    ReportTest("Test4_AddCondition", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: RemoveCondition
// ---------------------------------------------------------------------------

static void Test5_RemoveCondition()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));

    NodeConditionsEditModal modal(reg);
    modal.Open({});
    modal.AddCondition("p1");
    modal.AddCondition("p2");

    modal.RemoveCondition(0);
    TEST_ASSERT(modal.GetConditionCount() == 1u, "Expected 1 after removal");
    TEST_ASSERT(modal.GetConditionRefs()[0].presetID == "p2", "Wrong remaining preset");
    TEST_ASSERT(modal.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "Remaining first should be Start after removal");

    ReportTest("Test5_RemoveCondition", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: SetLogicalOp
// ---------------------------------------------------------------------------

static void Test6_SetLogicalOp()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));

    NodeConditionsEditModal modal(reg);
    modal.Open({});
    modal.AddCondition("p1");
    modal.AddCondition("p2");

    modal.SetLogicalOp(1, LogicalOp::Or);
    TEST_ASSERT(modal.GetConditionRefs()[1].logicalOp == LogicalOp::Or,
                "Index 1 should be Or");

    // Index 0 must stay Start
    modal.SetLogicalOp(0, LogicalOp::And);
    TEST_ASSERT(modal.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "Index 0 must always stay Start");

    ReportTest("Test6_SetLogicalOp", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 7: MoveConditionUp
// ---------------------------------------------------------------------------

static void Test7_MoveConditionUp()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));
    reg.CreatePreset(MakePreset("p3", "C"));

    NodeConditionsEditModal modal(reg);
    modal.Open({});
    modal.AddCondition("p1");
    modal.AddCondition("p2");
    modal.AddCondition("p3");

    // Move index 2 up → p3 should now be at index 1
    modal.MoveConditionUp(2);
    TEST_ASSERT(modal.GetConditionRefs()[1].presetID == "p3",
                "p3 should be at index 1 after MoveUp");
    TEST_ASSERT(modal.GetConditionRefs()[2].presetID == "p2",
                "p2 should be at index 2 after MoveUp");

    // MoveUp on index 0 is a no-op
    modal.MoveConditionUp(0);
    TEST_ASSERT(modal.GetConditionRefs()[0].presetID == "p1",
                "p1 should still be at index 0");

    ReportTest("Test7_MoveConditionUp", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 8: MoveConditionDown
// ---------------------------------------------------------------------------

static void Test8_MoveConditionDown()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));
    reg.CreatePreset(MakePreset("p3", "C"));

    NodeConditionsEditModal modal(reg);
    modal.Open({});
    modal.AddCondition("p1");
    modal.AddCondition("p2");
    modal.AddCondition("p3");

    // Move index 0 down → p1 should now be at index 1
    modal.MoveConditionDown(0);
    TEST_ASSERT(modal.GetConditionRefs()[0].presetID == "p2",
                "p2 should be at index 0 after MoveDown");
    TEST_ASSERT(modal.GetConditionRefs()[1].presetID == "p1",
                "p1 should be at index 1 after MoveDown");

    // MoveDown on last index is a no-op
    const size_t last = modal.GetConditionCount() - 1;
    const std::string lastID = modal.GetConditionRefs()[last].presetID;
    modal.MoveConditionDown(last);
    TEST_ASSERT(modal.GetConditionRefs()[last].presetID == lastID,
                "Last condition should be unchanged after MoveDown at end");

    ReportTest("Test8_MoveConditionDown", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 9: Confirm
// ---------------------------------------------------------------------------

static void Test9_Confirm()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsEditModal modal(reg);
    modal.Open({});

    TEST_ASSERT(!modal.IsConfirmed(), "Not confirmed initially");
    modal.Confirm();
    TEST_ASSERT(modal.IsConfirmed(), "Should be confirmed after Confirm()");
    TEST_ASSERT(!modal.IsOpen(),     "Modal should close after Confirm()");

    ReportTest("Test9_Confirm", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 10: GetConditionRefs_AfterConfirm
// ---------------------------------------------------------------------------

static void Test10_GetConditionRefs_AfterConfirm()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));

    NodeConditionsEditModal modal(reg);
    modal.Open({});
    modal.AddCondition("p1");
    modal.AddCondition("p2");
    modal.SetLogicalOp(1, LogicalOp::Or);
    modal.Confirm();

    const std::vector<NodeConditionRef>& refs = modal.GetConditionRefs();
    TEST_ASSERT(refs.size() == 2u, "Expected 2 conditions after confirm");
    TEST_ASSERT(refs[0].presetID == "p1", "First preset should be p1");
    TEST_ASSERT(refs[0].logicalOp == LogicalOp::Start, "First should be Start");
    TEST_ASSERT(refs[1].presetID == "p2", "Second preset should be p2");
    TEST_ASSERT(refs[1].logicalOp == LogicalOp::Or, "Second should be Or");

    ReportTest("Test10_GetConditionRefs_AfterConfirm", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 11: FilteredDropdown
// ---------------------------------------------------------------------------

static void Test11_FilteredDropdown()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "SpeedCheck"));
    reg.CreatePreset(MakePreset("p2", "HealthCheck"));
    reg.CreatePreset(MakePreset("p3", "AlertCheck"));

    NodeConditionsEditModal modal(reg);
    modal.Open({});

    // No filter: all 3 presets
    modal.SetDropdownFilter("");
    TEST_ASSERT(modal.GetFilteredPresetsForDropdown().size() == 3u,
                "Expected 3 presets with empty filter");

    // Filter "Health": only HealthCheck
    modal.SetDropdownFilter("Health");
    TEST_ASSERT(modal.GetFilteredPresetsForDropdown().size() == 1u,
                "Only HealthCheck should match 'Health'");

    // Filter "Check": all 3
    modal.SetDropdownFilter("Check");
    TEST_ASSERT(modal.GetFilteredPresetsForDropdown().size() == 3u,
                "All 3 match 'Check'");

    ReportTest("Test11_FilteredDropdown", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 12: NormalizeFirstOpStart
// ---------------------------------------------------------------------------

static void Test12_NormalizeFirstOpStart()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));

    // Build refs with wrong op for index 0
    std::vector<NodeConditionRef> refs;
    refs.emplace_back("p1", LogicalOp::And);  // wrong — should become Start

    NodeConditionsEditModal modal(reg);
    modal.Open(refs);

    TEST_ASSERT(modal.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "Open() must normalize index 0 to Start");

    ReportTest("Test12_NormalizeFirstOpStart", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== NodeConditionsEditModal Tests ===" << std::endl;

    Test1_DefaultState();
    Test2_Open();
    Test3_Close();
    Test4_AddCondition();
    Test5_RemoveCondition();
    Test6_SetLogicalOp();
    Test7_MoveConditionUp();
    Test8_MoveConditionDown();
    Test9_Confirm();
    Test10_GetConditionRefs_AfterConfirm();
    Test11_FilteredDropdown();
    Test12_NormalizeFirstOpStart();

    std::cout << "\nResults: "
              << s_passCount << " passed, "
              << s_failCount << " failed."
              << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
