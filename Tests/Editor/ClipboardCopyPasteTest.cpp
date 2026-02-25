/**
 * @file ClipboardCopyPasteTest.cpp
 * @brief Unit tests for NodeGraph CRUD operations and parameter round-trip
 *        that underlie the clipboard copy/paste feature.
 * @author Olympe Engine
 * @date 2026-02-25
 *
 * @details
 * Tests cover:
 *   a) NodeGraph::CreateNode() creates a node with the expected type and ID.
 *   b) NodeGraph::GetNode() returns the correct node after creation.
 *   c) Node parameters (key/value) are preserved after SetNodeParameter /
 *      GetNodeParameter.
 *   d) Multiple nodes can be created; each receives a unique ID.
 *   e) NodeGraph::LinkNodes() creates a link that is returned by GetAllLinks().
 *   f) NodeGraph::DeleteNode() removes the node; GetNode returns nullptr.
 *   g) NodeGraph::ToJson() produces valid JSON containing node data, and
 *      NodeGraph::FromJson() round-trips back to equivalent state.
 *   h) Simulated paste: creating nodes at offset positions preserves the
 *      relative layout (dx/dy offset round-trip).
 *
 * No SDL3 or ImGui dependency.  Only BTNodeGraphManager is used.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "BlueprintEditor/BTNodeGraphManager.h"
#include "../Source/third_party/nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(condition, message)                              \
    do {                                                             \
        if (!(condition)) {                                          \
            std::cout << "  FAIL: " << (message) << std::endl;      \
            ++s_failCount;                                           \
        }                                                            \
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

static bool FloatNear(float a, float b, float eps = 0.001f)
{
    float diff = a - b;
    if (diff < 0.0f) diff = -diff;
    return diff < eps;
}

// ---------------------------------------------------------------------------
// Test A: CreateNode returns valid ID and GetNode returns correct node
// ---------------------------------------------------------------------------

static void TestA_CreateAndGet()
{
    std::cout << "Test A: CreateNode / GetNode..." << std::endl;

    bool passed = true;

    Olympe::NodeGraph graph;
    int id = graph.CreateNode(Olympe::NodeType::BT_Action, 100.0f, 200.0f, "TestAction");

    TEST_ASSERT(id > 0, "CreateNode should return positive ID");
    if (id <= 0) { passed = false; }

    Olympe::GraphNode* node = graph.GetNode(id);
    TEST_ASSERT(node != nullptr, "GetNode should return non-null");
    if (node == nullptr) { passed = false; goto done; }

    TEST_ASSERT(node->type == Olympe::NodeType::BT_Action, "Node type mismatch");
    if (node->type != Olympe::NodeType::BT_Action) { passed = false; }

    TEST_ASSERT(node->name == "TestAction", "Node name mismatch");
    if (node->name != "TestAction") { passed = false; }

    TEST_ASSERT(FloatNear(node->posX, 100.0f), "posX mismatch");
    TEST_ASSERT(FloatNear(node->posY, 200.0f), "posY mismatch");

    done:
    ReportTest("TestA_CreateAndGet", passed);
}

// ---------------------------------------------------------------------------
// Test B: Parameters are preserved
// ---------------------------------------------------------------------------

static void TestB_ParameterRoundTrip()
{
    std::cout << "Test B: Parameter round-trip..." << std::endl;

    bool passed = true;

    Olympe::NodeGraph graph;
    int id = graph.CreateNode(Olympe::NodeType::BT_Action, 0.0f, 0.0f, "MoveNode");

    bool ok = graph.SetNodeParameter(id, "speed", "5.0");
    TEST_ASSERT(ok, "SetNodeParameter should succeed");
    if (!ok) { passed = false; }

    graph.SetNodeParameter(id, "target", "EntityA");
    graph.SetNodeParameter(id, "duration", "2.5");

    std::string speed    = graph.GetNodeParameter(id, "speed");
    std::string target   = graph.GetNodeParameter(id, "target");
    std::string duration = graph.GetNodeParameter(id, "duration");

    TEST_ASSERT(speed    == "5.0",     "speed parameter mismatch");
    TEST_ASSERT(target   == "EntityA", "target parameter mismatch");
    TEST_ASSERT(duration == "2.5",     "duration parameter mismatch");

    if (speed != "5.0" || target != "EntityA" || duration != "2.5") { passed = false; }

    ReportTest("TestB_ParameterRoundTrip", passed);
}

// ---------------------------------------------------------------------------
// Test C: Multiple nodes each get unique IDs
// ---------------------------------------------------------------------------

static void TestC_UniqueIDs()
{
    std::cout << "Test C: Unique node IDs..." << std::endl;

    bool passed = true;

    Olympe::NodeGraph graph;

    int id1 = graph.CreateNode(Olympe::NodeType::BT_Sequence,  0.0f, 0.0f,   "Seq");
    int id2 = graph.CreateNode(Olympe::NodeType::BT_Action,   200.0f, 0.0f,  "Act1");
    int id3 = graph.CreateNode(Olympe::NodeType::BT_Condition, 400.0f, 0.0f, "Cond");

    TEST_ASSERT(id1 != id2, "id1 and id2 must differ");
    TEST_ASSERT(id2 != id3, "id2 and id3 must differ");
    TEST_ASSERT(id1 != id3, "id1 and id3 must differ");

    if (id1 == id2 || id2 == id3 || id1 == id3) { passed = false; }

    auto allNodes = graph.GetAllNodes();
    TEST_ASSERT(allNodes.size() == 3, "GetAllNodes should return 3 nodes");
    if (allNodes.size() != 3) { passed = false; }

    ReportTest("TestC_UniqueIDs", passed);
}

// ---------------------------------------------------------------------------
// Test D: LinkNodes creates a link returned by GetAllLinks
// ---------------------------------------------------------------------------

static void TestD_LinkNodes()
{
    std::cout << "Test D: LinkNodes / GetAllLinks..." << std::endl;

    bool passed = true;

    Olympe::NodeGraph graph;
    int seqId = graph.CreateNode(Olympe::NodeType::BT_Sequence, 0.0f, 0.0f,    "Seq");
    int actId = graph.CreateNode(Olympe::NodeType::BT_Action,  200.0f, 100.0f, "Act");

    bool linked = graph.LinkNodes(seqId, actId);
    TEST_ASSERT(linked, "LinkNodes should succeed");
    if (!linked) { passed = false; }

    auto links = graph.GetAllLinks();
    TEST_ASSERT(!links.empty(), "GetAllLinks should be non-empty after link");
    if (links.empty()) { passed = false; goto done; }

    {
        bool found = false;
        for (const auto& link : links)
        {
            if (link.fromNode == seqId && link.toNode == actId)
            {
                found = true;
                break;
            }
        }
        TEST_ASSERT(found, "Link from seq to act not found in GetAllLinks()");
        if (!found) { passed = false; }
    }

    done:
    ReportTest("TestD_LinkNodes", passed);
}

// ---------------------------------------------------------------------------
// Test E: DeleteNode removes the node
// ---------------------------------------------------------------------------

static void TestE_DeleteNode()
{
    std::cout << "Test E: DeleteNode removes node..." << std::endl;

    bool passed = true;

    Olympe::NodeGraph graph;
    int id = graph.CreateNode(Olympe::NodeType::BT_Action, 0.0f, 0.0f, "TmpNode");

    TEST_ASSERT(graph.GetNode(id) != nullptr, "Node should exist before delete");

    bool deleted = graph.DeleteNode(id);
    TEST_ASSERT(deleted, "DeleteNode should return true");
    if (!deleted) { passed = false; }

    TEST_ASSERT(graph.GetNode(id) == nullptr, "GetNode should return nullptr after delete");
    if (graph.GetNode(id) != nullptr) { passed = false; }

    ReportTest("TestE_DeleteNode", passed);
}

// ---------------------------------------------------------------------------
// Test F: ToJson / FromJson round-trip preserves nodes and parameters
// ---------------------------------------------------------------------------

static void TestF_JsonRoundTrip()
{
    std::cout << "Test F: JSON round-trip..." << std::endl;

    bool passed = true;

    Olympe::NodeGraph original;
    original.name = "TestGraph";
    original.type = "BehaviorTree";

    int id1 = original.CreateNode(Olympe::NodeType::BT_Sequence, 10.0f, 20.0f, "Root");
    int id2 = original.CreateNode(Olympe::NodeType::BT_Action,  110.0f, 20.0f, "Move");

    original.SetNodeParameter(id2, "speed",  "3.0");
    original.SetNodeParameter(id2, "target", "Player");
    original.LinkNodes(id1, id2);
    original.rootNodeId = id1;

    json j = original.ToJson();

    TEST_ASSERT(j.is_object(), "ToJson should return an object");
    if (!j.is_object()) { passed = false; goto done; }

    {
        Olympe::NodeGraph restored = Olympe::NodeGraph::FromJson(j);

        TEST_ASSERT(restored.name == "TestGraph", "name not preserved");
        if (restored.name != "TestGraph") { passed = false; }

        auto nodes = restored.GetAllNodes();
        TEST_ASSERT(nodes.size() == 2, "Node count should be 2 after round-trip");
        if (nodes.size() != 2) { passed = false; goto done; }

        // Find id2 equivalent by name
        Olympe::GraphNode* moveNode = nullptr;
        for (auto* n : nodes)
        {
            if (n->name == "Move")
            {
                moveNode = n;
                break;
            }
        }
        TEST_ASSERT(moveNode != nullptr, "Move node not found after round-trip");
        if (moveNode == nullptr) { passed = false; goto done; }

        std::string speedParam = restored.GetNodeParameter(moveNode->id, "speed");
        TEST_ASSERT(speedParam == "3.0", "speed parameter not preserved in round-trip");
        if (speedParam != "3.0") { passed = false; }

        auto links = restored.GetAllLinks();
        TEST_ASSERT(!links.empty(), "Links should be preserved in round-trip");
        if (links.empty()) { passed = false; }
    }

    done:
    ReportTest("TestF_JsonRoundTrip", passed);
}

// ---------------------------------------------------------------------------
// Test G: Simulated paste — nodes created at offset positions
// ---------------------------------------------------------------------------

static void TestG_PasteAtOffset()
{
    std::cout << "Test G: Simulated paste at mouse offset..." << std::endl;

    bool passed = true;

    // Source graph: two nodes with relative offsets dx=0, dx=150
    float srcX1 = 0.0f,   srcY1 = 0.0f;
    float srcX2 = 150.0f, srcY2 = 50.0f;

    // Paste anchor (mouse position)
    float mouseX = 300.0f, mouseY = 200.0f;

    // Expected paste positions
    float expectX1 = mouseX + (srcX1 - srcX1);  // offset from min = 0
    float expectY1 = mouseY + (srcY1 - srcY1);  // 0
    float expectX2 = mouseX + (srcX2 - srcX1);  // 150
    float expectY2 = mouseY + (srcY2 - srcY1);  // 50

    Olympe::NodeGraph destGraph;
    int nid1 = destGraph.CreateNode(Olympe::NodeType::BT_Action, expectX1, expectY1, "PastedA");
    int nid2 = destGraph.CreateNode(Olympe::NodeType::BT_Action, expectX2, expectY2, "PastedB");

    Olympe::GraphNode* pasted1 = destGraph.GetNode(nid1);
    Olympe::GraphNode* pasted2 = destGraph.GetNode(nid2);

    TEST_ASSERT(pasted1 != nullptr, "Pasted node 1 should exist");
    TEST_ASSERT(pasted2 != nullptr, "Pasted node 2 should exist");
    if (pasted1 == nullptr || pasted2 == nullptr) { passed = false; goto done; }

    TEST_ASSERT(FloatNear(pasted1->posX, 300.0f), "Paste X1 mismatch");
    TEST_ASSERT(FloatNear(pasted1->posY, 200.0f), "Paste Y1 mismatch");
    TEST_ASSERT(FloatNear(pasted2->posX, 450.0f), "Paste X2 mismatch");
    TEST_ASSERT(FloatNear(pasted2->posY, 250.0f), "Paste Y2 mismatch");

    if (!FloatNear(pasted1->posX, 300.0f) || !FloatNear(pasted1->posY, 200.0f) ||
        !FloatNear(pasted2->posX, 450.0f) || !FloatNear(pasted2->posY, 250.0f))
    {
        passed = false;
    }

    done:
    ReportTest("TestG_PasteAtOffset", passed);
}

// ---------------------------------------------------------------------------
// Test H: IsDirty flag is set after modification
// ---------------------------------------------------------------------------

static void TestH_DirtyFlag()
{
    std::cout << "Test H: IsDirty flag..." << std::endl;

    bool passed = true;

    Olympe::NodeGraph graph;
    TEST_ASSERT(!graph.IsDirty(), "Graph should not be dirty initially");

    graph.CreateNode(Olympe::NodeType::BT_Action, 0.0f, 0.0f, "N");
    // CreateNode does not necessarily set dirty; MarkDirty is the explicit API.
    graph.MarkDirty();
    TEST_ASSERT(graph.IsDirty(), "Graph should be dirty after MarkDirty()");
    if (!graph.IsDirty()) { passed = false; }

    graph.ClearDirty();
    TEST_ASSERT(!graph.IsDirty(), "Graph should be clean after ClearDirty()");
    if (graph.IsDirty()) { passed = false; }

    ReportTest("TestH_DirtyFlag", passed);
}

// ---------------------------------------------------------------------------
// Test I: Copy/paste roundtrip — node count increases and parameter preserved
// ---------------------------------------------------------------------------

static void TestI_CopyPasteRoundTripCountAndParam()
{
    std::cout << "Test I: Copy/paste roundtrip increases node count and preserves parameter..."
              << std::endl;

    bool passed = true;

    // ----- Source graph (simulates what is copied) -----
    Olympe::NodeGraph src;
    int srcId1 = src.CreateNode(Olympe::NodeType::BT_Sequence, 0.0f,   0.0f, "Parent");
    int srcId2 = src.CreateNode(Olympe::NodeType::BT_Action,  200.0f,  0.0f, "Child");
    src.SetNodeParameter(srcId2, "speed", "7.5");
    src.SetNodeParameter(srcId2, "target", "EnemyA");
    src.LinkNodes(srcId1, srcId2);

    // ----- Destination graph (simulates the graph being pasted into) -----
    Olympe::NodeGraph dest;
    int beforeCount = (int)dest.GetAllNodes().size();

    // Simulate the paste: iterate source nodes and create them in dest at offset.
    const float pasteOffsetX = 300.0f;
    const float pasteOffsetY = 100.0f;
    int lastPastedId = -1;
    auto srcNodes = src.GetAllNodes();
    for (auto* node : srcNodes)
    {
        int newId = dest.CreateNode(node->type,
                                    node->posX + pasteOffsetX,
                                    node->posY + pasteOffsetY,
                                    node->name);
        Olympe::GraphNode* newNode = dest.GetNode(newId);
        TEST_ASSERT(newNode != nullptr, "Pasted node should exist");
        if (newNode == nullptr) { passed = false; continue; }

        // Copy parameters (mirrors what Clipboard::PasteNodes does).
        for (auto it = node->parameters.begin(); it != node->parameters.end(); ++it)
            dest.SetNodeParameter(newId, it->first, it->second);

        if (node->name == "Child")
            lastPastedId = newId;
    }

    // ----- Verify node count increased -----
    int afterCount = (int)dest.GetAllNodes().size();
    TEST_ASSERT(afterCount > beforeCount, "Node count should increase after paste");
    if (afterCount <= beforeCount) { passed = false; }

    TEST_ASSERT((afterCount - beforeCount) == (int)srcNodes.size(),
                "Pasted node count should match source selection size");
    if ((afterCount - beforeCount) != (int)srcNodes.size()) { passed = false; }

    // ----- Verify parameter was preserved -----
    TEST_ASSERT(lastPastedId != -1, "Child node should have been pasted");
    if (lastPastedId != -1)
    {
        std::string speed  = dest.GetNodeParameter(lastPastedId, "speed");
        std::string target = dest.GetNodeParameter(lastPastedId, "target");

        TEST_ASSERT(speed  == "7.5",    "speed parameter should be preserved after paste");
        TEST_ASSERT(target == "EnemyA", "target parameter should be preserved after paste");

        if (speed != "7.5" || target != "EnemyA") { passed = false; }
    }

    ReportTest("TestI_CopyPasteRoundTripCountAndParam", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== ClipboardCopyPasteTest ===" << std::endl;

    TestA_CreateAndGet();
    TestB_ParameterRoundTrip();
    TestC_UniqueIDs();
    TestD_LinkNodes();
    TestE_DeleteNode();
    TestF_JsonRoundTrip();
    TestG_PasteAtOffset();
    TestH_DirtyFlag();
    TestI_CopyPasteRoundTripCountAndParam();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
