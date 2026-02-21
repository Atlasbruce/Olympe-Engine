/**
 * @file TaskGraphLoaderTest.cpp
 * @brief Unit tests for TaskGraphLoader (Atomic Task System Phase 1.2)
 * @author Olympe Engine
 * @date 2026-02-21
 *
 * @details
 * Tests cover:
 *   a) Loading a schema v2 BehaviorTree file (guardV2_ai.json)
 *   b) Loading a schema v2 file with Repeater nodes (npc_wander_ai.json)
 *   c) Loading a minimal inline schema v3 TaskGraph JSON
 *   d) Invalid JSON (missing child reference) returns nullptr and fills outErrors
 *
 * Run from the repository root so that relative paths to Blueprints/AI/ resolve.
 * Compile with C++14 and link against OlympeCore.
 */

#include "TaskSystem/TaskGraphLoader.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "json_helper.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

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
// Test a: Load guardV2_ai.json (schema v2, multiple node types)
// ---------------------------------------------------------------------------

static void TestA_LoadGuardV2()
{
    std::cout << "Test A: Load guardV2_ai.json..." << std::endl;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl =
        Olympe::TaskGraphLoader::LoadFromFile("Blueprints/AI/guardV2_ai.json", errors);

    bool passed = true;

    // Must not be null
    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: LoadFromFile returned nullptr" << std::endl;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        }
        ReportTest("TestA_LoadGuardV2", false);
        return;
    }

    // RootNodeID must be 1
    TEST_ASSERT(tmpl->RootNodeID == 1, "RootNodeID should be 1");
    if (tmpl->RootNodeID != 1) { passed = false; }

    // Nodes must not be empty
    TEST_ASSERT(!tmpl->Nodes.empty(), "Nodes should not be empty");
    if (tmpl->Nodes.empty()) { passed = false; }

    // Lookup cache should work
    const Olympe::TaskNodeDefinition* rootNode = tmpl->GetNode(1);
    TEST_ASSERT(rootNode != nullptr, "Root node (id=1) must be found in lookup cache");
    if (rootNode == nullptr) { passed = false; }

    if (rootNode != nullptr)
    {
        // Root is a Selector in this file
        TEST_ASSERT(rootNode->Type == Olympe::TaskNodeType::Selector,
                    "Root node (id=1) type should be Selector");
        if (rootNode->Type != Olympe::TaskNodeType::Selector) { passed = false; }
    }

    // Verify at least one Action node maps to AtomicTask
    bool foundAction = false;
    for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
    {
        if (tmpl->Nodes[i].Type == Olympe::TaskNodeType::AtomicTask &&
            !tmpl->Nodes[i].AtomicTaskID.empty())
        {
            foundAction = true;
            break;
        }
    }
    TEST_ASSERT(foundAction, "At least one AtomicTask node should exist with a non-empty AtomicTaskID");
    if (!foundAction) { passed = false; }

    // Verify Condition nodes are mapped to AtomicTask
    bool conditionsMapped = true;
    for (size_t i = 0; i < tmpl->Nodes.size(); ++i)
    {
        // There should be no leftover "Condition" type; all must be AtomicTask
        if (tmpl->Nodes[i].NodeName.find("?") != std::string::npos)
        {
            if (tmpl->Nodes[i].Type != Olympe::TaskNodeType::AtomicTask)
            {
                conditionsMapped = false;
            }
        }
    }
    TEST_ASSERT(conditionsMapped, "Condition nodes should be mapped to AtomicTask");
    if (!conditionsMapped) { passed = false; }

    delete tmpl;
    ReportTest("TestA_LoadGuardV2", passed);
}

// ---------------------------------------------------------------------------
// Test b: Load npc_wander_ai.json (schema v2, Repeater node)
// ---------------------------------------------------------------------------

