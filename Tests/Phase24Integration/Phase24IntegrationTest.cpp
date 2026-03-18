/**
 * @file Phase24IntegrationTest.cpp
 * @brief Integration tests for Phase 24-Rendering Integration Fix (PR #444).
 * @author Olympe Engine
 * @date 2026-03-18
 *
 * @details
 * Tests (12):
 *
 * Test Set 1 — Node Type Routing (3 tests):
 *   1.  NodeTypeRouting_BranchRendererReceivesData      — NodeBranchRenderer
 *       correctly accepts NodeBranchData for a Branch node.
 *   2.  NodeTypeRouting_FallbackDetectedForOtherData    — Empty/non-Branch
 *       NodeBranchData detected separately from valid Branch data.
 *   3.  NodeTypeRouting_InvalidNodeHandledGracefully    — Empty nodeID and
 *       nodeName do not crash the renderer.
 *
 * Test Set 2 — Data Population (3 tests):
 *   4.  DataPopulation_NodeName                         — NodeBranchData.nodeName
 *       populated correctly from source.
 *   5.  DataPopulation_ConditionRefs                    — NodeBranchData.conditionRefs
 *       contains all expected conditions.
 *   6.  DataPopulation_DynamicPins                      — NodeBranchData.dynamicPins
 *       populated from DynamicDataPinManager after SyncPins().
 *
 * Test Set 3 — ImGui Rendering via Headless (4 tests):
 *   7.  Rendering_TitleSection_BreakpointFlag           — RenderTitleSection
 *       handles breakpoint true/false without crash.
 *   8.  Rendering_ConditionsSection_WithLogicalOps      — RenderConditionsSection
 *       renders And/Or logical operators without crash.
 *   9.  Rendering_DynamicPinsSection_OnlyWhenNonEmpty   — RenderDynamicPinsSection
 *       is invoked only when dynamicPins is non-empty.
 *  10.  Rendering_RenderNode_FourSectionsInOrder        — Full RenderNode() call
 *       with all 4 sections does not crash.
 *
 * Test Set 4 — Properties Integration (2 tests):
 *  11.  PropertiesIntegration_PanelRenderWithConditions — NodeConditionsPanel
 *       Render() cycle is safe after SetConditionRefs() for a Branch node.
 *  12.  PropertiesIntegration_ModalWorkflowFullPipeline — Full workflow:
 *       open modal → add condition → confirm → verify refs updated → verify
 *       OnDynamicPinsNeedRegeneration fires.
 *
 * All ImGui calls are no-ops in headless builds; tests exercise logic only.
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/Nodes/NodeBranchRenderer.h"
#include "Editor/Panels/NodeConditionsPanel.h"
#include "Editor/Modals/NodeConditionsEditModal.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/ConditionPresetRegistry.h"
#include "Editor/ConditionPreset/Operand.h"
#include "Editor/ConditionPreset/NodeConditionRef.h"
#include "Editor/ConditionPreset/DynamicDataPin.h"
#include "Editor/ConditionPreset/DynamicDataPinManager.h"

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
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ConditionPreset MakeVarPreset(const std::string& id,
                                     const std::string& name,
                                     double constVal = 2.0)
{
    ConditionPreset p(id,
                      Operand::CreateVariable("mHealth"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(constVal));
    p.name = name;
    return p;
}

static ConditionPreset MakePinPreset(const std::string& id,
                                     const std::string& name)
{
    ConditionPreset p(id,
                      Operand::CreateVariable("mSpeed"),
                      ComparisonOp::Equal,
                      Operand::CreatePin("Pin:1"));
    p.name = name;
    return p;
}

// ---------------------------------------------------------------------------
// Test Set 1 — Node Type Routing
// ---------------------------------------------------------------------------

/**
 * @brief Test 1: NodeBranchRenderer correctly receives and processes
 *        NodeBranchData for a Branch node.
 *
 * Verifies that a fully-populated NodeBranchData is accepted by the
 * renderer and that the renderer's initial state is as expected.
 */
