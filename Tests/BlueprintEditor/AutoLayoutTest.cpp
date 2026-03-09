/**
 * @file AutoLayoutTest.cpp
 * @brief Unit tests for GraphAutoLayout (Phase 6 ATS VS editor).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   1.  AutoLayout_EntryPoint_IsInLayer0
 *   2.  AutoLayout_LayersOrderedCorrectly
 *   3.  AutoLayout_PositionsAssigned_NonZero
 *   4.  AutoLayout_OrphanNodes_Placed
 *   5.  AutoLayout_EmptyGraph_NoCrash
 *
 * No SDL3, ImGui, or Editor dependency.
 * C++14 compliant — no C++17/20 features.
 */

#include "BlueprintEditor/GraphAutoLayout.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <algorithm>

using Olympe::TaskGraphTemplate;
using Olympe::TaskNodeDefinition;
using Olympe::TaskNodeType;
using Olympe::ExecPinConnection;
using Olympe::GraphAutoLayout;
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

static TaskNodeDefinition MakeNode(int id, const std::string& name,
                                    TaskNodeType type = TaskNodeType::AtomicTask)
{
    TaskNodeDefinition def;
    def.NodeID   = id;
    def.NodeName = name;
    def.Type     = type;
    return def;
}

static ExecPinConnection MakeConn(int src, int dst)
{
    ExecPinConnection ec;
    ec.SourceNodeID  = src;
    ec.SourcePinName = "Out";
    ec.TargetNodeID  = dst;
    ec.TargetPinName = "In";
    return ec;
}

/// Returns the layer index of a node in the built layers (-1 if not found).
static int FindLayer(const std::vector<GraphAutoLayout::Layer>& layers, int nodeID)
{
    for (int li = 0; li < static_cast<int>(layers.size()); ++li)
    {
        for (int id : layers[static_cast<std::size_t>(li)].NodeIDs)
        {
            if (id == nodeID) return li;
        }
    }
    return -1;
}

/// Returns the float position stored in __posX / __posY of a node (or 0 if absent).
static float GetPosX(const TaskNodeDefinition& n)
{
    auto it = n.Parameters.find("__posX");
    if (it == n.Parameters.end()) return 0.0f;
    return it->second.LiteralValue.AsFloat();
}

static float GetPosY(const TaskNodeDefinition& n)
{
    auto it = n.Parameters.find("__posY");
    if (it == n.Parameters.end()) return 0.0f;
    return it->second.LiteralValue.AsFloat();
}

// ---------------------------------------------------------------------------
// Test 1: EntryPoint is in layer 0
// ---------------------------------------------------------------------------

