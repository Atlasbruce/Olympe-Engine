# Canvas Grid Standardization (Phase 5)

Phase 5 standardized the grid rendering across all graph editors using a shared `CanvasGridRenderer`.

## Before Phase 5

Each editor had its own grid implementation:
- BT Editor: custom C++ grid code
- Visual Script Editor: different custom grid
- (Inconsistent appearance and behavior)

## After Phase 5

All editors use `CanvasGridRenderer`:

```cpp
#include "Utilities/CanvasGridRenderer.h"

CanvasGridRenderer grid;
grid.SetGridSize(25.0f);       // Minor grid
grid.SetMajorGridSize(100.0f); // Major grid
grid.SetColor(ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
grid.Render(canvasOrigin, canvasSize, offset, zoom);
```

## Grid Properties

| Property | Default | Description |
|----------|---------|-------------|
| minorGridSize | 25.0 | Minor grid cell size |
| majorGridSize | 100.0 | Major grid cell size |
| minorColor | 0.2,0.2,0.2,0.3 | Minor line color (RGBA) |
| majorColor | 0.3,0.3,0.3,0.5 | Major line color (RGBA) |
| showMinor | true | Show minor grid |
| showMajor | true | Show major grid |

## Scaling Behavior

The grid scales and pans with the canvas:
- Zoom in: grid lines become farther apart
- Zoom out: grid adapts (minor lines disappear at zoom < 0.3)
- Pan: grid moves with the canvas offset

## ICanvasEditor Integration

All editors implementing `ICanvasEditor` get grid rendering automatically via the interface:

```cpp
class ICanvasEditor {
public:
    virtual void RenderGrid() = 0;
    // ...
};
```
