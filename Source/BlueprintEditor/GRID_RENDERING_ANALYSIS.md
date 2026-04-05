/**
 * @file GRID_RENDERING_ANALYSIS.md
 * @brief Complete analysis of imnodes grid rendering architecture
 * @details Understanding the difference between imnodes native grid and custom grid rendering
 */

# Grid Rendering Architecture: imnodes vs Custom

## imnodes Native Grid Rendering (VisualScript)

### Architecture Overview
imnodes provides a **built-in, integrated grid rendering system** that is automatically called during the editor lifecycle.

### Call Stack

```
BeginNodeEditor()
  ├── ImGui::BeginChild("scrolling_region", ...)
  │   └── Set Background Color: ImNodesCol_GridBackground
  ├── DrawGrid(editor, canvas_size)  ← GRID RENDERING HAPPENS HERE
  │   └── Draws all grid lines using editor panning offset
  └── ... (nodes, links rendering follows)

EndNodeEditor()
  └── ImGui::EndChild()
```

### Key Implementation Details (imnodes.cpp - line 1391)

```cpp
void DrawGrid(ImNodesEditorContext& editor, const ImVec2& canvas_size)
{
    // 1. Get pan offset from editor context (maintained by imnodes internally)
    const ImVec2 offset = editor.Panning;

    // 2. Get colors from style (StyleColorsDark defaults)
    ImU32 line_color = GImNodes->Style.Colors[ImNodesCol_GridLine];          // (200,200,200,40)
    ImU32 line_color_prim = GImNodes->Style.Colors[ImNodesCol_GridLinePrimary]; // (240,240,240,60)

    // 3. Check if primary lines enabled
    bool draw_primary = GImNodes->Style.Flags & ImNodesStyleFlags_GridLinesPrimary;

    // 4. Draw VERTICAL lines
    for (float x = fmodf(offset.x, GImNodes->Style.GridSpacing); x < canvas_size.x;
         x += GImNodes->Style.GridSpacing)
    {
        GImNodes->CanvasDrawList->AddLine(
            EditorSpaceToScreenSpace(ImVec2(x, 0.0f)),
            EditorSpaceToScreenSpace(ImVec2(x, canvas_size.y)),
            offset.x - x == 0.f && draw_primary ? line_color_prim : line_color
        );
    }

    // 5. Draw HORIZONTAL lines
    for (float y = fmodf(offset.y, GImNodes->Style.GridSpacing); y < canvas_size.y;
         y += GImNodes->Style.GridSpacing)
    {
        GImNodes->CanvasDrawList->AddLine(
            EditorSpaceToScreenSpace(ImVec2(0.0f, y)),
            EditorSpaceToScreenSpace(ImVec2(canvas_size.x, y)),
            offset.y - y == 0.f && draw_primary ? line_color_prim : line_color
        );
    }
}
```

### Specificities of imnodes Grid

#### 1. **Grid-Space Coordinates (NOT Screen-Space)**
- All calculations are in **editor space** (canvas coordinates)
- Uses `EditorSpaceToScreenSpace()` transformation for rendering
- Automatically handles zoom and pan through this function
- **Key Difference**: No manual zoom/pan calculation needed

#### 2. **Single Grid Spacing**
- Only uses `GridSpacing` (24px default)
- NO minor grid lines by default
- All lines use same spacing formula

#### 3. **Panning Integration**
- Pan offset stored in `editor.Panning` (maintained by imnodes)
- Grid automatically adapts to panning
- `fmodf(offset.x, GridSpacing)` creates seamless grid wrapping

#### 4. **Primary Line Highlighting**
- Checks if `(offset.x - x == 0.f)` to highlight primary grid line
- Uses `line_color_prim` when true, `line_color` otherwise
- Creates visual alignment guide at grid origin

#### 5. **Background Handling**
- Background color set via `ImGui::PushStyleColor(ImGuiCol_ChildBg, ...)`
- Happens in `BeginNodeEditor()` BEFORE grid drawing
- Is the ImGui ChildWindow background

#### 6. **Flags Control**
- `ImNodesStyleFlags_GridLines`: Enable/disable all grid
- `ImNodesStyleFlags_GridLinesPrimary`: Enable/disable primary line highlighting

### Canvas Setup (BeginNodeEditor)

```cpp
ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
ImGui::BeginChild(
    "scrolling_region",
    ImVec2(0.f, 0.f),
    true,
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollWithMouse
);
```

**What this does:**
- Minimal padding (1px frame, 0px window)
- Child window background = grid background color (40,40,50,200)
- No scrollbars (manual panning with ImNodes instead)
- No move (locked to editor)

