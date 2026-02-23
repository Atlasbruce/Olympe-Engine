/**
 * @file Task_MoveToLocation_LocalBBTest.cpp
 * @brief Unit tests for Task_MoveToLocation in headless (LocalBlackboard) mode.
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Verifies that Task_MoveToLocation:
 *   a) Returns Running while the entity is not yet at the target.
 *   b) Returns Success and stops once the entity reaches the target.
 *   c) Returns Failure if the "Target" parameter is missing.
 *   d) Returns Failure if the "Position" BB key is absent.
 *
 * Tests drive ExecuteNode() directly to exercise the full LocalBlackboard
 * round-trip (Initialize → Execute → Serialize → Deserialize across ticks).
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/AtomicTaskRegistry.h"
#include "TaskSystem/IAtomicTask.h"
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
// Helper: build a template with "Position" BB variable and one MoveTo node
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakeMoveToTemplate(
    const ::Vector& defaultPos,
    const ::Vector& target,
    float            speed)
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "MoveToTest";
    tmpl.RootNodeID = 0;

    // Declare "Position" variable in the blackboard schema.
    Olympe::VariableDefinition posVar;
    posVar.Name         = "Position";
    posVar.Type         = Olympe::VariableType::Vector;
    posVar.DefaultValue = Olympe::TaskValue(defaultPos);
    posVar.IsLocal      = true;
    tmpl.LocalVariables.push_back(posVar);

    // Single AtomicTask node: Task_MoveToLocation
    Olympe::TaskNodeDefinition node;
    node.NodeID        = 0;
    node.NodeName      = "MoveTo";
    node.Type          = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID  = "Task_MoveToLocation";
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;

    // Bind "Target" as a literal Vector parameter.
    Olympe::ParameterBinding targetBinding;
    targetBinding.Type         = Olympe::ParameterBindingType::Literal;
    targetBinding.LiteralValue = Olympe::TaskValue(target);
    node.Parameters["Target"]  = targetBinding;

    // Bind "Speed" as a literal Float parameter.
    Olympe::ParameterBinding speedBinding;
    speedBinding.Type         = Olympe::ParameterBindingType::Literal;
    speedBinding.LiteralValue = Olympe::TaskValue(speed);
    node.Parameters["Speed"]  = speedBinding;

    tmpl.Nodes.push_back(node);
    tmpl.BuildLookupCache();
    return tmpl;
}

// ---------------------------------------------------------------------------
// Test A: Task_MoveToLocation returns Running then Success
// ---------------------------------------------------------------------------

static void TestA_MoveToSucceeds()
{
    std::cout << "Test A: Task_MoveToLocation moves from origin to target..." << std::endl;

    bool passed = true;

    const ::Vector start(0.0f, 0.0f, 0.0f);
    const ::Vector target(10.0f, 0.0f, 0.0f);
    const float    speed = 100.0f;
    const float    dt    = 0.016f;

    Olympe::TaskGraphTemplate   tmpl   = MakeMoveToTemplate(start, target, speed);
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    bool reachedSuccess = false;

    // Run up to 200 ticks - should converge much sooner.
    for (int tick = 0; tick < 200; ++tick)
    {
        system.ExecuteNode(1u, runner, &tmpl, dt);

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            reachedSuccess = (runner.LastStatus ==
                              Olympe::TaskRunnerComponent::TaskStatus::Success);
            break;
        }
    }

    TEST_ASSERT(reachedSuccess, "MoveTo should complete with Success");
    if (!reachedSuccess) passed = false;

    // activeTask must be null after completion.
    TEST_ASSERT(runner.activeTask == nullptr,
                "activeTask should be null after task completes");
    if (runner.activeTask) passed = false;

    ReportTest("TestA_MoveToSucceeds", passed);
}

// ---------------------------------------------------------------------------
// Test B: Task_MoveToLocation is Running for multiple ticks before Success
// ---------------------------------------------------------------------------

static void TestB_MoveToIsRunningThenSucceeds()
{
    std::cout << "Test B: Task_MoveToLocation returns Running before Success..." << std::endl;

    bool passed = true;

    // Use a slow speed so we observe Running ticks.
    const ::Vector start(0.0f, 0.0f, 0.0f);
    const ::Vector target(10.0f, 0.0f, 0.0f);
    const float    speed = 50.0f;   // 0.8 units per 16ms tick
    const float    dt    = 0.016f;

    Olympe::TaskGraphTemplate   tmpl   = MakeMoveToTemplate(start, target, speed);
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    int runningTicks = 0;

    for (int tick = 0; tick < 200; ++tick)
    {
        system.ExecuteNode(1u, runner, &tmpl, dt);

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            break;
        }
        ++runningTicks;
    }

    // With speed=50 and dt=0.016, step=0.8/tick. 10/0.8 = 12.5 ticks expected.
    TEST_ASSERT(runningTicks >= 5, "MoveTo should require multiple Running ticks");
    if (runningTicks < 5) passed = false;

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success,
                "MoveTo should ultimately succeed");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Success) passed = false;

    ReportTest("TestB_MoveToIsRunningThenSucceeds", passed);
}

// ---------------------------------------------------------------------------
// Test C: Missing Target parameter → Failure
// ---------------------------------------------------------------------------

static void TestC_MissingTargetReturnsFailure()
{
    std::cout << "Test C: Missing 'Target' parameter causes Failure..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "MoveToNoTarget";
    tmpl.RootNodeID = 0;

    // Add "Position" BB variable.
    Olympe::VariableDefinition posVar;
    posVar.Name         = "Position";
    posVar.Type         = Olympe::VariableType::Vector;
    posVar.DefaultValue = Olympe::TaskValue(::Vector(0.0f, 0.0f, 0.0f));
    posVar.IsLocal      = true;
    tmpl.LocalVariables.push_back(posVar);

    Olympe::TaskNodeDefinition node;
    node.NodeID       = 0;
    node.NodeName     = "MoveNoTarget";
    node.Type         = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID = "Task_MoveToLocation";
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;
    // No "Target" parameter bound.

    tmpl.Nodes.push_back(node);
    tmpl.BuildLookupCache();

    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    system.ExecuteNode(1u, runner, &tmpl, 0.016f);

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Failure,
                "Missing Target should cause Failure");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Failure) passed = false;

    ReportTest("TestC_MissingTargetReturnsFailure", passed);
}

// ---------------------------------------------------------------------------
// Test D: Missing Position BB key → Failure
// ---------------------------------------------------------------------------

static void TestD_MissingPositionBBKeyReturnsFailure()
{
    std::cout << "Test D: Missing 'Position' BB key causes Failure..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "MoveToNoPosBB";
    tmpl.RootNodeID = 0;
    // No LocalVariables declared.

    Olympe::TaskNodeDefinition node;
    node.NodeID       = 0;
    node.NodeName     = "MoveNoPos";
    node.Type         = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID = "Task_MoveToLocation";
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;

    Olympe::ParameterBinding targetBinding;
    targetBinding.Type         = Olympe::ParameterBindingType::Literal;
    targetBinding.LiteralValue = Olympe::TaskValue(::Vector(5.0f, 0.0f, 0.0f));
    node.Parameters["Target"]  = targetBinding;

    tmpl.Nodes.push_back(node);
    tmpl.BuildLookupCache();

    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    system.ExecuteNode(1u, runner, &tmpl, 0.016f);

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Failure,
                "Missing Position BB key should cause Failure");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Failure) passed = false;

    ReportTest("TestD_MissingPositionBBKeyReturnsFailure", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Task_MoveToLocation_LocalBBTest ===" << std::endl;

    TestA_MoveToSucceeds();
    TestB_MoveToIsRunningThenSucceeds();
    TestC_MissingTargetReturnsFailure();
    TestD_MissingPositionBBKeyReturnsFailure();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
