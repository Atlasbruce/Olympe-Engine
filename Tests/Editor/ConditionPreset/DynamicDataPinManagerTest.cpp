/**
 * @file DynamicDataPinManagerTest.cpp
 * @brief Unit tests for DynamicDataPinManager (Phase 24.3).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * Tests (10):
 *   1.  SyncPins_NoConditions       — empty conditions → 0 pins
 *   2.  SyncPins_ConstOperands      — Variable/Const conditions → 0 pins
 *   3.  SyncPins_LeftPinOperand     — left Pin → 1 pin generated (Left)
 *   4.  SyncPins_BothPinOperands    — both Pin → 2 pins for the condition
 *   5.  SyncPins_StableUUID         — repeated sync reuses existing UUIDs
 *   6.  GetPinByID_Found            — O(1) lookup returns correct pin
 *   7.  GetPinByID_NotFound         — returns nullptr for unknown ID
 *   8.  GetPinsForCondition         — returns pins for the right condition
 *   9.  InvalidatePreset_ClearsPins — InvalidatePreset removes all pins
 *  10.  PinLabelFormat              — label matches "In #NL/R: <preview>"
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/ConditionPreset/DynamicDataPinManager.h"
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

// ---------------------------------------------------------------------------
// Test 1: SyncPins_NoConditions
// ---------------------------------------------------------------------------

static void Test1_SyncPins_NoConditions()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager mgr(reg);

    std::vector<NodeConditionRef> refs;
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 0u, "Expected 0 pins for no conditions");

    ReportTest("Test1_SyncPins_NoConditions", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: SyncPins_ConstOperands
// ---------------------------------------------------------------------------

static void Test2_SyncPins_ConstOperands()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    // Variable <= Const — no Pin operands
    ConditionPreset p("p1",
                      Operand::CreateVariable("mHealth"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(2.0));
    p.name = "HealthCheck";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 0u, "Variable<=Const should generate 0 pins");

    ReportTest("Test2_SyncPins_ConstOperands", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: SyncPins_LeftPinOperand
// ---------------------------------------------------------------------------

static void Test3_SyncPins_LeftPinOperand()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    // Pin <= Const
    ConditionPreset p("p1",
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(2.0));
    p.name = "PinLeftCheck";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 1u, "Expected 1 pin for left Pin operand");
    TEST_ASSERT(mgr.GetAllPins()[0].position == OperandPosition::Left,
                "Pin should be on the Left side");
    TEST_ASSERT(!refs[0].leftPinID.empty(), "leftPinID should be written back");

    ReportTest("Test3_SyncPins_LeftPinOperand", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: SyncPins_BothPinOperands
// ---------------------------------------------------------------------------

static void Test4_SyncPins_BothPinOperands()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    // Pin <= Pin
    ConditionPreset p("p1",
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual,
                      Operand::CreatePin("Pin:2"));
    p.name = "BothPins";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 2u, "Expected 2 pins for both Pin operands");
    TEST_ASSERT(!refs[0].leftPinID.empty(),  "leftPinID must be written back");
    TEST_ASSERT(!refs[0].rightPinID.empty(), "rightPinID must be written back");

    ReportTest("Test4_SyncPins_BothPinOperands", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: SyncPins_StableUUID
// ---------------------------------------------------------------------------

static void Test5_SyncPins_StableUUID()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    ConditionPreset p("p1",
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(2.0));
    p.name = "PinStable";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    std::string firstID = mgr.GetAllPins()[0].id;
    TEST_ASSERT(!firstID.empty(), "First UUID should be non-empty");

    // Sync again with same conditions — UUID must be stable
    mgr.SyncPins(refs);
    TEST_ASSERT(mgr.GetPinCount() == 1u, "Still 1 pin after second sync");
    TEST_ASSERT(mgr.GetAllPins()[0].id == firstID, "UUID must be stable on re-sync");

    ReportTest("Test5_SyncPins_StableUUID", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: GetPinByID_Found
// ---------------------------------------------------------------------------

static void Test6_GetPinByID_Found()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    ConditionPreset p("p1",
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::Equal,
                      Operand::CreateConst(0.0));
    p.name = "PinEq";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    const std::string id = mgr.GetAllPins()[0].id;
    const DynamicDataPin* found = mgr.GetPinByID(id);
    TEST_ASSERT(found != nullptr, "GetPinByID should find the pin");
    TEST_ASSERT(found->id == id,  "Returned pin should have correct ID");

    ReportTest("Test6_GetPinByID_Found", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 7: GetPinByID_NotFound
// ---------------------------------------------------------------------------

static void Test7_GetPinByID_NotFound()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    DynamicDataPinManager mgr(reg);

    const DynamicDataPin* found = mgr.GetPinByID("nonexistent_uuid");
    TEST_ASSERT(found == nullptr, "GetPinByID should return nullptr for unknown ID");

    ReportTest("Test7_GetPinByID_NotFound", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 8: GetPinsForCondition
// ---------------------------------------------------------------------------

static void Test8_GetPinsForCondition()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    ConditionPreset p1("p1",
                       Operand::CreatePin("Pin:1"),
                       ComparisonOp::Less,
                       Operand::CreateConst(5.0));
    p1.name = "CondA";
    reg.CreatePreset(p1);

    ConditionPreset p2("p2",
                       Operand::CreateVariable("mSpeed"),
                       ComparisonOp::Greater,
                       Operand::CreateConst(0.0));
    p2.name = "CondB";
    reg.CreatePreset(p2);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs;
    refs.emplace_back("p1", LogicalOp::Start);
    refs.emplace_back("p2", LogicalOp::And);
    mgr.SyncPins(refs);

    auto pinsFor0 = mgr.GetPinsForCondition(0);
    auto pinsFor1 = mgr.GetPinsForCondition(1);

    TEST_ASSERT(pinsFor0.size() == 1u, "Condition 0 should have 1 pin");
    TEST_ASSERT(pinsFor1.size() == 0u, "Condition 1 has no Pin operands");

    ReportTest("Test8_GetPinsForCondition", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 9: InvalidatePreset_ClearsPins
// ---------------------------------------------------------------------------

static void Test9_InvalidatePreset_ClearsPins()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    ConditionPreset p("p1",
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual,
                      Operand::CreatePin("Pin:2"));
    p.name = "TwoPin";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);
    TEST_ASSERT(mgr.GetPinCount() == 2u, "Expected 2 pins before invalidate");

    mgr.InvalidatePreset("p1");
    TEST_ASSERT(mgr.GetPinCount() == 0u, "Expected 0 pins after invalidate");

    ReportTest("Test9_InvalidatePreset_ClearsPins", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 10: PinLabelFormat
// ---------------------------------------------------------------------------

static void Test10_PinLabelFormat()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    // Pin <= Pin — preview will be "[Pin:1] <= [Pin:2]"
    ConditionPreset p("p1",
                      Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual,
                      Operand::CreatePin("Pin:2"));
    p.name = "TwoPinLabel";
    reg.CreatePreset(p);

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 2u, "Expected 2 pins");

    const DynamicDataPin& leftPin  = mgr.GetAllPins()[0];
    const DynamicDataPin& rightPin = mgr.GetAllPins()[1];

    // Labels should contain "In #1L:" and "In #1R:"
    TEST_ASSERT(leftPin.label.find("In #1L:") != std::string::npos,
                "Left pin label should contain 'In #1L:'");
    TEST_ASSERT(rightPin.label.find("In #1R:") != std::string::npos,
                "Right pin label should contain 'In #1R:'");

    ReportTest("Test10_PinLabelFormat", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== DynamicDataPinManager Tests ===" << std::endl;

    Test1_SyncPins_NoConditions();
    Test2_SyncPins_ConstOperands();
    Test3_SyncPins_LeftPinOperand();
    Test4_SyncPins_BothPinOperands();
    Test5_SyncPins_StableUUID();
    Test6_GetPinByID_Found();
    Test7_GetPinByID_NotFound();
    Test8_GetPinsForCondition();
    Test9_InvalidatePreset_ClearsPins();
    Test10_PinLabelFormat();

    std::cout << "\nResults: "
              << s_passCount << " passed, "
              << s_failCount << " failed."
              << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
