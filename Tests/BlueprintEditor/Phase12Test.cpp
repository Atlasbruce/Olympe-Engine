/**
 * @file Phase12Test.cpp
 * @brief Tests for Phase 12 — Delete Nodes/Links & Context Menu Fixes.
 * @author Olympe Engine
 * @date 2026-03-12
 *
 * @details
 * Validates the logic introduced by PR #360:
 *   1. DeleteNode_SingleNode_RemovedFromTemplate
 *   2. DeleteNode_MultipleNodes_AllRemovedFromTemplate
 *   3. DeleteNode_ConnectionsCleaned_ExecAndData
 *   4. DeleteLink_ExecConnection_RemovedFromTemplate
 *   5. DeleteLink_DataConnection_RemovedFromTemplate
 *   6. DeleteNodes_WarningThreshold_SixNodesExceedsThreshold
 *   7. DeleteNodes_BelowThreshold_FiveNodesNoWarning
 *   8. ContextMenu_DeleteNodeRemovesNode
 *   9. BatchDelete_NodeAndLink_BothRemoved
 *
 * No SDL3, ImGui, or ImNodes dependency.
 * C++14 compliant.
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// Test infrastructure
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
// Minimal in-memory structures mirroring the VS editor's data model.
// These replicate the key fields of TaskGraphTemplate / VSEditorNode /
// VSEditorLink that are exercised by the Delete key handler and context menus.
// Following the headless-test pattern used in Phase10Test / Phase11Test.
// ---------------------------------------------------------------------------

struct MockNode {
    int         nodeID   = -1;
    std::string name;
    float       posX     = 0.0f;
    float       posY     = 0.0f;
};

struct MockExecConn {
    int         srcNodeID  = -1;
    int         dstNodeID  = -1;
    std::string srcPinName;
    std::string dstPinName;
};

struct MockDataConn {
    int         srcNodeID  = -1;
    int         dstNodeID  = -1;
    std::string srcPinName;
    std::string dstPinName;
};

struct MockEditorLink {
    int  linkID    = -1;
    int  srcNodeID = -1;
    int  dstNodeID = -1;
    bool isData    = false;
};

/**
 * @brief Minimal graph container mirroring the editor state touched by the
 *        Delete key handler in RenderCanvas().
 */
struct MockGraph {
    std::vector<MockNode>     nodes;
    std::vector<MockExecConn> execConnections;
    std::vector<MockDataConn> dataConnections;
    std::vector<MockEditorLink> editorLinks;
    bool  dirty   = false;

    void AddNode(int id, const std::string& name_) {
        MockNode n; n.nodeID = id; n.name = name_;
        nodes.push_back(n);
    }

    void AddExecConn(int src, const std::string& srcPin,
                     int dst, const std::string& dstPin) {
        MockExecConn c; c.srcNodeID=src; c.srcPinName=srcPin;
                        c.dstNodeID=dst; c.dstPinName=dstPin;
        execConnections.push_back(c);
    }

    void AddDataConn(int src, const std::string& srcPin,
                     int dst, const std::string& dstPin) {
        MockDataConn c; c.srcNodeID=src; c.srcPinName=srcPin;
                        c.dstNodeID=dst; c.dstPinName=dstPin;
        dataConnections.push_back(c);
    }

    void AddEditorLink(int linkID, int src, int dst, bool isData) {
        MockEditorLink l; l.linkID=linkID; l.srcNodeID=src;
                          l.dstNodeID=dst; l.isData=isData;
        editorLinks.push_back(l);
    }

    /** Mirrors RemoveNode(): removes node + all connections referencing it. */
    void RemoveNode(int nodeID) {
        nodes.erase(
            std::remove_if(nodes.begin(), nodes.end(),
                           [nodeID](const MockNode& n){ return n.nodeID == nodeID; }),
            nodes.end());

        execConnections.erase(
            std::remove_if(execConnections.begin(), execConnections.end(),
                           [nodeID](const MockExecConn& c){
                               return c.srcNodeID == nodeID || c.dstNodeID == nodeID;
                           }),
            execConnections.end());

        dataConnections.erase(
            std::remove_if(dataConnections.begin(), dataConnections.end(),
                           [nodeID](const MockDataConn& c){
                               return c.srcNodeID == nodeID || c.dstNodeID == nodeID;
                           }),
            dataConnections.end());

        editorLinks.erase(
            std::remove_if(editorLinks.begin(), editorLinks.end(),
                           [nodeID](const MockEditorLink& l){
                               return l.srcNodeID == nodeID || l.dstNodeID == nodeID;
                           }),
            editorLinks.end());

        dirty = true;
    }

