/**
 * @file CANVAS_ARCHITECTURE_ANALYSIS.md
 * @brief Complete analysis of why PrefabCanvas doesn't use imnodes and path to standardization
 * @details Understanding the architectural divergence and roadmap for centralization
 */

# Canvas Architecture Analysis: Why Divergence & Path to Standardization

## Executive Summary

**Current State**: Two completely separate implementations
- **VisualScript** (VisualScriptEditorPanel): Uses `ImNodes::BeginNodeEditor/EndNodeEditor` (native imnodes)
- **PrefabCanvas** (EntityPrefabEditor): Custom manual implementation (no imnodes dependency)

**The Core Problem**: imnodes does NOT expose zoom control
- imnodes is hardcoded to 1.0x zoom (fixed scale)
- imnodes pan offset is managed internally
- We needed zoom for PrefabCanvas (0.1x - 3.0x)
- Therefore: Had to create custom implementation separate from imnodes

**The Opportunity**: Standardize common functionality (pan, zoom, grid rendering)
- Create `ICanvasEditor` abstraction layer
- Implement `ImNodesCanvasEditor` adapter (wraps imnodes)
- Implement `CustomCanvasEditor` (current PrefabCanvas logic)
- Unify grid rendering, coordinate transformation, lifecycle

---

## Deep Analysis: Why imnodes Cannot Provide Zoom

### imnodes API Capabilities (What IS Exposed)

```cpp
// Pan Management (EXPOSED - Can READ and WRITE)
ImVec2 EditorContextGetPanning();              // ✅ Read current pan offset
void EditorContextResetPanning(const ImVec2&); // ✅ Set pan offset programmatically

// Node Position Systems (EXPOSED - Three coordinate spaces)
void SetNodeScreenSpacePos(int id, ImVec2);    // ✅ Screen absolute coordinates
void SetNodeEditorSpacePos(int id, ImVec2);    // ✅ Editor space (pan origin)
void SetNodeGridSpacePos(int id, ImVec2);      // ✅ Grid space (pan-independent)

ImVec2 GetNodeScreenSpacePos(int id);          // ✅ Read screen space
ImVec2 GetNodeEditorSpacePos(int id);          // ✅ Read editor space
ImVec2 GetNodeGridSpacePos(int id);            // ✅ Read grid space

// Context Management (EXPOSED)
ImNodesEditorContext* EditorContextCreate();   // ✅ Create new context per tab
void EditorContextSet(context);                // ✅ Switch context for multi-panel

// Grid Control (EXPOSED - Flags)
ImNodesStyleFlags_GridLines = 1 << 2;          // ✅ Enable/disable grid
ImNodesStyleFlags_GridLinesPrimary = 1 << 3;   // ✅ Enable/disable primary line
ImNodesStyleFlags_GridSnapping = 1 << 4;       // ✅ Enable/disable snap-to-grid

// Grid Styling (EXPOSED)
float GridSpacing = 24.0f;                     // ✅ Configure spacing
ImVec4 GridBackground color;                   // ✅ Configure colors
ImVec4 GridLine, GridLinePrimary colors;       // ✅ Configure colors
```

### imnodes API Gaps (What is NOT Exposed)

```cpp
// ❌ NO ZOOM CONTROL
// - GridSpacing is fixed (24px)
// - No zoom multiplier parameter
// - No ScaleLevel / ZoomLevel API
// - Grid spacing cannot be runtime-adjusted

// ❌ NO CUSTOM GRID RENDERING
// - Grid drawn internally in DrawGrid() (line 1391 imnodes.cpp)
// - No hook/callback to customize grid rendering
// - Cannot add minor grid lines
// - Cannot change line thickness dynamically

// ❌ NO COORDINATE TRANSFORMATION UTILITIES
// - EditorSpaceToScreenSpace() is INTERNAL (not exposed)
// - ScreenSpaceToEditorSpace() is INTERNAL (not exposed)
// - Users must implement coordinate math themselves

// ❌ NO INPUT HANDLING EXPOSURE
// - Pan drag detection is internal
// - Zoom scroll detection not available
// - No raw input events
// - Only high-level "IsNodeHovered()", "IsLinkCreated()"
```

### Why Zoom is Architecturally Incompatible with Current imnodes

**Problem 1: Grid Spacing is Hardcoded**
```cpp
// In imnodes.cpp DrawGrid() - line 1405-1415
for (float x = fmodf(offset.x, GImNodes->Style.GridSpacing); 
     x < canvas_size.x;
     x += GImNodes->Style.GridSpacing)
{
    // GridSpacing is always 24px
    // No multiplication by zoom
    // Grid doesn't scale with zoom
}
```

**Problem 2: Canvas Size Doesn't Account for Zoom**
```cpp
// imnodes expects canvas_size in SCREEN PIXELS
// If we zoom 2x, nodes should be larger, but canvas_size doesn't change
// Drawing 24px grid on zoomed-2x space looks wrong
```

