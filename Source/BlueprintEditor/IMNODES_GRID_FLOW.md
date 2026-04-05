/**
 * @file IMNODES_GRID_FLOW.md
 * @brief Visual flowchart of imnodes grid rendering
 */

# imnodes Grid Rendering Flow Diagram

## Complete Rendering Cycle

```
┌─────────────────────────────────────────────────────────────┐
│           VisualScriptEditorPanel::RenderCanvas()          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│        ImNodes::BeginNodeEditor()  [imnodes.cpp:2298]       │
│                                                              │
│  1. Reset state from previous frame                         │
│  2. Set up ImGui styling:                                   │
│     - PushStyleVar(FramePadding, {1, 1})                    │
│     - PushStyleVar(WindowPadding, {0, 0})                   │
│     - PushStyleColor(ChildBg, GridBackground)              │
│       └─ GridBackground = (40, 40, 50, 200)                │
│                                                              │
│  3. ImGui::BeginChild("scrolling_region", ...)              │
│     └─ Flags: NoScrollbar | NoMove | NoScrollWithMouse     │
│                                                              │
│  4. Get canvas draw list                                    │
│     └─ GImNodes->CanvasDrawList = GetWindowDrawList()       │
│                                                              │
│  5. GET CANVAS SIZE                                         │
│     └─ canvas_size = GetWindowSize()                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────┐
        │   Check Grid Rendering Flag     │
        │   if (Flags & GridLines)?       │
        └─────────────────────────────────┘
                     │         │
            YES ─────┘         └───── NO
             │                        │
             ▼                        ▼
    ┌──────────────────┐      [Skip grid rendering]
    │  DrawGrid()      │
    │  [line 1391]     │
    └──────────────────┘
             │
             ▼
    ┌─────────────────────────────────────┐
    │  1. Get pan offset                  │
    │     offset = editor.Panning         │
    │     (maintained by imnodes)         │
    │                                      │
    │  2. Get grid colors from style      │
    │     line_color = GridLine           │
    │     line_color_prim = GridLinePrimary
    │                                      │
    │  3. Get grid spacing               │
    │     spacing = GridSpacing (24px)   │
    └─────────────────────────────────────┘
             │
             ▼
    ┌─────────────────────────────────────┐
    │  VERTICAL LINES LOOP                │
    │                                      │
    │  for (x = fmod(offset.x, spacing)   │
    │       x < canvas_width              │
    │       x += spacing)                 │
    │  {                                   │
    │    // Determine if primary line     │
    │    bool isPrimary =                 │
    │      (offset.x - x == 0) &&         │
    │      (draw_primary flag)            │
    │                                      │
    │    color = isPrimary ?              │
    │      line_color_prim :              │
    │      line_color                     │
    │                                      │
    │    // Transform: Editor → Screen    │
    │    p1 = EditorSpaceToScreenSpace(   │
    │           {x, 0})                   │
    │    p2 = EditorSpaceToScreenSpace(   │
    │           {x, canvas_height})       │
    │                                      │
    │    CanvasDrawList->AddLine(p1, p2,  │
    │                            color)   │
    │  }                                   │
    └─────────────────────────────────────┘
             │
             ▼
    ┌─────────────────────────────────────┐
    │  HORIZONTAL LINES LOOP              │
    │  (Same as vertical but Y-axis)      │
    │                                      │
    │  for (y = fmod(offset.y, spacing)   │
    │       y < canvas_height             │
    │       y += spacing)                 │
    │  { ... AddLine(top, bottom, color) }│
    └─────────────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────────────────────────┐
│        [Grid rendering complete]                            │
│                                                              │
│  Render nodes, links, etc.                                 │
│  (All use same canvas context)                             │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│        ImNodes::EndNodeEditor()  [imnodes.cpp:2374]        │
│                                                              │
│  1. ImGui::EndChild()  (close scrolling_region)            │
│  2. Pop styles                                              │
│  3. Finalize node/link interactions                        │
└─────────────────────────────────────────────────────────────┘
```

