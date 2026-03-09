/**
 * @file VisualScriptEditorTest.cpp
 * @brief Unit tests for VS graph creation / modification / serialization (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests verify the core VS graph logic (in-memory template build + JSON v4
 * serialization) without requiring ImGui or ImNodes at link time.
 *
 * Tests cover:
 *   1.  VSTemplate_EmptyGraph_ValidateReturnsFalse
 *   2.  VSTemplate_AddEntryPoint_ValidatePasses
 *   3.  VSTemplate_ExecConnection_StoredCorrectly
 *   4.  VSTemplate_DataConnection_StoredCorrectly
 *   5.  VSTemplate_BlackboardEntry_AddedProperly
 *   6.  VSTemplate_SaveToFile_WritesValidJSON
 *   7.  VSTemplate_LoadFromFile_RestoresGraph
 *   8.  VSTemplate_RemoveNode_ConnectionsCleaned
 *
 * All operations are performed on TaskGraphTemplate in-memory (no UI).
 *
 * C++14 compliant — no C++17/20 features.
 */

#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "TaskSystem/TaskGraphLoader.h"
#include "json_helper.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeDefinition;
using Olympe::TaskNodeType;
using Olympe::ExecPinConnection;
using Olympe::DataPinConnection;
using Olympe::BlackboardEntry;
using Olympe::VariableType;
using Olympe::NODE_INDEX_NONE;

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

// ---------------------------------------------------------------------------
// Helper: build a minimal valid VS template with EntryPoint
// ---------------------------------------------------------------------------

static TaskGraphTemplate BuildMinimalVSTemplate()
{
    TaskGraphTemplate tmpl;
    tmpl.Name      = "TestGraph";
    tmpl.GraphType = "VisualScript";

    TaskNodeDefinition ep;
    ep.NodeID   = 1;
    ep.NodeName = "Entry";
    ep.Type     = TaskNodeType::EntryPoint;
    tmpl.Nodes.push_back(ep);

    tmpl.EntryPointID = 1;
    tmpl.RootNodeID   = 1;
    tmpl.BuildLookupCache();
    return tmpl;
}

// ---------------------------------------------------------------------------
// Helper: serialize template to JSON v4 file (minimal implementation)
// ---------------------------------------------------------------------------

