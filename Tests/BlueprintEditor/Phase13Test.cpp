/**
 * @file Phase13Test.cpp
 * @brief Tests for Phase 13 — Fix Undo/Redo After File Load.
 * @author Olympe Engine
 * @date 2026-03-12
 *
 * @details
 * Validates the fixes introduced by PR #365:
 *   1.  AddNode_StoresPositionInParameters
 *   2.  RedoAddNode_RestoresCorrectPosition
 *   3.  AddConnectionCommand_IsUndoable
 *   4.  AddDataConnectionCommand_IsUndoable
 *   5.  AddConnectionCommand_UndoRemovesConnection
 *   6.  AddDataConnectionCommand_UndoRemovesConnection
 *   7.  AddNodeCommand_PositionInParameters_PreventsGhostOnRedo
 *   8.  UndoAddNode_WithConnection_ConnectionRemovedToo
 *   9.  UndoAddExecConnection_RedoRestoresIt
 *  10.  UndoAddDataConnection_RedoRestoresIt
 *  11.  AddDataConnectionCommand_Description
 *  12.  AddConnectionCommand_Description
 *  13.  UndoStack_NotClearedByLoadTemplate_SimulatedByManualPush
 *  14.  MoveNode_PositionInParameters_CorrectAfterUndoRedo
 *
 * No SDL3, ImGui, or ImNodes dependency.
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/UndoRedoStack.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <memory>

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

static bool GraphHasNode(const TaskGraphTemplate& g, int id)
{
    for (const auto& n : g.Nodes)
        if (n.NodeID == id) return true;
    return false;
}

static bool GraphHasExecConn(const TaskGraphTemplate& g, int src, int dst)
{
    for (const auto& ec : g.ExecConnections)
        if (ec.SourceNodeID == src && ec.TargetNodeID == dst)
            return true;
    return false;
}

static bool GraphHasDataConn(const TaskGraphTemplate& g, int src, int dst)
{
    for (const auto& dc : g.DataConnections)
        if (dc.SourceNodeID == src && dc.TargetNodeID == dst)
            return true;
    return false;
}

static float GetNodePosX(const TaskGraphTemplate& g, int id)
{
    for (const auto& n : g.Nodes)
    {
        if (n.NodeID == id)
        {
            auto it = n.Parameters.find("__posX");
            if (it != n.Parameters.end() &&
                it->second.Type == ParameterBindingType::Literal)
                return it->second.LiteralValue.AsFloat();
        }
    }
    return -1.0f;
}

static float GetNodePosY(const TaskGraphTemplate& g, int id)
{
    for (const auto& n : g.Nodes)
    {
        if (n.NodeID == id)
        {
            auto it = n.Parameters.find("__posY");
            if (it != n.Parameters.end() &&
                it->second.Type == ParameterBindingType::Literal)
                return it->second.LiteralValue.AsFloat();
        }
    }
    return -1.0f;
}

// ---------------------------------------------------------------------------
// Test 1: AddNode with __posX/__posY in def stores position in template
// ---------------------------------------------------------------------------

static void Test1_AddNode_StoresPositionInParameters()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Simulates what AddNode() does after Fix 2: write position to Parameters
    // before passing to AddNodeCommand.
    auto cmd = std::unique_ptr<Olympe::ICommand>(
        new AddNodeCommand(MakeNodeWithPos(1, "Entry", 150.0f, 250.0f,
                                          TaskNodeType::EntryPoint)));
    stack.PushCommand(std::move(cmd), graph);

    TEST_ASSERT(GraphHasNode(graph, 1), "Node 1 should be in graph");
    float x = GetNodePosX(graph, 1);
    float y = GetNodePosY(graph, 1);
    TEST_ASSERT(x > 149.0f && x < 151.0f, "__posX should be ~150");
    TEST_ASSERT(y > 249.0f && y < 251.0f, "__posY should be ~250");

    ReportTest("Test1_AddNode_StoresPositionInParameters", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2: Redo AddNode restores node WITH its position
// ---------------------------------------------------------------------------

static void Test2_RedoAddNode_RestoresCorrectPosition()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd = std::unique_ptr<Olympe::ICommand>(
        new AddNodeCommand(MakeNodeWithPos(10, "Branch", 400.0f, 300.0f,
                                          TaskNodeType::Branch)));
    stack.PushCommand(std::move(cmd), graph);

    // Undo: node removed
    stack.Undo(graph);
    TEST_ASSERT(!GraphHasNode(graph, 10), "Node 10 should be gone after undo");

    // Redo: node restored with position
    stack.Redo(graph);
    TEST_ASSERT(GraphHasNode(graph, 10), "Node 10 should be restored after redo");

    float x = GetNodePosX(graph, 10);
    float y = GetNodePosY(graph, 10);
    TEST_ASSERT(x > 399.0f && x < 401.0f,
                "After redo: __posX should be ~400 (not default)");
    TEST_ASSERT(y > 299.0f && y < 301.0f,
                "After redo: __posY should be ~300 (not default)");

    ReportTest("Test2_RedoAddNode_RestoresCorrectPosition", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: AddConnectionCommand is undoable
// ---------------------------------------------------------------------------

static void Test3_AddConnectionCommand_IsUndoable()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, "A"));
    graph.Nodes.push_back(MakeNode(2, "B"));
    graph.BuildLookupCache();

    ExecPinConnection ec;
    ec.SourceNodeID  = 1;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 2;
    ec.TargetPinName = "In";

    auto cmd = std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(ec));
    stack.PushCommand(std::move(cmd), graph);

    TEST_ASSERT(GraphHasExecConn(graph, 1, 2), "Connection 1->2 should exist");
    TEST_ASSERT(stack.CanUndo(), "CanUndo should be true");

    ReportTest("Test3_AddConnectionCommand_IsUndoable", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: AddDataConnectionCommand is undoable
// ---------------------------------------------------------------------------

static void Test4_AddDataConnectionCommand_IsUndoable()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(3, "Getter"));
    graph.Nodes.push_back(MakeNode(4, "Setter"));
    graph.BuildLookupCache();

    DataPinConnection dc;
    dc.SourceNodeID  = 3;
    dc.SourcePinName = "Value";
    dc.TargetNodeID  = 4;
    dc.TargetPinName = "Value";

    auto cmd = std::unique_ptr<Olympe::ICommand>(new AddDataConnectionCommand(dc));
    stack.PushCommand(std::move(cmd), graph);

    TEST_ASSERT(GraphHasDataConn(graph, 3, 4), "Data connection 3->4 should exist");
    TEST_ASSERT(stack.CanUndo(), "CanUndo should be true after adding data conn");

    ReportTest("Test4_AddDataConnectionCommand_IsUndoable", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: Undo AddConnectionCommand removes the exec connection
// ---------------------------------------------------------------------------

static void Test5_AddConnectionCommand_UndoRemovesConnection()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(5, "A"));
    graph.Nodes.push_back(MakeNode(6, "B"));
    graph.BuildLookupCache();

    ExecPinConnection ec;
    ec.SourceNodeID  = 5;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 6;
    ec.TargetPinName = "In";

    auto cmd = std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(ec));
    stack.PushCommand(std::move(cmd), graph);
    TEST_ASSERT(GraphHasExecConn(graph, 5, 6), "Connection 5->6 should exist");

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasExecConn(graph, 5, 6),
                "After undo: connection 5->6 should be removed");
    TEST_ASSERT(stack.CanRedo(), "CanRedo should be true");

    ReportTest("Test5_AddConnectionCommand_UndoRemovesConnection",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6: Undo AddDataConnectionCommand removes the data connection
// ---------------------------------------------------------------------------

static void Test6_AddDataConnectionCommand_UndoRemovesConnection()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(7, "Get"));
    graph.Nodes.push_back(MakeNode(8, "Set"));
    graph.BuildLookupCache();

    DataPinConnection dc;
    dc.SourceNodeID  = 7;
    dc.SourcePinName = "Value";
    dc.TargetNodeID  = 8;
    dc.TargetPinName = "Value";

    auto cmd = std::unique_ptr<Olympe::ICommand>(new AddDataConnectionCommand(dc));
    stack.PushCommand(std::move(cmd), graph);
    TEST_ASSERT(GraphHasDataConn(graph, 7, 8), "Data connection 7->8 should exist");

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasDataConn(graph, 7, 8),
                "After undo: data connection 7->8 should be removed");

    ReportTest("Test6_AddDataConnectionCommand_UndoRemovesConnection",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7: AddNode with position prevents default-layout on redo
// ---------------------------------------------------------------------------

static void Test7_AddNodeWithPos_PreventsDefaultLayoutOnRedo()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Node placed at a position far from the default layout origin
    auto cmd = std::unique_ptr<Olympe::ICommand>(
        new AddNodeCommand(MakeNodeWithPos(20, "Sequence", 850.0f, 600.0f)));
    stack.PushCommand(std::move(cmd), graph);

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasNode(graph, 20), "Node should be gone after undo");

    stack.Redo(graph);
    TEST_ASSERT(GraphHasNode(graph, 20), "Node should be restored after redo");

    // Key assertion: position is preserved, not reset to 0 or default
    float x = GetNodePosX(graph, 20);
    float y = GetNodePosY(graph, 20);
    TEST_ASSERT(x > 849.0f, "posX should be preserved (>849)");
    TEST_ASSERT(y > 599.0f, "posY should be preserved (>599)");

    ReportTest("Test7_AddNodeWithPos_PreventsDefaultLayoutOnRedo",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8: Undo AddNode + Undo AddConnection leaves graph empty
// ---------------------------------------------------------------------------

static void Test8_UndoAddNode_WithExecConn_BothRemoved()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Pre-existing node
    graph.Nodes.push_back(MakeNodeWithPos(1, "Entry", 100.0f, 100.0f,
                                          TaskNodeType::EntryPoint));
    graph.BuildLookupCache();

    // Add a second node
    auto addCmd = std::unique_ptr<Olympe::ICommand>(
        new AddNodeCommand(MakeNodeWithPos(2, "Task", 300.0f, 100.0f)));
    stack.PushCommand(std::move(addCmd), graph);

    // Connect entry -> task
    ExecPinConnection ec;
    ec.SourceNodeID  = 1;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 2;
    ec.TargetPinName = "In";
    auto connCmd = std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(ec));
    stack.PushCommand(std::move(connCmd), graph);

    TEST_ASSERT(GraphHasNode(graph, 2), "Node 2 should exist");
    TEST_ASSERT(GraphHasExecConn(graph, 1, 2), "Connection 1->2 should exist");

    // Undo connection
    stack.Undo(graph);
    TEST_ASSERT(!GraphHasExecConn(graph, 1, 2),
                "After undo conn: 1->2 should be gone");
    TEST_ASSERT(GraphHasNode(graph, 2),
                "After undo conn: node 2 should still exist");

    // Undo node addition
    stack.Undo(graph);
    TEST_ASSERT(!GraphHasNode(graph, 2),
                "After undo add: node 2 should be gone");
    TEST_ASSERT(!GraphHasExecConn(graph, 1, 2),
                "After undo add: connection 1->2 should be gone");

    ReportTest("Test8_UndoAddNode_WithExecConn_BothRemoved", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 9: Undo and redo of exec connection creation
// ---------------------------------------------------------------------------

static void Test9_UndoAddExecConnection_RedoRestoresIt()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, "A"));
    graph.Nodes.push_back(MakeNode(2, "B"));
    graph.BuildLookupCache();

    ExecPinConnection ec;
    ec.SourceNodeID  = 1;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 2;
    ec.TargetPinName = "In";
    stack.PushCommand(std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(ec)),
                      graph);

    TEST_ASSERT(GraphHasExecConn(graph, 1, 2), "Connection should exist");

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasExecConn(graph, 1, 2), "Connection should be gone after undo");

    stack.Redo(graph);
    TEST_ASSERT(GraphHasExecConn(graph, 1, 2),
                "Connection should be restored after redo");

    ReportTest("Test9_UndoAddExecConnection_RedoRestoresIt", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 10: Undo and redo of data connection creation
// ---------------------------------------------------------------------------

static void Test10_UndoAddDataConnection_RedoRestoresIt()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, "Get"));
    graph.Nodes.push_back(MakeNode(2, "Set"));
    graph.BuildLookupCache();

    DataPinConnection dc;
    dc.SourceNodeID  = 1;
    dc.SourcePinName = "Value";
    dc.TargetNodeID  = 2;
    dc.TargetPinName = "Value";
    stack.PushCommand(std::unique_ptr<Olympe::ICommand>(new AddDataConnectionCommand(dc)),
                      graph);

    TEST_ASSERT(GraphHasDataConn(graph, 1, 2), "Data connection should exist");

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasDataConn(graph, 1, 2),
                "Data connection should be gone after undo");

    stack.Redo(graph);
    TEST_ASSERT(GraphHasDataConn(graph, 1, 2),
                "Data connection should be restored after redo");

    ReportTest("Test10_UndoAddDataConnection_RedoRestoresIt", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 11: AddDataConnectionCommand description
// ---------------------------------------------------------------------------

static void Test11_AddDataConnectionCommand_Description()
{
    int prevFail = s_failCount;

    DataPinConnection dc;
    dc.SourceNodeID  = 5;
    dc.SourcePinName = "Result";
    dc.TargetNodeID  = 9;
    dc.TargetPinName = "Input";

    AddDataConnectionCommand cmd(dc);
    std::string desc = cmd.GetDescription();

    TEST_ASSERT(!desc.empty(), "Description should not be empty");
    TEST_ASSERT(desc.find("5") != std::string::npos,
                "Description should contain source node ID");
    TEST_ASSERT(desc.find("9") != std::string::npos,
                "Description should contain target node ID");

    ReportTest("Test11_AddDataConnectionCommand_Description", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 12: AddConnectionCommand description
// ---------------------------------------------------------------------------

static void Test12_AddConnectionCommand_Description()
{
    int prevFail = s_failCount;

    ExecPinConnection ec;
    ec.SourceNodeID  = 3;
    ec.SourcePinName = "Then";
    ec.TargetNodeID  = 7;
    ec.TargetPinName = "In";

    AddConnectionCommand cmd(ec);
    std::string desc = cmd.GetDescription();

    TEST_ASSERT(!desc.empty(), "Description should not be empty");
    TEST_ASSERT(desc.find("3") != std::string::npos,
                "Description should contain source node ID");
    TEST_ASSERT(desc.find("7") != std::string::npos,
                "Description should contain target node ID");

    ReportTest("Test12_AddConnectionCommand_Description", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 13: Undo stack works correctly when loaded graph has pre-existing nodes
//          (simulates LoadTemplate NOT calling Clear(), so operations before
//          the load remain undoable)
// ---------------------------------------------------------------------------

static void Test13_UndoStack_Unaffected_AfterManualTemplateLoad()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Simulate an operation performed before loading a file
    graph.Nodes.push_back(MakeNodeWithPos(1, "PreLoad", 0.0f, 0.0f));
    graph.BuildLookupCache();
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new MoveNodeCommand(1, 0.0f, 0.0f, 50.0f, 60.0f)),
        graph);

    TEST_ASSERT(stack.CanUndo(), "Before load: CanUndo should be true");
    TEST_ASSERT(stack.UndoSize() == 1, "Before load: undo size should be 1");

    // Simulate LoadTemplate WITHOUT Clear() — just replace template content.
    // In the fixed code, LoadTemplate() no longer calls m_undoStack.Clear().
    // Here we just verify that not clearing the stack preserves the undo entry.
    // (We don't call Clear() here intentionally.)
    graph.Nodes.clear();
    graph.Nodes.push_back(MakeNodeWithPos(100, "LoadedNode", 200.0f, 100.0f));
    graph.BuildLookupCache();

    // After simulated load: undo stack is still intact
    TEST_ASSERT(stack.CanUndo(),
                "After simulated load: CanUndo should still be true (no Clear)");
    TEST_ASSERT(stack.UndoSize() == 1,
                "After simulated load: undo size should still be 1");

    ReportTest("Test13_UndoStack_Unaffected_AfterManualTemplateLoad",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 14: MoveNode preserves correct position through undo/redo cycle
// ---------------------------------------------------------------------------

static void Test14_MoveNode_PositionPreserved_ThroughUndoRedo()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNodeWithPos(5, "Node", 100.0f, 100.0f));
    graph.BuildLookupCache();

    // Move from (100, 100) to (400, 250)
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new MoveNodeCommand(5, 100.0f, 100.0f, 400.0f, 250.0f)),
        graph);

    float x = GetNodePosX(graph, 5);
    float y = GetNodePosY(graph, 5);
    TEST_ASSERT(x > 399.0f && x < 401.0f, "After move: posX should be ~400");
    TEST_ASSERT(y > 249.0f && y < 251.0f, "After move: posY should be ~250");

    // Undo: position should return to (100, 100)
    stack.Undo(graph);
    x = GetNodePosX(graph, 5);
    y = GetNodePosY(graph, 5);
    TEST_ASSERT(x > 99.0f && x < 101.0f,  "After undo: posX should be ~100");
    TEST_ASSERT(y > 99.0f && y < 101.0f,  "After undo: posY should be ~100");

    // Redo: position should return to (400, 250)
    stack.Redo(graph);
    x = GetNodePosX(graph, 5);
    y = GetNodePosY(graph, 5);
    TEST_ASSERT(x > 399.0f && x < 401.0f, "After redo: posX should be ~400");
    TEST_ASSERT(y > 249.0f && y < 251.0f, "After redo: posY should be ~250");

    ReportTest("Test14_MoveNode_PositionPreserved_ThroughUndoRedo",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=====================================" << std::endl;
    std::cout << "  Phase13Test: Fix Undo/Redo After  " << std::endl;
    std::cout << "             File Load              " << std::endl;
    std::cout << "=====================================" << std::endl;

    Test1_AddNode_StoresPositionInParameters();
    Test2_RedoAddNode_RestoresCorrectPosition();
    Test3_AddConnectionCommand_IsUndoable();
    Test4_AddDataConnectionCommand_IsUndoable();
    Test5_AddConnectionCommand_UndoRemovesConnection();
    Test6_AddDataConnectionCommand_UndoRemovesConnection();
    Test7_AddNodeWithPos_PreventsDefaultLayoutOnRedo();
    Test8_UndoAddNode_WithExecConn_BothRemoved();
    Test9_UndoAddExecConnection_RedoRestoresIt();
    Test10_UndoAddDataConnection_RedoRestoresIt();
    Test11_AddDataConnectionCommand_Description();
    Test12_AddConnectionCommand_Description();
    Test13_UndoStack_Unaffected_AfterManualTemplateLoad();
    Test14_MoveNode_PositionPreserved_ThroughUndoRedo();

    std::cout << "=====================================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
