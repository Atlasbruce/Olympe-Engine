/**
 * @file Phase24_FullRoundTrip_Tests.cpp
 * @brief System-level round-trip tests: Build node → Serialize to JSON → Deserialize → Validate identity (Phase 24 M2.2.2).
 * @author Olympe Engine
 * @date 2026-03-19
 *
 * @details
 * Tests (4):
 *   1.  CompleteRoundTrip_2Conditions_3PinModes
 *         — Variable/Const condition and Const/Pin condition both survive full
 *           JSON save→load cycle, with dynamicPinID preserved exactly.
 *   2.  MultipleConditions_AllPinModes
 *         — 3 conditions covering Var/Var, Pin/Const and Const/Pin all
 *           round-trip with correct conditionIndex and compareType.
 *   3.  ConditionIndex_IsPersisted
 *         — conditionIndex is non-trivial (e.g. 5) and survives ToJson/FromJson.
 *   4.  DynamicPinID_IsPersisted
 *         — UUID-style dynamicPinID string survives ToJson/FromJson verbatim.
 *
 * These tests exercise the complete stack that a save/load cycle would use:
 *   TaskNodeDefinition::conditionOperandRefs → ConditionRef::ToJson() →
 *   nlohmann::json string → ConditionRef::FromJson() → reconstructed node.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 * No ImGui / ImNodes required — safe to run headless.
 */

#include "TaskSystem/TaskGraphTemplate.h"   // TaskNodeDefinition, TaskGraphTemplate
#include "BlueprintEditor/ConditionRef.h"   // ConditionRef, OperandRef
#include "TaskSystem/TaskGraphTypes.h"      // VariableType, TaskNodeType

#include <iostream>
#include <string>
#include <vector>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure (matches existing project convention)
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
// Helper: serialize a node's conditionOperandRefs to a JSON array and back.
// Simulates the save → file → load pipeline without touching the filesystem.
// ---------------------------------------------------------------------------
static std::vector<ConditionRef> SimulateSaveLoad(
        const std::vector<ConditionRef>& originalRefs)
{
    // ── SAVE: serialize each ConditionRef to a JSON array element ──────────
    nlohmann::json condRefsArray = nlohmann::json::array();
    for (const auto& ref : originalRefs)
        condRefsArray.push_back(ref.ToJson());

    // (Optional: round-trip through string to catch any JSON encoding issues)
    const std::string jsonStr = condRefsArray.dump();
    const nlohmann::json parsedArray = nlohmann::json::parse(jsonStr);

    // ── LOAD: deserialize back to ConditionRef objects ──────────────────────
    std::vector<ConditionRef> restoredRefs;
    for (const auto& item : parsedArray)
        restoredRefs.push_back(ConditionRef::FromJson(item));

    return restoredRefs;
}

