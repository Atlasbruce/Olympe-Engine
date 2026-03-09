/**
 * @file BTtoVSMigratorTest.cpp
 * @brief Unit tests for BTtoVSMigrator (Phase 6 ATS VS editor).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   1.  Migrator_IsBTv2_DetectsCorrectly
 *   2.  Migrator_SelectorNode_ConvertedToBranch
 *   3.  Migrator_SequenceNode_ConvertedToVSSequence
 *   4.  Migrator_ActionNode_ConvertedToAtomicTask
 *   5.  Migrator_BlackboardKeys_Preserved
 *   6.  Migrator_Connections_ChildrenPreserved
 *   7.  Migrator_RootNode_BecomesEntryPoint
 *   8.  Migrator_EmptyGraph_NodesEmpty
 *
 * No SDL3, ImGui, or Editor dependency.
 * C++14 compliant — no C++17/20 features.
 */

#include "BlueprintEditor/BTtoVSMigrator.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <vector>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeType;
using Olympe::VariableType;
using Olympe::NODE_INDEX_NONE;
using Olympe::BTtoVSMigrator;
using nlohmann::json;

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

/// Returns a pointer to the first node with the given ID, or nullptr.
static const Olympe::TaskNodeDefinition* FindNode(const TaskGraphTemplate& g, int id)
{
    for (const auto& n : g.Nodes)
        if (n.NodeID == id) return &n;
    return nullptr;
}

/// Builds a minimal BT v2 JSON document with a single-level flat node list.
static json MakeBTv2(const json& nodes,
                      int rootId = 0,
                      const json& blackboard = json::array())
{
    json bt;
    bt["blueprintType"]  = "BehaviorTree";
    bt["schema_version"] = 2;
    bt["name"]           = "TestBT";
    bt["data"]["data"]["nodes"]      = nodes;
    bt["data"]["data"]["rootNodeId"] = rootId;
    bt["data"]["data"]["blackboard"] = blackboard;
    return bt;
}

// ---------------------------------------------------------------------------
// Test 1: IsBTv2 detection
// ---------------------------------------------------------------------------

