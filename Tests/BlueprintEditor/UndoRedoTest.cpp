/**
 * @file UndoRedoTest.cpp
 * @brief Unit tests for UndoRedoStack (Phase 6 ATS VS editor).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   1.  UndoRedo_InitialState_CannotUndoOrRedo
 *   2.  UndoRedo_PushAddNode_CanUndo
 *   3.  UndoRedo_UndoAddNode_NodeRemoved
 *   4.  UndoRedo_RedoAddNode_NodeRestored
 *   5.  UndoRedo_PushDeleteNode_NodeRemoved
 *   6.  UndoRedo_UndoDeleteNode_NodeRestored
 *   7.  UndoRedo_PushAddConnection_ConnectionStored
 *   8.  UndoRedo_UndoAddConnection_ConnectionRemoved
 *   9.  UndoRedo_PushMove_PositionUpdated
 *  10.  UndoRedo_StackOverflow_OldestEntryDropped
 *
 * No SDL3, ImGui, or Editor dependency.
 * C++14 compliant — no C++17/20 features.
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
using Olympe::UndoRedoStack;
using Olympe::AddNodeCommand;
using Olympe::DeleteNodeCommand;
using Olympe::MoveNodeCommand;
using Olympe::AddConnectionCommand;
using Olympe::NODE_INDEX_NONE;
using Olympe::ParameterBindingType;
using Olympe::TaskValue;

// ---------------------------------------------------------------------------
// Test helpers
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

static TaskNodeDefinition MakeNode(int id, const std::string& name,
                                    TaskNodeType type = TaskNodeType::AtomicTask)
{
    TaskNodeDefinition def;
    def.NodeID   = id;
    def.NodeName = name;
    def.Type     = type;
    return def;
}

static bool GraphHasNode(const TaskGraphTemplate& g, int id)
{
    for (const auto& n : g.Nodes)
        if (n.NodeID == id) return true;
    return false;
}

static bool GraphHasConnection(const TaskGraphTemplate& g,
                                int src, int dst,
                                const std::string& srcPin = "Out")
{
    for (const auto& ec : g.ExecConnections)
    {
        if (ec.SourceNodeID == src && ec.TargetNodeID == dst
            && ec.SourcePinName == srcPin) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Test 1: Initial state — stack is empty
// ---------------------------------------------------------------------------

static void Test1_InitialState_CannotUndoOrRedo()
{
    bool passed = true;
    UndoRedoStack stack;

    TEST_ASSERT(!stack.CanUndo(), "Initial state: CanUndo should be false");
    TEST_ASSERT(!stack.CanRedo(), "Initial state: CanRedo should be false");
    TEST_ASSERT(stack.UndoSize() == 0, "Initial state: UndoSize should be 0");
    TEST_ASSERT(stack.RedoSize() == 0, "Initial state: RedoSize should be 0");
    TEST_ASSERT(stack.PeekUndoDescription() == "", "Initial state: PeekUndo should be empty");

    passed = (s_failCount == 0);
    ReportTest("Test1_InitialState_CannotUndoOrRedo", passed);
}

// ---------------------------------------------------------------------------
// Test 2: PushAddNode — CanUndo becomes true
// ---------------------------------------------------------------------------

static void Test2_PushAddNode_CanUndo()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd = std::unique_ptr<Olympe::ICommand>(
        new AddNodeCommand(MakeNode(1, "EntryPoint", TaskNodeType::EntryPoint)));
    stack.PushCommand(std::move(cmd), graph);

    TEST_ASSERT(stack.CanUndo(), "After push: CanUndo should be true");
    TEST_ASSERT(!stack.CanRedo(), "After push: CanRedo should be false");
    TEST_ASSERT(stack.UndoSize() == 1, "After push: UndoSize should be 1");
    TEST_ASSERT(GraphHasNode(graph, 1), "After push: node 1 should be in graph");

    ReportTest("Test2_PushAddNode_CanUndo", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: Undo AddNode — node removed from graph
// ---------------------------------------------------------------------------

static void Test3_UndoAddNode_NodeRemoved()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd = std::unique_ptr<Olympe::ICommand>(
        new AddNodeCommand(MakeNode(42, "MyAction")));
    stack.PushCommand(std::move(cmd), graph);
    TEST_ASSERT(GraphHasNode(graph, 42), "Before undo: node 42 should exist");

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasNode(graph, 42), "After undo: node 42 should be removed");
    TEST_ASSERT(!stack.CanUndo(), "After undo: CanUndo should be false");
    TEST_ASSERT(stack.CanRedo(), "After undo: CanRedo should be true");

    ReportTest("Test3_UndoAddNode_NodeRemoved", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: Redo AddNode — node restored
// ---------------------------------------------------------------------------

static void Test4_RedoAddNode_NodeRestored()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd = std::unique_ptr<Olympe::ICommand>(
        new AddNodeCommand(MakeNode(7, "Branch", TaskNodeType::Branch)));
    stack.PushCommand(std::move(cmd), graph);
    stack.Undo(graph);
    TEST_ASSERT(!GraphHasNode(graph, 7), "After undo: node 7 gone");

    stack.Redo(graph);
    TEST_ASSERT(GraphHasNode(graph, 7), "After redo: node 7 restored");
    TEST_ASSERT(stack.CanUndo(), "After redo: CanUndo true");
    TEST_ASSERT(!stack.CanRedo(), "After redo: CanRedo false");

    ReportTest("Test4_RedoAddNode_NodeRestored", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: PushDeleteNode — node removed from graph
// ---------------------------------------------------------------------------

static void Test5_PushDeleteNode_NodeRemoved()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(10, "ToDelete"));
    graph.BuildLookupCache();

    auto cmd = std::unique_ptr<Olympe::ICommand>(new DeleteNodeCommand(10));
    stack.PushCommand(std::move(cmd), graph);

    TEST_ASSERT(!GraphHasNode(graph, 10), "After DeleteNode: node 10 should be gone");

    ReportTest("Test5_PushDeleteNode_NodeRemoved", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6: Undo DeleteNode — node restored
// ---------------------------------------------------------------------------

static void Test6_UndoDeleteNode_NodeRestored()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(20, "Restored"));
    graph.BuildLookupCache();

    auto cmd = std::unique_ptr<Olympe::ICommand>(new DeleteNodeCommand(20));
    stack.PushCommand(std::move(cmd), graph);
    TEST_ASSERT(!GraphHasNode(graph, 20), "After delete: node 20 gone");

    stack.Undo(graph);
    TEST_ASSERT(GraphHasNode(graph, 20), "After undo delete: node 20 restored");

    ReportTest("Test6_UndoDeleteNode_NodeRestored", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7: PushAddConnection — connection stored
// ---------------------------------------------------------------------------

static void Test7_PushAddConnection_ConnectionStored()
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

    TEST_ASSERT(GraphHasConnection(graph, 1, 2), "After add conn: 1->2 should exist");

    ReportTest("Test7_PushAddConnection_ConnectionStored", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8: Undo AddConnection — connection removed
// ---------------------------------------------------------------------------

static void Test8_UndoAddConnection_ConnectionRemoved()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(3, "A"));
    graph.Nodes.push_back(MakeNode(4, "B"));
    graph.BuildLookupCache();

    ExecPinConnection ec;
    ec.SourceNodeID  = 3;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 4;
    ec.TargetPinName = "In";

    auto cmd = std::unique_ptr<Olympe::ICommand>(new AddConnectionCommand(ec));
    stack.PushCommand(std::move(cmd), graph);
    TEST_ASSERT(GraphHasConnection(graph, 3, 4), "Connection 3->4 present");

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasConnection(graph, 3, 4), "After undo: connection 3->4 removed");

    ReportTest("Test8_UndoAddConnection_ConnectionRemoved", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 9: PushMove — position updated
// ---------------------------------------------------------------------------

static void Test9_PushMove_PositionUpdated()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(5, "MovableNode"));
    graph.BuildLookupCache();

    auto cmd = std::unique_ptr<Olympe::ICommand>(new MoveNodeCommand(5, 0.f, 0.f, 200.f, 300.f));
    stack.PushCommand(std::move(cmd), graph);

    const Olympe::TaskNodeDefinition* n = graph.GetNode(5);
    TEST_ASSERT(n != nullptr, "Node 5 should exist after move");
    if (n)
    {
        auto itX = n->Parameters.find("__posX");
        auto itY = n->Parameters.find("__posY");
        TEST_ASSERT(itX != n->Parameters.end(), "Node 5 should have __posX");
        TEST_ASSERT(itY != n->Parameters.end(), "Node 5 should have __posY");
        if (itX != n->Parameters.end())
        {
            float x = itX->second.LiteralValue.AsFloat();
            TEST_ASSERT(x > 199.f && x < 201.f, "posX should be ~200");
        }
        if (itY != n->Parameters.end())
        {
            float y = itY->second.LiteralValue.AsFloat();
            TEST_ASSERT(y > 299.f && y < 301.f, "posY should be ~300");
        }
    }

    // Undo should restore old position
    stack.Undo(graph);
    n = graph.GetNode(5);
    if (n)
    {
        auto itX = n->Parameters.find("__posX");
        if (itX != n->Parameters.end())
        {
            float x = itX->second.LiteralValue.AsFloat();
            TEST_ASSERT(x > -0.01f && x < 0.01f, "After undo: posX should be ~0");
        }
    }

    ReportTest("Test9_PushMove_PositionUpdated", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 10: Stack overflow — oldest entry dropped
// ---------------------------------------------------------------------------

static void Test10_StackOverflow_OldestEntryDropped()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Push MAX_STACK_SIZE + 10 commands
    const std::size_t PUSH_COUNT = UndoRedoStack::MAX_STACK_SIZE + 10;
    for (std::size_t i = 0; i < PUSH_COUNT; ++i)
    {
        TaskNodeDefinition def = MakeNode(static_cast<int>(i), "N");
        auto cmd = std::unique_ptr<Olympe::ICommand>(new AddNodeCommand(def));
        stack.PushCommand(std::move(cmd), graph);
    }

    TEST_ASSERT(stack.UndoSize() == UndoRedoStack::MAX_STACK_SIZE,
                "Undo stack size should be capped at MAX_STACK_SIZE");
    TEST_ASSERT(!stack.CanRedo(), "Redo stack should be empty after pushes");

    ReportTest("Test10_StackOverflow_OldestEntryDropped", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=============================" << std::endl;
    std::cout << "   UndoRedoStack Unit Tests  " << std::endl;
    std::cout << "=============================" << std::endl;

    Test1_InitialState_CannotUndoOrRedo();
    Test2_PushAddNode_CanUndo();
    Test3_UndoAddNode_NodeRemoved();
    Test4_RedoAddNode_NodeRestored();
    Test5_PushDeleteNode_NodeRemoved();
    Test6_UndoDeleteNode_NodeRestored();
    Test7_PushAddConnection_ConnectionStored();
    Test8_UndoAddConnection_ConnectionRemoved();
    Test9_PushMove_PositionUpdated();
    Test10_StackOverflow_OldestEntryDropped();

    std::cout << "=============================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
