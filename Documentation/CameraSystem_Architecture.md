# ECS Camera System Architecture

## Overview
The ECS Camera System is a comprehensive camera solution built on the Entity-Component-System architecture, providing multi-player support, split-screen capabilities, and advanced features like smooth zoom, rotation, target following, and camera effects.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    GameEngine / World                        │
│  - Initializes CameraSystem and CameraEventHandler          │
│  - Manages ECS lifecycle                                     │
└────────────────┬──────────────────────┬─────────────────────┘
                 │                      │
      ┌──────────▼──────────┐  ┌────────▼────────────┐
      │   CameraSystem      │  │  CameraEventHandler │
      │   (ECS_System)      │  │    (Singleton)      │
      └──────────┬──────────┘  └──────────┬──────────┘
                 │                        │
                 │ Processes              │ Dispatches
                 │ Camera Entities        │ Events
                 │                        │
      ┌──────────▼────────────────────────▼──────────┐
      │          Camera Components                    │
      │  - Camera_data                                │
      │  - CameraTarget_data                          │
      │  - CameraEffects_data                         │
      │  - CameraBounds_data                          │
      │  - CameraInputBinding_data                    │
      └───────────────────────┬───────────────────────┘
                              │
                   ┌──────────▼──────────┐
                   │   RenderingSystem   │
                   │  - CameraTransform  │
                   │  - Frustum Culling  │
                   └─────────────────────┘
```

## Component Definitions

### Camera_data
**Purpose**: Main camera component containing position, zoom, rotation, and control settings.

**Key Fields**:
- `playerId`: Owner player ID
- `position`: World position
- `zoom`, `targetZoom`: Current and target zoom levels
- `rotation`, `targetRotation`: Current and target rotation angles
- `controlMode`: Free, Follow, or FollowWithControl
- `viewportRect`: Rendering viewport
- `ZOOM_LEVELS[]`: Static array of discrete zoom levels (0.25 to 5.0)
- `currentZoomLevelIndex`: Current index in ZOOM_LEVELS array (default: 3 = 1.0)

**Update Frequency**: Every frame by CameraSystem::Process()

### CameraTarget_data
**Purpose**: Manages target following for both ECS entities and legacy GameObjects.

**Key Fields**:
- `targetEntityID`: ECS entity to follow
- `targetObject`: Legacy GameObject to follow
- `followTarget`: Enable/disable following
- `smoothFactor`: Following smoothness (0-10)
- `allowManualControl`: Allow manual override while following

**Update Frequency**: Every frame by CameraSystem::UpdateCameraFollow()

### CameraEffects_data
**Purpose**: Visual effects like camera shake.

**Key Fields**:
- `isShaking`: Shake active flag
- `shakeIntensity`: Shake magnitude
- `shakeDuration`: Total shake time
- `shakeTimeRemaining`: Time left
- `shakeOffset`: Current offset

**Update Frequency**: Every frame by CameraSystem::UpdateCameraShake()

### CameraBounds_data
**Purpose**: Constrains camera movement to a specific area.

**Key Fields**:
- `useBounds`: Enable/disable bounds
- `boundingBox`: World space constraint area
- `clampToViewport`: Clamp viewport edges to bounds

**Update Frequency**: Every frame by CameraSystem::ApplyCameraBounds()

### CameraInputBinding_data
**Purpose**: Configures input controls (keyboard or joystick).

**Key Fields**:
- `playerId`: Player ID (-1 = keyboard)
- `useKeyboard`: Keyboard vs joystick
- `key_*`: Keyboard bindings (including diagonal keys: key_up_left, key_up_right, key_down_left, key_down_right)
- `axis_*`, `button_*`: Joystick bindings
- `inputDirection`, `rotationInput`, `zoomInput`: Current input state

**Update Frequency**: Every frame by CameraSystem::UpdateCameraInput()

## System Processing Pipeline

### CameraSystem::Process() Flow
```
1. UpdateCameraInput()
   ├─ ProcessKeyboardInput() or
   └─ ProcessJoystickInput()
      └─ Apply input to controlOffset, targetZoom, targetRotation

