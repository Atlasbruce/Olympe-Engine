# Blueprint Graph Editing Flow

**Phase 15 — Complete Undo/Redo + Context Menus**
*Last updated: 2026-03-13*

---

## 1. Flux de Création de Graphe (New Graph)

### Entry Point
**`VisualScriptEditorPanel::RenderToolbar()` → "New Graph" button**

### Flow
1. `RenderToolbar()` resets state:
   - Clears `m_template.Nodes`, `m_editorNodes`, `m_editorLinks`
   - Resets `m_nextNodeID`, `m_nextLinkID` to 1
   - Sets `m_template.GraphType = "VisualScript"`

2. User drags node from palette onto canvas:
   - `RenderCanvas()` → drag-drop detected in `BeginDragDropTarget()`
   - Stores `m_pendingNodeDrop = true`, `m_pendingNodeType`, `m_pendingNodeX/Y`

3. After `EndNodeEditor()` (Phase 2):
   - `AddNode(type, x, y)` creates:
     - `TaskNodeDefinition` in `m_template.Nodes`
     - Writes `__posX/__posY` to `Parameters` (Phase 13 fix)
     - Pushes `AddNodeCommand` onto `m_undoStack`
     - Creates `VSEditorNode` in `m_editorNodes`
   - `ImNodes::SetNodeEditorSpacePos()` sets visual position

4. User moves node (drag with mouse):
   - `RenderCanvas()` tracks `ImNodes::GetNodeEditorSpacePos()` changes
   - First frame of drag: `m_nodeDragStartPositions[nodeID] = (oldX, oldY)`
   - On mouse release: pushes `MoveNodeCommand` onto `m_undoStack`

5. User creates link (drag pin to pin):
   - ImNodes fires `ImNodes::IsLinkCreated(&srcAttr, &dstAttr)`
   - Decode attribute UIDs → node IDs + pin names
   - Calls `ConnectExec()` → pushes `AddConnectionCommand`
   - Calls `ConnectData()` → pushes `AddDataConnectionCommand`

6. User deletes link (Ctrl+click or Delete key):
   - ImNodes fires `ImNodes::IsLinkDestroyed(&linkID)` or Delete key handler
   - Calls `RemoveLink(linkID)` → pushes `DeleteLinkCommand`

7. User presses Ctrl+Z (undo):
   - Keyboard handler detects Ctrl+Z → calls `PerformUndo()`
   - `PerformUndo()` calls `m_undoStack.Undo(m_template)`
   - Then calls `SyncEditorNodesFromTemplate()` → rebuilds `m_editorNodes`
   - Then calls `RebuildLinks()` → syncs `m_editorLinks` from template connections
   - Sets `m_skipPositionSyncNextFrame = true` to preserve restored positions

---

## 2. Flux de Chargement de Graphe (Load from File)

### Entry Point
**`EditorLayout::RenderBlueprintFileBrowser()` → User clicks .ats file**

### Flow
1. `TaskGraphLoader::LoadFromFile(path)` → returns `TaskGraphTemplate`
2. `VisualScriptEditorPanel::LoadTemplate(tmpl*, path)` called:
   - Copies template to `m_template`
   - `m_template.BuildLookupCache()`
   - Calls `SyncCanvasFromTemplate()`:
     - Creates `VSEditorNode` for each `m_template.Nodes[]`
     - Reads `__posX/__posY` from `Parameters`
     - Creates `VSEditorLink` for exec + data connections
   - Pre-populates `m_nodeDragStartPositions` with loaded positions (Phase 14 fix)
   - Sets `m_needsPositionSync = true`

3. First `RenderCanvas()` after load:
   - `m_needsPositionSync` triggers `SetNodeEditorSpacePos()` for all nodes
   - Nodes appear at loaded positions

4. User edits (moves, adds, deletes):
   - All operations push undo commands via `m_undoStack.PushCommand()`
   - Undo/Redo calls `PerformUndo()` / `PerformRedo()` which call
     `SyncEditorNodesFromTemplate()` + `RebuildLinks()` to eliminate ghost links

---

## 3. Opérations Undo/Redo — Points d'Injection

| **Operation**         | **Entry Point**                    | **Command Class**          | **Status**   |
|-----------------------|------------------------------------|----------------------------|--------------|
| Add Node              | `AddNode()`                        | `AddNodeCommand`           | ✅ Phase 12  |
| Delete Node           | `RemoveNode()`                     | `DeleteNodeCommand`        | ✅ Phase 12  |
| Move Node             | `RenderCanvas()` drag handler      | `MoveNodeCommand`          | ✅ Phase 12  |
| Add Exec Connection   | `ConnectExec()`                    | `AddConnectionCommand`     | ✅ Phase 13  |
| Add Data Connection   | `ConnectData()`                    | `AddDataConnectionCommand` | ✅ Phase 13  |
| Delete Link           | `RemoveLink()` / `IsLinkDestroyed` | `DeleteLinkCommand`        | ✅ Phase 13  |
| Undo (keyboard)       | `RenderToolbar()` Ctrl+Z handler   | calls `PerformUndo()`      | ✅ Phase 15  |
| Redo (keyboard)       | `RenderToolbar()` Ctrl+Y handler   | calls `PerformRedo()`      | ✅ Phase 15  |

