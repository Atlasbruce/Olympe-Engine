# Entity Prefab System

The Entity Prefab System provides a visual tool for authoring reusable entity templates as node graphs.

## Architecture

```
EntityPrefabRenderer (IGraphRenderer)
    └── PrefabCanvas
            ├── EntityPrefabGraphDocument  ← data model
            │       ├── ComponentNode[]
            │       └── Connections[]
            └── ComponentNodeRenderer      ← drawing
    └── ComponentPalettePanel              ← component list
```

## Key Classes

| Class | Responsibility |
|-------|---------------|
| `EntityPrefabRenderer` | IGraphRenderer adapter for tab integration |
| `PrefabCanvas` | ImGui canvas container, input handling |
| `EntityPrefabGraphDocument` | Data model (nodes, connections, dirty flag) |
| `ComponentNode` | Individual component data (position, params) |
| `ComponentNodeRenderer` | Draws nodes and Bezier connections |
| `ComponentPalettePanel` | Searchable component list |
| `PrefabLoader` | JSON deserialization |

## Phase Timeline

| Phase | Feature Added |
|-------|--------------|
| 27 | Basic rendering (IGraphRenderer → PrefabCanvas → ComponentNodeRenderer) |
| 28 | Pan, zoom, node dragging, multi-select, deletion |
| 29 | Drag-drop from palette, New Prefab menu (Ctrl+Alt+N) |
| 29b | Components loaded dynamically from JSON |
| 30 | Connection creation with Bezier curves |
| 31 | Rectangle selection, Properties inspector panel |

## JSON Schema (v4)

```json
{
  "version": 4,
  "nodes": [
    {
      "nodeId": 1,
      "componentType": "Transform",
      "componentName": "Position",
      "position": [100.0, 150.0],
      "parameters": {
        "x": 0.0,
        "y": 0.0,
        "rotation": 0.0
      }
    }
  ],
  "connections": [
    { "sourceId": 1, "targetId": 2 }
  ],
  "canvasState": {
    "zoom": 1.0,
    "offset": [0.0, 0.0]
  }
}
```

## Coordinate Transformation

Nodes are stored in **canvas space** and transformed to **screen space** for rendering:

```
screen = canvas * zoom + offset + canvasOrigin
```

The critical formula for ScreenToCanvas (fixed in Phase 29):
```cpp
// CORRECT:
canvas = (screen - canvasOrigin - offset) / zoom
// WRONG (old bug):
// canvas = (screen - canvasOrigin - offset * zoom) / zoom
```
