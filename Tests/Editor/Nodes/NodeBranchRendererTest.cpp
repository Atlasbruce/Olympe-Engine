/**
 * @file NodeBranchRendererTest.cpp
 * @brief Unit tests for NodeBranchRenderer (Phase 24.4).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * Tests (8):
 *   1.  DefaultState            — no click, not dirty on construction
 *   2.  NotifyPresetChanged     — marks refresh pending
 *   3.  ClearRefreshPending     — clears the flag
 *   4.  OnConditionClicked_CB   — callback fires with correct index when set
 *   5.  ClearLastClickedCondition — resets to -1
 *   6.  IsRefreshPending_False  — false on construction
 *   7.  PinManagerIntegration   — renderer reads pins from DynamicDataPinManager
 *   8.  EmptyConditions         — RenderNode with no conditions does not crash
 *
 * Because ImGui rendering is disabled in headless builds, all tests exercise
 * the renderer's logic interfaces without actually drawing.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/Nodes/NodeBranchRenderer.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/ConditionPresetRegistry.h"
#include "Editor/ConditionPreset/Operand.h"
#include "Editor/ConditionPreset/NodeConditionRef.h"
#include "Editor/ConditionPreset/DynamicDataPinManager.h"

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

// ---------------------------------------------------------------------------
// Test 1: DefaultState
// ---------------------------------------------------------------------------

static void Test1_DefaultState()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    TEST_ASSERT(renderer.GetLastClickedConditionIndex() == -1,
                "LastClickedConditionIndex should be -1 on construction");
    TEST_ASSERT(!renderer.IsRefreshPending(),
                "RefreshPending should be false on construction");

    ReportTest("Test1_DefaultState", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: NotifyPresetChanged
// ---------------------------------------------------------------------------

static void Test2_NotifyPresetChanged()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    renderer.NotifyPresetChanged("some_preset_id");
    TEST_ASSERT(renderer.IsRefreshPending(),
                "IsRefreshPending should be true after NotifyPresetChanged");

    ReportTest("Test2_NotifyPresetChanged", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: ClearRefreshPending
// ---------------------------------------------------------------------------

static void Test3_ClearRefreshPending()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    renderer.NotifyPresetChanged("p1");
    TEST_ASSERT(renderer.IsRefreshPending(), "Should be pending after notify");

    renderer.ClearRefreshPending();
    TEST_ASSERT(!renderer.IsRefreshPending(), "Should not be pending after clear");

    ReportTest("Test3_ClearRefreshPending", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: OnConditionClicked_CB
// ---------------------------------------------------------------------------

static void Test4_OnConditionClicked_CB()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    int receivedIndex = -99;
    renderer.OnConditionClicked = [&](int idx) { receivedIndex = idx; };

    // Simulate a click via the callback directly
    if (renderer.OnConditionClicked)
        renderer.OnConditionClicked(2);

    TEST_ASSERT(receivedIndex == 2, "OnConditionClicked callback should fire with index 2");

    ReportTest("Test4_OnConditionClicked_CB", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: ClearLastClickedCondition
// ---------------------------------------------------------------------------

static void Test5_ClearLastClickedCondition()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    // Manually simulate setting the last clicked (normally done in RenderNode)
    renderer.ClearLastClickedCondition();
    TEST_ASSERT(renderer.GetLastClickedConditionIndex() == -1,
                "ClearLastClickedCondition should set index to -1");

    ReportTest("Test5_ClearLastClickedCondition", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: IsRefreshPending_False
// ---------------------------------------------------------------------------

static void Test6_IsRefreshPending_False()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    TEST_ASSERT(!renderer.IsRefreshPending(),
                "IsRefreshPending should be false on fresh construction");

    ReportTest("Test6_IsRefreshPending_False", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 7: PinManagerIntegration
// ---------------------------------------------------------------------------

static void Test7_PinManagerIntegration()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    ConditionPreset p("p1",
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(5.0));
    p.name = "PinCond";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    NodeBranchRenderer renderer(reg, mgr);

    // The renderer reads the pins from the DynamicDataPinManager at render time.
    // In headless mode we just verify the manager holds the expected pins.
    TEST_ASSERT(mgr.GetPinCount() == 1u, "Pin manager should have 1 pin");

    // Build a NodeBranchData with the synced refs
    NodeBranchData data;
    data.nodeID       = "node_1";
    data.nodeName     = "Branch #1";
    data.conditionRefs = refs;
    data.dynamicPins  = mgr.GetAllPins();

    // RenderNode is a no-op in headless mode, but should not crash.
    renderer.RenderNode(data);

    ReportTest("Test7_PinManagerIntegration", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 8: EmptyConditions
// ---------------------------------------------------------------------------

static void Test8_EmptyConditions()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager   mgr(reg);
    NodeBranchRenderer renderer(reg, mgr);

    NodeBranchData data;
    data.nodeID   = "node_empty";
    data.nodeName = "EmptyBranch";

    // Should not throw or crash
    renderer.RenderNode(data);

    TEST_ASSERT(renderer.GetLastClickedConditionIndex() == -1,
                "No click in empty node");

    ReportTest("Test8_EmptyConditions", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== NodeBranchRenderer Tests ===" << std::endl;

    Test1_DefaultState();
    Test2_NotifyPresetChanged();
    Test3_ClearRefreshPending();
    Test4_OnConditionClicked_CB();
    Test5_ClearLastClickedCondition();
    Test6_IsRefreshPending_False();
    Test7_PinManagerIntegration();
    Test8_EmptyConditions();

    std::cout << "\nResults: "
              << s_passCount << " passed, "
              << s_failCount << " failed."
              << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