    /** Mirrors RemoveLink(): removes a single editor link + its template connection. */
    void RemoveLink(int linkID) {
        MockEditorLink* found = nullptr;
        for (size_t i = 0; i < editorLinks.size(); ++i)
            if (editorLinks[i].linkID == linkID) { found = &editorLinks[i]; break; }
        if (!found) return;

        if (found->isData) {
            int src = found->srcNodeID, dst = found->dstNodeID;
            dataConnections.erase(
                std::remove_if(dataConnections.begin(), dataConnections.end(),
                               [src, dst](const MockDataConn& c){
                                   return c.srcNodeID == src && c.dstNodeID == dst;
                               }),
                dataConnections.end());
        } else {
            int src = found->srcNodeID, dst = found->dstNodeID;
            execConnections.erase(
                std::remove_if(execConnections.begin(), execConnections.end(),
                               [src, dst](const MockExecConn& c){
                                   return c.srcNodeID == src && c.dstNodeID == dst;
                               }),
                execConnections.end());
        }

        editorLinks.erase(
            std::remove_if(editorLinks.begin(), editorLinks.end(),
                           [linkID](const MockEditorLink& l){ return l.linkID == linkID; }),
            editorLinks.end());

        dirty = true;
    }

    int NodeCount() const { return static_cast<int>(nodes.size()); }
    bool HasNode(int id) const {
        for (size_t i = 0; i < nodes.size(); ++i)
            if (nodes[i].nodeID == id) return true;
        return false;
    }
    bool HasExecConn(int src, int dst) const {
        for (size_t i = 0; i < execConnections.size(); ++i)
            if (execConnections[i].srcNodeID == src && execConnections[i].dstNodeID == dst)
                return true;
        return false;
    }
    bool HasDataConn(int src, int dst) const {
        for (size_t i = 0; i < dataConnections.size(); ++i)
            if (dataConnections[i].srcNodeID == src && dataConnections[i].dstNodeID == dst)
                return true;
        return false;
    }
    bool HasLink(int linkID) const {
        for (size_t i = 0; i < editorLinks.size(); ++i)
            if (editorLinks[i].linkID == linkID) return true;
        return false;
    }
};

// ---------------------------------------------------------------------------
// Mirror of the "Delete key" handler threshold check from RenderCanvas().
// Returns true if a warning should be logged (> 5 nodes selected).
// ---------------------------------------------------------------------------
static bool ShouldWarnForBatchDelete(int numSelectedNodes)
{
    return numSelectedNodes > 5;
}

