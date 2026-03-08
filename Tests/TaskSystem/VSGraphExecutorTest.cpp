/**
 * @file VSGraphExecutorTest.cpp
 * @brief Unit tests for VSGraphExecutor (Phase 3 ATS VS).
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Tests cover:
 *   1.  EntryPoint_AdvancesToFirstChild
 *   2.  Branch_TruePathTaken
 *   3.  Branch_FalsePathTaken
 *   4.  Delay_ReturnsRunning
 *   5.  Delay_CompletesAfterDuration
 *   6.  DoOnce_BlocksSecondExecution
 *   7.  BBRoundTrip_LocalScope
 *   8.  MaxStepsGuard_PreventInfiniteLoop
 *
 * All graphs are built in-memory (no file I/O) using helper functions.
 * All tests run headless (worldPtr == nullptr).
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/VSGraphExecutor.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "TaskSystem/LocalBlackboard.h"
#include "TaskSystem/AtomicTaskRegistry.h"
#include "ECS/Components/TaskRunnerComponent.h"

using Olympe::VSGraphExecutor;

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
// Graph builder helpers
// ---------------------------------------------------------------------------

/// Adds an ExecConnection to the template.
static void AddExecConn(Olympe::TaskGraphTemplate& tmpl,
                         int srcID, const std::string& srcPin,
                         int dstID, const std::string& dstPin)
{
    Olympe::ExecPinConnection c;
    c.SourceNodeID  = srcID;
    c.SourcePinName = srcPin;
    c.TargetNodeID  = dstID;
    c.TargetPinName = dstPin;
    tmpl.ExecConnections.push_back(c);
}

/// Adds a node with the given type and ID to the template.
static Olympe::TaskNodeDefinition& AddNode(Olympe::TaskGraphTemplate& tmpl,
                                            int id,
                                            Olympe::TaskNodeType type,
                                            const std::string& name = "")
{
    Olympe::TaskNodeDefinition node;
    node.NodeID   = id;
    node.Type     = type;
    node.NodeName = name.empty() ? std::string("Node_") + std::to_string(id) : name;
    tmpl.Nodes.push_back(node);
    return tmpl.Nodes.back();
}

/// Registers a local Bool variable in the template and blackboard.
static void RegisterBool(Olympe::TaskGraphTemplate& tmpl,
                          Olympe::LocalBlackboard& lb,
                          const std::string& varName,
                          bool defaultVal)
{
    Olympe::VariableDefinition def;
    def.Name         = varName;
    def.Type         = Olympe::VariableType::Bool;
    def.DefaultValue = Olympe::TaskValue(defaultVal);
    def.IsLocal      = true;
    tmpl.LocalVariables.push_back(def);
    tmpl.Blackboard.push_back({ varName, Olympe::VariableType::Bool,
                                  Olympe::TaskValue(defaultVal), false });

    // Also initialize the LocalBlackboard directly for the test.
    std::vector<Olympe::BlackboardEntry> entries;
    Olympe::BlackboardEntry e;
    e.Key      = varName;
    e.Type     = Olympe::VariableType::Bool;
    e.Default  = Olympe::TaskValue(defaultVal);
    e.IsGlobal = false;
    entries.push_back(e);
    lb.InitializeFromEntries(entries);
}

/// Registers a local Int variable in the blackboard entries only (for SetBBValue test).
static void RegisterInt(Olympe::LocalBlackboard& lb,
                         const std::string& varName,
                         int defaultVal)
{
    std::vector<Olympe::BlackboardEntry> entries;
    Olympe::BlackboardEntry e;
    e.Key      = varName;
    e.Type     = Olympe::VariableType::Int;
    e.Default  = Olympe::TaskValue(defaultVal);
    e.IsGlobal = false;
    entries.push_back(e);
    lb.InitializeFromEntries(entries);
}

// ---------------------------------------------------------------------------
// TEST 1: EntryPoint_AdvancesToFirstChild
// ---------------------------------------------------------------------------

static void Test1_EntryPointAdvancesToFirstChild()
{
    std::cout << "Test 1: EntryPoint_AdvancesToFirstChild..." << std::endl;

    bool passed = true;

    // Build template:
    //   EntryPoint(1) --Out--> AtomicTask(2, Task_LogMessage)
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test1";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID   = 1;
        ep.Type     = Olympe::TaskNodeType::EntryPoint;
        ep.NodeName = "EntryPoint";
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition task;
        task.NodeID       = 2;
        task.Type         = Olympe::TaskNodeType::AtomicTask;
        task.NodeName     = "LogMessage";
        task.AtomicTaskID = "Task_LogMessage";
        Olympe::ParameterBinding pb;
        pb.Type         = Olympe::ParameterBindingType::Literal;
        pb.LiteralValue = Olympe::TaskValue(std::string("Test1"));
        task.Parameters["message"] = pb;
        tmpl.Nodes.push_back(task);
    }
    AddExecConn(tmpl, 1, "Out", 2, "In");
    tmpl.BuildLookupCache();

    Olympe::LocalBlackboard lb;
    Olympe::TaskRunnerComponent runner;

    // Frame 1: CurrentNodeID == NODE_INDEX_NONE → set to EntryPointID, return.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);
    TEST_ASSERT(runner.CurrentNodeID == 1,
                "After frame 1, CurrentNodeID should be EntryPointID (1)");
    if (runner.CurrentNodeID != 1) passed = false;

    // Frame 2: process EntryPoint(1) → advance to LogMessage(2) → runs & stays at 2.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);
    TEST_ASSERT(runner.CurrentNodeID == 2,
                "After frame 2, CurrentNodeID should be LogMessage node (2)");
    if (runner.CurrentNodeID != 2) passed = false;

    ReportTest("Test1_EntryPointAdvancesToFirstChild", passed);
}

// ---------------------------------------------------------------------------
// TEST 2: Branch_TruePathTaken
// ---------------------------------------------------------------------------

static void Test2_BranchTruePathTaken()
{
    std::cout << "Test 2: Branch_TruePathTaken..." << std::endl;

    bool passed = true;

    // Build template:
    //   EntryPoint(1) --Out--> Branch(2) --Then--> AtomicTask(3)
    //                                     --Else--> AtomicTask(4)
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test2";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID = 1; ep.Type = Olympe::TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition br;
        br.NodeID = 2; br.Type = Olympe::TaskNodeType::Branch;
        tmpl.Nodes.push_back(br);
    }
    {
        Olympe::TaskNodeDefinition t;
        t.NodeID = 3; t.Type = Olympe::TaskNodeType::AtomicTask;
        t.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(t);
    }
    {
        Olympe::TaskNodeDefinition t;
        t.NodeID = 4; t.Type = Olympe::TaskNodeType::AtomicTask;
        t.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(t);
    }
    AddExecConn(tmpl, 1, "Out",  2, "In");
    AddExecConn(tmpl, 2, "Then", 3, "In");
    AddExecConn(tmpl, 2, "Else", 4, "In");
    tmpl.BuildLookupCache();

    Olympe::LocalBlackboard lb;
    Olympe::TaskRunnerComponent runner;

    // Pre-seed the Branch condition pin to true.
    runner.DataPinCache["2:Condition"] = Olympe::TaskValue(true);

    // Frame 1: initialise.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);
    // Frame 2: EntryPoint → Branch(true) → AtomicTask(3).
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    TEST_ASSERT(runner.CurrentNodeID == 3,
                "Branch true path: should land on node 3");
    if (runner.CurrentNodeID != 3) passed = false;

    ReportTest("Test2_BranchTruePathTaken", passed);
}

// ---------------------------------------------------------------------------
// TEST 3: Branch_FalsePathTaken
// ---------------------------------------------------------------------------

static void Test3_BranchFalsePathTaken()
{
    std::cout << "Test 3: Branch_FalsePathTaken..." << std::endl;

    bool passed = true;

    // Same topology as Test 2.
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test3";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID = 1; ep.Type = Olympe::TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition br;
        br.NodeID = 2; br.Type = Olympe::TaskNodeType::Branch;
        tmpl.Nodes.push_back(br);
    }
    {
        Olympe::TaskNodeDefinition t;
        t.NodeID = 3; t.Type = Olympe::TaskNodeType::AtomicTask;
        t.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(t);
    }
    {
        Olympe::TaskNodeDefinition t;
        t.NodeID = 4; t.Type = Olympe::TaskNodeType::AtomicTask;
        t.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(t);
    }
    AddExecConn(tmpl, 1, "Out",  2, "In");
    AddExecConn(tmpl, 2, "Then", 3, "In");
    AddExecConn(tmpl, 2, "Else", 4, "In");
    tmpl.BuildLookupCache();

    Olympe::LocalBlackboard lb;
    Olympe::TaskRunnerComponent runner;

    // Pre-seed the Branch condition pin to false.
    runner.DataPinCache["2:Condition"] = Olympe::TaskValue(false);

    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    TEST_ASSERT(runner.CurrentNodeID == 4,
                "Branch false path: should land on node 4");
    if (runner.CurrentNodeID != 4) passed = false;

    ReportTest("Test3_BranchFalsePathTaken", passed);
}

// ---------------------------------------------------------------------------
// TEST 4: Delay_ReturnsRunning
// ---------------------------------------------------------------------------

static void Test4_DelayReturnsRunning()
{
    std::cout << "Test 4: Delay_ReturnsRunning..." << std::endl;

    bool passed = true;

    // Build template:
    //   EntryPoint(1) --Out--> Delay(2, 1.0s)
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test4";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID = 1; ep.Type = Olympe::TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition delay;
        delay.NodeID       = 2;
        delay.Type         = Olympe::TaskNodeType::Delay;
        delay.DelaySeconds = 1.0f;
        tmpl.Nodes.push_back(delay);
    }
    AddExecConn(tmpl, 1, "Out", 2, "In");
    tmpl.BuildLookupCache();

    Olympe::LocalBlackboard lb;
    Olympe::TaskRunnerComponent runner;

    // Frame 1: initialise.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);
    // Frame 2: advance to Delay node; dt=0.016 < 1.0 → still running.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    TEST_ASSERT(runner.CurrentNodeID == 2,
                "Delay still running: CurrentNodeID should remain 2");
    if (runner.CurrentNodeID != 2) passed = false;

    // Verify the timer is accumulating.
    TEST_ASSERT(runner.StateTimer > 0.0f,
                "Delay: StateTimer should be > 0 after one tick");
    if (runner.StateTimer <= 0.0f) passed = false;

    ReportTest("Test4_DelayReturnsRunning", passed);
}

// ---------------------------------------------------------------------------
// TEST 5: Delay_CompletesAfterDuration
// ---------------------------------------------------------------------------

static void Test5_DelayCompletesAfterDuration()
{
    std::cout << "Test 5: Delay_CompletesAfterDuration..." << std::endl;

    bool passed = true;

    // Build template:
    //   EntryPoint(1) --Out--> Delay(2, 1.0s) --Completed--> AtomicTask(3)
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test5";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID = 1; ep.Type = Olympe::TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition delay;
        delay.NodeID       = 2;
        delay.Type         = Olympe::TaskNodeType::Delay;
        delay.DelaySeconds = 1.0f;
        tmpl.Nodes.push_back(delay);
    }
    {
        Olympe::TaskNodeDefinition task;
        task.NodeID       = 3;
        task.Type         = Olympe::TaskNodeType::AtomicTask;
        task.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(task);
    }
    AddExecConn(tmpl, 1, "Out",       2, "In");
    AddExecConn(tmpl, 2, "Completed", 3, "In");
    tmpl.BuildLookupCache();

    Olympe::LocalBlackboard lb;
    Olympe::TaskRunnerComponent runner;

    // Frame 1: initialise.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);
    // Frame 2: advance to Delay node and start timer.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    // Manually advance the timer beyond DelaySeconds to simulate elapsed time.
    runner.StateTimer = 1.1f;

    // Frame 3: Delay completes → advances to AtomicTask(3).
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    TEST_ASSERT(runner.CurrentNodeID == 3,
                "Delay completed: CurrentNodeID should advance to node 3");
    if (runner.CurrentNodeID != 3) passed = false;

    ReportTest("Test5_DelayCompletesAfterDuration", passed);
}

// ---------------------------------------------------------------------------
// TEST 6: DoOnce_BlocksSecondExecution
// ---------------------------------------------------------------------------

static void Test6_DoOnceBlocksSecondExecution()
{
    std::cout << "Test 6: DoOnce_BlocksSecondExecution..." << std::endl;

    bool passed = true;

    // Build template:
    //   EntryPoint(1) --Out--> DoOnce(2) --Out--> AtomicTask(3)
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test6";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID = 1; ep.Type = Olympe::TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition once;
        once.NodeID = 2; once.Type = Olympe::TaskNodeType::DoOnce;
        tmpl.Nodes.push_back(once);
    }
    {
        Olympe::TaskNodeDefinition task;
        task.NodeID       = 3;
        task.Type         = Olympe::TaskNodeType::AtomicTask;
        task.AtomicTaskID = "Task_LogMessage";
        tmpl.Nodes.push_back(task);
    }
    AddExecConn(tmpl, 1, "Out", 2, "In");
    AddExecConn(tmpl, 2, "Out", 3, "In");
    tmpl.BuildLookupCache();

    Olympe::LocalBlackboard lb;
    Olympe::TaskRunnerComponent runner;

    // First execution (frames 1 + 2):
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f); // init
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f); // run

    // After first run the AtomicTask(3) should have been reached.
    TEST_ASSERT(runner.CurrentNodeID == 3,
                "DoOnce first run: should reach AtomicTask(3)");
    if (runner.CurrentNodeID != 3) passed = false;

    // Reset current node to DoOnce (2) to simulate a second call.
    runner.CurrentNodeID = 2;

    // Second execution (frame 3 — starting at DoOnce which is already flagged):
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    // DoOnce was already fired → returns NODE_INDEX_NONE → execution ends,
    // CurrentNodeID remains 2 (the DoOnce node itself, not 3).
    TEST_ASSERT(runner.CurrentNodeID != 3,
                "DoOnce second run: should NOT reach AtomicTask(3)");
    if (runner.CurrentNodeID == 3) passed = false;

    ReportTest("Test6_DoOnceBlocksSecondExecution", passed);
}

// ---------------------------------------------------------------------------
// TEST 7: BBRoundTrip_LocalScope
// ---------------------------------------------------------------------------

static void Test7_BBRoundTripLocalScope()
{
    std::cout << "Test 7: BBRoundTrip_LocalScope..." << std::endl;

    bool passed = true;

    // Build template:
    //   EntryPoint(1) --Out--> SetBBValue(2, "local:myvar") --Out--> GetBBValue(3, "local:myvar")
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test7";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID = 1; ep.Type = Olympe::TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition set;
        set.NodeID = 2;
        set.Type   = Olympe::TaskNodeType::SetBBValue;
        set.BBKey  = "local:myvar";
        tmpl.Nodes.push_back(set);
    }
    {
        Olympe::TaskNodeDefinition get;
        get.NodeID = 3;
        get.Type   = Olympe::TaskNodeType::GetBBValue;
        get.BBKey  = "local:myvar";
        tmpl.Nodes.push_back(get);
    }
    AddExecConn(tmpl, 1, "Out", 2, "In");
    AddExecConn(tmpl, 2, "Out", 3, "In");
    tmpl.BuildLookupCache();

    // Register "myvar" as an Int in the LocalBlackboard.
    Olympe::LocalBlackboard lb;
    RegisterInt(lb, "myvar", 0);

    Olympe::TaskRunnerComponent runner;

    // Pre-seed the SetBBValue node's input data pin with the value 42.
    runner.DataPinCache["2:Value"] = Olympe::TaskValue(42);

    // Frame 1: initialise.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);
    // Frame 2: EntryPoint → SetBBValue → GetBBValue.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    // After the frame, LocalBB["myvar"] should be 42.
    const Olympe::TaskValue result = lb.GetValueScoped("local:myvar");
    TEST_ASSERT(result.GetType() == Olympe::VariableType::Int,
                "BBRoundTrip: result type should be Int");
    TEST_ASSERT(result.AsInt() == 42,
                "BBRoundTrip: result value should be 42");
    if (result.GetType() != Olympe::VariableType::Int || result.AsInt() != 42)
        passed = false;

    ReportTest("Test7_BBRoundTripLocalScope", passed);
}

// ---------------------------------------------------------------------------
// TEST 8: MaxStepsGuard_PreventInfiniteLoop
// ---------------------------------------------------------------------------

static void Test8_MaxStepsGuardPreventInfiniteLoop()
{
    std::cout << "Test 8: MaxStepsGuard_PreventInfiniteLoop..." << std::endl;

    bool passed = true;

    // Build circular graph:
    //   EntryPoint(1) --Out--> AtomicTask(2, LogMessage) --Out--> EntryPoint(1)
    // This creates an infinite execution loop within a single frame.
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name         = "Test8";
    tmpl.GraphType    = "VisualScript";
    tmpl.RootNodeID   = 1;
    tmpl.EntryPointID = 1;

    {
        Olympe::TaskNodeDefinition ep;
        ep.NodeID = 1; ep.Type = Olympe::TaskNodeType::EntryPoint;
        tmpl.Nodes.push_back(ep);
    }
    {
        Olympe::TaskNodeDefinition task;
        task.NodeID       = 2;
        task.Type         = Olympe::TaskNodeType::AtomicTask;
        task.AtomicTaskID = "Task_LogMessage";
        Olympe::ParameterBinding pb;
        pb.Type         = Olympe::ParameterBindingType::Literal;
        pb.LiteralValue = Olympe::TaskValue(std::string("loop"));
        task.Parameters["message"] = pb;
        tmpl.Nodes.push_back(task);
    }
    AddExecConn(tmpl, 1, "Out", 2, "In");  // EntryPoint → LogMessage
    AddExecConn(tmpl, 2, "Out", 1, "In");  // LogMessage → EntryPoint (circular)
    tmpl.BuildLookupCache();

    Olympe::LocalBlackboard lb;
    Olympe::TaskRunnerComponent runner;
    runner.CurrentNodeID = 1; // Start directly at EntryPoint (skip init frame).

    // ExecuteFrame must return within a bounded number of steps (MAX_STEPS_PER_FRAME).
    // If it hangs, the test will not complete.
    VSGraphExecutor::ExecuteFrame(1, runner, tmpl, lb, nullptr, 0.016f);

    // The frame should have completed (no hang).  The exact CurrentNodeID depends
    // on where the step limit was hit, but the function must have returned.
    TEST_ASSERT(true, "MaxStepsGuard: ExecuteFrame returned (no infinite loop)");

    ReportTest("Test8_MaxStepsGuardPreventInfiniteLoop", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=============================" << std::endl;
    std::cout << " VSGraphExecutor Tests" << std::endl;
    std::cout << "=============================" << std::endl;

    Test1_EntryPointAdvancesToFirstChild();
    Test2_BranchTruePathTaken();
    Test3_BranchFalsePathTaken();
    Test4_DelayReturnsRunning();
    Test5_DelayCompletesAfterDuration();
    Test6_DoOnceBlocksSecondExecution();
    Test7_BBRoundTripLocalScope();
    Test8_MaxStepsGuardPreventInfiniteLoop();

    std::cout << "-----------------------------" << std::endl;
    std::cout << "Passed: " << s_passCount << std::endl;
    std::cout << "Failed: " << s_failCount << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
