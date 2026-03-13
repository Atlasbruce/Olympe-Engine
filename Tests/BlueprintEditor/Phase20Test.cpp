/**
 * @file Phase20Test.cpp
 * @brief Tests for Phase 20-B — Real-Time Connection Validation.
 * @author Olympe Engine
 * @date 2026-03-13
 *
 * @details
 * Validates the VSConnectionValidator::IsExecConnectionValid() helper
 * introduced in Phase 20-B. Tests operate directly on TaskGraphTemplate
 * and VSConnectionValidator — no SDL3, ImGui, or ImNodes dependency.
 *
 * Test cases:
 *   1. Test_IsExecConnectionValid_SelfLoop_Rejected
 *      — srcNodeID == dstNodeID must return false.
 *   2. Test_IsExecConnectionValid_DuplicatePin_Rejected
 *      — same source pin already connected must return false.
 *   3. Test_IsExecConnectionValid_Cycle_Rejected
 *      — adding A->B when B->A already exists creates a cycle; must return false.
 *   4. Test_IsExecConnectionValid_Valid_Accepted
 *      — normal A->B with no prior connections must return true.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/VSConnectionValidator.h"
#include "BlueprintEditor/UndoRedoStack.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeDefinition;
using Olympe::ExecPinConnection;
using Olympe::VSConnectionValidator;
using Olympe::AddDynamicPinCommand;
using Olympe::TaskNodeType;

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
// Helper: add an exec connection to the graph
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Test 1 — Self-loop must be rejected
//
// Scenario: srcNodeID == dstNodeID == 1
// Expected: IsExecConnectionValid returns false
// ---------------------------------------------------------------------------

static void Test_IsExecConnectionValid_SelfLoop_Rejected()
{
    const std::string testName = "IsExecConnectionValid_SelfLoop_Rejected";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    bool result = VSConnectionValidator::IsExecConnectionValid(graph, 1, "Out", 1);
    TEST_ASSERT(result == false, "Self-loop (node 1 -> node 1) must be rejected");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2 — Duplicate output pin must be rejected
//
// Scenario: graph already has connection 1.Out -> 2.In
//           Attempting to add 1.Out -> 3.In (same source pin) must be rejected
// Expected: IsExecConnectionValid returns false
// ---------------------------------------------------------------------------

static void Test_IsExecConnectionValid_DuplicatePin_Rejected()
{
    const std::string testName = "IsExecConnectionValid_DuplicatePin_Rejected";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    AddExecConn(graph, 1, "Out", 2, "In");

    bool result = VSConnectionValidator::IsExecConnectionValid(graph, 1, "Out", 3);
    TEST_ASSERT(result == false,
                "Duplicate output pin (1.Out already connected) must be rejected");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3 — Cycle must be rejected
//
// Scenario: graph already has connections B(2)->C(3) and C(3)->A(1).
//           Attempting to add A(1)->B(2) would close the cycle A->B->C->A.
// Expected: IsExecConnectionValid returns false
// ---------------------------------------------------------------------------

static void Test_IsExecConnectionValid_Cycle_Rejected()
{
    const std::string testName = "IsExecConnectionValid_Cycle_Rejected";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    AddExecConn(graph, 2, "Out", 3, "In"); // B -> C
    AddExecConn(graph, 3, "Out", 1, "In"); // C -> A

    // Adding A(1) -> B(2) would create cycle A->B->C->A
    bool result = VSConnectionValidator::IsExecConnectionValid(graph, 1, "Out", 2);
    TEST_ASSERT(result == false,
                "Connection that would create a cycle (1->2->3->1) must be rejected");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4 — Valid connection must be accepted
//
// Scenario: empty graph, adding A(1)->B(2)
// Expected: IsExecConnectionValid returns true
// ---------------------------------------------------------------------------

static void Test_IsExecConnectionValid_Valid_Accepted()
{
    const std::string testName = "IsExecConnectionValid_Valid_Accepted";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    bool result = VSConnectionValidator::IsExecConnectionValid(graph, 1, "Out", 2);
    TEST_ASSERT(result == true,
                "Valid connection A->B with no prior connections must be accepted");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5 — AddDynamicPinCommand — Execute / Undo
//
// Scenario: a VSSequence node starts with no dynamic pins.
//           Execute adds "Out_1"; Undo removes it.
// ---------------------------------------------------------------------------

static void Test_AddDynamicPin_UndoRedo()
{
    const std::string testName = "AddDynamicPin_UndoRedo";
    int prevFail = s_failCount;

    // Build a graph with one VSSequence node
    TaskGraphTemplate graph;
    TaskNodeDefinition seqNode;
    seqNode.NodeID   = 1;
    seqNode.Type     = TaskNodeType::VSSequence;
    seqNode.NodeName = "Sequence";
    graph.Nodes.push_back(seqNode);
    graph.BuildLookupCache();

    // Initial state: no dynamic pins
    const TaskNodeDefinition* node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node should exist before Execute");
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 0,
                "DynamicExecOutputPins should be empty before Execute");

    // Execute: add pin "Out_1"
    AddDynamicPinCommand cmd(1, "Out_1");
    cmd.Execute(graph);

    const TaskNodeDefinition* nodeAfter = graph.GetNode(1);
    TEST_ASSERT(nodeAfter != nullptr, "Node should exist after Execute");
    TEST_ASSERT(nodeAfter->DynamicExecOutputPins.size() == 1,
                "DynamicExecOutputPins size should be 1 after Execute");
    TEST_ASSERT(nodeAfter->DynamicExecOutputPins[0] == "Out_1",
                "First dynamic pin should be 'Out_1'");

    // Undo: remove the pin
    cmd.Undo(graph);

    const TaskNodeDefinition* nodeUndo = graph.GetNode(1);
    TEST_ASSERT(nodeUndo != nullptr, "Node should exist after Undo");
    TEST_ASSERT(nodeUndo->DynamicExecOutputPins.empty(),
                "DynamicExecOutputPins should be empty after Undo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 20-B Tests: Real-Time Connection Validation ===" << std::endl;

    Test_IsExecConnectionValid_SelfLoop_Rejected();
    Test_IsExecConnectionValid_DuplicatePin_Rejected();
    Test_IsExecConnectionValid_Cycle_Rejected();
    Test_IsExecConnectionValid_Valid_Accepted();

    std::cout << "\n=== Phase 20-C Tests: AddDynamicPinCommand ===" << std::endl;

    Test_AddDynamicPin_UndoRedo();

    std::cout << "\n--- Results ---" << std::endl;
    std::cout << "Passed: " << s_passCount << std::endl;
    std::cout << "Failed: " << s_failCount << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