// ---------------------------------------------------------------------------
// Test 1: CompleteRoundTrip_2Conditions_3PinModes
// ---------------------------------------------------------------------------
static void Test1_CompleteRoundTrip_2Conditions_3PinModes()
{
    int before = s_failCount;

    // Build node (emulates what the editor does before Save)
    TaskNodeDefinition originalDef;
    originalDef.NodeID   = 42;
    originalDef.NodeName = "Is Health Critical?";
    originalDef.Type     = TaskNodeType::Branch;

    // Condition 0: Variable <= Const  (no pin)
    ConditionRef cond0;
    cond0.conditionIndex            = 0;
    cond0.leftOperand.mode          = OperandRef::Mode::Variable;
    cond0.leftOperand.variableName  = "mHealth";
    cond0.operatorStr               = "<=";
    cond0.rightOperand.mode         = OperandRef::Mode::Const;
    cond0.rightOperand.constValue   = "50";
    cond0.compareType               = VariableType::Float;
    originalDef.conditionOperandRefs.push_back(cond0);

    // Condition 1: Const == Pin  (pin mode — dynamicPinID critical!)
    const std::string expectedPinID = "pin_inst_7f3a9b2c1d0e";
    ConditionRef cond1;
    cond1.conditionIndex             = 1;
    cond1.leftOperand.mode           = OperandRef::Mode::Const;
    cond1.leftOperand.constValue     = "100";
    cond1.operatorStr                = "==";
    cond1.rightOperand.mode          = OperandRef::Mode::Pin;
    cond1.rightOperand.dynamicPinID  = expectedPinID;
    cond1.compareType                = VariableType::Float;
    originalDef.conditionOperandRefs.push_back(cond1);

    // ── Round-trip through JSON ────────────────────────────────────────────
    const auto restored = SimulateSaveLoad(originalDef.conditionOperandRefs);

    // ── Validate node-level structure ──────────────────────────────────────
    TEST_ASSERT(restored.size() == 2u, "2 conditionOperandRefs must survive round-trip");

    // Condition 0 checks
    TEST_ASSERT(restored[0].conditionIndex == 0,                        "cond0: conditionIndex preserved");
    TEST_ASSERT(restored[0].leftOperand.mode == OperandRef::Mode::Variable, "cond0: left mode=Variable");
    TEST_ASSERT(restored[0].leftOperand.variableName == "mHealth",      "cond0: variableName");
    TEST_ASSERT(restored[0].operatorStr == "<=",                        "cond0: operatorStr");
    TEST_ASSERT(restored[0].rightOperand.mode == OperandRef::Mode::Const, "cond0: right mode=Const");
    TEST_ASSERT(restored[0].rightOperand.constValue == "50",            "cond0: constValue");
    TEST_ASSERT(restored[0].compareType == VariableType::Float,         "cond0: compareType=Float");

    // Condition 1 checks — CRITICAL: Pin UUID must be verbatim
    TEST_ASSERT(restored[1].conditionIndex == 1,                        "cond1: conditionIndex preserved");
    TEST_ASSERT(restored[1].leftOperand.mode == OperandRef::Mode::Const, "cond1: left mode=Const");
    TEST_ASSERT(restored[1].leftOperand.constValue == "100",            "cond1: constValue");
    TEST_ASSERT(restored[1].operatorStr == "==",                        "cond1: operatorStr");
    TEST_ASSERT(restored[1].rightOperand.mode == OperandRef::Mode::Pin, "cond1: right mode=Pin");
    TEST_ASSERT(restored[1].rightOperand.dynamicPinID == expectedPinID, "cond1: dynamicPinID verbatim");
    TEST_ASSERT(restored[1].compareType == VariableType::Float,         "cond1: compareType=Float");

    std::cout << "  (conditionRefs JSON: " << nlohmann::json{cond1.ToJson()}.dump() << ")\n";

    ReportTest("Test1_CompleteRoundTrip_2Conditions_3PinModes", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 2: MultipleConditions_AllPinModes
// ---------------------------------------------------------------------------
static void Test2_MultipleConditions_AllPinModes()
{
    int before = s_failCount;

    std::vector<ConditionRef> originalRefs;

    // Cond 0: Var vs Var  (no pins)
    ConditionRef c0;
    c0.conditionIndex           = 0;
    c0.leftOperand.mode         = OperandRef::Mode::Variable;
    c0.leftOperand.variableName = "var1";
    c0.operatorStr              = "!=";
    c0.rightOperand.mode        = OperandRef::Mode::Variable;
    c0.rightOperand.variableName= "var2";
    c0.compareType              = VariableType::Int;
    originalRefs.push_back(c0);

    // Cond 1: Pin vs Const
    ConditionRef c1;
    c1.conditionIndex             = 1;
    c1.leftOperand.mode           = OperandRef::Mode::Pin;
    c1.leftOperand.dynamicPinID   = "pin_inst_001abc";
    c1.operatorStr                = ">";
    c1.rightOperand.mode          = OperandRef::Mode::Const;
    c1.rightOperand.constValue    = "10.5";
    c1.compareType                = VariableType::Float;
    originalRefs.push_back(c1);

    // Cond 2: Const vs Pin
    ConditionRef c2;
    c2.conditionIndex             = 2;
    c2.leftOperand.mode           = OperandRef::Mode::Const;
    c2.leftOperand.constValue     = "true";
    c2.operatorStr                = "==";
    c2.rightOperand.mode          = OperandRef::Mode::Pin;
    c2.rightOperand.dynamicPinID  = "pin_inst_002def";
    c2.compareType                = VariableType::Bool;
    originalRefs.push_back(c2);

    // ── Round-trip ─────────────────────────────────────────────────────────
    const auto restored = SimulateSaveLoad(originalRefs);

    TEST_ASSERT(restored.size() == 3u, "3 conditions survive round-trip");

    // Cond 0: Var/Var with Int type
    TEST_ASSERT(restored[0].conditionIndex == 0,                         "c0: conditionIndex=0");
    TEST_ASSERT(restored[0].leftOperand.mode == OperandRef::Mode::Variable, "c0: left=Variable");
    TEST_ASSERT(restored[0].leftOperand.variableName == "var1",          "c0: var1");
    TEST_ASSERT(restored[0].rightOperand.mode == OperandRef::Mode::Variable, "c0: right=Variable");
    TEST_ASSERT(restored[0].rightOperand.variableName == "var2",         "c0: var2");
    TEST_ASSERT(restored[0].compareType == VariableType::Int,            "c0: compareType=Int");

    // Cond 1: Pin/Const
    TEST_ASSERT(restored[1].conditionIndex == 1,                         "c1: conditionIndex=1");
    TEST_ASSERT(restored[1].leftOperand.mode == OperandRef::Mode::Pin,   "c1: left=Pin");
    TEST_ASSERT(restored[1].leftOperand.dynamicPinID == "pin_inst_001abc", "c1: left pinID");
    TEST_ASSERT(restored[1].rightOperand.constValue == "10.5",           "c1: constValue=10.5");
    TEST_ASSERT(restored[1].compareType == VariableType::Float,          "c1: compareType=Float");

    // Cond 2: Const/Pin with Bool type
    TEST_ASSERT(restored[2].conditionIndex == 2,                         "c2: conditionIndex=2");
    TEST_ASSERT(restored[2].rightOperand.mode == OperandRef::Mode::Pin,  "c2: right=Pin");
    TEST_ASSERT(restored[2].rightOperand.dynamicPinID == "pin_inst_002def", "c2: right pinID");
    TEST_ASSERT(restored[2].compareType == VariableType::Bool,           "c2: compareType=Bool");

    ReportTest("Test2_MultipleConditions_AllPinModes", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 3: ConditionIndex_IsPersisted  (non-trivial index)
// ---------------------------------------------------------------------------
static void Test3_ConditionIndex_IsPersisted()
{
    int before = s_failCount;

    std::vector<ConditionRef> refs;
    for (int idx = 0; idx < 5; ++idx)
    {
        ConditionRef ref;
        ref.conditionIndex           = idx;
        ref.leftOperand.mode         = OperandRef::Mode::Const;
        ref.leftOperand.constValue   = std::to_string(idx * 10);
        ref.operatorStr              = "==";
        ref.rightOperand.mode        = OperandRef::Mode::Const;
        ref.rightOperand.constValue  = std::to_string(idx * 10);
        ref.compareType              = VariableType::Float;
        refs.push_back(ref);
    }

    const auto restored = SimulateSaveLoad(refs);

    TEST_ASSERT(restored.size() == 5u, "5 conditions preserved");
    for (int idx = 0; idx < 5; ++idx)
    {
        TEST_ASSERT(restored[static_cast<size_t>(idx)].conditionIndex == idx,
                    "conditionIndex preserved for idx=" + std::to_string(idx));
    }

    ReportTest("Test3_ConditionIndex_IsPersisted", s_failCount == before);
}

// ---------------------------------------------------------------------------
// Test 4: DynamicPinID_IsPersisted  (exact UUID string)
// ---------------------------------------------------------------------------
static void Test4_DynamicPinID_IsPersisted()
{
    int before = s_failCount;

    // A realistically-formatted UUID-style pin ID
    const std::string pinUUID = "pin_inst_a1b2c3d4e5f60718293a4b5c";

    ConditionRef ref;
    ref.conditionIndex              = 3;
    ref.leftOperand.mode            = OperandRef::Mode::Pin;
    ref.leftOperand.dynamicPinID    = pinUUID;
    ref.operatorStr                 = ">=";
    ref.rightOperand.mode           = OperandRef::Mode::Const;
    ref.rightOperand.constValue     = "0";
    ref.compareType                 = VariableType::Float;

    const nlohmann::json j = ref.ToJson();
    const std::string serialized = j.dump();

    // Verify the UUID appears verbatim in the JSON string
    TEST_ASSERT(serialized.find(pinUUID) != std::string::npos,
                "UUID must appear verbatim in JSON output");

    // Deserialize and verify
    const ConditionRef loaded = ConditionRef::FromJson(j);
    TEST_ASSERT(loaded.conditionIndex == 3,                          "conditionIndex=3 preserved");
    TEST_ASSERT(loaded.leftOperand.mode == OperandRef::Mode::Pin,    "left mode=Pin preserved");
    TEST_ASSERT(loaded.leftOperand.dynamicPinID == pinUUID,          "UUID preserved verbatim");
    TEST_ASSERT(loaded.operatorStr == ">=",                          "operatorStr preserved");
    TEST_ASSERT(loaded.rightOperand.constValue == "0",               "constValue preserved");

    ReportTest("Test4_DynamicPinID_IsPersisted", s_failCount == before);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    std::cout << "=== Phase 24 Full Round-Trip System Tests (M2.2.2) ===\n";

    Test1_CompleteRoundTrip_2Conditions_3PinModes();
    Test2_MultipleConditions_AllPinModes();
    Test3_ConditionIndex_IsPersisted();
    Test4_DynamicPinID_IsPersisted();

    std::cout << "\nResults: " << s_passCount << " passed, "
              << s_failCount << " failed.\n";

    if (s_failCount == 0)
        std::cout << "=== FULL ROUND-TRIP SUCCESS ===\n";

    return (s_failCount == 0) ? 0 : 1;
}
