# Copilot Instructions

## General Guidelines
- First general instruction
- Second general instruction

## Project Directives
- Phase 24 Implementation: Condition Presets migrated from external file storage to graph-embedded serialization. Presets are now stored IN each blueprint's JSON (v4 schema), making graphs self-contained. New field added to TaskGraphTemplate: `std::vector<ConditionPreset> Presets`. Serialization handled by VisualScriptEditorPanel::SerializeAndWrite() and deserialization by TaskGraphLoader::ParseSchemaV4().

- Phase 26 UX Enhancement: Tab-based panel system for right editor section. The 3 panels (Preset Bank, Local Variables, Global Variables) are now grouped into a tabbed interface:
  - Part A (top): Node Properties panel (unchanged)
  - Part B (bottom): Tab-based layout with 3 tabs:
    * Tab 0: Preset Bank (Condition Presets)
    * Tab 1: Local Variables (Local Blackboard)
    * Tab 2: Global Variables (Global Blackboard)
  - Implementation: `m_rightPanelTabSelection` (0=Presets, 1=Local, 2=Global)
  - New functions: `RenderRightPanelTabs()` and `RenderRightPanelTabContent()`
  - Modified: `RenderContent()` in VisualScriptEditorPanel_RenderingCore.cpp
  - Removed: Old splitter system between Preset Bank and Variables; old radio-button selection for Local/Global

- Phase 27 (COMPLETED): Entity Prefab Editor - Basic rendering pipeline implemented:
  - **Architecture**: IGraphRenderer adapter pattern (EntityPrefabRenderer) → PrefabCanvas (ImGui container) → ComponentNodeRenderer (drawing)
  - **Data Model**: EntityPrefabGraphDocument loads/stores nodes and connections from JSON (v4 schema)
  - **Rendering**: Nodes render as boxes with titles, connections draw as Bezier curves
  - **Key Classes**:
    * `EntityPrefabGraphDocument`: Data model (loads guard.json format)
    * `EntityPrefabRenderer`: IGraphRenderer adapter for TabManager integration
    * `PrefabCanvas`: ImGui-based canvas container
    * `ComponentNodeRenderer`: Node/connection drawing logic
    * `ComponentNode`: Individual component data (position, size, properties, nodeId)
  - **Critical Fix**: Coordinate transformation - nodes must be transformed from canvas space to screen space using ImGui's `GetCursorScreenPos()` + zoom/offset
  - **Current Status**: ✅ Can load & display entity prefabs (tested with guard.json: 6 nodes, 5 connections)
  - **C++ Standard**: C++14 compliant (no C++17 features like structured bindings)

- Phase 28 (COMPLETED): Entity Prefab Editor - Interactive Features Phase 2:
  - **Features Implemented**:
    * ✅ Panning: Middle-mouse drag to pan canvas, offset stored in m_canvasOffset
    * ✅ Zoom: Mouse scroll wheel to zoom (clamped 0.1x - 3.0x), zoom-centered on mouse position
    * ✅ Node Dragging: Left-click and drag nodes to reposition, with snap-to-grid option
    * ✅ Multi-node Selection: Ctrl+Click to multi-select, Shift compatibility planned
    * ✅ Node Deletion: Delete key removes selected nodes
    * ✅ Node Selection: Left-click to select, visual feedback with blue glow
    * ✅ Hit Detection: IsPointInNode() for accurate click testing
    * ✅ SaveToFile(): Full JSON serialization of nodes, connections, canvas state with dirty flag tracking
    * ✅ Context Menu: Right-click menu for delete/select/clear operations (Phase 2 Refinement)
    * ✅ Proper Connection Rendering: Bezier curves with proportional offset (40% of horizontal distance)
    * ✅ Grid Scaling: Grid properly scales and pans with canvas (fixed zoom/pan issues)
  - **Dirty Flag System**:
    * `EntityPrefabGraphDocument::m_isDirty` tracks modifications (CreateNode, RemoveNode, ConnectNodes, DisconnectNodes)
    * Cleared on LoadFromFile() and SaveToFile()
    * EntityPrefabRenderer::IsDirty() checks document dirty flag for UI feedback
  - **ImGui Input Integration**:
    * Added ImGui input polling in PrefabCanvas::Render()
    * Polls mouse clicks/movement/scroll, keyboard modifiers (Ctrl, Shift)
    * Dispatches to OnMouseMove, OnMouseDown, OnMouseUp, OnMouseScroll, OnKeyDown handlers
    * Window hover checking ensures input only when canvas is focused
  - **Grid & Connection Rendering (Phase 2 Refinement)**:
    * Grid scales with zoom: `scaledGridSpacing = m_gridSpacing * m_canvasZoom`
    * Grid offset calculated in screen space using fmod for proper pan synchronization
    * Connection bezier curves use proportional offset: 40% of horizontal distance minimum 50px
    * Both grid and connections now move together seamlessly during panning
  - **Context Menu (Phase 2 Refinement)**:
    * Right-click on node: Delete Node, Select Node
    * Right-click on empty canvas: Clear Selection, Select All, Delete All, Reset View
    * Menu detects click context and shows appropriate options
  - **Key Methods in PrefabCanvas**:
    * `PanCanvas()`: Updates m_canvasOffset directly
    * `ZoomCanvas()`: Adjusts zoom with center-point calculation to prevent view jumping
    * `GetNodeAtPosition()`: Hit detection via ScreenToCanvas + IsPointInNode
    * `HandleNodeDrag*()`: Multi-node drag with offset calculation
    * `SnapNodePositionToGrid()`: Optional grid snapping (enabled by default)
    * `RenderContextMenu()`: ImGui context menu for node/canvas operations
  - **JSON Schema**: Mirrors LoadFromFile structure with nodes[], connections[], canvasState
  - **Current Status**: ✅ All interactive features working, grid and connections refined, build successful (0 errors)
  - **Integration**: Works seamlessly with PrefabCanvas::Render() ImGui event loop

- Phase 29 (FUTURE - Entity Prefab Editor Phase 3):
  - Add component palette panel (right-side searchable list of available components)
  - Drag-drop or double-click to add new component nodes to graph
  - Property editing panel (select node → edit properties in sidebar)
  - Minimap/viewport controls for large graphs
  - Connection creation UI (drag from node port to create connections)
  - Undo/Redo system for edit history
  - Export to runtime format

- Phase 30 (FUTURE - Entity Prefab Editor Optimization):
  - Performance profiling and optimization
  - Large graph support (1000+ nodes)
  - Hierarchical/grouped nodes
  - Custom component type registration
  - Graph template system for reusable prefab patterns

## File Management Protocol
- To add new files to the OlympeBlueprintEditor project:
  1. Create new C++ files using the `create_file` tool with complete implementation.
  2. Specify which project the files belong to (typically OlympeBlueprintEditor.vcxproj or Olympe Engine.vcxproj).
  3. Manually add files to the project via Visual Studio: Right-click project → "Add Existing Item" → select file(s) → OK → Compile to verify.
- This approach avoids XML manipulation errors and is faster/more reliable than programmatic project file editing.
- For code modifications to existing files, use `replace_string_in_file` directly without user intervention.