static void TestB_LoadNpcWander()
{
    std::cout << "Test B: Load npc_wander_ai.json..." << std::endl;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl =
        Olympe::TaskGraphLoader::LoadFromFile("Blueprints/AI/npc_wander_ai.json", errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: LoadFromFile returned nullptr" << std::endl;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        }
        ReportTest("TestB_LoadNpcWander", false);
        return;
    }

    // 6 nodes in npc_wander_ai.json
    TEST_ASSERT(tmpl->Nodes.size() == 6, "npc_wander_ai.json should have 6 nodes");
    if (tmpl->Nodes.size() != 6) { passed = false; }

    // RootNodeID == 1 (Repeater node)
    TEST_ASSERT(tmpl->RootNodeID == 1, "RootNodeID should be 1");
    if (tmpl->RootNodeID != 1) { passed = false; }

    // Node id=1 is a Repeater -> should map to Decorator
    const Olympe::TaskNodeDefinition* repeaterNode = tmpl->GetNode(1);
    TEST_ASSERT(repeaterNode != nullptr, "Node id=1 must exist");
    if (repeaterNode != nullptr)
    {
        TEST_ASSERT(repeaterNode->Type == Olympe::TaskNodeType::Decorator,
                    "Node id=1 (Repeater) should map to Decorator");
        if (repeaterNode->Type != Olympe::TaskNodeType::Decorator) { passed = false; }

        // Should have one child: node id=2
        TEST_ASSERT(repeaterNode->ChildrenIDs.size() == 1, "Repeater should have 1 child");
        if (!repeaterNode->ChildrenIDs.empty())
        {
            TEST_ASSERT(repeaterNode->ChildrenIDs[0] == 2, "Repeater child should be id=2");
            if (repeaterNode->ChildrenIDs[0] != 2) { passed = false; }
        }

        // repeatCount parameter should exist
        auto it = repeaterNode->Parameters.find("repeatCount");
        TEST_ASSERT(it != repeaterNode->Parameters.end(),
                    "Repeater should have repeatCount parameter");
        if (it != repeaterNode->Parameters.end())
        {
            TEST_ASSERT(it->second.Type == Olympe::ParameterBindingType::Literal,
                        "repeatCount binding should be Literal");
            if (it->second.Type != Olympe::ParameterBindingType::Literal) { passed = false; }
        }
    }

    // Node id=2 is a Sequence
    const Olympe::TaskNodeDefinition* seqNode = tmpl->GetNode(2);
    TEST_ASSERT(seqNode != nullptr, "Node id=2 must exist");
    if (seqNode != nullptr)
    {
        TEST_ASSERT(seqNode->Type == Olympe::TaskNodeType::Sequence,
                    "Node id=2 should be Sequence");
        if (seqNode->Type != Olympe::TaskNodeType::Sequence) { passed = false; }

        // Should have 4 children (nodes 3, 4, 5, 6)
        TEST_ASSERT(seqNode->ChildrenIDs.size() == 4, "Sequence should have 4 children");
        if (seqNode->ChildrenIDs.size() != 4) { passed = false; }
    }

    // Node id=5 (Action "RequestPathfinding") should be AtomicTask
    const Olympe::TaskNodeDefinition* actionNode = tmpl->GetNode(5);
    TEST_ASSERT(actionNode != nullptr, "Node id=5 must exist");
    if (actionNode != nullptr)
    {
        TEST_ASSERT(actionNode->Type == Olympe::TaskNodeType::AtomicTask,
                    "Node id=5 (Action) should map to AtomicTask");
        if (actionNode->Type != Olympe::TaskNodeType::AtomicTask) { passed = false; }

        TEST_ASSERT(actionNode->AtomicTaskID == "RequestPathfinding",
                    "Node id=5 AtomicTaskID should be 'RequestPathfinding'");
        if (actionNode->AtomicTaskID != "RequestPathfinding") { passed = false; }
    }

    delete tmpl;
    ReportTest("TestB_LoadNpcWander", passed);
}

// ---------------------------------------------------------------------------
// Test c: Inline schema v3 JSON
// ---------------------------------------------------------------------------