2. UpdateCameraFollow()
   ├─ Get target position (ECS or GameObject)
   ├─ Apply followOffset
   └─ Blend camera position towards target

3. UpdateCameraZoom()
   └─ Smooth interpolation: zoom → targetZoom

4. UpdateCameraRotation()
   └─ Smooth interpolation: rotation → targetRotation

5. UpdateCameraShake()
   ├─ Decrement shakeTimeRemaining
   └─ Generate random shakeOffset

6. ApplyCameraBounds()
   └─ Clamp position + controlOffset to bounds
```

### Rendering Integration
```
RenderingSystem::Render()
├─ Check if ECS cameras exist
├─ For each player camera:
│  ├─ GetActiveCameraTransform(playerID)
│  ├─ Set viewport and clip rect
│  └─ RenderEntitiesForCamera(cameraTransform)
│     ├─ For each entity with Position+Visual+BoundingBox:
│     │  ├─ Frustum culling check
│     │  ├─ WorldToScreen transformation
│     │  └─ Render sprite with rotation
│     └─ Next entity
└─ Reset viewport
```

## CameraTransform Structure

**Purpose**: Encapsulates camera transformation for rendering.

**Key Methods**:
- `WorldToScreen(worldPos)`: Transforms world coordinates to screen coordinates
  1. Relative to camera position
  2. Apply rotation
  3. Apply zoom
  4. Apply screen offset
  5. Center in viewport

- `WorldSizeToScreenSize(worldSize)`: Scales world size to screen size
- `IsVisible(worldBounds)`: Frustum culling check

## Event System Integration

### CameraEventHandler
**Purpose**: Provides event-driven API for camera control.

**Event Types Handled**:
- `Camera_Shake` / `Camera_Shake_Stop`
- `Camera_Teleport`
- `Camera_ZoomTo`
- `Camera_RotateTo`
- `Camera_Reset`
- `Camera_SetBounds` / `Camera_ClearBounds`
- `Camera_Target_Follow` / `Camera_Target_Unfollow`

**API Methods**:
- `TriggerCameraShake(playerID, intensity, duration)`
- `ZoomCameraTo(playerID, zoom, speed)`
- `RotateCameraTo(playerID, angle, speed)`
- `SetCameraBounds(playerID, bounds)`
- `FollowTarget(playerID, entityID)`
- etc.

## Multi-Player / Split-Screen

### Viewport Management
- Each camera has its own `viewportRect`
- Viewports configured through `ViewportManager`
- Supports 1-4 players with various layouts:
  - 1x1: Full screen
  - 2x1: Vertical split
  - 2x2: Grid
  - 3x1, 3x2, 4x2: Various configurations

### Rendering Per Camera
- Each camera renders independently to its viewport
- Frustum culling per camera avoids rendering off-screen entities
- Viewports use SDL clip rects for proper isolation

## Input System

### Keyboard (Numpad by Default)
```
7: Up-Left    8: Up       9: Up-Right     +: Zoom In (discrete)    /: Rotate Left
4: Left       5: Reset    6: Right        -: Zoom Out (discrete)   *: Rotate Right
1: Down-Left  2: Down     3: Down-Right
```

**Discrete Zoom Levels**: 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0
- Each press of +/- moves one step through these fixed levels
- Diagonal movement is normalized to prevent faster diagonal speed

### Joystick
```
Right Stick: Camera Movement
Left Trigger: Rotate Left
Right Trigger: Rotate Right
Button 10: Reset
```

### Input Processing
1. `KeyboardManager::Get().IsKeyHeld()` for keyboard
2. `JoystickManager::Get().GetAxis()` for joystick
3. Deadzone application
4. Direction vector construction
5. Apply to `controlOffset` with `manualMoveSpeed`

## Control Modes

### Mode_Free
- Camera moves freely
- No target following
- Full manual control

### Mode_Follow
- Camera strictly follows target
- No manual control offset
- `controlOffset` reset to zero

### Mode_FollowWithControl
- Camera follows target
- Manual control adds to position
- `controlOffset` decays towards zero based on `manualControlDecay`

## Smooth Interpolation

### Zoom and Rotation
```cpp
value += (target - value) * speed * deltaTime
```
- Exponential smooth interpolation
- `speed` parameter controls convergence rate
- Snap to target when very close (< 0.01 for zoom, < 1° for rotation)

### Target Following
```cpp
position = vBlend(targetPos, position, smoothFactor)
```
- Uses existing `vBlend()` function
- `smoothFactor` controls smoothness (higher = smoother)
- Delta time integrated in vBlend implementation

## Legacy Compatibility

### CameraManager Fallback
- RenderingSystem checks for active ECS cameras
- Falls back to legacy `CameraManager` if no ECS cameras
- Allows gradual migration from old to new system

### GameObject Support
- `CameraTarget_data.targetObject` supports legacy GameObjects
- Uses `GameObject::GetPosition()` for following
- Enables mixed ECS/legacy scenes

## Performance Optimizations

### Frustum Culling
- Transforms entity bounding box corners to screen space
- Checks intersection with viewport
- Skips rendering for off-screen entities
- Handles rotation correctly

### Component Access
- Direct pool access via `World::Get().GetComponent<T>()`
- No virtual function overhead
- Cache-friendly iteration over `m_entities`

### Viewport Clipping
- SDL clip rects prevent overdraw
- Each camera only affects its viewport
- No inter-camera interference

## Memory Layout

### Per-Camera Memory (Typical)
```
Camera_data:          ~100 bytes
CameraTarget_data:    ~40 bytes
CameraEffects_data:   ~40 bytes
CameraBounds_data:    ~30 bytes
CameraInputBinding:   ~120 bytes
Total per camera:     ~330 bytes
```

### 4-Player Setup
- ~1.3 KB for all camera components
- Negligible compared to sprite/texture memory
- No dynamic allocations during gameplay

## Extension Points

### Custom Camera Types
Add to `CameraType` enum:
```cpp
enum class CameraType : uint8_t {
    CameraType_2D,
    CameraType_2_5D,
    CameraType_Isometric,
    CameraType_Perspective  // Add custom type
};
```

### Custom Effects
Extend `CameraEffects_data`:
```cpp
struct CameraEffects_data {
    // Existing shake...
    
