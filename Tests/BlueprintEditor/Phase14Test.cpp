/**
 * @file Phase14Test.cpp
 * @brief Tests for Phase 14 — Fix Undo/Redo Completeness.
 * @author Olympe Engine
 * @date 2026-03-13
 *
 * @details
 * Validates the fixes introduced by Phase 14:
 *   1.  GhostLinks_UndoDeleteNode_ConnectionsRestoredInTemplate
 *   2.  MoveNode_UndoAfterLoad_RestoresOriginalPosition
 *   3.  MoveNode_UndoWithConnections_LinkPreserved
 *
 * These tests operate at the UndoRedoStack / TaskGraphTemplate level,
 * matching the existing Phase 13 test pattern.  No SDL3, ImGui or ImNodes
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

// Builds a node definition that already has __posX/__posY in Parameters,
// mimicking what AddNode() does after the Phase-13 fix.
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

// Returns the float position stored in Parameters["__posX"] or 0 if absent.
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

// Returns a pointer to a node in the template by ID, or nullptr.
static const TaskNodeDefinition* FindNode(const TaskGraphTemplate& graph, int id)
{
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
        if (graph.Nodes[i].NodeID == id)
            return &graph.Nodes[i];
    return nullptr;
}

// Returns true if a matching ExecPinConnection exists in the template.
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

// Returns true if a matching DataPinConnection exists in the template.
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
// Test 1 — Ghost links after undo of DeleteNode
//
// Scenario:
//   1. Create nodes A (id=1) and B (id=2)
//   2. Add exec connection A.Out -> B.In
//   3. Delete node B (DeleteNodeCommand) — this removes both the node AND
//      its connections from the template
//   4. Undo → node B and the connection are restored in m_template
//
// FIX 1 asserts: after Undo(), the connection exists in
//   m_template.ExecConnections so that RebuildLinks() can reconstruct
//   m_editorLinks without ghost links.
// ---------------------------------------------------------------------------
static void Test_GhostLinks_UndoDeleteNode_ConnectionsRestoredInTemplate()
{
    const std::string testName =
        "GhostLinks_UndoDeleteNode_ConnectionsRestoredInTemplate";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack stack;

    // Add nodes A and B
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(1, "NodeA"))),
        graph);
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(MakeNode(2, "NodeB"))),
        graph);

    // Connect A.Out -> B.In
    ExecPinConnection conn;
    conn.SourceNodeID  = 1;
    conn.SourcePinName = "Out";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(conn)),
        graph);

    TEST_ASSERT(graph.Nodes.size() == 2, "Expected 2 nodes after setup");
    TEST_ASSERT(graph.ExecConnections.size() == 1,
                "Expected 1 exec connection after setup");

    // Delete node B — must also remove the A->B connection from template
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new DeleteNodeCommand(2)),
        graph);

    TEST_ASSERT(graph.Nodes.size() == 1, "Expected 1 node after delete");
    TEST_ASSERT(graph.ExecConnections.size() == 0,
                "Expected 0 exec connections after delete (connection removed)");

    // Undo the deletion
    TEST_ASSERT(stack.CanUndo(), "Expected CanUndo() == true");
    stack.Undo(graph);

    // After undo: node B is back AND the connection is back
    TEST_ASSERT(graph.Nodes.size() == 2, "Expected 2 nodes after undo");
    TEST_ASSERT(FindNode(graph, 2) != nullptr,
                "Node B (id=2) should be restored after undo");
    TEST_ASSERT(graph.ExecConnections.size() == 1,
                "Expected 1 exec connection after undo (no ghost links in template)");
    TEST_ASSERT(HasExecConn(graph, 1, "Out", 2, "In"),
                "Connection A.Out->B.In should be restored after undo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2 — MoveNode undo after simulated LoadTemplate
//
// Scenario:
//   1. Create a node at (100, 100) with position stored in Parameters
//   2. Simulate "loaded from file" by directly setting graph state (no undo history)
//   3. Move the node to (200, 200) via MoveNodeCommand
//   4. Undo → node should return to (100, 100)
//
// FIX 2 asserts: MoveNodeCommand::Undo() correctly restores Parameters["__posX/Y"]
//   to (100, 100) even when the first command in the stack is a MoveNodeCommand
//   (i.e., when the graph was loaded and no AddNodeCommand is in the stack).
// ---------------------------------------------------------------------------
static void Test_MoveNode_UndoAfterLoad_RestoresOriginalPosition()
{
    const std::string testName =
        "MoveNode_UndoAfterLoad_RestoresOriginalPosition";

    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack stack;

    // Simulate LoadTemplate: add a node directly to the template (no undo history)
    // with positions stored in Parameters (as AddNode() / SyncCanvasFromTemplate() does).
    TaskNodeDefinition loadedNode = MakeNodeWithPos(1, "LoadedNode", 100.0f, 100.0f);
    graph.Nodes.push_back(loadedNode);
    graph.BuildLookupCache();

    // Verify initial state
    const TaskNodeDefinition* node = FindNode(graph, 1);
    TEST_ASSERT(node != nullptr, "Node should exist after simulated load");
    TEST_ASSERT(std::abs(GetParamPosX(*node) - 100.0f) < 0.01f,
                "Initial posX should be 100");
    TEST_ASSERT(std::abs(GetParamPosY(*node) - 100.0f) < 0.01f,
                "Initial posY should be 100");

    // Move node from (100,100) to (200,200) — this is what the drag handler does
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new MoveNodeCommand(1, 100.0f, 100.0f, 200.0f, 200.0f)),
        graph);

    node = FindNode(graph, 1);
    TEST_ASSERT(node != nullptr, "Node should still exist after move");
    TEST_ASSERT(std::abs(GetParamPosX(*node) - 200.0f) < 0.01f,
                "posX should be 200 after move");
    TEST_ASSERT(std::abs(GetParamPosY(*node) - 200.0f) < 0.01f,
                "posY should be 200 after move");

    // Undo the move
    TEST_ASSERT(stack.CanUndo(), "Expected CanUndo() == true");
    stack.Undo(graph);

    node = FindNode(graph, 1);
    TEST_ASSERT(node != nullptr, "Node should still exist after undo");
    TEST_ASSERT(std::abs(GetParamPosX(*node) - 100.0f) < 0.01f,
                "posX should return to 100 after undo");
    TEST_ASSERT(std::abs(GetParamPosY(*node) - 100.0f) < 0.01f,
                "posY should return to 100 after undo");

    // Redo should move back to (200, 200)
    TEST_ASSERT(stack.CanRedo(), "Expected CanRedo() == true");
    stack.Redo(graph);

    node = FindNode(graph, 1);
    TEST_ASSERT(node != nullptr, "Node should still exist after redo");
    TEST_ASSERT(std::abs(GetParamPosX(*node) - 200.0f) < 0.01f,
                "posX should return to 200 after redo");
    TEST_ASSERT(std::abs(GetParamPosY(*node) - 200.0f) < 0.01f,
                "posY should return to 200 after redo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3 — MoveNode undo preserves connections (no ghost links)
//
// Scenario:
//   1. Create nodes A (100,100) and B (200,100)
//   2. Connect A.Out -> B.In
//   3. Move B to (300, 200) via MoveNodeCommand
//   4. Undo → B returns to (200,100) AND connection A.Out->B.In still exists
//
// FIX 3 asserts: undo of a MoveNodeCommand does not affect connections, and
//   connections are still valid in the template (no ghost links created).
// ---------------------------------------------------------------------------
static void Test_MoveNode_UndoWithConnections_LinkPreserved()
{
    const std::string testName =
        "MoveNode_UndoWithConnections_LinkPreserved";

    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    UndoRedoStack stack;

    // Add nodes A and B with positions
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new AddNodeCommand(MakeNodeWithPos(1, "NodeA", 100.0f, 100.0f))),
        graph);
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new AddNodeCommand(MakeNodeWithPos(2, "NodeB", 200.0f, 100.0f))),
        graph);

    // Connect A.Out -> B.In
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

    // Move B from (200,100) to (300,200)
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new MoveNodeCommand(2, 200.0f, 100.0f, 300.0f, 200.0f)),
        graph);

    {
        const TaskNodeDefinition* nodeB = FindNode(graph, 2);
        TEST_ASSERT(nodeB != nullptr, "Node B should exist after move");
        TEST_ASSERT(std::abs(GetParamPosX(*nodeB) - 300.0f) < 0.01f,
                    "B posX should be 300 after move");
        TEST_ASSERT(std::abs(GetParamPosY(*nodeB) - 200.0f) < 0.01f,
                    "B posY should be 200 after move");
    }

    // Connection should still be valid after move
    TEST_ASSERT(HasExecConn(graph, 1, "Out", 2, "In"),
                "Connection A.Out->B.In should exist after move");

    // Undo the move
    TEST_ASSERT(stack.CanUndo(), "Expected CanUndo() == true");
    stack.Undo(graph);

    {
        const TaskNodeDefinition* nodeB = FindNode(graph, 2);
        TEST_ASSERT(nodeB != nullptr, "Node B should still exist after undo");
        TEST_ASSERT(std::abs(GetParamPosX(*nodeB) - 200.0f) < 0.01f,
                    "B posX should return to 200 after undo");
        TEST_ASSERT(std::abs(GetParamPosY(*nodeB) - 100.0f) < 0.01f,
                    "B posY should return to 100 after undo");
    }

    // Connection must still exist — no ghost links
    TEST_ASSERT(graph.ExecConnections.size() == 1,
                "Connection count should still be 1 after move undo");
    TEST_ASSERT(HasExecConn(graph, 1, "Out", 2, "In"),
                "Connection A.Out->B.In should still exist after undo (no ghost links)");

    // Node A should be unaffected
    {
        const TaskNodeDefinition* nodeA = FindNode(graph, 1);
        TEST_ASSERT(nodeA != nullptr, "Node A should still exist after undo");
        TEST_ASSERT(std::abs(GetParamPosX(*nodeA) - 100.0f) < 0.01f,
                    "Node A posX should be unchanged");
    }

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 14 Tests: Fix Undo/Redo Completeness ===" << std::endl;

    Test_GhostLinks_UndoDeleteNode_ConnectionsRestoredInTemplate();
    Test_MoveNode_UndoAfterLoad_RestoresOriginalPosition();
    Test_MoveNode_UndoWithConnections_LinkPreserved();

    std::cout << "\n--- Results ---" << std::endl;
    std::cout << "Passed: " << s_passCount << std::endl;
    std::cout << "Failed: " << s_failCount << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
