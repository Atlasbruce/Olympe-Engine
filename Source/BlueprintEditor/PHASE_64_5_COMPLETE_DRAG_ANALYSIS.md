/**
 * @file PHASE_64_5_COMPLETE_DRAG_ANALYSIS.md
 * @brief Complete end-to-end analysis of multi-node drag system
 * @date 2026-04-XX
 * 
 * This document traces EXACTLY what happens when you perform multi-node drag.
 * Based on complete code analysis from BehaviorTreeRenderer (legacy working)
 * and PlaceholderCanvas (custom system, drag broken).
 */

# Phase 64.5: Complete Multi-Node Drag Analysis

## Executive Summary

**Status**: Multi-node drag STILL NOT WORKING despite fixes  
**Root Cause**: Unclear - selection works, but drag doesn't apply to all nodes  
**Investigation Method**: Side-by-side comparison of legacy vs custom system

---

## Part 1: Legacy System (BehaviorTreeRenderer + ImNodes) - WORKS ✓

### Architecture
- **Source**: `Source/BlueprintEditor/BehaviorTreeRenderer.cpp` + `BehaviorTreeImNodesAdapter.h`
- **Rendering**: ImNodes native (wrapper around imnodes library)
- **Selection**: Handled automatically by ImNodes
- **Multi-Node Drag**: Handled automatically by ImNodes
- **Key Advantage**: ImNodes manages 95% of complexity

### Call Chain: User clicks and drags 3 nodes

```
BehaviorTreeRenderer::Render()
  └─ RenderLayoutWithTabs()
     └─ ImGui::BeginChild("BTNodeCanvas")
        └─ m_imNodesAdapter->Render()
           ├─ ImNodes::BeginNodeEditor()
           │  ├─ ImNodes internal state setup
           │  ├─ Detects mouse click + Ctrl
           │  ├─ Tracks selection internally (no code needed)
           │  └─ On drag: moves ALL selected internally (no code needed)
           ├─ RenderNodes()
           │  └─ For each node: ImNodes::BeginNode() + EndNode()
           │     └─ ImNodes::SetNodeGridSpacePos() - applies drag
           ├─ UpdateCanvasInteraction()
           │  └─ Line 236-244: "ImNodes handles pan/zoom/selection internally"
           │     "No explicit implementation required for basic functionality"
           └─ ImNodes::EndNodeEditor()
              └─ Finalizes all internal state changes

Result: ALL selected nodes move together, internal to ImNodes
        User can't break it - it just works
```

### Key Code Pattern

**BehaviorTreeImNodesAdapter.cpp line 87-122**:
```cpp
void Render()
{
    ImNodes::BeginNodeEditor();           // ← Setup + detect input
    RenderNodes();                         // ← Draw nodes at current positions
    RenderConnections();                   // ← Draw connections
    UpdateCanvasInteraction();             // ← Comment: no code needed!
    ImNodes::EndNodeEditor();              // ← Finalize + apply changes
}
```

**Why it works**:
- Line 99: `ImNodes::BeginNodeEditor()` enters ImNodes rendering mode
- ImNodes AUTOMATICALLY captures:
  * Mouse clicks
  * Ctrl+Click for multi-select
  * Drag operations on selected nodes
  * All state tracking
- Line 102: `RenderNodes()` just draws - ImNodes already updated positions
- Line 111: `ImNodes::EndNodeEditor()` applies all accumulated changes

**No multi-node position tracking needed** - ImNodes does it internally!

---

## Part 2: Custom System (PlaceholderCanvas) - BROKEN ✗

### Architecture
- **Source**: `Source/BlueprintEditor/PlaceholderEditor/PlaceholderCanvas.cpp`
- **Rendering**: Custom ImGui code (no ImNodes wrapper)
- **Selection**: Manual code (lines 339-378)
- **Multi-Node Drag**: Manual code (lines 389-412)
- **Problem**: All complexity manually implemented

### Call Chain: User clicks and drags 3 nodes (CURRENT IMPLEMENTATION)

