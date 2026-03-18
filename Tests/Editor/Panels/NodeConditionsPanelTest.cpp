/**
 * @file NodeConditionsPanelTest.cpp
 * @brief Unit tests for NodeConditionsPanel (Phase 24.2).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * Tests (10):
 *   1.  EmptyPanel              — no conditions on construction
 *   2.  AddCondition            — adds a NodeConditionRef and marks dirty
 *   3.  RemoveCondition         — removes by index, updates list
 *   4.  SetLogicalOp            — changes op for index > 0; index 0 stays Start
 *   5.  NormalizeFirstOpStart   — first condition always receives Start
 *   6.  OnPresetDeleted         — removes matching refs and sets dirty
 *   7.  FilteredDropdown        — GetFilteredPresetsForDropdown respects filter
 *   8.  IsValid_AllPresetsExist — IsValid() returns true when all IDs exist
 *   9.  IsValid_MissingPreset   — IsValid() returns false when ID missing
 *  10.  DirtyFlag               — ClearDirty resets dirty after AddCondition
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/Panels/NodeConditionsPanel.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/ConditionPresetRegistry.h"
#include "Editor/ConditionPreset/Operand.h"

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
// Test 1: EmptyPanel
// ---------------------------------------------------------------------------

static void Test1_EmptyPanel()
{
    bool ok = true;
    ConditionPresetRegistry reg;
    NodeConditionsPanel panel(reg);

    TEST_ASSERT(panel.GetConditionCount() == 0u, "Expected 0 conditions");
    TEST_ASSERT(!panel.IsDirty(),                "Should not be dirty initially");

    ReportTest("Test1_EmptyPanel", ok && s_failCount == 0);
}

// ---------------------------------------------------------------------------
// Test 2: AddCondition
// ---------------------------------------------------------------------------

static void Test2_AddCondition()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "Cond A"));

    NodeConditionsPanel panel(reg);
    panel.AddCondition("p1");

    TEST_ASSERT(panel.GetConditionCount() == 1u, "Expected 1 condition");
    TEST_ASSERT(panel.GetConditionRefs()[0].presetID == "p1", "Wrong presetID");
    TEST_ASSERT(panel.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "First condition should be Start");
    TEST_ASSERT(panel.IsDirty(), "Should be dirty after add");

    ReportTest("Test2_AddCondition", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: RemoveCondition
// ---------------------------------------------------------------------------

static void Test3_RemoveCondition()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));

    NodeConditionsPanel panel(reg);
    panel.AddCondition("p1");
    panel.AddCondition("p2");
    panel.ClearDirty();

    panel.RemoveCondition(0);

    TEST_ASSERT(panel.GetConditionCount() == 1u, "Expected 1 after removal");
    TEST_ASSERT(panel.GetConditionRefs()[0].presetID == "p2", "Wrong preset after removal");
    TEST_ASSERT(panel.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "Remaining first should be Start");
    TEST_ASSERT(panel.IsDirty(), "Should be dirty after remove");

    ReportTest("Test3_RemoveCondition", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: SetLogicalOp
// ---------------------------------------------------------------------------

static void Test4_SetLogicalOp()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));

    NodeConditionsPanel panel(reg);
    panel.AddCondition("p1");
    panel.AddCondition("p2");
    panel.ClearDirty();

    panel.SetLogicalOp(1, LogicalOp::Or);
    TEST_ASSERT(panel.GetConditionRefs()[1].logicalOp == LogicalOp::Or,
                "Index 1 should be Or");
    TEST_ASSERT(panel.IsDirty(), "Should be dirty after SetLogicalOp");

    // Index 0 must stay Start regardless
    panel.SetLogicalOp(0, LogicalOp::And);
    TEST_ASSERT(panel.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "Index 0 must stay Start");

    ReportTest("Test4_SetLogicalOp", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: NormalizeFirstOpStart
// ---------------------------------------------------------------------------

static void Test5_NormalizeFirstOpStart()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));

    // Build a ref manually with wrong logicalOp for index 0
    NodeConditionsPanel panel(reg);
    NodeConditionRef ref("p1", LogicalOp::And);  // wrong — should become Start
    std::vector<NodeConditionRef> refs = { ref };
    panel.SetConditionRefs(refs);

    TEST_ASSERT(panel.GetConditionRefs()[0].logicalOp == LogicalOp::Start,
                "SetConditionRefs must normalize index 0 to Start");

    ReportTest("Test5_NormalizeFirstOpStart", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: OnPresetDeleted
// ---------------------------------------------------------------------------

static void Test6_OnPresetDeleted()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));
    reg.CreatePreset(MakePreset("p2", "B"));

    NodeConditionsPanel panel(reg);
    panel.AddCondition("p1");
    panel.AddCondition("p2");
    panel.ClearDirty();

    panel.OnPresetDeleted("p1");

    TEST_ASSERT(panel.GetConditionCount() == 1u, "Expected 1 condition after delete");
    TEST_ASSERT(panel.GetConditionRefs()[0].presetID == "p2", "Wrong preset remains");
    TEST_ASSERT(panel.IsDirty(), "Should be dirty after OnPresetDeleted");

    ReportTest("Test6_OnPresetDeleted", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 7: FilteredDropdown
// ---------------------------------------------------------------------------

static void Test7_FilteredDropdown()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "SpeedCheck"));
    reg.CreatePreset(MakePreset("p2", "HealthCheck"));
    reg.CreatePreset(MakePreset("p3", "AlertCheck"));

    NodeConditionsPanel panel(reg);

    // No filter: all 3 presets returned
    panel.SetDropdownFilter("");
    TEST_ASSERT(panel.GetFilteredPresetsForDropdown().size() == 3u,
                "Expected 3 presets with empty filter");

    // Filter "Check": all 3 match
    panel.SetDropdownFilter("Check");
    TEST_ASSERT(panel.GetFilteredPresetsForDropdown().size() == 3u,
                "All 3 contain 'Check'");

    // Filter "Speed": only SpeedCheck
    panel.SetDropdownFilter("Speed");
    TEST_ASSERT(panel.GetFilteredPresetsForDropdown().size() == 1u,
                "Only SpeedCheck matches 'Speed'");

    ReportTest("Test7_FilteredDropdown", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 8: IsValid_AllPresetsExist
// ---------------------------------------------------------------------------

static void Test8_IsValid_AllPresetsExist()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));

    NodeConditionsPanel panel(reg);
    panel.AddCondition("p1");

    TEST_ASSERT(panel.IsValid(), "Panel should be valid when all presets exist");

    ReportTest("Test8_IsValid_AllPresetsExist", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 9: IsValid_MissingPreset
// ---------------------------------------------------------------------------

static void Test9_IsValid_MissingPreset()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    // Do NOT register "missing_id"

    NodeConditionsPanel panel(reg);
    // Force a ref to a non-existent preset
    std::vector<NodeConditionRef> refs;
    refs.emplace_back("missing_id", LogicalOp::Start);
    panel.SetConditionRefs(refs);

    TEST_ASSERT(!panel.IsValid(), "Panel should be invalid with missing preset");
    TEST_ASSERT(!panel.Validate().empty(), "Validate() should report errors");

    ReportTest("Test9_IsValid_MissingPreset", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 10: DirtyFlag
// ---------------------------------------------------------------------------

static void Test10_DirtyFlag()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p1", "A"));

    NodeConditionsPanel panel(reg);
    TEST_ASSERT(!panel.IsDirty(), "Initially not dirty");

    panel.AddCondition("p1");
    TEST_ASSERT(panel.IsDirty(), "Dirty after add");

    panel.ClearDirty();
    TEST_ASSERT(!panel.IsDirty(), "Not dirty after clear");

    ReportTest("Test10_DirtyFlag", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 11: NodeName
// ---------------------------------------------------------------------------

static void Test11_NodeName()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsPanel panel(reg);

    TEST_ASSERT(panel.GetNodeName().empty(), "Node name should be empty by default");

    panel.SetNodeName("Is Health Critical?");
    TEST_ASSERT(panel.GetNodeName() == "Is Health Critical?",
                "GetNodeName should return the set name");

    ReportTest("Test11_NodeName", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 12: EditModalRequest
// ---------------------------------------------------------------------------

static void Test12_EditModalRequest()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsPanel panel(reg);

    TEST_ASSERT(!panel.IsEditModalRequested(),
                "Edit modal should not be requested initially");

    panel.ClearEditModalRequest();
    TEST_ASSERT(!panel.IsEditModalRequested(),
                "ClearEditModalRequest should keep it false when already false");

    ReportTest("Test12_EditModalRequest", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== NodeConditionsPanel Tests ===" << std::endl;

    Test1_EmptyPanel();
    Test2_AddCondition();
    Test3_RemoveCondition();
    Test4_SetLogicalOp();
    Test5_NormalizeFirstOpStart();
    Test6_OnPresetDeleted();
    Test7_FilteredDropdown();
    Test8_IsValid_AllPresetsExist();
    Test9_IsValid_MissingPreset();
    Test10_DirtyFlag();
    Test11_NodeName();
    Test12_EditModalRequest();

    std::cout << "\nResults: "
              << s_passCount << " passed, "
              << s_failCount << " failed."
              << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
