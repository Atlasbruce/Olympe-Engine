# Olympe Blueprint Editor — UX Guidelines

**Document** : `Documentation/OLYMPE_EDITOR_UX_GUIDELINES.md`  
**Version** : 1.0  
**Date** : 2026-03-11  
**Status** : Living document — update whenever a new UX decision is made  
**Audience** : Engine developers, UI contributors, QA, new team members

---

## Table of Contents

1. [Terminology & Conventions](#1-terminology--conventions)
2. [Editor Layout & Panel Docking](#2-editor-layout--panel-docking)
3. [Multi-Document Interface (MDI) & Tab Management](#3-multi-document-interface-mdi--tab-management)
4. [Graph Canvas Behavior](#4-graph-canvas-behavior)
5. [File Operations CRUD](#5-file-operations-crud)
6. [Dirty State Management](#6-dirty-state-management)
7. [Context Menus & CRUD on Graph Elements](#7-context-menus--crud-on-graph-elements)
8. [Keyboard Shortcuts](#8-keyboard-shortcuts)
9. [Undo / Redo System](#9-undo--redo-system)
10. [Asset / File Browser Integration](#10-asset--file-browser-integration)
11. [Unified Graph Type Support](#11-unified-graph-type-support)
12. [Error Handling & Validation](#12-error-handling--validation)
13. [Known Bugs & How These Guidelines Address Them](#13-known-bugs--how-these-guidelines-address-them)
14. [Do's and Don'ts Quick Reference](#14-dos-and-donts-quick-reference)
15. [Version History](#15-version-history)

---

## 1. Terminology & Conventions

| Term | Definition |
|------|-----------|
| **Graph** | A single editable document (Visual Script, Behavior Tree, Anim Graph, etc.) |
| **Tab** | The UI widget in the tab bar that represents one open graph |
| **Canvas** | The main ImNodes drawing area where nodes and links are displayed |
| **Dirty / Modified** | A graph that has unsaved changes (flagged by `isDirty = true`) |
| **Clean / Saved** | A graph whose on-disk state matches the in-memory state |
| **Untitled-N** | Placeholder display name for a new, never-saved graph |
| **TabID** | Internal opaque string identifier for a tab (e.g. `"tab_3"`) |
| **Renderer** | A class implementing `IGraphRenderer` that owns the per-tab ImNodes context |
| **ImNodes context** | Per-tab `ImNodesEditorContext*` that stores node positions, pan, and zoom |

### Language rules in this document

- **SHALL** — mandatory requirement; must be implemented
- **SHOULD** — recommended; deviate only with justification
- **MAY** — optional, left to developer judgment
- **MUST NOT** — absolute prohibition

---

## 2. Editor Layout & Panel Docking

### 2.1 Reference Layout

```
┌─────────────────────────────────────────────────────────────────┐
│  [File] [Edit] [View] [Graph] [Help]          [Save] [Run]      │  ← Menu bar
├──────────────┬──────────────────────────────────────────────────┤
│              │  [guard_logic.ats] [Untitled-1 *] [patrol.bt] [+]│  ← Tab bar
│  Blueprint   ├──────────────────────────────────────────────────┤
│  Files       │                                                  │
│  ────────    │              Graph Canvas                        │
│  AI/         │           (ImNodes viewport)                     │
│   guard_…    │                                                  │
│   patrol.…   │                                                  │
│  BT/         │                                                  │
│   …          │                                                  │
├──────────────┤                                                  │
│  Inspector   │                                                  │
│  ────────    │                                                  │
│  (selected   │                                                  │
│   node       │                                                  │
│   props)     │                                                  │
└──────────────┴──────────────────────────────────────────────────┘
```

### 2.2 Panel Responsibilities

| Panel | Location | Content |
|-------|----------|---------|
| **Asset Browser / Blueprint Files** | Left — top | File tree of all `.ats` / `.json` graphs |
| **Inspector** | Left — bottom (resizable splitter) | Properties of the currently selected node |
| **Tab Bar** | Center — top | All open graphs; active tab highlighted |
| **Graph Canvas** | Center — main area | ImNodes canvas for the active tab |
| **Properties** | Right (optional, collapsible) | Node-type-specific extended properties |

### 2.3 Layout Rules

- The Inspector panel SHALL be placed **bottom-left**, below the Asset Browser (not right). This matches the Unreal Blueprint golden standard and keeps the canvas unobstructed.
- All panel separators SHALL be **resizable** via a draggable splitter.
- Panels MAY be collapsed to an icon strip. The canvas SHALL expand to fill the released space.
- Panel widths SHALL be **persisted** in `blueprint_editor_config.json` between sessions.
- The menu bar Save button (`[Save]`) SHALL be **disabled** (grayed out) when no tab is open or the active tab is clean.

---

## 3. Multi-Document Interface (MDI) & Tab Management

### 3.1 Tab Bar Overview

```
[guard_logic.ats] [Untitled-1 *] [patrol.bt] [player_anim.ag] [+]
       ↑                ↑              ↑              ↑          ↑
    Saved/Clean     Unsaved         BT graph      Anim Graph  New tab
                   (dirty flag)
```

- **`*`** after a name signals an unsaved (dirty) graph.
- The active tab is visually highlighted (ImGui default selection colour).
- The **`[+]`** button at the right end creates a new empty Visual Script tab.

### 3.2 Naming Convention

| Situation | Display Name |
|-----------|-------------|
| New, never-saved graph | `Untitled-1`, `Untitled-2`, … (monotonically increasing) |
| Graph loaded from file | Filename including extension, e.g. `guard_logic.ats` |
| Graph saved for the first time | Updated to the chosen filename immediately |
| Graph in "Save As" dialog | Pre-filled with the current tab display name |

The counter N in `Untitled-N` SHALL **never reset** during a session, ensuring every new tab gets a unique name even after previous untitled tabs are closed.

### 3.3 Tab State Indicators

```
[guard_logic.ats]    ← Clean: no asterisk
[Untitled-1 *]       ← Dirty: asterisk appended
[patrol.bt •]        ← Future: dot for "open but read-only" (optional)
```

The dirty indicator SHALL be applied by `TabManager::RenderTabBar()` by checking `tab.renderer->IsDirty()` on every frame before building the tab label string.

**Implementation reference** (`TabManager.cpp`):
```cpp
// CORRECT — checked every frame
std::string label = tab.displayName;
if (tab.isDirty)
    label += " *";
label += "###tab_" + tab.tabID;   // ImGui ID separator keeps label stable
```

### 3.4 Opening a New Tab

1. User clicks `[+]` in the tab bar  **or** uses `Ctrl+N`.
2. A new `EditorTab` is created with `displayName = "Untitled-N"` (next N).
3. A `VisualScriptRenderer` (default type) is instantiated and assigned to the tab.
4. The new tab becomes the **active** tab.
5. The existing open tabs are **not affected** — they keep their state, positions, and viewport.

**SHALL NOT**: Overwrite or replace the currently active tab.

### 3.5 Switching Between Tabs

Switching tabs SHALL:
1. Set `m_activeTabID` to the selected tab.
2. Call `ImNodes::EditorContextSet(tab.renderer->GetImNodesContext())` to restore the tab's specific canvas state (node positions, pan, zoom).
3. Restore the selection state for nodes/links that were selected before the switch.

```
User clicks [patrol.bt]
       ↓
TabManager::SetActiveTab("tab_3")
       ↓
m_pendingSelectTabID = "tab_3"    ← triggers ImGuiTabItemFlags_SetSelected
       ↓
Next frame: RenderTabBar() consumes m_pendingSelectTabID
       ↓
RenderActiveCanvas() calls tab->renderer->Render()
       ↓
Inside VisualScriptRenderer::Render():
    ImNodes::EditorContextSet(m_imnodesContext);  ← CRITICAL
    ImNodes::BeginNodeEditor();
    ...
```

**Critical rule**: `ImNodes::EditorContextSet()` SHALL be called at the **start** of every `Render()` call, before `ImNodes::BeginNodeEditor()`. Omitting this call is the root cause of nodes jumping to (0,0).

### 3.6 Tab Close Workflow

```
User clicks [X] on tab
        ↓
    isDirty ?
   /          \
 No            Yes
  ↓             ↓
DestroyTab   Defer modal: m_pendingCloseTabID = tabID
  ↓             ↓
Done         Next frame: Show modal dialog
                  ┌──────────────────────────────────────┐
                  │ "guard_logic.ats" has unsaved changes. │
                  │ Do you want to save before closing?   │
                  ├──────────────────────────────────────┤
                  │  [Save]   [Don't Save]   [Cancel]    │
                  └──────────────────────────────────────┘
                  /          |              \
            Save first   Close now       Abort close
```

- **Save**: Calls `SaveActiveTab()`, then `DestroyTab()`.
- **Don't Save**: Calls `DestroyTab()` immediately.
- **Cancel**: Leaves the tab open with its current dirty state.

When the closed tab was the **active** tab, the editor SHALL automatically activate the next tab in order (or the previous one if the closed tab was the last).

### 3.7 Exit / Close All Workflow

```
User quits the application
        ↓
   HasDirtyTabs() ?
  /                \
 No                 Yes
  ↓                  ↓
Exit              Show "Multiple Unsaved Changes" dialog:
                  ┌────────────────────────────────────────────┐
                  │ Multiple Unsaved Changes                   │
                  ├────────────────────────────────────────────┤
                  │  The following graphs have unsaved changes:│
                  │    • guard_logic.ats                       │
                  │    • Untitled-1                            │
                  ├────────────────────────────────────────────┤
                  │  [Save All]  [Discard All]  [Cancel]       │
                  └────────────────────────────────────────────┘
                  /             |              \
           Save each        Close all       Abort exit
           then exit        immediately
```

- **Save All**: Iterates dirty tabs; shows individual "Save As" for each untitled one.
- **Discard All**: Destroys all tabs without saving.
- **Cancel**: Returns the user to the editor with all tabs intact.

### 3.8 Tab Re-ordering

Tabs SHALL be re-orderable by drag-and-drop (enabled via `ImGuiTabBarFlags_Reorderable`). Re-ordering SHALL NOT affect tab state or dirty flags.

---

## 4. Graph Canvas Behavior

### 4.1 Per-Tab ImNodes Context

Each tab SHALL own its own `ImNodesEditorContext*`, created during renderer construction:

```cpp
// CORRECT — context created once per renderer instance
VisualScriptRenderer::VisualScriptRenderer()
{
    m_imnodesContext = ImNodes::EditorContextCreate();
}

VisualScriptRenderer::~VisualScriptRenderer()
{
    if (m_imnodesContext)
        ImNodes::EditorContextDestroy(m_imnodesContext);
}
```

```cpp
// CORRECT — context restored at the start of every render call
void VisualScriptRenderer::Render()
{
    ImNodes::EditorContextSet(m_imnodesContext);   // ← restore this tab's state
    ImNodes::BeginNodeEditor();
    // ... render nodes, links ...
    ImNodes::EndNodeEditor();
}
```

**WRONG pattern** — sharing a global context between tabs:
```cpp
// BAD — global context loses positions/pan/zoom when tabs switch
ImNodes::BeginNodeEditor();   // ← uses whatever context is active globally
```

### 4.2 Node Position Persistence

Node positions in the ImNodes context are stored as part of `m_imnodesContext`. When `Save()` is called, the editor SHALL serialize each node's position into the JSON file using the `__posX` / `__posY` convention:

```json
{
  "id": 3,
  "nodeType": "Sequence",
  "__posX": 240.5,
  "__posY": 120.0
}
```

When `Load()` is called, positions SHALL be applied via `ImNodes::SetNodeEditorSpacePos()` immediately after `ImNodes::BeginNodeEditor()`:

```cpp
// Called once per node after BeginNodeEditor()
ImNodes::SetNodeEditorSpacePos(node.id, ImVec2(node.posX, node.posY));
```

If a saved file contains no position data, nodes SHALL be placed in a deterministic grid layout (auto-layout fallback) rather than all at (0, 0).

### 4.3 Viewport (Pan & Zoom) Persistence

- Viewport pan and zoom are part of `ImNodesEditorContext` and are **automatically preserved** by the per-tab context mechanism.
- When a tab is first opened, the viewport SHALL be reset to show the graph's bounding box (auto-fit).
- After a tab has been viewed at least once, subsequent switches SHALL restore the exact pan and zoom the user last had.

### 4.4 Auto-Layout

When a graph is loaded without position metadata, `GraphAutoLayout::Apply()` SHALL be called once before the first render to arrange nodes in a left-to-right DAG layout. After the first render, nodes SHALL remain at their positions unless the user triggers "Auto Layout" manually.

### 4.5 Grid Snapping

Grid snapping (managed by `GridSnapping`) is a per-editor preference, not per-tab. The default grid size is 8 px. Grid snapping SHALL apply to all tabs simultaneously.

### 4.6 Mini-Map

The `MinimapWidget` overlay SHALL be rendered in the bottom-right corner of every canvas. It provides:
- An overview of all nodes in the graph.
- Click-and-drag navigation to any area of the canvas.
- It SHALL NOT be shared between tabs; each renderer manages its own widget state.

---

## 5. File Operations CRUD

### 5.1 New Graph — `Ctrl+N`

```
Action: Ctrl+N  or  File > New Graph
        ↓
Create EditorTab {
    displayName = "Untitled-N"
    filePath    = ""
    graphType   = "VisualScript"
    isDirty     = false
}
        ↓
New tab becomes active
        ↓
Empty canvas ready for editing
```

- **SHALL NOT** close or modify any currently open tab.
- The graph type for `[+]` button and `Ctrl+N` defaults to **VisualScript**. Future: a "New Graph" dialog may let the user choose the type.

### 5.2 Open / Load Graph — `Ctrl+O` or double-click

```
Action: Ctrl+O  or  double-click file in Asset Browser
        ↓
Is file already open in a tab?
   Yes → Activate that tab (no duplicate)
   No  → Detect graph type from file contents
        ↓
        Create new EditorTab
        Instantiate correct renderer (VS / BT / Anim)
        renderer->Load(filePath)
        ↓
        New tab becomes active
        ↓
        Auto-fit viewport to graph bounding box
```

**Duplicate prevention**: `TabManager::OpenFileInTab()` SHALL scan all open tabs' `filePath` before creating a new one. If the same path is already open, it SHALL activate the existing tab rather than opening a second copy.

### 5.3 Save — `Ctrl+S`

```
Action: Ctrl+S
        ↓
Is filePath empty?  (Untitled-N)
    Yes → trigger "Save As" dialog (pre-filled with tab display name)
    No  → renderer->Save(filePath)
        ↓
        tab.isDirty = false
        tab.displayName = filename extracted from filePath
        Tab label updates immediately (asterisk removed)
```

The Save button in the menu bar SHALL be **disabled** when no tab is open or the active tab is clean.

### 5.4 Save As — `Ctrl+Shift+S`

```
Action: Ctrl+Shift+S  or  File > Save As
        ↓
        ┌──────────────────────────────────────────────┐
        │ Save Graph As                                │
        ├──────────────────────────────────────────────┤
        │ Filename: [Untitled_1.ats           ]        │
        │            ↑ pre-filled from tab name        │
        ├──────────────────────────────────────────────┤
        │            [Save]       [Cancel]             │
        └──────────────────────────────────────────────┘
        ↓ (on Save)
        renderer->Save(chosenPath)
        tab.filePath    = chosenPath
        tab.displayName = filename portion of chosenPath
        tab.isDirty     = false
```

**Save As dialog pre-fill rule**: The dialog input field SHALL be pre-filled with the **current tab display name** (spaces replaced by underscores, invalid path characters stripped). For a tab named `Untitled-2`, the pre-fill SHALL be `Untitled_2.ats`. For a tab named `guard_logic.ats`, the pre-fill SHALL be `guard_logic.ats`.

```cpp
// CORRECT pre-fill logic
std::string suggestedName = tab->displayName;
// Replace spaces with underscores
std::replace(suggestedName.begin(), suggestedName.end(), ' ', '_');
// Replace hyphens with underscores for path safety
std::replace(suggestedName.begin(), suggestedName.end(), '-', '_');
// Append .ats if no extension already present
if (suggestedName.find('.') == std::string::npos)
    suggestedName += ".ats";
strncpy(m_saveAsBuffer, suggestedName.c_str(), sizeof(m_saveAsBuffer) - 1);
```

### 5.5 Close Tab — `Ctrl+W`

Refer to [Section 3.6](#36-tab-close-workflow) for the decision tree.

### 5.6 Exit Editor — `Alt+F4` / File > Exit

Refer to [Section 3.7](#37-exit--close-all-workflow) for the decision tree.

---

## 6. Dirty State Management

### 6.1 What Makes a Graph Dirty?

A graph becomes dirty (unsaved changes) when **any** of the following operations occur:

| Operation | Sets isDirty |
|-----------|-------------|
| Add node | ✅ Yes |
| Remove node | ✅ Yes |
| Add link/connection | ✅ Yes |
| Remove link/connection | ✅ Yes |
| Move node (position change) | ✅ Yes |
| Edit node property/parameter | ✅ Yes |
| Undo/Redo that changes the graph | ✅ Yes |
| Tab switch (navigation only) | ❌ No |
| Load from file | ❌ No (clears dirty) |
| Successful save | ❌ No (clears dirty) |

### 6.2 Visual Indicators

```
Tab bar:
  [guard_logic.ats]    ← clean: no asterisk
  [Untitled-1 *]       ← dirty: asterisk

Menu bar Save button:
  [💾 Save]            ← enabled when active tab is dirty
  [💾 Save] (grayed)   ← disabled when active tab is clean or no tab open

Window title (optional):
  "Olympe Editor — guard_logic.ats *"  ← asterisk in title when dirty
```

### 6.3 Implementation Pattern

```cpp
// CORRECT — dirty flag source of truth is the renderer
// TabManager syncs it every frame
if (tab.renderer)
    tab.isDirty = tab.renderer->IsDirty();
```

```cpp
// CORRECT — renderer marks itself dirty when its data changes
void VisualScriptRenderer::OnNodeAdded(...)
{
    // ... modify graph data ...
    m_isDirty = true;
}

bool VisualScriptRenderer::IsDirty() const
{
    return m_isDirty;
}
```

### 6.4 Clearing the Dirty Flag

The dirty flag SHALL be cleared in exactly two situations:
1. A successful `Save()` or `SaveAs()` call.
2. A `Load()` call (loading always replaces the in-memory state with disk state).

**It SHALL NOT be cleared** by Undo operations alone. Undo is a graph modification; the only way to clear dirty is a save.

---

## 7. Context Menus & CRUD on Graph Elements

### 7.1 Node Context Menu (right-click on node)

```
Right-click on a node
        ↓
┌───────────────────────────┐
│  Edit Properties     [↵]  │
│  ─────────────────────── │
│  Duplicate         Ctrl+D │
│  Copy              Ctrl+C │
│  ─────────────────────── │
│  Delete              Del  │
└───────────────────────────┘
```

Implementation:
```cpp
int hoveredNode = -1;
if (ImNodes::IsNodeHovered(&hoveredNode) &&
    ImGui::IsMouseClicked(ImGuiMouseButton_Right))
{
    m_contextMenuNodeID = hoveredNode;
    ImGui::OpenPopup("NodeContextMenu");
}

if (ImGui::BeginPopup("NodeContextMenu"))
{
    if (ImGui::MenuItem("Edit Properties"))
        OpenInspectorForNode(m_contextMenuNodeID);

    ImGui::Separator();

    if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
        DuplicateNode(m_contextMenuNodeID);

    if (ImGui::MenuItem("Copy", "Ctrl+C"))
        CopyNodeToClipboard(m_contextMenuNodeID);

    ImGui::Separator();

    if (ImGui::MenuItem("Delete", "Del"))
    {
        PushUndoCommand(new DeleteNodeCommand(m_contextMenuNodeID));
        RemoveNode(m_contextMenuNodeID);
    }

    ImGui::EndPopup();
}
```

### 7.2 Link / Edge Context Menu (right-click on link)

```
Right-click on a link
        ↓
┌─────────────────────────────┐
│  Delete Link           Del  │
│  ─────────────────────────  │
│  Reroute (add waypoint)     │
└─────────────────────────────┘
```

### 7.3 Canvas Context Menu (right-click on empty canvas area)

```
Right-click on empty canvas
        ↓
┌─────────────────────────────┐
│  Add Node…          Space   │
│  ─────────────────────────  │
│  Paste              Ctrl+V  │
│  ─────────────────────────  │
│  Select All         Ctrl+A  │
│  ─────────────────────────  │
│  Auto Layout        Ctrl+L  │
└─────────────────────────────┘
```

**"Add Node…"** SHALL open the node search palette (spawned at the cursor position) filtered to the current graph type. The Space key SHALL open the same palette.

### 7.4 Multi-Selection

- Rectangular drag on the canvas background selects multiple nodes.
- `Ctrl+Click` adds/removes individual nodes from the selection.
- `Ctrl+A` selects all nodes.
- Selected nodes can be moved together (drag any of them).
- **Delete** removes all selected nodes and their associated links.
- `Ctrl+C` / `Ctrl+V` copies and pastes all selected nodes (via the `Clipboard` system).

### 7.5 Delete Key Behavior

```
Del key pressed
        ↓
Any nodes selected ?   Any links selected ?
    Yes → DeleteNodeCommand for each
    Yes → DeleteLinkCommand for each
    No  → No-op (do not show error)
```

The Delete key SHALL **not** ask for confirmation when deleting nodes (Undo is available). It SHALL **only** ask for confirmation when closing a file with unsaved changes.

---

## 8. Keyboard Shortcuts

### 8.1 Global Editor Shortcuts

| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+N` | New graph (new tab) | Default type: VisualScript |
| `Ctrl+O` | Open file | Opens file picker dialog |
| `Ctrl+S` | Save active tab | "Save As" if untitled |
| `Ctrl+Shift+S` | Save As | Always shows dialog |
| `Ctrl+W` | Close active tab | Prompts if dirty |
| `Ctrl+Tab` | Next tab | Cycles forward |
| `Ctrl+Shift+Tab` | Previous tab | Cycles backward |
| `Alt+F4` | Exit editor | Prompts if any tab dirty |
| `F5` | Run / Execute graph | Where applicable |

### 8.2 Canvas / Graph Shortcuts

| Shortcut | Action | Context |
|----------|--------|---------|
| `Ctrl+Z` | Undo | Applies to active tab only |
| `Ctrl+Y` | Redo | Applies to active tab only |
| `Ctrl+Shift+Z` | Redo (alt) | Same as Ctrl+Y |
| `Ctrl+C` | Copy selected nodes | Active tab |
| `Ctrl+X` | Cut selected nodes | Active tab |
| `Ctrl+V` | Paste | Active tab, at mouse cursor |
| `Ctrl+D` | Duplicate selected nodes | Active tab |
| `Ctrl+A` | Select all nodes | Active tab |
| `Del` | Delete selected nodes/links | Active tab |
| `Esc` | Deselect all | Active tab |
| `Space` | Open node search palette | At mouse cursor |
| `Ctrl+L` | Auto-layout graph | Active tab |
| `Ctrl+F` | Find node by name | Active tab |
| `F` | Fit view to graph | Active tab — focus all nodes |
| `Home` | Reset viewport to origin | Active tab |
| `+` / `Scroll Up` | Zoom in | Active tab |
| `-` / `Scroll Down` | Zoom out | Active tab |

### 8.3 Node Search Palette Shortcuts

| Shortcut | Action |
|----------|--------|
| `Up` / `Down` | Navigate search results |
| `Enter` | Add highlighted node at cursor |
| `Esc` | Close palette without adding |

### 8.4 Inspector Panel Shortcuts

| Shortcut | Action |
|----------|--------|
| `Enter` | Confirm text field edit |
| `Esc` | Cancel text field edit and revert value |
| `Tab` | Move to next property field |

---

## 9. Undo / Redo System

### 9.1 Architecture

Each open tab maintains its own independent `UndoRedoStack`. When a tab is closed, its undo stack is destroyed. There is no global cross-tab undo.

```
Tab 1: guard_logic.ats    Tab 2: Untitled-1       Tab 3: patrol.bt
   UndoRedoStack #1          UndoRedoStack #2        UndoRedoStack #3
   [MoveNode #4]             [AddNode #1]            (empty)
   [AddNode #3]              [AddNode #2]
   [DeleteNode #2]           ← top (last action)
   ← top (last action)
```

`Ctrl+Z` and `Ctrl+Y` SHALL operate **only** on the active tab's stack.

### 9.2 Supported Operations

| Operation | Command Class | Undo Effect |
|-----------|--------------|-------------|
| Add node | `AddNodeCommand` | Remove the added node |
| Delete node | `DeleteNodeCommand` | Restore node + its connections |
| Move node | `MoveNodeCommand` | Restore previous position |
| Add exec link | `AddConnectionCommand` | Remove the added link |
| Remove exec link | (planned) `RemoveConnectionCommand` | Restore the link |
| Edit property | (planned) `EditPropertyCommand` | Restore previous value |

### 9.3 Using the UndoRedoStack

```cpp
// CORRECT — push before modifying the graph
void VisualScriptRenderer::OnAddNode(const TaskNodeDefinition& def)
{
    m_undoStack.PushCommand(
        std::unique_ptr<ICommand>(new AddNodeCommand(def)),
        m_graph
    );
    m_isDirty = true;
}

// CORRECT — undo on Ctrl+Z
if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)) &&
    ImGui::GetIO().KeyCtrl)
{
    if (m_undoStack.CanUndo())
    {
        m_undoStack.Undo(m_graph);
        m_isDirty = true;
    }
}
```

### 9.4 Undo Stack Limits

- The stack is bounded to `UndoRedoStack::MAX_STACK_SIZE` (100 entries).
- When the limit is reached, the **oldest** undo entry is silently dropped (FIFO eviction).
- Pushing any new command **clears** the redo stack.

### 9.5 Undo After Save

Saving does **not** clear the undo stack. The user SHALL be able to undo changes that were made before the last save. However, performing an undo after a save will re-dirty the file (it is now different from the saved state).

```
User saves → isDirty = false, stack unchanged
User Ctrl+Z → graph reverts → isDirty = true again
```

### 9.6 History Panel

The `HistoryPanel` widget displays the undo/redo stack for the active tab. It SHALL:
- Show command descriptions (e.g. "Add Node #3", "Delete Node #7").
- Highlight the current position (between undo and redo items).
- Allow clicking a past entry to undo/redo multiple steps at once.

---

## 10. Asset / File Browser Integration

### 10.1 Panel Behavior

| Interaction | Effect |
|-------------|--------|
| Single click on file | Highlights the file; may preview metadata in Asset Info panel |
| **Double-click** on file | Opens the file in a **new tab** (or activates existing) |
| Right-click on file | Shows file context menu |
| Right-click on folder | Shows folder context menu |
| Drag file to canvas | (Future) Creates a SubGraph reference node |

### 10.2 File Context Menu

```
Right-click on a file in the browser
        ↓
┌────────────────────────┐
│  Open in Tab           │
│  ──────────────────── │
│  Rename            F2  │
│  Duplicate             │
│  Delete          Del   │
│  ──────────────────── │
│  Copy Path             │
│  Show in Explorer      │
└────────────────────────┘
```

### 10.3 Duplicate Prevention (Already Open Files)

```
User double-clicks "guard_logic.ats"
        ↓
TabManager::OpenFileInTab("Blueprints/AI/guard_logic.ats")
        ↓
Scan existing tabs for matching filePath
        ↓
Already open in tab_2?
    Yes → SetActiveTab("tab_2")
    No  → Create new tab, load file
```

The file browser SHOULD visually indicate files that are already open (e.g. a dot or different icon colour) to prevent confusion.

### 10.4 File Type Detection

`TabManager::DetectGraphType()` inspects the JSON content to determine the renderer type:

| File content indicator | Resolved type |
|------------------------|--------------|
| `"graphType": "VisualScript"` | `VisualScript` |
| `"graphType": "BehaviorTree"` | `BehaviorTree` |
| `"graphType": "AnimGraph"` | `AnimGraph` |
| `schema_version == 4` + `execConnections` present | `VisualScript` |
| `rootNodeId` + `nodes` present | `BehaviorTree` |
| `states` + `transitions` present | `AnimGraph` |
| Unrecognized | Fallback: attempt `VisualScript` load |

---

## 11. Unified Graph Type Support

### 11.1 All Graph Types Use the Same Tab System

Every graph type — regardless of its internal complexity — SHALL be opened in a tab managed by `TabManager`. There SHALL be no separate modal window, floating window, or standalone panel for any graph type.

```
VisualScriptRenderer    ↗
BehaviorTreeRenderer    → implements IGraphRenderer → managed by TabManager
AnimGraphRenderer       ↘
(future) LevelRenderer
(future) StateMachineRenderer
```

### 11.2 IGraphRenderer Contract

Every renderer MUST implement:

```cpp
virtual void        Render()                       = 0;  // Draw canvas
virtual bool        Load(const std::string& path)  = 0;  // Load from disk
virtual bool        Save(const std::string& path)  = 0;  // Save to disk
virtual bool        IsDirty()              const   = 0;  // Unsaved changes?
virtual std::string GetGraphType()         const   = 0;  // "VisualScript" etc.
virtual std::string GetCurrentPath()       const   = 0;  // Last loaded/saved path
```

Every renderer SHOULD additionally expose:
- `ImNodesEditorContext* GetImNodesContext()` for canvas state management.
- `UndoRedoStack& GetUndoStack()` for per-tab undo/redo.

### 11.3 Graph Type Table

| Type | File Extension | Renderer Class | Status |
|------|---------------|---------------|--------|
| Visual Script | `.ats` | `VisualScriptRenderer` | ✅ Active |
| Behavior Tree | `.ats` / `.json` | `BehaviorTreeRenderer` | ✅ Active |
| Anim Graph | `.ag` / `.json` | `AnimGraphRenderer` | 🔄 Planned |
| Level Graph | `.lvl.json` | `LevelGraphRenderer` | 🔄 Planned |
| State Machine | `.sm.json` | `StateMachineRenderer` | 🔄 Planned |

### 11.4 Tab Bar Appearance by Graph Type

Each graph type SHOULD display a small coloured dot or icon prefix in the tab to help the user visually differentiate graph types:

```
[🟦 guard_logic.ats]   ← blue = VisualScript
[🟨 patrol.bt]         ← yellow = BehaviorTree
[🟩 player_anim.ag]    ← green = AnimGraph
[⬜ Untitled-1 *]      ← white/grey = new/unknown
```

This is a **recommended enhancement**, not a current requirement.

---

## 12. Error Handling & Validation

### 12.1 File Load Errors

```
Load("invalid_file.ats") fails
        ↓
renderer->Load() returns false
        ↓
TabManager logs: "[TabManager] Failed to load VS file: path"
        ↓
No tab is created (tab creation is aborted)
        ↓
(Optional) Show error notification: "Failed to open 'filename': invalid format"
```

### 12.2 Corrupted Graph Recovery

When a file can be parsed as JSON but contains invalid node references:
- `BlueprintValidator` SHALL run automatically on load.
- Invalid links (referencing non-existent node IDs) SHALL be **silently removed** and a warning logged.
- Missing node types SHALL be replaced with a placeholder "Unknown Node" node that displays the missing type name in red.
- The graph SHALL still open; the user is informed via the `GraphValidationPanel`.

### 12.3 Missing Node Type

```
Load encounters node type "AIChaseTarget" which is not registered
        ↓
Create placeholder node with:
    displayName = "⚠ Missing: AIChaseTarget"
    colour      = red
    pins        = none (or original pin count if metadata available)
        ↓
Log: "[Validator] Unknown node type 'AIChaseTarget' in node #12"
        ↓
isDirty = true (graph is now different from on-disk state)
```

### 12.4 Circular Dependency Detection (Behavior Trees)

Behavior Trees SHALL NOT contain cycles. `BlueprintValidator::CheckCircularDependencies()` SHALL be run:
- On every new link creation attempt.
- On file load.

If a cycle is detected:
- The link that would create the cycle SHALL be **rejected** and not added.
- A notification SHALL appear: "Cannot connect: would create a cycle."
- No undo entry is pushed (the operation was rejected before execution).

### 12.5 Save Errors

If `Save()` fails (disk full, permission denied, path invalid):
- The tab SHALL remain dirty (`isDirty = true`).
- The tab name SHALL remain unchanged.
- An error dialog SHALL inform the user: "Save failed: [reason]. The graph was NOT saved."

---

## 13. Known Bugs & How These Guidelines Address Them

This section documents the specific UX bugs encountered during development and explains which guideline section defines the correct behavior to prevent them.

### Bug 1 — Tab navigation stuck on first opened tab

**Symptom**: Clicking on a second or third tab did not switch the active canvas. The user remained on the first tab regardless.

**Root Cause**: `SetActiveTab()` was setting `m_activeTabID` but `RenderActiveCanvas()` was independently re-evaluating which renderer to use without consulting `m_activeTabID`. Additionally, the `ImGuiTabItemFlags_SetSelected` flag was not applied on programmatic tab switches.

**Guideline**: [Section 3.5 — Switching Between Tabs](#35-switching-between-tabs).

**Required fix**:
```cpp
// In TabManager::RenderTabBar()
if (ImGui::BeginTabItem(label.c_str(), &open, flags))
{
    // MUST check and update active tab on every BeginTabItem
    if (m_activeTabID != tab.tabID)
        SetActiveTab(tab.tabID);
    ImGui::EndTabItem();
}
```

---

### Bug 2 — Node positions reset to top-left when switching tabs

**Symptom**: Switching to any tab reset all node positions to (0, 0), causing all nodes to pile up in the top-left corner, making the graph unreadable.

**Root Cause**: All renderers were calling `ImNodes::BeginNodeEditor()` with the **same global ImNodes context** (no per-tab context was created). Switching tabs did not restore the previous context.

**Guideline**: [Section 4.1 — Per-Tab ImNodes Context](#41-per-tab-imnodes-context).

**Required fix**: Each `VisualScriptRenderer` / `BehaviorTreeRenderer` instance SHALL create and own its own `ImNodesEditorContext*` and call `ImNodes::EditorContextSet(m_imnodesContext)` at the top of `Render()`.

---

### Bug 3 — Save does not update tab name or remove asterisk

**Symptom**: Saving a file via `Ctrl+S` succeeded (file was written to disk), but the tab continued to display `Untitled-1 *` instead of the actual filename, and the asterisk remained.

**Root Cause**: `SaveActiveTabAs()` updated `tab.filePath` and `tab.isDirty` correctly, but `tab.displayName` was not updated.

**Guideline**: [Section 5.3 — Save](#53-save--ctrls) and [Section 6.2 — Visual Indicators](#62-visual-indicators).

**Required fix**:
```cpp
bool TabManager::SaveActiveTabAs(const std::string& path)
{
    // ... save ...
    if (ok)
    {
        tab->filePath    = path;
        tab->isDirty     = false;
        tab->displayName = DisplayNameFromPath(path);  // ← MUST update
    }
    return ok;
}
```

---

### Bug 4 — "Save As" dialog pre-fills with wrong/generic name

**Symptom**: When saving a tab named `Untitled-2`, the "Save As" filename input was pre-filled with `untitled_graph` instead of `Untitled_2`, creating confusion and potential filename errors.

**Guideline**: [Section 5.4 — Save As](#54-save-as--ctrlshifts).

**Required fix**: Pre-fill the save dialog input with the current tab's `displayName`, sanitized for use as a filename (spaces and hyphens replaced by underscores).

---

### Bug 5 — Context menus missing for nodes and links

**Symptom**: Right-clicking on a node or link did nothing. There was no way to delete, duplicate, or edit a node without keyboard shortcuts.

**Root Cause**: The `Render()` methods of the renderers did not include right-click detection logic or `ImGui::BeginPopup()` calls.

**Guideline**: [Section 7 — Context Menus & CRUD on Graph Elements](#7-context-menus--crud-on-graph-elements).

**Required fix**: Add `ImNodes::IsNodeHovered()` and `ImGui::IsMouseClicked(ImGuiMouseButton_Right)` checks inside every renderer's `Render()` loop, followed by `ImGui::OpenPopup()` and a `BeginPopup/EndPopup` block.

---

### Bug 6 — Undo/Redo not connected to editor operations

**Symptom**: Pressing `Ctrl+Z` after adding or moving a node had no effect. The `UndoRedoStack` existed but was never populated.

**Root Cause**: Node addition and deletion code paths did not call `UndoRedoStack::PushCommand()`.

**Guideline**: [Section 9 — Undo / Redo System](#9-undo--redo-system).

**Required fix**: Every graph-modifying action (add node, delete node, move node, add link, remove link) SHALL push an `ICommand` subclass to the active tab's `UndoRedoStack` **before** applying the modification. `Ctrl+Z` / `Ctrl+Y` SHALL be handled inside the renderer's `Render()` method via `ImGui::IsKeyPressed`.

---

## 14. Do's and Don'ts Quick Reference

### ✅ Do's

- **DO** call `ImNodes::EditorContextSet()` at the start of every `Render()` call.
- **DO** keep one `ImNodesEditorContext*` per renderer instance (i.e. per tab).
- **DO** update `tab.displayName` immediately after a successful save.
- **DO** pre-fill the Save As dialog with the current tab's display name.
- **DO** push an `ICommand` to the undo stack before every graph-modifying operation.
- **DO** show the asterisk `*` in the tab name as soon as `isDirty` becomes true.
- **DO** check for the file already being open before creating a duplicate tab.
- **DO** open files in a new tab (double-click in Asset Browser).
- **DO** persist panel widths and positions between sessions.
- **DO** run `BlueprintValidator` on every file load.

### ❌ Don'ts

- **DON'T** use a single global `ImNodesEditorContext` shared across all tabs.
- **DON'T** overwrite the active tab when opening a file — always create a new tab.
- **DON'T** silently discard unsaved changes when closing a tab or exiting.
- **DON'T** allow nodes to default to position (0, 0) when loading — use auto-layout.
- **DON'T** clear the undo stack on save.
- **DON'T** open a second copy of the same file if it is already in a tab.
- **DON'T** apply `Ctrl+Z` to a tab other than the active one.
- **DON'T** place the Inspector panel on the right side (use bottom-left).
- **DON'T** allow context menus to open when the canvas is in a "link drag" state.
- **DON'T** forget to sanitize filenames in the Save As dialog (replace hyphens and spaces).

---

## 15. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-03-11 | Olympe Engine Team | Initial document. Covers MDI/tabs, canvas, CRUD, context menus, shortcuts, undo/redo, asset browser, graph types, error handling, and the 6 known bugs from the multi-tab refactoring sprint. |

---

*This document is part of the Olympe Engine living documentation. When you implement or change a UX behavior, update the relevant section and bump the version history.*
