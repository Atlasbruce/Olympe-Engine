/**
 * @file ConditionPresetLibraryPanelTest.cpp
 * @brief Unit tests for Phase 24.1 — ConditionPresetLibraryPanel.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * Tests operate directly on ConditionPresetLibraryPanel logic methods and
 * the underlying ConditionPresetRegistry — no SDL3, ImGui, or ImNodes
 * dependency required.
 *
 * Test cases:
 *   1.  Test_Panel_RenderEmptyLibrary      — empty registry → GetFilteredPresets() == empty
 *   2.  Test_Panel_RenderListWithPresets   — 3 presets added → GetFilteredPresets() returns 3
 *   3.  Test_Panel_OnAddPresetClicked      — fires OnPresetCreated callback
 *   4.  Test_Panel_OnDuplicatePresetClicked — creates copy with "Copy of" name
 *   5.  Test_Panel_OnDeletePresetClicked   — shows delete confirmation
 *   6.  Test_Panel_OnDeleteConfirmed       — removes preset from registry
 *   7.  Test_Panel_SearchFilter            — filter by name/preview substring
 *   8.  Test_Panel_SelectedItemHighlight   — selection state updated correctly
 *   9.  Test_Panel_ReferenceAnalysisShown  — GetReferencingNodes returns correct nodes
 *  10.  Test_Panel_CallbackTriggered       — OnPresetDeleted callback fires on confirmed delete
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/ConditionPreset.h"
#include "Editor/Panels/ConditionPresetLibraryPanel.h"

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
// Helper: build a simple test preset
// ---------------------------------------------------------------------------

static ConditionPreset MakePreset(const std::string& name,
                                   const std::string& leftVar,
                                   const std::string& op,
                                   float              constVal)
{
    ConditionPreset p;
    p.name = name;
    p.condition.leftMode     = "Variable";
    p.condition.leftVariable = leftVar;
    p.condition.operatorStr  = op;
    p.condition.rightMode    = "Const";
    p.condition.rightConstValue = TaskValue(constVal);
    return p;
}

// ---------------------------------------------------------------------------
// Test 1: empty library
// ---------------------------------------------------------------------------

static void Test_Panel_RenderEmptyLibrary()
{
    const std::string name = "Panel_RenderEmptyLibrary";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    ConditionPresetLibraryPanel panel(reg);

    TEST_ASSERT(panel.GetFilteredPresets().empty(),
                "Empty registry should yield no presets");
    TEST_ASSERT(panel.GetSelectedPresetID().empty(),
                "Selected ID should be empty initially");
    TEST_ASSERT(!panel.IsDeleteConfirmationVisible(),
                "Delete confirmation should be hidden initially");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2: list with presets
// ---------------------------------------------------------------------------

static void Test_Panel_RenderListWithPresets()
{
    const std::string name = "Panel_RenderListWithPresets";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    reg.AddPreset(MakePreset("Condition #1", "mHealth", "<=", 2.f));
    reg.AddPreset(MakePreset("Condition #2", "mSpeed",  ">",  100.f));
    reg.AddPreset(MakePreset("Condition #3", "AI_Mode", "==", 3.f));

    ConditionPresetLibraryPanel panel(reg);

    const auto presets = panel.GetFilteredPresets();
    TEST_ASSERT(presets.size() == 3u, "Should return 3 presets");
    TEST_ASSERT(presets[0].name == "Condition #1", "First preset name");
    TEST_ASSERT(presets[1].name == "Condition #2", "Second preset name");
    TEST_ASSERT(presets[2].name == "Condition #3", "Third preset name");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: OnAddPresetClicked fires callback
// ---------------------------------------------------------------------------

static void Test_Panel_OnAddPresetClicked()
{
    const std::string name = "Panel_OnAddPresetClicked";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    ConditionPresetLibraryPanel panel(reg);

    bool callbackFired = false;
    std::string callbackID;
    panel.OnPresetCreated = [&](const std::string& id)
    {
        callbackFired = true;
        callbackID    = id;
    };

    const std::string newID = panel.OnAddPresetClicked();

    TEST_ASSERT(callbackFired,             "OnPresetCreated callback should fire");
    TEST_ASSERT(!callbackID.empty(),       "Callback ID should be non-empty");
    TEST_ASSERT(callbackID == newID,       "Callback ID should match returned ID");
    TEST_ASSERT(reg.HasPreset(newID),      "Registry should contain new preset");
    TEST_ASSERT(reg.GetCount() == 1u,      "Registry should have 1 preset");
    TEST_ASSERT(panel.GetSelectedPresetID() == newID, "New preset should be selected");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: OnDuplicatePresetClicked creates copy
// ---------------------------------------------------------------------------

static void Test_Panel_OnDuplicatePresetClicked()
{
    const std::string name = "Panel_OnDuplicatePresetClicked";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    const std::string origID = reg.AddPreset(MakePreset("MyPreset", "health", "<", 50.f));

    ConditionPresetLibraryPanel panel(reg);

    bool callbackFired = false;
    panel.OnPresetCreated = [&](const std::string&) { callbackFired = true; };

    const std::string dupID = panel.OnDuplicatePresetClicked(origID);

    TEST_ASSERT(callbackFired,               "OnPresetCreated callback should fire on duplicate");
    TEST_ASSERT(!dupID.empty(),              "Duplicate ID should be non-empty");
    TEST_ASSERT(dupID != origID,             "Duplicate ID must differ from original");
    TEST_ASSERT(reg.HasPreset(dupID),        "Registry should contain the duplicate");
    TEST_ASSERT(reg.GetCount() == 2u,        "Registry should have 2 presets after duplicate");

    const ConditionPreset* copy = reg.GetPreset(dupID);
    TEST_ASSERT(copy != nullptr,             "GetPreset(dupID) should not return nullptr");
    TEST_ASSERT(copy->name.find("Copy of") != std::string::npos,
                "Duplicate name should contain 'Copy of'");
    TEST_ASSERT(copy->condition.leftVariable == "health",
                "Duplicate condition data should match original");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: OnDeletePresetClicked shows confirmation (does not delete yet)
// ---------------------------------------------------------------------------

static void Test_Panel_OnDeletePresetClicked()
{
    const std::string name = "Panel_OnDeletePresetClicked";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    const std::string id = reg.AddPreset(MakePreset("ToDelete", "mHP", "<=", 10.f));

    ConditionPresetLibraryPanel panel(reg);

    TEST_ASSERT(!panel.IsDeleteConfirmationVisible(),
                "Confirmation should be hidden before click");

    panel.OnDeletePresetClicked(id);

    TEST_ASSERT(panel.IsDeleteConfirmationVisible(),
                "Confirmation should be visible after click");
    TEST_ASSERT(panel.GetPresetToDelete() == id,
                "PresetToDelete should match the clicked ID");
    TEST_ASSERT(reg.HasPreset(id),
                "Preset should NOT be deleted yet (only confirmation shown)");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6: OnDeleteConfirmed removes from registry
// ---------------------------------------------------------------------------

static void Test_Panel_OnDeleteConfirmed()
{
    const std::string name = "Panel_OnDeleteConfirmed";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    const std::string id = reg.AddPreset(MakePreset("Delete Me", "mHP", "<=", 10.f));

    ConditionPresetLibraryPanel panel(reg);
    panel.OnDeletePresetClicked(id);

    TEST_ASSERT(panel.IsDeleteConfirmationVisible(), "Confirmation visible before confirm");

    panel.OnDeleteConfirmed(id);

    TEST_ASSERT(!panel.IsDeleteConfirmationVisible(),
                "Confirmation should be hidden after confirm");
    TEST_ASSERT(!reg.HasPreset(id),
                "Preset should be removed from registry after confirm");
    TEST_ASSERT(reg.GetCount() == 0u,
                "Registry should be empty after deletion");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7: Search filter
// ---------------------------------------------------------------------------

static void Test_Panel_SearchFilter()
{
    const std::string name = "Panel_SearchFilter";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    reg.AddPreset(MakePreset("Health Check",   "mHealth", "<=", 2.f));
    reg.AddPreset(MakePreset("Speed Limit",    "mSpeed",  ">=", 100.f));
    reg.AddPreset(MakePreset("AI Mode Check",  "AI_Mode", "==", 3.f));

    ConditionPresetLibraryPanel panel(reg);

    // Empty filter → all 3
    panel.SetSearchFilter("");
    TEST_ASSERT(panel.GetFilteredPresets().size() == 3u,
                "Empty filter should return all presets");

    // Filter by name substring (case-insensitive)
    panel.SetSearchFilter("health");
    {
        const auto results = panel.GetFilteredPresets();
        TEST_ASSERT(results.size() == 1u,        "Filter 'health' should match 1");
        TEST_ASSERT(results[0].name == "Health Check", "Matched preset name");
    }

    // Filter by variable name inside preview
    panel.SetSearchFilter("mSpeed");
    {
        const auto results = panel.GetFilteredPresets();
        TEST_ASSERT(results.size() == 1u,             "Filter 'mSpeed' should match 1");
        TEST_ASSERT(results[0].name == "Speed Limit", "Matched by variable in preview");
    }

    // No match
    panel.SetSearchFilter("ZZZZZ");
    TEST_ASSERT(panel.GetFilteredPresets().empty(),
                "Non-matching filter should return empty");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8: Selected item highlight
// ---------------------------------------------------------------------------

static void Test_Panel_SelectedItemHighlight()
{
    const std::string name = "Panel_SelectedItemHighlight";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    const std::string id1 = reg.AddPreset(MakePreset("C1", "v1", "==", 1.f));
    const std::string id2 = reg.AddPreset(MakePreset("C2", "v2", ">", 5.f));

    ConditionPresetLibraryPanel panel(reg);

    TEST_ASSERT(panel.GetSelectedPresetID().empty(), "No selection initially");

    panel.OnPresetSelected(id1);
    TEST_ASSERT(panel.GetSelectedPresetID() == id1, "id1 should be selected");

    panel.OnPresetSelected(id2);
    TEST_ASSERT(panel.GetSelectedPresetID() == id2, "id2 should now be selected");

    // Deselect
    panel.SetSelectedPresetID("");
    TEST_ASSERT(panel.GetSelectedPresetID().empty(), "Selection should be cleared");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 9: Reference analysis
// ---------------------------------------------------------------------------

static void Test_Panel_ReferenceAnalysisShown()
{
    const std::string name = "Panel_ReferenceAnalysisShown";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    const std::string p1 = reg.AddPreset(MakePreset("C1", "h", "<=", 2.f));
    const std::string p2 = reg.AddPreset(MakePreset("C2", "s", ">=", 100.f));

    ConditionPresetLibraryPanel panel(reg);

    // nodeA uses p1 and p2; nodeB uses p1 only
    std::map<std::string, std::vector<std::string>> refMap;
    refMap["Node_A"] = { p1, p2 };
    refMap["Node_B"] = { p1 };
    refMap["Node_C"] = { p2 };
    panel.SetReferenceMap(refMap);

    const auto users1 = panel.GetReferencingNodes(p1);
    TEST_ASSERT(users1.size() == 2u, "p1 should be used by 2 nodes");
    // Order may vary; check both present
    bool hasNodeA = false, hasNodeB = false;
    for (const auto& n : users1)
    {
        if (n == "Node_A") { hasNodeA = true; }
        if (n == "Node_B") { hasNodeB = true; }
    }
    TEST_ASSERT(hasNodeA, "Node_A references p1");
    TEST_ASSERT(hasNodeB, "Node_B references p1");

    const auto users2 = panel.GetReferencingNodes(p2);
    TEST_ASSERT(users2.size() == 2u, "p2 should be used by 2 nodes");

    // Unknown preset → empty
    TEST_ASSERT(panel.GetReferencingNodes("does_not_exist").empty(),
                "Unknown preset should return empty reference list");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 10: OnPresetDeleted callback fires on confirmed delete
// ---------------------------------------------------------------------------

static void Test_Panel_CallbackTriggered()
{
    const std::string name = "Panel_CallbackTriggered";
    int prevFail = s_failCount;

    ConditionPresetRegistry reg;
    const std::string id = reg.AddPreset(MakePreset("TriggerTest", "x", "==", 0.f));

    ConditionPresetLibraryPanel panel(reg);

    bool deletedFired = false;
    std::string deletedID;
    panel.OnPresetDeleted = [&](const std::string& d)
    {
        deletedFired = true;
        deletedID    = d;
    };

    panel.OnDeletePresetClicked(id);
    TEST_ASSERT(!deletedFired, "Callback must NOT fire before confirmation");

    panel.OnDeleteConfirmed(id);
    TEST_ASSERT(deletedFired,       "OnPresetDeleted callback should fire after confirm");
    TEST_ASSERT(deletedID == id,    "Deleted ID should match");
    TEST_ASSERT(!reg.HasPreset(id), "Preset should be gone from registry");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 24.1 Tests — ConditionPresetLibraryPanel ===" << std::endl;

    Test_Panel_RenderEmptyLibrary();
    Test_Panel_RenderListWithPresets();
    Test_Panel_OnAddPresetClicked();
    Test_Panel_OnDuplicatePresetClicked();
    Test_Panel_OnDeletePresetClicked();
    Test_Panel_OnDeleteConfirmed();
    Test_Panel_SearchFilter();
    Test_Panel_SelectedItemHighlight();
    Test_Panel_ReferenceAnalysisShown();
    Test_Panel_CallbackTriggered();

    std::cout << "\n=== Results: "
              << s_passCount << " passed, "
              << s_failCount << " failed ===" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
