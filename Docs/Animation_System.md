# Animation System Documentation

## Overview

The Olympe Engine Animation System provides a complete 2D sprite animation framework with state machine-based animation control. It supports multi-frame spritesheets, animation events, and parameter-driven state transitions.

## Architecture

The animation system consists of several key components:

### Core Components

1. **AnimationBank** - Contains animation data (spritesheets, frames, events)
2. **AnimationGraph** - State machine for controlling animation transitions
3. **AnimationManager** - Singleton manager for loading and caching animation resources
4. **AnimationErrorHandler** - Handles error logging with de-duplication
5. **VisualAnimation_data** - ECS component for entity animation state

### Data Flow

```
JSON Files (Gamedata/Animations/)
    ↓
AnimationManager (loads at startup)
    ↓
AnimationBank + AnimationGraph (cached in memory)
    ↓
VisualAnimation_data (component on entities)
    ↓
AnimationSystem (future: updates animation state per frame)
    ↓
Rendering System (future: renders current frame)
```

## JSON File Formats

### Animation Bank Format

Animation banks define the visual data for animations.

**Location:** `Gamedata/Animations/AnimationBanks/*.json`

```json
{
  "schema_version": 1,
  "bankName": "player_animations",
  "description": "Animation bank for player character",
  "spritesheets": [
    {
      "id": "player_idle",
      "path": "./Resources/Sprites/player_idle.png",
      "frameWidth": 32,
      "frameHeight": 32,
      "columns": 4,
      "rows": 1,
      "totalFrames": 4,
      "hotspot": { "x": 16, "y": 16 }
    }
  ],
  "animations": [
    {
      "name": "Idle",
      "spritesheetId": "player_idle",
      "startFrame": 0,
      "endFrame": 3,
      "framerate": 8.0,
      "looping": true,
      "events": [
        {
          "frame": 2,
          "type": "sound",
          "data": {
            "soundPath": "./Resources/Sounds/footstep.wav",
            "volume": 0.5
          }
        }
      ]
    }
  ]
}
```

**Key Fields:**
- `spritesheets`: Array of spritesheet definitions
  - `id`: Unique identifier for the spritesheet
  - `path`: File path to the texture
  - `frameWidth`/`frameHeight`: Size of each frame in pixels
  - `columns`/`rows`: Grid layout of frames
  - `totalFrames`: Total number of frames in the sheet
  - `hotspot`: Rendering anchor point (default: center)

- `animations`: Array of animation definitions
  - `name`: Animation name (used by AnimationGraph states)
  - `spritesheetId`: Reference to a spritesheet
  - `startFrame`/`endFrame`: Frame range for the animation
  - `framerate`: Playback speed in frames per second
  - `looping`: Whether animation loops
  - `events`: Events triggered at specific frames

### Animation Graph Format

Animation graphs define state machines for controlling animations.

**Location:** `Gamedata/Animations/AnimationGraphs/*.json`

```json
{
  "schema_version": 1,
  "graphName": "player_animgraph",
  "description": "Animation state machine for player",
  "animationBankPath": "./Gamedata/Animations/AnimationBanks/player_animations.json",
  "parameters": [
    { "name": "speed", "type": "float", "defaultValue": 0.0 },
    { "name": "isMoving", "type": "bool", "defaultValue": false },
    { "name": "isDead", "type": "bool", "defaultValue": false }
  ],
  "states": [
    {
      "name": "Idle",
      "animationName": "Idle",
      "blendMode": "override",
      "priority": 0
    },
    {
      "name": "Walk",
      "animationName": "Walk",
      "blendMode": "override",
      "priority": 1
    }
  ],
  "transitions": [
    {
      "from": "Idle",
      "to": "Walk",
      "transitionTime": 0.1,
      "conditions": [
        { "parameter": "isMoving", "operator": "==", "value": true },
        { "parameter": "speed", "operator": ">", "value": 10.0 }
      ]
    },
    {
      "from": "ANY",
      "to": "Dead",
      "transitionTime": 0.1,
      "conditions": [
        { "parameter": "isDead", "operator": "==", "value": true }
      ]
    }
  ],
  "defaultState": "Idle"
}
```

**Key Fields:**
- `parameters`: Variables that control state transitions
  - Types: `bool`, `float`, `int`, `string`
  - Set by gameplay code or AI systems

