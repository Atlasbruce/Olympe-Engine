/**
 * @file Phase9Test.cpp
 * @brief Unit tests for Phase 9 ATS Visual Scripting — Advanced Node Features.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   1.  CommentManager_AddComment_AssignsId
 *   2.  CommentManager_RemoveComment_Succeeds
 *   3.  CommentManager_GetComment_ReturnsPointer
 *   4.  CommentManager_UpdateComment_ChangesText
 *   5.  CommentManager_Clear_ResetsState
 *   6.  CommentManager_Serialisation_RoundTrip
 *   7.  GraphComment_FromJson_DefaultsOnMissingFields
 *   8.  GroupManager_CreateGroup_AssignsId
 *   9.  GroupManager_CollapseGroup_SetsFlag
 *   10. GroupManager_ExpandGroup_ClearsFlag
 *   11. GroupManager_DeleteGroup_RemovesEntry
 *   12. GroupManager_GetGroupCount_Correct
 *   13. GroupManager_Serialisation_RoundTrip
 *   14. NodeGroup_FromJson_RestoresNodeIds
 *   15. LinkCache_Rebuild_PopulatesOutgoing
 *   16. LinkCache_Rebuild_PopulatesIncoming
 *   17. LinkCache_Invalidate_ClearsDirtyFlag
 *   18. LinkCache_GetOutgoing_MissingNode_ReturnsEmpty
 *   19. LinkCache_GetConnectionCount_Correct
 *   20. NodeValidator_NullGraph_ReturnsError
 *   21. NodeValidator_EmptyGraph_NoMessages
 *   22. NodeValidator_SubGraphMissingPath_Error
 *   23. NodeValidator_CycleDetected_Error
 *   24. NodeSearchPanel_OpenSearch_IsOpen
 *   25. NodeSearchPanel_CloseSearch_IsNotOpen
 *   26. NodeSearchPanel_UpdateQuery_EmptyReturnsAll
 *   27. NodeSearchPanel_UpdateQuery_FiltersByName
 *   28. NodeSearchPanel_SelectNext_Wraps
 *   29. NodeSearchPanel_ConfirmSelection_FiresCallback
 *   30. MiniMapPanel_SetBounds_VisibleAfterNodes
 *   31. MiniMapPanel_GetViewportRect_Normalised
 *   32. MiniMapPanel_HandleClick_InBounds
 *   33. MiniMapPanel_HandleClick_OutOfBounds_ReturnsFalse
 *   34. NodePresets_AddPreset_StoresEntry
 *   35. NodePresets_RemovePreset_Succeeds
 *   36. NodePresets_GetPresetInCategory_Filtered
 *   37. NodePresets_AddPreset_ReplacesDuplicate
 *   38. NodePresets_Serialisation_RoundTrip
 *
 * No SDL3, ImGui, or Editor dependency.
 * C++14 compliant — no C++17/20 features.
 */

#include "NodeGraphCore/GraphComment.h"
#include "NodeGraphCore/NodeGroup.h"
#include "NodeGraphCore/NodeValidator.h"
#include "NodeGraphCore/LinkCache.h"
#include "BlueprintEditor/NodeSearchPanel.h"
#include "BlueprintEditor/MiniMapPanel.h"
#include "BlueprintEditor/NodePresets.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "third_party/nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace Olympe;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Test infrastructure
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
// Helpers
// ---------------------------------------------------------------------------

/// Build a minimal TaskGraphTemplate with one EntryPoint node (no connections).
static TaskGraphTemplate MakeEmptyGraph()
{
    TaskGraphTemplate g;
    TaskNodeDefinition entry;
    entry.NodeID   = 1;
    entry.NodeName = "Start";
    entry.Type     = TaskNodeType::EntryPoint;
    g.Nodes.push_back(entry);
    return g;
}

/// Build a graph with a cycle: node 1 → node 2 → node 1.
static TaskGraphTemplate MakeCyclicGraph()
{
    TaskGraphTemplate g;

    TaskNodeDefinition n1;
    n1.NodeID   = 1;
    n1.NodeName = "A";
    n1.Type     = TaskNodeType::VSSequence;
    g.Nodes.push_back(n1);

    TaskNodeDefinition n2;
    n2.NodeID   = 2;
    n2.NodeName = "B";
    n2.Type     = TaskNodeType::VSSequence;
    g.Nodes.push_back(n2);

    ExecPinConnection c1;
    c1.SourceNodeID = 1;
    c1.TargetNodeID = 2;
    g.ExecConnections.push_back(c1);

    ExecPinConnection c2;
    c2.SourceNodeID = 2;
    c2.TargetNodeID = 1;
    g.ExecConnections.push_back(c2);

    return g;
}