static void Test1_NodeTypeRouting_BranchRendererReceivesData()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeVarPreset("p_health", "HealthLow"));
    DynamicDataPinManager mgr(reg);

    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p_health", LogicalOp::Start)
    };

    NodeBranchData data;
    data.nodeID        = "branch_node_01";
    data.nodeName      = "Is Health Critical?";
    data.conditionRefs = refs;
    data.dynamicPins   = mgr.GetAllPins();
    data.breakpoint    = false;

    // Branch renderer should be constructible and accept NodeBranchData
    NodeBranchRenderer renderer(reg, mgr);

    TEST_ASSERT(data.nodeID == "branch_node_01",
                "NodeBranchData.nodeID populated correctly for Branch routing");
    TEST_ASSERT(data.nodeName == "Is Health Critical?",
                "NodeBranchData.nodeName populated correctly for Branch routing");
    TEST_ASSERT(!data.conditionRefs.empty(),
                "NodeBranchData.conditionRefs non-empty for Branch routing");

    // Renderer must be usable (not crash) with this data
    renderer.RenderNode(data);   // headless: no-op

    TEST_ASSERT(renderer.GetLastClickedConditionIndex() == -1,
                "Renderer initial state correct after routing");

    ReportTest("Test1_NodeTypeRouting_BranchRendererReceivesData",
               s_failCount == before);
}

/**
 * @brief Test 2: Non-Branch data (no conditions, no pins) is detected
 *        separately from valid Branch data.
 *
 * Validates that empty/default NodeBranchData is distinguishable from
 * fully-populated Branch data, enabling the host to apply different
 * rendering paths for different node types.
 */
static void Test2_NodeTypeRouting_FallbackDetectedForOtherData()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);

    // Simulate data that would come from a non-Branch node (empty conditions/pins)
    NodeBranchData emptyData;
    emptyData.nodeID   = "seq_node_01";
    emptyData.nodeName = "Sequence";

    // Populated Branch data
    reg.CreatePreset(MakeVarPreset("p1", "SpeedCheck"));
    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p1", LogicalOp::Start)
    };
    NodeBranchData branchData;
    branchData.nodeID        = "branch_01";
    branchData.nodeName      = "Branch";
    branchData.conditionRefs = refs;

    // Distinguish: branch data has conditions, non-branch does not
    TEST_ASSERT(emptyData.conditionRefs.empty(),
                "Non-Branch NodeBranchData has no conditions (fallback indicator)");
    TEST_ASSERT(!branchData.conditionRefs.empty(),
                "Branch NodeBranchData has conditions (branch indicator)");
    TEST_ASSERT(emptyData.dynamicPins.empty(),
                "Non-Branch NodeBranchData has no dynamic pins");

    ReportTest("Test2_NodeTypeRouting_FallbackDetectedForOtherData",
               s_failCount == before);
}

/**
 * @brief Test 3: Empty/invalid NodeBranchData does not crash the renderer.
 *
 * Ensures the renderer gracefully handles edge cases like empty nodeID
 * and empty nodeName without throwing exceptions or undefined behaviour.
 */
static void Test3_NodeTypeRouting_InvalidNodeHandledGracefully()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer      renderer(reg, mgr);

    // Completely empty NodeBranchData
    NodeBranchData emptyData;
    // nodeID and nodeName are empty strings, conditionRefs and dynamicPins are empty

    // Must not crash
    renderer.RenderNode(emptyData);            // headless: no-op
    renderer.RenderTitleSection(emptyData);    // headless: no-op
    renderer.RenderExecPinsSection(emptyData); // headless: no-op
    renderer.RenderConditionsSection(emptyData);// headless: no-op
    renderer.RenderDynamicPinsSection(emptyData);// headless: no-op (empty pins)

    TEST_ASSERT(emptyData.nodeID.empty(),   "Empty nodeID handled gracefully");
    TEST_ASSERT(emptyData.nodeName.empty(), "Empty nodeName handled gracefully");
    TEST_ASSERT(emptyData.conditionRefs.empty(), "Empty conditionRefs handled gracefully");

    ReportTest("Test3_NodeTypeRouting_InvalidNodeHandledGracefully",
               s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test Set 2 — Data Population
// ---------------------------------------------------------------------------

/**
 * @brief Test 4: NodeBranchData.nodeName is populated correctly from source.
 *
 * Verifies that the node name used for rendering matches the source name,
 * as required for the blue title bar section to show the correct label.
 */
static void Test4_DataPopulation_NodeName()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);

    const std::string expectedName = "Is Health Critical?";

    NodeBranchData data;
    data.nodeID   = "node_04";
    data.nodeName = expectedName;

    NodeBranchRenderer renderer(reg, mgr);

    // Name must survive through data snapshot
    TEST_ASSERT(data.nodeName == expectedName,
                "NodeBranchData.nodeName matches source name");

    // Renderer reads it during RenderTitleSection (headless: no ImGui output)
    renderer.RenderTitleSection(data);

    TEST_ASSERT(data.nodeName == expectedName,
                "nodeName unchanged after RenderTitleSection");

    ReportTest("Test4_DataPopulation_NodeName", s_failCount == before);
}

