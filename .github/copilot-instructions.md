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
    * ✅ Node Dragging: Left-click and drag nodes to reposition (snap-to-grid removed for better UX)
    * ✅ Multi-node Selection: Ctrl+Click to multi-select
    * ✅ Node Deletion: Delete key removes selected nodes
    * ✅ Node Selection: Left-click to select, visual feedback with blue glow
    * ✅ Hit Detection: IsPointInNode() for accurate click testing
    * ✅ SaveToFile(): Full JSON serialization of nodes, connections, canvas state with dirty flag tracking
    * ✅ Context Menu: Right-click on node for Delete/Select; right-click empty for Clear/Select All/Reset
    * ✅ Proper Connection Rendering: Bezier curves with proportional offset (40% of horizontal distance)
    * ✅ Grid Scaling: Grid properly scales and pans with canvas
    * ✅ Component Palette Panel: Searchable list of available components to add to graph
  - **Bug Fixes (Phase 2 Refinement v2)**:
    * ✅ Node Selection Offset: Fixed ScreenToCanvas() coordinate transformation to properly account for zoom scaling of offset. Formula corrected: `canvas = (screen - canvasPos - offset*zoom) / zoom`
    * ✅ Panning Offset Compensation: Automatic correction through ScreenToCanvas() fix
    * ✅ Snap-to-Grid Removed: Eliminated grid snapping for free-form node positioning
    * ✅ Context Menu Safety: Removed "Delete All Nodes" (too risky), improved menu structure with separators
    * ✅ Component Palette Integration: Added resizable split panel (Canvas 75% | Palette 25%)
  - **Component Palette Features**:
    * Text search with case-insensitive substring matching
    * Category tabs: All, Core, Physics, Graphics, AI, Gameplay
    * 10 registered component types (Transform, Identity, Movement, Sprite, Collision, Health, AIBlackboard, BehaviorTree, VisualSprite, AnimationController)
    * Double-click to instantiate new nodes at default position
    * Tooltips with component descriptions
    * Drag-to-resize split panel between canvas and palette
  - **Dirty Flag System**:
    * `EntityPrefabGraphDocument::m_isDirty` tracks modifications (CreateNode, RemoveNode, ConnectNodes, DisconnectNodes)
    * Cleared on LoadFromFile() and SaveToFile()
    * EntityPrefabRenderer::IsDirty() checks document dirty flag for UI feedback
  - **ImGui Input Integration**:
    * Added ImGui input polling in PrefabCanvas::Render()
    * Polls mouse clicks/movement/scroll, keyboard modifiers (Ctrl, Shift)
    * Dispatches to OnMouseMove, OnMouseDown, OnMouseUp, OnMouseScroll, OnKeyDown handlers
    * Window hover checking ensures input only when canvas is focused
  - **Key Classes**:
    * `ComponentPalettePanel`: Searchable component registry with category filtering
    * `PrefabCanvas`: Canvas rendering and input handling (fixed ScreenToCanvas coordinate transform)
    * `EntityPrefabRenderer`: Orchestrates layout (canvas + palette side-by-side)
    * `EntityPrefabGraphDocument`: Data model with dirty flag tracking
    * `ComponentNodeRenderer`: Node/connection visual rendering
  - **JSON Schema**: Mirrors LoadFromFile structure with nodes[], connections[], canvasState
  - **Current Status**: ✅ All features working, bugs fixed, build successful (0 errors)
  - **Integration**: Works seamlessly with ImGui event loop; resizable split panel layout

