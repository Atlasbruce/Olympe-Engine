# Camera and Viewport System - ECS Architecture

## Overview

The camera and viewport system has been migrated to a full ECS (Entity Component System) architecture. Each player entity now has its own camera and viewport managed through ECS components, eliminating the need for global state.

## Components

### Camera_data

Stores all camera runtime parameters:

```cpp
struct Camera_data
{
    Vector position;        // Camera position in world space
    Vector offset;          // Offset from target (or screen center)
    float zoomLevel;        // Zoom level (default: 1.0)
    float rotation;         // Rotation angle in radians
    SDL_Rect bounds;        // Camera bounds to limit movement
    EntityID targetEntity;  // Entity the camera follows
    bool followTarget;      // Enable camera following
    float followSpeed;      // Smooth follow speed (0-1, higher = faster)
    
    enum class Type { Type_2D, Type_2_5D, Type_Isometric } type;
    enum class Mode { 
        Standard_Fixed,   // Fixed position on main renderer
        Standard_Follow,  // Follows entity on main renderer
        Viewport_Fixed,   // Fixed position in viewport
        Viewport_Follow   // Follows entity in viewport
    } mode;
};
```

### Viewport_data

Stores viewport rectangle and rendering information:

```cpp
struct Viewport_data
{
    SDL_FRect viewRect;    // Viewport rectangle on screen
    int viewportIndex;     // Index of this viewport (0-7)
    bool enabled;          // Whether this viewport is active
};
```

## Systems

### CameraSystem

Processes all entities with `Camera_data` components:

- Updates camera positions when following targets
- Applies smooth camera movement using `vBlend()`
- Enforces camera bounds
- Runs during the ECS Process phase

**Key features:**
- Automatic smooth following when `followTarget` is enabled
- Frame-rate independent movement using `vBlend()`
- Automatic cleanup when target entity is destroyed

### RenderingSystem

Renders entities with visual components:

- Accepts an active camera entity via `SetActiveCameraEntity()`
- Uses the camera's position to offset rendered entities
- Falls back to legacy `CameraManager` if no ECS camera is set

## Player Entity Setup

Each player entity created through `VideoGame::AddPlayerEntity()` receives:

1. **Camera_data** - Configured to follow the player entity itself
2. **Viewport_data** - With computed viewport rectangle

Example initialization:
```cpp
Camera_data camera;
camera.targetEntity = playerEntityID;
camera.followTarget = true;
camera.offset = Vector(-screenWidth/2.f, -screenHeight/2.f, 0.0f);
camera.mode = Camera_data::Mode::Viewport_Follow;
world.AddComponent<Camera_data>(playerEntityID, camera);
```

## Split-Screen Support

Viewport layout is automatically computed based on player count:

| Players | Layout |
|---------|--------|
| 1 | Full screen |
| 2 | Vertical split (2 columns) |
| 3 | Vertical split (3 columns) |
| 4 | 2x2 grid |
| 5-6 | 3x2 grid |
| 7-8 | 4x2 grid |

The layout is computed by `VideoGame::UpdateAllPlayerViewports()`, which:
- Calculates viewport rectangles based on screen size and player count
- Updates `Viewport_data.viewRect` for each player entity
- Updates camera offsets to match viewport dimensions

## Rendering Flow

The main render loop in `OlympeEngine.cpp`:

1. Queries all entities with `Camera_data` + `Viewport_data`
2. For each camera entity:
   - Sets the SDL viewport to the entity's `viewRect`
   - Sets the active camera entity in `RenderingSystem`
   - Calls `World::Render()` to render all visual entities

```cpp
for (EntityID cameraEntity : cameraEntities)
{
    Viewport_data& viewport = World::Get().GetComponent<Viewport_data>(cameraEntity);
    Camera_data& camera = World::Get().GetComponent<Camera_data>(cameraEntity);
    
    // Set SDL viewport
    SDL_SetRenderViewport(renderer, &viewportRect);
    
    // Set active camera for rendering
    RenderingSystem* renderSys = World::Get().GetSystem<RenderingSystem>();
    renderSys->SetActiveCameraEntity(cameraEntity);
    
    // Render world for this viewport
    World::Get().Render();
}
```

## Backward Compatibility

The legacy `CameraManager` is maintained for backward compatibility:

- Still processes camera events for non-ECS code
- Used as fallback when no ECS camera is active
- Can be synced from ECS data via `SetActiveCameraFromECS()`

**Migration path:** New code should use ECS components. Legacy code will continue to work through the fallback mechanism.

## Adding a New Camera

To create a new camera entity:

```cpp
// Create entity
EntityID cameraEntity = World::Get().CreateEntity();

// Add camera component
Camera_data camera;
camera.position = Vector(0, 0, 0);
camera.offset = Vector(-400, -300, 0);
camera.followTarget = true;
camera.targetEntity = playerEntity;
camera.followSpeed = 0.75f;
World::Get().AddComponent<Camera_data>(cameraEntity, camera);

// Add viewport component
Viewport_data viewport;
viewport.viewRect = {0.f, 0.f, 800.f, 600.f};
viewport.viewportIndex = 0;
viewport.enabled = true;
World::Get().AddComponent<Viewport_data>(cameraEntity, viewport);
```

## Best Practices

1. **Always use ECS components** for new camera features
2. **Call UpdateAllPlayerViewports()** when adding/removing players
3. **Set followTarget=true** to enable automatic camera following
4. **Use camera bounds** to prevent camera from going outside level limits
5. **Keep followSpeed** in range 0.1-1.0 for smooth movement

## Future Enhancements

Possible future improvements:
- Camera shake effects via temporary position offset
- Camera zoom transitions
- Multiple cameras per player (picture-in-picture)
- Camera deadzone (only move camera when player moves beyond threshold)
- Custom camera paths/rails
- Camera transition animations when switching targets