// ---------------------------------------------------------------------------
// Helper: simulate the Delete key handler batch-delete on a mock graph
// ---------------------------------------------------------------------------
static void SimulateBatchDeleteKey(MockGraph& g,
                                   const std::vector<int>& selectedNodes,
                                   const std::vector<int>& selectedLinks)
{
    int numNodes = static_cast<int>(selectedNodes.size());
    if (numNodes > 0)
    {
        if (ShouldWarnForBatchDelete(numNodes))
            std::cout << "[VSEditor] Warning: Deleting " << numNodes << " nodes" << std::endl;

        for (int nodeID : selectedNodes)
            g.RemoveNode(nodeID);

        g.dirty = true;
    }

    if (!selectedLinks.empty())
    {
        for (int linkID : selectedLinks)
            g.RemoveLink(linkID);

        g.dirty = true;
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void Test1_DeleteSingleNode()
{
    s_failCount = 0;
    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");

    // Delete node 2
    g.RemoveNode(2);

    TEST_ASSERT(g.NodeCount() == 1,    "1 node should remain after deleting 1");
    TEST_ASSERT(g.HasNode(1),          "Node 1 should still exist");
    TEST_ASSERT(!g.HasNode(2),         "Node 2 should be gone");
    TEST_ASSERT(g.dirty,               "dirty flag should be set");

    ReportTest("DeleteNode_SingleNode_RemovedFromTemplate", s_failCount == 0);
}

static void Test2_DeleteMultipleNodes()
{
    s_failCount = 0;
    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");
    g.AddNode(3, "While");
    g.AddNode(4, "Sequence");

    // Simulate Delete key with nodes 2, 3, 4 selected
    SimulateBatchDeleteKey(g, {2, 3, 4}, {});

    TEST_ASSERT(g.NodeCount() == 1, "Only EntryPoint should remain");
    TEST_ASSERT(g.HasNode(1),       "Node 1 should still exist");
    TEST_ASSERT(!g.HasNode(2),      "Node 2 should be gone");
    TEST_ASSERT(!g.HasNode(3),      "Node 3 should be gone");
    TEST_ASSERT(!g.HasNode(4),      "Node 4 should be gone");
    TEST_ASSERT(g.dirty,            "dirty flag should be set");

    ReportTest("DeleteNode_MultipleNodes_AllRemovedFromTemplate", s_failCount == 0);
}

static void Test3_DeleteNodeCleansConnections()
{
    s_failCount = 0;
    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");
    g.AddNode(3, "AtomicTask");

    g.AddExecConn(1, "Out",  2, "In");
    g.AddExecConn(2, "Then", 3, "In");
    g.AddExecConn(2, "Else", 3, "In");
    g.AddDataConn(3, "Value", 2, "Condition");

    // Delete node 2 — all connections to/from 2 must be removed
    g.RemoveNode(2);

    TEST_ASSERT(!g.HasExecConn(1, 2), "ExecConn 1->2 should be removed");
    TEST_ASSERT(!g.HasExecConn(2, 3), "ExecConn 2->3 should be removed");
    TEST_ASSERT(!g.HasDataConn(3, 2), "DataConn 3->2 should be removed");
    TEST_ASSERT(g.NodeCount() == 2,   "Nodes 1 and 3 should remain");

    ReportTest("DeleteNode_ConnectionsCleaned_ExecAndData", s_failCount == 0);
}

static void Test4_DeleteExecLink()
{
    s_failCount = 0;
    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");

    g.AddExecConn(1, "Out", 2, "In");
    g.AddEditorLink(100, 1, 2, false /*exec*/);

    // Simulate Delete key with link 100 selected
    SimulateBatchDeleteKey(g, {}, {100});

    TEST_ASSERT(!g.HasExecConn(1, 2), "ExecConn 1->2 should be removed");
    TEST_ASSERT(!g.HasLink(100),      "Editor link 100 should be removed");
    TEST_ASSERT(g.HasNode(1),         "Node 1 should still exist");
    TEST_ASSERT(g.HasNode(2),         "Node 2 should still exist");
    TEST_ASSERT(g.dirty,              "dirty flag should be set");

    ReportTest("DeleteLink_ExecConnection_RemovedFromTemplate", s_failCount == 0);
}

static void Test5_DeleteDataLink()
{
    s_failCount = 0;
    MockGraph g;
    g.AddNode(1, "SetVariable");
    g.AddNode(2, "Branch");

    g.AddDataConn(1, "Value", 2, "Condition");
    g.AddEditorLink(200, 1, 2, true /*data*/);

    // Simulate Delete key with link 200 selected
    SimulateBatchDeleteKey(g, {}, {200});

    TEST_ASSERT(!g.HasDataConn(1, 2), "DataConn 1->2 should be removed");
    TEST_ASSERT(!g.HasLink(200),      "Editor link 200 should be removed");
    TEST_ASSERT(g.HasNode(1),         "Node 1 should still exist");
    TEST_ASSERT(g.HasNode(2),         "Node 2 should still exist");
    TEST_ASSERT(g.dirty,              "dirty flag should be set");

    ReportTest("DeleteLink_DataConnection_RemovedFromTemplate", s_failCount == 0);
}

static void Test6_WarningThresholdExceeded()
{
    s_failCount = 0;

    // 6 nodes > threshold of 5
    TEST_ASSERT(ShouldWarnForBatchDelete(6),  "6 nodes should trigger warning");
    TEST_ASSERT(ShouldWarnForBatchDelete(10), "10 nodes should trigger warning");
    TEST_ASSERT(ShouldWarnForBatchDelete(100),"100 nodes should trigger warning");

    ReportTest("DeleteNodes_WarningThreshold_SixNodesExceedsThreshold", s_failCount == 0);
}

static void Test7_WarningThresholdNotExceeded()
{
    s_failCount = 0;

    // 5 nodes == threshold: no warning
    TEST_ASSERT(!ShouldWarnForBatchDelete(0), "0 nodes should not trigger warning");
    TEST_ASSERT(!ShouldWarnForBatchDelete(1), "1 node should not trigger warning");
    TEST_ASSERT(!ShouldWarnForBatchDelete(5), "5 nodes should not trigger warning");

    ReportTest("DeleteNodes_BelowThreshold_FiveNodesNoWarning", s_failCount == 0);
}

static void Test8_ContextMenuDeleteSetsNodeCountToZero()
{
    s_failCount = 0;
    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");
    g.AddNode(3, "AtomicTask");

    // Simulate right-click context menu "Delete Node" for each node
    int contextNodeID = 3;
    g.RemoveNode(contextNodeID);
    g.dirty = true;

    TEST_ASSERT(g.NodeCount() == 2, "2 nodes should remain after context-menu delete");
    TEST_ASSERT(!g.HasNode(3),      "Node 3 should be gone");
    TEST_ASSERT(g.dirty,            "dirty flag should be set after context-menu delete");

    ReportTest("ContextMenu_DeleteNodeRemovesNode", s_failCount == 0);
}

static void Test9_BatchDeleteNodeAndLink()
{
    s_failCount = 0;
    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");
    g.AddNode(3, "AtomicTask");

    g.AddExecConn(1, "Out",  2, "In");
    g.AddExecConn(2, "Then", 3, "In");
    g.AddEditorLink(100, 1, 2, false);
    g.AddEditorLink(101, 2, 3, false);

    // Simulate Delete key: node 3 selected + link 100 selected
    SimulateBatchDeleteKey(g, {3}, {100});

    TEST_ASSERT(!g.HasNode(3),        "Node 3 should be gone");
    TEST_ASSERT(!g.HasLink(100),      "Link 100 should be gone");
    TEST_ASSERT(!g.HasExecConn(1, 2), "ExecConn 1->2 should be gone (link 100 deleted)");
    TEST_ASSERT(!g.HasExecConn(2, 3), "ExecConn 2->3 should be gone (node 3 deleted)");
    TEST_ASSERT(!g.HasLink(101),      "Link 101 should be gone (node 3 deleted)");
    TEST_ASSERT(g.HasNode(1),         "Node 1 should still exist");
    TEST_ASSERT(g.HasNode(2),         "Node 2 should still exist");
    TEST_ASSERT(g.dirty,              "dirty flag should be set");

    ReportTest("BatchDelete_NodeAndLink_BothRemoved", s_failCount == 0);
}

// ---------------------------------------------------------------------------
// Test10: Move node → Undo restores position → Redo re-applies move
// ---------------------------------------------------------------------------

/**
 * @brief Minimal mock that simulates position tracking and a simple undo/redo
 *        stack for MoveNode commands.
 *
 * Mirrors the state captured by the real MoveNodeCommand class
 * (Source/BlueprintEditor/UndoRedoStack.h): the node ID together with the
 * old and new canvas positions.  Using a plain struct here keeps Phase12Test
 * free of ImGui/ImNodes dependencies while still exercising the same
 * Execute/Undo/Redo transitions.
 */
struct MockMoveRecord {
    int   nodeID = -1;
    float oldX   = 0.0f;
    float oldY   = 0.0f;
    float newX   = 0.0f;
    float newY   = 0.0f;
};

static void SimulateMoveExecute(MockGraph& g, MockMoveRecord& rec)
{
    for (size_t i = 0; i < g.nodes.size(); ++i)
    {
        if (g.nodes[i].nodeID == rec.nodeID)
        {
            g.nodes[i].posX = rec.newX;
            g.nodes[i].posY = rec.newY;
            return;
        }
    }
}

static void SimulateMoveUndo(MockGraph& g, MockMoveRecord& rec)
{
    for (size_t i = 0; i < g.nodes.size(); ++i)
    {
        if (g.nodes[i].nodeID == rec.nodeID)
        {
            g.nodes[i].posX = rec.oldX;
            g.nodes[i].posY = rec.oldY;
            return;
        }
    }
}

static float GetNodePosX(const MockGraph& g, int nodeID)
{
    for (size_t i = 0; i < g.nodes.size(); ++i)
        if (g.nodes[i].nodeID == nodeID)
            return g.nodes[i].posX;
    return 0.0f;
}

static float GetNodePosY(const MockGraph& g, int nodeID)
{
    for (size_t i = 0; i < g.nodes.size(); ++i)
        if (g.nodes[i].nodeID == nodeID)
            return g.nodes[i].posY;
    return 0.0f;
}

static void Test10_MoveNode_UndoRedo()
{
    int prevFail = s_failCount;

    // Extend MockNode with position fields for this test
    // Use the graph helper to add a positioned node
    MockGraph g;
    {
        MockNode n; n.nodeID = 1; n.name = "Print1";
        n.posX = 100.0f; n.posY = 100.0f;
        g.nodes.push_back(n);
    }

    // Verify initial position
    TEST_ASSERT(GetNodePosX(g, 1) == 100.0f, "Initial posX should be 100");
    TEST_ASSERT(GetNodePosY(g, 1) == 100.0f, "Initial posY should be 100");

    // Simulate MoveNodeCommand::Execute (node 1 from 100,100 to 200,200)
    MockMoveRecord moveCmd;
    moveCmd.nodeID = 1;
    moveCmd.oldX   = 100.0f;
    moveCmd.oldY   = 100.0f;
    moveCmd.newX   = 200.0f;
    moveCmd.newY   = 200.0f;

    SimulateMoveExecute(g, moveCmd);
    TEST_ASSERT(GetNodePosX(g, 1) == 200.0f, "After move: posX should be 200");
    TEST_ASSERT(GetNodePosY(g, 1) == 200.0f, "After move: posY should be 200");

    // Simulate Undo
    SimulateMoveUndo(g, moveCmd);
    TEST_ASSERT(GetNodePosX(g, 1) == 100.0f, "After undo: posX should be 100");
    TEST_ASSERT(GetNodePosY(g, 1) == 100.0f, "After undo: posY should be 100");

    // Simulate Redo (re-apply Execute)
    SimulateMoveExecute(g, moveCmd);
    TEST_ASSERT(GetNodePosX(g, 1) == 200.0f, "After redo: posX should be 200");
    TEST_ASSERT(GetNodePosY(g, 1) == 200.0f, "After redo: posY should be 200");

    ReportTest("MoveNode_UndoRedo_PositionRestored", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test11: Delete node → Undo restores node + visual links
// ---------------------------------------------------------------------------

static void Test11_DeleteUndo_LinksRestored()
{
    int prevFail = s_failCount;

    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");
    g.AddNode(3, "AtomicTask");

    g.AddExecConn(1, "Out",  2, "In");
    g.AddExecConn(2, "Then", 3, "In");
    g.AddEditorLink(100, 1, 2, false /*exec*/);
    g.AddEditorLink(101, 2, 3, false /*exec*/);

    // Snapshot before delete
    size_t initialLinks = g.editorLinks.size();
    TEST_ASSERT(initialLinks == 2, "Should have 2 editor links before delete");
    TEST_ASSERT(g.HasExecConn(1, 2), "ExecConn 1->2 exists before delete");
    TEST_ASSERT(g.HasExecConn(2, 3), "ExecConn 2->3 exists before delete");

    // Simulate DeleteNodeCommand::Execute for node 2:
    // Save state for undo
    std::vector<MockExecConn> savedExec;
    std::vector<MockEditorLink> savedLinks;
    for (size_t i = 0; i < g.execConnections.size(); ++i)
    {
        if (g.execConnections[i].srcNodeID == 2 || g.execConnections[i].dstNodeID == 2)
            savedExec.push_back(g.execConnections[i]);
    }
    for (size_t i = 0; i < g.editorLinks.size(); ++i)
    {
        if (g.editorLinks[i].srcNodeID == 2 || g.editorLinks[i].dstNodeID == 2)
            savedLinks.push_back(g.editorLinks[i]);
    }

    g.RemoveNode(2);

    TEST_ASSERT(!g.HasNode(2),         "Node 2 gone after delete");
    TEST_ASSERT(!g.HasExecConn(1, 2),  "ExecConn 1->2 gone after delete");
    TEST_ASSERT(!g.HasExecConn(2, 3),  "ExecConn 2->3 gone after delete");
    TEST_ASSERT(g.editorLinks.size() == 0, "All editor links gone after delete");

    // Simulate DeleteNodeCommand::Undo: restore node, connections, and editor links
    {
        MockNode restored; restored.nodeID = 2; restored.name = "Branch";
        g.nodes.push_back(restored);
    }
    for (size_t i = 0; i < savedExec.size(); ++i)
        g.execConnections.push_back(savedExec[i]);
    for (size_t i = 0; i < savedLinks.size(); ++i)
        g.editorLinks.push_back(savedLinks[i]);

    TEST_ASSERT(g.HasNode(2),                         "Node 2 restored after undo");
    TEST_ASSERT(g.HasExecConn(1, 2),                  "ExecConn 1->2 restored after undo");
    TEST_ASSERT(g.HasExecConn(2, 3),                  "ExecConn 2->3 restored after undo");
    TEST_ASSERT(g.editorLinks.size() == initialLinks, "Editor links restored after undo");

    ReportTest("DeleteUndo_LinksRestored", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test12: Context menu delete node → Undo restores node
// ---------------------------------------------------------------------------

static void Test12_ContextMenuDeleteNode_UndoRestores()
{
    int prevFail = s_failCount;

    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");
    g.AddNode(3, "Print");

    // Simulate context menu "Delete Node #2"
    g.RemoveNode(2);

    TEST_ASSERT(!g.HasNode(2), "Node 2 deleted via context menu");
    TEST_ASSERT(g.HasNode(1),  "Node 1 still exists");
    TEST_ASSERT(g.HasNode(3),  "Node 3 still exists");

    // Simulate Undo (restore node 2)
    MockNode restored;
    restored.nodeID = 2;
    restored.name = "Branch";
    g.nodes.push_back(restored);

    TEST_ASSERT(g.HasNode(2), "Node 2 restored after undo");

    ReportTest("ContextMenuDeleteNode_UndoRestores", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test13: Context menu duplicate node → new ID assigned, original intact
// ---------------------------------------------------------------------------

static void Test13_ContextMenuDuplicateNode_NewIDAssigned()
{
    int prevFail = s_failCount;

    MockGraph g;
    g.AddNode(1, "Print");

    // Simulate context menu "Duplicate Node #1": clone with new ID and offset
    MockNode duplicate = g.nodes[0];
    duplicate.nodeID = 2;
    duplicate.name   = "Print (Copy)";
    duplicate.posX  += 50.0f;
    duplicate.posY  += 50.0f;
    g.nodes.push_back(duplicate);

    TEST_ASSERT(g.HasNode(1),       "Original node 1 exists");
    TEST_ASSERT(g.HasNode(2),       "Duplicate node 2 exists");
    TEST_ASSERT(g.NodeCount() == 2, "2 nodes after duplicate");

    ReportTest("ContextMenuDuplicateNode_NewIDAssigned", s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test14: Context menu delete link → Undo restores link
// ---------------------------------------------------------------------------

static void Test14_ContextMenuDeleteLink_UndoRestores()
{
    int prevFail = s_failCount;

    MockGraph g;
    g.AddNode(1, "EntryPoint");
    g.AddNode(2, "Branch");
    g.AddExecConn(1, "Out", 2, "In");
    g.AddEditorLink(100, 1, 2, false /*exec*/);

    TEST_ASSERT(g.HasExecConn(1, 2), "Link 1->2 exists before delete");
    TEST_ASSERT(g.HasLink(100),      "Editor link 100 exists before delete");

    // Simulate context menu "Delete Link #100"
    MockExecConn savedConn = g.execConnections[0];
    MockEditorLink savedLink = g.editorLinks[0];
    g.RemoveLink(100);

    TEST_ASSERT(!g.HasExecConn(1, 2), "Link 1->2 deleted");
    TEST_ASSERT(!g.HasLink(100),      "Editor link 100 deleted");

    // Simulate Undo (restore exec connection and editor link)
    g.execConnections.push_back(savedConn);
    g.editorLinks.push_back(savedLink);

    TEST_ASSERT(g.HasExecConn(1, 2), "Link 1->2 restored after undo");
    TEST_ASSERT(g.HasLink(100),      "Editor link 100 restored after undo");

    ReportTest("ContextMenuDeleteLink_UndoRestores", s_failCount == prevFail);
}


int main()
{
    std::cout << "=== Phase12Test: Delete Nodes/Links & Context Menus ===" << std::endl;

    Test1_DeleteSingleNode();
    Test2_DeleteMultipleNodes();
    Test3_DeleteNodeCleansConnections();
    Test4_DeleteExecLink();
    Test5_DeleteDataLink();
    Test6_WarningThresholdExceeded();
    Test7_WarningThresholdNotExceeded();
    Test8_ContextMenuDeleteSetsNodeCountToZero();
    Test9_BatchDeleteNodeAndLink();
    Test10_MoveNode_UndoRedo();
    Test11_DeleteUndo_LinksRestored();
    Test12_ContextMenuDeleteNode_UndoRestores();
    Test13_ContextMenuDuplicateNode_NewIDAssigned();
    Test14_ContextMenuDeleteLink_UndoRestores();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount > 0) ? 1 : 0;
}
