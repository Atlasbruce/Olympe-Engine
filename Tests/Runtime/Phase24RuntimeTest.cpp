/**
 * @file Phase24RuntimeTest.cpp
 * @brief Unit tests for Phase 24.5 — Runtime Condition Evaluation System.
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * Tests (43):
 *
 * --- RuntimeEnvironment (Tests 1-8) ---
 *   1.  SetAndGetBlackboardVariable      — stores and retrieves a float
 *   2.  HasBlackboardVariable            — presence check
 *   3.  GetBlackboardVariable_NotFound   — returns false for missing key
 *   4.  SetAndGetDynamicPinValue         — stores and retrieves a pin value
 *   5.  GetDynamicPinValue_NotFound      — returns false for missing pin
 *   6.  OverwriteBlackboardVariable      — last write wins
 *   7.  OverwriteDynamicPinValue         — last write wins
 *   8.  Clear                            — removes all entries
 *
 * --- ConditionEvaluator — operators (Tests 9-14) ---
 *   9.  Operator_Equal_True              — 5 == 5
 *   10. Operator_Equal_False             — 5 == 6
 *   11. Operator_NotEqual_True           — 5 != 6
 *   12. Operator_NotEqual_False          — 5 != 5
 *   13. Operator_Less_True               — 3 < 5
 *   14. Operator_Less_False              — 5 < 3
 *   15. Operator_LessEqual_True_Equal    — 5 <= 5
 *   16. Operator_LessEqual_True_Less     — 3 <= 5
 *   17. Operator_LessEqual_False         — 6 <= 5
 *   18. Operator_Greater_True            — 6 > 5
 *   19. Operator_Greater_False           — 3 > 5
 *   20. Operator_GreaterEqual_True_Equal — 5 >= 5
 *   21. Operator_GreaterEqual_True_Greater — 6 >= 5
 *   22. Operator_GreaterEqual_False      — 3 >= 5
 *
 * --- ConditionEvaluator — operand resolution (Tests 23-27) ---
 *   23. Resolve_Const                    — Const operand returns constValue
 *   24. Resolve_Variable_Found           — Variable resolved from Blackboard
 *   25. Resolve_Variable_NotFound_Error  — missing Blackboard key → error
 *   26. Resolve_Pin_Found                — Pin resolved from dynamic pin map
 *   27. Resolve_Pin_NotFound_Error       — missing pin ID → error
 *
 * --- ConditionEvaluator — edge cases (Tests 28-33) ---
 *   28. EdgeCase_NaN_Equal               — NaN == NaN is false (IEEE 754)
 *   29. EdgeCase_NaN_NotEqual            — NaN != NaN is true  (IEEE 754)
 *   30. EdgeCase_Infinity_Greater        — Inf > 1e30 is true
 *   31. EdgeCase_NegativeInfinity_Less   — -Inf < -1e30 is true
 *   32. EdgeCase_LargeNumbers            — 1e38 > 1e37 is true
 *   33. EdgeCase_Zero_Comparison         — 0 == 0, 0 < 1, -1 < 0
 *
 * --- ConditionEvaluator — error handling (Tests 34-36) ---
 *   34. Error_MissingVariable_ReturnsFalse — evaluate returns false + error
 *   35. Error_MissingPin_ReturnsFalse      — evaluate returns false + error
 *   36. Error_EmptyErrorOnSuccess          — outErrorMsg is empty on success
 *
 * --- GraphRuntimeInstance (Tests 37-43) ---
 *   37. StartExecution_EmptyGraph        — no crash on empty graph
 *   38. StartExecution_SingleNode        — IsExecuting true after start
 *   39. StepExecution_AdvancesStack      — moves to NextOnSuccess
 *   40. StepExecution_ReachesEnd         — IsExecuting false when stack empty
 *   41. Breakpoint_PausesExecution       — hits breakpoint, stays paused
 *   42. ResumeFromBreakpoint             — resumes and processes the node
 *   43. GetActiveNodeIDs_ReflectsStack   — stack contents visible
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Runtime/RuntimeEnvironment.h"
#include "Runtime/ConditionPresetEvaluator.h"
#include "Runtime/GraphRuntimeInstance.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/Operand.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure (same pattern as other Phase 24 tests)
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
// Helper: build a simple linear TaskGraphTemplate
//   nodeA (root) → nodeB → (end)
// ---------------------------------------------------------------------------

static TaskGraphTemplate MakeLinearGraph(int32_t idA, int32_t idB)
{
    TaskGraphTemplate g;
    g.Name        = "TestGraph";
    g.RootNodeID  = idA;
    g.EntryPointID = NODE_INDEX_NONE;

    TaskNodeDefinition nodeA;
    nodeA.NodeID        = idA;
    nodeA.Type          = TaskNodeType::AtomicTask;
    nodeA.NextOnSuccess = idB;
    nodeA.NextOnFailure = NODE_INDEX_NONE;

    TaskNodeDefinition nodeB;
    nodeB.NodeID        = idB;
    nodeB.Type          = TaskNodeType::AtomicTask;
    nodeB.NextOnSuccess = NODE_INDEX_NONE;
    nodeB.NextOnFailure = NODE_INDEX_NONE;

    g.Nodes.push_back(nodeA);
    g.Nodes.push_back(nodeB);
    g.BuildLookupCache();
    return g;
}

// ---------------------------------------------------------------------------
// Helper: build a branch graph
//   nodeA (root, Branch) → nodeB (then) or nodeC (else)
// ---------------------------------------------------------------------------

static TaskGraphTemplate MakeBranchGraph(int32_t idA, int32_t idB, int32_t idC)
{
    TaskGraphTemplate g;
    g.Name       = "BranchGraph";
    g.RootNodeID = idA;
    g.EntryPointID = NODE_INDEX_NONE;

    TaskNodeDefinition branch;
    branch.NodeID        = idA;
    branch.Type          = TaskNodeType::Branch;
    branch.NextOnSuccess = idB; // "Then" path
    branch.NextOnFailure = idC; // "Else" path

    TaskNodeDefinition thenNode;
    thenNode.NodeID        = idB;
    thenNode.Type          = TaskNodeType::AtomicTask;
    thenNode.NextOnSuccess = NODE_INDEX_NONE;

    TaskNodeDefinition elseNode;
    elseNode.NodeID        = idC;
    elseNode.Type          = TaskNodeType::AtomicTask;
    elseNode.NextOnSuccess = NODE_INDEX_NONE;

    g.Nodes.push_back(branch);
    g.Nodes.push_back(thenNode);
    g.Nodes.push_back(elseNode);
    g.BuildLookupCache();
    return g;
}

// ===========================================================================
// RuntimeEnvironment tests (1-8)
// ===========================================================================

static void Test1_SetAndGetBlackboardVariable()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("mHealth", 10.0f);

    float v = 0.0f;
    bool found = env.GetBlackboardVariable("mHealth", v);

    bool ok = found && (v == 10.0f);
    TEST_ASSERT(found, "Variable should be found");
    TEST_ASSERT(v == 10.0f, "Value should be 10.0f");
    ReportTest("SetAndGetBlackboardVariable", ok);
}

static void Test2_HasBlackboardVariable()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("mSpeed", 5.0f);

    bool ok = env.HasBlackboardVariable("mSpeed")
           && !env.HasBlackboardVariable("missing");
    TEST_ASSERT(env.HasBlackboardVariable("mSpeed"),    "mSpeed should exist");
    TEST_ASSERT(!env.HasBlackboardVariable("missing"),  "missing should not exist");
    ReportTest("HasBlackboardVariable", ok);
}

static void Test3_GetBlackboardVariable_NotFound()
{
    RuntimeEnvironment env;
    float v = 42.0f;
    bool found = env.GetBlackboardVariable("nokey", v);

    bool ok = !found && (v == 42.0f); // outValue unchanged on miss
    TEST_ASSERT(!found, "Should return false for missing key");
    TEST_ASSERT(v == 42.0f, "outValue should not be modified on miss");
    ReportTest("GetBlackboardVariable_NotFound", ok);
}

static void Test4_SetAndGetDynamicPinValue()
{
    RuntimeEnvironment env;
    env.SetDynamicPinValue("pin_abc", 3.14f);

    float v = 0.0f;
    bool found = env.GetDynamicPinValue("pin_abc", v);

    bool ok = found && (v == 3.14f);
    TEST_ASSERT(found, "Pin should be found");
    TEST_ASSERT(v == 3.14f, "Pin value should be 3.14f");
    ReportTest("SetAndGetDynamicPinValue", ok);
}

static void Test5_GetDynamicPinValue_NotFound()
{
    RuntimeEnvironment env;
    float v = 99.0f;
    bool found = env.GetDynamicPinValue("nopin", v);

    bool ok = !found && (v == 99.0f);
    TEST_ASSERT(!found, "Should return false for missing pin");
    TEST_ASSERT(v == 99.0f, "outValue should not be modified on miss");
    ReportTest("GetDynamicPinValue_NotFound", ok);
}

static void Test6_OverwriteBlackboardVariable()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("x", 1.0f);
    env.SetBlackboardVariable("x", 2.0f);

    float v = 0.0f;
    env.GetBlackboardVariable("x", v);

    bool ok = (v == 2.0f);
    TEST_ASSERT(v == 2.0f, "Last write should win (2.0f)");
    ReportTest("OverwriteBlackboardVariable", ok);
}

static void Test7_OverwriteDynamicPinValue()
{
    RuntimeEnvironment env;
    env.SetDynamicPinValue("pin", 1.0f);
    env.SetDynamicPinValue("pin", 9.0f);

    float v = 0.0f;
    env.GetDynamicPinValue("pin", v);

    bool ok = (v == 9.0f);
    TEST_ASSERT(v == 9.0f, "Last write should win (9.0f)");
    ReportTest("OverwriteDynamicPinValue", ok);
}

static void Test8_Clear()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("k", 1.0f);
    env.SetDynamicPinValue("p", 2.0f);
    env.Clear();

    bool ok = !env.HasBlackboardVariable("k");
    float v = 0.0f;
    bool pinFound = env.GetDynamicPinValue("p", v);

    ok = ok && !pinFound;
    TEST_ASSERT(!env.HasBlackboardVariable("k"), "Blackboard should be cleared");
    TEST_ASSERT(!pinFound,                       "Pin map should be cleared");
    ReportTest("Clear", ok);
}

// ===========================================================================
// ConditionEvaluator — operator tests (9-22)
// ===========================================================================

static void EvalOperatorTest(
    const std::string& name,
    float left,
    ComparisonOp op,
    float right,
    bool expectedResult)
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("L", left);
    env.SetBlackboardVariable("R", right);

    ConditionPreset preset("p",
        Operand::CreateVariable("L"),
        op,
        Operand::CreateVariable("R"));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = (result == expectedResult) && err.empty();
    TEST_ASSERT(result == expectedResult,
                name + ": expected " + (expectedResult ? "true" : "false")
                     + " got " + (result ? "true" : "false"));
    TEST_ASSERT(err.empty(), name + ": unexpected error: " + err);
    ReportTest(name, ok);
}

static void Test9_Operator_Equal_True()
{
    EvalOperatorTest("Operator_Equal_True", 5.0f, ComparisonOp::Equal, 5.0f, true);
}

static void Test10_Operator_Equal_False()
{
    EvalOperatorTest("Operator_Equal_False", 5.0f, ComparisonOp::Equal, 6.0f, false);
}

static void Test11_Operator_NotEqual_True()
{
    EvalOperatorTest("Operator_NotEqual_True", 5.0f, ComparisonOp::NotEqual, 6.0f, true);
}

static void Test12_Operator_NotEqual_False()
{
    EvalOperatorTest("Operator_NotEqual_False", 5.0f, ComparisonOp::NotEqual, 5.0f, false);
}

static void Test13_Operator_Less_True()
{
    EvalOperatorTest("Operator_Less_True", 3.0f, ComparisonOp::Less, 5.0f, true);
}

static void Test14_Operator_Less_False()
{
    EvalOperatorTest("Operator_Less_False", 5.0f, ComparisonOp::Less, 3.0f, false);
}

static void Test15_Operator_LessEqual_True_Equal()
{
    EvalOperatorTest("Operator_LessEqual_True_Equal", 5.0f, ComparisonOp::LessEqual, 5.0f, true);
}

static void Test16_Operator_LessEqual_True_Less()
{
    EvalOperatorTest("Operator_LessEqual_True_Less", 3.0f, ComparisonOp::LessEqual, 5.0f, true);
}

static void Test17_Operator_LessEqual_False()
{
    EvalOperatorTest("Operator_LessEqual_False", 6.0f, ComparisonOp::LessEqual, 5.0f, false);
}

static void Test18_Operator_Greater_True()
{
    EvalOperatorTest("Operator_Greater_True", 6.0f, ComparisonOp::Greater, 5.0f, true);
}

static void Test19_Operator_Greater_False()
{
    EvalOperatorTest("Operator_Greater_False", 3.0f, ComparisonOp::Greater, 5.0f, false);
}

static void Test20_Operator_GreaterEqual_True_Equal()
{
    EvalOperatorTest("Operator_GreaterEqual_True_Equal", 5.0f, ComparisonOp::GreaterEqual, 5.0f, true);
}

static void Test21_Operator_GreaterEqual_True_Greater()
{
    EvalOperatorTest("Operator_GreaterEqual_True_Greater", 6.0f, ComparisonOp::GreaterEqual, 5.0f, true);
}

static void Test22_Operator_GreaterEqual_False()
{
    EvalOperatorTest("Operator_GreaterEqual_False", 3.0f, ComparisonOp::GreaterEqual, 5.0f, false);
}

// ===========================================================================
// ConditionEvaluator — operand resolution (23-27)
// ===========================================================================

static void Test23_Resolve_Const()
{
    RuntimeEnvironment env;
    ConditionPreset preset("p",
        Operand::CreateConst(7.0),
        ComparisonOp::Equal,
        Operand::CreateConst(7.0));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result && err.empty();
    TEST_ASSERT(result,     "Const operand 7 == 7 should be true");
    TEST_ASSERT(err.empty(), "No error expected");
    ReportTest("Resolve_Const", ok);
}

static void Test24_Resolve_Variable_Found()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("mHealth", 2.0f);

    ConditionPreset preset("p",
        Operand::CreateVariable("mHealth"),
        ComparisonOp::LessEqual,
        Operand::CreateConst(2.0));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result && err.empty();
    TEST_ASSERT(result,     "mHealth(2) <= 2 should be true");
    TEST_ASSERT(err.empty(), "No error expected");
    ReportTest("Resolve_Variable_Found", ok);
}

static void Test25_Resolve_Variable_NotFound_Error()
{
    RuntimeEnvironment env; // mHealth not set

    ConditionPreset preset("p",
        Operand::CreateVariable("mHealth"),
        ComparisonOp::Equal,
        Operand::CreateConst(0.0));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = !result && !err.empty();
    TEST_ASSERT(!result,    "Should return false when variable missing");
    TEST_ASSERT(!err.empty(), "Should produce an error message");
    ReportTest("Resolve_Variable_NotFound_Error", ok);
}

static void Test26_Resolve_Pin_Found()
{
    RuntimeEnvironment env;
    env.SetDynamicPinValue("Pin:1", 42.0f);

    ConditionPreset preset("p",
        Operand::CreatePin("Pin:1"),
        ComparisonOp::Equal,
        Operand::CreateConst(42.0));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result && err.empty();
    TEST_ASSERT(result,     "Pin(42) == 42 should be true");
    TEST_ASSERT(err.empty(), "No error expected");
    ReportTest("Resolve_Pin_Found", ok);
}

static void Test27_Resolve_Pin_NotFound_Error()
{
    RuntimeEnvironment env; // Pin:1 not set

    ConditionPreset preset("p",
        Operand::CreatePin("Pin:1"),
        ComparisonOp::Equal,
        Operand::CreateConst(0.0));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = !result && !err.empty();
    TEST_ASSERT(!result,    "Should return false when pin missing");
    TEST_ASSERT(!err.empty(), "Should produce an error message");
    ReportTest("Resolve_Pin_NotFound_Error", ok);
}

// ===========================================================================
// ConditionEvaluator — edge cases (28-33)
// ===========================================================================

static void Test28_EdgeCase_NaN_Equal()
{
    // IEEE 754: NaN == NaN is false
    const float nan = std::numeric_limits<float>::quiet_NaN();
    RuntimeEnvironment env;
    env.SetBlackboardVariable("x", nan);
    env.SetBlackboardVariable("y", nan);

    ConditionPreset preset("p",
        Operand::CreateVariable("x"),
        ComparisonOp::Equal,
        Operand::CreateVariable("y"));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = !result; // NaN == NaN is false per IEEE 754
    TEST_ASSERT(!result, "NaN == NaN should be false (IEEE 754)");
    ReportTest("EdgeCase_NaN_Equal", ok);
}

static void Test29_EdgeCase_NaN_NotEqual()
{
    // IEEE 754: NaN != NaN is true
    const float nan = std::numeric_limits<float>::quiet_NaN();
    RuntimeEnvironment env;
    env.SetBlackboardVariable("x", nan);
    env.SetBlackboardVariable("y", nan);

    ConditionPreset preset("p",
        Operand::CreateVariable("x"),
        ComparisonOp::NotEqual,
        Operand::CreateVariable("y"));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result; // NaN != NaN is true per IEEE 754
    TEST_ASSERT(result, "NaN != NaN should be true (IEEE 754)");
    ReportTest("EdgeCase_NaN_NotEqual", ok);
}

static void Test30_EdgeCase_Infinity_Greater()
{
    const float inf = std::numeric_limits<float>::infinity();
    RuntimeEnvironment env;
    env.SetBlackboardVariable("big", inf);
    env.SetBlackboardVariable("large", 1e30f);

    ConditionPreset preset("p",
        Operand::CreateVariable("big"),
        ComparisonOp::Greater,
        Operand::CreateVariable("large"));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result;
    TEST_ASSERT(result, "Infinity > 1e30 should be true");
    ReportTest("EdgeCase_Infinity_Greater", ok);
}

static void Test31_EdgeCase_NegativeInfinity_Less()
{
    const float neg_inf = -std::numeric_limits<float>::infinity();
    RuntimeEnvironment env;
    env.SetBlackboardVariable("small", neg_inf);
    env.SetBlackboardVariable("neg",   -1e30f);

    ConditionPreset preset("p",
        Operand::CreateVariable("small"),
        ComparisonOp::Less,
        Operand::CreateVariable("neg"));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result;
    TEST_ASSERT(result, "-Infinity < -1e30 should be true");
    ReportTest("EdgeCase_NegativeInfinity_Less", ok);
}

static void Test32_EdgeCase_LargeNumbers()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("big",   1e38f);
    env.SetBlackboardVariable("small", 1e37f);

    ConditionPreset preset("p",
        Operand::CreateVariable("big"),
        ComparisonOp::Greater,
        Operand::CreateVariable("small"));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result;
    TEST_ASSERT(result, "1e38 > 1e37 should be true");
    ReportTest("EdgeCase_LargeNumbers", ok);
}

static void Test33_EdgeCase_Zero_Comparison()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("zero", 0.0f);
    env.SetBlackboardVariable("one",  1.0f);
    env.SetBlackboardVariable("neg",  -1.0f);

    std::string err;

    // 0 == 0
    ConditionPreset p1("p1", Operand::CreateVariable("zero"),
                       ComparisonOp::Equal, Operand::CreateVariable("zero"));
    bool r1 = ConditionEvaluator::Evaluate(p1, env, err);

    // 0 < 1
    ConditionPreset p2("p2", Operand::CreateVariable("zero"),
                       ComparisonOp::Less, Operand::CreateVariable("one"));
    bool r2 = ConditionEvaluator::Evaluate(p2, env, err);

    // -1 < 0
    ConditionPreset p3("p3", Operand::CreateVariable("neg"),
                       ComparisonOp::Less, Operand::CreateVariable("zero"));
    bool r3 = ConditionEvaluator::Evaluate(p3, env, err);

    bool ok = r1 && r2 && r3;
    TEST_ASSERT(r1, "0 == 0 should be true");
    TEST_ASSERT(r2, "0 < 1 should be true");
    TEST_ASSERT(r3, "-1 < 0 should be true");
    ReportTest("EdgeCase_Zero_Comparison", ok);
}

// ===========================================================================
// ConditionEvaluator — error handling (34-36)
// ===========================================================================

static void Test34_Error_MissingVariable_ReturnsFalse()
{
    RuntimeEnvironment env;
    ConditionPreset preset("p",
        Operand::CreateVariable("MISSING"),
        ComparisonOp::Equal,
        Operand::CreateConst(0.0));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = !result && !err.empty();
    TEST_ASSERT(!result,    "Should return false for missing variable");
    TEST_ASSERT(!err.empty(), "Error message should be non-empty");
    ReportTest("Error_MissingVariable_ReturnsFalse", ok);
}

static void Test35_Error_MissingPin_ReturnsFalse()
{
    RuntimeEnvironment env;
    ConditionPreset preset("p",
        Operand::CreatePin("MISSING_PIN"),
        ComparisonOp::Equal,
        Operand::CreateConst(0.0));

    std::string err;
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = !result && !err.empty();
    TEST_ASSERT(!result,    "Should return false for missing pin");
    TEST_ASSERT(!err.empty(), "Error message should be non-empty");
    ReportTest("Error_MissingPin_ReturnsFalse", ok);
}

static void Test36_Error_EmptyErrorOnSuccess()
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("x", 1.0f);

    ConditionPreset preset("p",
        Operand::CreateVariable("x"),
        ComparisonOp::Greater,
        Operand::CreateConst(0.0));

    std::string err = "previously set error";
    bool result = ConditionEvaluator::Evaluate(preset, env, err);

    bool ok = result && err.empty();
    TEST_ASSERT(result,     "1 > 0 should be true");
    TEST_ASSERT(err.empty(), "Error string should be cleared on success");
    ReportTest("Error_EmptyErrorOnSuccess", ok);
}

// ===========================================================================
// GraphRuntimeInstance tests (37-43)
// ===========================================================================

static void Test37_StartExecution_EmptyGraph()
{
    // A graph with no root should not crash.
    TaskGraphTemplate g;
    g.Name       = "Empty";
    g.RootNodeID = NODE_INDEX_NONE;
    g.EntryPointID = NODE_INDEX_NONE;
    g.BuildLookupCache();

    GraphRuntimeInstance inst(g);
    inst.StartExecution();

    bool ok = !inst.IsExecuting();
    TEST_ASSERT(!inst.IsExecuting(), "Empty graph should not be executing");
    ReportTest("StartExecution_EmptyGraph", ok);
}

static void Test38_StartExecution_SingleNode()
{
    TaskGraphTemplate g;
    g.Name       = "Single";
    g.RootNodeID = 1;
    g.EntryPointID = NODE_INDEX_NONE;

    TaskNodeDefinition n;
    n.NodeID        = 1;
    n.Type          = TaskNodeType::AtomicTask;
    n.NextOnSuccess = NODE_INDEX_NONE;
    g.Nodes.push_back(n);
    g.BuildLookupCache();

    GraphRuntimeInstance inst(g);
    inst.StartExecution();

    bool ok = inst.IsExecuting();
    TEST_ASSERT(inst.IsExecuting(), "Should be executing after start");
    ReportTest("StartExecution_SingleNode", ok);
}

static void Test39_StepExecution_AdvancesStack()
{
    TaskGraphTemplate g = MakeLinearGraph(1, 2);

    GraphRuntimeInstance inst(g);
    inst.StartExecution();

    const std::vector<int32_t>& stack = inst.GetActiveNodeIDs();
    TEST_ASSERT(!stack.empty() && stack.front() == 1,
                "Initial stack should contain node 1");

    bool stillRunning = inst.StepExecution();
    TEST_ASSERT(stillRunning, "Should still be executing after step 1");

    const std::vector<int32_t>& stack2 = inst.GetActiveNodeIDs();
    bool ok = !stack2.empty() && (stack2.front() == 2);
    TEST_ASSERT(ok, "After step 1, stack should contain node 2");
    ReportTest("StepExecution_AdvancesStack", ok);
}

static void Test40_StepExecution_ReachesEnd()
{
    TaskGraphTemplate g = MakeLinearGraph(1, 2);

    GraphRuntimeInstance inst(g);
    inst.StartExecution();

    inst.StepExecution(); // node 1 → node 2
    bool done = !inst.StepExecution(); // node 2 → end

    bool ok = done && !inst.IsExecuting();
    TEST_ASSERT(done,               "StepExecution should return false at end");
    TEST_ASSERT(!inst.IsExecuting(), "IsExecuting should be false at end");
    ReportTest("StepExecution_ReachesEnd", ok);
}

static void Test41_Breakpoint_PausesExecution()
{
    TaskGraphTemplate g = MakeLinearGraph(1, 2);

    GraphRuntimeInstance inst(g);
    inst.AddBreakpoint(1); // break before processing node 1
    inst.StartExecution();

    bool notRunning = !inst.StepExecution(); // should pause at node 1
    bool hitBP      = inst.IsBreakpointHit();

    bool ok = notRunning && hitBP;
    TEST_ASSERT(notRunning, "StepExecution should return false at breakpoint");
    TEST_ASSERT(hitBP,      "IsBreakpointHit should be true");
    ReportTest("Breakpoint_PausesExecution", ok);
}

static void Test42_ResumeFromBreakpoint()
{
    TaskGraphTemplate g = MakeLinearGraph(1, 2);

    GraphRuntimeInstance inst(g);
    inst.AddBreakpoint(1);
    inst.StartExecution();

    inst.StepExecution();         // pauses at node 1
    inst.ResumeFromBreakpoint();  // clear the pause flag

    bool stillRunning = inst.StepExecution(); // now processes node 1 → node 2
    bool ok = stillRunning && !inst.IsBreakpointHit();
    TEST_ASSERT(stillRunning,           "Should be running after resume");
    TEST_ASSERT(!inst.IsBreakpointHit(), "Breakpoint flag should be cleared");
    ReportTest("ResumeFromBreakpoint", ok);
}

static void Test43_GetActiveNodeIDs_ReflectsStack()
{
    TaskGraphTemplate g = MakeLinearGraph(10, 20);

    GraphRuntimeInstance inst(g);
    inst.StartExecution();

    const std::vector<int32_t>& ids = inst.GetActiveNodeIDs();
    bool ok = (ids.size() == 1) && (ids.front() == 10);
    TEST_ASSERT(ids.size() == 1,   "Stack should have one entry after start");
    TEST_ASSERT(ids.front() == 10, "That entry should be node 10");
    ReportTest("GetActiveNodeIDs_ReflectsStack", ok);
}

// ===========================================================================
// Main
// ===========================================================================

int main()
{
    std::cout << "=== Phase24RuntimeTest (Phase 24.5) ===" << std::endl;
    std::cout << std::endl;

    std::cout << "-- RuntimeEnvironment --" << std::endl;
    Test1_SetAndGetBlackboardVariable();
    Test2_HasBlackboardVariable();
    Test3_GetBlackboardVariable_NotFound();
    Test4_SetAndGetDynamicPinValue();
    Test5_GetDynamicPinValue_NotFound();
    Test6_OverwriteBlackboardVariable();
    Test7_OverwriteDynamicPinValue();
    Test8_Clear();

    std::cout << std::endl << "-- ConditionEvaluator Operators --" << std::endl;
    Test9_Operator_Equal_True();
    Test10_Operator_Equal_False();
    Test11_Operator_NotEqual_True();
    Test12_Operator_NotEqual_False();
    Test13_Operator_Less_True();
    Test14_Operator_Less_False();
    Test15_Operator_LessEqual_True_Equal();
    Test16_Operator_LessEqual_True_Less();
    Test17_Operator_LessEqual_False();
    Test18_Operator_Greater_True();
    Test19_Operator_Greater_False();
    Test20_Operator_GreaterEqual_True_Equal();
    Test21_Operator_GreaterEqual_True_Greater();
    Test22_Operator_GreaterEqual_False();

    std::cout << std::endl << "-- ConditionEvaluator Operand Resolution --" << std::endl;
    Test23_Resolve_Const();
    Test24_Resolve_Variable_Found();
    Test25_Resolve_Variable_NotFound_Error();
    Test26_Resolve_Pin_Found();
    Test27_Resolve_Pin_NotFound_Error();

    std::cout << std::endl << "-- ConditionEvaluator Edge Cases --" << std::endl;
    Test28_EdgeCase_NaN_Equal();
    Test29_EdgeCase_NaN_NotEqual();
    Test30_EdgeCase_Infinity_Greater();
    Test31_EdgeCase_NegativeInfinity_Less();
    Test32_EdgeCase_LargeNumbers();
    Test33_EdgeCase_Zero_Comparison();

    std::cout << std::endl << "-- ConditionEvaluator Error Handling --" << std::endl;
    Test34_Error_MissingVariable_ReturnsFalse();
    Test35_Error_MissingPin_ReturnsFalse();
    Test36_Error_EmptyErrorOnSuccess();

    std::cout << std::endl << "-- GraphRuntimeInstance --" << std::endl;
    Test37_StartExecution_EmptyGraph();
    Test38_StartExecution_SingleNode();
    Test39_StepExecution_AdvancesStack();
    Test40_StepExecution_ReachesEnd();
    Test41_Breakpoint_PausesExecution();
    Test42_ResumeFromBreakpoint();
    Test43_GetActiveNodeIDs_ReflectsStack();

    std::cout << std::endl
              << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
