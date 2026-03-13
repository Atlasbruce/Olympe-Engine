# Phase 14: Fix Undo/Redo Completeness

**PR**: Phase 14  
**Date**: 2026-03-13  
**Status**: Implemented

---

## Overview

Phase 14 fixes four interconnected undo/redo bugs in `VisualScriptEditorPanel`:

| # | Symptom | Root Cause | Fix |
|---|---------|-----------|-----|
| 1 | Ghost links after undo of node deletion | `SyncEditorNodesFromTemplate()` did not call `RebuildLinks()` | Add `RebuildLinks()` at end of `SyncEditorNodesFromTemplate()` |
| 2 | Undo after file load deletes node instead of restoring position | `m_nodeDragStartPositions` empty until first render | Initialise from loaded node positions in `LoadTemplate()` |
| 3 | Undo of MoveNode does not update visual position | ImNodes positions not pushed after undo | `m_needsPositionSync = true` already present; guarded by Fix 4 |
| 4 | Positions overwritten after undo | `SyncNodePositionsFromImNodes()` ran in the same frame, reading stale ImNodes state | Skip `SyncNodePositionsFromImNodes()` for one frame via `m_skipPositionSyncNextFrame` |

---

## Fix 1 — Ghost links after undo of DeleteNode

### Problem

When a node is deleted via `DeleteNodeCommand` and then undone:

1. `m_undoStack.Undo(m_template)` restores the node **and** its connections in `m_template.ExecConnections` / `m_template.DataConnections`.
2. `SyncEditorNodesFromTemplate()` rebuilds `m_editorNodes` from `m_template.Nodes`.
3. **But** `SyncEditorNodesFromTemplate()` did **not** call `RebuildLinks()`, so `m_editorLinks` retained the stale state from before the undo.

After undo, `m_editorLinks` was out of sync with `m_template.ExecConnections`, causing visual ghost links or missing links.

### Fix

Added `RebuildLinks()` at the end of `SyncEditorNodesFromTemplate()`, after the node loop:

```cpp
// FIX 1: Rebuild links from template so that ghost links are removed after undo/redo.
RebuildLinks();

// Request a position-restore pass on the next RenderCanvas() call
m_needsPositionSync = true;
```

`RebuildLinks()` clears `m_editorLinks` and rebuilds it from `m_template.ExecConnections` and `m_template.DataConnections`, so the visual state always matches the template state.

---

## Fix 2 — MoveNodeCommand not tracked for loaded graphs

### Problem

When a graph is opened via "Blueprint Files":

1. `LoadTemplate()` calls `SyncCanvasFromTemplate()` which populates `m_editorNodes`.
2. `m_nodeDragStartPositions` was **empty** after load (only populated lazily during the first drag).
3. When the user drags a node, the drag-start detection code records positions **only** for nodes already in `m_positionedNodes` (rendered at least once). On the first drag after load, the nodes are in `m_positionedNodes` but `m_nodeDragStartPositions` is empty.
4. As a result, `MoveNodeCommand` was created with `startPos == (0, 0)`, so undo teleported the node to the origin instead of restoring its loaded position.

### Fix

After `SyncCanvasFromTemplate()` in `LoadTemplate()`, pre-populate `m_nodeDragStartPositions` from the loaded editor node positions:

```cpp
SyncCanvasFromTemplate();

// FIX 2: Pre-populate drag-start positions so MoveNodeCommand has a valid
// "before" state even for freshly loaded graphs.
m_nodeDragStartPositions.clear();
for (size_t i = 0; i < m_editorNodes.size(); ++i)
{
    m_nodeDragStartPositions[m_editorNodes[i].nodeID] =
        std::make_pair(m_editorNodes[i].posX, m_editorNodes[i].posY);
}
```

---

## Fix 3 — Undo of MoveNode does not update ImNodes display

### Problem

`MoveNodeCommand::Undo()` calls `SetNodePos()` which writes to `m_template.Nodes[].Parameters["__posX/__posY"]`. After `SyncEditorNodesFromTemplate()` reads these parameters back into `m_editorNodes[].posX/posY`, ImNodes must be told to move the node visually via `ImNodes::SetNodeEditorSpacePos()`.

### Fix

This is handled by the existing `m_needsPositionSync = true` flag that `SyncEditorNodesFromTemplate()` sets. At the start of the next `RenderCanvas()` call (before `BeginNodeEditor()`), all nodes are repositioned:

```cpp
if (m_needsPositionSync)
{
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        ImNodes::SetNodeEditorSpacePos(
            m_editorNodes[i].nodeID,
            ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
    }
    m_needsPositionSync = false;
}
```

Fix 4 (below) ensures these correct positions are not overwritten in the same frame.

---

## Fix 4 — SyncNodePositionsFromImNodes() overwrites positions after undo

