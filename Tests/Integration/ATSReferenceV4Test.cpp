/**
 * @file ATSReferenceV4Test.cpp
 * @brief Integration tests for ATS Visual Script v4 reference graph.
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * Tests cover:
 *   a) Loading reference_complete_v4.ats successfully
 *   b) Verifying schema version and graph type
 *   c) Verifying expected node count (16 nodes)
 *   d) Verifying all required node types are present
 *   e) Verifying blackboard variables are declared
 *   f) Executing one frame and verifying execution advances past EntryPoint
 *
 * Run from the repository root so that relative paths to Gamedata/ resolve.
 */

#include "TaskSystem/TaskGraphLoader.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "TaskSystem/VSGraphExecutor.h"
#include "TaskSystem/LocalBlackboard.h"
#include "ECS/Components/TaskRunnerComponent.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(condition, message)                                \
    do {                                                               \
        if (!(condition)) {                                            \
            std::cout << "  FAIL: " << (message) << std::endl;        \
            ++s_failCount;                                             \
        }                                                              \
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
// Test A: Load reference_complete_v4.ats
// ---------------------------------------------------------------------------

static void TestA_LoadReferenceCompleteV4()
{
    std::cout << "Test A: Load reference_complete_v4.ats..." << std::endl;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl = Olympe::TaskGraphLoader::LoadFromFile(
        "Gamedata/TaskGraph/Examples/reference_complete_v4.ats", errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: LoadFromFile returned nullptr" << std::endl;
        for (size_t i = 0; i < errors.size(); ++i)
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        ReportTest("TestA_LoadReferenceCompleteV4", false);
        return;
    }

    TEST_ASSERT(errors.empty(), "No load errors expected");
    if (!errors.empty()) { passed = false; }

    TEST_ASSERT(tmpl->GraphType == "VisualScript", "GraphType should be 'VisualScript'");
    if (tmpl->GraphType != "VisualScript") { passed = false; }

    TEST_ASSERT(tmpl->Name == "ReferenceCompleteV4", "Name should be 'ReferenceCompleteV4'");
    if (tmpl->Name != "ReferenceCompleteV4") { passed = false; }

    delete tmpl;
    ReportTest("TestA_LoadReferenceCompleteV4", passed);
}

// ---------------------------------------------------------------------------
// Test B: Verify node count
// ---------------------------------------------------------------------------

static void TestB_NodeCount()
{
    std::cout << "Test B: Verify node count = 16..." << std::endl;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl = Olympe::TaskGraphLoader::LoadFromFile(
        "Gamedata/TaskGraph/Examples/reference_complete_v4.ats", errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: Could not load file" << std::endl;
        ReportTest("TestB_NodeCount", false);
        return;
    }

    TEST_ASSERT(tmpl->Nodes.size() == 16u,
                ("Expected 16 nodes, got " + std::to_string(tmpl->Nodes.size())).c_str());
    if (tmpl->Nodes.size() != 16u) { passed = false; }

    delete tmpl;
    ReportTest("TestB_NodeCount", passed);
}

// ---------------------------------------------------------------------------
// Test C: Verify all required node types present
// ---------------------------------------------------------------------------

static void TestC_AllNodeTypesPresent()
{
    std::cout << "Test C: Verify all required node types present..." << std::endl;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl = Olympe::TaskGraphLoader::LoadFromFile(
        "Gamedata/TaskGraph/Examples/reference_complete_v4.ats", errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: Could not load file" << std::endl;
        ReportTest("TestC_AllNodeTypesPresent", false);
        return;
    }

    bool hasEntryPoint  = false;
    bool hasVSSequence  = false;
    bool hasBranch      = false;
    bool hasAtomicTask  = false;
    bool hasGetBBValue  = false;
    bool hasSetBBValue  = false;
    bool hasMathOp      = false;
    bool hasDoOnce      = false;
    bool hasWhile       = false;
    bool hasDelay       = false;
    bool hasSubGraph    = false;

    for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
    {
        const Olympe::TaskNodeDefinition& n = tmpl->Nodes[i];
        switch (n.Type)
        {
            case Olympe::TaskNodeType::EntryPoint:  hasEntryPoint  = true; break;
            case Olympe::TaskNodeType::VSSequence:  hasVSSequence  = true; break;
            case Olympe::TaskNodeType::Branch:      hasBranch      = true; break;
            case Olympe::TaskNodeType::AtomicTask:  hasAtomicTask  = true; break;
            case Olympe::TaskNodeType::GetBBValue:  hasGetBBValue  = true; break;
            case Olympe::TaskNodeType::SetBBValue:  hasSetBBValue  = true; break;
            case Olympe::TaskNodeType::MathOp:      hasMathOp      = true; break;
            case Olympe::TaskNodeType::DoOnce:      hasDoOnce      = true; break;
            case Olympe::TaskNodeType::While:       hasWhile       = true; break;
            case Olympe::TaskNodeType::Delay:       hasDelay       = true; break;
            case Olympe::TaskNodeType::SubGraph:    hasSubGraph    = true; break;
            default: break;
        }
    }

    TEST_ASSERT(hasEntryPoint,  "Missing EntryPoint node");
    TEST_ASSERT(hasVSSequence,  "Missing VSSequence node");
    TEST_ASSERT(hasBranch,      "Missing Branch node");
    TEST_ASSERT(hasAtomicTask,  "Missing AtomicTask node");
    TEST_ASSERT(hasGetBBValue,  "Missing GetBBValue node");
    TEST_ASSERT(hasSetBBValue,  "Missing SetBBValue node");
    TEST_ASSERT(hasMathOp,      "Missing MathOp node");
    TEST_ASSERT(hasDoOnce,      "Missing DoOnce node");
    TEST_ASSERT(hasWhile,       "Missing While node");
    TEST_ASSERT(hasDelay,       "Missing Delay node");
    TEST_ASSERT(hasSubGraph,    "Missing SubGraph node");

    if (!hasEntryPoint || !hasVSSequence || !hasBranch || !hasAtomicTask ||
        !hasGetBBValue || !hasSetBBValue || !hasMathOp || !hasDoOnce ||
        !hasWhile || !hasDelay || !hasSubGraph)
    {
        passed = false;
    }

    delete tmpl;
    ReportTest("TestC_AllNodeTypesPresent", passed);
}

// ---------------------------------------------------------------------------
// Test D: Verify blackboard variables
// ---------------------------------------------------------------------------

static void TestD_BlackboardVariables()
{
    std::cout << "Test D: Verify blackboard variables..." << std::endl;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl = Olympe::TaskGraphLoader::LoadFromFile(
        "Gamedata/TaskGraph/Examples/reference_complete_v4.ats", errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: Could not load file" << std::endl;
        ReportTest("TestD_BlackboardVariables", false);
        return;
    }

    TEST_ASSERT(tmpl->Blackboard.size() >= 5u,
                "Expected at least 5 blackboard variables");
    if (tmpl->Blackboard.size() < 5u) { passed = false; }

    // Check that Counter key exists
    bool hasCounter = false;
    bool hasIsActive = false;
    for (size_t i = 0; i < tmpl->Blackboard.size(); ++i)
    {
        if (tmpl->Blackboard[i].Key == "Counter")   hasCounter  = true;
        if (tmpl->Blackboard[i].Key == "IsActive")  hasIsActive = true;
    }
    TEST_ASSERT(hasCounter, "Blackboard should have 'Counter' variable");
    if (!hasCounter) { passed = false; }

    TEST_ASSERT(hasIsActive, "Blackboard should have 'IsActive' variable");
    if (!hasIsActive) { passed = false; }

    delete tmpl;
    ReportTest("TestD_BlackboardVariables", passed);
}

// ---------------------------------------------------------------------------
// Test E: Execute one frame — verify EntryPoint is found and graph has
//         connections so execution can proceed
// ---------------------------------------------------------------------------

static void TestE_ExecuteOneFrame()
{
    std::cout << "Test E: Execute one frame..." << std::endl;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl = Olympe::TaskGraphLoader::LoadFromFile(
        "Gamedata/TaskGraph/Examples/reference_complete_v4.ats", errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: Could not load file" << std::endl;
        ReportTest("TestE_ExecuteOneFrame", false);
        return;
    }

    // EntryPointID must be valid
    TEST_ASSERT(tmpl->EntryPointID != Olympe::NODE_INDEX_NONE,
                "EntryPointID should be set");
    if (tmpl->EntryPointID == Olympe::NODE_INDEX_NONE) { passed = false; }

    // Must have exec connections
    TEST_ASSERT(!tmpl->ExecConnections.empty(),
                "ExecConnections should not be empty");
    if (tmpl->ExecConnections.empty()) { passed = false; }

    // Set up runner component and blackboard
    Olympe::TaskRunnerComponent runner;
    runner.CurrentNodeID = tmpl->EntryPointID;
    runner.graphAssetPath = "Gamedata/TaskGraph/Examples/reference_complete_v4.ats";

    Olympe::LocalBlackboard bb;
    bb.Initialize(*tmpl);

    // Execute one frame
    Olympe::VSGraphExecutor::ExecuteFrame(0 /* entityID */, runner, *tmpl, bb,
                                          nullptr /* world */, 0.016f);

    // After one frame, CurrentNodeID should have advanced past EntryPoint
    TEST_ASSERT(runner.CurrentNodeID != tmpl->EntryPointID,
                "Execution should advance past EntryPoint after one frame");
    if (runner.CurrentNodeID == tmpl->EntryPointID) { passed = false; }

    delete tmpl;
    ReportTest("TestE_ExecuteOneFrame", passed);
}

// ---------------------------------------------------------------------------
// Test F: All v4 Blueprints/AI files load without type warnings
//         (i.e. have "type" == "graphType" at root level)
// ---------------------------------------------------------------------------

static void TestF_BlueprintsHaveTypeField()
{
    std::cout << "Test F: Blueprints/AI v4 files have root 'type' field..." << std::endl;

    // These are the files that were updated
    const char* files[] = {
        "Blueprints/AI/idle.json",
        "Blueprints/AI/guard_patrol.json",
        "Blueprints/AI/guard_combat.json",
        "Blueprints/AI/guard_ai.json",
        "Blueprints/AI/npc_ai.json",
        "Blueprints/AI/patrol.json",
        "Blueprints/AI/guard.json",
        "Blueprints/AI/investigate.json",
        nullptr
    };

    bool passed = true;

    for (int i = 0; files[i] != nullptr; ++i)
    {
        std::vector<std::string> errors;
        Olympe::TaskGraphTemplate* tmpl =
            Olympe::TaskGraphLoader::LoadFromFile(files[i], errors);

        if (tmpl == nullptr)
        {
            std::cout << "  FAIL: Could not load " << files[i] << std::endl;
            passed = false;
            continue;
        }

        TEST_ASSERT(tmpl->GraphType == "VisualScript",
                    (std::string(files[i]) + " graphType should be VisualScript").c_str());
        if (tmpl->GraphType != "VisualScript")
            passed = false;

        delete tmpl;
    }

    ReportTest("TestF_BlueprintsHaveTypeField", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== ATSReferenceV4Test ===" << std::endl;

    TestA_LoadReferenceCompleteV4();
    TestB_NodeCount();
    TestC_AllNodeTypesPresent();
    TestD_BlackboardVariables();
    TestE_ExecuteOneFrame();
    TestF_BlueprintsHaveTypeField();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