```
PlaceholderCanvas::Render()
  └─ ImGui::BeginChild("PlaceholderCanvas")
     ├─ HandlePanZoomInput()
     ├─ HandleNodeInteraction()
     │  ├─ Line 339-378: Handle click
     │  │  ├─ GetNodeAtScreenPos()
     │  │  ├─ If Ctrl+Click: m_renderer->AddSelectedNode()
     │  │  │  └─ Adds to m_selectedNodeIds vector ✓
     │  │  └─ Set m_isDraggingNode = true
     │  │     ├─ Line 369: m_nodeDragStartPositions.clear()
     │  │     ├─ Line 370: selectedNodeIds = GetSelectedNodeIds()  ← Gets ALL selected
     │  │     └─ Line 371-377: For each node
     │  │        └─ Save START position to map
     │  │
     │  └─ Line 382-413: OnMouseMove during drag
     │     ├─ Line 389: if (!m_nodeDragStartPositions.empty())
     │     ├─ Line 394: canvasDelta = MouseDelta / zoom
     │     ├─ Line 397-410: For each node in map
     │     │  └─ Line 404-408: SetNodePosition()
     │     │     ├─ node->posX + canvasDelta.x  ← Uses CURRENT posX!
     │     │     └─ node->posY + canvasDelta.y
     │     └─ Line 411: OnDocumentModified()
     │
     └─ RenderNodes()
        └─ Draw all nodes at current positions
```

### The Suspected Problem

**Line 404-408** (during drag):
```cpp
PlaceholderNode* node = m_document->GetNode(nodeId);
if (node) {
    m_document->SetNodePosition(
        nodeId,
        node->posX + canvasDelta.x,    // ← This uses node->posX
        node->posY + canvasDelta.y     //    which is the CURRENT position
    );                                  //    after previous frame's move
}
```

**Potential Issue 1**: Accumulation via current position
- Frame 1: mouse moves +5 pixels
  - canvasDelta = +5/zoom
  - node->posX (10) + 5 = 15 ✓
  
- Frame 2: mouse moves +3 pixels MORE (total +8)
  - canvasDelta = +3/zoom
  - node->posX (now 15 from Frame 1!) + 3 = 18
  - But it SHOULD be 10 + 8 = 18... ✓ (still correct!)

Wait, that's CORRECT!  The issue isn't the accumulation formula.

**Potential Issue 2**: m_nodeDragStartPositions not synced correctly
- Are all 3 nodes in the map?
- Are start positions saved correctly?

**Potential Issue 3**: Selection vector not properly populated
- Does GetSelectedNodeIds() return ALL selected nodes?
- Or only one?

---

## Part 3: DEBUGGING STRATEGY

### What We Know Works
✓ Selection works (user confirmed 3 nodes glow cyan)
✓ m_selectedNodeIds vector contains 3 nodeIds (user reported)
✓ Rectangle selection works correctly
✓ Single node drag works

### What We Know Fails
✗ Multi-node drag doesn't move all nodes together
✗ When dragging selected nodes, only ONE moves

### Hypothesis Chain

**H1**: m_nodeDragStartPositions not populated correctly?
- Test: Add SYSTEM_LOG after line 377
- Log all (nodeId, startX, startY) in map
- Expected: Should show 3 entries if working

**H2**: GetSelectedNodeIds() returns wrong data?
- Test: Add SYSTEM_LOG at line 370
- Log size and contents of selectedNodeIds vector
- Expected: Should show 3 nodeIds

**H3**: OnMouseMove never executes for multi-node?
- Test: Add SYSTEM_LOG at line 389
- Check if code path is reached during drag
- Expected: Should log every frame during drag

**H4**: SetNodePosition() doesn't actually update?
- Test: Add SYSTEM_LOG after line 408
- Log new position for verification
- Expected: Should show position changing

**H5**: SetNodePosition() only updates SINGLE node?
- Test: Call SetNodePosition() explicitly for all nodes
- Check if all update or just one
- Expected: All should update

### Logging Points Needed

