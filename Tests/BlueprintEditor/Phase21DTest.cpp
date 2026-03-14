/**
 * @file Phase21DTest.cpp
 * @brief Unit tests for Phase 21-D — Dynamic Pins Sequence/Switch.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * Tests operate directly on TaskGraphTemplate, AddDynamicPinCommand and
 * RemoveExecPinCommand — no SDL3, ImGui, or ImNodes dependency.
 *
 * Test cases:
 *   1. Test_AddPin_Sequence_UndoRedo
 *      — Add pin on VSSequence → pin created, undoable, redoable.
 *   2. Test_RemovePin_NoLink_UndoRedo
 *      — Remove pin with no link → pin removed, undoable, redoable.
 *   3. Test_RemovePin_WithLink_UndoRedo
 *      — Remove pin with link → pin and link removed; undo restores both.
 *   4. Test_AddThenRemove_DoubleUndo
 *      — Add then Remove → initial state restored after double undo.
 *   5. Test_BasePin_NotInDynamicList
 *      — The base pin ("Out" for Sequence, "Case_0" for Switch) is not in
 *        DynamicExecOutputPins and therefore has no remove command created.
 *   6. Test_AddPin_Switch_UndoRedo
 *      — Add pin on Switch ("Case_1") → pin created, undoable, redoable.
 *   7. Test_RemovePin_AtIndex_Correct
 *      — Removing the middle pin of three dynamic pins leaves the others intact.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/UndoRedoStack.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeDefinition;
using Olympe::ExecPinConnection;
using Olympe::AddDynamicPinCommand;
using Olympe::RemoveExecPinCommand;
using Olympe::UndoRedoStack;
using Olympe::TaskNodeType;

// ---------------------------------------------------------------------------
// Test infrastructure (same pattern as Phase20Test.cpp / Phase21ATest.cpp)
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
// Helper: build a minimal graph with a single node
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
    graph.BuildLookupCache();
}

// ---------------------------------------------------------------------------
// Test 1 — Add pin on VSSequence: Execute / Undo / Redo
// ---------------------------------------------------------------------------

static void Test_AddPin_Sequence_UndoRedo()
{
    const std::string testName = "AddPin_Sequence_UndoRedo";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::VSSequence, "Seq"));
    graph.BuildLookupCache();

    UndoRedoStack stack;

    // --- Execute ---
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddDynamicPinCommand(1, "Out_1")),
        graph);

    const TaskNodeDefinition* node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after add");
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                "DynamicExecOutputPins size must be 1 after add");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Out_1",
                "First dynamic pin must be 'Out_1'");

    // --- Undo ---
    stack.Undo(graph);

    node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after undo");
    TEST_ASSERT(node->DynamicExecOutputPins.empty(),
                "DynamicExecOutputPins must be empty after undo");
    TEST_ASSERT(stack.CanRedo(), "Redo must be available after undo");

    // --- Redo ---
    stack.Redo(graph);

    node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after redo");
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                "DynamicExecOutputPins size must be 1 after redo");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Out_1",
                "Dynamic pin must be 'Out_1' after redo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2 — Remove pin with no link: Execute / Undo / Redo
// ---------------------------------------------------------------------------

static void Test_RemovePin_NoLink_UndoRedo()
{
    const std::string testName = "RemovePin_NoLink_UndoRedo";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    TaskNodeDefinition seqNode = MakeNode(1, TaskNodeType::VSSequence, "Seq");
    seqNode.DynamicExecOutputPins.push_back("Out_1");
    graph.Nodes.push_back(seqNode);
    graph.BuildLookupCache();

    UndoRedoStack stack;

    // --- Execute: remove Out_1 (index 0 in DynamicExecOutputPins), no link ---
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new RemoveExecPinCommand(1, "Out_1", 0, -1, "")),
        graph);

    const TaskNodeDefinition* node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after remove");
    TEST_ASSERT(node->DynamicExecOutputPins.empty(),
                "DynamicExecOutputPins must be empty after remove");

    // --- Undo ---
    stack.Undo(graph);

    node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after undo");
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                "DynamicExecOutputPins size must be 1 after undo");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Out_1",
                "Dynamic pin must be 'Out_1' after undo");

    // --- Redo ---
    stack.Redo(graph);

    node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after redo");
    TEST_ASSERT(node->DynamicExecOutputPins.empty(),
                "DynamicExecOutputPins must be empty after redo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3 — Remove pin WITH link: Execute removes pin+link; Undo restores both
// ---------------------------------------------------------------------------

static void Test_RemovePin_WithLink_UndoRedo()
{
    const std::string testName = "RemovePin_WithLink_UndoRedo";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    TaskNodeDefinition seqNode = MakeNode(1, TaskNodeType::VSSequence, "Seq");
    seqNode.DynamicExecOutputPins.push_back("Out_1");
    graph.Nodes.push_back(seqNode);
    graph.Nodes.push_back(MakeNode(2, TaskNodeType::AtomicTask, "Task"));
    graph.BuildLookupCache();

    // Add link from Out_1 -> node 2 In
    AddExecConn(graph, 1, "Out_1", 2, "In");
    TEST_ASSERT(graph.ExecConnections.size() == 1, "Setup: one link must exist");

    UndoRedoStack stack;

    // --- Execute: remove Out_1, link to node 2 ---
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new RemoveExecPinCommand(1, "Out_1", 0, 2, "In")),
        graph);

    const TaskNodeDefinition* node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after remove");
    TEST_ASSERT(node->DynamicExecOutputPins.empty(),
                "DynamicExecOutputPins must be empty after remove");
    TEST_ASSERT(graph.ExecConnections.empty(),
                "ExecConnections must be empty after remove (link removed)");

    // --- Undo ---
    stack.Undo(graph);

    node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after undo");
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                "DynamicExecOutputPins size must be 1 after undo (pin restored)");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Out_1",
                "Dynamic pin must be 'Out_1' after undo");
    TEST_ASSERT(graph.ExecConnections.size() == 1,
                "ExecConnections must have 1 entry after undo (link restored)");
    TEST_ASSERT(graph.ExecConnections[0].SourceNodeID  == 1,  "Link src must be 1");
    TEST_ASSERT(graph.ExecConnections[0].SourcePinName == "Out_1", "Link src pin must be 'Out_1'");
    TEST_ASSERT(graph.ExecConnections[0].TargetNodeID  == 2,  "Link dst must be 2");
    TEST_ASSERT(graph.ExecConnections[0].TargetPinName == "In",   "Link dst pin must be 'In'");

    // --- Redo ---
    stack.Redo(graph);

    node = graph.GetNode(1);
    TEST_ASSERT(node != nullptr, "Node must exist after redo");
    TEST_ASSERT(node->DynamicExecOutputPins.empty(),
                "DynamicExecOutputPins must be empty after redo");
    TEST_ASSERT(graph.ExecConnections.empty(),
                "ExecConnections must be empty after redo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4 — Add then Remove: double undo returns to initial state
// ---------------------------------------------------------------------------

static void Test_AddThenRemove_DoubleUndo()
{
    const std::string testName = "AddThenRemove_DoubleUndo";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(1, TaskNodeType::VSSequence, "Seq"));
    graph.BuildLookupCache();

    UndoRedoStack stack;

    // Add Out_1
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddDynamicPinCommand(1, "Out_1")),
        graph);

    TEST_ASSERT(graph.GetNode(1)->DynamicExecOutputPins.size() == 1,
                "After add: 1 dynamic pin");

    // Remove Out_1 (index 0, no link)
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new RemoveExecPinCommand(1, "Out_1", 0, -1, "")),
        graph);

    TEST_ASSERT(graph.GetNode(1)->DynamicExecOutputPins.empty(),
                "After remove: 0 dynamic pins");

    // Undo remove → pin restored
    stack.Undo(graph);
    TEST_ASSERT(graph.GetNode(1)->DynamicExecOutputPins.size() == 1,
                "After undo-remove: 1 dynamic pin");

    // Undo add → initial state (0 dynamic pins)
    stack.Undo(graph);
    TEST_ASSERT(graph.GetNode(1)->DynamicExecOutputPins.empty(),
                "After double undo: initial state (0 dynamic pins)");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5 — Base pin is not in DynamicExecOutputPins
//
// The base pin ("Out" for Sequence, "Case_0" for Switch) must NOT appear in
// DynamicExecOutputPins — it is a static pin and cannot be removed.
// ---------------------------------------------------------------------------

static void Test_BasePin_NotInDynamicList()
{
    const std::string testName = "BasePin_NotInDynamicList";
    int prevFail = s_failCount;

    // VSSequence: default node has no dynamic pins at all
    {
        TaskNodeDefinition seqNode = MakeNode(1, TaskNodeType::VSSequence, "Seq");
        TEST_ASSERT(seqNode.DynamicExecOutputPins.empty(),
                    "Sequence: DynamicExecOutputPins must be empty on creation");
        // "Out" is NOT in DynamicExecOutputPins — it is a static pin.
    }

    // After adding Out_1, the dynamic list contains only "Out_1", not "Out"
    {
        TaskGraphTemplate graph;
        graph.Nodes.push_back(MakeNode(1, TaskNodeType::VSSequence, "Seq"));
        graph.BuildLookupCache();

        AddDynamicPinCommand cmd(1, "Out_1");
        cmd.Execute(graph);

        const TaskNodeDefinition* node = graph.GetNode(1);
        TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                    "Sequence: after add, DynamicExecOutputPins has 1 entry");
        TEST_ASSERT(node->DynamicExecOutputPins[0] == "Out_1",
                    "Sequence: dynamic pin must be 'Out_1', not 'Out'");
    }

    // VSSwitch: default node has no dynamic pins at all
    {
        TaskNodeDefinition swNode = MakeNode(2, TaskNodeType::Switch, "Switch");
        TEST_ASSERT(swNode.DynamicExecOutputPins.empty(),
                    "Switch: DynamicExecOutputPins must be empty on creation");
        // "Case_0" is NOT in DynamicExecOutputPins — it is a static pin.
    }

    // After adding Case_1, the dynamic list contains only "Case_1", not "Case_0"
    {
        TaskGraphTemplate graph;
        graph.Nodes.push_back(MakeNode(2, TaskNodeType::Switch, "Switch"));
        graph.BuildLookupCache();

        AddDynamicPinCommand cmd(2, "Case_1");
        cmd.Execute(graph);

        const TaskNodeDefinition* node = graph.GetNode(2);
        TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                    "Switch: after add, DynamicExecOutputPins has 1 entry");
        TEST_ASSERT(node->DynamicExecOutputPins[0] == "Case_1",
                    "Switch: dynamic pin must be 'Case_1', not 'Case_0'");
    }

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6 — Add pin on Switch ("Case_1"): Execute / Undo / Redo
// ---------------------------------------------------------------------------

static void Test_AddPin_Switch_UndoRedo()
{
    const std::string testName = "AddPin_Switch_UndoRedo";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(10, TaskNodeType::Switch, "Switch"));
    graph.BuildLookupCache();

    UndoRedoStack stack;

    // --- Execute ---
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(new AddDynamicPinCommand(10, "Case_1")),
        graph);

    const TaskNodeDefinition* node = graph.GetNode(10);
    TEST_ASSERT(node != nullptr, "Node must exist after add");
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                "Switch: DynamicExecOutputPins size must be 1 after add");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Case_1",
                "Switch: first dynamic pin must be 'Case_1'");

    // --- Undo ---
    stack.Undo(graph);
    node = graph.GetNode(10);
    TEST_ASSERT(node->DynamicExecOutputPins.empty(),
                "Switch: DynamicExecOutputPins must be empty after undo");

    // --- Redo ---
    stack.Redo(graph);
    node = graph.GetNode(10);
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 1,
                "Switch: DynamicExecOutputPins size must be 1 after redo");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Case_1",
                "Switch: dynamic pin must be 'Case_1' after redo");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7 — Removing middle pin leaves others intact
// ---------------------------------------------------------------------------

static void Test_RemovePin_AtIndex_Correct()
{
    const std::string testName = "RemovePin_AtIndex_Correct";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    TaskNodeDefinition seqNode = MakeNode(1, TaskNodeType::VSSequence, "Seq");
    seqNode.DynamicExecOutputPins.push_back("Out_1");
    seqNode.DynamicExecOutputPins.push_back("Out_2");
    seqNode.DynamicExecOutputPins.push_back("Out_3");
    graph.Nodes.push_back(seqNode);
    graph.BuildLookupCache();

    UndoRedoStack stack;

    // Remove Out_2 (dynamic index 1), no link
    stack.PushCommand(
        std::unique_ptr<Olympe::ICommand>(
            new RemoveExecPinCommand(1, "Out_2", 1, -1, "")),
        graph);

    const TaskNodeDefinition* node = graph.GetNode(1);
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 2,
                "After remove middle: 2 pins remain");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Out_1",
                "After remove middle: first pin is still 'Out_1'");
    TEST_ASSERT(node->DynamicExecOutputPins[1] == "Out_3",
                "After remove middle: second pin is now 'Out_3'");

    // Undo: Out_2 re-inserted at index 1
    stack.Undo(graph);
    node = graph.GetNode(1);
    TEST_ASSERT(node->DynamicExecOutputPins.size() == 3,
                "After undo: 3 pins restored");
    TEST_ASSERT(node->DynamicExecOutputPins[0] == "Out_1",
                "After undo: pin[0] must be 'Out_1'");
    TEST_ASSERT(node->DynamicExecOutputPins[1] == "Out_2",
                "After undo: pin[1] must be 'Out_2' (restored at correct index)");
    TEST_ASSERT(node->DynamicExecOutputPins[2] == "Out_3",
                "After undo: pin[2] must be 'Out_3'");

    ReportTest(testName, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase 21-D Tests: Dynamic Pins Sequence/Switch ===" << std::endl;

    Test_AddPin_Sequence_UndoRedo();
    Test_RemovePin_NoLink_UndoRedo();
    Test_RemovePin_WithLink_UndoRedo();
    Test_AddThenRemove_DoubleUndo();
    Test_BasePin_NotInDynamicList();
    Test_AddPin_Switch_UndoRedo();
    Test_RemovePin_AtIndex_Correct();

    std::cout << "\n--- Results ---" << std::endl;
    std::cout << "Passed: " << s_passCount << std::endl;
    std::cout << "Failed: " << s_failCount << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