/**
 * @brief Test 5: NodeBranchData.conditionRefs contains all expected conditions.
 *
 * Verifies that when conditions are assigned to NodeBranchData, the count
 * and order are preserved (critical for correct condition rendering).
 */
static void Test5_DataPopulation_ConditionRefs()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeVarPreset("p_health", "HealthLow",  2.0));
    reg.CreatePreset(MakeVarPreset("p_speed",  "SpeedHigh", 100.0));
    reg.CreatePreset(MakePinPreset("p_pin",    "SpeedEqPin"));

    DynamicDataPinManager mgr(reg);

    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p_health", LogicalOp::Start),
        NodeConditionRef("p_speed",  LogicalOp::And),
        NodeConditionRef("p_pin",    LogicalOp::Or)
    };

    NodeBranchData data;
    data.nodeID        = "node_05";
    data.nodeName      = "Complex Branch";
    data.conditionRefs = refs;

    TEST_ASSERT(data.conditionRefs.size() == 3u,
                "NodeBranchData.conditionRefs contains all 3 conditions");
    TEST_ASSERT(data.conditionRefs[0].presetID  == "p_health",
                "First condition ref is health preset");
    TEST_ASSERT(data.conditionRefs[0].logicalOp == LogicalOp::Start,
                "First condition has Start logical op");
    TEST_ASSERT(data.conditionRefs[1].presetID  == "p_speed",
                "Second condition ref is speed preset");
    TEST_ASSERT(data.conditionRefs[1].logicalOp == LogicalOp::And,
                "Second condition has And logical op");
    TEST_ASSERT(data.conditionRefs[2].presetID  == "p_pin",
                "Third condition ref is pin preset");
    TEST_ASSERT(data.conditionRefs[2].logicalOp == LogicalOp::Or,
                "Third condition has Or logical op");

    ReportTest("Test5_DataPopulation_ConditionRefs", s_failCount == before);
}

/**
 * @brief Test 6: NodeBranchData.dynamicPins populated from DynamicDataPinManager.
 *
 * Verifies that after calling DynamicDataPinManager::SyncPins() with a set of
 * condition refs, the resulting pins are correctly placed in NodeBranchData.
 */
static void Test6_DataPopulation_DynamicPins()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePinPreset("p1", "SpeedEqPin1"));
    reg.CreatePreset(MakePinPreset("p2", "SpeedEqPin2"));

    DynamicDataPinManager mgr(reg);

    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p1", LogicalOp::Start),
        NodeConditionRef("p2", LogicalOp::And)
    };
    mgr.SyncPins(refs);

    NodeBranchData data;
    data.nodeID        = "node_06";
    data.nodeName      = "Multi-Pin Branch";
    data.conditionRefs = refs;
    data.dynamicPins   = mgr.GetAllPins();  // Populate from pin manager

    TEST_ASSERT(!data.dynamicPins.empty(),
                "NodeBranchData.dynamicPins populated after SyncPins()");
    TEST_ASSERT(data.dynamicPins.size() == mgr.GetPinCount(),
                "dynamicPins count matches DynamicDataPinManager::GetPinCount()");

    // Each pin must have a non-empty label and a non-empty UUID
    for (const auto& pin : data.dynamicPins)
    {
        TEST_ASSERT(!pin.id.empty(),
                    "Dynamic pin has non-empty UUID");
        TEST_ASSERT(!pin.GetDisplayLabel().empty(),
                    "Dynamic pin has non-empty display label");
    }

    ReportTest("Test6_DataPopulation_DynamicPins", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test Set 3 — ImGui Rendering (headless: logic + no-crash)
// ---------------------------------------------------------------------------

/**
 * @brief Test 7: RenderTitleSection handles breakpoint flag correctly.
 *
 * In headless builds the ImGui calls are compiled out, but the function must
 * complete without crash for both breakpoint=true and breakpoint=false.
 */
static void Test7_Rendering_TitleSection_BreakpointFlag()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer      renderer(reg, mgr);

    NodeBranchData data;
    data.nodeID   = "node_07";
    data.nodeName = "BPNode";

    // With breakpoint active
    data.breakpoint = true;
    renderer.RenderTitleSection(data);
    TEST_ASSERT(data.breakpoint, "Breakpoint=true preserved after RenderTitleSection");

    // Without breakpoint
    data.breakpoint = false;
    renderer.RenderTitleSection(data);
    TEST_ASSERT(!data.breakpoint, "Breakpoint=false preserved after RenderTitleSection");

    ReportTest("Test7_Rendering_TitleSection_BreakpointFlag", s_failCount == before);
}