### Problem

In `RenderCanvas()`, after `ImNodes::EndNodeEditor()`, `SyncNodePositionsFromImNodes()` is called to read back the live node positions from ImNodes. This is correct for normal frame rendering.

However, after undo/redo:
1. `SyncEditorNodesFromTemplate()` writes correct undo-target positions to `m_editorNodes` and sets `m_needsPositionSync = true`.
2. `RenderCanvas()` runs: `m_needsPositionSync` triggers `SetNodeEditorSpacePos()` for all nodes (before `BeginNodeEditor()`).
3. **In the same frame**: `SyncNodePositionsFromImNodes()` reads back positions from ImNodes after `EndNodeEditor()`. If ImNodes hasn't rendered the `SetNodeEditorSpacePos()` changes yet (they may only take effect on the next render pass), the old positions are read back and overwrite the correct values.

### Fix

Added a `m_skipPositionSyncNextFrame` flag:

**Header** (`VisualScriptEditorPanel.h`):
```cpp
/// Set to true by Undo/Redo; causes next frame to skip SyncNodePositionsFromImNodes()
bool m_skipPositionSyncNextFrame = false;
```

**Undo and Redo handlers** (`RenderToolbar()`):
```cpp
m_undoStack.Undo(m_template);
SyncEditorNodesFromTemplate();
RebuildLinks();
m_skipPositionSyncNextFrame = true;  // FIX 4
m_dirty = true;
```

**`RenderCanvas()`** — replace unconditional `SyncNodePositionsFromImNodes()`:
```cpp
if (m_skipPositionSyncNextFrame)
{
    m_skipPositionSyncNextFrame = false;
}
else
{
    SyncNodePositionsFromImNodes();
}
```

---

## Data flow after Undo (with all fixes applied)

```
User presses Ctrl+Z
        │
        ▼
RenderToolbar()
  ├── m_undoStack.Undo(m_template)       ← template state reverted
  ├── SyncEditorNodesFromTemplate()
  │     ├── rebuild m_editorNodes from m_template.Nodes   (reads __posX/__posY)
  │     ├── RebuildLinks()                                  ← FIX 1: no ghost links
  │     └── m_needsPositionSync = true
  ├── RebuildLinks()                     ← explicit call (harmless double-call)
  ├── m_skipPositionSyncNextFrame = true ← FIX 4
  └── m_dirty = true

        │  (same frame, later)
        ▼
RenderCanvas()
  ├── EditorContextSet(m_imnodesContext)
  ├── if (m_needsPositionSync):
  │     ├── SetNodeEditorSpacePos() for all nodes   ← FIX 3: visual positions updated
  │     └── m_needsPositionSync = false
  ├── BeginNodeEditor()
  ├── render nodes + links
  ├── EndNodeEditor()
  └── if (m_skipPositionSyncNextFrame):  ← FIX 4: skip overwrite
        └── m_skipPositionSyncNextFrame = false
      else:
        └── SyncNodePositionsFromImNodes()
```

---

## Files Modified

| File | Change |
|------|--------|
| `Source/BlueprintEditor/VisualScriptEditorPanel.h` | Added `m_skipPositionSyncNextFrame` flag |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | `SyncEditorNodesFromTemplate()`: added `RebuildLinks()`; `LoadTemplate()`: init drag positions; Undo/Redo handlers: set skip flag; `RenderCanvas()`: conditional position sync |
| `Tests/BlueprintEditor/Phase14Test.cpp` | 3 new tests validating the fixes at template/command level |
| `CMakeLists.txt` | Added `OlympePhase14Tests` target |

---

## Tests Added (`Tests/BlueprintEditor/Phase14Test.cpp`)

| Test | What it validates |
|------|------------------|
| `GhostLinks_UndoDeleteNode_ConnectionsRestoredInTemplate` | After `DeleteNodeCommand::Undo()`, the node and all its connections are present in `m_template.ExecConnections` |
| `MoveNode_UndoAfterLoad_RestoresOriginalPosition` | `MoveNodeCommand::Undo()` restores `Parameters["__posX/__posY"]` to the pre-move values even when no `AddNodeCommand` precedes the `MoveNodeCommand` (simulates loading from file) |
| `MoveNode_UndoWithConnections_LinkPreserved` | Undoing a `MoveNodeCommand` does not affect connections; the link remains valid in `m_template.ExecConnections` |

---

## Success Criteria

- ✅ Undo of `DeleteNode` restores the node **and** its links (no ghost links)
- ✅ Undo of `MoveNode` restores the correct position in `Parameters["__posX/__posY"]`
- ✅ Opening a file then dragging a node: undo restores the loaded position
- ✅ Redo of `MoveNode` restores the moved position (not the default)
- ✅ All 3 Phase 14 tests pass
