# Framework Integration Guide: What Every New Graph Type Needs

## Overview
When adding a new graph editor type (e.g., EntityPrefabEditorV2, PlaceholderGraphRenderer), you must implement a complete rendering pipeline. This guide explains EXACTLY what's required.

**Reference Implementation**: PlaceholderGraphRenderer (complete example)

---

## CRITICAL PATTERNS

### Pattern 1: RenderCommonToolbar() - The Complete Toolbar Pattern

```cpp
void YourGraphRenderer::RenderCommonToolbar()
{
    // ====== STEP 1: Framework Toolbar (Save/SaveAs/Browse) ======
    // This is REQUIRED - renders unified file operations
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->Render();  // LINE 1: CRITICAL CALL
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::Separator();
        ImGui::SameLine(0.0f, 20.0f);
    }

    // ====== STEP 2: Grid Toggle Checkbox ======
    // Connects UI checkbox to canvas SetGridVisible() method
    if (ImGui::Checkbox("Grid##toolbar", &m_gridVisible))
    {
        if (m_canvas) m_canvas->SetGridVisible(m_gridVisible);
    }
    ImGui::SameLine(0.0f, 10.0f);

    // ====== STEP 3: Reset View Button ======
    // Resets pan and zoom to default state
    if (ImGui::Button("Reset View##btn", ImVec2(80, 0))) {
        if (m_canvas) m_canvas->ResetPanZoom();
    }
    ImGui::SameLine(0.0f, 10.0f);

    // ====== STEP 4: Minimap Toggle Checkbox ======
    // Connects UI checkbox to canvas SetMinimapVisible() method
    if (ImGui::Checkbox("Minimap##toolbar", &m_minimapVisible))
    {
        if (m_canvas) m_canvas->SetMinimapVisible(m_minimapVisible);
    }
}
```

**Key Points:**
- All 3 toolbar buttons REQUIRED (Grid, Reset View, Minimap)
- Separators for visual grouping
- Each button must call corresponding canvas method

---

### Pattern 2: Canvas::RenderGrid() - The Grid Rendering Pattern

Grid rendering must be called FIRST in render order:

```cpp
void PlaceholderCanvas::Render()
{
    if (!m_document) {
        ImGui::Text("No document loaded");
        return;
    }

    // Input handling (pan, zoom, selection)
    HandlePanZoomInput();
    HandleNodeInteraction();
    HandleDragDropInput();

    // Rendering in order: grid → connections → nodes → selection → preview → menu → minimap
    RenderGrid();          // <-- FIRST: Grid background
    RenderConnections();
    RenderNodes();
    RenderSelectionRectangle();
    RenderConnectionPreviewLine();
    RenderContextMenu();
    RenderMinimap();       // <-- LAST: Minimap overlay
}
```

**RenderGrid() Implementation:**

```cpp
void YourCanvas::RenderGrid()
{
    // Step 1: Check grid visibility (state from toolbar checkbox)
    if (!m_gridVisible) {
        // Draw background only, no grid lines
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        const ImU32 bgColor = IM_COL32(38, 38, 47, 255);        // #26262FFF
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bgColor);
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    // Step 2: Grid parameters (Phase 5 standardization)
    const float gridSpacing = 24.0f * m_canvasZoom;           // Scale with zoom
    const ImU32 gridColor = IM_COL32(63, 63, 71, 255);        // #3F3F47FF (imnodes color)
    const ImU32 bgColor = IM_COL32(38, 38, 47, 255);          // #26262FFF (imnodes color)

    // Step 3: Draw background
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bgColor);

    // Step 4: Draw grid lines with pan offset compensation
    float startX = fmodf(m_canvasOffset.x * m_canvasZoom, gridSpacing);
    float startY = fmodf(m_canvasOffset.y * m_canvasZoom, gridSpacing);

    for (float x = startX; x < canvasSize.x; x += gridSpacing) {
        drawList->AddLine(
            ImVec2(canvasPos.x + x, canvasPos.y),
            ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
            gridColor
        );
    }

    for (float y = startY; y < canvasSize.y; y += gridSpacing) {
        drawList->AddLine(
            ImVec2(canvasPos.x, canvasPos.y + y),
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
            gridColor
        );
    }
}
```

**Key Points:**
- Grid MUST be zoom-aware: `gridSpacing = 24.0f * m_canvasZoom`
- Grid MUST be pan-aware: Use `fmodf()` to offset grid start
- Use imnodes color scheme for consistency: `#26262FFF` (bg), `#3F3F47FF` (lines)
- Check `m_gridVisible` flag before rendering

---

### Pattern 3: Canvas::SetGridVisible() - The State Management Method

Canvas must have these 3 methods:

```cpp
void YourCanvas::SetGridVisible(bool visible)
{
    m_gridVisible = visible;
    // Grid now renders/hides on next Render() call
}

void YourCanvas::SetMinimapVisible(bool visible)
{
    if (m_minimapRenderer)
    {
        m_minimapRenderer->SetVisible(visible);
    }
}

void YourCanvas::ResetPanZoom()
{
    m_canvasOffset = ImVec2(0.0f, 0.0f);
    m_canvasZoom = 1.0f;
    // Pan/zoom reset on next Render() call
}
```

---

### Pattern 4: Constructor Initialization - Required Members

Every graph renderer MUST initialize these members:

```cpp
class YourGraphRenderer : public IGraphRenderer
{
private:
    // ====== Framework Components (REQUIRED) ======
    std::unique_ptr<CanvasFramework> m_framework;          // Unified toolbar + modals
    std::unique_ptr<YourCanvas> m_canvas;                  // Canvas rendering
    std::unique_ptr<YourPropertyEditorPanel> m_propertyEditor;

    // ====== Toolbar State (REQUIRED) ======
    bool m_gridVisible = true;                             // Grid checkbox state
    bool m_minimapVisible = true;                          // Minimap checkbox state
    float m_propertiesPanelWidth = 280.0f;                 // Resizable properties panel
};

YourGraphRenderer::YourGraphRenderer()
    : GraphEditorBase(),
      m_gridVisible(true),
      m_minimapVisible(true),
      m_propertiesPanelWidth(280.0f)
{
}
```

---

### Pattern 5: Load() Method - Framework Initialization

Every graph renderer's Load() method MUST do this:

```cpp
bool YourGraphRenderer::Load(const std::string& filePath)
{
    // Step 1: Create/load document
    if (!m_document)
    {
        m_document = std::make_unique<YourGraphDocument>();
    }

    bool success = m_document->Load(filePath);
    if (!success) return false;

    // Step 2: Create/initialize canvas
    if (!m_canvas)
    {
        m_canvas = std::make_unique<YourCanvas>();
        m_canvas->Initialize(m_document.get());
    }

    // Step 3: CREATE CANVASFRAMEWORK (CRITICAL - THIS IS THE MISSING PIECE)
    // This provides unified toolbar (Save/SaveAs/Browse) and modals
    if (!m_framework)
    {
        m_framework = std::make_unique<CanvasFramework>(m_document.get());
    }

    // Step 4: Initialize property editor
    if (!m_propertyEditor)
    {
        m_propertyEditor = std::make_unique<YourPropertyEditorPanel>();
        m_propertyEditor->Initialize(m_document.get());
    }

    return true;
}
```

---

### Pattern 6: Canvas member variables (REQUIRED)

Every Canvas MUST have these:

```cpp
class YourCanvas
{
private:
    // ====== Core State (REQUIRED) ======
    YourGraphDocument* m_document = nullptr;
    ImVec2 m_canvasOffset = ImVec2(0.0f, 0.0f);           // Pan offset
    float m_canvasZoom = 1.0f;                             // Zoom factor (0.1 - 3.0)

    // ====== Toolbar State (REQUIRED) ======
    bool m_gridVisible = true;                             // Reflects toolbar checkbox
    bool m_minimapVisible = true;                          // Reflects toolbar checkbox

    // ====== Minimap Renderer (REQUIRED) ======
    std::unique_ptr<CanvasMinimapRenderer> m_minimapRenderer;

public:
    // ====== Interface Methods (REQUIRED) ======
    void SetGridVisible(bool visible) { m_gridVisible = visible; }
    void SetMinimapVisible(bool visible) { /* Call m_minimapRenderer->SetVisible() */ }
    void ResetPanZoom() { m_canvasOffset = ImVec2(0.0f, 0.0f); m_canvasZoom = 1.0f; }

    // ====== Render Methods in Order (REQUIRED) ======
    void Render()
    {
        RenderGrid();           // First: background grid
        RenderConnections();    // Connections
        RenderNodes();          // Nodes
        RenderSelectionRectangle();
        RenderConnectionPreviewLine();
        RenderContextMenu();
        RenderMinimap();        // Last: overlay minimap
    }

private:
    void RenderGrid();          // Renders grid with zoom/pan awareness
    void RenderNodes();
    void RenderConnections();
    void RenderMinimap();       // Calls m_minimapRenderer->RenderCustom()
};
```

---

## Integration Checklist for New Graph Types

Use this checklist when implementing a new graph editor:

### Required Framework Components
- [ ] `std::unique_ptr<CanvasFramework> m_framework` member
- [ ] `std::unique_ptr<YourCanvas> m_canvas` member
- [ ] `std::unique_ptr<YourPropertyEditorPanel> m_propertyEditor` member
- [ ] `bool m_gridVisible` toolbar state
- [ ] `bool m_minimapVisible` toolbar state

