# Placeholder Editor - Target UI Layout Mockup

**Phase 4 Complete Design** - Validation Reference Document

---

## Visual Layout (ASCII Mockup)

```
┌────────────────────────────────────────────────────────────────────────────┐
│  Olympe Blueprint Editor                                                   │
├────────────────────────────────────────────────────────────────────────────┤
│ File  Edit  Tools  View  Help                                              │
├────────────────────────────────────────────────────────────────────────────┤
│ [Save] [Save As] [Browse] │ [Verify] [Run Graph] │ [✓ Minimap] [0.20] ▼   │
├────────────────────────────┬───────────────────────────────────────────────┤
│                            │  Right Panel                                   │
│                            │ ┌──────────────────────────────────────────┐  │
│                            │ │ Components      │ Node                  │  │
│                            │ ├──────────────────────────────────────────┤  │
│                            │ │                                          │  │
│                            │ │  Active Tab Content:                     │  │
│                            │ │  • If Components: Component palette      │  │
│                            │ │  • If Node: Selected node properties     │  │
│                            │ │                                          │  │
│  Canvas Area               │ │  (Empty placeholder or editor content)   │  │
│  ═══════════════════════   │ │                                          │  │
│  ≡ Grid Background         │ │                                          │  │
│                            │ │                                          │  │
│  ┌─────────┐   ┌──────────┐│ │                                          │  │
│  │ Blue    │──│ Magenta  ││ │                                          │  │
│  │ Node    │   │ Node     ││ │ [Apply] [Reset] (shown when editing)    │  │
│  └─────────┘   └──────────┘│ │                                          │  │
│        │           ▲        │ │                                          │  │
│        └───────────┘        │ │                                          │  │
│                            │ │                                          │  │
│  Right-click: Context Menu  │ │                                          │  │
│  ├─ Delete Node            │ │                                          │  │
│  ├─ Duplicate              │ │                                          │  │
│  ├─ Properties             │ │                                          │  │
│  └─ ─────────              │ │                                          │  │
│      Select All            │ │                                          │  │
│                            │ │                                          │  │
│ [Pan: Middle-Mouse]        │ │                                          │  │
│ [Zoom: Mouse Wheel]        │ │                                          │  │
│ [Select: Click/Drag Rect]  │ │                                          │  │
│                            │ │                                          │  │
│                            │ └──────────────────────────────────────────┘  │
│                            │◄─────────  Resizable Handle                   │
└────────────────────────────┴───────────────────────────────────────────────┘
│ Minimap (Top-Right corner, configurable)                                   │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## Component Breakdown

### 1. Menu Bar (Top)
- File, Edit, Tools, View, Help (standard application menus)
- Location: Top of window

### 2. Framework Toolbar (Below menu)
**Rendered by CanvasToolbarRenderer (framework)**
- `[Save]` - Save current graph to file
- `[Save As]` - Save as new file (opens dialog)
- `[Browse]` - Open file browser  
- `|` - Visual separator
- Buttons for type-specific actions (next section)

### 3. Type-Specific Toolbar (Placeholder-only)
**Rendered by RenderTypeSpecificToolbar()**
- `[Verify]` - Verify graph validity (logs: "[PlaceholderGraphRenderer] Verify button clicked")
- `[Run Graph]` - Execute graph (logs: "[PlaceholderGraphRenderer] Run Graph button clicked")
- `|` - Visual separator
- `[✓ Minimap]` - Checkbox to toggle minimap visibility
- `[Size: 0.20]` - Drag slider to adjust minimap size (0.05 - 0.5)
- Position selector (future: Top-Left, Top-Right, etc.)

### 4. Canvas Area (Left Side - 70% width default)
**Rendered by PlaceholderCanvas::Render()**

**Content**:
- Dark grid background (#26262FFF - imnodes style)
- Blue/Green/Magenta colored nodes
- Yellow connection lines (Bezier curves)
- Minimap overlay (if enabled) in corner

**Interactions**:
- Left-click: Select node (blue highlight glow)
- Left-click + drag: Move selected node(s)
- Left-click + drag (empty): Rectangle selection
- Ctrl+Click: Multi-select nodes
- Right-click on node:
  ```
  Context Menu:
  ├─ Delete
  ├─ Properties
  └─ ──────────
      Select All
      Deselect All
  ```
- Right-click on empty:
  ```
  Context Menu:
  ├─ New Node (Blue)
  ├─ New Node (Green)
  ├─ New Node (Magenta)
  └─ ──────────
      Select All
      Reset View
  ```
- Middle-mouse drag: Pan canvas
- Mouse wheel: Zoom (0.1x - 3.0x centered on cursor)
- Delete key: Delete selected nodes
- Ctrl+A: Select all nodes

### 5. Right Panel (30% width default - resizable)
**Rendered by RenderGraphContent() - Part B**

#### Tab Bar (Top)
Two tabs:
- `Components` - Component palette for instantiation
- `Node` - Properties of selected node(s)

#### Tab Content Area

**Tab 0: Components**
- Placeholder: "(Component palette - TBD)"
- Future: Searchable list of available component types
- Future: Drag-drop to canvas for instantiation

**Tab 1: Node**
- Show when node(s) selected
- Display: Node ID, Title, Position (X, Y), Size (W, H), Enabled flag
- Edit fields with InputText
- Show Apply/Reset buttons when editing
- Multi-select support: Show "(Multiple values)" for differing fields

#### Apply/Reset Buttons
- Only render when `HasChanges() == true`
- `[Apply]` - Save changes to selected nodes, mark document dirty
- `[Reset]` - Discard changes, revert to original values

### 6. Resize Handles

**Horizontal Handle (between canvas and right panel)**
- 6px wide gray bar
- Cursor changes to ↔ on hover
- Drag to resize: Canvas gets larger → Right panel shrinks
- Range: Right panel 200px (min) to 60% of width (max)

**Vertical Handle (future - between Node Properties and tabs)**
- Currently not used (single tabs fill entire right panel)
- Reserved for future split between fixed header and tabbed content

### 7. Minimap (Overlay)
**Rendered by CanvasMinimapRenderer**
- Position: Top-right corner of canvas (default, configurable)
- Size: 20% of canvas (configurable via toolbar slider)
- Shows:
  - Canvas grid in miniature
  - Viewport rectangle (white outline)
  - Nodes as small colored squares
- Click+drag: Pan canvas to viewport location

---

## Data Flow & State Management

### Document State
```cpp
PlaceholderGraphDocument
├─ std::vector<PlaceholderNode> nodes
├─ std::vector<Connection> connections  
├─ bool isDirty (set by Save/Update operations)
└─ std::string filePath (current open file)
```

### UI State
```cpp
PlaceholderGraphRenderer
├─ m_selectedNodeIds: std::vector<int> (multi-select)
├─ m_propertiesPanelWidth: float (280px default)
├─ m_rightPanelTabSelection: int (0=Components, 1=Node)
├─ m_canvasOffset: ImVec2 (pan state)
├─ m_canvasZoom: float (zoom 0.1x - 3.0x)
├─ m_minimapVisible: bool
├─ m_minimapSize: float (0.05 - 0.5)
└─ m_isDrawingSelectionRect: bool
```

### Property Edit State
```cpp
PlaceholderPropertyEditorPanel
├─ m_selectedNodeIds: std::vector<int> (which nodes to edit)
├─ PropertyEditState[7] (one per property)
│  ├─ editBuffer[256] (text being edited)
│  ├─ isEditing: bool (has user modified?)
│  ├─ isDifferent: bool (differs across multi-select?)
│  └─ originalValue: std::string (for Reset)
├─ HasChanges(): bool
├─ ApplyChanges(): void
└─ ResetChanges(): void
```

---

## Phase 4 Implementation Checklist

### ✅ Completed

- [x] **Tab System**: Components | Node tabs rendering
- [x] **Two-Column Layout**: Canvas left (70%) + Right panel (30%)
- [x] **Resize Handles**: Horizontal handle between columns
- [x] **Property Editor**: Edit fields for 7 properties (title, filepath, posX, posY, width, height, enabled)
- [x] **Apply/Reset Buttons**: Conditional rendering on HasChanges()
- [x] **Batch Editing**: UpdateSelectedNodesProperty() applies to all selected nodes
- [x] **Minimap Controls**: Checkbox + Size slider in toolbar
- [x] **Build**: 0 errors, 0 warnings

### ⏳ In Progress / Pending

- [ ] **Toolbar Integration**: Framework toolbar (Save, Save As, Browse) must render
- [ ] **Type-Specific Buttons**: Verify, Run Graph buttons with logging
- [ ] **Context Menus**: Right-click menus on nodes and empty canvas
- [ ] **Minimap Rendering**: Actual minimap display in corner
- [ ] **Multi-Node Selection**: Visual feedback, rectangle selection
- [ ] **Node Dragging**: Pan individual nodes smoothly
- [ ] **Connection Rendering**: Draw Bezier curves between connected nodes
- [ ] **Delete Node**: Keyboard delete + context menu
- [ ] **Node Instantiation**: Buttons to create new nodes

---

## Tab System Design Detail

### Tab Bar Implementation
```
┌─────────────────────────────────────────┐
│ Components      │ Node                  │  ← ImGui::BeginTabBar
│                                         │     ImGui::BeginTabItem("Components")
│ (Placeholder text)                      │     ImGui::BeginTabItem("Node")
│ Components palette - TBD                │
│                                         │
│ Future: Searchable list of types        │
│         Drag-drop instantiation         │
│                                         │
│                                         │
│                                         │
│                                         │
│                                         │
│                                         │
│ [Apply] [Reset]                         │  ← Only when HasChanges()
└─────────────────────────────────────────┘
```

### Tab 0: Components
- Currently: Placeholder text "(Component palette - TBD)"
- Future: Populated from ComponentPalettePanel in Phase 5

### Tab 1: Node
- Shows 7 editable properties when node(s) selected
- Each property: Label + InputText field
- Multi-select handling: Show "(Multiple values)" for conflicting properties
- Apply/Reset buttons at bottom

---

## Error Cases & Edge Cases

### No Node Selected
```
Node Tab shows:
"(Select a node to edit)"
- No properties displayed
- Apply/Reset buttons hidden
```

### Multiple Nodes Selected
```
Node Tab shows:
- For matching properties: Value displayed
- For differing properties: "(Multiple values)" placeholder
- Edit any field: Applied to ALL selected nodes
- Apply: Updates all, marks dirty
```

### Empty Graph
```
Canvas shows:
- Grid background
- No nodes
- Canvas interaction still works (pan/zoom/select)
```

### Unsaved Changes
```
- Document dirty flag set after changes
- Window title would show "*" indicator (future)
- Save buttons become active
```

---

## Rendering Call Stack (Target Architecture)

```
BlueprintEditorGUI::Render()
├─ RenderTabBar()  
├─ RenderActiveCanvas()
│  └─ PlaceholderGraphRenderer::Render()
│     ├─ RenderContent() - MAIN LAYOUT
│     │  ├─ RenderToolbar() - Framework toolbar
│     │  ├─ RenderTypeSpecificToolbar() - Verify, Run, Minimap
│     │  └─ RenderGraphContent() - TWO-COLUMN LAYOUT
│     │     ├─ Left: PlaceholderCanvas::Render()
│     │     │  ├─ Grid rendering
│     │     │  ├─ Node rendering
│     │     │  ├─ Connection rendering
│     │     │  ├─ Context menu handling
│     │     │  └─ Minimap overlay
│     │     └─ Right: RenderRightPanelTabs()
│     │        ├─ Tab bar (Components | Node)
│     │        └─ Tab content
│     │           ├─ Tab 0: Component palette placeholder
│     │           └─ Tab 1: Property editor + Apply/Reset
│     └─ RenderFrameworkModals()
│        └─ Save As dialog, etc.
└─ Modals (framework)
```

---

## Design Validation Checklist

**Before implementation, confirm:**

- [x] Two tabs: "Components" and "Node" (NOT "Properties")
- [x] Right panel: 30% width, resizable (200px-60% range)
- [x] Tab bar at TOP of right panel
- [x] Content area below tabs (fills remaining space)
- [x] Apply/Reset buttons at BOTTOM of content
- [x] NO duplicate tab rendering
- [x] Toolbar buttons: Save, Save As, Verify, Run Graph, Minimap controls
- [x] Canvas takes remaining left space (70%)
- [x] Horizontal resize handle between canvas and right panel
- [x] NO vertical split within right panel (tabs fill entire height)
- [x] Minimap overlay (not in right panel, over canvas)
- [x] Context menus on right-click (nodes and empty)
- [x] Grid, nodes, connections render correctly on canvas
- [x] Multi-select and batch editing supported

---

## Next Steps (Phase 4 Continuation)

1. **Fix Tab Duplication**: RenderRightPanelTabs() called only ONCE
2. **Fix Tab Names**: "Node" instead of "Properties"
3. **Integrate Toolbar**: Ensure framework toolbar renders first
4. **Add Type-Specific Buttons**: Verify, Run Graph with logging
5. **Implement Context Menus**: Right-click handling on nodes/canvas
6. **Minimap Rendering**: Display actual minimap if enabled
7. **Build & Test**: Verify layout matches this mockup

**Target Completion**: End of Phase 4 Step 6

---

**Document Status**: ✅ DESIGN REFERENCE - Use for validation
**Last Updated**: Phase 4 Step 5
**Mockup Version**: 1.0