---

## Grid Line Drawing Detail

### Primary Line Detection

```cpp
// This creates an alignment guide at grid origin
bool isPrimary = (offset.x - x == 0.f) && draw_primary;

// Example:
if offset.x = 24.0
   spacing = 24.0
   then x = 0.0 is primary ✓
   then x = 24.0 is NOT primary
   then x = 48.0 is NOT primary

// Visual result:
[BRIGHT] ──── [dim] ──── [dim] ──── [dim] ──── [BRIGHT]
  x=0      x=24     x=48     x=72     x=96
```

### Coordinate Transformation

```cpp
EditorSpaceToScreenSpace(editor_pos)
    ↓
  Apply offset:     editor_pos + editor.Panning
    ↓
  Apply zoom:       * GImNodes->CurrentZoom (usually 1.0)
    ↓
  Convert to screen coords
    ↓
  Result: Screen position for rendering
```

---

## Comparison: imnodes vs PrefabCanvas Grid

### imnodes Grid

```
INPUT: editor.Panning = {10, 20}
       GridSpacing = 24

LOOP:
  x = fmod(10, 24) = 10

  Draw line at x=10 (aligned with panning)
  x += 24 → x = 34

  Draw line at x=34
  x += 24 → x = 58

  etc...
```

### PrefabCanvas Grid

```
INPUT: m_canvasOffset = {10, 20}
       zoom = 1.5
       majorSpacing = 24

CALCULATION:
  scaledSpacing = 24 * 1.5 = 36
  gridStartX = canvasPos.x + 10 * 1.5
  gridOffsetX = fmod(gridStartX, 36)

LOOP:
  x = canvasPos.x + gridOffsetX - 36

  while (x < canvasEnd.x):
    Draw line at x
    x += 36
```

---

## Key Insight: Why fmod() Works

```
fmod(offset, spacing) gives you the fractional offset

Example:
offset = 37, spacing = 24
fmod(37, 24) = 13

This means:
- Grid starts at 13 pixels from origin
- Next line at 13 + 24 = 37
- Next line at 37 + 24 = 61
- etc...

Result: Grid appears to "slide" as pan changes
        but always aligns to multiples of spacing
```

---

## Flag System

```cpp
// BeginNodeEditor sets these:
Flags = ImNodesStyleFlags_NodeOutline |
        ImNodesStyleFlags_GridLines;  // Enables grid rendering

if (Flags & ImNodesStyleFlags_GridLines) {
    DrawGrid(...);  // This check allows disabling grid
}

if (Flags & ImNodesStyleFlags_GridLinesPrimary) {
    // Highlight primary line (at origin)
}
```

---

## Color System

```cpp
StyleColorsDark():  // Applied to all nodes
  GridBackground = IM_COL32(40, 40, 50, 200)     // Dark blue
  GridLine = IM_COL32(200, 200, 200, 40)         // Light gray, subtle
  GridLinePrimary = IM_COL32(240, 240, 240, 60)  // Light gray, brighter

Visual:
┌─────────────────────────────────┐
│  Background: (40,40,50)         │
│                                 │
│  [subtle line] ── [subtle line] │  ← GridLine colors
│                                 │
│  [bright line]                  │  ← GridLinePrimary (at origin)
│                                 │
└─────────────────────────────────┘
```

---

## Pan Behavior

```
Panning update happens OUTSIDE grid rendering:
1. User middle-clicks and drags
2. imnodes updates editor.Panning = {new_x, new_y}
3. Next frame: DrawGrid() uses new panning offset
4. Grid appears to slide/translate
```

---

## Summary

**imnodes DrawGrid = Simple + Elegant**

```cpp
for each_pixel_x {
    calc starting_position using fmod(pan, spacing)
    draw line at starting_position
    next line = starting_position + spacing
}
```

**That's it!** The simplicity comes from:
- Fixed coordinate system (editor space)
- Internal pan tracking
- No zoom support needed
- No background drawing needed (ImGui handles it)