static void Test1_EntryPoint_IsInLayer0()
{
    int prevFail = s_failCount;

    // Build: EntryPoint(0) → Branch(1) → Action(2)
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(0, "Start",  TaskNodeType::EntryPoint));
    graph.Nodes.push_back(MakeNode(1, "Branch", TaskNodeType::Branch));
    graph.Nodes.push_back(MakeNode(2, "Action", TaskNodeType::AtomicTask));
    graph.ExecConnections.push_back(MakeConn(0, 1));
    graph.ExecConnections.push_back(MakeConn(1, 2));
    graph.EntryPointID = 0;
    graph.BuildLookupCache();

    auto layers = GraphAutoLayout::BuildLayers(graph);

    TEST_ASSERT(!layers.empty(), "Layers should not be empty");
    int layer0 = FindLayer(layers, 0);
    TEST_ASSERT(layer0 == 0, "EntryPoint (node 0) should be in layer 0");
    int layer1 = FindLayer(layers, 1);
    TEST_ASSERT(layer1 == 1, "Branch (node 1) should be in layer 1");
    int layer2 = FindLayer(layers, 2);
    TEST_ASSERT(layer2 == 2, "Action (node 2) should be in layer 2");

    ReportTest("Test1_EntryPoint_IsInLayer0", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2: Layers ordered correctly (child always in higher layer than parent)
// ---------------------------------------------------------------------------

static void Test2_LayersOrderedCorrectly()
{
    int prevFail = s_failCount;

    // Diamond: EntryPoint(0) → A(1), A(1) → B(2), A(1) → C(3), B(2) → D(4), C(3) → D(4)
    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(0, "Entry", TaskNodeType::EntryPoint));
    graph.Nodes.push_back(MakeNode(1, "A"));
    graph.Nodes.push_back(MakeNode(2, "B"));
    graph.Nodes.push_back(MakeNode(3, "C"));
    graph.Nodes.push_back(MakeNode(4, "D"));
    graph.ExecConnections.push_back(MakeConn(0, 1));
    graph.ExecConnections.push_back(MakeConn(1, 2));
    graph.ExecConnections.push_back(MakeConn(1, 3));
    graph.ExecConnections.push_back(MakeConn(2, 4));
    graph.ExecConnections.push_back(MakeConn(3, 4));
    graph.EntryPointID = 0;
    graph.BuildLookupCache();

    auto layers = GraphAutoLayout::BuildLayers(graph);

    int l0 = FindLayer(layers, 0);
    int l1 = FindLayer(layers, 1);
    int l2 = FindLayer(layers, 2);
    int l3 = FindLayer(layers, 3);
    int l4 = FindLayer(layers, 4);

    TEST_ASSERT(l0 == 0, "Entry in layer 0");
    TEST_ASSERT(l1 == 1, "A in layer 1");
    TEST_ASSERT(l2 == 2, "B in layer 2");
    TEST_ASSERT(l3 == 2, "C in layer 2");
    TEST_ASSERT(l4 == 3, "D in layer 3");
    TEST_ASSERT(l2 > l1, "B layer > A layer");
    TEST_ASSERT(l3 > l1, "C layer > A layer");
    TEST_ASSERT(l4 > l2, "D layer > B layer");
    TEST_ASSERT(l4 > l3, "D layer > C layer");

    ReportTest("Test2_LayersOrderedCorrectly", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: AssignPositions gives each node a non-trivial position
// ---------------------------------------------------------------------------

static void Test3_PositionsAssigned_NonZero()
{
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(0, "Entry", TaskNodeType::EntryPoint));
    graph.Nodes.push_back(MakeNode(1, "A"));
    graph.Nodes.push_back(MakeNode(2, "B"));
    graph.ExecConnections.push_back(MakeConn(0, 1));
    graph.ExecConnections.push_back(MakeConn(1, 2));
    graph.EntryPointID = 0;
    graph.BuildLookupCache();

    GraphAutoLayout::ApplyHierarchicalLayout(graph);

    // Every node should have a __posX >= ORIGIN_X
    bool allHavePos = true;
    for (const auto& n : graph.Nodes)
    {
        if (n.Parameters.find("__posX") == n.Parameters.end())
            allHavePos = false;
        if (n.Parameters.find("__posY") == n.Parameters.end())
            allHavePos = false;
    }
    TEST_ASSERT(allHavePos, "All nodes should have __posX and __posY parameters");

    // EntryPoint should be at ORIGIN_X
    const auto& ep = graph.Nodes[0];
    float epX = GetPosX(ep);
    TEST_ASSERT(epX > GraphAutoLayout::ORIGIN_X - 1.f &&
                epX < GraphAutoLayout::ORIGIN_X + 1.f,
                "EntryPoint x should equal ORIGIN_X");

    // Node 2 should be further right than node 1
    float x1 = 0.0f, x2 = 0.0f;
    for (const auto& n : graph.Nodes)
    {
        if (n.NodeID == 1) x1 = GetPosX(n);
        if (n.NodeID == 2) x2 = GetPosX(n);
    }
    TEST_ASSERT(x2 > x1, "Node 2 should be further right than node 1");

    ReportTest("Test3_PositionsAssigned_NonZero", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: Orphan nodes (not reachable from entry) get placed
// ---------------------------------------------------------------------------

static void Test4_OrphanNodes_Placed()
{
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    graph.Nodes.push_back(MakeNode(0, "Entry", TaskNodeType::EntryPoint));
    graph.Nodes.push_back(MakeNode(1, "Connected"));
    graph.Nodes.push_back(MakeNode(99, "Orphan")); // not connected
    graph.ExecConnections.push_back(MakeConn(0, 1));
    graph.EntryPointID = 0;
    graph.BuildLookupCache();

    auto layers = GraphAutoLayout::BuildLayers(graph);

    // Orphan should appear in some layer
    int orphanLayer = FindLayer(layers, 99);
    TEST_ASSERT(orphanLayer >= 0, "Orphan node 99 should be assigned to a layer");

    // All layers combined should contain all 3 nodes
    int total = 0;
    for (const auto& l : layers) total += static_cast<int>(l.NodeIDs.size());
    TEST_ASSERT(total == 3, "All 3 nodes should appear in layers");

    ReportTest("Test4_OrphanNodes_Placed", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: Empty graph — no crash
// ---------------------------------------------------------------------------

static void Test5_EmptyGraph_NoCrash()
{
    int prevFail = s_failCount;

    TaskGraphTemplate emptyGraph;

    // Should not throw or crash
    GraphAutoLayout::ApplyHierarchicalLayout(emptyGraph);
    auto layers = GraphAutoLayout::BuildLayers(emptyGraph);

    TEST_ASSERT(emptyGraph.Nodes.empty(), "Empty graph stays empty after layout");
    TEST_ASSERT(layers.empty(), "Empty graph produces empty layers");

    ReportTest("Test5_EmptyGraph_NoCrash", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=============================" << std::endl;
    std::cout << "   GraphAutoLayout Unit Tests " << std::endl;
    std::cout << "=============================" << std::endl;

    Test1_EntryPoint_IsInLayer0();
    Test2_LayersOrderedCorrectly();
    Test3_PositionsAssigned_NonZero();
    Test4_OrphanNodes_Placed();
    Test5_EmptyGraph_NoCrash();

    std::cout << "=============================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