/// Build a graph with one SubGraph node that has no SubGraphPath.
static TaskGraphTemplate MakeMissingPathGraph()
{
    TaskGraphTemplate g;

    TaskNodeDefinition sg;
    sg.NodeID       = 5;
    sg.NodeName     = "CallSub";
    sg.Type         = TaskNodeType::SubGraph;
    sg.SubGraphPath = "";  // deliberately empty
    g.Nodes.push_back(sg);

    return g;
}

// ===========================================================================
// CommentManager tests
// ===========================================================================

static void Test_CommentManager_AddComment_AssignsId()
{
    int prevFail = s_failCount;
    CommentManager& cm = CommentManager::Get();
    cm.Clear();

    GraphComment c;
    c.text = "Hello";
    int id = cm.AddComment(c);

    TEST_ASSERT(id >= 1,              "AddComment should assign ID >= 1");
    TEST_ASSERT(cm.GetCommentCount() == 1, "CommentCount should be 1 after one add");

    ReportTest("CommentManager_AddComment_AssignsId", s_failCount == prevFail);
}

static void Test_CommentManager_RemoveComment_Succeeds()
{
    int prevFail = s_failCount;
    CommentManager& cm = CommentManager::Get();
    cm.Clear();

    GraphComment c;
    c.text = "To remove";
    int id = cm.AddComment(c);
    cm.RemoveComment(id);

    TEST_ASSERT(cm.GetCommentCount() == 0, "CommentCount should be 0 after remove");

    ReportTest("CommentManager_RemoveComment_Succeeds", s_failCount == prevFail);
}

static void Test_CommentManager_GetComment_ReturnsPointer()
{
    int prevFail = s_failCount;
    CommentManager& cm = CommentManager::Get();
    cm.Clear();

    GraphComment c;
    c.text = "Test pointer";
    int id = cm.AddComment(c);

    GraphComment* ptr = cm.GetComment(id);
    TEST_ASSERT(ptr != nullptr,        "GetComment should return non-null pointer");
    TEST_ASSERT(ptr->text == "Test pointer", "Pointer should point to correct comment");

    GraphComment* missing = cm.GetComment(9999);
    TEST_ASSERT(missing == nullptr,    "GetComment for unknown ID should return null");

    ReportTest("CommentManager_GetComment_ReturnsPointer", s_failCount == prevFail);
}

static void Test_CommentManager_UpdateComment_ChangesText()
{
    int prevFail = s_failCount;
    CommentManager& cm = CommentManager::Get();
    cm.Clear();

    GraphComment c;
    c.text = "Original";
    int id = cm.AddComment(c);

    GraphComment updated;
    updated.text = "Updated text";
    cm.UpdateComment(id, updated);

    GraphComment* ptr = cm.GetComment(id);
    TEST_ASSERT(ptr != nullptr,             "Comment should still exist after update");
    TEST_ASSERT(ptr->text == "Updated text", "Text should reflect update");

    ReportTest("CommentManager_UpdateComment_ChangesText", s_failCount == prevFail);
}

static void Test_CommentManager_Clear_ResetsState()
{
    int prevFail = s_failCount;
    CommentManager& cm = CommentManager::Get();
    cm.Clear();

    GraphComment c;
    c.text = "A";
    cm.AddComment(c);
    cm.AddComment(c);

    cm.Clear();

    TEST_ASSERT(cm.GetCommentCount() == 0, "Count should be 0 after Clear");

    // After Clear the next ID should restart from 1
    int newId = cm.AddComment(c);
    TEST_ASSERT(newId == 1, "ID after Clear should restart at 1");

    ReportTest("CommentManager_Clear_ResetsState", s_failCount == prevFail);
}

static void Test_CommentManager_Serialisation_RoundTrip()
{
    int prevFail = s_failCount;
    CommentManager& cm = CommentManager::Get();
    cm.Clear();

    GraphComment c;
    c.text    = "Round trip";
    c.posX    = 10.0f;
    c.posY    = 20.0f;
    c.color   = 0xDEADBEEFu;
    cm.AddComment(c);

    json j = json::object();
    cm.SaveToJson(j);

    // Reload into the same singleton
    cm.LoadFromJson(j);

    TEST_ASSERT(cm.GetCommentCount() == 1, "Loaded count should match saved count");

    GraphComment* loaded = cm.GetComment(1);
    TEST_ASSERT(loaded != nullptr,              "Loaded comment should be accessible");
    TEST_ASSERT(loaded->text  == "Round trip",  "Text should survive round-trip");
    TEST_ASSERT(loaded->posX  == 10.0f,         "posX should survive round-trip");
    TEST_ASSERT(loaded->color == 0xDEADBEEFu,   "Color should survive round-trip");

    ReportTest("CommentManager_Serialisation_RoundTrip", s_failCount == prevFail);
}

