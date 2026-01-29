# ECS Camera System Usage Guide

## Overview
The ECS Camera System provides a complete camera solution with support for multiple players, split-screen, zoom, rotation, target following, and various effects.

## Basic Setup

### Creating a Camera
```cpp
// Get the camera system
CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();

// Create a camera for player 0 with keyboard controls
EntityID camera = camSys->CreateCameraForPlayer(0, true);

// Create cameras for additional players (joystick-controlled)
EntityID camera1 = camSys->CreateCameraForPlayer(1, false);
camSys->BindCameraToJoystick(camera1, 1, joystickId);
```

## Camera Controls

### Keyboard Controls (Numpad)

**Directional Movement:**
- **Numpad 8**: Move camera up (↑)
- **Numpad 2**: Move camera down (↓)
- **Numpad 4**: Move camera left (←)
- **Numpad 6**: Move camera right (→)
- **Numpad 7**: Move camera diagonal up-left (↖)
- **Numpad 9**: Move camera diagonal up-right (↗)
- **Numpad 1**: Move camera diagonal down-left (↙)
- **Numpad 3**: Move camera diagonal down-right (↘)

**Zoom Control (Discrete Levels):**
- **Numpad +**: Zoom in (steps through fixed levels)
- **Numpad -**: Zoom out (steps through fixed levels)
- **Zoom Levels**: 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0

**Rotation Control (Discrete Levels):**
- **Numpad ***: Rotate clockwise (+15°)
- **Numpad /**: Rotate counter-clockwise (-15°)
- **Rotation Levels**: 24 steps of 15° (0°, 15°, 30°, ..., 345°)

**Other Controls:**
- **Numpad 5**: Reset camera controls (zoom=1.0, rotation=0.0, and resumes target following if a target is set)

### Joystick Controls
- **Right Stick**: Move camera
- **Left/Right Triggers**: Rotate camera
- **Button 10**: Reset camera

## Target Following

### Follow an ECS Entity
```cpp
// Create a player entity
EntityID player = World::Get().CreateEntity();
World::Get().AddComponent<Position_data>(player);

// Make camera follow the player
camSys->SetCameraTarget_ECS(camera, player);
```

### Follow a Legacy GameObject
```cpp
GameObject* player = /* ... */;
camSys->SetCameraTarget_Legacy(camera, player);
```

### Stop Following
```cpp
camSys->ClearCameraTarget(camera);
```

## Camera Effects

### Camera Shake
```cpp
// Trigger shake with intensity and duration
CameraEventHandler::Get().TriggerCameraShake(0, 10.0f, 0.5f);

// Stop shake immediately
CameraEventHandler::Get().StopCameraShake(0);
```

### Zoom Control

**Manual Zoom (Keyboard):**
The numpad +/- keys control zoom using discrete levels for precision:
- **Zoom Levels**: `0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0`
- Each press of + or - moves to the next/previous level
- Default starting level is 1.0 (index 3)
- Keyboard controls only allow selecting from these fixed zoom levels (smooth interpolation occurs during transitions)

**Programmatic Zoom:**
```cpp
// Smooth zoom to target level
CameraEventHandler::Get().ZoomCameraTo(0, 2.0f, 5.0f); // zoom to 2x, speed 5

// Instant zoom
CameraEventHandler::Get().ZoomCameraTo(0, 2.0f, 0.0f); // speed 0 = instant
```

**Note**: When using the numpad 5 reset key, zoom returns to 1.0 (index 3).

### Rotation Control

**Manual Rotation (Keyboard - Discrete Levels):**
The numpad * and / keys control rotation using discrete 15° steps for precision:
- **Rotation Levels**: 24 steps of 15° (0°, 15°, 30°, 45°, ..., 330°, 345°)
- Each press of * or / rotates by exactly 15°
- Automatic wraparound: 345° + 15° = 0°, and 0° - 15° = 345°
- Default starting level is 0° (level 0)
- Keyboard controls only allow selecting from these fixed rotation levels (smooth interpolation occurs during transitions)
- **Cardinal angles** are easily accessible: 0° (North), 90° (East), 180° (South), 270° (West)

**Examples:**
- 0° → * → 15° → * → 30°
- 345° → * → 0° (wraparound)
- 0° → / → 345° (reverse wraparound)

**Programmatic Rotation:**
```cpp
// Smooth rotation
CameraEventHandler::Get().RotateCameraTo(0, 45.0f, 5.0f); // rotate to 45°

// Instant rotation
CameraEventHandler::Get().RotateCameraTo(0, 45.0f, 0.0f);
```

**Note**: When using the numpad 5 reset key, rotation returns to exactly 0° (level 0).

## Camera Reset (Numpad 5)

The reset key (Numpad 5) provides a complete camera reset:

**What Gets Reset:**
- **Zoom**: Returns to 1.0 (index 3 in discrete zoom levels)
- **Rotation**: Returns to 0.0 degrees (level 0 in discrete rotation levels)
- **Control Offset**: Cleared to (0, 0)
- **Camera Shake**: Stopped if active

**Target Following Behavior:**
- If the camera is following an entity (`CameraTarget_data` is set), pressing reset will **re-enable** target following
- This is useful when you've manually moved the camera away from its target and want to return to automatic following
- If the camera is in Free mode, it resets position to (0, 0)

```cpp
// Example: Reset camera programmatically
CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
camSys->ResetCameraControls(cameraEntity);
```

## Camera Bounds

### Set Movement Bounds
```cpp
SDL_FRect bounds = {0, 0, 1920, 1080}; // World space bounds
CameraEventHandler::Get().SetCameraBounds(0, bounds);
```

### Clear Bounds
```cpp
CameraEventHandler::Get().ClearCameraBounds(0);
```

## Split-Screen Setup

### 2-Player Horizontal Split
```cpp
// Setup viewports through ViewportManager
ViewportManager::Get().AddPlayer(0, ViewportLayout::ViewportLayout_Grid2x1);
ViewportManager::Get().AddPlayer(1, ViewportLayout::ViewportLayout_Grid2x1);

