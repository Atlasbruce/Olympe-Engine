# C++ Class Reference

Key classes in Olympe Engine.

## Entity Prefab Editor

### EntityPrefabGraphDocument
**File**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.h`

Data model for entity prefab graphs.

| Method | Description |
|--------|-------------|
| `CreateComponentNode(type)` | Add new node, returns `NodeId` |
| `RemoveNode(id)` | Remove node and its connections |
| `ConnectNodes(src, dst)` | Create connection |
| `DisconnectNodes(src, dst)` | Remove connection |
| `LoadFromFile(path)` | Deserialize from JSON |
| `SaveToFile(path)` | Serialize to JSON |
| `IsDirty()` | Returns true if unsaved changes exist |

### PrefabCanvas
**File**: `Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.h`

ImGui canvas container with pan, zoom, and input handling.

| Method | Description |
|--------|-------------|
| `Render()` | Draw the canvas |
| `ScreenToCanvas(x, y)` | Convert screen to canvas coords |
| `CanvasToScreen(x, y)` | Convert canvas to screen coords |
| `PanCanvas(dx, dy)` | Translate viewport |
| `ZoomCanvas(delta, cx, cy)` | Scale with pivot |

### ComponentPalettePanel
**File**: `Source/BlueprintEditor/EntityPrefabEditor/ComponentPalettePanel.h`

Searchable component list panel.

| Method | Description |
|--------|-------------|
| `Initialize()` | Load components (JSON or fallback) |
| `Render()` | Draw the palette |
| `GetSelectedComponent()` | Get currently selected component |
| `LoadComponentsFromJSON(path)` | Load from file |

## Canvas Utilities

### CanvasMinimapRenderer
**File**: `Source/BlueprintEditor/Utilities/CanvasMinimapRenderer.h`

Centralized minimap rendering for all canvas types.

| Method | Description |
|--------|-------------|
| `Render(nodes, viewport)` | Draw minimap |
| `SetPosition(pos)` | Set corner placement |
| `SetSizeFraction(f)` | Set minimap size [0..1] |

### CanvasGridRenderer
**File**: `Source/BlueprintEditor/Utilities/CanvasGridRenderer.h`

Standardized grid rendering.

| Method | Description |
|--------|-------------|
| `Render(origin, size, offset, zoom)` | Draw grid overlay |
| `SetGridSize(f)` | Set minor grid cell size |
| `SetMajorGridSize(f)` | Set major grid cell size |

## Blackboard System

### BlackboardSystem
**File**: `Source/NodeGraphCore/BlackboardSystem.h`

Per-graph key-value store.

| Method | Description |
|--------|-------------|
| `SetInt/Float/Bool/String(key, val)` | Write value |
| `GetInt/Float/Bool/String(key)` | Read value |
| `HasKey(key)` | Check existence |
| `Serialize()` | Return JSON |
| `Deserialize(json)` | Load from JSON |

### GlobalBlackboard
**File**: `Source/NodeGraphCore/GlobalBlackboard.h`

Singleton engine-wide state store.

| Method | Description |
|--------|-------------|
| `GetInstance()` | Access singleton |
| `SetFloat(key, val)` | Write global float |
| `GetFloat(key)` | Read global float |
