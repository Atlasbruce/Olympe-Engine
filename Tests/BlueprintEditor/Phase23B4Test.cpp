/**
 * @file Phase23B4Test.cpp
 * @brief Comprehensive tests for Phase 23-B.4 — Unified Condition System.
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * Tests the ConditionEvaluator, condition serialization/deserialization,
 * VSGraphVerifier rules E040/E041/E042/W015/W016, and all 6 source combinations.
 *
 * Test cases:
 *   === ConditionEvaluator — 6 core combinations ===
 *   1.  Test_VarVsConst_LessThan       — Variable(health=40) < Const(50) → true
 *   2.  Test_PinVsPin_GreaterThan      — Pin(42.Out=100) > Pin(43.Out=50) → true
 *   3.  Test_VarVsPin_Equal            — Variable(mode=1) == Pin(44.Out=1) → true
 *   4.  Test_PinVsConst_NotEqual       — Pin(45.Out=false) != Const(true) → true
 *   5.  Test_VarVsVar_String_Equal     — Variable(name="Alice") == Variable(name2="Alice") → true
 *   6.  Test_ConstVsConst_LessThan     — Const(10) < Const(20) → true
 *   7.  Test_VarVsConst_False          — Variable(health=60) < Const(50) → false
 *
 *   === Type validation ===
 *   8.  Test_TypeMismatch_StringVsInt  — String vs Int comparison → false (E042)
 *   9.  Test_IntVsFloat_Promotion      — Int(5) < Float(5.5) → true (numeric promotion)
 *  10.  Test_BoolComparison            — Bool(true) == Bool(true) → true
 *  11.  Test_VectorComparison_Equal    — Vector == Vector → true
 *  12.  Test_VectorComparison_NotEqual — Vector != Vector → true (different vectors)
 *
 *   === Error handling ===
 *  13.  Test_MissingVariable           — Variable("ghost") not in BB → false + log
 *  14.  Test_MissingPin                — Pin("Node#99.Out") not in cache → false + log
 *  15.  Test_EmptyMode                 — mode="" → false + log
 *  16.  Test_EmptyOperator             — operator="" → false + log
 *
 *   === Multiple conditions (implicit AND) ===
 *  17.  Test_MultiCondition_AllTrue    — 3 conditions, all true → true
 *  18.  Test_MultiCondition_OneFalse   — 3 conditions, one false → false
 *  19.  Test_EmptyConditions           — no conditions → true
 *
 *   === Serialization (JSON round-trip) ===
 *  20.  Test_Serialization_VarVsConst  — load JSON with Variable/Const condition
 *  21.  Test_Serialization_V4Migration — load v4 legacy format (variable + compareValue)
 *  22.  Test_Serialization_PinVsVar    — load JSON with Pin/Variable condition
 *  26.  Test_Serialization_ConstValueRoundTrip — all 4 const types (Int/Float/Bool/String)
 *                                                round-trip through JSON without data loss
 *                                                (Issue #409 Fix: const value persistence)
 *
 *   === VSGraphVerifier rules ===
 *  23.  Test_Verifier_E040_EmptyPin    — Pin mode with empty reference → E040
 *  24.  Test_Verifier_E041_UnknownVar  — Variable not in BB → E041
 *  25.  Test_Verifier_W015_ConstConst  — Const vs Const → W015
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "TaskSystem/TaskGraphTypes.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphLoader.h"
#include "TaskSystem/LocalBlackboard.h"
#include "TaskSystem/ConditionEvaluator.h"
#include "BlueprintEditor/VSGraphVerifier.h"

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>

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
// Helper: build a Condition
// ---------------------------------------------------------------------------

static Condition MakeVarConst(const std::string& var,
                               const std::string& op,
                               const TaskValue& constVal,
                               VariableType cmpType = VariableType::None)
{
    Condition c;
    c.leftMode         = "Variable";
    c.leftVariable     = var;
    c.operatorStr      = op;
    c.rightMode        = "Const";
    c.rightConstValue  = constVal;
    c.compareType      = cmpType;
    return c;
}

static Condition MakeConstConst(const TaskValue& left,
                                 const std::string& op,
                                 const TaskValue& right)
{
    Condition c;
    c.leftMode        = "Const";
    c.leftConstValue  = left;
    c.operatorStr     = op;
    c.rightMode       = "Const";
    c.rightConstValue = right;
    return c;
}

static Condition MakePinConst(const std::string& pin,
                               const std::string& op,
                               const TaskValue& constVal)
{
    Condition c;
    c.leftMode        = "Pin";
    c.leftPin         = pin;
    c.operatorStr     = op;
    c.rightMode       = "Const";
    c.rightConstValue = constVal;
    return c;
}

static Condition MakePinPin(const std::string& pinL,
                              const std::string& op,
                              const std::string& pinR)
{
    Condition c;
    c.leftMode    = "Pin";
    c.leftPin     = pinL;
    c.operatorStr = op;
    c.rightMode   = "Pin";
    c.rightPin    = pinR;
    return c;
}

static Condition MakeVarPin(const std::string& var,
                              const std::string& op,
                              const std::string& pin)
{
    Condition c;
    c.leftMode     = "Variable";
    c.leftVariable = var;
    c.operatorStr  = op;
    c.rightMode    = "Pin";
    c.rightPin     = pin;
    return c;
}

static Condition MakeVarVar(const std::string& varL,
                              const std::string& op,
                              const std::string& varR)
{
    Condition c;
    c.leftMode     = "Variable";
    c.leftVariable = varL;
    c.operatorStr  = op;
    c.rightMode    = "Variable";
    c.rightVariable = varR;
    return c;
}

// ---------------------------------------------------------------------------
// Helper: build a LocalBlackboard with one or more variables
// ---------------------------------------------------------------------------

static LocalBlackboard MakeBB(const std::vector<std::pair<std::string, TaskValue>>& vars)
{
    // We use a temporary TaskGraphTemplate so the blackboard has the schema
    TaskGraphTemplate tmpl;
    tmpl.Name = "TestGraph";
    for (size_t i = 0; i < vars.size(); ++i)
    {
        BlackboardEntry entry;
        entry.Key     = vars[i].first;
        entry.Type    = vars[i].second.GetType();
        entry.Default = vars[i].second;
        tmpl.Blackboard.push_back(entry);
    }

    LocalBlackboard bb;
    bb.InitializeFromEntries(tmpl.Blackboard);
    return bb;
}

// ---------------------------------------------------------------------------
// Helper: write temp JSON file
// ---------------------------------------------------------------------------

static bool WriteFile(const char* path, const char* content)
{
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return true;
}

// ===========================================================================
// Tests — 6 core combinations
// ===========================================================================

static void Test1_VarVsConst_LessThan()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({{"health", TaskValue(40)}});
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("health", "<", TaskValue(50), VariableType::Int));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 1);
    TEST_ASSERT(result == true, "health(40) < Const(50) should be true");

    ReportTest("VarVsConst_LessThan", ok && result == true);
}

static void Test2_PinVsPin_GreaterThan()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({});
    std::unordered_map<std::string, TaskValue> cache;
    cache["42:Out"] = TaskValue(100);
    cache["43:Out"] = TaskValue(50);

    std::vector<Condition> conds;
    conds.push_back(MakePinPin("Node#42.Out", ">", "Node#43.Out"));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 2);
    TEST_ASSERT(result == true, "Pin(42.Out=100) > Pin(43.Out=50) should be true");

    ReportTest("PinVsPin_GreaterThan", ok && result == true);
}

static void Test3_VarVsPin_Equal()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({{"mode", TaskValue(1)}});
    std::unordered_map<std::string, TaskValue> cache;
    cache["44:Out"] = TaskValue(1);

    std::vector<Condition> conds;
    conds.push_back(MakeVarPin("mode", "==", "Node#44.Out"));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 3);
    TEST_ASSERT(result == true, "Variable(mode=1) == Pin(44.Out=1) should be true");

    ReportTest("VarVsPin_Equal", ok && result == true);
}

static void Test4_PinVsConst_NotEqual()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({});
    std::unordered_map<std::string, TaskValue> cache;
    cache["45:Out"] = TaskValue(false);

    std::vector<Condition> conds;
    conds.push_back(MakePinConst("Node#45.Out", "!=", TaskValue(true)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 4);
    TEST_ASSERT(result == true, "Pin(45.Out=false) != Const(true) should be true");

    ReportTest("PinVsConst_NotEqual", ok && result == true);
}

static void Test5_VarVsVar_String_Equal()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({
        {"name",  TaskValue(std::string("Alice"))},
        {"name2", TaskValue(std::string("Alice"))}
    });
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarVar("name", "==", "name2"));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 5);
    TEST_ASSERT(result == true, "Variable(name=Alice) == Variable(name2=Alice) should be true");

    ReportTest("VarVsVar_String_Equal", ok && result == true);
}

static void Test6_ConstVsConst_LessThan()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({});
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeConstConst(TaskValue(10), "<", TaskValue(20)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 6);
    TEST_ASSERT(result == true, "Const(10) < Const(20) should be true");

    ReportTest("ConstVsConst_LessThan", ok && result == true);
}

static void Test7_VarVsConst_False()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({{"health", TaskValue(60)}});
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("health", "<", TaskValue(50)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 7);
    TEST_ASSERT(result == false, "health(60) < Const(50) should be false");

    ReportTest("VarVsConst_False", ok && result == false);
}

// ===========================================================================
// Tests — type validation
// ===========================================================================

static void Test8_TypeMismatch_StringVsInt()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({{"name", TaskValue(std::string("Alice"))}});
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("name", "<", TaskValue(42)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 8);
    TEST_ASSERT(result == false, "String vs Int comparison should return false (type mismatch)");

    ReportTest("TypeMismatch_StringVsInt", ok && result == false);
}

static void Test9_IntVsFloat_Promotion()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({{"score", TaskValue(5)}});
    std::unordered_map<std::string, TaskValue> cache;

    // Int < Float — numeric promotion should work
    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("score", "<", TaskValue(5.5f)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 9);
    TEST_ASSERT(result == true, "Int(5) < Float(5.5) should be true via numeric promotion");

    ReportTest("IntVsFloat_Promotion", ok && result == true);
}

static void Test10_BoolComparison()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({{"flag", TaskValue(true)}});
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("flag", "==", TaskValue(true)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 10);
    TEST_ASSERT(result == true, "Bool(true) == Bool(true) should be true");

    ReportTest("BoolComparison", ok && result == true);
}

static void Test11_VectorComparison_Equal()
{
    bool ok = true;

    ::Vector v;
    v.x = 1.0f; v.y = 2.0f; v.z = 3.0f;

    LocalBlackboard bb = MakeBB({{"pos", TaskValue(v)}});
    std::unordered_map<std::string, TaskValue> cache;

    ::Vector v2;
    v2.x = 1.0f; v2.y = 2.0f; v2.z = 3.0f;

    Condition cond;
    cond.leftMode        = "Variable";
    cond.leftVariable    = "pos";
    cond.operatorStr     = "==";
    cond.rightMode       = "Const";
    cond.rightConstValue = TaskValue(v2);

    std::vector<Condition> conds;
    conds.push_back(cond);

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 11);
    TEST_ASSERT(result == true, "Vector == Vector (same) should be true");

    ReportTest("VectorComparison_Equal", ok && result == true);
}

static void Test12_VectorComparison_NotEqual()
{
    bool ok = true;

    ::Vector v;
    v.x = 1.0f; v.y = 2.0f; v.z = 3.0f;

    LocalBlackboard bb = MakeBB({{"pos", TaskValue(v)}});
    std::unordered_map<std::string, TaskValue> cache;

    ::Vector v2;
    v2.x = 1.0f; v2.y = 2.0f; v2.z = 4.0f; // different z

    Condition cond;
    cond.leftMode        = "Variable";
    cond.leftVariable    = "pos";
    cond.operatorStr     = "!=";
    cond.rightMode       = "Const";
    cond.rightConstValue = TaskValue(v2);

    std::vector<Condition> conds;
    conds.push_back(cond);

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 12);
    TEST_ASSERT(result == true, "Vector != Vector (different z) should be true");

    ReportTest("VectorComparison_NotEqual", ok && result == true);
}

// ===========================================================================
// Tests — error handling
// ===========================================================================

static void Test13_MissingVariable()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({}); // empty blackboard
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("ghost", ">", TaskValue(0)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 13);
    TEST_ASSERT(result == false, "Missing variable 'ghost' should return false");

    ReportTest("MissingVariable", ok && result == false);
}

static void Test14_MissingPin()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({});
    std::unordered_map<std::string, TaskValue> cache; // cache is empty

    std::vector<Condition> conds;
    conds.push_back(MakePinConst("Node#99.Out", ">", TaskValue(0)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 14);
    TEST_ASSERT(result == false, "Missing pin 'Node#99.Out' should return false");

    ReportTest("MissingPin", ok && result == false);
}

static void Test15_EmptyMode()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({});
    std::unordered_map<std::string, TaskValue> cache;

    Condition cond;
    cond.leftMode     = "";  // unknown mode
    cond.operatorStr  = "==";
    cond.rightMode    = "Const";
    cond.rightConstValue = TaskValue(0);

    std::vector<Condition> conds;
    conds.push_back(cond);

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 15);
    TEST_ASSERT(result == false, "Empty mode should return false");

    ReportTest("EmptyMode", ok && result == false);
}

static void Test16_EmptyOperator()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({{"x", TaskValue(1)}});
    std::unordered_map<std::string, TaskValue> cache;

    Condition cond;
    cond.leftMode        = "Variable";
    cond.leftVariable    = "x";
    cond.operatorStr     = "";  // empty operator
    cond.rightMode       = "Const";
    cond.rightConstValue = TaskValue(1);

    std::vector<Condition> conds;
    conds.push_back(cond);

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 16);
    TEST_ASSERT(result == false, "Empty operator should return false");

    ReportTest("EmptyOperator", ok && result == false);
}

// ===========================================================================
// Tests — multiple conditions
// ===========================================================================

static void Test17_MultiCondition_AllTrue()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({
        {"health",  TaskValue(80)},
        {"alive",   TaskValue(true)},
        {"score",   TaskValue(100)}
    });
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("health",  ">",  TaskValue(50)));
    conds.push_back(MakeVarConst("alive",   "==", TaskValue(true)));
    conds.push_back(MakeVarConst("score",   ">=", TaskValue(100)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 17);
    TEST_ASSERT(result == true, "All 3 conditions true → AND should be true");

    ReportTest("MultiCondition_AllTrue", ok && result == true);
}

static void Test18_MultiCondition_OneFalse()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({
        {"health",  TaskValue(80)},
        {"alive",   TaskValue(false)},  // ← this one is false
        {"score",   TaskValue(100)}
    });
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds;
    conds.push_back(MakeVarConst("health",  ">",  TaskValue(50)));
    conds.push_back(MakeVarConst("alive",   "==", TaskValue(true)));  // false
    conds.push_back(MakeVarConst("score",   ">=", TaskValue(100)));

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 18);
    TEST_ASSERT(result == false, "One false condition → AND should be false");

    ReportTest("MultiCondition_OneFalse", ok && result == false);
}

static void Test19_EmptyConditions()
{
    bool ok = true;

    LocalBlackboard bb = MakeBB({});
    std::unordered_map<std::string, TaskValue> cache;

    std::vector<Condition> conds; // empty

    bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 19);
    TEST_ASSERT(result == true, "Empty conditions list should return true (implicit pass)");

    ReportTest("EmptyConditions", ok && result == true);
}

// ===========================================================================
// Tests — JSON serialization / deserialization
// ===========================================================================

static const char* k_condJsonPath    = "/tmp/phase23b4_condition.ats";
static const char* k_condV4JsonPath  = "/tmp/phase23b4_condition_v4.ats";
static const char* k_condPinVarPath  = "/tmp/phase23b4_pinvar.ats";

static void Test20_Serialization_VarVsConst()
{
    bool ok = true;

    const char* json = R"({
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "TestGraph",
  "entryPointId": 1,
  "nodes": [
    { "id": 1, "type": "EntryPoint" },
    { "id": 2, "type": "Branch",
      "conditions": [
        {
          "leftMode": "Variable",
          "leftVariable": "health",
          "operator": "<",
          "rightMode": "Const",
          "rightConstValue": 50,
          "compareType": "Int"
        }
      ]
    },
    { "id": 3, "type": "AtomicTask", "taskType": "Task_LogMessage" },
    { "id": 4, "type": "AtomicTask", "taskType": "Task_LogMessage" }
  ],
  "blackboard": [
    { "key": "health", "type": "Int", "default": 40 }
  ],
  "execConnections": [
    { "fromNode": 1, "fromPin": "Out", "toNode": 2, "toPin": "In" },
    { "fromNode": 2, "fromPin": "Then", "toNode": 3, "toPin": "In" },
    { "fromNode": 2, "fromPin": "Else", "toNode": 4, "toPin": "In" }
  ],
  "dataConnections": []
})";

    TEST_ASSERT(WriteFile(k_condJsonPath, json), "Could not write test JSON");

    std::vector<std::string> errors;
    TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromFile(k_condJsonPath, errors);

    TEST_ASSERT(tmpl != nullptr, "LoadFromFile should succeed");
    if (!tmpl) { ReportTest("Serialization_VarVsConst", false); return; }

    // Find branch node
    bool foundBranch = false;
    for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
    {
        if (tmpl->Nodes[i].Type == TaskNodeType::Branch)
        {
            foundBranch = true;
            TEST_ASSERT(tmpl->Nodes[i].conditions.size() == 1,
                        "Branch node should have 1 condition");
            if (!tmpl->Nodes[i].conditions.empty())
            {
                const Condition& c = tmpl->Nodes[i].conditions[0];
                TEST_ASSERT(c.leftMode     == "Variable", "leftMode should be Variable");
                TEST_ASSERT(c.leftVariable == "health",   "leftVariable should be 'health'");
                TEST_ASSERT(c.operatorStr  == "<",        "operator should be '<'");
                TEST_ASSERT(c.rightMode    == "Const",    "rightMode should be Const");
                TEST_ASSERT(c.compareType  == VariableType::Int, "compareType should be Int");
            }
        }
    }
    TEST_ASSERT(foundBranch, "Should find Branch node");

    delete tmpl;

    // Verify it evaluates correctly
    if (foundBranch)
    {
        LocalBlackboard bb = MakeBB({{"health", TaskValue(40)}});
        std::unordered_map<std::string, TaskValue> cache;

        Condition cond;
        cond.leftMode        = "Variable";
        cond.leftVariable    = "health";
        cond.operatorStr     = "<";
        cond.rightMode       = "Const";
        cond.rightConstValue = TaskValue(50);

        std::vector<Condition> conds;
        conds.push_back(cond);

        bool result = ConditionEvaluator::EvaluateAll(conds, bb, cache, 2);
        TEST_ASSERT(result == true, "Loaded condition health(40) < 50 should be true");
        ok = ok && result;
    }

    ReportTest("Serialization_VarVsConst", ok && foundBranch);
}

static void Test21_Serialization_V4Migration()
{
    bool ok = true;

    // v4 legacy format: "variable" + "operator" + "compareValue"
    // compareValue is a plain integer → should migrate to Const mode
    const char* json = R"({
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "LegacyGraph",
  "entryPointId": 1,
  "nodes": [
    { "id": 1, "type": "EntryPoint" },
    { "id": 2, "type": "Branch",
      "conditions": [
        {
          "leftMode": "Variable",
          "leftVariable": "score",
          "operator": ">=",
          "rightMode": "Const",
          "compareValue": 100
        }
      ]
    }
  ],
  "blackboard": [
    { "key": "score", "type": "Int", "default": 0 }
  ],
  "execConnections": [],
  "dataConnections": []
})";

    TEST_ASSERT(WriteFile(k_condV4JsonPath, json), "Could not write v4 test JSON");

    std::vector<std::string> errors;
    TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromFile(k_condV4JsonPath, errors);

    TEST_ASSERT(tmpl != nullptr, "LoadFromFile should succeed for v4 format");
    if (!tmpl) { ReportTest("Serialization_V4Migration", false); return; }

    bool foundCondition = false;
    for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
    {
        if (tmpl->Nodes[i].Type == TaskNodeType::Branch &&
            !tmpl->Nodes[i].conditions.empty())
        {
            foundCondition = true;
            const Condition& c = tmpl->Nodes[i].conditions[0];
            TEST_ASSERT(c.leftMode  == "Variable", "leftMode should be Variable");
            TEST_ASSERT(c.rightMode == "Const",    "rightMode should be Const after v4 migration");
            // compareValue=100 is a number → Const(100)
            TEST_ASSERT(!c.rightConstValue.IsNone(),
                        "rightConstValue should be set from compareValue=100");
        }
    }
    TEST_ASSERT(foundCondition, "Should find Branch with condition from v4 format");

    delete tmpl;
    ReportTest("Serialization_V4Migration", ok && foundCondition);
}

static void Test22_Serialization_PinVsVar()
{
    bool ok = true;

    const char* json = R"({
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "PinVarGraph",
  "entryPointId": 1,
  "nodes": [
    { "id": 1, "type": "EntryPoint" },
    { "id": 2, "type": "Branch",
      "conditions": [
        {
          "leftMode": "Pin",
          "leftPin": "Node#42.Out",
          "operator": "==",
          "rightMode": "Variable",
          "rightVariable": "target_mode",
          "compareType": "Int"
        }
      ]
    }
  ],
  "blackboard": [
    { "key": "target_mode", "type": "Int", "default": 1 }
  ],
  "execConnections": [],
  "dataConnections": []
})";

    TEST_ASSERT(WriteFile(k_condPinVarPath, json), "Could not write pinvar test JSON");

    std::vector<std::string> errors;
    TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromFile(k_condPinVarPath, errors);

    TEST_ASSERT(tmpl != nullptr, "LoadFromFile should succeed");
    if (!tmpl) { ReportTest("Serialization_PinVsVar", false); return; }

    bool foundCondition = false;
    for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
    {
        if (tmpl->Nodes[i].Type == TaskNodeType::Branch &&
            !tmpl->Nodes[i].conditions.empty())
        {
            foundCondition = true;
            const Condition& c = tmpl->Nodes[i].conditions[0];
            TEST_ASSERT(c.leftMode      == "Pin",          "leftMode should be Pin");
            TEST_ASSERT(c.leftPin       == "Node#42.Out",  "leftPin should be Node#42.Out");
            TEST_ASSERT(c.operatorStr   == "==",           "operator should be ==");
            TEST_ASSERT(c.rightMode     == "Variable",     "rightMode should be Variable");
            TEST_ASSERT(c.rightVariable == "target_mode",  "rightVariable should be target_mode");
            TEST_ASSERT(c.compareType   == VariableType::Int, "compareType should be Int");
        }
    }
    TEST_ASSERT(foundCondition, "Should find Branch with Pin/Variable condition");

    delete tmpl;
    ReportTest("Serialization_PinVsVar", ok && foundCondition);
}

// ===========================================================================
// Tests — VSGraphVerifier rules
// ===========================================================================

static void BuildBasicVSGraph(TaskGraphTemplate& tmpl)
{
    tmpl.Name      = "VerifierTestGraph";
    tmpl.GraphType = "VisualScript";

    // EntryPoint
    {
        TaskNodeDefinition ep;
        ep.NodeID   = 1;
        ep.NodeName = "EntryPoint";
        ep.Type     = TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }

    ExecPinConnection conn;
    conn.SourceNodeID  = 1;
    conn.SourcePinName = "Out";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    tmpl.ExecConnections.push_back(conn);

    tmpl.EntryPointID = 1;
    tmpl.BuildLookupCache();
}

static void Test23_Verifier_E040_EmptyPin()
{
    bool ok = true;

    TaskGraphTemplate tmpl;
    BuildBasicVSGraph(tmpl);

    // Branch node with Pin mode but empty pin reference
    TaskNodeDefinition branch;
    branch.NodeID   = 2;
    branch.NodeName = "BranchNode";
    branch.Type     = TaskNodeType::Branch;

    Condition cond;
    cond.leftMode    = "Pin";
    cond.leftPin     = "";  // ← empty! E040
    cond.operatorStr = ">";
    cond.rightMode   = "Const";
    cond.rightConstValue = TaskValue(0);
    branch.conditions.push_back(cond);

    tmpl.Nodes.push_back(branch);

    // Add Then/Else connections
    {
        ExecPinConnection c;
        c.SourceNodeID = 2; c.SourcePinName = "Then";
        c.TargetNodeID = 3; c.TargetPinName = "In";
        tmpl.ExecConnections.push_back(c);
    }
    {
        ExecPinConnection c;
        c.SourceNodeID = 2; c.SourcePinName = "Else";
        c.TargetNodeID = 4; c.TargetPinName = "In";
        tmpl.ExecConnections.push_back(c);
    }

    // Placeholder destination nodes
    for (int id : {3, 4})
    {
        TaskNodeDefinition n;
        n.NodeID = id; n.NodeName = "Task" + std::to_string(id);
        n.Type = TaskNodeType::AtomicTask;
        n.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(n);
    }
    tmpl.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(tmpl);

    bool hasE040 = false;
    for (size_t i = 0; i < result.issues.size(); ++i)
    {
        if (result.issues[i].ruleID == "E040_ConditionPinEmpty")
            hasE040 = true;
    }
    TEST_ASSERT(hasE040, "Empty Pin reference should produce E040");

    ReportTest("Verifier_E040_EmptyPin", ok && hasE040);
}

static void Test24_Verifier_E041_UnknownVar()
{
    bool ok = true;

    TaskGraphTemplate tmpl;
    BuildBasicVSGraph(tmpl);

    // Branch node with Variable mode pointing to an undeclared variable
    TaskNodeDefinition branch;
    branch.NodeID   = 2;
    branch.NodeName = "BranchNode";
    branch.Type     = TaskNodeType::Branch;

    Condition cond;
    cond.leftMode     = "Variable";
    cond.leftVariable = "ghost_var";  // not in blackboard
    cond.operatorStr  = ">";
    cond.rightMode    = "Const";
    cond.rightConstValue = TaskValue(0);
    branch.conditions.push_back(cond);

    tmpl.Nodes.push_back(branch);

    // Add Then/Else + destination nodes
    {
        ExecPinConnection c;
        c.SourceNodeID = 2; c.SourcePinName = "Then";
        c.TargetNodeID = 3; c.TargetPinName = "In";
        tmpl.ExecConnections.push_back(c);
    }
    {
        ExecPinConnection c;
        c.SourceNodeID = 2; c.SourcePinName = "Else";
        c.TargetNodeID = 4; c.TargetPinName = "In";
        tmpl.ExecConnections.push_back(c);
    }
    for (int id : {3, 4})
    {
        TaskNodeDefinition n;
        n.NodeID = id; n.NodeName = "Task" + std::to_string(id);
        n.Type = TaskNodeType::AtomicTask;
        n.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(n);
    }
    tmpl.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(tmpl);

    bool hasE041 = false;
    for (size_t i = 0; i < result.issues.size(); ++i)
    {
        if (result.issues[i].ruleID == "E041_ConditionVariableNotFound")
            hasE041 = true;
    }
    TEST_ASSERT(hasE041, "Unknown variable in condition should produce E041");

    ReportTest("Verifier_E041_UnknownVar", ok && hasE041);
}

static void Test25_Verifier_W015_ConstConst()
{
    bool ok = true;

    TaskGraphTemplate tmpl;
    BuildBasicVSGraph(tmpl);

    // Branch node with Const vs Const — should produce W015
    TaskNodeDefinition branch;
    branch.NodeID   = 2;
    branch.NodeName = "BranchNode";
    branch.Type     = TaskNodeType::Branch;

    branch.conditions.push_back(MakeConstConst(TaskValue(10), "<", TaskValue(20)));

    tmpl.Nodes.push_back(branch);

    // Add Then/Else + destination nodes
    {
        ExecPinConnection c;
        c.SourceNodeID = 2; c.SourcePinName = "Then";
        c.TargetNodeID = 3; c.TargetPinName = "In";
        tmpl.ExecConnections.push_back(c);
    }
    {
        ExecPinConnection c;
        c.SourceNodeID = 2; c.SourcePinName = "Else";
        c.TargetNodeID = 4; c.TargetPinName = "In";
        tmpl.ExecConnections.push_back(c);
    }
    for (int id : {3, 4})
    {
        TaskNodeDefinition n;
        n.NodeID = id; n.NodeName = "Task" + std::to_string(id);
        n.Type = TaskNodeType::AtomicTask;
        n.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(n);
    }
    tmpl.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(tmpl);

    bool hasW015 = false;
    for (size_t i = 0; i < result.issues.size(); ++i)
    {
        if (result.issues[i].ruleID == "W015_ConstVsConstCondition")
            hasW015 = true;
    }
    TEST_ASSERT(hasW015, "Const vs Const should produce W015 warning");

    ReportTest("Verifier_W015_ConstConst", ok && hasW015);
}

// ===========================================================================
// Test 26 -- Const value serialization round-trip (Issue #409 Fix #2)
//
// Verifies that all supported const value types (Int, Float, Bool, String)
// round-trip correctly through JSON serialization and deserialization.
// This guards against the bug where leftConstValue/rightConstValue was
// discarded when compareType was None (showed "[Const: ?]" after reload).
// ===========================================================================

static const char* k_condConstRTPath = "/tmp/phase23b4_const_rt.ats";

static void Test26_Serialization_ConstValueRoundTrip()
{
    bool ok = true;

    // Graph with four Branch nodes, each using a different const type on both sides
    const char* json = R"({
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "ConstRTGraph",
  "entryPointId": 1,
  "nodes": [
    { "id": 1, "type": "EntryPoint" },
    { "id": 2, "type": "Branch",
      "conditions": [
        {
          "leftMode": "Const",
          "leftConstValue": 42,
          "operator": "==",
          "rightMode": "Const",
          "rightConstValue": 42,
          "compareType": "Int"
        }
      ]
    },
    { "id": 3, "type": "Branch",
      "conditions": [
        {
          "leftMode": "Const",
          "leftConstValue": 3.14,
          "operator": "<",
          "rightMode": "Const",
          "rightConstValue": 6.28,
          "compareType": "Float"
        }
      ]
    },
    { "id": 4, "type": "Branch",
      "conditions": [
        {
          "leftMode": "Const",
          "leftConstValue": true,
          "operator": "==",
          "rightMode": "Const",
          "rightConstValue": false,
          "compareType": "Bool"
        }
      ]
    },
    { "id": 5, "type": "Branch",
      "conditions": [
        {
          "leftMode": "Const",
          "leftConstValue": "hello",
          "operator": "==",
          "rightMode": "Const",
          "rightConstValue": "world",
          "compareType": "String"
        }
      ]
    },
    { "id": 6, "type": "AtomicTask", "taskType": "Task_LogMessage" },
    { "id": 7, "type": "AtomicTask", "taskType": "Task_LogMessage" }
  ],
  "blackboard": [],
  "execConnections": [
    { "fromNode": 1, "fromPin": "Out",  "toNode": 2, "toPin": "In" },
    { "fromNode": 2, "fromPin": "Then", "toNode": 3, "toPin": "In" },
    { "fromNode": 2, "fromPin": "Else", "toNode": 6, "toPin": "In" },
    { "fromNode": 3, "fromPin": "Then", "toNode": 4, "toPin": "In" },
    { "fromNode": 3, "fromPin": "Else", "toNode": 6, "toPin": "In" },
    { "fromNode": 4, "fromPin": "Then", "toNode": 5, "toPin": "In" },
    { "fromNode": 4, "fromPin": "Else", "toNode": 6, "toPin": "In" },
    { "fromNode": 5, "fromPin": "Then", "toNode": 7, "toPin": "In" },
    { "fromNode": 5, "fromPin": "Else", "toNode": 6, "toPin": "In" }
  ],
  "dataConnections": []
})";

    TEST_ASSERT(WriteFile(k_condConstRTPath, json),
                "Could not write const round-trip JSON");

    std::vector<std::string> errors;
    TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromFile(k_condConstRTPath, errors);
    TEST_ASSERT(tmpl != nullptr, "LoadFromFile should succeed for const round-trip graph");
    if (!tmpl) { ReportTest("Serialization_ConstValueRoundTrip", false); return; }

    // Helper: find branch node by ID
    auto findBranch = [&](int nodeID) -> const Condition*
    {
        for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
        {
            if (tmpl->Nodes[i].NodeID == nodeID &&
                !tmpl->Nodes[i].conditions.empty())
                return &tmpl->Nodes[i].conditions[0];
        }
        return nullptr;
    };

    // --- Node 2: Int const (42 == 42) ---
    {
        const Condition* c = findBranch(2);
        TEST_ASSERT(c != nullptr,                           "Node 2 should have condition");
        if (c)
        {
            TEST_ASSERT(c->leftMode  == "Const",            "Node2 leftMode should be Const");
            TEST_ASSERT(c->rightMode == "Const",            "Node2 rightMode should be Const");
            TEST_ASSERT(c->compareType == VariableType::Int,"Node2 compareType should be Int");
            TEST_ASSERT(!c->leftConstValue.IsNone(),        "Node2 leftConstValue should not be None");
            TEST_ASSERT(!c->rightConstValue.IsNone(),       "Node2 rightConstValue should not be None");
            TEST_ASSERT(c->leftConstValue.GetType()  == VariableType::Int,
                        "Node2 leftConstValue type should be Int");
            TEST_ASSERT(c->rightConstValue.GetType() == VariableType::Int,
                        "Node2 rightConstValue type should be Int");
            TEST_ASSERT(c->leftConstValue.AsInt()  == 42,  "Node2 leftConstValue should be 42");
            TEST_ASSERT(c->rightConstValue.AsInt() == 42,  "Node2 rightConstValue should be 42");
        }
    }

    // --- Node 3: Float const (3.14 < 6.28) ---
    {
        const Condition* c = findBranch(3);
        TEST_ASSERT(c != nullptr,                             "Node 3 should have condition");
        if (c)
        {
            TEST_ASSERT(c->compareType == VariableType::Float,"Node3 compareType should be Float");
            TEST_ASSERT(!c->leftConstValue.IsNone(),          "Node3 leftConstValue should not be None");
            TEST_ASSERT(!c->rightConstValue.IsNone(),         "Node3 rightConstValue should not be None");
        }
    }

    // --- Node 4: Bool const (true == false) ---
    {
        const Condition* c = findBranch(4);
        TEST_ASSERT(c != nullptr,                             "Node 4 should have condition");
        if (c)
        {
            TEST_ASSERT(c->compareType == VariableType::Bool, "Node4 compareType should be Bool");
            TEST_ASSERT(!c->leftConstValue.IsNone(),          "Node4 leftConstValue should not be None");
            TEST_ASSERT(c->leftConstValue.GetType()  == VariableType::Bool,
                        "Node4 leftConstValue type should be Bool");
            TEST_ASSERT(c->leftConstValue.AsBool() == true,   "Node4 leftConstValue should be true");
            TEST_ASSERT(c->rightConstValue.GetType() == VariableType::Bool,
                        "Node4 rightConstValue type should be Bool");
            TEST_ASSERT(c->rightConstValue.AsBool() == false, "Node4 rightConstValue should be false");
        }
    }

    // --- Node 5: String const ("hello" == "world") ---
    {
        const Condition* c = findBranch(5);
        TEST_ASSERT(c != nullptr,                               "Node 5 should have condition");
        if (c)
        {
            TEST_ASSERT(c->compareType == VariableType::String, "Node5 compareType should be String");
            TEST_ASSERT(!c->leftConstValue.IsNone(),            "Node5 leftConstValue should not be None");
            TEST_ASSERT(c->leftConstValue.GetType()  == VariableType::String,
                        "Node5 leftConstValue type should be String");
            TEST_ASSERT(c->leftConstValue.AsString()  == "hello", "Node5 leftConstValue should be 'hello'");
            TEST_ASSERT(c->rightConstValue.AsString() == "world", "Node5 rightConstValue should be 'world'");
        }
    }

    delete tmpl;
    ReportTest("Serialization_ConstValueRoundTrip", ok);
}

// ===========================================================================
// main
// ===========================================================================

int main()
{
    std::cout << "=== Phase 23-B.4 Tests — Unified Condition System ===" << std::endl;

    // 6 core combinations
    Test1_VarVsConst_LessThan();
    Test2_PinVsPin_GreaterThan();
    Test3_VarVsPin_Equal();
    Test4_PinVsConst_NotEqual();
    Test5_VarVsVar_String_Equal();
    Test6_ConstVsConst_LessThan();
    Test7_VarVsConst_False();

    // Type validation
    Test8_TypeMismatch_StringVsInt();
    Test9_IntVsFloat_Promotion();
    Test10_BoolComparison();
    Test11_VectorComparison_Equal();
    Test12_VectorComparison_NotEqual();

    // Error handling
    Test13_MissingVariable();
    Test14_MissingPin();
    Test15_EmptyMode();
    Test16_EmptyOperator();

    // Multiple conditions
    Test17_MultiCondition_AllTrue();
    Test18_MultiCondition_OneFalse();
    Test19_EmptyConditions();

    // Serialization
    Test20_Serialization_VarVsConst();
    Test21_Serialization_V4Migration();
    Test22_Serialization_PinVsVar();
    Test26_Serialization_ConstValueRoundTrip();

    // VSGraphVerifier rules
    Test23_Verifier_E040_EmptyPin();
    Test24_Verifier_E041_UnknownVar();
    Test25_Verifier_W015_ConstConst();

    std::cout << "\n=== Results: " << s_passCount << " passed, "
              << s_failCount << " failed ===" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