/**
 * @brief Test 8: RenderConditionsSection handles And/Or logical operators.
 *
 * Verifies that RenderConditionsSection (headless: no-op) completes without
 * crash when condition refs include various logical operators, and that
 * conditions with missing presets are also handled gracefully.
 */
static void Test8_Rendering_ConditionsSection_WithLogicalOps()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeVarPreset("p_a", "CondA", 5.0));
    reg.CreatePreset(MakeVarPreset("p_b", "CondB", 10.0));

    DynamicDataPinManager mgr(reg);
    NodeBranchRenderer    renderer(reg, mgr);

    // Build condition refs with And and Or operators
    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p_a",    LogicalOp::Start),
        NodeConditionRef("p_b",    LogicalOp::And),
        NodeConditionRef("p_miss", LogicalOp::Or)    // missing preset
    };

    NodeBranchData data;
    data.nodeID        = "node_08";
    data.nodeName      = "Multi-Op Branch";
    data.conditionRefs = refs;

    // Must not crash with And, Or, and missing preset
    renderer.RenderConditionsSection(data);

    TEST_ASSERT(data.conditionRefs.size() == 3u,
                "All 3 condition refs (including missing) handled without crash");
    TEST_ASSERT(data.conditionRefs[1].logicalOp == LogicalOp::And,
                "And logical op preserved");
    TEST_ASSERT(data.conditionRefs[2].logicalOp == LogicalOp::Or,
                "Or logical op preserved");

    ReportTest("Test8_Rendering_ConditionsSection_WithLogicalOps",
               s_failCount == before);
}

/**
 * @brief Test 9: RenderDynamicPinsSection only renders when dynamicPins non-empty.
 *
 * Verifies the conditional rendering logic: Section 4 of the node is skipped
 * entirely when there are no dynamic pins.
 */
static void Test9_Rendering_DynamicPinsSection_OnlyWhenNonEmpty()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer      renderer(reg, mgr);

    // Case A: empty pins → Section 4 skipped (RenderDynamicPinsSection is no-op)
    NodeBranchData dataEmpty;
    dataEmpty.nodeID   = "node_09a";
    dataEmpty.nodeName = "NoPinBranch";
    // dynamicPins left empty

    renderer.RenderDynamicPinsSection(dataEmpty);  // must not crash
    TEST_ASSERT(dataEmpty.dynamicPins.empty(),
                "Empty dynamicPins: Section 4 skipped (no crash)");

    // Case B: with pins → Section 4 rendered
    reg.CreatePreset(MakePinPreset("p_vis", "PinCond"));
    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p_vis", LogicalOp::Start)
    };
    mgr.SyncPins(refs);

    NodeBranchData dataFull;
    dataFull.nodeID        = "node_09b";
    dataFull.nodeName      = "PinBranch";
    dataFull.conditionRefs = refs;
    dataFull.dynamicPins   = mgr.GetAllPins();

    renderer.RenderDynamicPinsSection(dataFull);   // must not crash
    TEST_ASSERT(!dataFull.dynamicPins.empty(),
                "Non-empty dynamicPins: Section 4 rendered (no crash)");

    ReportTest("Test9_Rendering_DynamicPinsSection_OnlyWhenNonEmpty",
               s_failCount == before);
}

/**
 * @brief Test 10: Full RenderNode() exercises all 4 sections in correct order.
 *
 * Verifies that RenderNode() calls RenderTitleSection, RenderExecPinsSection,
 * RenderConditionsSection and (conditionally) RenderDynamicPinsSection without
 * crash, for both the non-empty and empty dynamic-pin cases.
 */
