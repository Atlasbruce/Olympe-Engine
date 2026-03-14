/**
 * @file Phase21BTest.cpp
 * @brief Unit tests for Phase 21-B — VSGraphVerifier UI Integration.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * Tests operate directly on TaskGraphTemplate and VSGraphVerifier — no SDL3,
 * ImGui, or ImNodes dependency.
 *
 * Test cases:
 *   1. Test_B_NoEntryPoint_HasErrors     — empty graph -> HasErrors() == true
 *   2. Test_B_ValidGraph_IsValid          — EntryPoint + AtomicTask connected -> IsValid() == true
 *   3. Test_B_EmptyAtomicTaskID_HasWarnings — EntryPoint + AtomicTask (empty ID) -> HasWarnings() == true
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
// Test infrastructure (same pattern as Phase21ATest.cpp)
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

// ---------------------------------------------------------------------------
// Test 1 — Empty graph → HasErrors() == true
// ---------------------------------------------------------------------------

static void Test_B_NoEntryPoint_HasErrors()
{
    const std::string testName = "B_NoEntryPoint_HasErrors";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    // Empty graph — no nodes at all

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(result.HasErrors(),
                "Empty graph must have errors (no EntryPoint)");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2 — EntryPoint + connected AtomicTask (with ID) → IsValid() == true
// ---------------------------------------------------------------------------

static void Test_B_ValidGraph_IsValid()
{
    const std::string testName = "B_ValidGraph_IsValid";
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
                "EntryPoint->AtomicTask with valid ID must be valid (IsValid() == true)");
    TEST_ASSERT(!result.HasErrors(),
                "Valid graph must have no errors");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3 — EntryPoint + AtomicTask with empty AtomicTaskID → HasWarnings() == true
// ---------------------------------------------------------------------------

static void Test_B_EmptyAtomicTaskID_HasWarnings()
{
    const std::string testName = "B_EmptyAtomicTaskID_HasWarnings";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::EntryPoint, "Entry"));

    TaskNodeDefinition task = MakeNode(2, TaskNodeType::AtomicTask, "EmptyTask");
    task.AtomicTaskID = ""; // explicitly empty — triggers W001
    graph.Nodes.push_back(task);

    AddExecConn(graph, 1, "Out", 2, "In");
    graph.BuildLookupCache();

    VSVerificationResult result = VSGraphVerifier::Verify(graph);

    TEST_ASSERT(result.HasWarnings(),
                "AtomicTask with empty ID must produce a warning (HasWarnings() == true)");
    TEST_ASSERT(result.IsValid(),
                "Graph with only warnings must still be valid (IsValid() == true)");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 21-B Tests: VSGraphVerifier UI Integration ===" << std::endl;

    Test_B_NoEntryPoint_HasErrors();
    Test_B_ValidGraph_IsValid();
    Test_B_EmptyAtomicTaskID_HasWarnings();

    std::cout << "\n--- Results ---" << std::endl;
    std::cout << "Passed: " << s_passCount << std::endl;
    std::cout << "Failed: " << s_failCount << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
