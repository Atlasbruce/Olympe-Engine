# Coordinate Systems

Three coordinate spaces are used in Olympe Engine canvas editors.

## Space Definitions

### 1. Screen Space
- **Origin**: Top-left corner of the OS window
- **Units**: Pixels
- **Source**: `ImGui::GetMousePos()`, `ImGui::GetCursorScreenPos()`

### 2. Canvas Space
- **Origin**: Virtual canvas, moves with pan/zoom
- **Units**: Logical units (1 unit ≈ 1 pixel at zoom=1.0)
- **Used for**: Node positions stored in JSON

### 3. Normalized Space [0..1]
- **Origin**: Bottom-left of bounding box
- **Range**: [0.0, 1.0] in both axes
- **Used for**: Minimap coordinates

## Transformations

```
Screen → Canvas:
  canvasX = (screenX - canvasOriginX - offsetX) / zoom
  canvasY = (screenY - canvasOriginY - offsetY) / zoom

Canvas → Screen:
  screenX = canvasX * zoom + offsetX + canvasOriginX
  screenY = canvasY * zoom + offsetY + canvasOriginY

Canvas → Normalized:
  normX = (canvasX - minX) / (maxX - minX)
  normY = (canvasY - minY) / (maxY - minY)
```

## Critical Bug (Fixed Phase 29)

The old ScreenToCanvas formula was wrong:

```cpp
// WRONG (Phase 28 and earlier):
canvas = (screen - canvasOrigin - offset * zoom) / zoom

// CORRECT (Phase 29+):
canvas = (screen - canvasOrigin - offset) / zoom
```

The wrong formula caused node selection to be offset at non-1.0 zoom levels.

## Canvas Origin

```cpp
// Captured BEFORE BeginChild():
ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
ImGui::BeginChild("Canvas", size, false, flags);
// ...
```
