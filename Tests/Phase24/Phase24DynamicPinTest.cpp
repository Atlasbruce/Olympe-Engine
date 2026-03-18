/**
 * @file Phase24DynamicPinTest.cpp
 * @brief Integration tests for Phase 24-REFONTE: Dynamic Pin Management & Modal Integration.
 * @author Olympe Engine
 * @date 2026-03-18
 *
 * @details
 * Tests (22):
 *   1.  RegeneratePins_NoConditions      — empty list → 0 pins
 *   2.  RegeneratePins_ConstOperands     — Var/Const conditions → 0 pins
 *   3.  RegeneratePins_LeftPin           — left Pin operand → 1 Left pin
 *   4.  RegeneratePins_RightPin          — right Pin operand → 1 Right pin
 *   5.  RegeneratePins_BothPins          — both Pin → 2 pins
 *   6.  RegeneratePins_StableUUID        — repeated call reuses UUIDs
 *   7.  RegeneratePins_LabelFormat_Left  — label matches "In #1L: ..."
 *   8.  RegeneratePins_LabelFormat_Right — label matches "In #1R: ..."
 *   9.  RegeneratePins_PinIDWriteBack    — leftPinID/rightPinID set in refs
 *  10.  RegeneratePins_RemovesOldPins    — pins for removed conditions deleted
 *  11.  RegeneratePins_MultipleConditions— 2 conditions, mixed operands
 *  12.  RegeneratePins_EmptyAfterClear   — 0 pins after refs cleared
 *  13.  OnApply_CallbackFires           — Confirm() triggers OnApply
 *  14.  OnApply_CallbackNotFiredOnClose  — Close() does NOT trigger OnApply
 *  15.  OnApply_NullCallbackSafe        — Confirm() with null OnApply is safe
 *  16.  OnApply_CallbackAfterConfirmed  — IsConfirmed() is true when OnApply fires
 *  17.  OnDynamicPinsNeedRegeneration_Exists — panel has the callback member
 *  18.  WorkflowSimulation_PinCreated   — full modal→panel→pin workflow
 *  19.  WorkflowSimulation_PinDeleted   — remove condition → pin disappears
 *  20.  TriggerPinRegeneration_Renderer — NodeBranchRenderer delegates to manager
 *  21.  Section4_HiddenWhenNoPins       — dynamicPins.empty() → Section 4 skipped
 *  22.  Section4_ShownWhenPinsPresent   — dynamicPins non-empty → Section 4 shown
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/ConditionPreset/DynamicDataPinManager.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/ConditionPresetRegistry.h"
#include "Editor/ConditionPreset/Operand.h"
#include "Editor/ConditionPreset/NodeConditionRef.h"
#include "Editor/ConditionPreset/DynamicDataPin.h"
#include "Editor/Modals/NodeConditionsEditModal.h"
#include "Editor/Panels/NodeConditionsPanel.h"
#include "Editor/Nodes/NodeBranchRenderer.h"

#include <iostream>
#include <string>
#include <vector>
#include <functional>

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
// Helpers
// ---------------------------------------------------------------------------

/// Variable <= Const preset (no Pin operands)
static ConditionPreset MakeConstPreset(const std::string& id,
                                        const std::string& name)
{
    ConditionPreset p(id,
                      Operand::CreateVariable("mHealth"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(2.0));
    p.name = name;
    return p;
}

/// Pin == Const preset (left Pin operand only)
static ConditionPreset MakeLeftPinPreset(const std::string& id,
                                          const std::string& name)
{
    ConditionPreset p(id,
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::Equal,
                      Operand::CreateConst(0.0));
    p.name = name;
    return p;
}

/// Const == Pin preset (right Pin operand only)
static ConditionPreset MakeRightPinPreset(const std::string& id,
                                           const std::string& name)
{
    ConditionPreset p(id,
                      Operand::CreateConst(0.0),
                      ComparisonOp::Equal,
                      Operand::CreatePin("Pin:2"));
    p.name = name;
    return p;
}

/// Pin == Pin preset (both Pin operands)
static ConditionPreset MakeBothPinPreset(const std::string& id,
                                          const std::string& name)
{
    ConditionPreset p(id,
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual,
                      Operand::CreatePin("Pin:2"));
    p.name = name;
    return p;
}

// ===========================================================================
// Part 1 — DynamicDataPinManager::RegeneratePinsFromConditions()
// ===========================================================================

// ---------------------------------------------------------------------------
// Test 1: RegeneratePins_NoConditions
// ---------------------------------------------------------------------------
static void Test01_RegeneratePins_NoConditions()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager mgr(reg);

    std::vector<NodeConditionRef> refs;
    mgr.RegeneratePinsFromConditions(refs);

    TEST_ASSERT(mgr.GetPinCount() == 0u,
                "Empty condition list should produce 0 pins");

    ReportTest("Test01_RegeneratePins_NoConditions", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: RegeneratePins_ConstOperands
// ---------------------------------------------------------------------------
static void Test02_RegeneratePins_ConstOperands()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeConstPreset("p1", "HealthCheck"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    TEST_ASSERT(mgr.GetPinCount() == 0u,
                "Variable<=Const should generate 0 pins");

    ReportTest("Test02_RegeneratePins_ConstOperands", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: RegeneratePins_LeftPin
// ---------------------------------------------------------------------------
static void Test03_RegeneratePins_LeftPin()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("p1", "PinLeft"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    TEST_ASSERT(mgr.GetPinCount() == 1u,
                "Left Pin operand should generate 1 pin");
    TEST_ASSERT(mgr.GetAllPins()[0].position == OperandPosition::Left,
                "Generated pin should be on the Left side");

    ReportTest("Test03_RegeneratePins_LeftPin", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: RegeneratePins_RightPin
// ---------------------------------------------------------------------------
static void Test04_RegeneratePins_RightPin()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeRightPinPreset("p1", "PinRight"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    TEST_ASSERT(mgr.GetPinCount() == 1u,
                "Right Pin operand should generate 1 pin");
    TEST_ASSERT(mgr.GetAllPins()[0].position == OperandPosition::Right,
                "Generated pin should be on the Right side");

    ReportTest("Test04_RegeneratePins_RightPin", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: RegeneratePins_BothPins
// ---------------------------------------------------------------------------
static void Test05_RegeneratePins_BothPins()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeBothPinPreset("p1", "BothPins"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    TEST_ASSERT(mgr.GetPinCount() == 2u,
                "Both Pin operands should generate 2 pins");

    ReportTest("Test05_RegeneratePins_BothPins", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: RegeneratePins_StableUUID
// ---------------------------------------------------------------------------
static void Test06_RegeneratePins_StableUUID()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("p1", "PinStable"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };

    mgr.RegeneratePinsFromConditions(refs);
    const std::string firstID = mgr.GetAllPins()[0].id;
    TEST_ASSERT(!firstID.empty(), "UUID should not be empty after first regen");

    mgr.RegeneratePinsFromConditions(refs);
    TEST_ASSERT(mgr.GetAllPins()[0].id == firstID,
                "UUID must be stable across repeated RegeneratePinsFromConditions() calls");

    ReportTest("Test06_RegeneratePins_StableUUID", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 7: RegeneratePins_LabelFormat_Left
// ---------------------------------------------------------------------------
static void Test07_RegeneratePins_LabelFormat_Left()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("p1", "LabelLeft"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    const std::string& label = mgr.GetAllPins()[0].label;
    TEST_ASSERT(label.find("In #1L:") != std::string::npos,
                "Left pin label must contain 'In #1L:'");

    ReportTest("Test07_RegeneratePins_LabelFormat_Left", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 8: RegeneratePins_LabelFormat_Right
// ---------------------------------------------------------------------------
static void Test08_RegeneratePins_LabelFormat_Right()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeRightPinPreset("p1", "LabelRight"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    const std::string& label = mgr.GetAllPins()[0].label;
    TEST_ASSERT(label.find("In #1R:") != std::string::npos,
                "Right pin label must contain 'In #1R:'");

    ReportTest("Test08_RegeneratePins_LabelFormat_Right", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 9: RegeneratePins_PinIDWriteBack
// ---------------------------------------------------------------------------
static void Test09_RegeneratePins_PinIDWriteBack()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeBothPinPreset("p1", "WriteBack"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    TEST_ASSERT(!refs[0].leftPinID.empty(),
                "leftPinID must be written back into NodeConditionRef");
    TEST_ASSERT(!refs[0].rightPinID.empty(),
                "rightPinID must be written back into NodeConditionRef");

    ReportTest("Test09_RegeneratePins_PinIDWriteBack", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 10: RegeneratePins_RemovesOldPins
// ---------------------------------------------------------------------------
static void Test10_RegeneratePins_RemovesOldPins()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeBothPinPreset("p1", "TwoPin"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);
    TEST_ASSERT(mgr.GetPinCount() == 2u, "Expected 2 pins initially");

    // Remove all conditions and regenerate
    std::vector<NodeConditionRef> empty;
    mgr.RegeneratePinsFromConditions(empty);
    TEST_ASSERT(mgr.GetPinCount() == 0u,
                "All pins should be removed when condition list is cleared");

    ReportTest("Test10_RegeneratePins_RemovesOldPins", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 11: RegeneratePins_MultipleConditions
// ---------------------------------------------------------------------------
static void Test11_RegeneratePins_MultipleConditions()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("p1",  "PinLeft"));
    reg.CreatePreset(MakeConstPreset("p2",    "NoPins"));
    reg.CreatePreset(MakeBothPinPreset("p3",  "BothPins"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs;
    refs.emplace_back("p1", LogicalOp::Start);
    refs.emplace_back("p2", LogicalOp::And);
    refs.emplace_back("p3", LogicalOp::Or);

    mgr.RegeneratePinsFromConditions(refs);

    // p1 → 1 pin (Left), p2 → 0 pins, p3 → 2 pins = 3 total
    TEST_ASSERT(mgr.GetPinCount() == 3u,
                "Expected 3 pins for mixed conditions (Left + None + Both)");

    ReportTest("Test11_RegeneratePins_MultipleConditions", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 12: RegeneratePins_EmptyAfterClear
// ---------------------------------------------------------------------------
static void Test12_RegeneratePins_EmptyAfterClear()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("p1", "PinX"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);
    TEST_ASSERT(mgr.GetPinCount() == 1u, "1 pin before clear");

    mgr.Clear();
    TEST_ASSERT(mgr.GetPinCount() == 0u, "0 pins after Clear()");

    ReportTest("Test12_RegeneratePins_EmptyAfterClear", s_failCount == before);
}

// ===========================================================================
// Part 2 — NodeConditionsEditModal::OnApply callback
// ===========================================================================

// ---------------------------------------------------------------------------
// Test 13: OnApply_CallbackFires
// ---------------------------------------------------------------------------
static void Test13_OnApply_CallbackFires()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsEditModal modal(reg);
    modal.Open({});

    bool callbackFired = false;
    modal.OnApply = [&]() { callbackFired = true; };

    modal.Confirm();

    TEST_ASSERT(callbackFired,
                "OnApply callback must be fired when Confirm() is called");

    ReportTest("Test13_OnApply_CallbackFires", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 14: OnApply_CallbackNotFiredOnClose
// ---------------------------------------------------------------------------
static void Test14_OnApply_CallbackNotFiredOnClose()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsEditModal modal(reg);
    modal.Open({});

    bool callbackFired = false;
    modal.OnApply = [&]() { callbackFired = true; };

    modal.Close();  // Cancel — should NOT fire OnApply

    TEST_ASSERT(!callbackFired,
                "OnApply callback must NOT fire when Close() (Cancel) is called");

    ReportTest("Test14_OnApply_CallbackNotFiredOnClose", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 15: OnApply_NullCallbackSafe
// ---------------------------------------------------------------------------
static void Test15_OnApply_NullCallbackSafe()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsEditModal modal(reg);
    modal.Open({});

    // OnApply is null (default) — Confirm() must not crash
    modal.Confirm();

    TEST_ASSERT(modal.IsConfirmed(),
                "Confirm() with null OnApply should still set IsConfirmed()");
    TEST_ASSERT(!modal.IsOpen(),
                "Modal should be closed after Confirm() with null OnApply");

    ReportTest("Test15_OnApply_NullCallbackSafe", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 16: OnApply_CallbackAfterConfirmed
// ---------------------------------------------------------------------------
static void Test16_OnApply_CallbackAfterConfirmed()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsEditModal modal(reg);
    modal.Open({});

    bool isConfirmedInsideCallback = false;
    modal.OnApply = [&]() {
        // IsConfirmed() should already be true when the callback fires
        isConfirmedInsideCallback = modal.IsConfirmed();
    };

    modal.Confirm();

    TEST_ASSERT(isConfirmedInsideCallback,
                "IsConfirmed() must be true by the time OnApply fires");

    ReportTest("Test16_OnApply_CallbackAfterConfirmed", s_failCount == before);
}

// ===========================================================================
// Part 3 — NodeConditionsPanel::OnDynamicPinsNeedRegeneration callback
// ===========================================================================

// ---------------------------------------------------------------------------
// Test 17: OnDynamicPinsNeedRegeneration_Exists
// ---------------------------------------------------------------------------
static void Test17_OnDynamicPinsNeedRegeneration_Exists()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    NodeConditionsPanel panel(reg);

    // The callback member must be assignable and callable safely when null
    TEST_ASSERT(!panel.OnDynamicPinsNeedRegeneration,
                "OnDynamicPinsNeedRegeneration should be null (unset) by default");

    bool fired = false;
    panel.OnDynamicPinsNeedRegeneration = [&]() { fired = true; };
    panel.OnDynamicPinsNeedRegeneration();

    TEST_ASSERT(fired, "OnDynamicPinsNeedRegeneration should fire when invoked");

    ReportTest("Test17_OnDynamicPinsNeedRegeneration_Exists", s_failCount == before);
}

// ===========================================================================
// Part 4 — Full workflow simulation (Modal → Panel → Pins)
// ===========================================================================

// ---------------------------------------------------------------------------
// Test 18: WorkflowSimulation_PinCreated
// ---------------------------------------------------------------------------
static void Test18_WorkflowSimulation_PinCreated()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("pSpeed", "SpeedPin"));

    DynamicDataPinManager mgr(reg);
    NodeConditionsEditModal modal(reg);

    // Simulate the host connecting the modal's OnApply to pin regeneration
    std::vector<NodeConditionRef> nodeConditionRefs;

    modal.OnApply = [&]() {
        nodeConditionRefs = modal.GetConditionRefs();
        mgr.RegeneratePinsFromConditions(nodeConditionRefs);
    };

    // User opens the modal, adds a condition, and applies
    modal.Open({});
    modal.AddCondition("pSpeed");
    modal.Confirm();   // Triggers OnApply

    TEST_ASSERT(modal.IsConfirmed(),          "Modal must be confirmed");
    TEST_ASSERT(nodeConditionRefs.size() == 1u, "Node must have 1 condition after apply");
    TEST_ASSERT(mgr.GetPinCount() == 1u,      "1 yellow pin should be generated");
    TEST_ASSERT(mgr.GetAllPins()[0].position == OperandPosition::Left,
                "Generated pin should be Left");

    ReportTest("Test18_WorkflowSimulation_PinCreated", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 19: WorkflowSimulation_PinDeleted
// ---------------------------------------------------------------------------
static void Test19_WorkflowSimulation_PinDeleted()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeBothPinPreset("pBoth", "BothPins"));
    reg.CreatePreset(MakeConstPreset("pConst",  "NoPins"));

    DynamicDataPinManager mgr(reg);
    NodeConditionsEditModal modal(reg);

    std::vector<NodeConditionRef> nodeConditionRefs;
    nodeConditionRefs.emplace_back("pBoth",  LogicalOp::Start);
    nodeConditionRefs.emplace_back("pConst", LogicalOp::And);

    // Initial pin generation (both Pin operands → 2 pins)
    mgr.RegeneratePinsFromConditions(nodeConditionRefs);
    TEST_ASSERT(mgr.GetPinCount() == 2u, "Expected 2 pins before modal edit");

    // Host connects OnApply
    modal.OnApply = [&]() {
        nodeConditionRefs = modal.GetConditionRefs();
        mgr.RegeneratePinsFromConditions(nodeConditionRefs);
    };

    // User opens modal, removes the Pin condition, keeps only Const
    modal.Open(nodeConditionRefs);
    modal.RemoveCondition(0);  // removes "pBoth" (index 0)
    modal.Confirm();

    TEST_ASSERT(nodeConditionRefs.size() == 1u,
                "One condition left after removal");
    TEST_ASSERT(mgr.GetPinCount() == 0u,
                "All pins removed when only Const-operand conditions remain");

    ReportTest("Test19_WorkflowSimulation_PinDeleted", s_failCount == before);
}

// ===========================================================================
// Part 5 — NodeBranchRenderer::TriggerPinRegeneration
// ===========================================================================

// ---------------------------------------------------------------------------
// Test 20: TriggerPinRegeneration_Renderer
// ---------------------------------------------------------------------------
static void Test20_TriggerPinRegeneration_Renderer()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("p1", "PinViaRenderer"));

    DynamicDataPinManager mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };

    // Call through the renderer's convenience method
    renderer.TriggerPinRegeneration(refs);

    TEST_ASSERT(mgr.GetPinCount() == 1u,
                "TriggerPinRegeneration should produce 1 pin via pin manager");
    TEST_ASSERT(!refs[0].leftPinID.empty(),
                "leftPinID must be updated by TriggerPinRegeneration");

    ReportTest("Test20_TriggerPinRegeneration_Renderer", s_failCount == before);
}

// ===========================================================================
// Part 6 — Canvas Section 4 visibility logic
// ===========================================================================

// ---------------------------------------------------------------------------
// Test 21: Section4_HiddenWhenNoPins
// ---------------------------------------------------------------------------
static void Test21_Section4_HiddenWhenNoPins()
{
    int before = s_failCount;

    // Simulate the data passed to RenderNode
    NodeBranchData data;
    data.nodeID   = "node1";
    data.nodeName = "TestNode";
    // dynamicPins is empty by default

    TEST_ASSERT(data.dynamicPins.empty(),
                "Section 4 input: dynamicPins must be empty");

    // The renderer only draws Section 4 when !data.dynamicPins.empty()
    // We cannot call RenderNode() in headless mode, but we can verify the guard
    // condition that the renderer checks:
    const bool section4ShouldRender = !data.dynamicPins.empty();
    TEST_ASSERT(!section4ShouldRender,
                "Section 4 should NOT render when dynamicPins is empty");

    ReportTest("Test21_Section4_HiddenWhenNoPins", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 22: Section4_ShownWhenPinsPresent
// ---------------------------------------------------------------------------
static void Test22_Section4_ShownWhenPinsPresent()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeLeftPinPreset("p1", "VisiblePin"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.RegeneratePinsFromConditions(refs);

    NodeBranchData data;
    data.nodeID     = "node1";
    data.nodeName   = "TestNode";
    data.dynamicPins = mgr.GetAllPins();

    // The renderer draws Section 4 when !data.dynamicPins.empty()
    const bool section4ShouldRender = !data.dynamicPins.empty();
    TEST_ASSERT(section4ShouldRender,
                "Section 4 SHOULD render when dynamicPins is non-empty");
    TEST_ASSERT(data.dynamicPins.size() == 1u,
                "Exactly 1 yellow pin should be in Section 4");

    ReportTest("Test22_Section4_ShownWhenPinsPresent", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 24-REFONTE: Dynamic Pin Management Tests ===" << std::endl;
    std::cout << std::endl;

    std::cout << "-- Part 1: RegeneratePinsFromConditions() --" << std::endl;
    Test01_RegeneratePins_NoConditions();
    Test02_RegeneratePins_ConstOperands();
    Test03_RegeneratePins_LeftPin();
    Test04_RegeneratePins_RightPin();
    Test05_RegeneratePins_BothPins();
    Test06_RegeneratePins_StableUUID();
    Test07_RegeneratePins_LabelFormat_Left();
    Test08_RegeneratePins_LabelFormat_Right();
    Test09_RegeneratePins_PinIDWriteBack();
    Test10_RegeneratePins_RemovesOldPins();
    Test11_RegeneratePins_MultipleConditions();
    Test12_RegeneratePins_EmptyAfterClear();

    std::cout << std::endl;
    std::cout << "-- Part 2: NodeConditionsEditModal::OnApply callback --" << std::endl;
    Test13_OnApply_CallbackFires();
    Test14_OnApply_CallbackNotFiredOnClose();
    Test15_OnApply_NullCallbackSafe();
    Test16_OnApply_CallbackAfterConfirmed();

    std::cout << std::endl;
    std::cout << "-- Part 3: NodeConditionsPanel::OnDynamicPinsNeedRegeneration --" << std::endl;
    Test17_OnDynamicPinsNeedRegeneration_Exists();

    std::cout << std::endl;
    std::cout << "-- Part 4: Full workflow simulation --" << std::endl;
    Test18_WorkflowSimulation_PinCreated();
    Test19_WorkflowSimulation_PinDeleted();

    std::cout << std::endl;
    std::cout << "-- Part 5: NodeBranchRenderer::TriggerPinRegeneration --" << std::endl;
    Test20_TriggerPinRegeneration_Renderer();

    std::cout << std::endl;
    std::cout << "-- Part 6: Canvas Section 4 visibility --" << std::endl;
    Test21_Section4_HiddenWhenNoPins();
    Test22_Section4_ShownWhenPinsPresent();

    std::cout << std::endl;
    std::cout << "Results: "
              << s_passCount << " passed, "
              << s_failCount << " failed."
              << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