static void TestC_InlineSchemaV3()
{
    std::cout << "Test C: Inline schema v3 JSON..." << std::endl;

    // Build minimal v3 JSON in-memory
    json v3Json = json::object();
    v3Json["schema_version"] = 3;
    v3Json["name"] = "MinimalV3Task";
    v3Json["description"] = "Test v3 schema";

    json dataObj = json::object();
    dataObj["rootNodeId"] = 1;

    json nodesArr = json::array();

    // Root node
    json rootNode = json::object();
    rootNode["id"]   = 1;
    rootNode["name"] = "Root Sequence";
    rootNode["type"] = "Sequence";
    json rootChildren = json::array();
    rootChildren.push_back(2);
    rootNode["children"] = rootChildren;

    // AtomicTask node
    json atomicNode = json::object();
    atomicNode["id"]           = 2;
    atomicNode["name"]         = "Move To Target";
    atomicNode["type"]         = "AtomicTask";
    atomicNode["atomicTaskId"] = "MoveToTarget";

    json atomicParams = json::object();
    atomicParams["speed"] = 5.0f;
    atomicNode["parameters"] = atomicParams;

    nodesArr.push_back(rootNode);
    nodesArr.push_back(atomicNode);
    dataObj["nodes"] = nodesArr;
    v3Json["data"]   = dataObj;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl =
        Olympe::TaskGraphLoader::LoadFromJson(v3Json, errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: LoadFromJson returned nullptr" << std::endl;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        }
        ReportTest("TestC_InlineSchemaV3", false);
        return;
    }

    TEST_ASSERT(tmpl->Name == "MinimalV3Task", "Name should be 'MinimalV3Task'");
    if (tmpl->Name != "MinimalV3Task") { passed = false; }

    TEST_ASSERT(tmpl->RootNodeID == 1, "RootNodeID should be 1");
    if (tmpl->RootNodeID != 1) { passed = false; }

    TEST_ASSERT(tmpl->Nodes.size() == 2, "Should have 2 nodes");
    if (tmpl->Nodes.size() != 2) { passed = false; }

    const Olympe::TaskNodeDefinition* seqNode = tmpl->GetNode(1);
    TEST_ASSERT(seqNode != nullptr, "Node id=1 must exist");
    if (seqNode != nullptr)
    {
        TEST_ASSERT(seqNode->Type == Olympe::TaskNodeType::Sequence,
                    "Node id=1 should be Sequence");
        if (seqNode->Type != Olympe::TaskNodeType::Sequence) { passed = false; }
    }

    const Olympe::TaskNodeDefinition* atomNode = tmpl->GetNode(2);
    TEST_ASSERT(atomNode != nullptr, "Node id=2 must exist");
    if (atomNode != nullptr)
    {
        TEST_ASSERT(atomNode->Type == Olympe::TaskNodeType::AtomicTask,
                    "Node id=2 should be AtomicTask");
        if (atomNode->Type != Olympe::TaskNodeType::AtomicTask) { passed = false; }

        TEST_ASSERT(atomNode->AtomicTaskID == "MoveToTarget",
                    "AtomicTaskID should be 'MoveToTarget'");
        if (atomNode->AtomicTaskID != "MoveToTarget") { passed = false; }

        // Check speed parameter
        auto it = atomNode->Parameters.find("speed");
        TEST_ASSERT(it != atomNode->Parameters.end(),
                    "Node id=2 should have 'speed' parameter");
        if (it != atomNode->Parameters.end())
        {
            TEST_ASSERT(it->second.Type == Olympe::ParameterBindingType::Literal,
                        "speed parameter should be Literal");
            if (it->second.Type != Olympe::ParameterBindingType::Literal) { passed = false; }
        }
    }

    delete tmpl;
    ReportTest("TestC_InlineSchemaV3", passed);
}

// ---------------------------------------------------------------------------
// Test d: Invalid JSON (missing child reference) -> nullptr, outErrors filled
// ---------------------------------------------------------------------------

static void TestD_InvalidMissingChild()
{
    std::cout << "Test D: Invalid JSON with missing child reference..." << std::endl;

    // Build JSON where a node references a child that does not exist
    json badJson = json::object();
    badJson["schema_version"] = 2;
    badJson["name"] = "BrokenGraph";

    json dataObj = json::object();
    dataObj["rootNodeId"] = 1;

    json nodesArr = json::array();

    json rootNode = json::object();
    rootNode["id"]   = 1;
    rootNode["name"] = "Root Selector";
    rootNode["type"] = "Selector";
    json rootChildren = json::array();
    rootChildren.push_back(99); // node 99 does not exist
    rootNode["children"] = rootChildren;

    nodesArr.push_back(rootNode);
    dataObj["nodes"] = nodesArr;
    badJson["data"] = dataObj;

    std::vector<std::string> errors;
    Olympe::TaskGraphTemplate* tmpl =
        Olympe::TaskGraphLoader::LoadFromJson(badJson, errors);

    bool passed = true;

    // Must return nullptr because validation should fail (child 99 does not exist)
    TEST_ASSERT(tmpl == nullptr, "LoadFromJson should return nullptr for invalid graph");
    if (tmpl != nullptr)
    {
        passed = false;
        delete tmpl;
    }

    // outErrors must not be empty
    TEST_ASSERT(!errors.empty(), "outErrors should not be empty for invalid graph");
    if (errors.empty()) { passed = false; }

    ReportTest("TestD_InvalidMissingChild", passed);
}

// ---------------------------------------------------------------------------
// Test e: ValidateJson with missing data section
// ---------------------------------------------------------------------------

static void TestE_ValidateJsonMissingData()
{
    std::cout << "Test E: ValidateJson - missing data section..." << std::endl;

    json badJson = json::object();
    badJson["schema_version"] = 2;
    badJson["name"] = "NoDataSection";
    // No "data" field

    std::vector<std::string> errors;
    bool valid = Olympe::TaskGraphLoader::ValidateJson(badJson, errors);

    bool passed = (!valid && !errors.empty());
    TEST_ASSERT(!valid, "ValidateJson should return false for missing data section");
    TEST_ASSERT(!errors.empty(), "outErrors should not be empty");

    ReportTest("TestE_ValidateJsonMissingData", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== TaskGraphLoaderTest ===" << std::endl;

    TestA_LoadGuardV2();
    TestB_LoadNpcWander();
    TestC_InlineSchemaV3();
    TestD_InvalidMissingChild();
    TestE_ValidateJsonMissingData();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