// Create cameras for both players
CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
EntityID cam0 = camSys->CreateCameraForPlayer(0, true);  // Keyboard
EntityID cam1 = camSys->CreateCameraForPlayer(1, false); // Joystick
camSys->BindCameraToJoystick(cam1, 1, joystickId);
```

### 4-Player Grid
```cpp
ViewportManager::Get().AddPlayer(0, ViewportLayout::ViewportLayout_Grid2x2);
ViewportManager::Get().AddPlayer(1, ViewportLayout::ViewportLayout_Grid2x2);
ViewportManager::Get().AddPlayer(2, ViewportLayout::ViewportLayout_Grid2x2);
ViewportManager::Get().AddPlayer(3, ViewportLayout::ViewportLayout_Grid2x2);

// Create cameras for all players
for (short i = 0; i < 4; i++)
{
    EntityID cam = camSys->CreateCameraForPlayer(i, false);
    camSys->BindCameraToJoystick(cam, i, joystickIds[i]);
}
```

## Direct Component Access

### Modify Camera Properties Directly
```cpp
EntityID camera = camSys->GetCameraEntityForPlayer(0);

// Modify camera data
Camera_data& cam = World::Get().GetComponent<Camera_data>(camera);
cam.zoom = 1.5f;
cam.rotation = 30.0f;
cam.manualMoveSpeed = 300.0f;

// Modify target following
CameraTarget_data& target = World::Get().GetComponent<CameraTarget_data>(camera);
target.smoothFactor = 8.0f;
target.allowManualControl = true;

// Add camera shake
CameraEffects_data& effects = World::Get().GetComponent<CameraEffects_data>(camera);
effects.isShaking = true;
effects.shakeIntensity = 15.0f;
effects.shakeDuration = 1.0f;
effects.shakeTimeRemaining = 1.0f;
```

## Advanced Features

### Custom Input Bindings
```cpp
CameraInputBinding_data& binding = World::Get().GetComponent<CameraInputBinding_data>(camera);

// Change keyboard bindings
binding.key_up = SDLK_W;
binding.key_down = SDLK_S;
binding.key_left = SDLK_A;
binding.key_right = SDLK_D;

// Change joystick axes
binding.axis_horizontal = 0; // Use left stick instead
binding.axis_vertical = 1;

// Adjust deadzone
binding.deadzone = 0.2f;
```

### Camera Modes
```cpp
Camera_data& cam = World::Get().GetComponent<Camera_data>(camera);

// Free camera (no target)
cam.controlMode = CameraControlMode::Mode_Free;

// Strict follow (no manual control)
cam.controlMode = CameraControlMode::Mode_Follow;

// Follow with manual override
cam.controlMode = CameraControlMode::Mode_FollowWithControl;
```

## Example: Boss Fight Camera

```cpp
void SetupBossFightCamera(short playerID, EntityID bossEntity)
{
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    EntityID camera = camSys->GetCameraEntityForPlayer(playerID);
    
    // Follow the boss
    camSys->SetCameraTarget_ECS(camera, bossEntity);
    
    // Zoom out to see more
    CameraEventHandler::Get().ZoomCameraTo(playerID, 0.7f, 3.0f);
    
    // Set arena bounds
    SDL_FRect arenaBounds = {-500, -500, 1000, 1000};
    CameraEventHandler::Get().SetCameraBounds(playerID, arenaBounds);
    
    // Trigger dramatic shake
    CameraEventHandler::Get().TriggerCameraShake(playerID, 20.0f, 0.3f);
}
```

## Example: Cinematic Camera

```cpp
void PlayCinematicSequence(short playerID)
{
    CameraEventHandler& handler = CameraEventHandler::Get();
    
    // Move to location 1
    handler.TeleportCamera(playerID, 100, 100);
    handler.ZoomCameraTo(playerID, 1.5f, 2.0f);
    
    // Wait 2 seconds, then move to location 2
    // (Use a timer system or coroutine)
    
    // Rotate for dramatic effect
    handler.RotateCameraTo(playerID, 180.0f, 1.0f);
    
    // Return to normal
    handler.ResetCamera(playerID);
}
```

## Rendering Integration

The camera system automatically integrates with the RenderingSystem:
- Cameras render to their assigned viewports
- World-to-screen transformation is automatic
- Frustum culling is applied for performance
- Rotation and zoom are handled transparently

No additional rendering code is required - just create entities with `Position_data`, `VisualSprite_data`, and `BoundingBox_data` components.

## Performance Notes

- Frustum culling automatically skips entities outside camera view
- Multiple cameras use viewport clipping for efficient rendering
- Camera updates run once per frame in CameraSystem::Process()
- Smooth interpolation uses delta time for frame-rate independence

## Troubleshooting

### Camera Not Moving
- Check that input bindings are correct
- Verify keyboard/joystick is connected
- Ensure camera is not in Mode_Follow with strict following

### No Rendering
- Verify camera.isActive is true
- Check that viewport is configured correctly
- Ensure entities have required rendering components

### Jerky Following
- Increase smoothFactor for smoother following
- Check that target entity position is being updated
- Verify delta time (GameEngine::fDt) is correct
