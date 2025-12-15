# RenderBackendSystem Usage Guide

## Overview

The `RenderBackendSystem` is an ECS-based rendering backend that provides native support for multi-pass rendering, split-screen, and multi-window configurations. It follows a data-driven approach where render targets, viewports, and cameras are all ECS entities, enabling dynamic and flexible rendering architectures.

## Key Components

### RenderTarget_data

Represents a render target (window + renderer pair).

```cpp
struct RenderTarget_data
{
    SDL_Window* window;      // Window handle (null for offscreen targets)
    SDL_Renderer* renderer;  // Renderer handle
    RenderTargetType type;   // Primary, Secondary, or Offscreen
    int index;               // Index for ordering/identification
    bool isActive;           // Whether this target is active
    int width, height;       // Target dimensions
};
```

### Viewport_data

Represents a viewport (portion of a render target).

```cpp
struct Viewport_data
{
    SDL_FRect rect;                // Viewport rectangle
    short playerIndex;             // Which player this viewport belongs to
    EntityID renderTargetEntity;   // RenderTarget to render to
    EntityID cameraEntity;         // Camera for this viewport
    bool isActive;                 // Whether viewport is active
    int renderOrder;               // Order in which viewports are rendered
};
```

### Camera_data (Enhanced)

Enhanced to support viewport-based rendering.

```cpp
struct Camera_data
{
    float zoomLevel;           // Zoom level
    float rotation;            // Rotation angle in radians
    EntityID targetEntity;     // Entity the camera follows
    EntityID viewportEntity;   // Viewport this camera renders to
    Vector offset;             // Camera offset from target
    SDL_Rect bounds;           // Camera movement bounds
};
```

## Basic Usage

### 1. Accessing the RenderBackendSystem

```cpp
// Get the system from World
RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
```

### 2. Setting Up Split-Screen Mode

The simplest way to enable split-screen for multiple players:

```cpp
// Setup split-screen for 4 players
std::vector<EntityID> viewports = renderBackend->SetupSplitScreen(4);

// Each viewport entity corresponds to a player (0-3)
// Viewports are automatically laid out in an optimal grid
```

**Supported Layouts:**
- 1 player: Full screen
- 2 players: Vertical split (2 columns)
- 3 players: Vertical split (3 columns)
- 4 players: 2x2 grid
- 5-6 players: 3x2 grid
- 7-8 players: 4x2 grid

### 3. Setting Up Multi-Window Mode

Create separate windows for each player:

```cpp
// Create 2 separate windows (800x600 each)
auto windowsAndViewports = renderBackend->SetupMultiWindow(2, 800, 600);

// Returns vector of (RenderTarget, Viewport) pairs
for (const auto& [rtEntity, vpEntity] : windowsAndViewports)
{
    // Each pair represents one player's window + viewport
    // Can be used to customize individual windows
}
```

### 4. Switching Between Modes Dynamically

```cpp
// Switch from split-screen to multi-window without restart
renderBackend->SwitchToMultiWindow(2, 1024, 768);

// Or switch back to split-screen
renderBackend->SwitchToSplitScreen(2);
```

### 5. Manual Configuration (Advanced)

For fine-grained control:

```cpp
// Get the primary render target
auto renderTargets = renderBackend->GetActiveRenderTargets();
EntityID primaryTarget = renderTargets[0];

// Create custom viewport
SDL_FRect customRect = {100.f, 100.f, 400.f, 300.f};
EntityID viewport = renderBackend->CreateViewport(
    0,              // Player index
    customRect,     // Viewport rectangle
    primaryTarget   // Render target to use
);

// Create custom camera for this viewport
EntityID camera = World::Get().CreateEntity();
Camera_data& cam = World::Get().AddComponent<Camera_data>(camera);
cam.viewportEntity = viewport;
cam.zoomLevel = 2.0f;

// Link camera to viewport
Viewport_data& vp = World::Get().GetComponent<Viewport_data>(viewport);
vp.cameraEntity = camera;
```

### 6. Creating Secondary Windows Manually

```cpp
// Create a secondary window for debugging or special purposes
EntityID secondaryRT = renderBackend->CreateSecondaryRenderTarget(
    "Debug View",  // Window title
    640,           // Width
    480,           // Height
    1              // Index
);

// Create viewport for entire secondary window
RenderTarget_data& rt = World::Get().GetComponent<RenderTarget_data>(secondaryRT);
SDL_FRect fullRect = {0.f, 0.f, (float)rt.width, (float)rt.height};
EntityID debugViewport = renderBackend->CreateViewport(0, fullRect, secondaryRT);
```