    // Add new effects
    bool isVibrating;
    float vibrateIntensity;
    Vector colorTint;
};
```

### Custom Input Schemes
Modify `CameraInputBinding_data`:
```cpp
// Add touch controls
Vector touchStartPos;
bool isTouching;

// Add gesture support
enum class Gesture { None, Pinch, Swipe };
Gesture currentGesture;
```

## Testing Recommendations

### Unit Tests
- Camera creation/destruction
- Input binding
- Target following
- Zoom/rotation interpolation
- Bounds clamping

### Integration Tests
- Multi-camera rendering
- Split-screen layouts
- Event handling
- Legacy compatibility

### Performance Tests
- Frustum culling effectiveness
- Frame time with 4 cameras
- Memory usage scaling

## Known Limitations

1. **Maximum 100 cameras**: Limited by ECS entity count (can be increased)
2. **No 3D perspective**: Currently 2D/2.5D/Isometric only
3. **Rotation**: Affects all entities uniformly (no per-entity compensation)
4. **Input binding**: One input source per camera (no shared control)

## Future Enhancements

### Potential Features
- Camera zones (auto-switch based on player position)
- Cinematic camera paths
- Camera collision avoidance
- Depth-of-field effects
- Screen-space effects (blur, distortion)
- Camera presets/profiles

### Optimization Opportunities
- Spatial partitioning for large worlds
- Level-of-detail based on distance from camera
- Occlusion culling
- Multi-threaded camera updates