static bool SaveTemplateToFile(const TaskGraphTemplate& tmpl, const std::string& path)
{
    json root;
    root["schema_version"] = 4;
    root["name"]           = tmpl.Name;
    root["graphType"]      = tmpl.GraphType.empty() ? "VisualScript" : tmpl.GraphType;

    // Blackboard
    json bbArray = json::array();
    for (size_t i = 0; i < tmpl.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = tmpl.Blackboard[i];
        json e;
        e["key"]      = entry.Key;
        e["isGlobal"] = entry.IsGlobal;
        switch (entry.Type)
        {
            case VariableType::Float:  e["type"] = "Float";  e["value"] = entry.Default.AsFloat();   break;
            case VariableType::Int:    e["type"] = "Int";    e["value"] = entry.Default.AsInt();     break;
            case VariableType::Bool:   e["type"] = "Bool";   e["value"] = entry.Default.AsBool();   break;
            case VariableType::String: e["type"] = "String"; e["value"] = entry.Default.AsString(); break;
            default:                  e["type"] = "None";   e["value"] = nullptr;            break;
        }
        bbArray.push_back(e);
    }
    root["blackboard"] = bbArray;

    // Nodes
    json nodesArray = json::array();
    for (size_t i = 0; i < tmpl.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& def = tmpl.Nodes[i];
        json n;
        n["id"]    = def.NodeID;
        n["label"] = def.NodeName;

        switch (def.Type)
        {
            case TaskNodeType::EntryPoint:  n["type"] = "EntryPoint";  break;
            case TaskNodeType::AtomicTask:  n["type"] = "AtomicTask";
                                            n["taskType"] = def.AtomicTaskID; break;
            case TaskNodeType::Branch:      n["type"] = "Branch";      break;
            case TaskNodeType::Delay:       n["type"] = "Delay";
                                            n["delaySeconds"] = def.DelaySeconds; break;
            case TaskNodeType::SubGraph:    n["type"] = "SubGraph";
                                            n["subGraphPath"] = def.SubGraphPath; break;
            case TaskNodeType::GetBBValue:  n["type"] = "GetBBValue";
                                            n["bbKey"] = def.BBKey; break;
            case TaskNodeType::SetBBValue:  n["type"] = "SetBBValue";
                                            n["bbKey"] = def.BBKey; break;
            case TaskNodeType::VSSequence:  n["type"] = "Sequence";    break;
            case TaskNodeType::While:       n["type"] = "While";       break;
            case TaskNodeType::DoOnce:      n["type"] = "DoOnce";      break;
            case TaskNodeType::MathOp:      n["type"] = "MathOp";
                                            n["mathOp"] = def.MathOperator; break;
            default:                        n["type"] = "Unknown";     break;
        }
        nodesArray.push_back(n);
    }
    root["nodes"] = nodesArray;

    // Exec connections
    json execArray = json::array();
    for (size_t i = 0; i < tmpl.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& conn = tmpl.ExecConnections[i];
        json c;
        c["fromNode"] = conn.SourceNodeID;
        c["fromPin"]  = conn.SourcePinName;
        c["toNode"]   = conn.TargetNodeID;
        c["toPin"]    = conn.TargetPinName;
        execArray.push_back(c);
    }
    root["execConnections"] = execArray;

    // Data connections
    json dataArray = json::array();
    for (size_t i = 0; i < tmpl.DataConnections.size(); ++i)
    {
        const DataPinConnection& conn = tmpl.DataConnections[i];
        json c;
        c["fromNode"] = conn.SourceNodeID;
        c["fromPin"]  = conn.SourcePinName;
        c["toNode"]   = conn.TargetNodeID;
        c["toPin"]    = conn.TargetPinName;
        dataArray.push_back(c);
    }
    root["dataConnections"] = dataArray;

    std::ofstream ofs(path);
    if (!ofs.is_open())
        return false;
    ofs << root.dump(2);
    ofs.close();
    return true;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void Test_EmptyGraph_ValidateReturnsFalse()
{
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl;
    // Empty template has no nodes, so Validate() should return false
    TEST_ASSERT(!tmpl.Validate(), "Empty template should fail Validate()");

    ReportTest("VSTemplate_EmptyGraph_ValidateReturnsFalse", s_failCount == prevFail);
}

static void Test_AddEntryPoint_ValidatePasses()
{
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl = BuildMinimalVSTemplate();
    TEST_ASSERT(tmpl.Validate(), "Minimal VS template should pass Validate()");
    TEST_ASSERT(tmpl.EntryPointID == 1, "EntryPointID should be 1");
    TEST_ASSERT(!tmpl.Nodes.empty(), "Nodes should not be empty");
    TEST_ASSERT(tmpl.GetNode(1) != nullptr, "Node 1 should be findable in lookup cache");
    TEST_ASSERT(tmpl.GraphType == "VisualScript", "GraphType should be VisualScript");

    ReportTest("VSTemplate_AddEntryPoint_ValidatePasses", s_failCount == prevFail);
}

static void Test_ExecConnection_StoredCorrectly()
{
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl = BuildMinimalVSTemplate();

    // Add a second node
    TaskNodeDefinition task;
    task.NodeID   = 2;
    task.NodeName = "WaitNode";
    task.Type     = TaskNodeType::Delay;
    task.DelaySeconds = 1.5f;
    tmpl.Nodes.push_back(task);
    tmpl.BuildLookupCache();

    // Add exec connection
    ExecPinConnection conn;
    conn.SourceNodeID  = 1;
    conn.SourcePinName = "Out";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    tmpl.ExecConnections.push_back(conn);

    TEST_ASSERT(tmpl.ExecConnections.size() == 1,
                "Should have 1 exec connection");
    TEST_ASSERT(tmpl.ExecConnections[0].SourceNodeID == 1,
                "Source should be node 1");
    TEST_ASSERT(tmpl.ExecConnections[0].TargetNodeID == 2,
                "Target should be node 2");
    TEST_ASSERT(tmpl.ExecConnections[0].SourcePinName == "Out",
                "Source pin should be Out");
    TEST_ASSERT(tmpl.ExecConnections[0].TargetPinName == "In",
                "Target pin should be In");

    ReportTest("VSTemplate_ExecConnection_StoredCorrectly", s_failCount == prevFail);
}

static void Test_DataConnection_StoredCorrectly()
{
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl = BuildMinimalVSTemplate();

    TaskNodeDefinition bbGet;
    bbGet.NodeID  = 2;
    bbGet.NodeName = "GetHealth";
    bbGet.Type    = TaskNodeType::GetBBValue;
    bbGet.BBKey   = "local:Health";
    tmpl.Nodes.push_back(bbGet);

    TaskNodeDefinition bbSet;
    bbSet.NodeID  = 3;
    bbSet.NodeName = "SetHealth";
    bbSet.Type    = TaskNodeType::SetBBValue;
    bbSet.BBKey   = "local:Health";
    tmpl.Nodes.push_back(bbSet);

    tmpl.BuildLookupCache();

    DataPinConnection dconn;
    dconn.SourceNodeID  = 2;
    dconn.SourcePinName = "value";
    dconn.TargetNodeID  = 3;
    dconn.TargetPinName = "input";
    tmpl.DataConnections.push_back(dconn);

    TEST_ASSERT(tmpl.DataConnections.size() == 1,
                "Should have 1 data connection");
    TEST_ASSERT(tmpl.DataConnections[0].SourceNodeID == 2,
                "Source node should be 2");
    TEST_ASSERT(tmpl.DataConnections[0].TargetNodeID == 3,
                "Target node should be 3");
    TEST_ASSERT(tmpl.DataConnections[0].SourcePinName == "value",
                "Source pin should be value");

    ReportTest("VSTemplate_DataConnection_StoredCorrectly", s_failCount == prevFail);
}

static void Test_BlackboardEntry_AddedProperly()
{
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl = BuildMinimalVSTemplate();

    BlackboardEntry entry;
    entry.Key      = "Health";
    entry.Type     = VariableType::Float;
    entry.IsGlobal = false;
    entry.Default  = Olympe::TaskValue(100.0f);
    tmpl.Blackboard.push_back(entry);

    TEST_ASSERT(tmpl.Blackboard.size() == 1, "Should have 1 blackboard entry");
    TEST_ASSERT(tmpl.Blackboard[0].Key == "Health", "Key should be Health");
    TEST_ASSERT(tmpl.Blackboard[0].Type == VariableType::Float, "Type should be Float");
    TEST_ASSERT(!tmpl.Blackboard[0].IsGlobal, "Should be local (not global)");
    TEST_ASSERT(tmpl.Blackboard[0].Default.AsFloat() == 100.0f, "Default value should be 100.0f");

    ReportTest("VSTemplate_BlackboardEntry_AddedProperly", s_failCount == prevFail);
}

static void Test_SaveToFile_WritesValidJSON()
{
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl = BuildMinimalVSTemplate();

    TaskNodeDefinition task;
    task.NodeID       = 2;
    task.NodeName     = "WaitHalf";
    task.Type         = TaskNodeType::Delay;
    task.DelaySeconds = 0.5f;
    tmpl.Nodes.push_back(task);

    ExecPinConnection conn;
    conn.SourceNodeID  = 1;
    conn.SourcePinName = "Out";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    tmpl.ExecConnections.push_back(conn);

    const std::string path = "/tmp/vs_template_test_save.json";
    bool saved = SaveTemplateToFile(tmpl, path);
    TEST_ASSERT(saved, "SaveTemplateToFile should return true");

    // Verify file content
    std::ifstream ifs(path);
    TEST_ASSERT(ifs.is_open(), "Saved file should exist");

    if (ifs.is_open())
    {
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        TEST_ASSERT(content.find("schema_version") != std::string::npos,
                    "JSON should have schema_version");
        TEST_ASSERT(content.find("\"4\"") != std::string::npos ||
                    content.find(": 4") != std::string::npos ||
                    content.find(":4") != std::string::npos,
                    "schema_version should be 4");
        TEST_ASSERT(content.find("VisualScript") != std::string::npos,
                    "graphType should be VisualScript");
        TEST_ASSERT(content.find("EntryPoint") != std::string::npos,
                    "JSON should contain EntryPoint node");
        TEST_ASSERT(content.find("execConnections") != std::string::npos,
                    "JSON should have execConnections array");
    }

    ReportTest("VSTemplate_SaveToFile_WritesValidJSON", s_failCount == prevFail);
}

static void Test_LoadFromFile_RestoresGraph()
{
    int prevFail = s_failCount;

    // First save a template
    TaskGraphTemplate tmpl = BuildMinimalVSTemplate();

    TaskNodeDefinition task;
    task.NodeID       = 2;
    task.NodeName     = "AtomicNode";
    task.Type         = TaskNodeType::AtomicTask;
    task.AtomicTaskID = "Task_Wait";
    tmpl.Nodes.push_back(task);
    tmpl.BuildLookupCache();

    ExecPinConnection conn;
    conn.SourceNodeID  = 1;
    conn.SourcePinName = "Out";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    tmpl.ExecConnections.push_back(conn);

    const std::string path = "/tmp/vs_template_test_load.json";
    bool saved = SaveTemplateToFile(tmpl, path);
    TEST_ASSERT(saved, "Should save successfully");

    // Now load it back
    std::vector<std::string> errors;
    TaskGraphTemplate* loaded = Olympe::TaskGraphLoader::LoadFromFile(path, errors);

    TEST_ASSERT(loaded != nullptr, "LoadFromFile should succeed");

    if (loaded != nullptr)
    {
        TEST_ASSERT(loaded->Nodes.size() == 2,
                    "Loaded template should have 2 nodes");
        TEST_ASSERT(loaded->ExecConnections.size() == 1,
                    "Loaded template should have 1 exec connection");
        TEST_ASSERT(loaded->ExecConnections[0].SourceNodeID == 1,
                    "Loaded exec connection source should be 1");
        TEST_ASSERT(loaded->ExecConnections[0].TargetNodeID == 2,
                    "Loaded exec connection target should be 2");

        delete loaded;
    }

    ReportTest("VSTemplate_LoadFromFile_RestoresGraph", s_failCount == prevFail);
}

static void Test_RemoveNode_ConnectionsCleaned()
{
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl = BuildMinimalVSTemplate();

    TaskNodeDefinition nodeB;
    nodeB.NodeID   = 2;
    nodeB.NodeName = "NodeB";
    nodeB.Type     = TaskNodeType::AtomicTask;
    tmpl.Nodes.push_back(nodeB);

    TaskNodeDefinition nodeC;
    nodeC.NodeID   = 3;
    nodeC.NodeName = "NodeC";
    nodeC.Type     = TaskNodeType::Delay;
    tmpl.Nodes.push_back(nodeC);

    tmpl.BuildLookupCache();

    // Connect 1→2 and 2→3
    ExecPinConnection c1;
    c1.SourceNodeID = 1; c1.SourcePinName = "Out";
    c1.TargetNodeID = 2; c1.TargetPinName = "In";
    tmpl.ExecConnections.push_back(c1);

    ExecPinConnection c2;
    c2.SourceNodeID = 2; c2.SourcePinName = "Completed";
    c2.TargetNodeID = 3; c2.TargetPinName = "In";
    tmpl.ExecConnections.push_back(c2);

    TEST_ASSERT(tmpl.ExecConnections.size() == 2, "Should have 2 connections before remove");

    // Remove node 2 and its connections (simulate editor behavior)
    tmpl.Nodes.erase(
        std::remove_if(tmpl.Nodes.begin(), tmpl.Nodes.end(),
                       [](const TaskNodeDefinition& n) { return n.NodeID == 2; }),
        tmpl.Nodes.end());

    tmpl.ExecConnections.erase(
        std::remove_if(tmpl.ExecConnections.begin(), tmpl.ExecConnections.end(),
                       [](const ExecPinConnection& c) {
                           return c.SourceNodeID == 2 || c.TargetNodeID == 2;
                       }),
        tmpl.ExecConnections.end());

    tmpl.BuildLookupCache();

    TEST_ASSERT(tmpl.Nodes.size() == 2,
                "Should have 2 nodes (1 and 3) after removing node 2");
    TEST_ASSERT(tmpl.ExecConnections.empty(),
                "Both connections referencing node 2 should be removed");
    TEST_ASSERT(tmpl.GetNode(2) == nullptr,
                "Node 2 should not be findable after removal");

    ReportTest("VSTemplate_RemoveNode_ConnectionsCleaned", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "======================================" << std::endl;
    std::cout << " VS Graph Template/Serialization Tests" << std::endl;
    std::cout << "======================================" << std::endl;

    Test_EmptyGraph_ValidateReturnsFalse();
    Test_AddEntryPoint_ValidatePasses();
    Test_ExecConnection_StoredCorrectly();
    Test_DataConnection_StoredCorrectly();
    Test_BlackboardEntry_AddedProperly();
    Test_SaveToFile_WritesValidJSON();
    Test_LoadFromFile_RestoresGraph();
    Test_RemoveNode_ConnectionsCleaned();

    std::cout << "======================================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
