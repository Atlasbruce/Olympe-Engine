/**
 * @file DynamicDataPinTest.cpp
 * @brief Unit tests for DynamicDataPin (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * Tests (6):
 *   1. GenerateUniqueID        — generated IDs are non-empty and distinct
 *   2. GetDisplayLabel         — label format matches expected pattern
 *   3. CorrectConditionIndex   — conditionIndex is stored correctly
 *   4. CorrectPosition         — position (Left vs Right) is stored correctly
 *   5. SerializationRoundTrip  — ToJson / FromJson preserves all fields
 *   6. UUIDPersistenceAfterRoundTrip — id survives serialization intact
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/ConditionPreset/DynamicDataPin.h"

#include <iostream>
#include <string>
#include <set>

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
// Test 1: GenerateUniqueID
// ---------------------------------------------------------------------------

static void Test1_GenerateUniqueID()
{
    bool ok = true;

    // IDs must be non-empty
    std::string id1 = DynamicDataPin::GenerateUniqueID();
    std::string id2 = DynamicDataPin::GenerateUniqueID();

    TEST_ASSERT(!id1.empty(), "GenerateUniqueID should return non-empty string");
    TEST_ASSERT(!id2.empty(), "GenerateUniqueID second call should return non-empty string");

    // IDs must be distinct (probabilistic — extremely unlikely to collide)
    TEST_ASSERT(id1 != id2, "Two generated IDs should be distinct");

    // Batch uniqueness check
    std::set<std::string> ids;
    for (int i = 0; i < 20; ++i)
        ids.insert(DynamicDataPin::GenerateUniqueID());
    TEST_ASSERT(ids.size() == 20, "20 generated IDs should all be distinct");

    ok = !id1.empty() && !id2.empty() && (id1 != id2) && (ids.size() == 20);
    ReportTest("GenerateUniqueID", ok);
}

// ---------------------------------------------------------------------------
// Test 2: GetDisplayLabel
// ---------------------------------------------------------------------------

static void Test2_GetDisplayLabel()
{
    bool ok = true;

    // Left side, conditionIndex 2, preview "[mSpeed] == [Pin:1]"
    DynamicDataPin pinL(2, OperandPosition::Left, "[mSpeed] == [Pin:1]");
    std::string labelL = pinL.GetDisplayLabel();
    TEST_ASSERT(labelL == "In #3L: [mSpeed] == [Pin:1]",
                "Label should be 'In #3L: [mSpeed] == [Pin:1]', got: " + labelL);

    // Right side, conditionIndex 0, preview "[mHealth] <= [2]"
    DynamicDataPin pinR(0, OperandPosition::Right, "[mHealth] <= [2]");
    std::string labelR = pinR.GetDisplayLabel();
    TEST_ASSERT(labelR == "In #1R: [mHealth] <= [2]",
                "Label should be 'In #1R: [mHealth] <= [2]', got: " + labelR);

    ok = (labelL == "In #3L: [mSpeed] == [Pin:1]")
      && (labelR == "In #1R: [mHealth] <= [2]");
    ReportTest("GetDisplayLabel", ok);
}

// ---------------------------------------------------------------------------
// Test 3: CorrectConditionIndex
// ---------------------------------------------------------------------------

static void Test3_CorrectConditionIndex()
{
    bool ok = true;

    DynamicDataPin pin0(0, OperandPosition::Left, "preview");
    DynamicDataPin pin3(3, OperandPosition::Left, "preview");

    TEST_ASSERT(pin0.conditionIndex == 0, "conditionIndex should be 0");
    TEST_ASSERT(pin3.conditionIndex == 3, "conditionIndex should be 3");

    ok = (pin0.conditionIndex == 0) && (pin3.conditionIndex == 3);
    ReportTest("CorrectConditionIndex", ok);
}

// ---------------------------------------------------------------------------
// Test 4: CorrectPosition
// ---------------------------------------------------------------------------

static void Test4_CorrectPosition()
{
    bool ok = true;

    DynamicDataPin pinL(0, OperandPosition::Left,  "preview");
    DynamicDataPin pinR(0, OperandPosition::Right, "preview");

    TEST_ASSERT(pinL.position == OperandPosition::Left,
                "position should be Left");
    TEST_ASSERT(pinR.position == OperandPosition::Right,
                "position should be Right");

    ok = (pinL.position == OperandPosition::Left)
      && (pinR.position == OperandPosition::Right);
    ReportTest("CorrectPosition", ok);
}

// ---------------------------------------------------------------------------
// Test 5: SerializationRoundTrip
// ---------------------------------------------------------------------------

static void Test5_SerializationRoundTrip()
{
    bool ok = true;

    DynamicDataPin original(1, OperandPosition::Right, "[mHealth] != [Pin:2]");
    original.nodePinID  = "node_pin_42";
    original.dataValue  = 7.5f;

    nlohmann::json j = original.ToJson();
    DynamicDataPin loaded = DynamicDataPin::FromJson(j);

    TEST_ASSERT(loaded.conditionIndex == 1,               "RT: conditionIndex should be 1");
    TEST_ASSERT(loaded.position == OperandPosition::Right, "RT: position should be Right");
    TEST_ASSERT(loaded.nodePinID == "node_pin_42",         "RT: nodePinID should be 'node_pin_42'");
    TEST_ASSERT(loaded.dataValue == 7.5f,                  "RT: dataValue should be 7.5");
    // Label round-trip
    TEST_ASSERT(!loaded.label.empty(),                     "RT: label should not be empty");

    ok = (loaded.conditionIndex == 1)
      && (loaded.position == OperandPosition::Right)
      && (loaded.nodePinID == "node_pin_42")
      && (loaded.dataValue == 7.5f)
      && !loaded.label.empty();

    ReportTest("SerializationRoundTrip", ok);
}

// ---------------------------------------------------------------------------
// Test 6: UUIDPersistenceAfterRoundTrip
// ---------------------------------------------------------------------------

static void Test6_UUIDPersistenceAfterRoundTrip()
{
    bool ok = true;

    DynamicDataPin original(0, OperandPosition::Left, "preview");
    std::string originalID = original.id;

    TEST_ASSERT(!originalID.empty(), "Original ID should not be empty");

    nlohmann::json j = original.ToJson();
    DynamicDataPin loaded = DynamicDataPin::FromJson(j);

    TEST_ASSERT(loaded.id == originalID, "Loaded ID should match original after round-trip");

    ok = !originalID.empty() && (loaded.id == originalID);
    ReportTest("UUIDPersistenceAfterRoundTrip", ok);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== DynamicDataPinTest (Phase 24.0) ===" << std::endl;

    Test1_GenerateUniqueID();
    Test2_GetDisplayLabel();
    Test3_CorrectConditionIndex();
    Test4_CorrectPosition();
    Test5_SerializationRoundTrip();
    Test6_UUIDPersistenceAfterRoundTrip();

    std::cout << std::endl
              << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