### Toolbar Implementation
- [ ] `RenderCommonToolbar()` method with 3 buttons:
  - [ ] Framework toolbar render (Save/SaveAs/Browse)
  - [ ] Grid checkbox (calls `m_canvas->SetGridVisible()`)
  - [ ] Reset View button (calls `m_canvas->ResetPanZoom()`)
  - [ ] Minimap checkbox (calls `m_canvas->SetMinimapVisible()`)

### Canvas Implementation
- [ ] `SetGridVisible(bool)` method
- [ ] `SetMinimapVisible(bool)` method
- [ ] `ResetPanZoom()` method
- [ ] `RenderGrid()` method (zoom/pan aware, imnodes colors)
- [ ] `Render()` method with correct ordering:
  1. `RenderGrid()`
  2. `RenderConnections()`
  3. `RenderNodes()`
  4. `RenderSelectionRectangle()`
  5. `RenderConnectionPreviewLine()`
  6. `RenderContextMenu()`
  7. `RenderMinimap()`

### Load/Save Integration
- [ ] `Load()` creates `CanvasFramework` (LINE 1: CRITICAL)
- [ ] `Load()` creates Canvas and initializes it
- [ ] `Load()` creates PropertyEditorPanel
- [ ] `Save()` delegates to document

### Pan/Zoom Implementation
- [ ] `m_canvasOffset` (ImVec2) for panning
- [ ] `m_canvasZoom` (float) for zoom (clamped 0.1 - 3.0)
- [ ] `HandlePanZoomInput()` for input processing
- [ ] `CanvasToScreen()` coordinate transformation

### Minimap Integration
- [ ] `std::unique_ptr<CanvasMinimapRenderer> m_minimapRenderer` member
- [ ] Create in Canvas constructor
- [ ] Call `m_minimapRenderer->RenderCustom()` in `RenderMinimap()`

---

## Common Mistakes (What NOT to Do)

❌ **Mistake 1**: Not creating CanvasFramework in Load()
- Result: Save/SaveAs/Browse buttons don't work

❌ **Mistake 2**: Not rendering grid
- Result: Canvas is blank, no grid visible

❌ **Mistake 3**: Grid not zoom-aware
- Formula: `gridSpacing = 24.0f * m_canvasZoom`
- Result: Grid doesn't scale with zoom

❌ **Mistake 4**: Forgetting toolbar buttons (Grid, Reset View, Minimap)
- Result: Incomplete toolbar, less than PlaceholderGraphRenderer

❌ **Mistake 5**: Wrong render order (grid must be FIRST)
- Result: Grid overlaps nodes, looks broken

❌ **Mistake 6**: Not calling canvas methods from toolbar buttons
- Result: Buttons exist but don't do anything

---

## Why EntityPrefabEditorV2 Was Incomplete

**What Was Missing:**
1. ❌ `RenderCommonToolbar()` method (only basic toolbar status bar)
2. ❌ Grid checkbox button and `SetGridVisible()` method
3. ❌ Reset View button and `ResetPanZoom()` method
4. ❌ Minimap checkbox button and `SetMinimapVisible()` method
5. ❌ `RenderGrid()` implementation in canvas
6. ❌ Proper render method ordering

**Why It Happened:**
- Didn't study PlaceholderGraphRenderer first
- Assumed toolbar framework = "all done"
- No visual testing until runtime

**How It Was Fixed:**
1. ✅ Added `RenderCommonToolbar()` with all 3 buttons
2. ✅ Implemented grid rendering in canvas
3. ✅ Added state management methods
4. ✅ Connected toolbar buttons to canvas methods
5. ✅ Verified render ordering

---

## File References

**Complete Working Example**: `PlaceholderGraphRenderer.cpp` (RenderCommonToolbar lines 125-156)
**Complete Canvas Example**: `PlaceholderCanvas.cpp` (RenderGrid lines 67-111, Render lines 45-65)
**Legacy Reference**: `EntityPrefabRenderer.cpp` (V1 working implementation)

---

## Quick Test Checklist

After implementing, verify:
- [ ] Save button works
- [ ] SaveAs button works
- [ ] Browse button works
- [ ] Grid checkbox toggles grid on/off
- [ ] Grid is visible and scales with zoom
- [ ] Reset View button resets zoom/pan
- [ ] Minimap checkbox shows/hides minimap
- [ ] Minimap appears in corner (correct position)
- [ ] Pan works (middle mouse drag)
- [ ] Zoom works (scroll wheel)
- [ ] No rendering glitches or overlaps

---

## Questions?

This guide covers 100% of what's needed. If something is unclear, compare your implementation with PlaceholderGraphRenderer.cpp line by line.