- `states`: Animation states in the state machine
  - `name`: State name
  - `animationName`: Which animation to play in this state
  - `blendMode`: How to blend (`override`, `additive`, `blend`)
  - `priority`: Higher priority states override lower priority

- `transitions`: Rules for changing states
  - `from`: Source state (or "ANY" for any state)
  - `to`: Target state
  - `transitionTime`: Blend duration in seconds
  - `conditions`: All must be true to trigger transition
    - Operators: `==`, `!=`, `>`, `>=`, `<`, `<=`

## Entity Prefab Integration

Add the `VisualAnimation_data` component to entity prefabs:

```json
{
  "type": "VisualAnimation_data",
  "properties": {
    "animGraphPath": "./Gamedata/Animations/AnimationGraphs/player_animgraph.json",
    "currentState": "Idle",
    "playbackSpeed": 1.0,
    "flipX": false,
    "flipY": false
  }
}
```

## C++ API Usage

### Initialization (in GameEngine)

```cpp
#include "Animation/AnimationManager.h"

// In GameEngine::Initialize()
OlympeAnimation::AnimationManager::Get().Init();
OlympeAnimation::AnimationManager::Get().LoadAnimationBanks("Gamedata/Animations/AnimationBanks");
OlympeAnimation::AnimationManager::Get().LoadAnimationGraphs("Gamedata/Animations/AnimationGraphs");
```

### Accessing Animation Resources

```cpp
using namespace OlympeAnimation;

// Get an animation graph
AnimationGraph* graph = AnimationManager::Get().GetGraph("player_animgraph");

// Get an animation bank
AnimationBank* bank = AnimationManager::Get().GetBank("player_animations");

// Get a specific animation
Animation* anim = bank->GetAnimation("Walk");

// Get a spritesheet
SpriteSheet* sheet = bank->GetSpriteSheet("player_walk");
```

### Controlling Animation State

```cpp
// Set parameters (from gameplay code)
graph->SetParameter("isMoving", true);
graph->SetParameter("speed", 150.0f);
graph->SetParameter("isDead", false);

// Update state machine (checks transitions)
bool stateChanged = graph->Update(deltaTime);

// Get current state
std::string currentState = graph->GetCurrentState();

// Get animation for current state
std::string animName = graph->GetCurrentAnimationName();
```

### ECS Component Usage

```cpp
// Access component on an entity
auto* animComp = world.GetComponent<VisualAnimation_data>(entityID);

if (animComp)
{
    // Initialize runtime pointers (first time only)
    if (!animComp->animGraph)
    {
        animComp->animGraph = AnimationManager::Get().GetGraph("player_animgraph");
        if (animComp->animGraph)
        {
            std::string bankPath = animComp->animGraph->GetAnimationBankPath();
            // Extract bank name from path
            animComp->animBank = AnimationManager::Get().GetBank("player_animations");
        }
    }

    // Update animation state
    if (animComp->animGraph)
    {
        animComp->animGraph->Update(GameEngine::fDt);
        animComp->currentState = animComp->animGraph->GetCurrentState();
    }
}
```

## Error Handling

The system provides graceful error handling with automatic fallbacks:

```cpp
#include "Animation/AnimationErrorHandler.h"

// Errors are logged only once per unique combination
AnimationErrorHandler::Get().LogMissingAnimation(
    "Player_1",
    "./Gamedata/Animations/AnimationGraphs/player_animgraph.json",
    "Walk",
    "Walk"
);

// Fallback behavior:
// - Missing animations use placeholder texture: ./Resources/Sprites/player.png
// - Debug overlay shows: "MISSING ANIM\nState: {state}\nEntity: {entity}"
```

## Adding New Animations

### Step 1: Prepare Spritesheet

1. Create a PNG spritesheet with frames arranged in a grid
2. All frames should be the same size
3. Place in `Resources/Sprites/` directory

### Step 2: Create Animation Bank

Create or edit a file in `Gamedata/Animations/AnimationBanks/`:

```json
{
  "spritesheets": [
    {
      "id": "mychar_jump",
      "path": "./Resources/Sprites/mychar_jump.png",
      "frameWidth": 32,
      "frameHeight": 32,
      "columns": 8,
      "rows": 1,
      "totalFrames": 8,
      "hotspot": { "x": 16, "y": 16 }
    }
  ],
  "animations": [
    {
      "name": "Jump",
      "spritesheetId": "mychar_jump",
      "startFrame": 0,
      "endFrame": 7,
      "framerate": 16.0,
      "looping": false,
      "events": []
    }
  ]
}
```