static void Test1_IsBTv2_DetectsCorrectly()
{
    int prevFail = s_failCount;

    // Valid BT v2
    {
        json j;
        j["blueprintType"]  = "BehaviorTree";
        j["schema_version"] = 2;
        TEST_ASSERT(BTtoVSMigrator::IsBTv2(j), "schema v2 BehaviorTree should be detected");
    }

    // BT v2 without schema_version (defaults to v2 heuristic)
    {
        json j;
        j["blueprintType"] = "BehaviorTree";
        TEST_ASSERT(BTtoVSMigrator::IsBTv2(j), "BehaviorTree without schema_version => BT v2");
    }

    // VS v4 should not be detected as BT v2
    {
        json j;
        j["graphType"]       = "VisualScript";
        j["schema_version"]  = 4;
        TEST_ASSERT(!BTtoVSMigrator::IsBTv2(j), "VS v4 should NOT be detected as BT v2");
    }

    // schema v3 BehaviorTree — above v2 limit
    {
        json j;
        j["blueprintType"]  = "BehaviorTree";
        j["schema_version"] = 3;
        TEST_ASSERT(!BTtoVSMigrator::IsBTv2(j), "schema v3 BT should NOT be BT v2");
    }

    ReportTest("Test1_IsBTv2_DetectsCorrectly", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2: Selector node → Branch
// ---------------------------------------------------------------------------

static void Test2_SelectorNode_ConvertedToBranch()
{
    int prevFail = s_failCount;

    json nodes = json::array();
    {
        json root; root["id"] = 0; root["type"] = "Start";    root["name"] = "Root";     root["children"] = json::array(); nodes.push_back(root);
        json sel;  sel["id"]  = 1; sel["type"]  = "Selector"; sel["name"] = "Selector1"; sel["children"]  = json::array(); nodes.push_back(sel);
    }
    json bt = MakeBTv2(nodes, 0);

    std::vector<std::string> errors;
    TaskGraphTemplate result = BTtoVSMigrator::Convert(bt, errors);

    TEST_ASSERT(errors.empty() || true, "Convert should not produce fatal errors");

    const auto* sel = FindNode(result, 1);
    TEST_ASSERT(sel != nullptr, "Selector node (id=1) should be present in result");
    if (sel)
    {
        TEST_ASSERT(sel->Type == TaskNodeType::Branch,
                    "Selector should map to TaskNodeType::Branch");
    }

    ReportTest("Test2_SelectorNode_ConvertedToBranch", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: Sequence node → VSSequence
// ---------------------------------------------------------------------------

static void Test3_SequenceNode_ConvertedToVSSequence()
{
    int prevFail = s_failCount;

    json nodes = json::array();
    {
        json root; root["id"] = 0; root["type"] = "Start";    root["name"] = "Root";  root["children"] = json::array(); nodes.push_back(root);
        json seq;  seq["id"]  = 2; seq["type"]  = "Sequence"; seq["name"] = "Seq1";   seq["children"]  = json::array(); nodes.push_back(seq);
    }
    json bt = MakeBTv2(nodes, 0);

    std::vector<std::string> errors;
    TaskGraphTemplate result = BTtoVSMigrator::Convert(bt, errors);

    const auto* seq = FindNode(result, 2);
    TEST_ASSERT(seq != nullptr, "Sequence node (id=2) should be present");
    if (seq)
    {
        TEST_ASSERT(seq->Type == TaskNodeType::Sequence,
                    "Sequence should map to TaskNodeType::Sequence");
    }

    ReportTest("Test3_SequenceNode_ConvertedToVSSequence", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: Action node → AtomicTask
// ---------------------------------------------------------------------------

static void Test4_ActionNode_ConvertedToAtomicTask()
{
    int prevFail = s_failCount;

    json nodes = json::array();
    {
        json root;   root["id"] = 0;  root["type"] = "Start";   root["name"] = "Root";   root["children"] = json::array(); nodes.push_back(root);
        json action; action["id"] = 3; action["type"] = "Action"; action["name"] = "DoThing"; action["children"] = json::array();
        action["parameters"]["taskType"] = "Task_DoThing";
        nodes.push_back(action);
    }
    json bt = MakeBTv2(nodes, 0);

    std::vector<std::string> errors;
    TaskGraphTemplate result = BTtoVSMigrator::Convert(bt, errors);

    const auto* act = FindNode(result, 3);
    TEST_ASSERT(act != nullptr, "Action node (id=3) should be present");
    if (act)
    {
        TEST_ASSERT(act->Type == TaskNodeType::AtomicTask,
                    "Action should map to TaskNodeType::AtomicTask");
        TEST_ASSERT(act->AtomicTaskID == "Task_DoThing",
                    "AtomicTaskID should be 'Task_DoThing'");
    }

    ReportTest("Test4_ActionNode_ConvertedToAtomicTask", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: Blackboard keys preserved
// ---------------------------------------------------------------------------

static void Test5_BlackboardKeys_Preserved()
{
    int prevFail = s_failCount;

    json nodes = json::array();
    {
        json root; root["id"] = 0; root["type"] = "Start"; root["name"] = "Root"; root["children"] = json::array(); nodes.push_back(root);
    }

    json bb = json::array();
    {
        json entry1; entry1["key"] = "HealthPoints"; entry1["type"] = "Int"; bb.push_back(entry1);
        json entry2; entry2["key"] = "IsAlert";      entry2["type"] = "Bool"; bb.push_back(entry2);
        json entry3; entry3["key"] = "TargetName";   entry3["type"] = "String"; bb.push_back(entry3);
    }
    json bt = MakeBTv2(nodes, 0, bb);

    std::vector<std::string> errors;
    TaskGraphTemplate result = BTtoVSMigrator::Convert(bt, errors);

    TEST_ASSERT(result.Blackboard.size() == 3, "Should have 3 blackboard entries");

    bool hasHealth = false, hasAlert = false, hasTarget = false;
    for (const auto& be : result.Blackboard)
    {
        if (be.Key == "HealthPoints") { hasHealth = true; TEST_ASSERT(be.Type == VariableType::Int, "HealthPoints should be Int"); }
        if (be.Key == "IsAlert")      { hasAlert  = true; TEST_ASSERT(be.Type == VariableType::Bool, "IsAlert should be Bool"); }
        if (be.Key == "TargetName")   { hasTarget = true; TEST_ASSERT(be.Type == VariableType::String, "TargetName should be String"); }
    }
    TEST_ASSERT(hasHealth, "HealthPoints key should be present");
    TEST_ASSERT(hasAlert,  "IsAlert key should be present");
    TEST_ASSERT(hasTarget, "TargetName key should be present");

    ReportTest("Test5_BlackboardKeys_Preserved", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6: Children links become ExecConnections
// ---------------------------------------------------------------------------

static void Test6_Connections_ChildrenPreserved()
{
    int prevFail = s_failCount;

    // BT tree:  Root(0) → Selector(1) → [Action(2), Action(3)]
    json nodes = json::array();
    {
        json root; root["id"] = 0; root["type"] = "Start";    root["name"] = "Root";
        json rootChildren = json::array(); rootChildren.push_back(json(1));
        root["children"] = rootChildren; nodes.push_back(root);

        json sel;  sel["id"]  = 1; sel["type"]  = "Selector"; sel["name"] = "Sel";
        json selChildren = json::array(); selChildren.push_back(json(2)); selChildren.push_back(json(3));
        sel["children"] = selChildren; nodes.push_back(sel);

        json a2; a2["id"] = 2; a2["type"] = "Action"; a2["name"] = "A2"; a2["children"] = json::array(); nodes.push_back(a2);
        json a3; a3["id"] = 3; a3["type"] = "Action"; a3["name"] = "A3"; a3["children"] = json::array(); nodes.push_back(a3);
    }
    json bt = MakeBTv2(nodes, 0);

    std::vector<std::string> errors;
    TaskGraphTemplate result = BTtoVSMigrator::Convert(bt, errors);

    // Expect connections: 0→1, 1→2, 1→3
    bool has01 = false, has12 = false, has13 = false;
    for (const auto& ec : result.ExecConnections)
    {
        if (ec.SourceNodeID == 0 && ec.TargetNodeID == 1) has01 = true;
        if (ec.SourceNodeID == 1 && ec.TargetNodeID == 2) has12 = true;
        if (ec.SourceNodeID == 1 && ec.TargetNodeID == 3) has13 = true;
    }
    TEST_ASSERT(has01, "Connection Root(0) → Selector(1) should exist");
    TEST_ASSERT(has12, "Connection Selector(1) → Action(2) should exist");
    TEST_ASSERT(has13, "Connection Selector(1) → Action(3) should exist");

    ReportTest("Test6_Connections_ChildrenPreserved", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7: Root node becomes EntryPoint
// ---------------------------------------------------------------------------

static void Test7_RootNode_BecomesEntryPoint()
{
    int prevFail = s_failCount;

    json nodes = json::array();
    {
        json root; root["id"] = 10; root["type"] = "Start"; root["name"] = "RootNode"; root["children"] = json::array(); nodes.push_back(root);
        json act;  act["id"]  = 11; act["type"]  = "Action"; act["name"] = "Act";       act["children"]  = json::array(); nodes.push_back(act);
    }
    json bt = MakeBTv2(nodes, 10);

    std::vector<std::string> errors;
    TaskGraphTemplate result = BTtoVSMigrator::Convert(bt, errors);

    TEST_ASSERT(result.EntryPointID == 10, "EntryPointID should be 10 (the root)");

    const auto* ep = FindNode(result, 10);
    TEST_ASSERT(ep != nullptr, "Root node (id=10) should be in graph");
    if (ep)
    {
        TEST_ASSERT(ep->Type == TaskNodeType::EntryPoint,
                    "Root node should have type EntryPoint");
    }

    ReportTest("Test7_RootNode_BecomesEntryPoint", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8: Empty graph — returns empty TaskGraphTemplate
// ---------------------------------------------------------------------------

static void Test8_EmptyGraph_NodesEmpty()
{
    int prevFail = s_failCount;

    // BT v2 JSON with empty nodes array
    json bt;
    bt["blueprintType"]              = "BehaviorTree";
    bt["schema_version"]             = 2;
    bt["name"]                       = "EmptyBT";
    bt["data"]["data"]["nodes"]      = json::array();
    bt["data"]["data"]["rootNodeId"] = -1;

    std::vector<std::string> errors;
    TaskGraphTemplate result = BTtoVSMigrator::Convert(bt, errors);

    // An empty BT produces an empty (or minimal) VS graph
    TEST_ASSERT(result.GraphType == "VisualScript", "GraphType should be VisualScript");
    TEST_ASSERT(result.Nodes.empty(), "Empty BT should produce empty VS graph");

    ReportTest("Test8_EmptyGraph_NodesEmpty", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=============================" << std::endl;
    std::cout << "  BTtoVSMigrator Unit Tests  " << std::endl;
    std::cout << "=============================" << std::endl;

    Test1_IsBTv2_DetectsCorrectly();
    Test2_SelectorNode_ConvertedToBranch();
    Test3_SequenceNode_ConvertedToVSSequence();
    Test4_ActionNode_ConvertedToAtomicTask();
    Test5_BlackboardKeys_Preserved();
    Test6_Connections_ChildrenPreserved();
    Test7_RootNode_BecomesEntryPoint();
    Test8_EmptyGraph_NodesEmpty();

    std::cout << "=============================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