static void Test_GraphComment_FromJson_DefaultsOnMissingFields()
{
    int prevFail = s_failCount;

    json j = json::object();  // completely empty
    GraphComment c = GraphComment::FromJson(j);

    TEST_ASSERT(c.id      == -1,    "Default id should be -1");
    TEST_ASSERT(c.text.empty(),     "Default text should be empty");
    TEST_ASSERT(c.width   == 200.0f, "Default width should be 200");
    TEST_ASSERT(c.height  == 80.0f,  "Default height should be 80");
    TEST_ASSERT(c.isVisible,         "Default isVisible should be true");

    ReportTest("GraphComment_FromJson_DefaultsOnMissingFields", s_failCount == prevFail);
}

// ===========================================================================
// GroupManager tests
// ===========================================================================

static void Test_GroupManager_CreateGroup_AssignsId()
{
    int prevFail = s_failCount;
    GroupManager& gm = GroupManager::Get();
    gm.Clear();

    std::vector<int> ids = {1, 2, 3};
    int gid = gm.CreateGroup("MyGroup", ids);

    TEST_ASSERT(gid >= 1,                "CreateGroup should assign ID >= 1");
    TEST_ASSERT(gm.GetGroupCount() == 1, "GroupCount should be 1");

    NodeGroup* g = gm.GetGroup(gid);
    TEST_ASSERT(g != nullptr,            "GetGroup should return non-null");
    TEST_ASSERT(g->name == "MyGroup",    "Group name should match");
    TEST_ASSERT(g->nodeIds.size() == 3,  "Group should have 3 node IDs");

    ReportTest("GroupManager_CreateGroup_AssignsId", s_failCount == prevFail);
}

static void Test_GroupManager_CollapseGroup_SetsFlag()
{
    int prevFail = s_failCount;
    GroupManager& gm = GroupManager::Get();
    gm.Clear();

    std::vector<int> ids;
    int gid = gm.CreateGroup("Collapsible", ids);
    gm.CollapseGroup(gid);

    NodeGroup* g = gm.GetGroup(gid);
    TEST_ASSERT(g != nullptr,        "Group should still exist");
    TEST_ASSERT(g->isCollapsed,      "isCollapsed should be true after CollapseGroup");

    ReportTest("GroupManager_CollapseGroup_SetsFlag", s_failCount == prevFail);
}

static void Test_GroupManager_ExpandGroup_ClearsFlag()
{
    int prevFail = s_failCount;
    GroupManager& gm = GroupManager::Get();
    gm.Clear();

    std::vector<int> ids;
    int gid = gm.CreateGroup("Expandable", ids);
    gm.CollapseGroup(gid);
    gm.ExpandGroup(gid);

    NodeGroup* g = gm.GetGroup(gid);
    TEST_ASSERT(g != nullptr,       "Group should exist");
    TEST_ASSERT(!g->isCollapsed,    "isCollapsed should be false after ExpandGroup");

    ReportTest("GroupManager_ExpandGroup_ClearsFlag", s_failCount == prevFail);
}

static void Test_GroupManager_DeleteGroup_RemovesEntry()
{
    int prevFail = s_failCount;
    GroupManager& gm = GroupManager::Get();
    gm.Clear();

    std::vector<int> ids;
    int gid = gm.CreateGroup("ToDelete", ids);
    gm.DeleteGroup(gid);

    TEST_ASSERT(gm.GetGroupCount() == 0, "GroupCount should be 0 after delete");
    TEST_ASSERT(gm.GetGroup(gid) == nullptr, "Deleted group should not be found");

    ReportTest("GroupManager_DeleteGroup_RemovesEntry", s_failCount == prevFail);
}

static void Test_GroupManager_GetGroupCount_Correct()
{
    int prevFail = s_failCount;
    GroupManager& gm = GroupManager::Get();
    gm.Clear();

    std::vector<int> ids;
    gm.CreateGroup("G1", ids);
    gm.CreateGroup("G2", ids);
    gm.CreateGroup("G3", ids);

    TEST_ASSERT(gm.GetGroupCount() == 3, "GetGroupCount should return 3");

    ReportTest("GroupManager_GetGroupCount_Correct", s_failCount == prevFail);
}

static void Test_GroupManager_Serialisation_RoundTrip()
{
    int prevFail = s_failCount;
    GroupManager& gm = GroupManager::Get();
    gm.Clear();

    std::vector<int> ids = {10, 20};
    gm.CreateGroup("Persist", ids);

    json j = json::object();
    gm.SaveToJson(j);

    // Reload into the same singleton
    gm.LoadFromJson(j);

    TEST_ASSERT(gm.GetGroupCount() == 1, "Loaded group count should match");

    NodeGroup* g = gm.GetGroup(1);
    TEST_ASSERT(g != nullptr,           "Loaded group should be accessible");
    TEST_ASSERT(g->name == "Persist",   "Group name should survive round-trip");
    TEST_ASSERT(g->nodeIds.size() == 2, "Node IDs should survive round-trip");

    ReportTest("GroupManager_Serialisation_RoundTrip", s_failCount == prevFail);
}