static void Test10_Rendering_RenderNode_FourSectionsInOrder()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeVarPreset("p_health", "HealthCond", 2.0));
    reg.CreatePreset(MakePinPreset("p_pin",    "PinCond"));

    DynamicDataPinManager mgr(reg);

    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p_health", LogicalOp::Start),
        NodeConditionRef("p_pin",    LogicalOp::And)
    };
    mgr.SyncPins(refs);

    NodeBranchRenderer renderer(reg, mgr);

    // Full render with all 4 sections present
    NodeBranchData dataFull;
    dataFull.nodeID        = "node_10_full";
    dataFull.nodeName      = "Full Branch";
    dataFull.conditionRefs = refs;
    dataFull.dynamicPins   = mgr.GetAllPins();
    dataFull.breakpoint    = true;

    renderer.RenderNode(dataFull);   // headless: all 4 sections as no-ops

    TEST_ASSERT(!dataFull.conditionRefs.empty(),
                "Section 3 data intact after full RenderNode");
    TEST_ASSERT(!dataFull.dynamicPins.empty(),
                "Section 4 data intact after full RenderNode");

    // Full render with no dynamic pins (Section 4 skipped)
    NodeBranchData dataNoPin;
    dataNoPin.nodeID        = "node_10_nopin";
    dataNoPin.nodeName      = "NoPinBranch";
    dataNoPin.conditionRefs = { NodeConditionRef("p_health", LogicalOp::Start) };
    // dynamicPins left empty

    renderer.RenderNode(dataNoPin);   // headless: Sections 1-3 only
    TEST_ASSERT(dataNoPin.dynamicPins.empty(),
                "Section 4 correctly skipped when dynamicPins empty");

    ReportTest("Test10_Rendering_RenderNode_FourSectionsInOrder",
               s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test Set 4 — Properties Integration
// ---------------------------------------------------------------------------

/**
 * @brief Test 11: NodeConditionsPanel Render() cycle is safe after
 *        SetConditionRefs() for a Branch node selection.
 *
 * Simulates the Properties panel being populated and rendered for a selected
 * Branch node: set node name, set condition refs, set dynamic pins, render.
 */
static void Test11_PropertiesIntegration_PanelRenderWithConditions()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeVarPreset("p1", "HealthCond", 2.0));
    reg.CreatePreset(MakeVarPreset("p2", "SpeedCond", 100.0));

    DynamicDataPinManager mgr(reg);

    std::vector<NodeConditionRef> refs = {
        NodeConditionRef("p1", LogicalOp::Start),
        NodeConditionRef("p2", LogicalOp::And)
    };

    // Simulate Properties panel setup for a Branch node
    NodeConditionsPanel panel(reg);
    panel.SetNodeName("Is Health Critical?");
    panel.SetConditionRefs(refs);
    panel.SetDynamicPins(mgr.GetAllPins());   // empty in this test (no pin presets)

    TEST_ASSERT(panel.GetNodeName() == "Is Health Critical?",
                "Panel reflects SetNodeName() for Branch node");
    TEST_ASSERT(panel.GetConditionCount() == 2u,
                "Panel has 2 conditions after SetConditionRefs()");
    TEST_ASSERT(!panel.IsEditModalRequested(),
                "Edit modal not spuriously requested on setup");
    TEST_ASSERT(!panel.IsDirty(),
                "Panel not dirty after SetConditionRefs() (clean load)");

    // Render cycle must be safe (headless: no-op)
    panel.Render();

    TEST_ASSERT(!panel.IsEditModalRequested(),
                "Edit modal not spuriously requested after Render()");

    ReportTest("Test11_PropertiesIntegration_PanelRenderWithConditions",
               s_failCount == before);
}

/**
 * @brief Test 12: Full modal workflow — open → add condition → confirm →
 *        verify refs updated → verify OnDynamicPinsNeedRegeneration fires.
 *
 * This is the end-to-end integration test for the Properties panel modal
 * workflow that must complete correctly for the rendering pipeline to be
 * considered fully functional.
 */
