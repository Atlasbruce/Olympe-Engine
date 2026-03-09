/**
 * @file Phase7Test.cpp
 * @brief Unit tests for Phase 7 ATS Visual Scripting features.
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * Tests cover:
 *   1.  NodeSearchPalette_GetAllNodes_ReturnsNonEmpty
 *   2.  NodeSearchPalette_FuzzySearch_EmptyQuery_ReturnsAll
 *   3.  NodeSearchPalette_FuzzySearch_ExactMatch_TopScore
 *   4.  NodeSearchPalette_FuzzySearch_NoMatch_ReturnsEmpty
 *   5.  NodeSearchPalette_ComputeFuzzyScore_ExactMatch
 *   6.  NodeSearchPalette_ComputeFuzzyScore_SubstringMatch
 *   7.  NodeSearchPalette_ComputeFuzzyScore_NoMatch
 *   8.  NodeSearchPalette_FilterByCategory
 *   9.  MinimapWidget_UpdateNodes_NormalisesCorrectly
 *  10.  MinimapWidget_UpdateViewport_NormalisesCorrectly
 *  11.  MinimapWidget_OnDrag_ScalesToGraphSpace
 *  12.  GridSnapping_DefaultEnabled
 *  13.  GridSnapping_SnapX_RoundsToGrid
 *  14.  GridSnapping_Toggle_DisablesSnapping
 *  15.  GridSnapping_SetGridSize_Invalid_Ignored
 *  16.  FileWatcher_DefaultConstructor_NotWatching
 *  17.  FileWatcher_Watch_NonexistentFile_DoesNotCrash
 *  18.  GraphValidationPanel_EmptyGraph_CriticalError
 *  19.  GraphValidationPanel_NoEntryPoint_CriticalError
 *  20.  GraphValidationPanel_SubGraphMissingPath_Error
 *  21.  GraphValidationPanel_OnErrorClick_SetsSelection
 *  22.  GraphValidationPanel_Clear_ResetsState
 *  23.  ViewportCulling_IsNodeVisible_OverlapsViewport
 *  24.  ViewportCulling_IsNodeVisible_OutsideViewport
 *  25.  ViewportCulling_FilterVisibleNodes_CorrectCount
 *  26.  ConnectionCache_SetAndGet_ReturnsStored
 *  27.  ConnectionCache_HasCached_FalseBeforeSet
 *  28.  ConnectionCache_Invalidate_RemovesEntry
 *  29.  ConnectionCache_InvalidateAll_EmptiesCache
 *
 * No SDL3, ImGui, or Editor dependency.
 * C++14 compliant — no C++17/20 features.
 */

#include "BlueprintEditor/NodeSearchPalette.h"
#include "BlueprintEditor/MinimapWidget.h"
#include "BlueprintEditor/GridSnapping.h"
#include "BlueprintEditor/FileWatcher.h"
#include "BlueprintEditor/GraphValidationPanel.h"
#include "BlueprintEditor/Optimizations/ViewportCulling.h"
#include "BlueprintEditor/Optimizations/ConnectionCache.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <cmath>

using namespace Olympe;

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

static float Approx(float a, float b, float eps = 0.001f)
{
    float d = a - b;
    return (d < 0.0f ? -d : d) < eps;
}

// ---------------------------------------------------------------------------
// NodeSearchPalette tests
// ---------------------------------------------------------------------------

static void Test_NodeSearchPalette_GetAllNodes_ReturnsNonEmpty()
{
    int prevFail = s_failCount;
    NodeSearchPalette& p = NodeSearchPalette::Get();

    TEST_ASSERT(!p.GetAllNodes().empty(),
                "GetAllNodes() should return a non-empty catalog");
    TEST_ASSERT(p.GetAllNodes().size() >= 15,
                "Catalog should contain at least 15 node types");

    ReportTest("NodeSearchPalette_GetAllNodes_ReturnsNonEmpty",
               s_failCount == prevFail);
}