static void Test_NodeGroup_FromJson_RestoresNodeIds()
{
    int prevFail = s_failCount;

    json j          = json::object();
    j["id"]         = 42;
    j["name"]       = "TestGroup";
    j["isCollapsed"]= true;
    json ids        = json::array();
    ids.push_back(7);
    ids.push_back(8);
    j["nodeIds"]    = ids;

    NodeGroup g = NodeGroup::FromJson(j);
    TEST_ASSERT(g.id          == 42,          "ID should be 42");
    TEST_ASSERT(g.name        == "TestGroup", "Name should match");
    TEST_ASSERT(g.isCollapsed == true,        "isCollapsed should be true");
    TEST_ASSERT(g.nodeIds.size() == 2,        "Should have 2 node IDs");
    TEST_ASSERT(g.nodeIds[0]  == 7,           "First node ID should be 7");
    TEST_ASSERT(g.nodeIds[1]  == 8,           "Second node ID should be 8");

    ReportTest("NodeGroup_FromJson_RestoresNodeIds", s_failCount == prevFail);
}

// ===========================================================================
// LinkCache tests
// ===========================================================================

static void Test_LinkCache_Rebuild_PopulatesOutgoing()
{
    int prevFail = s_failCount;
    LinkCache& lc = LinkCache::Get();
    lc.Invalidate();

    std::vector<ExecPinConnection> conns;
    ExecPinConnection c;
    c.SourceNodeID = 1;
    c.TargetNodeID = 2;
    conns.push_back(c);

    lc.Rebuild(conns);

    const std::vector<ExecPinConnection>& out = lc.GetOutgoing(1);
    TEST_ASSERT(out.size() == 1,                  "Node 1 should have 1 outgoing connection");
    TEST_ASSERT(out[0].TargetNodeID == 2,         "Target should be node 2");
    TEST_ASSERT(lc.IsValid(),                     "Cache should be valid after Rebuild");

    ReportTest("LinkCache_Rebuild_PopulatesOutgoing", s_failCount == prevFail);
}

static void Test_LinkCache_Rebuild_PopulatesIncoming()
{
    int prevFail = s_failCount;
    LinkCache& lc = LinkCache::Get();

    const std::vector<ExecPinConnection>& inc = lc.GetIncoming(2);
    TEST_ASSERT(inc.size() == 1,              "Node 2 should have 1 incoming connection");
    TEST_ASSERT(inc[0].SourceNodeID == 1,     "Source should be node 1");

    ReportTest("LinkCache_Rebuild_PopulatesIncoming", s_failCount == prevFail);
}

static void Test_LinkCache_Invalidate_ClearsDirtyFlag()
{
    int prevFail = s_failCount;
    LinkCache& lc = LinkCache::Get();

    lc.Invalidate();
    TEST_ASSERT(!lc.IsValid(), "Cache should not be valid after Invalidate");

    ReportTest("LinkCache_Invalidate_ClearsDirtyFlag", s_failCount == prevFail);
}

static void Test_LinkCache_GetOutgoing_MissingNode_ReturnsEmpty()
{
    int prevFail = s_failCount;
    LinkCache& lc = LinkCache::Get();

    std::vector<ExecPinConnection> conns;  // empty
    lc.Rebuild(conns);

    const std::vector<ExecPinConnection>& out = lc.GetOutgoing(999);
    TEST_ASSERT(out.empty(), "Missing node ID should return empty vector");

    ReportTest("LinkCache_GetOutgoing_MissingNode_ReturnsEmpty", s_failCount == prevFail);
}

static void Test_LinkCache_GetConnectionCount_Correct()
{
    int prevFail = s_failCount;
    LinkCache& lc = LinkCache::Get();

    std::vector<ExecPinConnection> conns;
    ExecPinConnection c1; c1.SourceNodeID = 1; c1.TargetNodeID = 2; conns.push_back(c1);
    ExecPinConnection c2; c2.SourceNodeID = 2; c2.TargetNodeID = 3; conns.push_back(c2);
    ExecPinConnection c3; c3.SourceNodeID = 3; c3.TargetNodeID = 4; conns.push_back(c3);

    lc.Rebuild(conns);

    TEST_ASSERT(lc.GetConnectionCount() == 3, "Connection count should be 3");

    ReportTest("LinkCache_GetConnectionCount_Correct", s_failCount == prevFail);
}

// ===========================================================================
// NodeValidator tests
// ===========================================================================

static void Test_NodeValidator_NullGraph_ReturnsError()
{
    int prevFail = s_failCount;

    std::vector<ValidationMessage> msgs = NodeValidator::ValidateGraph(nullptr);
    TEST_ASSERT(!msgs.empty(),                   "Null graph should produce at least one message");
    TEST_ASSERT(msgs[0].severity == NVSeverity::Error, "Severity should be Error");

    ReportTest("NodeValidator_NullGraph_ReturnsError", s_failCount == prevFail);
}

