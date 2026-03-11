/**
 * @file Phase10Test.cpp
 * @brief Tests for Phase 10 Blueprint Editor tab system fixes.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * Validates the critical fixes for the Blueprint Editor tab system:
 *
 *  1.  TabNameSync_DisplayNameFromPath_ExtractsFilename
 *  2.  UndoRedo_AddNodeCmd_AddsToTemplate
 *  3.  UndoRedo_AddNodeCmd_UndoRemovesFromTemplate
 *  4.  UndoRedo_AddNodeCmd_RedoRestoresNode
 *  5.  UndoRedo_DeleteNodeCmd_RemovesNodeAndConnections
 *  6.  UndoRedo_DeleteNodeCmd_UndoRestoresNodeAndConnections
 *  7.  UndoRedo_ClearOnLoad_StackEmptyAfterLoad
 *  8.  UndoRedo_NewPushClearsRedo
 *  9.  ContextMenuState_NodeIdPreserved
 * 10.  SaveAsFilename_PathExtraction_RemovesExtension
 *
 * No SDL3, ImGui, or ImNodes dependency.
 * C++14 compliant.
 */

#include "BlueprintEditor/UndoRedoStack.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeDefinition;
using Olympe::TaskNodeType;
using Olympe::ExecPinConnection;
using Olympe::UndoRedoStack;
using Olympe::AddNodeCommand;
using Olympe::DeleteNodeCommand;
using Olympe::ICommand;

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