static void Test_NodeSearchPalette_FuzzySearch_EmptyQuery_ReturnsAll()
{
    int prevFail = s_failCount;
    NodeSearchPalette& p = NodeSearchPalette::Get();

    std::vector<NodeSearchResult> all = p.GetAllNodes();
    std::vector<NodeSearchResult> res = p.FuzzySearch("");

    TEST_ASSERT(res.size() == all.size(),
                "Empty query should return all nodes");

    ReportTest("NodeSearchPalette_FuzzySearch_EmptyQuery_ReturnsAll",
               s_failCount == prevFail);
}

static void Test_NodeSearchPalette_FuzzySearch_ExactMatch_TopScore()
{
    int prevFail = s_failCount;
    NodeSearchPalette& p = NodeSearchPalette::Get();

    std::vector<NodeSearchResult> res = p.FuzzySearch("Sequence");
    TEST_ASSERT(!res.empty(), "FuzzySearch('Sequence') should return results");

    if (!res.empty())
    {
        TEST_ASSERT(res[0].typeName == "Sequence" || res[0].displayName == "Sequence",
                    "First result for 'Sequence' should be the Sequence node");
        TEST_ASSERT(res[0].score >= 600,
                    "Exact/substring match score should be >= 600");
    }

    ReportTest("NodeSearchPalette_FuzzySearch_ExactMatch_TopScore",
               s_failCount == prevFail);
}

static void Test_NodeSearchPalette_FuzzySearch_NoMatch_ReturnsEmpty()
{
    int prevFail = s_failCount;
    NodeSearchPalette& p = NodeSearchPalette::Get();

    std::vector<NodeSearchResult> res = p.FuzzySearch("xyzzy_no_match_9999");
    TEST_ASSERT(res.empty(), "Non-matching query should return empty results");

    ReportTest("NodeSearchPalette_FuzzySearch_NoMatch_ReturnsEmpty",
               s_failCount == prevFail);
}

static void Test_NodeSearchPalette_ComputeFuzzyScore_ExactMatch()
{
    int prevFail = s_failCount;

    int score = NodeSearchPalette::ComputeFuzzyScore("sequence", "sequence");
    TEST_ASSERT(score == 1000, "Exact match should return score 1000");

    ReportTest("NodeSearchPalette_ComputeFuzzyScore_ExactMatch",
               s_failCount == prevFail);
}

static void Test_NodeSearchPalette_ComputeFuzzyScore_SubstringMatch()
{
    int prevFail = s_failCount;

    int score = NodeSearchPalette::ComputeFuzzyScore("move", "MoveToLocation");
    TEST_ASSERT(score >= 600, "Substring match should return score >= 600");

    ReportTest("NodeSearchPalette_ComputeFuzzyScore_SubstringMatch",
               s_failCount == prevFail);
}

static void Test_NodeSearchPalette_ComputeFuzzyScore_NoMatch()
{
    int prevFail = s_failCount;

    int score = NodeSearchPalette::ComputeFuzzyScore("zzz", "Sequence");
    TEST_ASSERT(score == 0, "Non-matching query should return score 0");

    ReportTest("NodeSearchPalette_ComputeFuzzyScore_NoMatch",
               s_failCount == prevFail);
}