**Problem 3: Pin Detection Assumes 1.0x Scale**
```cpp
// Pin click detection uses GetMousePos() directly
// With zoom, click positions need transformation (multiply/divide by zoom)
// imnodes doesn't do this transformation
```

---

## Current Architecture Comparison

| Aspect | imnodes (VisualScript) | Custom (PrefabCanvas) |
|--------|----------------------|---------------------|
| **Zoom** | ❌ Fixed 1.0x | ✅ 0.1x - 3.0x |
| **Pan** | ✅ EditorContextGetPanning() | ✅ m_canvasOffset manual |
| **Grid** | ✅ Built-in DrawGrid() | ✅ CanvasGridRenderer custom |
| **Grid Zoom** | ❌ Not scaled | ✅ scaled spacing |
| **Minor Grid** | ❌ Single spacing | ✅ Major + minor |
| **Coordinate Transform** | Internal API | Manual math |
| **Node Position** | Grid/Editor/Screen space | Grid space + manual math |
| **Link Drawing** | Built-in | Manual (planned) |
| **Selection** | Built-in (Ctrl+Click) | Rectangle selection custom |
| **Context Switching** | ✅ Per-editor context | N/A (single canvas) |
| **Undo/Redo** | Custom integration | Not yet integrated |

---

## Technical Debt & Why Standardization Matters

### Current Situation (Pre-Standardization)

**Problem 1: Duplicate Code**
- Pan/Zoom logic exists in VisualScript (implicit in imnodes)
- Pan/Zoom logic exists in PrefabCanvas (explicit manual code)
- Grid rendering duplicated (imnodes internal + CanvasGridRenderer)
- Coordinate math duplicated (GetNodeEditorSpacePos + manual transforms)

**Problem 2: Inconsistent Behavior**
- VisualScript: Middle-mouse to pan (imnodes default)
- PrefabCanvas: Middle-mouse to pan (m_canvasOffset logic)
- But they pan DIFFERENTLY due to different underlying systems
- Grid appears different despite using same colors (due to zoom scaling differences)

**Problem 3: Hard to Add Features**
- Want to add minimap? Must implement twice
- Want zoom indicator? Must implement twice
- Want snap-to-grid? Must implement twice
- Want keyboard shortcuts for pan/zoom? Must implement twice