- Phase 29 (COMPLETED - Entity Prefab Editor Phase 3 - Drag-Drop + Menu):
  - **Completed Features**:
    * ✅ **Drag-Drop Component Instantiation**: Components from palette drag onto canvas to create new nodes at correct position
    * ✅ **Coordinate Transformation Fix**: Fixed critical bug in `ScreenToCanvas()` formula - removed incorrect `* m_canvasZoom` multiplication on offset term
      - Correct formula: `canvas = (screen - canvasPos - offset) / zoom` (NOT `offset * zoom`)
      - Applies to both `PrefabCanvas::ScreenToCanvas()` and new `AcceptComponentDropAtScreenPos()`
    * ✅ **Zoom/Pan Coordinate Handling**: Drag-drop works correctly with any zoom level (0.1x to 3.0x) and any pan offset
    * ✅ **Canvas Screen Position Tracking**: Added `m_canvasScreenPos` member to PrefabCanvas to store canvas position during Render() call, used by drag-drop handler
    * ✅ **New Entity Prefab Menu**: Added "New Entity Prefab" to File menu (Ctrl+Alt+N keyboard shortcut)
    * ✅ **Removed Spam Logs**: Cleaned up excessive logging from drag-drop operations
  - **Architecture Notes**:
    * Drag-drop uses ImGui's invisible overlay technique (SetCursorScreenPos + Dummy + BeginDragDropTarget)
    * Overlay positioned outside main canvas BeginChild to work across palette↔canvas windows
    * Coordinate flow: Screen (absolute) → Canvas-relative (subtract canvasPos) → Canvas logical (apply zoom/pan transformation via ScreenToCanvas)
  - **Bug Fixes**:
    * ✅ Coordinate Transformation: `ScreenToCanvas()` was multiplying offset by zoom incorrectly
      - Before: `canvas = (screen - canvasPos - offset*zoom) / zoom` ❌
      - After: `canvas = (screen - canvasPos - offset) / zoom` ✅
    * ✅ Added `GetCanvasScreenPos()` getter and `AcceptComponentDropAtScreenPos()` method to bypass ImGui context issues during drag-drop
  - **Files Modified**:
    * PrefabCanvas.cpp/h - Fixed coordinate transformation, added canvas screen position tracking
    * EntityPrefabRenderer.cpp - Implemented drag-drop target with overlay technique
    * ComponentPalettePanel.cpp - Drag source setup (removed spam logs)
    * BlueprintEditorGUI.cpp - Added "New Entity Prefab" menu entry and Ctrl+Alt+N shortcut
  - **Current Status**: ✅ Phase 29 complete - Full drag-drop working with correct coordinates at all zoom/pan levels
  - **Build**: ✅ 0 errors, 0 warnings

- Phase 29b (COMPLETED - Entity Prefab Editor - Component Panel Dynamic Loading):
  - **Architecture Understanding**:
    * **ComponentPalettePanel** ≠ **ParameterSchemaRegistry**
      - ComponentPalettePanel: UI list of available component types (Transform, Identity, Movement, etc.)
      - ParameterSchemaRegistry: Type definitions for component parameters (Position_data has "position" Vector3, etc.)
    * **Previous State**: Component types hardcoded in ComponentPalettePanel::Initialize() (10 types registered in C++)
    * **New State**: Component types loaded from `./Gamedata/PrefabEntities/ComponentsParameters.json` with hardcoded fallback
  - **Implemented Features**:
    * ✅ **LoadComponentsFromJSON() Method**: Reads component definitions from JSON file
    * ✅ **Fallback System**: If JSON file not found, uses hardcoded components (backward compatible)
    * ✅ **Error Handling**: Graceful handling of missing/invalid JSON with detailed logging
    * ✅ **Category Auto-Generation**: Dynamically rebuilds category list from loaded components
    * ✅ **JSON File Created**: `./Gamedata/PrefabEntities/ComponentsParameters.json` with all 10 standard components
  - **Parameter Type Support**:
    * Supported types in ComponentParameter enum: Bool, Int, Float, String, Vector2, Vector3, Color, Array, EntityRef
    * Vector types fully supported with dual JSON format:
      - Object format: `{"x": 1.0, "y": 2.0, "z": 3.0}` (human-readable)
      - Array format: `[1.0, 2.0, 3.0]` (compact, preferred)
    * ParseParameterWithSchema() automatically handles both formats
  - **Implementation Details**:
    * Method: `ComponentPalettePanel::LoadComponentsFromJSON(const std::string& filepath)` (new)
    * Modified: `ComponentPalettePanel::Initialize()` - now attempts JSON load first with fallback
    * Updated: ComponentPalettePanel.h - Added LoadComponentsFromJSON() declaration
    * Updated: ComponentPalettePanel.cpp - Added #include nlohmann/json.hpp and <fstream>
  - **JSON Structure** (./Gamedata/PrefabEntities/ComponentsParameters.json):
    ```json
    {
      "components": [
        {
          "name": "Transform",
          "category": "Core",
          "description": "Position, rotation, scale",
          "parameters": [
            { "name": "position", "type": "Vector3", "defaultValue": [0.0, 0.0, 0.0] },
            { "name": "rotation", "type": "Vector3", "defaultValue": [0.0, 0.0, 0.0] },
            { "name": "scale", "type": "Vector3", "defaultValue": [1.0, 1.0, 1.0] }
          ]
        },
        { "name": "Identity", "category": "Core", ... },
        { "name": "Movement", "category": "Physics", ... },
        ... (10 components total)
      ]
    }
    ```
  - **Logging Output**:
    * "[ComponentPalettePanel] Loading components from: ./Gamedata/PrefabEntities/ComponentsParameters.json"
    * "[ComponentPalettePanel] Found X component types"
    * "[ComponentPalettePanel] Loaded: ComponentName (Category)"
    * "[ComponentPalettePanel] Successfully loaded N components from JSON (M categories)"
  - **Files Modified**:
    * ComponentPalettePanel.h - Added LoadComponentsFromJSON() declaration
    * ComponentPalettePanel.cpp - Implemented JSON loading with error handling and fallback
    * NEW: Gamedata/PrefabEntities/ComponentsParameters.json - Component definitions
  - **Backward Compatibility**: ✅ If JSON file missing, automatically falls back to hardcoded components (10 types)
  - **Current Status**: ✅ Phase 29b complete - Components now loaded from JSON with fallback
  - **Build**: ✅ 0 errors, 0 warnings

