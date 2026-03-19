/**
 * @file DynamicDataPinManager_Tests.cpp
 * @brief Additional unit tests for DynamicDataPinManager — pin generation, naming, sequence numbers (Phase 24 M2.2).
 * @author Olympe Engine
 * @date 2026-03-19
 *
 * @details
 * Tests (6) — supplements the 10 tests in DynamicDataPinManagerTest.cpp:
 *   1.  ShortLabel_SinglePin       — GetShortLabel() returns "Pin-in #1" for first pin
 *   2.  ShortLabel_MultiplePins    — sequence numbers are 1-based and increment correctly
 *   3.  SequenceNumber_AfterResync — sequence numbers reassigned stably on re-sync
 *   4.  MultiCond_PinOrder         — pins ordered: cond0-Left, cond0-Right, cond1-Left
 *   5.  ShortLabel_AfterInvalidate — pins regenerated after clear have correct seq numbers
 *   6.  PinCount_TwoCondBothPins   — 2 conditions × 2 Pin operands = 4 pins
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
// Helpers
// ---------------------------------------------------------------------------

static ConditionPreset MakePinLeftPreset(const std::string& id, const std::string& name)
{
    ConditionPreset p(id, Operand::CreatePin("Pin:1"),
                      ComparisonOp::LessEqual, Operand::CreateConst(5.0));
    p.name = name;
    return p;
}

static ConditionPreset MakeBothPinPreset(const std::string& id, const std::string& name)
{
    ConditionPreset p(id, Operand::CreatePin("Pin:1"),
                      ComparisonOp::Equal, Operand::CreatePin("Pin:2"));
    p.name = name;
    return p;
}

// ---------------------------------------------------------------------------
// Test 1: ShortLabel_SinglePin
// ---------------------------------------------------------------------------
static void Test1_ShortLabel_SinglePin()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePinLeftPreset("p1", "SinglePin"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 1u, "Expected 1 pin");
    const DynamicDataPin& pin = mgr.GetAllPins()[0];
    TEST_ASSERT(pin.sequenceNumber == 1, "sequenceNumber should be 1");
    TEST_ASSERT(pin.GetShortLabel() == "Pin-in #1",
                "GetShortLabel() should return 'Pin-in #1'");

    ReportTest("Test1_ShortLabel_SinglePin", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: ShortLabel_MultiplePins
// ---------------------------------------------------------------------------
static void Test2_ShortLabel_MultiplePins()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeBothPinPreset("p1", "TwoPins"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 2u, "Expected 2 pins");
    TEST_ASSERT(mgr.GetAllPins()[0].sequenceNumber == 1,
                "First pin sequenceNumber should be 1");
    TEST_ASSERT(mgr.GetAllPins()[1].sequenceNumber == 2,
                "Second pin sequenceNumber should be 2");
    TEST_ASSERT(mgr.GetAllPins()[0].GetShortLabel() == "Pin-in #1",
                "First short label should be 'Pin-in #1'");
    TEST_ASSERT(mgr.GetAllPins()[1].GetShortLabel() == "Pin-in #2",
                "Second short label should be 'Pin-in #2'");

    ReportTest("Test2_ShortLabel_MultiplePins", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: SequenceNumber_AfterResync
// ---------------------------------------------------------------------------
static void Test3_SequenceNumber_AfterResync()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePinLeftPreset("p1", "ResyncPin"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);

    std::string firstID = mgr.GetAllPins()[0].id;
    // Re-sync should preserve UUID and keep sequenceNumber = 1
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 1u, "Still 1 pin after resync");
    TEST_ASSERT(mgr.GetAllPins()[0].id == firstID, "UUID stable on resync");
    TEST_ASSERT(mgr.GetAllPins()[0].sequenceNumber == 1,
                "sequenceNumber still 1 after resync");
    TEST_ASSERT(mgr.GetAllPins()[0].GetShortLabel() == "Pin-in #1",
                "Short label stable after resync");

    ReportTest("Test3_SequenceNumber_AfterResync", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: MultiCond_PinOrder
// ---------------------------------------------------------------------------
static void Test4_MultiCond_PinOrder()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;

    // Cond 0: both pins, Cond 1: left pin only
    reg.CreatePreset(MakeBothPinPreset("p1", "BothPins"));
    reg.CreatePreset(MakePinLeftPreset("p2", "LeftPin"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs;
    refs.emplace_back("p1", LogicalOp::Start);
    refs.emplace_back("p2", LogicalOp::And);
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 3u, "Expected 3 pins total");
    // Sequence should be 1, 2, 3 in order
    TEST_ASSERT(mgr.GetAllPins()[0].sequenceNumber == 1, "Pin[0] seq=1");
    TEST_ASSERT(mgr.GetAllPins()[1].sequenceNumber == 2, "Pin[1] seq=2");
    TEST_ASSERT(mgr.GetAllPins()[2].sequenceNumber == 3, "Pin[2] seq=3");
    TEST_ASSERT(mgr.GetAllPins()[2].GetShortLabel() == "Pin-in #3",
                "Third pin short label 'Pin-in #3'");

    ReportTest("Test4_MultiCond_PinOrder", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 5: ShortLabel_AfterInvalidate
// ---------------------------------------------------------------------------
static void Test5_ShortLabel_AfterInvalidate()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeBothPinPreset("p1", "InvalidateTest"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs = { NodeConditionRef("p1", LogicalOp::Start) };
    mgr.SyncPins(refs);
    TEST_ASSERT(mgr.GetPinCount() == 2u, "2 pins before invalidate");

    mgr.InvalidatePreset("p1");
    TEST_ASSERT(mgr.GetPinCount() == 0u, "0 pins after invalidate");

    // Re-sync should regenerate with correct sequence numbers
    mgr.SyncPins(refs);
    TEST_ASSERT(mgr.GetPinCount() == 2u, "2 pins after re-sync");
    TEST_ASSERT(mgr.GetAllPins()[0].sequenceNumber == 1, "Re-synced pin[0] seq=1");
    TEST_ASSERT(mgr.GetAllPins()[1].sequenceNumber == 2, "Re-synced pin[1] seq=2");

    ReportTest("Test5_ShortLabel_AfterInvalidate", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 6: PinCount_TwoCondBothPins
// ---------------------------------------------------------------------------
static void Test6_PinCount_TwoCondBothPins()
{
    int before = s_failCount;
    ConditionPresetRegistry reg;
    reg.CreatePreset(MakeBothPinPreset("p1", "BothA"));
    reg.CreatePreset(MakeBothPinPreset("p2", "BothB"));

    DynamicDataPinManager mgr(reg);
    std::vector<NodeConditionRef> refs;
    refs.emplace_back("p1", LogicalOp::Start);
    refs.emplace_back("p2", LogicalOp::And);
    mgr.SyncPins(refs);

    TEST_ASSERT(mgr.GetPinCount() == 4u, "2 conditions × 2 pins = 4 pins");
    TEST_ASSERT(mgr.GetAllPins()[3].sequenceNumber == 4, "Last pin seq=4");
    TEST_ASSERT(mgr.GetAllPins()[3].GetShortLabel() == "Pin-in #4",
                "Last short label 'Pin-in #4'");

    ReportTest("Test6_PinCount_TwoCondBothPins", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    std::cout << "=== DynamicDataPinManager Additional Tests (M2.2) ===\n";

    Test1_ShortLabel_SinglePin();
    Test2_ShortLabel_MultiplePins();
    Test3_SequenceNumber_AfterResync();
    Test4_MultiCond_PinOrder();
    Test5_ShortLabel_AfterInvalidate();
    Test6_PinCount_TwoCondBothPins();

    std::cout << "\nResults: " << s_passCount << " passed, "
              << s_failCount << " failed.\n";
    return (s_failCount == 0) ? 0 : 1;
}