static void Test_NodeValidator_EmptyGraph_NoMessages()
{
    int prevFail = s_failCount;

    TaskGraphTemplate g = MakeEmptyGraph();
    std::vector<ValidationMessage> msgs = NodeValidator::ValidateGraph(&g);

    // An EntryPoint node with no connections is exempt from "unconnected" warnings.
    bool hasError = false;
    for (size_t i = 0; i < msgs.size(); ++i)
    {
        if (msgs[i].severity == NVSeverity::Error)
        {
            hasError = true;
            break;
        }
    }
    TEST_ASSERT(!hasError, "Empty graph with only EntryPoint should have no Error messages");

    ReportTest("NodeValidator_EmptyGraph_NoMessages", s_failCount == prevFail);
}

static void Test_NodeValidator_SubGraphMissingPath_Error()
{
    int prevFail = s_failCount;

    TaskGraphTemplate g = MakeMissingPathGraph();
    std::vector<ValidationMessage> msgs = NodeValidator::ValidateGraph(&g);

    bool foundError = false;
    for (size_t i = 0; i < msgs.size(); ++i)
    {
        if (msgs[i].severity == NVSeverity::Error && msgs[i].nodeId == 5)
        {
            foundError = true;
            break;
        }
    }
    TEST_ASSERT(foundError, "SubGraph with missing subGraphPath should produce Error for node 5");

    ReportTest("NodeValidator_SubGraphMissingPath_Error", s_failCount == prevFail);
}

static void Test_NodeValidator_CycleDetected_Error()
{
    int prevFail = s_failCount;

    TaskGraphTemplate g = MakeCyclicGraph();
    std::vector<ValidationMessage> msgs = NodeValidator::ValidateGraph(&g);

    bool hasCycleError = false;
    for (size_t i = 0; i < msgs.size(); ++i)
    {
        if (msgs[i].severity == NVSeverity::Error)
        {
            hasCycleError = true;
            break;
        }
    }
    TEST_ASSERT(hasCycleError, "Cyclic graph should produce at least one Error message");

    ReportTest("NodeValidator_CycleDetected_Error", s_failCount == prevFail);
}

// ===========================================================================
// NodeSearchPanel tests
// ===========================================================================

static void Test_NodeSearchPanel_OpenSearch_IsOpen()
{
    int prevFail = s_failCount;
    NodeSearchPanel& sp = NodeSearchPanel::Get();
    sp.CloseSearch();

    sp.OpenSearch(100.0f, 200.0f);
    TEST_ASSERT(sp.IsOpen(),             "Panel should be open after OpenSearch");
    TEST_ASSERT(sp.GetSpawnX() == 100.0f, "SpawnX should be 100");
    TEST_ASSERT(sp.GetSpawnY() == 200.0f, "SpawnY should be 200");

    ReportTest("NodeSearchPanel_OpenSearch_IsOpen", s_failCount == prevFail);
}

static void Test_NodeSearchPanel_CloseSearch_IsNotOpen()
{
    int prevFail = s_failCount;
    NodeSearchPanel& sp = NodeSearchPanel::Get();

    sp.OpenSearch(0.0f, 0.0f);
    sp.CloseSearch();
    TEST_ASSERT(!sp.IsOpen(), "Panel should be closed after CloseSearch");

    ReportTest("NodeSearchPanel_CloseSearch_IsNotOpen", s_failCount == prevFail);
}

static void Test_NodeSearchPanel_UpdateQuery_EmptyReturnsAll()
{
    int prevFail = s_failCount;
    NodeSearchPanel& sp = NodeSearchPanel::Get();
    sp.OpenSearch(0.0f, 0.0f);

    // Opening already calls UpdateQuery(""), so results should be non-empty
    TEST_ASSERT(!sp.GetResults().empty(),  "Results should be non-empty for empty query");
    TEST_ASSERT(sp.GetResults().size() >= 10, "Catalogue should have at least 10 entries");

    ReportTest("NodeSearchPanel_UpdateQuery_EmptyReturnsAll", s_failCount == prevFail);
}

static void Test_NodeSearchPanel_UpdateQuery_FiltersByName()
{
    int prevFail = s_failCount;
    NodeSearchPanel& sp = NodeSearchPanel::Get();
    sp.OpenSearch(0.0f, 0.0f);
    sp.UpdateQuery("Sequence");

    const std::vector<NodeSearchResult>& results = sp.GetResults();
    TEST_ASSERT(!results.empty(), "Should find at least one result for 'Sequence'");
    if (!results.empty())
    {
        TEST_ASSERT(results[0].nodeType == "Sequence",
                    "First result for 'Sequence' should be the Sequence node type");
        TEST_ASSERT(results[0].relevanceScore > 0.5f,
                    "Exact match should have relevanceScore > 0.5");
    }

    ReportTest("NodeSearchPanel_UpdateQuery_FiltersByName", s_failCount == prevFail);
}

