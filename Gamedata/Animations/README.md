# Animation System Assets Directory

This directory contains animation definitions for the Olympe Engine 2D Animation System.

## Directory Structure

```
Gamedata/Animations/
├── AnimationBanks/     # Animation frame definitions and playback properties
│   ├── thesee_animations.json
│   ├── player_animations.json
│   ├── enemy_goblin_animations.json
│   └── boss_dragon_animations.json
└── AnimationGraphs/    # Finite State Machines (FSM) for animation control
    ├── thesee_animgraph.json
    ├── player_animgraph.json
    ├── enemy_goblin_animgraph.json
    └── boss_dragon_animgraph.json
```

## Animation Banks

**Purpose**: Define sprite animations using spritesheet frames.

**Location**: `AnimationBanks/`

**File Format**: JSON

**Contents**:
- Spritesheet path and dimensions
- Frame layout (columns, spacing, margin)
- Animation sequences (frames, timing, looping)
- Per-frame events and properties

**Naming Convention**: `{entity_name}_animations.json`

**Examples**:
- `thesee_animations.json` - Thésée character animations
- `player_animations.json` - Player character animations
- `enemy_goblin_animations.json` - Goblin enemy animations

**Documentation**: See [Animation Banks Reference](../../website/docs/user-guide/animation-system/animation-banks.md)

## Animation Graphs

**Purpose**: Define finite state machines (FSM) for controlling animation flow.

**Location**: `AnimationGraphs/`

**File Format**: JSON

**Contents**:
- State definitions (idle, walk, run, attack)
- State transitions (idle → walk, walk → run)
- Default starting state
- Transition validation rules

**Naming Convention**: `{entity_name}_animgraph.json` or `{entity_name}_fsm.json`

**Examples**:
- `thesee_animgraph.json` - Thésée FSM
- `player_animgraph.json` - Player FSM
- `enemy_goblin_animgraph.json` - Goblin FSM

**Optional**: Not required if you don't need FSM state control.

**Documentation**: See [Animation Graphs Reference](../../website/docs/user-guide/animation-system/animation-graphs.md)

## Quick Start

### 1. Create Animation Bank

Create `AnimationBanks/my_character_animations.json`:

```json
{
  "id": "my_character",
  "spritesheetPath": "Gamedata/Sprites/Characters/my_character.png",
  "frameWidth": 64,
  "frameHeight": 64,
  "columns": 8,
  "spacing": 0,
  "margin": 0,
  "animations": [
    {
      "name": "idle",
      "loop": true,
      "speed": 1.0,
      "frameRange": {
        "start": 0,
        "end": 7,
        "frameDuration": 0.1
      }
    }
  ]
}
```

### 2. (Optional) Create Animation Graph

Create `AnimationGraphs/my_character_animgraph.json`:

```json
{
  "id": "my_character_fsm",
  "defaultState": "idle",
  "states": [
    {
      "name": "idle",
      "animation": "idle",
      "transitions": [
        { "to": "walk" }
      ]
    },
    {
      "name": "walk",
      "animation": "walk",
      "transitions": [
        { "to": "idle" }
      ]
    }
  ]
}
```

### 3. Load at Startup

In your game initialization code:

```cpp
#include "Animation/AnimationManager.h"

// Load all banks
AnimationManager::Get().LoadAnimationBanksFromDirectory(
    "Gamedata/Animations/AnimationBanks/"
);

// Load all graphs (optional)
AnimationManager::Get().LoadAnimationGraphsFromDirectory(
    "Gamedata/Animations/AnimationGraphs/"
);
```

### 4. Use in Entity Prefab

In `Gamedata/Blueprints/Characters/my_character.json`:

```json
{
  "version": "1.0",
  "name": "My Character",
  "components": {
    "VisualSprite_data": {
      "texturePath": "Gamedata/Sprites/Characters/my_character.png",
      "srcRect": { "x": 0, "y": 0, "w": 64, "h": 64 },
      "dstRect": { "x": 0, "y": 0, "w": 64, "h": 64 },
      "drawLayer": 5,
      "visible": true
    },
    "VisualAnimation_data": {
      "bankId": "my_character",
      "graphId": "my_character_fsm",
      "currentAnimName": "idle",
      "isPlaying": true,
      "autoStart": true
    }
  }
}
```