- Phase 30 (FUTURE - Entity Prefab Editor Phase 3 - Connection UI + Selection):
  - Connection creation UI (drag from node port to create edges)
  - Rectangle selection (click-drag in empty space to multi-select nodes)
  - Property editing panel (select node → edit properties in inspector)
  - Undo/Redo system for edit history

- Phase 31 (FUTURE - Entity Prefab Editor Optimization):
  - Performance profiling and optimization
  - Large graph support (1000+ nodes)
  - Minimap/viewport controls for large graphs
  - Export to runtime format
  - Copy/Paste nodes and subgraphs

## Architecture Reference - Entity Prefab Editor Data & Parameter System

### Component Registry Architecture
**Two independent but complementary systems:**

1. **ComponentPalettePanel** (UI Layer)
   - Location: `Source/BlueprintEditor/EntityPrefabEditor/ComponentPalettePanel.cpp`
   - Purpose: UI list of draggable component types available for instantiation
   - Data Source: Currently HARDCODED in C++ (10 types: Transform, Identity, Movement, Sprite, Collision, Health, AIBlackboard, BehaviorTree, VisualSprite, AnimationController)
   - Future: Will load from `./Gamedata/PrefabEntities/ComponentsParameters.json`
   - Responsibilities: Rendering, search filtering, category tabs, drag-drop source

2. **ParameterSchemaRegistry** (Schema Layer)
   - Location: `Source/ParameterSchema.cpp` / `Source/ParameterSchema.h`
   - Purpose: Schema definitions for component parameters (what fields/parameters each component type has)
   - Data Source: Registered dynamically via ParameterSchemaRegistry::InitializeBuiltInSchemas()
   - Contains schemas for: Position_data, PhysicsBody_data, AIBlackboard_data, VisualSprite_data, Identity_data, etc.
   - Responsibilities: Type validation, parameter parsing, default value management

### Component Parameter System
**Supported Types in ComponentParameter enum:**
- Bool, Int, Float, String
- Vector2, Vector3 (with dual JSON format support)
- Color (supports hex and rgba formats)
- Array (nlohmann::json-based for C++14 compatibility)
- EntityRef (entity ID reference)

**Vector Parameter JSON Formats** (both supported by ParseParameterWithSchema):
- Object format: `{"x": 1.5, "y": 2.3, "z": 0.0}` (human-readable)
- Array format: `[1.5, 2.3, 0.0]` (compact, preferred)

**Example Component Schema Definition:**
```cpp
// Position_data component in ParameterSchemaRegistry
RegisterParameterSchema(ParameterSchemaEntry(
    "position",              // Parameter name in UI/JSON
    "Position_data",         // Component type
    "position",              // Target field name
    ComponentParameter::Type::Vector3,  // Type
    true,                    // Required
    ComponentParameter::FromVector3(0.0f, 0.0f, 0.0f)  // Default
));
```

**Example Entity Prefab Node with Vector3 Parameters:**
```json
{
  "nodes": [
    {
      "nodeId": 1,
      "componentType": "Transform",
      "componentName": "MainTransform",
      "position": [10.5, 20.3, 0.0],
      "size": [150.0, 80.0, 0.0],
      "enabled": true,
      "properties": {
        "position": [100.0, 150.0, 0.0],
        "rotation": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0]
      }
    }
  ]
}
```

### Data Flow for Node Creation
1. User drags component from ComponentPalettePanel
2. Component name passed via ImGui drag-drop payload
3. PrefabCanvas::AcceptComponentDropAtScreenPos() receives screen coordinates
4. ScreenToCanvas() transforms to canvas logical coordinates
5. EntityPrefabGraphDocument::CreateComponentNode(componentType, componentName)
6. New ComponentNode created with position, size, enabled flag
7. Node properties can be populated from ParameterSchemaRegistry defaults

## File Management Protocol
- To add new files to the OlympeBlueprintEditor project:
  1. Create new C++ files using the `create_file` tool with complete implementation.
  2. Specify which project the files belong to (typically OlympeBlueprintEditor.vcxproj or Olympe Engine.vcxproj).
  3. Manually add files to the project via Visual Studio: Right-click project → "Add Existing Item" → select file(s) → OK → Compile to verify.
- This approach avoids XML manipulation errors and is faster/more reliable than programmatic project file editing.
- For code modifications to existing files, use `replace_string_in_file` directly without user intervention.