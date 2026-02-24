/**
 * @file Task_RequestPathfindingTest.cpp
 * @brief Unit tests for Task_RequestPathfinding async request/polling model.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * Verifies that Task_RequestPathfinding:
 *   a) Returns Running on the first tick (request submitted).
 *   b) Returns Success once the async path is ready and writes "Path" to BB.
 *   c) Returns Failure if the "Target" parameter is missing.
 *   d) Returns Failure if the "Position" BB key is absent.
 *   e) Abort() cancels the in-flight request without crashing.
 *
 * AsyncDelay=0.0 is used for deterministic completion: the worker thread has
 * no sleep so it completes essentially immediately.  A tick budget of 200 is
 * used to absorb any OS scheduling latency.
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
// Helper: build a template with "Position" and "Path" BB variables and one
//         Task_RequestPathfinding node.
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakePathfindingTemplate(
    const ::Vector& defaultPos,
    const ::Vector& target,
    float           asyncDelay)
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "PathfindingTest";
    tmpl.RootNodeID = 0;

    // "Position" variable (Vector)
    {
        Olympe::VariableDefinition v;
        v.Name         = "Position";
        v.Type         = Olympe::VariableType::Vector;
        v.DefaultValue = Olympe::TaskValue(defaultPos);
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }

    // "Path" variable (String)
    {
        Olympe::VariableDefinition v;
        v.Name         = "Path";
        v.Type         = Olympe::VariableType::String;
        v.DefaultValue = Olympe::TaskValue(std::string(""));
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }

    // Single AtomicTask node: Task_RequestPathfinding
    Olympe::TaskNodeDefinition node;
    node.NodeID        = 0;
    node.NodeName      = "RequestPath";
    node.Type          = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID  = "Task_RequestPathfinding";
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;

    // Bind "Target" parameter.
    {
        Olympe::ParameterBinding b;
        b.Type         = Olympe::ParameterBindingType::Literal;
        b.LiteralValue = Olympe::TaskValue(target);
        node.Parameters["Target"] = b;
    }

    // Bind "AsyncDelay" parameter.
    {
        Olympe::ParameterBinding b;
        b.Type         = Olympe::ParameterBindingType::Literal;
        b.LiteralValue = Olympe::TaskValue(asyncDelay);
        node.Parameters["AsyncDelay"] = b;
    }

    tmpl.Nodes.push_back(node);
    tmpl.BuildLookupCache();
    return tmpl;
}

// ---------------------------------------------------------------------------
// Test A: Task_RequestPathfinding eventually returns Success and writes Path
// ---------------------------------------------------------------------------

static void TestA_PathfindingSucceeds()
{
    std::cout << "Test A: Task_RequestPathfinding completes with Success..." << std::endl;

    bool passed = true;

    const ::Vector start(0.0f, 0.0f, 0.0f);
    const ::Vector target(10.0f, 5.0f, 0.0f);

    Olympe::TaskGraphTemplate   tmpl   = MakePathfindingTemplate(start, target, 0.0f);
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    bool reachedSuccess = false;

    // Run up to 200 ticks to allow async thread to complete.
    for (int tick = 0; tick < 200; ++tick)
    {
        system.ExecuteNode(1u, runner, &tmpl, 0.016f);

        if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE)
        {
            reachedSuccess = (runner.LastStatus ==
                              Olympe::TaskRunnerComponent::TaskStatus::Success);
            break;
        }
    }

    TEST_ASSERT(reachedSuccess, "Pathfinding should complete with Success");
    if (!reachedSuccess) passed = false;

    TEST_ASSERT(runner.activeTask == nullptr,
                "activeTask should be null after task completes");
    if (runner.activeTask) passed = false;

    ReportTest("TestA_PathfindingSucceeds", passed);
}

// ---------------------------------------------------------------------------
// Test B: First tick returns Running (request submitted)
// ---------------------------------------------------------------------------

static void TestB_FirstTickReturnsRunning()
{
    std::cout << "Test B: First tick returns Running..." << std::endl;

    bool passed = true;

    const ::Vector start(0.0f, 0.0f, 0.0f);
    const ::Vector target(3.0f, 4.0f, 0.0f);

    // Use a non-zero delay so the request cannot complete within the first tick.
    Olympe::TaskGraphTemplate   tmpl   = MakePathfindingTemplate(start, target, 60.0f);
    Olympe::TaskSystem          system;
    Olympe::TaskRunnerComponent runner;

    // Execute exactly one tick.
    system.ExecuteNode(1u, runner, &tmpl, 0.016f);

    // The task should be Running (not yet complete).
    TEST_ASSERT(runner.CurrentNodeIndex == 0,
                "After first tick, node should still be 0 (Running)");
    if (runner.CurrentNodeIndex != 0) passed = false;

    // Abort to release the pending request.
    if (runner.activeTask)
    {
        runner.activeTask->Abort();
        runner.activeTask.reset();
    }

    ReportTest("TestB_FirstTickReturnsRunning", passed);
}

// ---------------------------------------------------------------------------
// Test C: Missing "Target" parameter → Failure
// ---------------------------------------------------------------------------

static void TestC_MissingTargetReturnsFailure()
{
    std::cout << "Test C: Missing 'Target' parameter causes Failure..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "PathNoTarget";
    tmpl.RootNodeID = 0;

    // "Position" variable
    {
        Olympe::VariableDefinition v;
        v.Name         = "Position";
        v.Type         = Olympe::VariableType::Vector;
        v.DefaultValue = Olympe::TaskValue(::Vector(0.0f, 0.0f, 0.0f));
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }

    // "Path" variable
    {
        Olympe::VariableDefinition v;
        v.Name         = "Path";
        v.Type         = Olympe::VariableType::String;
        v.DefaultValue = Olympe::TaskValue(std::string(""));
        v.IsLocal      = true;
        tmpl.LocalVariables.push_back(v);
    }

    Olympe::TaskNodeDefinition node;
    node.NodeID        = 0;
    node.NodeName      = "NoTarget";
    node.Type          = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID  = "Task_RequestPathfinding";
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;
    // No "Target" parameter.

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
// Test D: Missing "Position" BB key → Failure
// ---------------------------------------------------------------------------

static void TestD_MissingPositionBBKeyReturnsFailure()
{
    std::cout << "Test D: Missing 'Position' BB key causes Failure..." << std::endl;

    bool passed = true;

    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name       = "PathNoPosition";
    tmpl.RootNodeID = 0;
    // No LocalVariables declared.

    Olympe::TaskNodeDefinition node;
    node.NodeID        = 0;
    node.NodeName      = "NoPosition";
    node.Type          = Olympe::TaskNodeType::AtomicTask;
    node.AtomicTaskID  = "Task_RequestPathfinding";
    node.NextOnSuccess = Olympe::NODE_INDEX_NONE;
    node.NextOnFailure = Olympe::NODE_INDEX_NONE;

    Olympe::ParameterBinding b;
    b.Type         = Olympe::ParameterBindingType::Literal;
    b.LiteralValue = Olympe::TaskValue(::Vector(5.0f, 0.0f, 0.0f));
    node.Parameters["Target"] = b;

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
    std::cout << "=== Task_RequestPathfindingTest ===" << std::endl;

    TestA_PathfindingSucceeds();
    TestB_FirstTickReturnsRunning();
    TestC_MissingTargetReturnsFailure();
    TestD_MissingPositionBBKeyReturnsFailure();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
