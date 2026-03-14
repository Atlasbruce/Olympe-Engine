/**
 * @file Phase21ATest.cpp
 * @brief Unit tests for Phase 21-A — VSGraphVerifier (global graph verifier).
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * Tests operate directly on TaskGraphTemplate and VSGraphVerifier — no SDL3,
 * ImGui, or ImNodes dependency.
 *
 * Test cases:
 *   1. Test_E001_NoEntryPoint         — empty graph -> error E001
 *   2. Test_E001_MultipleEntryPoints  — 2 EntryPoints -> error E001
 *   3. Test_E001_ValidEntryPoint      — 1 EntryPoint, no E001 error
 *   4. Test_E002_DanglingNode         — AtomicTask with no connections -> E002
 *   5. Test_E003_ExecCycle            — cycle A->B->A -> error E003
 *   6. Test_E003_NoCycle              — linear graph -> no E003 error
 *   7. Test_W001_EmptyAtomicTaskID    — AtomicTask with empty ID -> W001
 *   8. Test_W002_NegativeDelay        — Delay with DelaySeconds=-1 -> W002
 *   9. Test_I001_UnreachableNode      — node not connected from EntryPoint -> I001
 *  10. Test_ValidGraph                — EntryPoint->AtomicTask correct -> IsValid()
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/VSGraphVerifier.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeDefinition;
using Olympe::ExecPinConnection;
using Olympe::VSGraphVerifier;
using Olympe::VSVerificationResult;
using Olympe::VSVerificationSeverity;
using Olympe::TaskNodeType;

// ---------------------------------------------------------------------------
// Test infrastructure (same pattern as Phase20Test.cpp)
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
// Helpers
// ---------------------------------------------------------------------------

static TaskNodeDefinition MakeNode(int id, TaskNodeType type, const std::string& name)
{
    TaskNodeDefinition node;
    node.NodeID   = id;
    node.Type     = type;
    node.NodeName = name;
    return node;
}

static void AddExecConn(TaskGraphTemplate& graph,
                        int srcID, const std::string& srcPin,
                        int dstID, const std::string& dstPin)
{
    ExecPinConnection c;
    c.SourceNodeID  = srcID;
    c.SourcePinName = srcPin;
    c.TargetNodeID  = dstID;
    c.TargetPinName = dstPin;
    graph.ExecConnections.push_back(c);
}

static bool HasIssueWithRule(const VSVerificationResult& result, const std::string& rulePrefix)
{
    for (size_t i = 0; i < result.issues.size(); ++i)
    {
        const std::string& rid = result.issues[i].ruleID;
        if (rid.size() >= rulePrefix.size() &&
            rid.compare(0, rulePrefix.size(), rulePrefix) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool HasIssueWithSeverity(const VSVerificationResult& result, VSVerificationSeverity sev)
{
    for (size_t i = 0; i < result.issues.size(); ++i)
    {
        if (result.issues[i].severity == sev)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Test 1 — E001: No EntryPoint → error
// ---------------------------------------------------------------------------

static void Test_E001_NoEntryPoint()
{
    const std::string testName = "E001_NoEntryPoint";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    // Empty graph — no nodes at all
    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(HasIssueWithRule(result, "E001"),
                "Empty graph must produce an E001 issue");
    TEST_ASSERT(result.HasErrors(),
                "Empty graph must have errors");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2 — E001: Two EntryPoints → error
// ---------------------------------------------------------------------------

static void Test_E001_MultipleEntryPoints()
{
    const std::string testName = "E001_MultipleEntryPoints";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry1"));
    graph.Nodes.push_back(MakeNode(2, TaskNodeType::EntryPoint, "Entry2"));
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(HasIssueWithRule(result, "E001"),
                "Two EntryPoints must produce an E001 issue");
    TEST_ASSERT(result.HasErrors(),
                "Two EntryPoints must result in errors");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3 — E001: Exactly one EntryPoint → no E001
// ---------------------------------------------------------------------------

static void Test_E001_ValidEntryPoint()
{
    const std::string testName = "E001_ValidEntryPoint";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(!HasIssueWithRule(result, "E001"),
                "Exactly one EntryPoint must not produce an E001 issue");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4 — E002: Dangling AtomicTask (no exec connections)
// ---------------------------------------------------------------------------

static void Test_E002_DanglingNode()
{
    const std::string testName = "E002_DanglingNode";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));
    graph.Nodes.push_back(MakeNode(2, TaskNodeType::AtomicTask, "Task"));
    // No exec connections — node 2 is dangling
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(HasIssueWithRule(result, "E002"),
                "AtomicTask with no connections must produce an E002 issue");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5 — E003: Cycle A->B->A → error E003
// ---------------------------------------------------------------------------

static void Test_E003_ExecCycle()
{
    const std::string testName = "E003_ExecCycle";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));
    graph.Nodes.push_back(MakeNode(2, TaskNodeType::AtomicTask, "A"));
    graph.Nodes.push_back(MakeNode(3, TaskNodeType::AtomicTask, "B"));

    AddExecConn(graph, 1, "Out", 2, "In"); // Entry -> A
    AddExecConn(graph, 2, "Out", 3, "In"); // A -> B
    AddExecConn(graph, 3, "Out", 2, "In"); // B -> A (cycle!)
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(HasIssueWithRule(result, "E003"),
                "Cycle A->B->A must produce an E003 issue");
    TEST_ASSERT(result.HasErrors(),
                "Cycle must result in errors");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6 — E003: Linear graph → no E003
// ---------------------------------------------------------------------------

static void Test_E003_NoCycle()
{
    const std::string testName = "E003_NoCycle";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));
    graph.Nodes.push_back(MakeNode(2, TaskNodeType::AtomicTask, "Task"));

    AddExecConn(graph, 1, "Out", 2, "In"); // Entry -> Task (linear, no cycle)
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(!HasIssueWithRule(result, "E003"),
                "Linear graph must not produce an E003 issue");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7 — W001: AtomicTask with empty AtomicTaskID
// ---------------------------------------------------------------------------

static void Test_W001_EmptyAtomicTaskID()
{
    const std::string testName = "W001_EmptyAtomicTaskID";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));

    TaskNodeDefinition task = MakeNode(2, TaskNodeType::AtomicTask, "EmptyTask");
    task.AtomicTaskID = ""; // explicitly empty
    graph.Nodes.push_back(task);

    AddExecConn(graph, 1, "Out", 2, "In");
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(HasIssueWithRule(result, "W001"),
                "AtomicTask with empty ID must produce a W001 issue");
    TEST_ASSERT(HasIssueWithSeverity(result, VSVerificationSeverity::Warning),
                "W001 must be reported as a Warning");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8 — W002: Delay with DelaySeconds = -1
// ---------------------------------------------------------------------------

static void Test_W002_NegativeDelay()
{
    const std::string testName = "W002_NegativeDelay";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));

    TaskNodeDefinition delayNode = MakeNode(2, TaskNodeType::Delay, "MyDelay");
    delayNode.DelaySeconds = -1.0f;
    graph.Nodes.push_back(delayNode);

    AddExecConn(graph, 1, "Out", 2, "In");
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(HasIssueWithRule(result, "W002"),
                "Delay with negative seconds must produce a W002 issue");
    TEST_ASSERT(HasIssueWithSeverity(result, VSVerificationSeverity::Warning),
                "W002 must be reported as a Warning");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 9 — I001: Unreachable node (not connected from EntryPoint)
// ---------------------------------------------------------------------------

static void Test_I001_UnreachableNode()
{
    const std::string testName = "I001_UnreachableNode";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));
    graph.Nodes.push_back(MakeNode(2, TaskNodeType::AtomicTask, "Reachable"));
    graph.Nodes.push_back(MakeNode(3, TaskNodeType::AtomicTask, "Unreachable"));

    AddExecConn(graph, 1, "Out", 2, "In"); // Entry -> Reachable
    // Node 3 has a connection but is not reachable from EntryPoint
    AddExecConn(graph, 3, "Out", 2, "In"); // Unreachable -> Reachable (has exec out, not dangling)
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(HasIssueWithRule(result, "I001"),
                "Node not reachable from EntryPoint must produce an I001 issue");
    TEST_ASSERT(HasIssueWithSeverity(result, VSVerificationSeverity::Info),
                "I001 must be reported as Info");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 10 — Valid graph: EntryPoint -> AtomicTask with TaskID set
// ---------------------------------------------------------------------------

static void Test_ValidGraph()
{
    const std::string testName = "ValidGraph";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));

    TaskNodeDefinition task = MakeNode(2, TaskNodeType::AtomicTask, "Patrol");
    task.AtomicTaskID = "MoveTo";
    graph.Nodes.push_back(task);

    AddExecConn(graph, 1, "Out", 2, "In");
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(result.IsValid(),
                "EntryPoint->AtomicTask with valid ID must pass verification (IsValid)");
    TEST_ASSERT(!result.HasErrors(),
                "Valid graph must have no errors");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 21-A Tests: VSGraphVerifier ===" << std::endl;

    Test_E001_NoEntryPoint();
    Test_E001_MultipleEntryPoints();
    Test_E001_ValidEntryPoint();
    Test_E002_DanglingNode();
    Test_E003_ExecCycle();
    Test_E003_NoCycle();
    Test_W001_EmptyAtomicTaskID();
    Test_W002_NegativeDelay();
    Test_I001_UnreachableNode();
    Test_ValidGraph();

    std::cout << "\n--- Results ---" << std::endl;
    std::cout << "Passed: " << s_passCount << std::endl;
    std::cout << "Failed: " << s_failCount << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
