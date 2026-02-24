/**
 * @file TaskSystem_E2E_ExampleGraph.cpp
 * @brief End-to-end integration test: MoveTo -> Wait -> SetVariable graph.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Validates that the TaskSystem correctly executes a three-node linear graph:
 *
 *   Node 0  Task_MoveToLocation  (Running x N ticks, then Success)
 *     |
 *   Node 1  Task_Wait            (Running x M ticks, then Success)
 *     |
 *   Node 2  Task_SetVariable     (Success immediately, sets "Done"=true)
 *     |
 *   [graph complete: CurrentNodeIndex == NODE_INDEX_NONE]
 *
 * Assertions:
 *   - Graph completes within the allowed tick budget.
 *   - Final runner.LastStatus == Success.
 *   - Each node is visited in order (tracked via SYSTEM_LOG side-effects and
 *     CurrentNodeIndex transitions).
 *
 * Runs in headless mode (ctx.WorldPtr == nullptr).
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/AtomicTaskRegistry.h"
#include "TaskSystem/IAtomicTask.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "TaskSystem/LocalBlackboard.h"
#include "ECS/Components/TaskRunnerComponent.h"

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                      \
    do {                                                            \
        if (!(cond)) {                                              \
            std::cout << "  FAIL: " << (msg) << std::endl;         \
            ++s_failCount;                                          \
        }                                                           \
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
// Build the E2E template:
//   BB vars: "Position" (Vector), "Done" (Bool)
//   Node 0: Task_MoveToLocation  target=(5,0,0) speed=200   -> node 1 on success
//   Node 1: Task_Wait            duration=0.1s              -> node 2 on success
//   Node 2: Task_SetVariable     VarName="Done" Value=true  -> done
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakeE2ETemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "E2E_ExampleGraph";
    tmpl.RootNodeID = 0;

    // --- Blackboard variables ---
    {
        Olympe::VariableDefinition v;
        v.Name         = "Position";
        v.Type         = Olympe::VariableType::Vector;
        v.DefaultValue = Olympe::TaskValue(::Vector(0.0f, 0.0f, 0.0f));
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }
    {
        Olympe::VariableDefinition v;
        v.Name         = "Done";
        v.Type         = Olympe::VariableType::Bool;
        v.DefaultValue = Olympe::TaskValue(false);
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }

    // --- Node 0: Task_MoveToLocation ---
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID       = 0;
        node.NodeName     = "MoveTo";
        node.Type         = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID = "Task_MoveToLocation";
        node.NextOnSuccess = 1;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;

        Olympe::ParameterBinding targetB;
        targetB.Type         = Olympe::ParameterBindingType::Literal;
        targetB.LiteralValue = Olympe::TaskValue(::Vector(5.0f, 0.0f, 0.0f));
        node.Parameters["Target"] = targetB;

        Olympe::ParameterBinding speedB;
        speedB.Type         = Olympe::ParameterBindingType::Literal;
        speedB.LiteralValue = Olympe::TaskValue(200.0f);
        node.Parameters["Speed"] = speedB;

        tmpl.Nodes.push_back(node);
    }

    // --- Node 1: Task_Wait ---
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID       = 1;
        node.NodeName     = "Wait";
        node.Type         = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID = "Task_Wait";
        node.NextOnSuccess = 2;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;

        Olympe::ParameterBinding durB;
        durB.Type         = Olympe::ParameterBindingType::Literal;
        durB.LiteralValue = Olympe::TaskValue(0.05f); // ~3 ticks at 16ms
        node.Parameters["Duration"] = durB;

        tmpl.Nodes.push_back(node);
    }

    // --- Node 2: Task_SetVariable ---
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID       = 2;
        node.NodeName     = "SetDone";
        node.Type         = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID = "Task_SetVariable";
        node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;

        Olympe::ParameterBinding nameB;
        nameB.Type         = Olympe::ParameterBindingType::Literal;
        nameB.LiteralValue = Olympe::TaskValue(std::string("Done"));
        node.Parameters["VarName"] = nameB;

        Olympe::ParameterBinding valueB;
        valueB.Type         = Olympe::ParameterBindingType::Literal;
        valueB.LiteralValue = Olympe::TaskValue(true);
        node.Parameters["Value"] = valueB;

        tmpl.Nodes.push_back(node);
    }

    tmpl.BuildLookupCache();
    return tmpl;
}

// ---------------------------------------------------------------------------
// E2E Test: full MoveTo -> Wait -> SetVariable graph completes successfully
// ---------------------------------------------------------------------------

static void TestE2E_GraphCompletesSuccessfully()
{
    std::cout << "E2E: MoveTo -> Wait -> SetVariable graph completion..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate   tmpl   = MakeE2ETemplate();
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    const float dt           = 0.016f;
    const int   maxTicks     = 300;
    int         completedAt  = -1;

    // Track node transitions for diagnostic output.
    int prevNode = runner.CurrentNodeIndex;

    for (int tick = 0; tick < maxTicks; ++tick)
    {
        system.ExecuteNode(1u, runner, &tmpl, dt);

        if (runner.CurrentNodeIndex != prevNode)
        {
            std::cout << "  tick " << tick
                      << ": node " << prevNode
                      << " -> " << runner.CurrentNodeIndex << std::endl;
            prevNode = runner.CurrentNodeIndex;
        }

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            completedAt = tick;
            break;
        }
    }

    TEST_ASSERT(completedAt >= 0,
                "Graph should complete within tick budget");
    if (completedAt < 0) passed = false;

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success,
                "Final status should be Success");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Success) passed = false;

    TEST_ASSERT(runner.activeTask == nullptr,
                "activeTask should be null after graph completes");
    if (runner.activeTask) passed = false;

    if (completedAt >= 0)
    {
        std::cout << "  Graph completed at tick " << completedAt << std::endl;
    }

    ReportTest("TestE2E_GraphCompletesSuccessfully", passed);
}

// ---------------------------------------------------------------------------
// E2E Test: nodes visited in order (0, 1, 2, NONE)
// ---------------------------------------------------------------------------

static void TestE2E_NodesVisitedInOrder()
{
    std::cout << "E2E: Nodes visited in order 0->1->2->NONE..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate   tmpl   = MakeE2ETemplate();
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    const float dt       = 0.016f;
    const int   maxTicks = 300;

    std::vector<int> transitions;
    transitions.push_back(runner.CurrentNodeIndex); // initial = 0

    int prev = runner.CurrentNodeIndex;

    for (int tick = 0; tick < maxTicks; ++tick)
    {
        system.ExecuteNode(1u, runner, &tmpl, dt);

        if (runner.CurrentNodeIndex != prev)
        {
            transitions.push_back(runner.CurrentNodeIndex);
            prev = runner.CurrentNodeIndex;
        }

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            break;
        }
    }

    // Expected: [0, 1, 2, NODE_INDEX_NONE]
    TEST_ASSERT(transitions.size() == 4u,
                "Should observe exactly 4 node states: 0, 1, 2, NONE");
    if (transitions.size() == 4u)
    {
        TEST_ASSERT(transitions[0] == 0,  "First node should be 0 (MoveTo)");
        TEST_ASSERT(transitions[1] == 1,  "Second node should be 1 (Wait)");
        TEST_ASSERT(transitions[2] == 2,  "Third node should be 2 (SetVariable)");
        TEST_ASSERT(transitions[3] == Olympe::NODE_INDEX_NONE,
                    "Fourth state should be NODE_INDEX_NONE");
    }
    else
    {
        passed = false;
    }

    ReportTest("TestE2E_NodesVisitedInOrder", passed);
}

// ---------------------------------------------------------------------------
// E2E Test: short IDs ("MoveToLocation", "Wait", "SetVariable") are accepted
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakeE2ETemplateShortIds()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "E2E_ShortIds";
    tmpl.RootNodeID = 0;

    // BB vars
    {
        Olympe::VariableDefinition v;
        v.Name         = "Position";
        v.Type         = Olympe::VariableType::Vector;
        v.DefaultValue = Olympe::TaskValue(::Vector(0.0f, 0.0f, 0.0f));
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }
    {
        Olympe::VariableDefinition v;
        v.Name         = "Done";
        v.Type         = Olympe::VariableType::Bool;
        v.DefaultValue = Olympe::TaskValue(false);
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }

    // Node 0: short ID "MoveToLocation"
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID        = 0;
        node.NodeName      = "MoveTo";
        node.Type          = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID  = "MoveToLocation"; // short ID
        node.NextOnSuccess = 1;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;

        Olympe::ParameterBinding targetB;
        targetB.Type         = Olympe::ParameterBindingType::Literal;
        targetB.LiteralValue = Olympe::TaskValue(::Vector(5.0f, 0.0f, 0.0f));
        node.Parameters["Target"] = targetB;

        Olympe::ParameterBinding speedB;
        speedB.Type         = Olympe::ParameterBindingType::Literal;
        speedB.LiteralValue = Olympe::TaskValue(200.0f);
        node.Parameters["Speed"] = speedB;

        tmpl.Nodes.push_back(node);
    }

    // Node 1: short ID "Wait"
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID        = 1;
        node.NodeName      = "Wait";
        node.Type          = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID  = "Wait"; // short ID
        node.NextOnSuccess = 2;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;

        Olympe::ParameterBinding durB;
        durB.Type         = Olympe::ParameterBindingType::Literal;
        durB.LiteralValue = Olympe::TaskValue(0.05f);
        node.Parameters["Duration"] = durB;

        tmpl.Nodes.push_back(node);
    }

    // Node 2: short ID "SetVariable"
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID        = 2;
        node.NodeName      = "SetDone";
        node.Type          = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID  = "SetVariable"; // short ID
        node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;

        Olympe::ParameterBinding nameB;
        nameB.Type         = Olympe::ParameterBindingType::Literal;
        nameB.LiteralValue = Olympe::TaskValue(std::string("Done"));
        node.Parameters["VarName"] = nameB;

        Olympe::ParameterBinding valueB;
        valueB.Type         = Olympe::ParameterBindingType::Literal;
        valueB.LiteralValue = Olympe::TaskValue(true);
        node.Parameters["Value"] = valueB;

        tmpl.Nodes.push_back(node);
    }

    tmpl.BuildLookupCache();
    return tmpl;
}

static void TestE2E_ShortIdsAccepted()
{
    std::cout << "E2E: Short IDs (MoveToLocation/Wait/SetVariable) accepted..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate   tmpl   = MakeE2ETemplateShortIds();
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    const float dt       = 0.016f;
    const int   maxTicks = 300;
    int         completedAt = -1;

    for (int tick = 0; tick < maxTicks; ++tick)
    {
        system.ExecuteNode(1u, runner, &tmpl, dt);

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            completedAt = tick;
            break;
        }
    }

    TEST_ASSERT(completedAt >= 0,
                "Graph using short IDs should complete within tick budget");
    if (completedAt < 0) passed = false;

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success,
                "Final status should be Success with short IDs");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Success) passed = false;

    if (completedAt >= 0)
    {
        std::cout << "  Graph with short IDs completed at tick " << completedAt
                  << std::endl;
    }

    ReportTest("TestE2E_ShortIdsAccepted", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== TaskSystem_E2E_ExampleGraph ===" << std::endl;

    TestE2E_GraphCompletesSuccessfully();
    TestE2E_NodesVisitedInOrder();
    TestE2E_ShortIdsAccepted();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
