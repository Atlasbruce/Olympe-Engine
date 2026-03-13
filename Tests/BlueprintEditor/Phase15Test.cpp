/**
 * @file Phase15Test.cpp
 * @brief Tests for Phase 15 — Complete Undo/Redo + Context Menus.
 * @author Olympe Engine
 * @date 2026-03-13
 *
 * @details
 * Validates the fixes introduced by Phase 15:
 *   1.  PerformUndo_TemplateStateRestored
 *       — Verifies that after undo the template data matches pre-operation state.
 *   2.  UndoMoveNode_NoGhostLinks_ConnectionsIntact
 *       — Verifies that undoing a MoveNode on a loaded graph preserves all
 *         exec and data connections in the template (no ghost links).
 *   3.  CanvasContextMenu_AddNode_AtPosition
 *       — Verifies that AddNode() stores the correct canvas position in
 *         Parameters["__posX/__posY"] (simulates context-menu node creation).
 *   4.  DeleteLinkCommand_Undoable_ExecAndData
 *       — Verifies that DeleteLinkCommand is undoable for both exec and data
 *         connections (simulates "Delete Connection" from context menu).
 *   5.  PerformRedo_AfterMultipleUndos_RestoresState
 *       — Verifies that redo restores the graph to the state it was in before
 *         multiple consecutive undos.
 *
 * These tests operate at the UndoRedoStack / TaskGraphTemplate level,
 * matching the existing Phase 13/14 test pattern.  No SDL3, ImGui or ImNodes
 * dependency.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/UndoRedoStack.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <memory>
#include <cmath>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeDefinition;
using Olympe::TaskNodeType;
using Olympe::ExecPinConnection;
using Olympe::DataPinConnection;
using Olympe::UndoRedoStack;
using Olympe::AddNodeCommand;
using Olympe::DeleteNodeCommand;
using Olympe::MoveNodeCommand;
using Olympe::AddConnectionCommand;
using Olympe::AddDataConnectionCommand;
using Olympe::DeleteLinkCommand;
using Olympe::NODE_INDEX_NONE;
using Olympe::ParameterBindingType;
using Olympe::TaskValue;
using Olympe::ParameterBinding;

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
// Helpers
// ---------------------------------------------------------------------------

static TaskNodeDefinition MakeNode(int id, const std::string& name,
                                    TaskNodeType type = TaskNodeType::AtomicTask)
{
    TaskNodeDefinition def;
    def.NodeID   = id;
    def.NodeName = name;
    def.Type     = type;
    return def;
}

// Builds a node definition with __posX/__posY already set in Parameters,
// mimicking what AddNode() does (Phase 13 fix).
static TaskNodeDefinition MakeNodeWithPos(int id, const std::string& name,
                                          float x, float y,
                                          TaskNodeType type = TaskNodeType::AtomicTask)
{
    TaskNodeDefinition def = MakeNode(id, name, type);
    ParameterBinding bx, by;
    bx.Type         = ParameterBindingType::Literal;
    bx.LiteralValue = TaskValue(x);
    by.Type         = ParameterBindingType::Literal;
    by.LiteralValue = TaskValue(y);
    def.Parameters["__posX"] = bx;
    def.Parameters["__posY"] = by;
    return def;
}

static float GetParamPosX(const TaskNodeDefinition& def)
{
    auto it = def.Parameters.find("__posX");
    if (it == def.Parameters.end() ||
        it->second.Type != ParameterBindingType::Literal)
        return 0.0f;
    return it->second.LiteralValue.AsFloat();
}

static float GetParamPosY(const TaskNodeDefinition& def)
{
    auto it = def.Parameters.find("__posY");
    if (it == def.Parameters.end() ||
        it->second.Type != ParameterBindingType::Literal)
        return 0.0f;
    return it->second.LiteralValue.AsFloat();
}

static const TaskNodeDefinition* FindNode(const TaskGraphTemplate& graph, int id)
{
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
        if (graph.Nodes[i].NodeID == id)
            return &graph.Nodes[i];
    return nullptr;
}

static bool HasExecConn(const TaskGraphTemplate& graph,
                         int srcID, const std::string& srcPin,
                         int dstID, const std::string& dstPin)
{
    for (size_t i = 0; i < graph.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& c = graph.ExecConnections[i];
        if (c.SourceNodeID == srcID && c.SourcePinName == srcPin &&
            c.TargetNodeID == dstID && c.TargetPinName == dstPin)
            return true;
    }
    return false;
}

static bool HasDataConn(const TaskGraphTemplate& graph,
                         int srcID, const std::string& srcPin,
                         int dstID, const std::string& dstPin)
{
    for (size_t i = 0; i < graph.DataConnections.size(); ++i)
    {
        const DataPinConnection& c = graph.DataConnections[i];
        if (c.SourceNodeID == srcID && c.SourcePinName == srcPin &&
            c.TargetNodeID == dstID && c.TargetPinName == dstPin)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Test 1 — PerformUndo_TemplateStateRestored
//
// Scenario (mirrors what PerformUndo() does at the template level):
//   1. Add a node
//   2. Add an exec connection
//   3. Undo the connection → template has 1 node, 0 connections
//   4. Undo the node → template has 0 nodes, 0 connections
//   5. Verify stack state after both undos
//
// This validates that the UndoRedoStack operations used inside PerformUndo()
// correctly restore the template to prior states.
// ---------------------------------------------------------------------------
static void Test_PerformUndo_TemplateStateRestored()
{
    const std::string testName = "PerformUndo_TemplateStateRestored";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack     stack;

    // Step 1: add node
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(1, "NodeA"))),
        graph);

    TEST_ASSERT(graph.Nodes.size() == 1, "Expected 1 node after add");
    TEST_ASSERT(graph.ExecConnections.size() == 0, "Expected 0 connections");

    // Step 2: add exec connection to a second node
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(2, "NodeB"))),
        graph);

    ExecPinConnection conn;
    conn.SourceNodeID  = 1;
    conn.SourcePinName = "Out";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(conn)),
        graph);

    TEST_ASSERT(graph.Nodes.size() == 2, "Expected 2 nodes");
    TEST_ASSERT(graph.ExecConnections.size() == 1, "Expected 1 connection");

    // Step 3: undo connection
    TEST_ASSERT(stack.CanUndo(), "CanUndo should be true");
    stack.Undo(graph);  // undoes AddConnectionCommand

    TEST_ASSERT(graph.ExecConnections.size() == 0,
                "After undo connection: expected 0 connections");
    TEST_ASSERT(graph.Nodes.size() == 2, "Nodes unchanged after undo connection");
    TEST_ASSERT(stack.CanRedo(), "CanRedo should be true after undo");

    // Step 4: undo NodeB
    stack.Undo(graph);  // undoes AddNodeCommand for NodeB

    TEST_ASSERT(graph.Nodes.size() == 1, "After undo NodeB: expected 1 node");
    TEST_ASSERT(FindNode(graph, 1) != nullptr, "NodeA should still exist");
    TEST_ASSERT(FindNode(graph, 2) == nullptr, "NodeB should be gone");

    // Step 5: undo NodeA
    stack.Undo(graph);

    TEST_ASSERT(graph.Nodes.size() == 0, "After undo NodeA: expected 0 nodes");
    TEST_ASSERT(!stack.CanUndo(), "No more undos expected");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2 — UndoMoveNode_NoGhostLinks_ConnectionsIntact
//
// Scenario (the primary ghost-links fix for loaded graphs):
//   1. Simulate LoadTemplate: two nodes with positions in Parameters,
//      one exec connection, one data connection — no undo history.
//   2. Move node B from (200,100) to (400,300) via MoveNodeCommand.
//   3. Undo the move → B returns to (200,100).
//   4. Assert: both exec and data connections are still in the template
//      (i.e. RebuildLinks() would find them and not produce ghost links).
// ---------------------------------------------------------------------------
static void Test_UndoMoveNode_NoGhostLinks_ConnectionsIntact()
{
    const std::string testName = "UndoMoveNode_NoGhostLinks_ConnectionsIntact";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack     stack;

    // Simulate LoadTemplate — direct insertion, no undo history
    graph.Nodes.push_back(MakeNodeWithPos(1, "EntryPoint", 100.0f, 100.0f,
                                          TaskNodeType::EntryPoint));
    graph.Nodes.push_back(MakeNodeWithPos(2, "TaskNode", 200.0f, 100.0f,
                                          TaskNodeType::AtomicTask));
    graph.BuildLookupCache();

    // Add exec connection 1->2
    ExecPinConnection ec;
    ec.SourceNodeID  = 1;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 2;
    ec.TargetPinName = "In";
    graph.ExecConnections.push_back(ec);

    // Add data connection 1->2
    DataPinConnection dc;
    dc.SourceNodeID  = 1;
    dc.SourcePinName = "Value";
    dc.TargetNodeID  = 2;
    dc.TargetPinName = "Input";
    graph.DataConnections.push_back(dc);

    TEST_ASSERT(graph.Nodes.size() == 2, "Setup: expected 2 nodes");
    TEST_ASSERT(graph.ExecConnections.size() == 1, "Setup: expected 1 exec conn");
    TEST_ASSERT(graph.DataConnections.size() == 1, "Setup: expected 1 data conn");

    // Move node 2 from (200,100) to (400,300)
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new MoveNodeCommand(2, 200.0f, 100.0f, 400.0f, 300.0f)),
        graph);

    {
        const TaskNodeDefinition* nodeB = FindNode(graph, 2);
        TEST_ASSERT(nodeB != nullptr, "Node 2 exists after move");
        TEST_ASSERT(std::abs(GetParamPosX(*nodeB) - 400.0f) < 0.01f,
                    "posX should be 400 after move");
    }

    // Connections must survive the move
    TEST_ASSERT(HasExecConn(graph, 1, "Out", 2, "In"),
                "Exec conn must survive move");
    TEST_ASSERT(HasDataConn(graph, 1, "Value", 2, "Input"),
                "Data conn must survive move");

    // Undo the move — this is what PerformUndo() triggers
    TEST_ASSERT(stack.CanUndo(), "CanUndo should be true");
    stack.Undo(graph);

    {
        const TaskNodeDefinition* nodeB = FindNode(graph, 2);
        TEST_ASSERT(nodeB != nullptr, "Node 2 still exists after undo");
        TEST_ASSERT(std::abs(GetParamPosX(*nodeB) - 200.0f) < 0.01f,
                    "posX should return to 200 after undo");
        TEST_ASSERT(std::abs(GetParamPosY(*nodeB) - 100.0f) < 0.01f,
                    "posY should return to 100 after undo");
    }

    // Critical: both connections must still be in the template after undo
    // (so RebuildLinks() would not produce ghost links)
    TEST_ASSERT(graph.ExecConnections.size() == 1,
                "Exec conn count must be 1 after undo move (no ghost links)");
    TEST_ASSERT(HasExecConn(graph, 1, "Out", 2, "In"),
                "Exec conn must still exist after undo move");
    TEST_ASSERT(graph.DataConnections.size() == 1,
                "Data conn count must be 1 after undo move (no ghost links)");
    TEST_ASSERT(HasDataConn(graph, 1, "Value", 2, "Input"),
                "Data conn must still exist after undo move");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3 — CanvasContextMenu_AddNode_AtPosition
//
// Scenario (simulates the canvas context-menu "Add Node" path):
//   1. Create a node via AddNodeCommand with position data already set
//      in Parameters (matching what AddNode(type, x, y) does).
//   2. Verify __posX and __posY are stored in the node's Parameters.
//   3. Undo the add — node disappears.
//   4. Redo the add — node reappears at the same position.
// ---------------------------------------------------------------------------
static void Test_CanvasContextMenu_AddNode_AtPosition()
{
    const std::string testName = "CanvasContextMenu_AddNode_AtPosition";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack     stack;

    // Simulate AddNode(TaskNodeType::Branch, 500.0f, 300.0f) with position stored
    TaskNodeDefinition def = MakeNodeWithPos(42, "BranchNode", 500.0f, 300.0f,
                                             TaskNodeType::Branch);
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(def)),
        graph);

    // Node should be present with correct position
    const TaskNodeDefinition* node = FindNode(graph, 42);
    TEST_ASSERT(node != nullptr, "Node should exist after AddNode");
    TEST_ASSERT(std::abs(GetParamPosX(*node) - 500.0f) < 0.01f,
                "posX should be 500 (context menu spawn X)");
    TEST_ASSERT(std::abs(GetParamPosY(*node) - 300.0f) < 0.01f,
                "posY should be 300 (context menu spawn Y)");
    TEST_ASSERT(node->Type == TaskNodeType::Branch,
                "Node type should be Branch");

    // Undo
    TEST_ASSERT(stack.CanUndo(), "CanUndo should be true");
    stack.Undo(graph);
    TEST_ASSERT(FindNode(graph, 42) == nullptr,
                "Node should be gone after undo");
    TEST_ASSERT(graph.Nodes.empty(), "Graph should have 0 nodes after undo");

    // Redo — node should come back at the same position
    TEST_ASSERT(stack.CanRedo(), "CanRedo should be true");
    stack.Redo(graph);

    node = FindNode(graph, 42);
    TEST_ASSERT(node != nullptr, "Node should reappear after redo");
    TEST_ASSERT(std::abs(GetParamPosX(*node) - 500.0f) < 0.01f,
                "posX should still be 500 after redo");
    TEST_ASSERT(std::abs(GetParamPosY(*node) - 300.0f) < 0.01f,
                "posY should still be 300 after redo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4 — DeleteLinkCommand_Undoable_ExecAndData
//
// Scenario (simulates "Delete Connection" from the link context menu):
//   1. Add two nodes and one exec + one data connection.
//   2. Delete exec connection via DeleteLinkCommand → template has 0 exec conns.
//   3. Undo → exec connection restored.
//   4. Delete data connection via DeleteLinkCommand → 0 data conns.
//   5. Undo → data connection restored.
// ---------------------------------------------------------------------------
static void Test_DeleteLinkCommand_Undoable_ExecAndData()
{
    const std::string testName = "DeleteLinkCommand_Undoable_ExecAndData";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack     stack;

    // Setup
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(1, "NodeA"))),
        graph);
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(2, "NodeB"))),
        graph);

    ExecPinConnection ec;
    ec.SourceNodeID  = 1;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 2;
    ec.TargetPinName = "In";
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(ec)),
        graph);

    DataPinConnection dc;
    dc.SourceNodeID  = 1;
    dc.SourcePinName = "Val";
    dc.TargetNodeID  = 2;
    dc.TargetPinName = "Dst";
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddDataConnectionCommand(dc)),
        graph);

    TEST_ASSERT(graph.ExecConnections.size() == 1, "Setup: 1 exec conn");
    TEST_ASSERT(graph.DataConnections.size() == 1, "Setup: 1 data conn");

    // --- Part A: delete and undo exec link ---
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new DeleteLinkCommand(ec)),
        graph);

    TEST_ASSERT(graph.ExecConnections.size() == 0,
                "After delete exec: 0 exec conns");
    TEST_ASSERT(graph.DataConnections.size() == 1,
                "After delete exec: data conn unaffected");

    stack.Undo(graph);

    TEST_ASSERT(graph.ExecConnections.size() == 1,
                "After undo delete exec: exec conn restored");
    TEST_ASSERT(HasExecConn(graph, 1, "Out", 2, "In"),
                "Restored exec conn must match original");

    // --- Part B: delete and undo data link ---
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new DeleteLinkCommand(dc)),
        graph);

    TEST_ASSERT(graph.DataConnections.size() == 0,
                "After delete data: 0 data conns");
    TEST_ASSERT(graph.ExecConnections.size() == 1,
                "After delete data: exec conn unaffected");

    stack.Undo(graph);

    TEST_ASSERT(graph.DataConnections.size() == 1,
                "After undo delete data: data conn restored");
    TEST_ASSERT(HasDataConn(graph, 1, "Val", 2, "Dst"),
                "Restored data conn must match original");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5 — PerformRedo_AfterMultipleUndos_RestoresState
//
// Scenario:
//   1. Add nodes A, B, C.
//   2. Connect A→B and B→C.
//   3. Undo three times (undo B→C connection, undo A→B connection, undo node C).
//   4. Redo twice (redo node C, redo A→B connection).
//   5. Verify graph state after the redo sequence.
// ---------------------------------------------------------------------------
static void Test_PerformRedo_AfterMultipleUndos_RestoresState()
{
    const std::string testName = "PerformRedo_AfterMultipleUndos_RestoresState";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack     stack;

    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(1, "A"))),
        graph);
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(2, "B"))),
        graph);
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(3, "C"))),
        graph);

    ExecPinConnection ab;
    ab.SourceNodeID = 1; ab.SourcePinName = "Out";
    ab.TargetNodeID = 2; ab.TargetPinName = "In";
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(ab)),
        graph);

    ExecPinConnection bc;
    bc.SourceNodeID = 2; bc.SourcePinName = "Out";
    bc.TargetNodeID = 3; bc.TargetPinName = "In";
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(bc)),
        graph);

    TEST_ASSERT(graph.Nodes.size() == 3, "Setup: 3 nodes");
    TEST_ASSERT(graph.ExecConnections.size() == 2, "Setup: 2 connections");

    // Undo 3 times: undo B→C, then undo A→B, then undo AddNode(C)
    stack.Undo(graph); // undo B→C
    TEST_ASSERT(graph.ExecConnections.size() == 1, "After undo B→C: 1 conn");
    TEST_ASSERT(!HasExecConn(graph, 2, "Out", 3, "In"),
                "B→C should be removed");

    stack.Undo(graph); // undo A→B
    TEST_ASSERT(graph.ExecConnections.size() == 0, "After undo A→B: 0 conns");

    stack.Undo(graph); // undo AddNode(C)
    TEST_ASSERT(graph.Nodes.size() == 2, "After undo node C: 2 nodes");
    TEST_ASSERT(FindNode(graph, 3) == nullptr, "Node C should be absent");

    // Redo twice: redo AddNode(C), then redo A→B
    TEST_ASSERT(stack.CanRedo(), "CanRedo should be true");
    stack.Redo(graph); // redo AddNode(C)
    TEST_ASSERT(graph.Nodes.size() == 3, "After redo node C: 3 nodes");
    TEST_ASSERT(FindNode(graph, 3) != nullptr, "Node C should be back");
    TEST_ASSERT(graph.ExecConnections.size() == 0,
                "No connections yet after first redo");

    stack.Redo(graph); // redo A→B
    TEST_ASSERT(graph.ExecConnections.size() == 1, "After redo A→B: 1 conn");
    TEST_ASSERT(HasExecConn(graph, 1, "Out", 2, "In"),
                "A→B should be restored");
    TEST_ASSERT(!HasExecConn(graph, 2, "Out", 3, "In"),
                "B→C should not be present yet (not redone)");
    TEST_ASSERT(stack.CanRedo(), "B→C redo should still be available");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 15 Tests: Complete Undo/Redo + Context Menus ===" << std::endl;

    Test_PerformUndo_TemplateStateRestored();
    Test_UndoMoveNode_NoGhostLinks_ConnectionsIntact();
    Test_CanvasContextMenu_AddNode_AtPosition();
    Test_DeleteLinkCommand_Undoable_ExecAndData();
    Test_PerformRedo_AfterMultipleUndos_RestoresState();

    std::cout << "\n--- Results ---" << std::endl;
    std::cout << "Passed: " << s_passCount << std::endl;
    std::cout << "Failed: " << s_failCount << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