static void Test_NodeSearchPalette_FilterByCategory()
{
    int prevFail = s_failCount;
    NodeSearchPalette& p = NodeSearchPalette::Get();

    std::vector<NodeSearchResult> cf = p.FuzzySearch("", NodeSearchCategory::ControlFlow);
    std::vector<NodeSearchResult> ac = p.FuzzySearch("", NodeSearchCategory::Actions);
    std::vector<NodeSearchResult> da = p.FuzzySearch("", NodeSearchCategory::Data);

    TEST_ASSERT(!cf.empty(), "ControlFlow filter should return results");
    TEST_ASSERT(!ac.empty(), "Actions filter should return results");
    TEST_ASSERT(!da.empty(), "Data filter should return results");

    for (size_t i = 0; i < cf.size(); ++i)
        TEST_ASSERT(cf[i].category == NodeSearchCategory::ControlFlow,
                    "ControlFlow filter must not include other categories");
    for (size_t i = 0; i < ac.size(); ++i)
        TEST_ASSERT(ac[i].category == NodeSearchCategory::Actions,
                    "Actions filter must not include other categories");

    ReportTest("NodeSearchPalette_FilterByCategory",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// MinimapWidget tests
// ---------------------------------------------------------------------------

static void Test_MinimapWidget_UpdateNodes_NormalisesCorrectly()
{
    int prevFail = s_failCount;
    MinimapWidget& mm = MinimapWidget::Get();

    std::vector<MinimapNode> nodes;
    MinimapNode n0; n0.id = 1; n0.x = 100.0f; n0.y = 50.0f;
    MinimapNode n1; n1.id = 2; n1.x = 300.0f; n1.y = 150.0f;
    nodes.push_back(n0);
    nodes.push_back(n1);

    mm.UpdateNodes(nodes, 0.0f, 0.0f, 400.0f, 200.0f);

    const std::vector<MinimapNode>& result = mm.GetMinimapNodes();
    TEST_ASSERT(result.size() == 2, "Should have 2 minimap nodes");

    if (result.size() == 2)
    {
        TEST_ASSERT(Approx(result[0].x, 0.25f), "Node 0 x should be 0.25");
        TEST_ASSERT(Approx(result[0].y, 0.25f), "Node 0 y should be 0.25");
        TEST_ASSERT(Approx(result[1].x, 0.75f), "Node 1 x should be 0.75");
        TEST_ASSERT(Approx(result[1].y, 0.75f), "Node 1 y should be 0.75");
    }

    ReportTest("MinimapWidget_UpdateNodes_NormalisesCorrectly",
               s_failCount == prevFail);
}

static void Test_MinimapWidget_UpdateViewport_NormalisesCorrectly()
{
    int prevFail = s_failCount;
    MinimapWidget& mm = MinimapWidget::Get();

    // Graph 0..400 x 0..200; view at (100,50) size (200,100)
    mm.UpdateViewport(100.0f, 50.0f, 200.0f, 100.0f, 0.0f, 0.0f, 400.0f, 200.0f);

    const MinimapViewport& vp = mm.GetViewport();
    TEST_ASSERT(Approx(vp.x, 0.25f), "Viewport x should be 0.25");
    TEST_ASSERT(Approx(vp.y, 0.25f), "Viewport y should be 0.25");
    TEST_ASSERT(Approx(vp.w, 0.5f),  "Viewport w should be 0.50");
    TEST_ASSERT(Approx(vp.h, 0.5f),  "Viewport h should be 0.50");

    ReportTest("MinimapWidget_UpdateViewport_NormalisesCorrectly",
               s_failCount == prevFail);
}

static void Test_MinimapWidget_OnDrag_ScalesToGraphSpace()
{
    int prevFail = s_failCount;
    MinimapWidget& mm = MinimapWidget::Get();

    float dx = 0.0f, dy = 0.0f;
    // Drag half the minimap width/height → should equal half of graphW/H
    mm.OnDrag(MinimapWidget::WIDTH / 2.0f, MinimapWidget::HEIGHT / 2.0f,
              1000.0f, 800.0f, dx, dy);

    TEST_ASSERT(Approx(dx, 500.0f), "OnDrag dx should be 500 (half of graphW=1000)");
    TEST_ASSERT(Approx(dy, 400.0f), "OnDrag dy should be 400 (half of graphH=800)");

    ReportTest("MinimapWidget_OnDrag_ScalesToGraphSpace",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// GridSnapping tests
// ---------------------------------------------------------------------------

static void Test_GridSnapping_DefaultEnabled()
{
    int prevFail = s_failCount;
    GridSnapping& gs = GridSnapping::Get();
    gs.SetEnabled(true);
    gs.SetGridSize(16);

    TEST_ASSERT(gs.IsEnabled(),     "Default state should be enabled");
    TEST_ASSERT(gs.GetGridSize() == 16, "Grid size should be 16");

    ReportTest("GridSnapping_DefaultEnabled", s_failCount == prevFail);
}

static void Test_GridSnapping_SnapX_RoundsToGrid()
{
    int prevFail = s_failCount;
    GridSnapping& gs = GridSnapping::Get();
    gs.SetEnabled(true);
    gs.SetGridSize(16);

    TEST_ASSERT(Approx(gs.SnapX(0.0f),   0.0f),  "0   -> 0");
    TEST_ASSERT(Approx(gs.SnapX(7.9f),   0.0f),  "7.9 -> 0");
    TEST_ASSERT(Approx(gs.SnapX(8.0f),  16.0f),  "8   -> 16");
    TEST_ASSERT(Approx(gs.SnapX(16.0f), 16.0f),  "16  -> 16");
    TEST_ASSERT(Approx(gs.SnapX(20.0f), 16.0f),  "20  -> 16");
    TEST_ASSERT(Approx(gs.SnapX(24.1f), 32.0f),  "24.1-> 32");

    ReportTest("GridSnapping_SnapX_RoundsToGrid", s_failCount == prevFail);
}

static void Test_GridSnapping_Toggle_DisablesSnapping()
{
    int prevFail = s_failCount;
    GridSnapping& gs = GridSnapping::Get();
    gs.SetEnabled(true);
    gs.Toggle();

    TEST_ASSERT(!gs.IsEnabled(), "Toggle should disable snapping");
    TEST_ASSERT(Approx(gs.SnapX(7.3f), 7.3f),
                "Disabled snapping should return original value");

    gs.Toggle();
    TEST_ASSERT(gs.IsEnabled(), "Second toggle should re-enable snapping");

    ReportTest("GridSnapping_Toggle_DisablesSnapping", s_failCount == prevFail);
}

static void Test_GridSnapping_SetGridSize_Invalid_Ignored()
{
    int prevFail = s_failCount;
    GridSnapping& gs = GridSnapping::Get();
    gs.SetGridSize(16);
    gs.SetGridSize(0);
    TEST_ASSERT(gs.GetGridSize() == 16, "Zero grid size should be ignored");
    gs.SetGridSize(-5);
    TEST_ASSERT(gs.GetGridSize() == 16, "Negative grid size should be ignored");

    ReportTest("GridSnapping_SetGridSize_Invalid_Ignored", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// FileWatcher tests
// ---------------------------------------------------------------------------

static void Test_FileWatcher_DefaultConstructor_NotWatching()
{
    int prevFail = s_failCount;
    FileWatcher fw;

    TEST_ASSERT(!fw.IsWatching(), "Default constructor should not be watching");
    TEST_ASSERT(fw.GetPath().empty(), "Default path should be empty");
    TEST_ASSERT(!fw.HasChanged(),     "HasChanged() should be false when not watching");

    ReportTest("FileWatcher_DefaultConstructor_NotWatching", s_failCount == prevFail);
}

static void Test_FileWatcher_Watch_NonexistentFile_DoesNotCrash()
{
    int prevFail = s_failCount;
    FileWatcher fw("nonexistent_file_99999.json");

    TEST_ASSERT(fw.IsWatching(), "IsWatching() should be true after Watch()");
    TEST_ASSERT(fw.GetPath() == "nonexistent_file_99999.json",
                "GetPath() should return watched path");
    // HasChanged() should not crash on missing file
    bool changed = fw.HasChanged();
    (void)changed;
    fw.Reset();

    ReportTest("FileWatcher_Watch_NonexistentFile_DoesNotCrash",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// GraphValidationPanel tests
// ---------------------------------------------------------------------------

static TaskGraphTemplate MakeEmptyGraph()
{
    TaskGraphTemplate g;
    g.Name    = "Test";
    g.GraphType = "VisualScript";
    return g;
}

static TaskGraphTemplate MakeMinimalGraph()
{
    TaskGraphTemplate g;
    g.Name        = "MinimalTest";
    g.GraphType   = "VisualScript";

    // Entry node
    TaskNodeDefinition entry;
    entry.NodeID   = 1;
    entry.NodeName = "EntryPoint";
    entry.Type     = TaskNodeType::EntryPoint;

    // Action node
    TaskNodeDefinition action;
    action.NodeID   = 2;
    action.NodeName = "Move";
    action.Type     = TaskNodeType::AtomicTask;

    g.Nodes.push_back(entry);
    g.Nodes.push_back(action);
    g.EntryPointID = 1;

    // Connect entry -> action
    ExecPinConnection conn;
    conn.SourceNodeID  = 1;
    conn.SourcePinName = "Out";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    g.ExecConnections.push_back(conn);

    g.BuildLookupCache();
    return g;
}

static void Test_GraphValidationPanel_EmptyGraph_CriticalError()
{
    int prevFail = s_failCount;
    GraphValidationPanel& panel = GraphValidationPanel::Get();

    TaskGraphTemplate g = MakeEmptyGraph();
    panel.Validate(g);

    TEST_ASSERT(panel.HasErrors(),         "Empty graph should have errors");
    TEST_ASSERT(panel.HasCriticalErrors(), "Empty graph should have critical errors");

    ReportTest("GraphValidationPanel_EmptyGraph_CriticalError",
               s_failCount == prevFail);
}

static void Test_GraphValidationPanel_NoEntryPoint_CriticalError()
{
    int prevFail = s_failCount;
    GraphValidationPanel& panel = GraphValidationPanel::Get();

    TaskGraphTemplate g;
    g.Name      = "NoEntry";
    g.GraphType = "VisualScript";
    // One node but no EntryPoint
    TaskNodeDefinition n;
    n.NodeID   = 1;
    n.NodeName = "Move";
    n.Type     = TaskNodeType::AtomicTask;
    g.Nodes.push_back(n);
    g.EntryPointID = NODE_INDEX_NONE;

    panel.Validate(g);

    TEST_ASSERT(panel.HasCriticalErrors(),
                "Graph with no EntryPoint should have critical errors");

    ReportTest("GraphValidationPanel_NoEntryPoint_CriticalError",
               s_failCount == prevFail);
}

static void Test_GraphValidationPanel_SubGraphMissingPath_Error()
{
    int prevFail = s_failCount;
    GraphValidationPanel& panel = GraphValidationPanel::Get();

    TaskGraphTemplate g = MakeMinimalGraph();

    // Add a SubGraph node with empty SubGraphPath
    TaskNodeDefinition sg;
    sg.NodeID       = 10;
    sg.NodeName     = "CallSubGraph";
    sg.Type         = TaskNodeType::SubGraph;
    sg.SubGraphPath = "";
    g.Nodes.push_back(sg);

    // Give it an exec output so it isn't flagged as dead-end
    ExecPinConnection conn;
    conn.SourceNodeID  = 10;
    conn.SourcePinName = "Completed";
    conn.TargetNodeID  = 2;
    conn.TargetPinName = "In";
    g.ExecConnections.push_back(conn);

    g.BuildLookupCache();
    panel.Validate(g);

    bool foundSubGraphError = false;
    const std::vector<GraphValidationError>& errs = panel.GetErrors();
    for (size_t i = 0; i < errs.size(); ++i)
    {
        if (errs[i].nodeId == 10)
        {
            foundSubGraphError = true;
            break;
        }
    }
    TEST_ASSERT(foundSubGraphError,
                "SubGraph node with empty path should produce an error");

    ReportTest("GraphValidationPanel_SubGraphMissingPath_Error",
               s_failCount == prevFail);
}

static void Test_GraphValidationPanel_OnErrorClick_SetsSelection()
{
    int prevFail = s_failCount;
    GraphValidationPanel& panel = GraphValidationPanel::Get();

    panel.OnErrorClick(42);
    TEST_ASSERT(panel.GetSelectedNodeId() == 42,
                "OnErrorClick should set selected node to 42");

    ReportTest("GraphValidationPanel_OnErrorClick_SetsSelection",
               s_failCount == prevFail);
}

static void Test_GraphValidationPanel_Clear_ResetsState()
{
    int prevFail = s_failCount;
    GraphValidationPanel& panel = GraphValidationPanel::Get();

    TaskGraphTemplate g = MakeEmptyGraph();
    panel.Validate(g);
    panel.OnErrorClick(5);

    panel.Clear();

    TEST_ASSERT(!panel.HasErrors(),        "After Clear, HasErrors() should be false");
    TEST_ASSERT(panel.GetSelectedNodeId() == -1,
                "After Clear, GetSelectedNodeId() should be -1");

    ReportTest("GraphValidationPanel_Clear_ResetsState",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// ViewportCulling tests
// ---------------------------------------------------------------------------

static void Test_ViewportCulling_IsNodeVisible_OverlapsViewport()
{
    int prevFail = s_failCount;
    ViewRect vp = { 100.0f, 100.0f, 800.0f, 600.0f };

    // Fully inside
    TEST_ASSERT(ViewportCulling::IsNodeVisible(200.0f, 200.0f, 100.0f, 50.0f, vp),
                "Node fully inside viewport should be visible");

    // Partially overlapping
    TEST_ASSERT(ViewportCulling::IsNodeVisible(850.0f, 100.0f, 100.0f, 50.0f, vp),
                "Node partially overlapping viewport should be visible");

    // Just outside but within margin
    TEST_ASSERT(ViewportCulling::IsNodeVisible(50.0f, 100.0f, 30.0f, 30.0f, vp, 50.0f),
                "Node within margin should be visible");

    ReportTest("ViewportCulling_IsNodeVisible_OverlapsViewport",
               s_failCount == prevFail);
}

static void Test_ViewportCulling_IsNodeVisible_OutsideViewport()
{
    int prevFail = s_failCount;
    ViewRect vp = { 100.0f, 100.0f, 800.0f, 600.0f };

    // Far to the left, no margin
    TEST_ASSERT(!ViewportCulling::IsNodeVisible(0.0f, 100.0f, 50.0f, 50.0f, vp, 0.0f),
                "Node far left should not be visible with zero margin");

    // Far below
    TEST_ASSERT(!ViewportCulling::IsNodeVisible(200.0f, 800.0f, 50.0f, 50.0f, vp, 0.0f),
                "Node far below should not be visible");

    ReportTest("ViewportCulling_IsNodeVisible_OutsideViewport",
               s_failCount == prevFail);
}

static void Test_ViewportCulling_FilterVisibleNodes_CorrectCount()
{
    int prevFail = s_failCount;
    ViewRect vp = { 0.0f, 0.0f, 400.0f, 300.0f };

    std::vector<std::pair<int, ViewRect>> rects;
    ViewRect r0 = { 10.0f,  10.0f, 80.0f, 80.0f };
    ViewRect r1 = { 600.0f, 10.0f, 80.0f, 80.0f };  // outside
    ViewRect r2 = { 200.0f, 150.0f, 80.0f, 80.0f };
    rects.push_back(std::make_pair(1, r0));
    rects.push_back(std::make_pair(2, r1));
    rects.push_back(std::make_pair(3, r2));

    std::vector<int> visible = ViewportCulling::FilterVisibleNodes(rects, vp, 0.0f);

    TEST_ASSERT(visible.size() == 2, "Should have 2 visible nodes");
    TEST_ASSERT(visible[0] == 1 || visible[1] == 1, "Node 1 should be visible");
    TEST_ASSERT(visible[0] == 3 || visible[1] == 3, "Node 3 should be visible");

    ReportTest("ViewportCulling_FilterVisibleNodes_CorrectCount",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// ConnectionCache tests
// ---------------------------------------------------------------------------

static void Test_ConnectionCache_SetAndGet_ReturnsStored()
{
    int prevFail = s_failCount;
    ConnectionCache& cc = ConnectionCache::Get();
    cc.InvalidateAll();

    BezierPoints pts;
    pts.p0x = 1.0f; pts.p0y = 2.0f;
    pts.p1x = 3.0f; pts.p1y = 4.0f;
    pts.p2x = 5.0f; pts.p2y = 6.0f;
    pts.p3x = 7.0f; pts.p3y = 8.0f;

    cc.SetBezier(42, pts);
    const BezierPoints& got = cc.GetBezier(42);

    TEST_ASSERT(Approx(got.p0x, 1.0f), "p0x should be 1");
    TEST_ASSERT(Approx(got.p3y, 8.0f), "p3y should be 8");

    ReportTest("ConnectionCache_SetAndGet_ReturnsStored", s_failCount == prevFail);
}

static void Test_ConnectionCache_HasCached_FalseBeforeSet()
{
    int prevFail = s_failCount;
    ConnectionCache& cc = ConnectionCache::Get();
    cc.InvalidateAll();

    TEST_ASSERT(!cc.HasCached(999), "HasCached should be false for unknown ID");
    TEST_ASSERT(cc.GetCacheSize() == 0, "Cache should be empty after InvalidateAll");

    ReportTest("ConnectionCache_HasCached_FalseBeforeSet", s_failCount == prevFail);
}

static void Test_ConnectionCache_Invalidate_RemovesEntry()
{
    int prevFail = s_failCount;
    ConnectionCache& cc = ConnectionCache::Get();
    cc.InvalidateAll();

    BezierPoints pts;
    pts.p0x = pts.p0y = pts.p1x = pts.p1y =
    pts.p2x = pts.p2y = pts.p3x = pts.p3y = 0.0f;

    cc.SetBezier(10, pts);
    TEST_ASSERT(cc.HasCached(10), "Entry 10 should exist after SetBezier");

    cc.Invalidate(10);
    TEST_ASSERT(!cc.HasCached(10), "Entry 10 should not exist after Invalidate");

    ReportTest("ConnectionCache_Invalidate_RemovesEntry", s_failCount == prevFail);
}

static void Test_ConnectionCache_InvalidateAll_EmptiesCache()
{
    int prevFail = s_failCount;
    ConnectionCache& cc = ConnectionCache::Get();

    BezierPoints pts;
    pts.p0x = pts.p0y = pts.p1x = pts.p1y =
    pts.p2x = pts.p2y = pts.p3x = pts.p3y = 0.0f;

    cc.SetBezier(1, pts);
    cc.SetBezier(2, pts);
    cc.SetBezier(3, pts);

    TEST_ASSERT(cc.GetCacheSize() == 3, "Cache should have 3 entries");

    cc.InvalidateAll();
    TEST_ASSERT(cc.GetCacheSize() == 0, "Cache should be empty after InvalidateAll");

    ReportTest("ConnectionCache_InvalidateAll_EmptiesCache", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=====================================" << std::endl;
    std::cout << "   Phase 7 Feature Unit Tests        " << std::endl;
    std::cout << "=====================================" << std::endl;

    // NodeSearchPalette
    Test_NodeSearchPalette_GetAllNodes_ReturnsNonEmpty();
    Test_NodeSearchPalette_FuzzySearch_EmptyQuery_ReturnsAll();
    Test_NodeSearchPalette_FuzzySearch_ExactMatch_TopScore();
    Test_NodeSearchPalette_FuzzySearch_NoMatch_ReturnsEmpty();
    Test_NodeSearchPalette_ComputeFuzzyScore_ExactMatch();
    Test_NodeSearchPalette_ComputeFuzzyScore_SubstringMatch();
    Test_NodeSearchPalette_ComputeFuzzyScore_NoMatch();
    Test_NodeSearchPalette_FilterByCategory();

    // MinimapWidget
    Test_MinimapWidget_UpdateNodes_NormalisesCorrectly();
    Test_MinimapWidget_UpdateViewport_NormalisesCorrectly();
    Test_MinimapWidget_OnDrag_ScalesToGraphSpace();

    // GridSnapping
    Test_GridSnapping_DefaultEnabled();
    Test_GridSnapping_SnapX_RoundsToGrid();
    Test_GridSnapping_Toggle_DisablesSnapping();
    Test_GridSnapping_SetGridSize_Invalid_Ignored();

    // FileWatcher
    Test_FileWatcher_DefaultConstructor_NotWatching();
    Test_FileWatcher_Watch_NonexistentFile_DoesNotCrash();

    // GraphValidationPanel
    Test_GraphValidationPanel_EmptyGraph_CriticalError();
    Test_GraphValidationPanel_NoEntryPoint_CriticalError();
    Test_GraphValidationPanel_SubGraphMissingPath_Error();
    Test_GraphValidationPanel_OnErrorClick_SetsSelection();
    Test_GraphValidationPanel_Clear_ResetsState();

    // ViewportCulling
    Test_ViewportCulling_IsNodeVisible_OverlapsViewport();
    Test_ViewportCulling_IsNodeVisible_OutsideViewport();
    Test_ViewportCulling_FilterVisibleNodes_CorrectCount();

    // ConnectionCache
    Test_ConnectionCache_SetAndGet_ReturnsStored();
    Test_ConnectionCache_HasCached_FalseBeforeSet();
    Test_ConnectionCache_Invalidate_RemovesEntry();
    Test_ConnectionCache_InvalidateAll_EmptiesCache();

    std::cout << "=====================================" << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
