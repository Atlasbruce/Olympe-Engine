# Blueprint Editor - Dual Mode Implementation

## Overview
This document describes the implementation of dual editor modes (Runtime and Standalone) for the Olympe Engine Blueprint Editor, along with fixes for several critical regressions.

## Issues Addressed

### 1. Tab Selection Regression (FIXED ✅)
**Problem**: Changing tabs would always revert to the first tab.

**Root Cause**: `ImGui::BeginTabItem()` returns true for the currently visible tab every frame, causing `SetActiveGraph()` to be called repeatedly, potentially switching back to the first tab.

**Solution**:
- Cache the active graph ID before rendering tabs
- Use `ImGuiTabItemFlags_SetSelected` flag for the active tab to ensure visual consistency
- Only call `SetActiveGraph()` when the active tab differs from the cached value
- This ensures tabs stay selected as expected

**Code Changes**:
```cpp
// Before: Always called SetActiveGraph inside BeginTabItem
if (ImGui::BeginTabItem(graphName.c_str(), nullptr, ImGuiTabItemFlags_None))
{
    if (NodeGraphManager::Get().GetActiveGraphId() != graphId)
        NodeGraphManager::Get().SetActiveGraph(graphId);
}

// After: Cache active ID and use SetSelected flag
int activeGraphId = NodeGraphManager::Get().GetActiveGraphId();
ImGuiTabItemFlags flags = (graphId == activeGraphId) 
    ? ImGuiTabItemFlags_SetSelected 
    : ImGuiTabItemFlags_None;
if (ImGui::BeginTabItem(graphName.c_str(), nullptr, flags))
{
    if (activeGraphId != graphId)
        NodeGraphManager::Get().SetActiveGraph(graphId);
}
```

### 2. Drag & Drop Crash (FIXED ✅)
**Problem**: Adding a node via drag & drop from the node palette would crash the application.

**Root Cause**: `ImGui::BeginDragDropTarget()` was called AFTER `ImNodes::EndNodeEditor()`, meaning it was outside the ImNodes context. This caused coordinate conversion issues and crashes when calling `ImNodes::ScreenSpaceToGridSpace()`.

**Solution**:
- Move drag & drop handling BEFORE `ImNodes::EndNodeEditor()`
- Guard with `ImNodes::IsEditorHovered()` to ensure we're in the right context
- This ensures proper ImNodes coordinate system is active when converting mouse position to grid space

**Code Changes**:
```cpp
// Before: Drag & drop target after EndNodeEditor()
ImNodes::EndNodeEditor();
// ... other code ...
if (ImGui::BeginDragDropTarget())
{
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_TYPE"))
    {
        ImVec2 canvasPos = ImNodes::ScreenSpaceToGridSpace(mouseScreenPos); // CRASH!
    }
}

// After: Drag & drop target before EndNodeEditor()
if (ImNodes::IsEditorHovered())
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_TYPE"))
        {
            ImVec2 canvasPos = ImNodes::ScreenSpaceToGridSpace(mouseScreenPos); // Works!
        }
    }
}
ImNodes::EndNodeEditor();
```

### 3. CRUD Options Visibility (VERIFIED ✅)
**Status**: CRUD options are present and accessible.

**Available Operations**:
- **Delete Key**: Press Del to delete selected node or link
- **Right-Click Context Menu**: Edit, Duplicate, Delete options
- **Keyboard Shortcuts**: Ctrl+Z (Undo), Ctrl+Y (Redo), Ctrl+D (Duplicate)
- **Double-Click**: Open node edit modal
- **Drag Nodes**: Create links by dragging between node ports

## New Feature: Dual Editor Modes

### Blueprint Editor Mode Enum
```cpp
enum class BlueprintEditorMode
{
    Standalone,     // Full CRUD editing, no entity binding
    Runtime         // Read-only or light operations, for runtime entity inspection
};
```

### Mode 1: Runtime Blueprint Editor
**Purpose**: Visualization and inspection of blueprints during runtime execution.

**Characteristics**:
- **Read-Only**: No creation, editing, or deletion of nodes
- **Inspection**: View node properties and parameters
- **Entity Context**: Can be linked to runtime entities for debugging
- **Use Cases**: 
  - Debugging behavior trees during gameplay
  - Inspecting entity blueprints at runtime
  - Monitoring state machines in real-time

**Disabled Operations**:
- Node creation (drag & drop, context menu, "+" button)
- Node editing (parameters, types)
- Node deletion (Delete key)
- Link creation and deletion
- Node position updates
- Keyboard shortcuts (Ctrl+Z/Y, Ctrl+D)

**Enabled Operations**:
- Viewing all nodes and links
- Viewing node properties (read-only)
- Panning and zooming the graph
- Tab navigation