### PerformUndo() / PerformRedo() — Phase 15 Fix

The key fix in Phase 15 is centralising all undo/redo side-effects in two methods:

```cpp
void VisualScriptEditorPanel::PerformUndo()
{
    if (!m_undoStack.CanUndo()) return;
    m_undoStack.Undo(m_template);
    SyncEditorNodesFromTemplate();  // Rebuilds m_editorNodes from template
    RebuildLinks();                 // Syncs m_editorLinks → eliminates ghost links
    m_skipPositionSyncNextFrame = true;
    m_dirty = true;
}

void VisualScriptEditorPanel::PerformRedo()
{
    if (!m_undoStack.CanRedo()) return;
    m_undoStack.Redo(m_template);
    SyncEditorNodesFromTemplate();
    RebuildLinks();
    m_skipPositionSyncNextFrame = true;
    m_dirty = true;
}
```

The keyboard handlers in `RenderToolbar()` call these wrappers:
```cpp
if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
    PerformUndo();
if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
    PerformRedo();
```

---

## 4. Menus Contextuels — Points d'Injection

All context menus are rendered in `RenderCanvas()` after `EndNodeEditor()`.

### Canvas Right-Click (empty space)
**Opens `"VSNodePalette"` popup**

```cpp
if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
{
    // Store canvas-space mouse position for node spawn
    m_contextMenuX = mp.x - wp.x;
    m_contextMenuY = mp.y - wp.y;
    ImGui::OpenPopup("VSNodePalette");
}
```

The palette popup (`RenderNodePalette()`) shows submenus:
- **Flow Control**: EntryPoint, Branch, Sequence, While, ForEach, DoOnce, Delay
- **Actions**: AtomicTask
- **Data**: GetBBValue, SetBBValue, MathOp
- **SubGraph**: SubGraph

All items call `AddNode(type, m_contextMenuX, m_contextMenuY)`.
**Status**: ✅ Implemented (Phase 15)

---

### Node Right-Click
**Opens `"VSNodeContextMenu"` popup**

```cpp
if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
{
    int hoveredNode = -1;
    if (ImNodes::IsNodeHovered(&hoveredNode))
    {
        m_contextNodeID = hoveredNode;
        ImGui::OpenPopup("VSNodeContextMenu");
    }
}
```

Menu items:
- **Edit Properties** → sets `m_selectedNodeID` to focus properties panel
- **Delete Node** → calls `RemoveNode(m_contextNodeID)` (undoable)
- **Add/Remove Breakpoint (F9)** → toggles via `DebugController::Get().ToggleBreakpoint()`
- **Duplicate** → calls `AddNode()` via `AddNodeCommand` (undoable copy)

**Status**: ✅ Implemented (Phase 15)

---

### Link Right-Click
**Opens `"VSLinkContextMenu"` popup**

```cpp
if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
{
    int hoveredLink = -1;
    if (ImNodes::IsLinkHovered(&hoveredLink))
    {
        m_contextLinkID = hoveredLink;
        ImGui::OpenPopup("VSLinkContextMenu");
    }
}
```

Menu items:
- **Delete Connection** → calls `RemoveLink(m_contextLinkID)` (undoable)

**Status**: ✅ Implemented (Phase 15)

---

## 5. Ghost Links — Root Cause & Fix

### Symptom
After undo of a node deletion or a move, previously removed edges (blue links)
remained visible on the canvas.

### Root Cause
`m_editorLinks` (the ImNodes visual link array) was not rebuilt after undo/redo.
The template's `ExecConnections` / `DataConnections` were correctly restored, but
`RebuildLinks()` was not called, so ImNodes kept rendering stale link objects.

### Fix (Phase 15)
`PerformUndo()` and `PerformRedo()` always call `RebuildLinks()` after modifying
the template. `RebuildLinks()` clears `m_editorLinks` and re-creates it from the
current template connections, ensuring the visual state matches the data model.

---

## 6. Undo Stack Lifecycle

| **Event**                  | **Stack action**                  |
|----------------------------|-----------------------------------|
| New graph                  | Stack NOT cleared (preserves history) |
| Load from file             | Stack NOT cleared (Phase 13 fix)  |
| `PushCommand()`            | Clears redo stack, appends to undo |
| Stack size > 100           | Oldest undo entry evicted (FIFO)  |
| `Save()` / `SaveAs()`      | Stack NOT cleared                 |

---

## 7. References

- `Source/BlueprintEditor/VisualScriptEditorPanel.h` — class definition
- `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — implementation
- `Source/BlueprintEditor/UndoRedoStack.h/.cpp` — Command pattern
- `Documentation/PHASE14_UNDO_REDO_FIXES.md` — Phase 14 context
- `Tests/BlueprintEditor/Phase15Test.cpp` — Unit tests for this phase