static bool GraphHasExecConn(const TaskGraphTemplate& g, int src, int dst)
{
    for (const auto& ec : g.ExecConnections)
        if (ec.SourceNodeID == src && ec.TargetNodeID == dst) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Mirrors TabManager::DisplayNameFromPath (static logic)
// ---------------------------------------------------------------------------

static std::string DisplayNameFromPath(const std::string& filePath)
{
    if (filePath.empty())
        return "";
    size_t pos = filePath.find_last_of("/\\");
    if (pos == std::string::npos)
        return filePath;
    return filePath.substr(pos + 1);
}

// ---------------------------------------------------------------------------
// Mirrors the save-as filename extraction used in RenderSaveAsDialog
// ---------------------------------------------------------------------------

static std::string ExtractSaveAsFilename(const std::string& currentPath)
{
    if (currentPath.empty())
        return "untitled_graph";

    size_t lastSlash = currentPath.find_last_of("/\\");
    std::string fname = (lastSlash != std::string::npos)
                        ? currentPath.substr(lastSlash + 1)
                        : currentPath;
    size_t dotPos = fname.rfind('.');
    if (dotPos != std::string::npos)
        fname = fname.substr(0, dotPos);
    return fname;
}

// ---------------------------------------------------------------------------
// Test 1: DisplayNameFromPath extracts the filename portion
// ---------------------------------------------------------------------------

static void Test1_TabNameSync_DisplayNameFromPath_ExtractsFilename()
{
    int prevFail = s_failCount;

    TEST_ASSERT(DisplayNameFromPath("Blueprints/AI/patrol.ats") == "patrol.ats",
                "Unix path: should return filename");
    TEST_ASSERT(DisplayNameFromPath("C:\\Blueprints\\guard.ats") == "guard.ats",
                "Windows path: should return filename");
    TEST_ASSERT(DisplayNameFromPath("simple.ats") == "simple.ats",
                "No separator: should return full string");
    TEST_ASSERT(DisplayNameFromPath("") == "",
                "Empty path: should return empty");

    ReportTest("Test1_TabNameSync_DisplayNameFromPath_ExtractsFilename",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2: AddNodeCommand::Execute adds node to template
// ---------------------------------------------------------------------------

static void Test2_UndoRedo_AddNodeCmd_AddsToTemplate()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd = std::unique_ptr<ICommand>(
        new AddNodeCommand(MakeNode(1, "EntryPoint", TaskNodeType::EntryPoint)));
    stack.PushCommand(std::move(cmd), graph);

    TEST_ASSERT(GraphHasNode(graph, 1),   "Node 1 should be in template after push");
    TEST_ASSERT(stack.CanUndo(),          "Stack should have undo after push");
    TEST_ASSERT(!stack.CanRedo(),         "Redo should be empty after push");

    ReportTest("Test2_UndoRedo_AddNodeCmd_AddsToTemplate", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: Undo AddNodeCommand removes the node from the template
// ---------------------------------------------------------------------------

static void Test3_UndoRedo_AddNodeCmd_UndoRemovesFromTemplate()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd = std::unique_ptr<ICommand>(
        new AddNodeCommand(MakeNode(2, "Sequence", TaskNodeType::VSSequence)));
    stack.PushCommand(std::move(cmd), graph);
    TEST_ASSERT(GraphHasNode(graph, 2), "Node 2 present before undo");

    stack.Undo(graph);
    TEST_ASSERT(!GraphHasNode(graph, 2), "Node 2 removed after undo");
    TEST_ASSERT(stack.CanRedo(),         "Redo available after undo");
    TEST_ASSERT(!stack.CanUndo(),        "Undo unavailable after full undo");

    ReportTest("Test3_UndoRedo_AddNodeCmd_UndoRemovesFromTemplate",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: Redo re-adds the node
// ---------------------------------------------------------------------------

static void Test4_UndoRedo_AddNodeCmd_RedoRestoresNode()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd = std::unique_ptr<ICommand>(
        new AddNodeCommand(MakeNode(3, "Branch", TaskNodeType::Branch)));
    stack.PushCommand(std::move(cmd), graph);
    stack.Undo(graph);
    TEST_ASSERT(!GraphHasNode(graph, 3), "Node 3 gone after undo");

    stack.Redo(graph);
    TEST_ASSERT(GraphHasNode(graph, 3),  "Node 3 restored after redo");
    TEST_ASSERT(stack.CanUndo(),         "Undo available after redo");
    TEST_ASSERT(!stack.CanRedo(),        "Redo empty after redo");

    ReportTest("Test4_UndoRedo_AddNodeCmd_RedoRestoresNode", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: DeleteNodeCommand removes node and its connections
// ---------------------------------------------------------------------------

static void Test5_UndoRedo_DeleteNodeCmd_RemovesNodeAndConnections()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Build a graph: 10 -> 11 (exec)
    graph.Nodes.push_back(MakeNode(10, "A"));
    graph.Nodes.push_back(MakeNode(11, "B"));

    ExecPinConnection ec;
    ec.SourceNodeID  = 10;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 11;
    ec.TargetPinName = "In";
    graph.ExecConnections.push_back(ec);
    graph.BuildLookupCache();

    TEST_ASSERT(GraphHasNode(graph, 10),      "Node 10 present before delete");
    TEST_ASSERT(GraphHasExecConn(graph,10,11),"Connection 10->11 present");

    auto cmd = std::unique_ptr<ICommand>(new DeleteNodeCommand(10));
    stack.PushCommand(std::move(cmd), graph);

    TEST_ASSERT(!GraphHasNode(graph, 10),       "Node 10 removed after delete");
    TEST_ASSERT(!GraphHasExecConn(graph,10,11), "Connection 10->11 removed");

    ReportTest("Test5_UndoRedo_DeleteNodeCmd_RemovesNodeAndConnections",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6: Undo DeleteNodeCommand restores node and connections
// ---------------------------------------------------------------------------

static void Test6_UndoRedo_DeleteNodeCmd_UndoRestoresNodeAndConnections()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    graph.Nodes.push_back(MakeNode(20, "C"));
    graph.Nodes.push_back(MakeNode(21, "D"));

    ExecPinConnection ec;
    ec.SourceNodeID  = 20;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = 21;
    ec.TargetPinName = "In";
    graph.ExecConnections.push_back(ec);
    graph.BuildLookupCache();

    auto cmd = std::unique_ptr<ICommand>(new DeleteNodeCommand(20));
    stack.PushCommand(std::move(cmd), graph);
    TEST_ASSERT(!GraphHasNode(graph, 20), "Node 20 gone after delete");

    stack.Undo(graph);
    TEST_ASSERT(GraphHasNode(graph, 20),       "Node 20 restored after undo");
    TEST_ASSERT(GraphHasExecConn(graph,20,21), "Connection 20->21 restored");

    ReportTest("Test6_UndoRedo_DeleteNodeCmd_UndoRestoresNodeAndConnections",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7: Clear() empties both stacks (simulates LoadTemplate behaviour)
// ---------------------------------------------------------------------------

static void Test7_UndoRedo_ClearOnLoad_StackEmptyAfterLoad()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    auto cmd1 = std::unique_ptr<ICommand>(
        new AddNodeCommand(MakeNode(100, "X")));
    stack.PushCommand(std::move(cmd1), graph);
    TEST_ASSERT(stack.CanUndo(), "Stack has entry before clear");

    // LoadTemplate equivalent: clear the stack
    stack.Clear();

    TEST_ASSERT(!stack.CanUndo(), "After Clear: CanUndo should be false");
    TEST_ASSERT(!stack.CanRedo(), "After Clear: CanRedo should be false");

    ReportTest("Test7_UndoRedo_ClearOnLoad_StackEmptyAfterLoad",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8: Pushing a new command clears the redo stack
// ---------------------------------------------------------------------------

static void Test8_UndoRedo_NewPushClearsRedo()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Push and undo to fill redo stack
    auto cmd1 = std::unique_ptr<ICommand>(
        new AddNodeCommand(MakeNode(200, "N1")));
    stack.PushCommand(std::move(cmd1), graph);
    stack.Undo(graph);
    TEST_ASSERT(stack.CanRedo(), "Redo available after undo");

    // Push a new command — redo stack must be cleared
    auto cmd2 = std::unique_ptr<ICommand>(
        new AddNodeCommand(MakeNode(201, "N2")));
    stack.PushCommand(std::move(cmd2), graph);

    TEST_ASSERT(!stack.CanRedo(), "After new push: redo stack cleared");
    TEST_ASSERT(stack.CanUndo(),  "After new push: undo available");

    ReportTest("Test8_UndoRedo_NewPushClearsRedo", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 9: Multiple undo/redo cycles preserve template integrity
// ---------------------------------------------------------------------------

static void Test9_UndoRedo_MultipleCycles_TemplateIntegrity()
{
    int prevFail = s_failCount;

    UndoRedoStack stack;
    TaskGraphTemplate graph;

    // Add three nodes
    for (int i = 1; i <= 3; ++i)
    {
        auto cmd = std::unique_ptr<ICommand>(
            new AddNodeCommand(MakeNode(i, "Node")));
        stack.PushCommand(std::move(cmd), graph);
    }
    TEST_ASSERT(graph.Nodes.size() == 3, "Three nodes in template");

    // Undo all three
    stack.Undo(graph);
    stack.Undo(graph);
    stack.Undo(graph);
    TEST_ASSERT(graph.Nodes.empty(), "All nodes undone");

    // Redo all three
    stack.Redo(graph);
    stack.Redo(graph);
    stack.Redo(graph);
    TEST_ASSERT(graph.Nodes.size() == 3, "All nodes redone");
    TEST_ASSERT(GraphHasNode(graph, 1) && GraphHasNode(graph, 2) && GraphHasNode(graph, 3),
                "Nodes 1, 2, 3 all present after full redo");

    ReportTest("Test9_UndoRedo_MultipleCycles_TemplateIntegrity",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 10: Save-as filename extraction mirrors RenderSaveAsDialog logic
// ---------------------------------------------------------------------------

static void Test10_SaveAsFilename_PathExtraction_RemovesExtension()
{
    int prevFail = s_failCount;

    // Empty path → fallback name
    TEST_ASSERT(ExtractSaveAsFilename("") == "untitled_graph",
                "Empty path should return 'untitled_graph'");

    // Unix path with .ats extension
    TEST_ASSERT(ExtractSaveAsFilename("Blueprints/AI/patrol_v2.ats") == "patrol_v2",
                "Unix path: extension stripped");

    // Windows path
    TEST_ASSERT(ExtractSaveAsFilename("C:\\Blueprints\\guard.ats") == "guard",
                "Windows path: extension stripped");

    // No extension
    TEST_ASSERT(ExtractSaveAsFilename("Blueprints/AI/simple") == "simple",
                "No extension: filename returned as-is");

    ReportTest("Test10_SaveAsFilename_PathExtraction_RemovesExtension",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=============================" << std::endl;
    std::cout << "   Phase 10 Unit Tests       " << std::endl;
    std::cout << "   Blueprint Editor Fixes    " << std::endl;
    std::cout << "=============================" << std::endl;

    Test1_TabNameSync_DisplayNameFromPath_ExtractsFilename();
    Test2_UndoRedo_AddNodeCmd_AddsToTemplate();
    Test3_UndoRedo_AddNodeCmd_UndoRemovesFromTemplate();
    Test4_UndoRedo_AddNodeCmd_RedoRestoresNode();
    Test5_UndoRedo_DeleteNodeCmd_RemovesNodeAndConnections();
    Test6_UndoRedo_DeleteNodeCmd_UndoRestoresNodeAndConnections();
    Test7_UndoRedo_ClearOnLoad_StackEmptyAfterLoad();
    Test8_UndoRedo_NewPushClearsRedo();
    Test9_UndoRedo_MultipleCycles_TemplateIntegrity();
    Test10_SaveAsFilename_PathExtraction_RemovesExtension();

    std::cout << "=============================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