## Best Practices

### Organization

1. **One bank per character/entity type**
   - Keep related animations together
   - Easier to maintain and debug

2. **Clear naming conventions**
   - Use descriptive names: `hero_animations.json`, not `anim1.json`
   - Match bank ID to filename: `"id": "hero"` in `hero_animations.json`

3. **Separate concerns**
   - AnimationBanks: Define "what" (frames, timing)
   - AnimationGraphs: Define "when" (state transitions)

### Performance

1. **Batch load at startup**
   - Load all banks/graphs before creating entities
   - Avoid loading during gameplay (causes frame drops)

2. **Reuse banks across entities**
   - Multiple entities can reference the same bank
   - No memory duplication (shared data)

3. **Optimize spritesheets**
   - Use texture atlases
   - Keep under 2048×2048 for compatibility
   - Use power-of-2 dimensions when possible

### Asset Management

1. **Version control**
   - Commit all JSON files to git
   - Use meaningful commit messages for animation changes

2. **Documentation**
   - Add comments in JSON (if parser supports)
   - Document special animations in README files

3. **Testing**
   - Verify animations load without errors at startup
   - Test all state transitions in graphs
   - Check frame counts match spritesheet layout

## Common Patterns

### Standard Character Animations

Most characters need these animations:

```
- idle      (looping, slow)
- walk      (looping, medium)
- run       (looping, fast)
- jump      (one-shot, transitions to fall)
- fall      (looping while in air)
- land      (one-shot, transitions to idle)
- attack    (one-shot, transitions to idle)
- hurt      (one-shot, transitions to idle)
- death     (one-shot, stays on last frame)
```

### Enemy Animations

Enemies typically need:

```
- idle      (looping)
- patrol    (looping)
- alert     (one-shot, transitions to chase)
- chase     (looping)
- attack    (one-shot, transitions to idle)
- hurt      (one-shot, transitions to idle)
- death     (one-shot, stays on last frame)
```

### Boss Animations

Bosses may have multiple phases:

```
Phase 1:
- idle_phase1
- attack1_phase1
- attack2_phase1

Phase 2:
- transform (one-shot, transitions to idle_phase2)
- idle_phase2
- attack1_phase2
- attack2_phase2
- special_attack_phase2
```

## Troubleshooting

### Animations Don't Load

**Symptoms**: Console error "Failed to load animation bank"

**Solutions**:
1. Check file path is correct (relative to executable)
2. Validate JSON syntax (use online JSON validator)
3. Ensure all required fields are present

### Wrong Frames Display

**Symptoms**: Animation shows wrong part of spritesheet

**Solutions**:
1. Verify `frameWidth`, `frameHeight` match actual frame size
2. Count `columns` manually in image editor
3. Check `spacing` and `margin` values

### Animation Too Fast/Slow

**Symptoms**: Animation plays at wrong speed

**Solutions**:
1. Adjust `frameDuration` in bank JSON (0.05 to 0.15 typical)
2. Check `playbackSpeed` in component (should be 1.0)
3. Verify frame rate is consistent (60 FPS target)

## Documentation Links

- [Animation System Overview](../../website/docs/user-guide/animation-system/animation-system-overview.md)
- [Quick Start Guide](../../website/docs/user-guide/animation-system/animation-system-quick-start.md)
- [Animation Banks Reference](../../website/docs/user-guide/animation-system/animation-banks.md)
- [Animation Graphs Reference](../../website/docs/user-guide/animation-system/animation-graphs.md)
- [Examples Gallery](../../website/docs/user-guide/animation-system/examples.md)
- [Troubleshooting Guide](../../website/docs/user-guide/animation-system/troubleshooting.md)
- [Technical Reference](../../website/docs/technical-reference/animation/animation-system.md)
- [API Reference](../../website/docs/technical-reference/animation/api-reference.md)

## Support

For questions or issues:
- Check the [Troubleshooting Guide](../../website/docs/user-guide/animation-system/troubleshooting.md)
- Visit the [Olympe Engine Documentation](https://atlasbruce.github.io/Olympe-Engine/)
- Open an issue on [GitHub](https://github.com/Atlasbruce/Olympe-Engine/issues)
- Ask in the Discord community

---

**Last Updated**: 2026-02-15
**Animation System Version**: 1.0