**Visual Indicators**:
- Window title: "Node Graph Viewer (Runtime)"
- Yellow badge: "[Runtime Mode - Read Only]"
- Mode switch button: "Switch to Standalone"
- Context menu: "View" instead of "Edit"
- Modal dialog: "View Node" with "Close" button only

### Mode 2: Standalone Blueprint Editor
**Purpose**: Full CRUD editing of blueprints, independent of runtime entities.

**Characteristics**:
- **Full Editing**: Complete node creation, editing, and deletion
- **No Entity Binding**: Works with blueprint files directly
- **Professional Tools**: Undo/redo, keyboard shortcuts, drag & drop
- **Use Cases**:
  - Designing new behavior trees
  - Creating state machines
  - Editing blueprint templates
  - Batch editing multiple graphs

**Available Operations**:
- All CRUD operations
- Drag & drop from node palette
- Context menus with full options
- Keyboard shortcuts
- Node position updates
- Link creation and deletion
- Undo/redo support

**Visual Indicators**:
- Window title: "Node Graph Editor (Standalone)"
- Green badge: "[Standalone Mode - Full Editing]"
- Mode switch button: "Switch to Runtime"
- Context menu: "Edit", "Duplicate", "Delete"
- Modal dialog: "Edit Node" with "OK" and "Cancel" buttons

### Implementation Details

#### NodeGraphPanel.h Changes
1. Added `BlueprintEditorMode` enum
2. Added `m_EditorMode` member (default: Standalone)
3. Added `SetEditorMode()` and `GetEditorMode()` methods
4. Added `IsReadOnly()` helper method

#### NodeGraphPanel.cpp Changes
1. **Render() method**:
   - Dynamic window title based on mode
   - Visual badge showing current mode
   - Mode switch button for testing
   - Conditional keyboard shortcut handling
   - Conditional graph creation buttons

2. **RenderGraphTabs() method**:
   - Conditional "+" button (Standalone only)
   - Conditional graph creation popup (Standalone only)

3. **RenderGraph() method**:
   - Conditional drag & drop handling (Standalone only)
   - Conditional Delete key handler (Standalone only)
   - Conditional link creation (Standalone only)
   - Conditional node position updates (Standalone only)
   - Conditional context menu items (Standalone only)

4. **RenderNodeEditModal() method**:
   - Different modal title based on mode
   - Read-only name display in Runtime mode
   - Read-only type/parameter display in Runtime mode
   - Different buttons: "Close" (Runtime) vs "OK"/"Cancel" (Standalone)

## Testing Recommendations

### Manual Testing
1. **Tab Selection**:
   - Open multiple graphs
   - Switch between tabs
   - Verify active tab stays selected
   - Verify no automatic switching occurs

2. **Drag & Drop**:
   - Open node palette
   - Drag nodes onto canvas
   - Verify no crashes
   - Verify correct node placement

3. **Runtime Mode**:
   - Switch to Runtime mode
   - Verify all editing is disabled
   - Verify viewing works correctly
   - Verify mode switch button works

4. **Standalone Mode**:
   - Switch to Standalone mode
   - Verify all editing works
   - Test keyboard shortcuts
   - Test CRUD operations

### Integration Testing
1. Test with runtime entities attached
2. Test with standalone blueprint files
3. Test mode switching during active editing
4. Test undo/redo with mode changes

## Compatibility

### C++14 Compliance
- Uses `enum class` (C++11 feature, available in C++14)
- No C++17+ features used
- Compatible with existing codebase

### JSON API Compatibility
- No changes to JSON structure
- Uses existing nlohmann::json minimal API
- Compatible with JsonHelper utilities

## Future Enhancements

### Potential Improvements
1. **Persistent Mode Setting**: Save preferred mode to config file
2. **Per-Graph Mode**: Allow different modes for different graphs
3. **Runtime Breakpoints**: Add breakpoint support in Runtime mode
4. **Mode Permissions**: Add user-level restrictions for modes
5. **Custom Modes**: Allow plugins to define custom modes

### API Extensions
```cpp
// Potential future API
class NodeGraphPanel
{
public:
    // Advanced mode management
    void SetDefaultMode(BlueprintEditorMode mode);
    void LockMode(bool locked);
    bool IsModeChangeable() const;
    
    // Mode-specific callbacks
    using ModeChangeCallback = std::function<void(BlueprintEditorMode)>;
    void SetModeChangeCallback(ModeChangeCallback callback);
};
```

## Conclusion

This implementation provides a robust dual-mode system for the Blueprint Editor:
1. **Runtime mode** for safe, read-only visualization during execution
2. **Standalone mode** for full-featured blueprint editing
3. **Clear visual indicators** to prevent user confusion
4. **Fixed regressions** that were blocking basic functionality

All changes are minimal, focused, and maintain compatibility with the existing codebase.