### Step 3: Add State to Animation Graph

Edit the corresponding graph file in `Gamedata/Animations/AnimationGraphs/`:

```json
{
  "parameters": [
    { "name": "isJumping", "type": "bool", "defaultValue": false }
  ],
  "states": [
    {
      "name": "Jump",
      "animationName": "Jump",
      "blendMode": "override",
      "priority": 5
    }
  ],
  "transitions": [
    {
      "from": "ANY",
      "to": "Jump",
      "transitionTime": 0.05,
      "conditions": [
        { "parameter": "isJumping", "operator": "==", "value": true }
      ]
    }
  ]
}
```

### Step 4: Update Gameplay Code

Set parameters from your game logic:

```cpp
// In PlayerController or AIBehaviorTree
animGraph->SetParameter("isJumping", true);
```

## Animation Events

Events are triggered at specific frames during playback. They can be used for:

- **Sound effects**: Play footstep sounds, attack swooshes, etc.
- **Hitboxes**: Activate attack hitboxes at impact frame
- **VFX**: Spawn particle effects, screen shake, etc.
- **Game logic**: Signal animation completion, trigger state changes

Example event types:

```json
{
  "events": [
    {
      "frame": 3,
      "type": "sound",
      "data": { "soundPath": "./Resources/Sounds/hit.wav", "volume": 0.8 }
    },
    {
      "frame": 3,
      "type": "hitbox",
      "data": { "offsetX": 20, "offsetY": 0, "width": 30, "height": 32, "damage": 15.0 }
    },
    {
      "frame": 5,
      "type": "vfx",
      "data": { "effectType": "dust_cloud", "duration": 0.5 }
    },
    {
      "frame": 7,
      "type": "gamelogic",
      "data": { "action": "animation_complete" }
    }
  ]
}
```

## Best Practices

1. **Naming Conventions**
   - Use PascalCase for animation names: `Idle`, `Walk`, `Attack`
   - Use snake_case for file names: `player_animations.json`
   - Use descriptive spritesheet IDs: `player_walk`, `zombie_attack`

2. **Frame Organization**
   - Keep related animations in the same bank
   - Use consistent frame sizes within a bank
   - Place hotspots at the character's feet or center for consistent positioning

3. **State Machine Design**
   - Keep "Idle" as the default state
   - Use higher priorities for important animations (hit, death)
   - Use "ANY" transitions for interrupting states (death, stun)
   - Test all transition paths

4. **Performance**
   - Animation resources are loaded once at startup
   - Textures are cached in DataManager
   - State machines are lightweight and fast
   - Use appropriate framerates (8-16 fps for pixel art, 24-60 for smooth animation)

5. **Error Prevention**
   - Validate JSON syntax before committing
   - Ensure spritesheet paths are correct
   - Match animation names between banks and graphs
   - Test missing animation fallback behavior

## Future Enhancements

The following features are planned for future releases:

1. **AnimationSystem (ECS System)**
   - Automatic frame advancement
   - Event triggering
   - Sprite rendering integration

2. **Advanced Blending**
   - Smooth transitions between animations
   - Additive animation support
   - Layer-based animation mixing

3. **Blueprint Editor Integration**
   - Visual state machine editor
   - Animation preview
   - Parameter debugging

4. **Runtime Features**
   - Animation speed modulation
   - Frame skipping for performance
   - Dynamic animation loading/unloading

## Troubleshooting

### Animation not playing
- Check that `animGraphPath` is correct in entity prefab
- Verify animation bank and graph files exist
- Ensure animation names match between bank and graph states
- Check console for error messages

### Wrong animation displayed
- Verify parameter values are being set correctly
- Check transition conditions in animation graph
- Ensure state priorities are correct
- Use AnimationGraph::GetCurrentState() to debug

### Spritesheet not loading
- Verify texture path is correct
- Check that texture exists in Resources/Sprites/
- Ensure DataManager can load the texture
- Look for texture loading errors in console

### Performance issues
- Reduce framerate for less important animations
- Use fewer total frames
- Optimize spritesheet size and format
- Profile AnimationGraph::Update() calls

## Support

For questions or issues, please refer to:
- Engine documentation: `/Docs/`
- Example files: `Gamedata/Animations/`
- Source code: `Source/Animation/`

---

**Last Updated:** 2026-02-12
**Version:** 1.0
**Author:** Olympe Engine Team