static void Test_NodeSearchPanel_SelectNext_Wraps()
{
    int prevFail = s_failCount;
    NodeSearchPanel& sp = NodeSearchPanel::Get();
    sp.OpenSearch(0.0f, 0.0f);
    sp.UpdateQuery("");  // load all results

    int total = static_cast<int>(sp.GetResults().size());
    if (total > 1)
    {
        sp.SelectNext();
        int after = sp.GetSelectedIndex();
        TEST_ASSERT(after == 1, "After one SelectNext, index should be 1");
    }

    // Reset selection to 0, then move to the last element (total-1) via total-1 steps,
    // and verify that one more SelectNext wraps back to 0.
    sp.OpenSearch(0.0f, 0.0f);
    sp.UpdateQuery("");
    // sp starts at index 0
    for (int i = 0; i < total - 1; ++i)
        sp.SelectNext();
    // Now at index total-1 (last element)
    TEST_ASSERT(sp.GetSelectedIndex() == total - 1,
                "Should reach last element after total-1 steps");
    sp.SelectNext();  // wrap to 0
    TEST_ASSERT(sp.GetSelectedIndex() == 0, "SelectNext should wrap to index 0");

    ReportTest("NodeSearchPanel_SelectNext_Wraps", s_failCount == prevFail);
}

static void Test_NodeSearchPanel_ConfirmSelection_FiresCallback()
{
    int prevFail = s_failCount;
    NodeSearchPanel& sp = NodeSearchPanel::Get();

    std::string capturedType;
    float capturedX = -1.0f;
    float capturedY = -1.0f;

    sp.SetNodeAddCallback([&](const std::string& type, float x, float y) {
        capturedType = type;
        capturedX    = x;
        capturedY    = y;
    });

    sp.OpenSearch(50.0f, 75.0f);
    sp.UpdateQuery("Sequence");

    sp.ConfirmSelection(0);

    TEST_ASSERT(capturedType == "Sequence", "Callback should receive 'Sequence' type");
    TEST_ASSERT(capturedX    == 50.0f,      "Callback should receive spawn X=50");
    TEST_ASSERT(capturedY    == 75.0f,      "Callback should receive spawn Y=75");
    TEST_ASSERT(!sp.IsOpen(),               "Panel should be closed after confirmation");

    ReportTest("NodeSearchPanel_ConfirmSelection_FiresCallback", s_failCount == prevFail);
}

// ===========================================================================
// MiniMapPanel tests
// ===========================================================================

static void Test_MiniMapPanel_SetBounds_VisibleAfterNodes()
{
    int prevFail = s_failCount;
    MiniMapPanel& mm = MiniMapPanel::Get();

    mm.SetGraphBounds(0.0f, 500.0f, 0.0f, 400.0f);
    mm.SetViewport(0.0f, 200.0f, 0.0f, 150.0f);

    typedef std::pair<int, std::pair<float, float>> NodePos;
    std::vector<NodePos> positions;
    positions.push_back(NodePos(1, std::make_pair(100.0f, 50.0f)));
    positions.push_back(NodePos(2, std::make_pair(300.0f, 200.0f)));

    mm.UpdateNodePositions(positions);

    TEST_ASSERT(mm.IsVisible(), "MiniMap should be visible after UpdateNodePositions");
    TEST_ASSERT(mm.GetNodes().size() == 2, "Should have 2 node entries");

    const std::vector<MiniMapNodeEntry>& nodes = mm.GetNodes();
    // Node 1 at x=100 in [0,500] → normalised 0.2
    TEST_ASSERT(nodes[0].nx > 0.0f && nodes[0].nx < 1.0f,
                "Normalised X should be in (0,1)");

    ReportTest("MiniMapPanel_SetBounds_VisibleAfterNodes", s_failCount == prevFail);
}

static void Test_MiniMapPanel_GetViewportRect_Normalised()
{
    int prevFail = s_failCount;
    MiniMapPanel& mm = MiniMapPanel::Get();

    mm.SetGraphBounds(0.0f, 1000.0f, 0.0f, 800.0f);
    mm.SetViewport(0.0f, 500.0f, 0.0f, 400.0f);

    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    mm.GetViewportRect(x, y, w, h);

    TEST_ASSERT(x >= 0.0f && x <= 1.0f, "Viewport X should be in [0,1]");
    TEST_ASSERT(y >= 0.0f && y <= 1.0f, "Viewport Y should be in [0,1]");
    TEST_ASSERT(w > 0.0f  && w <= 1.0f, "Viewport W should be in (0,1]");
    TEST_ASSERT(h > 0.0f  && h <= 1.0f, "Viewport H should be in (0,1]");

    // viewport 500/1000 = 0.5 width
    bool wApprox = (w > 0.49f && w < 0.51f);
    TEST_ASSERT(wApprox, "Viewport W should be ~0.5 for half-graph viewport");

    ReportTest("MiniMapPanel_GetViewportRect_Normalised", s_failCount == prevFail);
}