---

## Custom Grid Rendering (PrefabCanvas)

### Architecture Overview
Custom implementation must replicate imnodes functionality but manually.

### Call Stack (Current)

```
PrefabCanvas::Render()
  ├── RenderGrid()
  │   ├── Set canvas background manually
  │   ├── Calculate grid offset from pan (m_canvasOffset)
  │   └── Draw all grid lines with zoom/pan math
  └── RenderNodes()
```

### Current CanvasGridRenderer Implementation

```cpp
void CanvasGridRenderer::RenderGrid(ImDrawList* drawList, const GridConfig& config)
{
    // 1. Background (manual - imnodes does this via ImGui::BeginChild)
    ImU32 backgroundColor = ImGui::GetColorU32(config.backgroundColor);
    drawList->AddRectFilled(config.canvasPos, canvasEnd, backgroundColor);

    // 2. Grid calculations with ZOOM awareness (imnodes doesn't expose zoom)
    float scaledGridSpacing = config.majorSpacing * config.zoom;  // NEW
    float scaledMinorSpacing = scaledGridSpacing / config.minorDivisor;  // NEW

    // 3. Pan offset calculation
    float gridStartX = config.canvasPos.x + config.offsetX * config.zoom;
    float gridStartY = config.canvasPos.y + config.offsetY * config.zoom;

    float gridOffsetX = fmod(gridStartX, scaledGridSpacing);  // Similar to imnodes
    float gridOffsetY = fmod(gridStartY, scaledGridSpacing);

    // 4. Draw minor lines (imnodes doesn't support this)
    for (float x = ...; x < canvasEnd.x; x += scaledMinorSpacing) { ... }

    // 5. Draw major lines
    for (float x = ...; x < canvasEnd.x; x += scaledGridSpacing) { ... }
}
```

---

## Key Differences Summary

| Aspect | imnodes | PrefabCanvas |
|--------|---------|--------------|
| **Background** | ImGui ChildWindow (automatic) | Manual AddRectFilled |
| **Coordinate System** | Editor space + transform | Screen space + math |
| **Zoom Support** | Not exposed (fixed 1.0) | Manual zoom scaling |
| **Pan Tracking** | Internal editor.Panning | Manual m_canvasOffset |
| **Grid Lines** | Single spacing only | Major + minor support |
| **Line Weights** | Fixed (1 color per line) | Configurable thickness |
| **Primary Line** | Highlights at origin (offset.x == 0) | All major lines same |
| **Flags** | GridLines, GridLinesPrimary | Custom GridStylePreset |

---

## Why PrefabCanvas Needs Custom Rendering

### Reason 1: No Zoom in imnodes
- imnodes doesn't expose zoom level
- We added zoom support (0.1x - 3.0x)
- Grid must scale with zoom: `spacing * zoom`

### Reason 2: No Zoom Pan Integration
- imnodes panning via internal offset
- We added manual pan: `m_canvasOffset`
- Must recalculate grid position each frame

### Reason 3: Canvas Not Using imnodes
- PrefabCanvas is custom ImGui canvas
- Not using ImNodes::BeginNodeEditor/EndNodeEditor
- Must replicate all canvas rendering

### Reason 4: Minor Grid Lines
- imnodes only supports single grid spacing
- We wanted minor grid for better UX
- Added `minorDivisor` support

---

## How to Align PrefabCanvas to imnodes

To make PrefabCanvas grid match imnodes perfectly:

```cpp
// Use GetStylePreset(Style_VisualScript) which includes:
config.majorSpacing = 24.0f;        // imnodes default
config.backgroundColor = (40,40,50,200);  // StyleColorsDark default
config.majorLineColor = (240,240,240,60); // GridLinePrimary
config.minorLineColor = (200,200,200,40); // GridLine
config.minorDivisor = 1.0f;        // Disable minor lines to match imnodes
config.majorLineThickness = 1.0f;  // No thickness override

// Transform calculations mirror imnodes:
float gridOffset = fmod(canvasPos.x + offset.x * zoom, scaledSpacing);
// This is equivalent to imnodes: fmod(offset.x, GridSpacing)
// But with zoom: * zoom
```

---

## Conclusion

**imnodes native grid** is simpler because:
1. No zoom (fixed scale)
2. Pan handled internally
3. Automatic background via ImGui
4. Single grid spacing
5. Optimized for node editing (no custom transformations)

**PrefabCanvas grid** is more flexible:
1. Full zoom/pan control
2. Minor grid lines
3. Configurable spacing presets
4. More visual customization
5. Suitable for component layout

Both approach the same problem differently based on their constraints.
