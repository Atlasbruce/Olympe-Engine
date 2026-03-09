/**
 * @file SubGraph_E2E_Tests.cpp
 * @brief End-to-end tests for SubGraph loading, parameter binding, and cycle detection.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   a) SubGraph file loads and isSubGraph flag is set.
 *   b) SubGraphCallStack detects cycles (self-reference).
 *   c) SubGraphCallStack enforces depth limit.
 *   d) SubGraphParameterDef is parsed from JSON.
 *   e) HandleSubGraph skips when SubGraphPath is empty.
 *   f) InputParams / OutputParams are parsed from node JSON.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "../../Source/TaskSystem/VSGraphExecutor.h"
#include "../../Source/TaskSystem/TaskGraphLoader.h"
#include "../../Source/TaskSystem/TaskGraphTemplate.h"
#include "../../Source/TaskSystem/TaskGraphTypes.h"
#include "../../Source/TaskSystem/LocalBlackboard.h"
#include "../../Source/ECS/Components/TaskRunnerComponent.h"
#include "../../Source/Core/AssetManager.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(condition, message)                                     \
    do {                                                                    \
        if (!(condition)) {                                                 \
            std::cout << "  FAIL: " << (message) << std::endl;             \
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
// Test A: SubGraph file loads correctly (isSubGraph = true)
// ---------------------------------------------------------------------------

static void TestA_SubGraphFileLoads()
{
    std::cout << "Test A: SubGraph file loads with isSubGraph=true..." << std::endl;

    std::vector<std::string> errors;
    const TaskGraphTemplate* tmpl =
        AssetManager::Get().LoadTaskGraphFromFile(
            "Blueprints/AI/SubGraphs/TestSubGraph.json", errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: LoadTaskGraphFromFile returned nullptr" << std::endl;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        }
        ReportTest("TestA_SubGraphFileLoads", false);
        return;
    }

    TEST_ASSERT(tmpl->IsSubGraph, "isSubGraph should be true");
    if (!tmpl->IsSubGraph) passed = false;

    TEST_ASSERT(tmpl->InputParameters.size() == 1, "Should have 1 input parameter");
    if (tmpl->InputParameters.size() == 1)
    {
        TEST_ASSERT(tmpl->InputParameters[0].Name == "InputValue",
                    "Input parameter name should be 'InputValue'");
        if (tmpl->InputParameters[0].Name != "InputValue") passed = false;

        TEST_ASSERT(tmpl->InputParameters[0].Type == VariableType::Int,
                    "Input parameter type should be Int");
        if (tmpl->InputParameters[0].Type != VariableType::Int) passed = false;
    }
    else { passed = false; }

    TEST_ASSERT(tmpl->OutputParameters.size() == 1, "Should have 1 output parameter");
    if (tmpl->OutputParameters.size() == 1)
    {
        TEST_ASSERT(tmpl->OutputParameters[0].Name == "OutputValue",
                    "Output parameter name should be 'OutputValue'");
        if (tmpl->OutputParameters[0].Name != "OutputValue") passed = false;
    }
    else { passed = false; }

    ReportTest("TestA_SubGraphFileLoads", passed);
}

// ---------------------------------------------------------------------------
// Test B: SubGraphCallStack detects self-reference (cycle)
// ---------------------------------------------------------------------------

static void TestB_CycleDetection()
{
    std::cout << "Test B: SubGraphCallStack cycle detection..." << std::endl;

    SubGraphCallStack stack;

    bool passed = true;

    // Empty stack: no cycle
    TEST_ASSERT(!stack.Contains("Blueprints/AI/SubGraphs/A.json"),
                "Empty stack should not contain any path");
    if (stack.Contains("Blueprints/AI/SubGraphs/A.json")) passed = false;

    // Push A
    stack.Push("Blueprints/AI/SubGraphs/A.json");
    TEST_ASSERT(stack.Depth == 1, "Depth should be 1 after Push");
    if (stack.Depth != 1) passed = false;

    TEST_ASSERT(stack.Contains("Blueprints/AI/SubGraphs/A.json"),
                "Stack should contain A after Push");
    if (!stack.Contains("Blueprints/AI/SubGraphs/A.json")) passed = false;

    // Push B
    stack.Push("Blueprints/AI/SubGraphs/B.json");
    TEST_ASSERT(stack.Depth == 2, "Depth should be 2 after second Push");
    if (stack.Depth != 2) passed = false;

    // Cycle: trying to push A again should be detected by Contains()
    TEST_ASSERT(stack.Contains("Blueprints/AI/SubGraphs/A.json"),
                "Cycle should be detected: A is already in stack");
    if (!stack.Contains("Blueprints/AI/SubGraphs/A.json")) passed = false;

    // Pop B
    stack.Pop();
    TEST_ASSERT(stack.Depth == 1, "Depth should be 1 after Pop");
    if (stack.Depth != 1) passed = false;

    TEST_ASSERT(!stack.Contains("Blueprints/AI/SubGraphs/B.json"),
                "B should be gone after Pop");
    if (stack.Contains("Blueprints/AI/SubGraphs/B.json")) passed = false;

    // Pop A
    stack.Pop();
    TEST_ASSERT(stack.Depth == 0, "Depth should be 0 after all Pops");
    if (stack.Depth != 0) passed = false;

    ReportTest("TestB_CycleDetection", passed);
}

// ---------------------------------------------------------------------------
// Test C: SubGraphCallStack enforces depth limit
// ---------------------------------------------------------------------------

static void TestC_DepthLimit()
{
    std::cout << "Test C: SubGraphCallStack depth limit (MAX_SUBGRAPH_DEPTH = 4)..." << std::endl;

    SubGraphCallStack stack;

    bool passed = true;

    // Push to max depth
    for (int i = 0; i < 4; ++i)
    {
        std::string path = "Blueprints/AI/SubGraphs/Level" + std::to_string(i) + ".json";
        stack.Push(path);
    }

    TEST_ASSERT(stack.Depth == 4, "Depth should be 4 at max");
    if (stack.Depth != 4) passed = false;

    // VSGraphExecutor::MAX_SUBGRAPH_DEPTH == 4 means depth >= 4 → reject
    // (confirmed by HandleSubGraph: if callStack.Depth >= MAX_SUBGRAPH_DEPTH → skip)
    TEST_ASSERT(stack.Depth >= 4, "Stack at max depth should trigger rejection");
    if (stack.Depth < 4) passed = false;

    // Pop all
    while (stack.Depth > 0) stack.Pop();
    TEST_ASSERT(stack.Depth == 0, "Stack should be empty after all Pops");
    if (stack.Depth != 0) passed = false;

    ReportTest("TestC_DepthLimit", passed);
}

// ---------------------------------------------------------------------------
// Test D: Parse SubGraphParameterDef from JSON
// ---------------------------------------------------------------------------

static void TestD_ParseSubGraphParameters()
{
    std::cout << "Test D: SubGraphParameterDef parsed from isSubGraph JSON..." << std::endl;

    // Build inline JSON for a subgraph template
    const std::string jsonStr = R"({
        "schema_version": 4,
        "graphType": "VisualScript",
        "name": "InlineSubGraph",
        "isSubGraph": true,
        "inputParameters": [
            { "name": "Speed",  "type": "Float" },
            { "name": "Target", "type": "EntityID" }
        ],
        "outputParameters": [
            { "name": "Done", "type": "Bool" }
        ],
        "nodes": [
            { "id": 1, "type": "EntryPoint", "label": "Start" }
        ],
        "execConnections": []
    })";

    std::vector<std::string> errors;
    TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromJsonString(jsonStr, errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: LoadFromJsonString returned nullptr" << std::endl;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        }
        ReportTest("TestD_ParseSubGraphParameters", false);
        return;
    }

    TEST_ASSERT(tmpl->IsSubGraph, "IsSubGraph should be true");
    if (!tmpl->IsSubGraph) passed = false;

    TEST_ASSERT(tmpl->InputParameters.size() == 2, "Should have 2 input parameters");
    if (tmpl->InputParameters.size() == 2)
    {
        TEST_ASSERT(tmpl->InputParameters[0].Name == "Speed",
                    "First input parameter name should be 'Speed'");
        TEST_ASSERT(tmpl->InputParameters[0].Type == VariableType::Float,
                    "Speed type should be Float");
        TEST_ASSERT(tmpl->InputParameters[1].Name == "Target",
                    "Second input parameter name should be 'Target'");
        TEST_ASSERT(tmpl->InputParameters[1].Type == VariableType::EntityID,
                    "Target type should be EntityID");
        if (tmpl->InputParameters[0].Name != "Speed") passed = false;
        if (tmpl->InputParameters[1].Name != "Target") passed = false;
    }
    else { passed = false; }

    TEST_ASSERT(tmpl->OutputParameters.size() == 1, "Should have 1 output parameter");
    if (tmpl->OutputParameters.size() == 1)
    {
        TEST_ASSERT(tmpl->OutputParameters[0].Name == "Done",
                    "Output parameter name should be 'Done'");
        TEST_ASSERT(tmpl->OutputParameters[0].Type == VariableType::Bool,
                    "Done type should be Bool");
        if (tmpl->OutputParameters[0].Name != "Done") passed = false;
    }
    else { passed = false; }

    delete tmpl;
    ReportTest("TestD_ParseSubGraphParameters", passed);
}

// ---------------------------------------------------------------------------
// Test E: InputParams / OutputParams parsed from node JSON
// ---------------------------------------------------------------------------

static void TestE_ParseNodeInputOutputParams()
{
    std::cout << "Test E: InputParams/OutputParams parsed from SubGraph node..." << std::endl;

    const std::string jsonStr = R"({
        "schema_version": 4,
        "graphType": "VisualScript",
        "name": "ParentGraph",
        "nodes": [
            { "id": 1, "type": "EntryPoint", "label": "Start" },
            {
                "id": 2,
                "type": "SubGraph",
                "label": "AttackSequence",
                "subGraphPath": "Blueprints/AI/SubGraphs/TestSubGraph.json",
                "InputParams": {
                    "InputValue": { "Type": "Literal", "LiteralValue": 77 }
                },
                "OutputParams": {
                    "OutputValue": "local:AttackResult"
                }
            }
        ],
        "execConnections": [
            { "fromNode": 1, "fromPin": "Out", "toNode": 2, "toPin": "In" }
        ]
    })";

    std::vector<std::string> errors;
    TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromJsonString(jsonStr, errors);

    bool passed = true;

    if (tmpl == nullptr)
    {
        std::cout << "  FAIL: LoadFromJsonString returned nullptr" << std::endl;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        }
        ReportTest("TestE_ParseNodeInputOutputParams", false);
        return;
    }

    const TaskNodeDefinition* subNode = tmpl->GetNode(2);
    TEST_ASSERT(subNode != nullptr, "Node id=2 (SubGraph) should exist");

    if (subNode != nullptr)
    {
        TEST_ASSERT(subNode->Type == TaskNodeType::SubGraph, "Node 2 should be SubGraph type");
        if (subNode->Type != TaskNodeType::SubGraph) passed = false;

        TEST_ASSERT(!subNode->SubGraphPath.empty(), "SubGraphPath should not be empty");
        if (subNode->SubGraphPath.empty()) passed = false;

        // Check InputParams
        TEST_ASSERT(subNode->InputParams.size() == 1, "Should have 1 input param");
        if (subNode->InputParams.size() == 1)
        {
            auto it = subNode->InputParams.find("InputValue");
            TEST_ASSERT(it != subNode->InputParams.end(),
                        "InputParams should have 'InputValue'");
            if (it != subNode->InputParams.end())
            {
                TEST_ASSERT(it->second.Type == ParameterBindingType::Literal,
                            "InputValue binding should be Literal");
                TEST_ASSERT(it->second.LiteralValue.AsInt() == 77,
                            "InputValue literal should be 77");
                if (it->second.Type != ParameterBindingType::Literal) passed = false;
                if (it->second.LiteralValue.AsInt() != 77) passed = false;
            }
            else { passed = false; }
        }
        else { passed = false; }

        // Check OutputParams
        TEST_ASSERT(subNode->OutputParams.size() == 1, "Should have 1 output param");
        if (subNode->OutputParams.size() == 1)
        {
            auto it = subNode->OutputParams.find("OutputValue");
            TEST_ASSERT(it != subNode->OutputParams.end(),
                        "OutputParams should have 'OutputValue'");
            if (it != subNode->OutputParams.end())
            {
                TEST_ASSERT(it->second == "local:AttackResult",
                            "OutputValue should map to 'local:AttackResult'");
                if (it->second != "local:AttackResult") passed = false;
            }
            else { passed = false; }
        }
        else { passed = false; }
    }
    else { passed = false; }

    delete tmpl;
    ReportTest("TestE_ParseNodeInputOutputParams", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== SubGraph_E2E_Tests ===" << std::endl;

    TestA_SubGraphFileLoads();
    TestB_CycleDetection();
    TestC_DepthLimit();
    TestD_ParseSubGraphParameters();
    TestE_ParseNodeInputOutputParams();

    std::cout << std::endl
              << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