static void Test_MiniMapPanel_HandleClick_InBounds()
{
    int prevFail = s_failCount;
    MiniMapPanel& mm = MiniMapPanel::Get();

    mm.SetGraphBounds(0.0f, 1000.0f, 0.0f, 800.0f);

    float sx = 0.0f, sy = 0.0f;
    bool ok = mm.HandleClick(0.5f, 0.5f, sx, sy);

    TEST_ASSERT(ok,         "HandleClick at (0.5, 0.5) should succeed");
    TEST_ASSERT(sx == 500.0f, "Scroll X at 0.5 in [0,1000] should be 500");
    TEST_ASSERT(sy == 400.0f, "Scroll Y at 0.5 in [0,800]  should be 400");

    ReportTest("MiniMapPanel_HandleClick_InBounds", s_failCount == prevFail);
}

static void Test_MiniMapPanel_HandleClick_OutOfBounds_ReturnsFalse()
{
    int prevFail = s_failCount;
    MiniMapPanel& mm = MiniMapPanel::Get();

    float sx = 0.0f, sy = 0.0f;
    bool ok = mm.HandleClick(-0.1f, 0.5f, sx, sy);
    TEST_ASSERT(!ok, "HandleClick with negative X should return false");

    ok = mm.HandleClick(0.5f, 1.5f, sx, sy);
    TEST_ASSERT(!ok, "HandleClick with Y > 1 should return false");

    ReportTest("MiniMapPanel_HandleClick_OutOfBounds_ReturnsFalse", s_failCount == prevFail);
}

// ===========================================================================
// NodePresets tests
// ===========================================================================

static void Test_NodePresets_AddPreset_StoresEntry()
{
    int prevFail = s_failCount;
    NodePresetManager& pm = NodePresetManager::Get();
    pm.Clear();

    NodePreset p;
    p.name     = "PatrolPreset";
    p.category = "AI";
    pm.AddPreset(p);

    TEST_ASSERT(pm.GetPresetCount() == 1, "PresetCount should be 1");
    TEST_ASSERT(pm.GetPreset("PatrolPreset") != nullptr, "Should find preset by name");

    ReportTest("NodePresets_AddPreset_StoresEntry", s_failCount == prevFail);
}

static void Test_NodePresets_RemovePreset_Succeeds()
{
    int prevFail = s_failCount;
    NodePresetManager& pm = NodePresetManager::Get();
    pm.Clear();

    NodePreset p;
    p.name     = "ToRemove";
    p.category = "Test";
    pm.AddPreset(p);
    pm.RemovePreset("ToRemove");

    TEST_ASSERT(pm.GetPresetCount() == 0,            "Count should be 0 after remove");
    TEST_ASSERT(pm.GetPreset("ToRemove") == nullptr, "Removed preset should not be found");

    ReportTest("NodePresets_RemovePreset_Succeeds", s_failCount == prevFail);
}

static void Test_NodePresets_GetPresetInCategory_Filtered()
{
    int prevFail = s_failCount;
    NodePresetManager& pm = NodePresetManager::Get();
    pm.Clear();

    NodePreset a; a.name = "A"; a.category = "AI";
    NodePreset b; b.name = "B"; b.category = "UI";
    NodePreset c; c.name = "C"; c.category = "AI";
    pm.AddPreset(a);
    pm.AddPreset(b);
    pm.AddPreset(c);

    std::vector<NodePreset> ai = pm.GetPresetsInCategory("AI");
    std::vector<NodePreset> ui = pm.GetPresetsInCategory("UI");

    TEST_ASSERT(ai.size() == 2, "Should find 2 AI presets");
    TEST_ASSERT(ui.size() == 1, "Should find 1 UI preset");

    ReportTest("NodePresets_GetPresetInCategory_Filtered", s_failCount == prevFail);
}

static void Test_NodePresets_AddPreset_ReplacesDuplicate()
{
    int prevFail = s_failCount;
    NodePresetManager& pm = NodePresetManager::Get();
    pm.Clear();

    NodePreset p1; p1.name = "Dup"; p1.description = "first";
    NodePreset p2; p2.name = "Dup"; p2.description = "second";
    pm.AddPreset(p1);
    pm.AddPreset(p2);

    TEST_ASSERT(pm.GetPresetCount() == 1, "Duplicate add should replace, not append");
    NodePreset* found = pm.GetPreset("Dup");
    TEST_ASSERT(found != nullptr,                "Should find the preset");
    TEST_ASSERT(found->description == "second",  "Second add should overwrite description");

    ReportTest("NodePresets_AddPreset_ReplacesDuplicate", s_failCount == prevFail);
}

