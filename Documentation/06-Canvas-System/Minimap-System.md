# Minimap System (Phase 37)

The centralized `CanvasMinimapRenderer` provides bird's-eye navigation for all graph editors.

## Architecture

The minimap is a **shared utility** in `Source/BlueprintEditor/Utilities/CanvasMinimapRenderer.h`.

## Integration Modes

### Mode 1: ImNodes Native (BT, Visual Script)

```cpp
ImNodes::MiniMap(0.15f, ImNodesMiniMapLocation_BottomRight);
```

### Mode 2: Custom Renderer (Entity Prefab)

```cpp
CanvasMinimapRenderer minimap;
minimap.SetPosition(MinimapPosition::BottomRight);
minimap.SetSizeFraction(0.15f);

std::vector<MinimapNodeData> nodeData;
for (auto& node : document.GetAllNodes()) {
    nodeData.push_back({node.nodeId, normalizedX, normalizedY, normW, normH});
}

MinimapViewportData vp = {vpNormX, vpNormY, vpNormW, vpNormH};
minimap.Render(nodeData, vp);
```

## Coordinate Normalization

```cpp
// Compute bounds
float minX = ..., maxX = ..., minY = ..., maxY = ...;
float rangeX = maxX - minX;
float rangeY = maxY - minY;

// Normalize each node
float normX = (node.x - minX) / rangeX;
float normY = (node.y - minY) / rangeY;
```

## Click-to-Navigate

Clicking on the minimap pans the canvas to that region:

```cpp
if (ImGui::IsItemClicked()) {
    ImVec2 click = ImGui::GetMousePos();
    // Convert click → canvas offset
    PanToCanvasPosition(click);
}
```

## Visual Design

- **Background**: Semi-transparent dark rectangle
- **Nodes**: Small colored rectangles (blue = selected, white = unselected)
- **Viewport**: Highlighted rectangle showing current view
- **Connections**: Not rendered (performance)
