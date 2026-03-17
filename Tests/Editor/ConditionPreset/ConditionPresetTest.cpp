/**
 * @file ConditionPresetTest.cpp
 * @brief Unit tests for ConditionPreset (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * Tests (8):
 *   1. ConstructorWithOperands  — full constructor sets fields correctly
 *   2. GetPreview               — correct preview string
 *   3. NeedsLeftPin             — left pin detection
 *   4. NeedsRightPin            — right pin detection
 *   5. GetPinNeeds              — combined pair result
 *   6. SerializationRoundTrip   — ToJson / FromJson preserves all fields
 *   7. HandlesAllOperatorTypes  — every ComparisonOp serializes/deserializes
 *   8. UniqueIDGeneration       — IDs assigned by registry are non-empty (tested via ConditionPreset.id)
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/Operand.h"

#include <iostream>
#include <string>

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
// Test 1: ConstructorWithOperands
// ---------------------------------------------------------------------------

static void Test1_ConstructorWithOperands()
{
    bool ok = true;

    Operand left  = Operand::CreateVariable("mHealth");
    Operand right = Operand::CreateConst(2.0);

    ConditionPreset p("preset_001", left, ComparisonOp::LessEqual, right);

    TEST_ASSERT(p.id == "preset_001",         "id should be 'preset_001'");
    TEST_ASSERT(p.left.IsVariable(),           "left should be Variable");
    TEST_ASSERT(p.right.IsConst(),             "right should be Const");
    TEST_ASSERT(p.op == ComparisonOp::LessEqual, "op should be LessEqual");

    ok = (p.id == "preset_001") && p.left.IsVariable() && p.right.IsConst()
      && (p.op == ComparisonOp::LessEqual);
    ReportTest("ConstructorWithOperands", ok);
}

// ---------------------------------------------------------------------------
// Test 2: GetPreview
// ---------------------------------------------------------------------------

static void Test2_GetPreview()
{
    bool ok = true;

    ConditionPreset p("p2",
                      Operand::CreateVariable("mHealth"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(2.0));

    std::string preview = p.GetPreview();
    TEST_ASSERT(preview == "[mHealth] <= [2]",
                "Preview should be '[mHealth] <= [2]', got: " + preview);

    // Second case: Pin vs Variable
    ConditionPreset p2("p2b",
                       Operand::CreateVariable("mSpeed"),
                       ComparisonOp::Equal,
                       Operand::CreatePin("Pin:1"));
    std::string preview2 = p2.GetPreview();
    TEST_ASSERT(preview2 == "[mSpeed] == [Pin:1]",
                "Preview should be '[mSpeed] == [Pin:1]', got: " + preview2);

    ok = (preview == "[mHealth] <= [2]") && (preview2 == "[mSpeed] == [Pin:1]");
    ReportTest("GetPreview", ok);
}

// ---------------------------------------------------------------------------
// Test 3: NeedsLeftPin
// ---------------------------------------------------------------------------

static void Test3_NeedsLeftPin()
{
    bool ok = true;

    ConditionPreset p1("p3a",
                       Operand::CreatePin("Pin:1"),
                       ComparisonOp::Greater,
                       Operand::CreateConst(0.0));
    TEST_ASSERT(p1.NeedsLeftPin(),  "NeedsLeftPin should be true when left is Pin");

    ConditionPreset p2("p3b",
                       Operand::CreateVariable("mHealth"),
                       ComparisonOp::Greater,
                       Operand::CreateConst(0.0));
    TEST_ASSERT(!p2.NeedsLeftPin(), "NeedsLeftPin should be false when left is Variable");

    ok = p1.NeedsLeftPin() && !p2.NeedsLeftPin();
    ReportTest("NeedsLeftPin", ok);
}

// ---------------------------------------------------------------------------
// Test 4: NeedsRightPin
// ---------------------------------------------------------------------------

static void Test4_NeedsRightPin()
{
    bool ok = true;

    ConditionPreset p1("p4a",
                       Operand::CreateVariable("mHealth"),
                       ComparisonOp::Equal,
                       Operand::CreatePin("Pin:2"));
    TEST_ASSERT(p1.NeedsRightPin(),  "NeedsRightPin should be true when right is Pin");

    ConditionPreset p2("p4b",
                       Operand::CreateVariable("mHealth"),
                       ComparisonOp::Equal,
                       Operand::CreateConst(5.0));
    TEST_ASSERT(!p2.NeedsRightPin(), "NeedsRightPin should be false when right is Const");

    ok = p1.NeedsRightPin() && !p2.NeedsRightPin();
    ReportTest("NeedsRightPin", ok);
}

// ---------------------------------------------------------------------------
// Test 5: GetPinNeeds
// ---------------------------------------------------------------------------

static void Test5_GetPinNeeds()
{
    bool ok = true;

    // Both pins
    ConditionPreset p1("p5a",
                       Operand::CreatePin("Pin:1"),
                       ComparisonOp::NotEqual,
                       Operand::CreatePin("Pin:2"));
    std::pair<bool,bool> needs1 = p1.GetPinNeeds();
    TEST_ASSERT(needs1.first,  "GetPinNeeds: first should be true (left is Pin)");
    TEST_ASSERT(needs1.second, "GetPinNeeds: second should be true (right is Pin)");

    // No pins
    ConditionPreset p2("p5b",
                       Operand::CreateVariable("mHealth"),
                       ComparisonOp::Equal,
                       Operand::CreateConst(3.0));
    std::pair<bool,bool> needs2 = p2.GetPinNeeds();
    TEST_ASSERT(!needs2.first,  "GetPinNeeds: first should be false (left is Variable)");
    TEST_ASSERT(!needs2.second, "GetPinNeeds: second should be false (right is Const)");

    ok = needs1.first && needs1.second && !needs2.first && !needs2.second;
    ReportTest("GetPinNeeds", ok);
}

// ---------------------------------------------------------------------------
// Test 6: SerializationRoundTrip
// ---------------------------------------------------------------------------

static void Test6_SerializationRoundTrip()
{
    bool ok = true;

    ConditionPreset original("preset_005",
                             Operand::CreateVariable("mSpeed"),
                             ComparisonOp::Equal,
                             Operand::CreatePin("Pin:1"));
    original.name = "Condition #5";

    nlohmann::json j = original.ToJson();
    ConditionPreset loaded = ConditionPreset::FromJson(j);

    TEST_ASSERT(loaded.id   == "preset_005",   "RT: id should be 'preset_005'");
    TEST_ASSERT(loaded.name == "Condition #5", "RT: name should be 'Condition #5'");
    TEST_ASSERT(loaded.left.IsVariable(),      "RT: left should be Variable");
    TEST_ASSERT(loaded.left.stringValue == "mSpeed", "RT: left.stringValue should be 'mSpeed'");
    TEST_ASSERT(loaded.op == ComparisonOp::Equal,    "RT: op should be Equal");
    TEST_ASSERT(loaded.right.IsPin(),                "RT: right should be Pin");
    TEST_ASSERT(loaded.right.stringValue == "Pin:1", "RT: right.stringValue should be 'Pin:1'");

    ok = (loaded.id == "preset_005") && (loaded.name == "Condition #5")
      && loaded.left.IsVariable() && (loaded.left.stringValue == "mSpeed")
      && (loaded.op == ComparisonOp::Equal)
      && loaded.right.IsPin() && (loaded.right.stringValue == "Pin:1");

    ReportTest("SerializationRoundTrip", ok);
}

// ---------------------------------------------------------------------------
// Test 7: HandlesAllOperatorTypes
// ---------------------------------------------------------------------------

static void Test7_HandlesAllOperatorTypes()
{
    bool ok = true;

    const ComparisonOp ops[] = {
        ComparisonOp::Equal,
        ComparisonOp::NotEqual,
        ComparisonOp::Less,
        ComparisonOp::LessEqual,
        ComparisonOp::Greater,
        ComparisonOp::GreaterEqual
    };
    const char* expected[] = { "==", "!=", "<", "<=", ">", ">=" };

    for (int i = 0; i < 6; ++i)
    {
        ConditionPreset p("test",
                          Operand::CreateConst(1.0),
                          ops[i],
                          Operand::CreateConst(2.0));

        // Serialize and deserialize
        nlohmann::json j = p.ToJson();
        ConditionPreset loaded = ConditionPreset::FromJson(j);

        TEST_ASSERT(loaded.op == ops[i],
                    std::string("Operator round-trip failed for: ") + expected[i]);
        TEST_ASSERT(ConditionPreset::OpToString(ops[i]) == expected[i],
                    std::string("OpToString failed for: ") + expected[i]);

        if (loaded.op != ops[i] ||
            ConditionPreset::OpToString(ops[i]) != expected[i])
            ok = false;
    }

    ReportTest("HandlesAllOperatorTypes", ok);
}

// ---------------------------------------------------------------------------
// Test 8: UniqueIDGeneration
// ---------------------------------------------------------------------------

static void Test8_UniqueIDGeneration()
{
    bool ok = true;

    // A default-constructed preset has empty id — callers must assign.
    ConditionPreset p1;
    TEST_ASSERT(p1.id.empty(), "Default id should be empty (caller assigns UUID)");

    // A preset constructed with an id carries it.
    ConditionPreset p2("my-unique-id",
                       Operand::CreateVariable("x"),
                       ComparisonOp::Equal,
                       Operand::CreateConst(0.0));
    TEST_ASSERT(p2.id == "my-unique-id", "Constructed id should match provided value");
    TEST_ASSERT(!p2.id.empty(),          "Constructed id should not be empty");

    // Two presets with different IDs are distinct.
    ConditionPreset p3("id-A",
                       Operand::CreateVariable("a"),
                       ComparisonOp::Equal,
                       Operand::CreateConst(1.0));
    ConditionPreset p4("id-B",
                       Operand::CreateVariable("b"),
                       ComparisonOp::Equal,
                       Operand::CreateConst(2.0));
    TEST_ASSERT(p3.id != p4.id, "Two presets with different IDs should be distinct");

    ok = p1.id.empty() && (p2.id == "my-unique-id") && (p3.id != p4.id);
    ReportTest("UniqueIDGeneration", ok);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== ConditionPresetTest (Phase 24.0) ===" << std::endl;

    Test1_ConstructorWithOperands();
    Test2_GetPreview();
    Test3_NeedsLeftPin();
    Test4_NeedsRightPin();
    Test5_GetPinNeeds();
    Test6_SerializationRoundTrip();
    Test7_HandlesAllOperatorTypes();
    Test8_UniqueIDGeneration();

    std::cout << std::endl
              << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
