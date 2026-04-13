---
id: coordinate-systems
title: Coordinate Systems
sidebar_label: Coordinate Systems
sidebar_position: 3
---

# Coordinate Systems

Olympe Engine uses three coordinate systems in canvas editors.

## Coordinate Spaces

### Screen Space
- Origin: top-left corner of the window
- Units: pixels
- Provided by `ImGui::GetCursorScreenPos()`

### Canvas Space
- Origin: virtual canvas origin (panned, zoomed)
- Units: logical canvas units
- Conversion: `ScreenToCanvas(screenPos)`

### Normalized Space
- Range: [0..1] in both axes
- Used for: minimap, viewport fractions
- Conversion: `(canvasPos - bounds.min) / bounds.size`

## Transformation Formulas

```
// Screen → Canvas
canvasX = (screenX - canvasOriginX - offsetX) / zoom
canvasY = (screenY - canvasOriginY - offsetY) / zoom

// Canvas → Screen
screenX = canvasX * zoom + offsetX + canvasOriginX
screenY = canvasY * zoom + offsetY + canvasOriginY

// Canvas → Normalized
normX = (canvasX - minX) / (maxX - minX)
normY = (canvasY - minY) / (maxY - minY)
```

## ImGui Canvas Origin

```cpp
// Get the canvas origin (top-left of BeginChild window)
ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
```

:::note
Always capture `canvasOrigin` **before** calling `ImGui::BeginChild()` or drawing content, as it changes during rendering.
:::

## Zoom Clamping

All canvas editors clamp zoom between `0.1` and `3.0`:

```cpp
m_zoom = std::clamp(m_zoom + delta * 0.1f, 0.1f, 3.0f);
```

## Related

- [Canvas Overview](canvas-overview)
- [Minimap System](minimap-system)
