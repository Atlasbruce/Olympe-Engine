/**
 * @file Task_MoveToLocation_LocalBBTest.cpp
 * @brief Unit/E2E test for Task_MoveToLocation in LocalBlackboard (headless) mode.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Registers a TaskGraphTemplate with a single Task_MoveToLocation node,
 * binds a TaskRunnerComponent, and calls TaskSystem::ExecuteNode() repeatedly
 * until the runner's CurrentNodeIndex becomes NODE_INDEX_NONE (graph complete).
 *
 * Verifies:
 *   a) The runner starts in Running state (returns Running before arrival).
 *   b) After N ticks the task completes with Success.
 *   c) runner.CurrentNodeIndex transitions to NODE_INDEX_NONE on completion.
 *   d) runner.LastStatus is Success.
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
// Build a template for MoveTo test
//
//   LocalVariables:
//     "Position" (Vector, default (0,0,0))
//     "Velocity" (Vector, default (0,0,0))
//
//   Node 0: Task_MoveToLocation
//     Target          = (10, 0, 0)
//     Speed           = 100.0
//     AcceptanceRadius = 2.0
//     NextOnSuccess   = NODE_INDEX_NONE
//     NextOnFailure   = NODE_INDEX_NONE
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate BuildMoveToTemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "MoveToLocalBBTest";
    tmpl.RootNodeID = 0;

    // Declare blackboard variables required by Task_MoveToLocation.
    {
        Olympe::VariableDefinition posVar;
        posVar.Name         = "Position";
        posVar.Type         = Olympe::VariableType::Vector;
        posVar.DefaultValue = Olympe::TaskValue(Vector(0.0f, 0.0f, 0.0f));
        posVar.IsLocal      = true;
        tmpl.LocalVariables.push_back(posVar);
    }
    {
        Olympe::VariableDefinition velVar;
        velVar.Name         = "Velocity";
        velVar.Type         = Olympe::VariableType::Vector;
        velVar.DefaultValue = Olympe::TaskValue(Vector(0.0f, 0.0f, 0.0f));
        velVar.IsLocal      = true;
        tmpl.LocalVariables.push_back(velVar);
    }

    // Build the single MoveTo node.
    Olympe::TaskNodeDefinition node;
    node.NodeID       = 0;
    node.NodeName     = "MoveTo";
    node.Type         = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID = "Task_MoveToLocation";

    // Target = (10, 0, 0) as a Literal binding.
    {
        Olympe::ParameterBinding b;
        b.Type         = Olympe::ParameterBindingType::Literal;
        b.LiteralValue = Olympe::TaskValue(Vector(10.0f, 0.0f, 0.0f));
        node.Parameters["Target"] = b;
    }
    // Speed = 100.0
    {
        Olympe::ParameterBinding b;
        b.Type         = Olympe::ParameterBindingType::Literal;
        b.LiteralValue = Olympe::TaskValue(100.0f);
        node.Parameters["Speed"] = b;
    }
    // AcceptanceRadius = 2.0
    {
        Olympe::ParameterBinding b;
        b.Type         = Olympe::ParameterBindingType::Literal;
        b.LiteralValue = Olympe::TaskValue(2.0f);
        node.Parameters["AcceptanceRadius"] = b;
    }

    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;

    tmpl.Nodes.push_back(node);
    tmpl.BuildLookupCache();

    return tmpl;
}

// ---------------------------------------------------------------------------
// Test A: Task_MoveToLocation returns Running before arrival
// ---------------------------------------------------------------------------

static void TestA_MoveToRunningBeforeArrival()
{
    std::cout << "Test A: Task_MoveToLocation returns Running before arrival..." << std::endl;

    Olympe::TaskGraphTemplate     tmpl   = BuildMoveToTemplate();
    Olympe::TaskSystem            system;
    Olympe::TaskRunnerComponent   runner;

    bool passed = true;

    // First tick: position=(0,0), target=(10,0), should be Running.
    system.ExecuteNode(1u, runner, &tmpl, 0.016f);

    TEST_ASSERT(runner.activeTask != nullptr,
                "activeTask should be non-null while Running");
    if (!runner.activeTask) { passed = false; }

    TEST_ASSERT(runner.CurrentNodeIndex == 0,
                "CurrentNodeIndex should stay at 0 while Running");
    if (runner.CurrentNodeIndex != 0) { passed = false; }

    ReportTest("TestA_MoveToRunningBeforeArrival", passed);
}

// ---------------------------------------------------------------------------
// Test B: Task_MoveToLocation completes with Success after enough ticks
// ---------------------------------------------------------------------------

static void TestB_MoveToSucceedsOnArrival()
{
    std::cout << "Test B: Task_MoveToLocation completes with Success on arrival..." << std::endl;

    Olympe::TaskGraphTemplate     tmpl   = BuildMoveToTemplate();
    Olympe::TaskSystem            system;
    Olympe::TaskRunnerComponent   runner;

    bool passed  = true;
    bool arrived = false;

    // With Speed=100, dt=0.016, AcceptanceRadius=2, distance=10:
    // Each tick moves 1.6 units; within 7 ticks the entity should arrive.
    // Run for up to 50 ticks to be safe.
    const int   MAX_TICKS = 50;
    const float dt        = 0.016f;

    for (int tick = 0; tick < MAX_TICKS; ++tick)
    {
        system.ExecuteNode(2u, runner, &tmpl, dt);

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            arrived = true;
            std::cout << "  Arrived after " << (tick + 1) << " ticks\n";
            break;
        }
    }

    TEST_ASSERT(arrived, "Task_MoveToLocation should complete within MAX_TICKS");
    if (!arrived) { passed = false; }

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success,
                "LastStatus should be Success on arrival");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Success)
        passed = false;

    TEST_ASSERT(runner.activeTask == nullptr,
                "activeTask should be null after task completes");
    if (runner.activeTask) passed = false;

    ReportTest("TestB_MoveToSucceedsOnArrival", passed);
}

// ---------------------------------------------------------------------------
// Test C: Task_MoveToLocation is registered in the AtomicTaskRegistry
// ---------------------------------------------------------------------------

static void TestC_MoveToIsRegistered()
{
    std::cout << "Test C: Task_MoveToLocation is registered in AtomicTaskRegistry..." << std::endl;

    bool passed = true;

    TEST_ASSERT(Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_MoveToLocation"),
                "Task_MoveToLocation should be auto-registered");
    if (!Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_MoveToLocation"))
        passed = false;

    ReportTest("TestC_MoveToIsRegistered", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Task_MoveToLocation_LocalBBTest ===" << std::endl;

    TestC_MoveToIsRegistered();
    TestA_MoveToRunningBeforeArrival();
    TestB_MoveToSucceedsOnArrival();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