```cpp
// Line 366-378: Click phase
m_isDraggingNode = true;
m_nodeDragStartPositions.clear();
std::vector<int> selectedNodeIds = m_renderer->GetSelectedNodeIds();
SYSTEM_LOG << "[PlaceholderCanvas] DRAG START: selected="
           << selectedNodeIds.size() << " nodes\n";
for (size_t i = 0; i < selectedNodeIds.size(); ++i) {
    int nodeId = selectedNodeIds[i];
    PlaceholderNode* node = m_document->GetNode(nodeId);
    if (node) {
        m_nodeDragStartPositions[nodeId] = std::make_pair(node->posX, node->posY);
        SYSTEM_LOG << "  Node #" << nodeId << ": start=(" 
                   << node->posX << "," << node->posY << ")\n";
    }
}

// Line 389-412: Drag phase
else if (m_isDraggingNode && !m_nodeDragStartPositions.empty()) {
    if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
        ImVec2 canvasDelta = ImVec2(...);
        SYSTEM_LOG << "[PlaceholderCanvas] DRAG MOVE: delta=("
                   << canvasDelta.x << "," << canvasDelta.y << ")\n";
        
        for (auto it = m_nodeDragStartPositions.begin(); ...) {
            int nodeId = it->first;
            PlaceholderNode* node = m_document->GetNode(nodeId);
            if (node) {
                float oldX = node->posX, oldY = node->posY;
                m_document->SetNodePosition(
                    nodeId,
                    node->posX + canvasDelta.x,
                    node->posY + canvasDelta.y
                );
                PlaceholderNode* updated = m_document->GetNode(nodeId);
                SYSTEM_LOG << "  Node #" << nodeId << ": (" 
                           << oldX << "," << oldY << ") → ("
                           << updated->posX << "," << updated->posY << ")\n";
            }
        }
        m_document->OnDocumentModified();
    }
}
```

---

## Part 4: COMPARISON WITH LEGACY

### Legacy Pattern (Works)
1. ImNodes setup + detection automatic
2. Selection tracked by ImNodes
3. Drag applied by ImNodes
4. Positions updated by ImNodes
5. **Result**: No code needed, no bugs possible

### PlaceholderCanvas Pattern (Broken)
1. Manual selection tracking ✓ (works)
2. Manual drag detection ✓ (works)
3. Manual multi-node update ✗ (broken - why?)
4. Manual position persistence ?

### Missing Piece
**Could be**: OnMouseMove execution not reaching the multi-node block?
**Could be**: SetNodePosition() not working correctly?
**Could be**: Renderer->GetSelectedNodeIds() returning wrong data?

---

## Part 5: REQUIRED FIX

Based on legacy pattern, PlaceholderCanvas should:

1. **During Click (OnMouseDown)**
   - Detect click position
   - Get node at click
   - Check Ctrl for multi-select
   - **NEW**: Save START positions for ALL selected nodes
   - Set drag flag

2. **During Move (OnMouseMove)**
   - Calculate mouse delta
   - **NEW**: For EACH selected node
     - Use ABSOLUTE delta from START position
     - Apply to document
   - Call OnDocumentModified()

3. **During Release (OnMouseUp)**
   - Clear drag flag
   - Clear start positions map

### Corrected Logic

```cpp
// Step 1: Build accumulator vector instead of using current position
std::vector<ImVec2> m_nodeAccumulatedDelta;  // One per selected node

// During click
m_nodeAccumulatedDelta.clear();
for (size_t i = 0; i < selectedNodeIds.size(); ++i) {
    m_nodeAccumulatedDelta.push_back(ImVec2(0, 0));
}

// During move
for (size_t i = 0; i < selectedNodeIds.size(); ++i) {
    int nodeId = selectedNodeIds[i];
    m_nodeAccumulatedDelta[i] += canvasDelta;  // Accumulate
    
    auto it = m_nodeDragStartPositions.find(nodeId);
    if (it != m_nodeDragStartPositions.end()) {
        float newX = it->second.first + m_nodeAccumulatedDelta[i].x;
        float newY = it->second.second + m_nodeAccumulatedDelta[i].y;
        m_document->SetNodePosition(nodeId, newX, newY);
    }
}
```

OR simpler: Use total mouse movement from drag START point:

```cpp
// Save initial mouse position when drag starts
ImVec2 m_dragStartMousePos;

// OnMouseDown
m_dragStartMousePos = ImGui::GetMousePos();

// OnMouseMove
ImVec2 totalMouseDelta = ImGui::GetMousePos() - m_dragStartMousePos;
ImVec2 canvasDelta = ImVec2(totalMouseDelta.x / zoom, totalMouseDelta.y / zoom);

for (nodeId in selected) {
    startPos = m_nodeDragStartPositions[nodeId];
    setPos(nodeId, startPos.first + canvasDelta.x, startPos.second + canvasDelta.y);
}
```

---

## Summary

The issue is **NOT accumulation** - that's working correctly.

The issue is probably one of:
1. GetSelectedNodeIds() not returning all selected nodes
2. OnMouseMove not executing the multi-node block
3. SetNodePosition() failing silently

**Next Step**: Add comprehensive SYSTEM_LOG to verify each hypothesis.