## Dynamic Player Management

### Adding a Player

```cpp
// Get current viewport count
auto currentViewports = renderBackend->GetActiveRenderTargets();
int currentPlayers = currentViewports.size();

// Add one more player to split-screen
renderBackend->SetupSplitScreen(currentPlayers + 1);
```

### Removing a Player

```cpp
// Reduce by one player
int currentPlayers = /* current count */;
if (currentPlayers > 1)
{
    renderBackend->SetupSplitScreen(currentPlayers - 1);
}
```

### Clean Slate

```cpp
// Remove all viewports and secondary windows
renderBackend->ClearAllViewportsAndTargets();

// Primary render target is preserved
// Can now setup new configuration
```

## Integration with Existing Systems

### Rendering Systems

The `RenderBackendSystem` automatically manages the render loop. Your existing rendering systems (like `RenderingSystem`) don't need modification - they just render to the currently active viewport.

### Camera System

When a viewport has a linked camera entity, the `RenderBackendSystem` automatically applies the camera transform before rendering that viewport.

```cpp
// In your game code, update camera to follow player
Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
cam.targetEntity = playerEntity;  // Camera follows player

// RenderBackendSystem handles the rest automatically
```

## Performance Considerations

1. **Split-Screen**: Single window, minimal overhead. Best for local multiplayer.
2. **Multi-Window**: Multiple windows/renderers. More GPU memory but better for different monitor setups.
3. **Viewport Count**: Each viewport requires a full render pass. Keep count reasonable (1-8 recommended).

## Example: 4-Player Split-Screen Game

```cpp
// In your game initialization
void InitializeMultiplayer()
{
    RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
    
    // Setup 4-player split-screen
    auto viewports = renderBackend->SetupSplitScreen(4);
    
    // Create player entities and cameras
    for (int i = 0; i < 4; ++i)
    {
        // Create player entity
        EntityID player = World::Get().CreateEntity();
        World::Get().AddComponent<Position_data>(player);
        World::Get().AddComponent<PlayerBinding_data>(player).playerIndex = i;
        
        // Create camera for this player
        EntityID camera = World::Get().CreateEntity();
        Camera_data& cam = World::Get().AddComponent<Camera_data>(camera);
        cam.targetEntity = player;
        cam.viewportEntity = viewports[i];
        cam.zoomLevel = 1.0f;
        
        // Link camera to viewport
        Viewport_data& vp = World::Get().GetComponent<Viewport_data>(viewports[i]);
        vp.cameraEntity = camera;
    }
}
```

## Example: Switching Modes at Runtime

```cpp
// Toggle between split-screen and multi-window
void ToggleDisplayMode(bool useMultiWindow, int numPlayers)
{
    RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
    
    if (useMultiWindow)
    {
        // Use 800x600 windows for each player
        renderBackend->SwitchToMultiWindow(numPlayers, 800, 600);
    }
    else
    {
        renderBackend->SwitchToSplitScreen(numPlayers);
    }
    
    // Game continues seamlessly - no restart required
    // All entity data is preserved
}
```

## Architecture Benefits

1. **Data-Driven**: All rendering configuration is stored in ECS entities
2. **No Special Code**: Non-rendering systems don't need to know about viewports
3. **Dynamic**: Add/remove players without code changes
4. **Flexible**: Switch between split-screen and multi-window at runtime
5. **Testable**: Easy to setup different configurations for testing
6. **Serializable**: Viewport/camera configurations can be saved/loaded

## Backward Compatibility

The system is designed to work alongside the legacy `ViewportManager`. If no RenderBackend entities are created, the engine falls back to the old rendering path automatically.

```cpp
// In OlympeEngine.cpp render loop:
RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();

if (renderBackend && !renderBackend->GetActiveRenderTargets().empty())
{
    // New ECS-based rendering
    renderBackend->Render();
}
else
{
    // Legacy rendering path
    // ... existing code ...
}
```

## Future Enhancements

Potential extensions to the system:

1. **Render-to-Texture**: Support offscreen render targets for mirrors, cameras, etc.
2. **Post-Processing**: Per-viewport effects
3. **VR Support**: Stereo rendering with separate viewports per eye
4. **Picture-in-Picture**: Nested viewports for mini-maps
5. **Dynamic Resolution**: Per-viewport resolution scaling

---

**Version**: 1.0  
**Date**: December 2025  
**Author**: Olympe Engine Team