static void Test_NodePresets_Serialisation_RoundTrip()
{
    int prevFail = s_failCount;
    NodePresetManager& pm = NodePresetManager::Get();
    pm.Clear();

    NodePreset p;
    p.name        = "SerialPreset";
    p.description = "Saved description";
    p.category    = "TestCat";
    p.nodeIds     = {10, 20, 30};
    pm.AddPreset(p);

    // Serialise to JSON string and back via the singleton itself
    json root         = json::object();
    json arr          = json::array();
    NodePreset* found = pm.GetPreset("SerialPreset");
    TEST_ASSERT(found != nullptr, "Preset should be found before serialisation");

    if (found)
    {
        arr.push_back(found->ToJson());
        root["presets"] = arr;

        // Round-trip: clear and reload
        pm.Clear();
        if (root.contains("presets") && root["presets"].is_array())
        {
            const json& presetArr = root["presets"];
            for (auto it = presetArr.begin(); it != presetArr.end(); ++it)
                pm.AddPreset(NodePreset::FromJson(*it));
        }

        TEST_ASSERT(pm.GetPresetCount() == 1,           "Loaded count should match");
        NodePreset* loaded = pm.GetPreset("SerialPreset");
        TEST_ASSERT(loaded != nullptr,                   "Loaded preset accessible");
        TEST_ASSERT(loaded->description == "Saved description",
                    "Description should survive round-trip");
        TEST_ASSERT(loaded->nodeIds.size() == 3,
                    "Node IDs should survive round-trip");
    }

    ReportTest("NodePresets_Serialisation_RoundTrip", s_failCount == prevFail);
}

// ===========================================================================
// main
// ===========================================================================

int main()
{
    std::cout << "===== Phase 9 Advanced Node Features Tests =====" << std::endl;

    s_passCount = 0;
    s_failCount = 0;

    // CommentManager
    Test_CommentManager_AddComment_AssignsId();
    Test_CommentManager_RemoveComment_Succeeds();
    Test_CommentManager_GetComment_ReturnsPointer();
    Test_CommentManager_UpdateComment_ChangesText();
    Test_CommentManager_Clear_ResetsState();
    Test_CommentManager_Serialisation_RoundTrip();
    Test_GraphComment_FromJson_DefaultsOnMissingFields();

    // GroupManager
    Test_GroupManager_CreateGroup_AssignsId();
    Test_GroupManager_CollapseGroup_SetsFlag();
    Test_GroupManager_ExpandGroup_ClearsFlag();
    Test_GroupManager_DeleteGroup_RemovesEntry();
    Test_GroupManager_GetGroupCount_Correct();
    Test_GroupManager_Serialisation_RoundTrip();
    Test_NodeGroup_FromJson_RestoresNodeIds();

    // LinkCache
    Test_LinkCache_Rebuild_PopulatesOutgoing();
    Test_LinkCache_Rebuild_PopulatesIncoming();
    Test_LinkCache_Invalidate_ClearsDirtyFlag();
    Test_LinkCache_GetOutgoing_MissingNode_ReturnsEmpty();
    Test_LinkCache_GetConnectionCount_Correct();

    // NodeValidator
    Test_NodeValidator_NullGraph_ReturnsError();
    Test_NodeValidator_EmptyGraph_NoMessages();
    Test_NodeValidator_SubGraphMissingPath_Error();
    Test_NodeValidator_CycleDetected_Error();

    // NodeSearchPanel
    Test_NodeSearchPanel_OpenSearch_IsOpen();
    Test_NodeSearchPanel_CloseSearch_IsNotOpen();
    Test_NodeSearchPanel_UpdateQuery_EmptyReturnsAll();
    Test_NodeSearchPanel_UpdateQuery_FiltersByName();
    Test_NodeSearchPanel_SelectNext_Wraps();
    Test_NodeSearchPanel_ConfirmSelection_FiresCallback();

    // MiniMapPanel
    Test_MiniMapPanel_SetBounds_VisibleAfterNodes();
    Test_MiniMapPanel_GetViewportRect_Normalised();
    Test_MiniMapPanel_HandleClick_InBounds();
    Test_MiniMapPanel_HandleClick_OutOfBounds_ReturnsFalse();

    // NodePresets
    Test_NodePresets_AddPreset_StoresEntry();
    Test_NodePresets_RemovePreset_Succeeds();
    Test_NodePresets_GetPresetInCategory_Filtered();
    Test_NodePresets_AddPreset_ReplacesDuplicate();
    Test_NodePresets_Serialisation_RoundTrip();

    std::cout << "\n===== Results: "
              << s_passCount << " passed, "
              << s_failCount << " failed =====" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