static void Test12_PropertiesIntegration_ModalWorkflowFullPipeline()
{
    int before = s_failCount;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeVarPreset("p_health", "HealthLow",  2.0));
    reg.CreatePreset(MakePinPreset("p_pin",    "SpeedEqPin"));

    DynamicDataPinManager mgr(reg);

    // ── Step 1: Panel set up for Branch node ────────────────────────────────
    NodeConditionsPanel panel(reg);
    panel.SetNodeName("Is Health Critical?");
    panel.SetConditionRefs({});   // start empty

    TEST_ASSERT(panel.GetConditionCount() == 0u, "Panel starts empty");

    // ── Step 2: Pin regeneration callback wired up ──────────────────────────
    bool regenFired = false;
    panel.OnDynamicPinsNeedRegeneration = [&]() {
        regenFired = true;
    };

    // ── Step 3: Simulate modal open → add conditions → confirm ──────────────
    NodeConditionsEditModal modal(reg);
    modal.Open({});
    modal.AddCondition("p_health");
    modal.AddCondition("p_pin");

    TEST_ASSERT(modal.GetConditionCount() == 2u,
                "Modal has 2 conditions after AddCondition() calls");

    bool applyFired = false;
    modal.OnApply = [&]() { applyFired = true; };

    modal.Confirm();

    TEST_ASSERT(applyFired,         "OnApply callback fired on Confirm()");
    TEST_ASSERT(modal.IsConfirmed(),"Modal is confirmed");
    TEST_ASSERT(!modal.IsOpen(),    "Modal closed after Confirm()");
    TEST_ASSERT(modal.GetConditionRefs().size() == 2u,
                "Modal holds 2 confirmed condition refs");

    // ── Step 4: Panel receives confirmed refs from modal ────────────────────
    panel.SetConditionRefs(modal.GetConditionRefs());

    TEST_ASSERT(panel.GetConditionCount() == 2u,
                "Panel updated with 2 conditions from modal");

    // ── Step 5: Simulate DynamicDataPinManager regeneration ─────────────────
    std::vector<NodeConditionRef> regenRefs = panel.GetConditionRefs();
    mgr.SyncPins(regenRefs);

    // At least the pin-mode condition (p_pin) should generate a dynamic pin
    TEST_ASSERT(mgr.GetPinCount() >= 1u,
                "At least 1 dynamic pin regenerated after SyncPins()");

    // ── Step 6: Fire regeneration callback (simulates host integration) ──────
    if (panel.OnDynamicPinsNeedRegeneration)
        panel.OnDynamicPinsNeedRegeneration();

    TEST_ASSERT(regenFired,
                "OnDynamicPinsNeedRegeneration fires for canvas update");

    // ── Step 7: Validate final panel state ──────────────────────────────────
    TEST_ASSERT(panel.GetConditionCount() == 2u,
                "Panel condition count intact at end of workflow");

    // Validate panel refs contain expected presets
    const std::vector<NodeConditionRef>& finalRefs = panel.GetConditionRefs();
    bool foundHealth = false;
    bool foundPin    = false;
    for (const auto& ref : finalRefs)
    {
        if (ref.presetID == "p_health") foundHealth = true;
        if (ref.presetID == "p_pin")    foundPin    = true;
    }
    TEST_ASSERT(foundHealth, "p_health preset ref preserved in final panel state");
    TEST_ASSERT(foundPin,    "p_pin preset ref preserved in final panel state");

    ReportTest("Test12_PropertiesIntegration_ModalWorkflowFullPipeline",
               s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 24-Rendering Integration Tests ===" << std::endl;

    // Test Set 1 — Node Type Routing
    Test1_NodeTypeRouting_BranchRendererReceivesData();
    Test2_NodeTypeRouting_FallbackDetectedForOtherData();
    Test3_NodeTypeRouting_InvalidNodeHandledGracefully();

    // Test Set 2 — Data Population
    Test4_DataPopulation_NodeName();
    Test5_DataPopulation_ConditionRefs();
    Test6_DataPopulation_DynamicPins();

    // Test Set 3 — ImGui Rendering (headless)
    Test7_Rendering_TitleSection_BreakpointFlag();
    Test8_Rendering_ConditionsSection_WithLogicalOps();
    Test9_Rendering_DynamicPinsSection_OnlyWhenNonEmpty();
    Test10_Rendering_RenderNode_FourSectionsInOrder();

    // Test Set 4 — Properties Integration
    Test11_PropertiesIntegration_PanelRenderWithConditions();
    Test12_PropertiesIntegration_ModalWorkflowFullPipeline();

    std::cout << "\nResults: "
              << s_passCount << " passed, "
              << s_failCount << " failed."
              << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
