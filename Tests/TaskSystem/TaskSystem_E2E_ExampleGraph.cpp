/**
 * @file TaskSystem_E2E_ExampleGraph.cpp
 * @brief End-to-end test: MoveTo -> Wait -> SetVariable graph via LocalBlackboard.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Builds a three-node task graph template:
 *   Node 0 (Task_MoveToLocation) -> Node 1 (Task_Wait) -> Node 2 (Task_SetVariable)
 *
 * Registers the template, binds a TaskRunnerComponent to an entity, then drives
 * TaskSystem::ExecuteNode() in a loop until the graph finishes (CurrentNodeIndex
 * becomes NODE_INDEX_NONE).
 *
 * Verifies:
 *   a) The graph completes within MAX_TICKS.
 *   b) runner.LastStatus is Success when the graph finishes.
 *   c) Each atomic task is registered in the AtomicTaskRegistry.
 *
 * No SDL3 or World dependency - purely LocalBlackboard-based.
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/AtomicTaskRegistry.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "ECS/Components/TaskRunnerComponent.h"

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(condition, message)                             \
    do {                                                            \
        if (!(condition)) {                                         \
            std::cout << "  FAIL: " << (message) << std::endl;     \
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
// Build a 3-node E2E template
//
//   LocalVariables:
//     "Position" (Vector,  default (0,0,0))
//     "Velocity" (Vector,  default (0,0,0))
//     "Result"   (Bool,    default false)
//
//   Node 0: Task_MoveToLocation
//     Target=(5,0,0), Speed=100, AcceptanceRadius=2
//     NextOnSuccess = 1
//   Node 1: Task_Wait
//     Duration = 0.05
//     NextOnSuccess = 2
//   Node 2: Task_SetVariable
//     VariableName = "Result", Value = true
//     NextOnSuccess = NODE_INDEX_NONE
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate BuildE2ETemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "E2E_MoveWaitSet";
    tmpl.RootNodeID = 0;

    // ----- LocalVariables -----
    {
        Olympe::VariableDefinition v;
        v.Name         = "Position";
        v.Type         = Olympe::VariableType::Vector;
        v.DefaultValue = Olympe::TaskValue(Vector(0.0f, 0.0f, 0.0f));
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }
    {
        Olympe::VariableDefinition v;
        v.Name         = "Velocity";
        v.Type         = Olympe::VariableType::Vector;
        v.DefaultValue = Olympe::TaskValue(Vector(0.0f, 0.0f, 0.0f));
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }
    {
        Olympe::VariableDefinition v;
        v.Name         = "Result";
        v.Type         = Olympe::VariableType::Bool;
        v.DefaultValue = Olympe::TaskValue(false);
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }

    // ----- Node 0: Task_MoveToLocation -----
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID       = 0;
        node.NodeName     = "MoveTo";
        node.Type         = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID = "Task_MoveToLocation";

        {
            Olympe::ParameterBinding b;
            b.Type         = Olympe::ParameterBindingType::Literal;
            b.LiteralValue = Olympe::TaskValue(Vector(5.0f, 0.0f, 0.0f));
            node.Parameters["Target"] = b;
        }
        {
            Olympe::ParameterBinding b;
            b.Type         = Olympe::ParameterBindingType::Literal;
            b.LiteralValue = Olympe::TaskValue(100.0f);
            node.Parameters["Speed"] = b;
        }
        {
            Olympe::ParameterBinding b;
            b.Type         = Olympe::ParameterBindingType::Literal;
            b.LiteralValue = Olympe::TaskValue(2.0f);
            node.Parameters["AcceptanceRadius"] = b;
        }

        node.NextOnSuccess = 1;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;
        tmpl.Nodes.push_back(node);
    }

    // ----- Node 1: Task_Wait -----
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID       = 1;
        node.NodeName     = "Wait";
        node.Type         = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID = "Task_Wait";

        {
            Olympe::ParameterBinding b;
            b.Type         = Olympe::ParameterBindingType::Literal;
            b.LiteralValue = Olympe::TaskValue(0.05f);
            node.Parameters["Duration"] = b;
        }

        node.NextOnSuccess = 2;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;
        tmpl.Nodes.push_back(node);
    }

    // ----- Node 2: Task_SetVariable -----
    {
        Olympe::TaskNodeDefinition node;
        node.NodeID       = 2;
        node.NodeName     = "SetResult";
        node.Type         = Olympe::TaskNodeType::AtomicTask;
        node.AtomicTaskID = "Task_SetVariable";

        {
            Olympe::ParameterBinding b;
            b.Type         = Olympe::ParameterBindingType::Literal;
            b.LiteralValue = Olympe::TaskValue(std::string("Result"));
            node.Parameters["VariableName"] = b;
        }
        {
            Olympe::ParameterBinding b;
            b.Type         = Olympe::ParameterBindingType::Literal;
            b.LiteralValue = Olympe::TaskValue(true);
            node.Parameters["Value"] = b;
        }

        node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
        node.NextOnFailure = Olympe::NODE_INDEX_NONE;
        tmpl.Nodes.push_back(node);
    }

    tmpl.BuildLookupCache();
    return tmpl;
}

// ---------------------------------------------------------------------------
// Test A: All task types are registered
// ---------------------------------------------------------------------------

static void TestA_TasksAreRegistered()
{
    std::cout << "Test A: All task types are registered in AtomicTaskRegistry..." << std::endl;

    bool passed = true;

    TEST_ASSERT(Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_MoveToLocation"),
                "Task_MoveToLocation should be registered");
    if (!Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_MoveToLocation"))
        passed = false;

    TEST_ASSERT(Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_Wait"),
                "Task_Wait should be registered");
    if (!Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_Wait"))
        passed = false;

    TEST_ASSERT(Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_SetVariable"),
                "Task_SetVariable should be registered");
    if (!Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_SetVariable"))
        passed = false;

    ReportTest("TestA_TasksAreRegistered", passed);
}

// ---------------------------------------------------------------------------
// Test B: E2E graph completes with Success within MAX_TICKS
// ---------------------------------------------------------------------------

static void TestB_E2EGraphCompletes()
{
    std::cout << "Test B: E2E graph (MoveTo->Wait->SetVariable) completes..." << std::endl;

    Olympe::TaskGraphTemplate     tmpl   = BuildE2ETemplate();
    Olympe::TaskSystem            system;
    Olympe::TaskRunnerComponent   runner;

    bool passed   = true;
    bool finished = false;

    // Budget: MoveTo (~3 ticks) + Wait (~5 ticks) + SetVariable (1 tick) = ~9.
    // Use 60 ticks as a generous upper bound.
    const int   MAX_TICKS = 60;
    const float dt        = 0.016f;

    int32_t prevNodeIndex = runner.CurrentNodeIndex;

    for (int tick = 0; tick < MAX_TICKS; ++tick)
    {
        system.ExecuteNode(10u, runner, &tmpl, dt);

        // Log node transitions.
        if (runner.CurrentNodeIndex != prevNodeIndex)
        {
            std::cout << "  Tick " << tick + 1
                      << ": node " << prevNodeIndex
                      << " -> " << runner.CurrentNodeIndex << "\n";
            prevNodeIndex = runner.CurrentNodeIndex;
        }

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            finished = true;
            std::cout << "  Graph finished after " << (tick + 1) << " ticks\n";
            break;
        }
    }

    TEST_ASSERT(finished, "E2E graph should complete within MAX_TICKS");
    if (!finished) { passed = false; }

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success,
                "LastStatus should be Success after graph completes");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Success)
        passed = false;

    ReportTest("TestB_E2EGraphCompletes", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== TaskSystem_E2E_ExampleGraph ===" << std::endl;

    TestA_TasksAreRegistered();
    TestB_E2EGraphCompletes();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