**Problem 4: Difficult to Maintain**
- New developer doesn't understand why two systems exist
- Bug fix in one doesn't transfer to other
- Feature parity broken (PrefabCanvas has zoom, VisualScript doesn't)
- Hard to test (two code paths)

---

## Standardization Strategy: ICanvasEditor Abstraction

### Architecture Diagram

```
ICanvasEditor (Abstract Base)
├── Lifecycle
│   ├── BeginRender()
│   ├── EndRender()
│   └── Input handling (pan, zoom, select)
├── Coordinate Systems
│   ├── ScreenToCanvas(screenPos) → canvasPos
│   ├── CanvasToScreen(canvasPos) → screenPos
│   └── GridToEditor(gridPos) → editorPos
├── Pan/Zoom Management
│   ├── GetPan() → offset
│   ├── SetPan(offset)
│   ├── GetZoom() → scale
│   ├── SetZoom(scale, zoomCenter)
│   └── ResetView()
├── Grid Rendering
│   ├── RenderGrid(preset)
│   └── GetGridConfig()
└── Events
    ├── SelectNode(id)
    ├── OnNodeDrag(id, delta)
    └── OnLinkCreated(fromId, toId)

    ↓

ImNodesCanvasEditor : ICanvasEditor
├── Wraps ImNodes::BeginNodeEditor/EndNodeEditor
├── Pan control via EditorContextGetPanning()
├── Zoom is "1.0x" (read-only wrapper returning 1.0f)
├── Grid delegated to CanvasGridRenderer
└── Future: expose zoom once imnodes supports it

    ↓

CustomCanvasEditor : ICanvasEditor
├── Manual pan (m_canvasOffset)
├── Manual zoom (m_canvasZoom 0.1x-3.0x)
├── Uses CanvasGridRenderer with zoom scaling
├── Input handling (drag, scroll, select)
└── Coordinate transformation with zoom math
```

### What Gets Unified

**1. Grid Rendering** ✅ Already via CanvasGridRenderer
```cpp
// Both use:
CanvasGridRenderer::GridConfig config = GetStylePreset(Style_VisualScript);
CanvasGridRenderer::RenderGrid(config);
```

**2. Coordinate Transformation** → New CanvasCoordinateTransformer
```cpp
class CanvasCoordinateTransformer
{
public:
    ImVec2 ScreenToCanvas(ImVec2 screenPos, ImVec2 canvasPos, 
                         ImVec2 pan, float zoom) const;
    ImVec2 CanvasToScreen(ImVec2 canvasPos, ImVec2 canvasPos, 
                         ImVec2 pan, float zoom) const;
    ImVec2 GetGridSpaceFromEditor(ImVec2 editorPos, ImVec2 pan) const;
};
```

**3. Pan/Zoom API** → ICanvasEditor interface
```cpp
class ICanvasEditor
{
public:
    virtual void BeginRender() = 0;
    virtual void EndRender() = 0;
    virtual ImVec2 GetPan() const = 0;
    virtual void SetPan(ImVec2 offset) = 0;
    virtual float GetZoom() const = 0;
    virtual void SetZoom(float scale, ImVec2 zoomCenter) = 0;
    // ... more
};
```

**4. Lifecycle Management** → Common pattern
```cpp
// VisualScript
void VisualScriptEditorPanel::RenderCanvas()
{
    m_canvasEditor->BeginRender();
    // ... render nodes
    m_canvasEditor->EndRender();
}

// PrefabCanvas
void EntityPrefabRenderer::Render()
{
    m_canvasEditor->BeginRender();
    // ... render components
    m_canvasEditor->EndRender();
}
```

### What Stays Separate

**1. Node/Link Rendering**
- VisualScript: Uses imnodes node/link rendering
- PrefabCanvas: Uses custom ComponentNodeRenderer
- These are UI-domain specific, not canvas-specific

**2. Selection Logic**
- VisualScript: Uses imnodes selection (Ctrl+Click, box select built-in)
- PrefabCanvas: Uses custom rectangle selection
- These match domain requirements, not a bug

**3. Undo/Redo System**
- VisualScript: Already has UndoStack
- PrefabCanvas: Will need to add similar
- These are command pattern, can be independent

---

## Implementation Roadmap

### Phase 1: Foundation (Step 1-3 in plan)
1. Analyze imnodes API completely ✓ (this document)
2. Create `ICanvasEditor` base class
3. Create `ImNodesCanvasEditor` adapter

### Phase 2: Integration (Step 4-6 in plan)
4. Create `CustomCanvasEditor` for PrefabCanvas
5. Refactor VisualScript to use `ImNodesCanvasEditor`
6. Refactor PrefabCanvas to use `CustomCanvasEditor`

### Phase 3: Utilities (Step 7)
7. Create `CanvasCoordinateTransformer` shared utility

### Phase 4: Validation (Step 8-9)
8. Test both canvases
9. Document architecture

### Phase 5: Future Enhancements (Step 10)
10. Plan next features (undo/redo centralization, input standardization, minimap, etc.)

---

## Risk Assessment

### Low Risk
- ✅ Creating base class interface (no changes to existing code)
- ✅ Creating adapter (imnodes stays unchanged)
- ✅ Creating utilities (purely additive)

### Medium Risk
- ⚠️ Refactoring VisualScript to use adapter
  - Mitigation: Adapter just wraps existing API, minimal logic changes
  - Verification: Run full test suite

- ⚠️ Refactoring PrefabCanvas coordinate transforms
  - Mitigation: Keep existing zoom/pan logic, just wrap in interface
  - Verification: Test zoom at all levels (0.1x, 1.0x, 3.0x)

### Minimal Risk
- ✅ Both canvases keep their node/link rendering (no impact)
- ✅ Both canvases keep their selection logic (no impact)
- ✅ Both canvases keep their undo/redo (independent)

---

## Why This Matters for the Team

### For New Developers
- **Before**: "Why are there two canvas implementations? How do they work?"
- **After**: "ICanvasEditor is the contract. ImNodesCanvasEditor uses imnodes. CustomCanvasEditor for zoom."

### For Bug Fixes
- **Before**: Fix bug in pan logic, must fix in two places
- **After**: Coordinate transform is in one place (CanvasCoordinateTransformer)

### For New Features
- **Before**: Add minimap → implement twice or choose which editor gets it
- **After**: Add to ICanvasEditor → both canvases get it automatically

### For Performance
- **Before**: Optimize grid rendering in two places (imnodes + CanvasGridRenderer)
- **After**: Optimize once in CanvasGridRenderer, both canvases benefit

---

## Conclusion

imnodes cannot provide zoom because:
1. **Architectural limitation**: Grid spacing is hardcoded
2. **API limitation**: No zoom exposure in public API
3. **Scale incompatibility**: Pin detection, grid rendering assume 1.0x

The solution is not "force imnodes to zoom" but rather:
1. **Accept imnodes as 1.0x solution** (ideal for standard node editor)
2. **Keep custom for zooming needs** (perfect for component layout, minimap preview)
3. **Unify common concerns** (grid, pan, coordinate transforms)

This gives us:
- **VisualScript**: Full imnodes power + standardized pan/grid
- **PrefabCanvas**: Freedom to zoom + standardized coordinate math
- **Both**: Shared grid rendering, shared coordinate transformer, consistent behavior

Result: Professional, maintainable, scalable canvas architecture for the entire editor.
