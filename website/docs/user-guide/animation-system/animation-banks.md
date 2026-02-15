---
id: animation-banks
title: "Animation Banks Reference"
sidebar_label: "Animation Banks"
---

# Animation Banks Reference

Animation banks are JSON files that define sprite animations, frame sequences, and playback properties. This reference covers the complete JSON format and best practices.

## File Structure Overview

```json
{
  "id": "string",
  "spritesheetPath": "string",
  "frameWidth": number,
  "frameHeight": number,
  "columns": number,
  "spacing": number,
  "margin": number,
  "animations": [
    {
      "name": "string",
      "loop": boolean,
      "speed": number,
      "nextAnimation": "string",
      "frameRange": { ... } | "frames": [ ... ]
    }
  ]
}
```

## Top-Level Properties

### id (required)

**Type**: `string`  
**Purpose**: Unique identifier for this animation bank

Used to reference the bank in:
- `VisualAnimation_data.bankId` component field
- `AnimationManager.GetBank(id)` API calls
- Debug logs and error messages

**Rules**:
- Must be unique across all loaded banks
- Case-sensitive
- Use alphanumeric characters and underscores
- Should match the filename (convention)

**Examples**:
```json
"id": "player"        // Good: Simple and clear
"id": "enemy_goblin"  // Good: Descriptive with underscore
"id": "boss_dragon_phase1"  // Good: Specific variant
"id": "Player 1"      // Bad: Contains space
"id": "123"           // Bad: Numeric only (use "enemy_123")
```

### spritesheetPath (required)

**Type**: `string`  
**Purpose**: Path to the PNG spritesheet file

Path is relative to the **executable location**, not the JSON file.

**Best Practices**:
- Use forward slashes `/` (cross-platform compatible)
- Organize spritesheets in `Gamedata/Sprites/` directory
- Use descriptive filenames: `character_idle_walk.png`
- Keep spritesheets small (under 2048×2048 for compatibility)

**Examples**:
```json
"spritesheetPath": "Gamedata/Sprites/Characters/hero.png"
"spritesheetPath": "Gamedata/Sprites/Enemies/zombie_animations.png"
"spritesheetPath": "../Assets/player_spritesheet.png"  // Relative path
```

**Common Errors**:
```json
"spritesheetPath": "C:\\Gamedata\\Sprites\\hero.png"  // Bad: Absolute path
"spritesheetPath": "Sprites/hero.png"  // May fail if Gamedata is expected
```

### frameWidth, frameHeight (required)

**Type**: `number` (pixels)  
**Purpose**: Dimensions of each frame in the spritesheet

All frames in the spritesheet must have the **same dimensions**. If you need variable-sized frames, use multiple banks.

**How to Measure**:
1. Open spritesheet in an image editor
2. Measure a single frame's width and height
3. Verify all frames are the same size

**Examples**:
```json
"frameWidth": 64,
"frameHeight": 64    // 64×64 pixel frames (common for 2D games)

"frameWidth": 32,
"frameHeight": 48    // 32×48 pixel frames (tall characters)

"frameWidth": 128,
"frameHeight": 128   // 128×128 pixel frames (high-res)
```

### columns (required)

**Type**: `number`  
**Purpose**: Number of frames per row in the spritesheet

**Calculation**:
```
columns = floor(spritesheetWidth / frameWidth)
```

Or manually count the frames in the first row.

**Examples**:

```
Spritesheet: 512×256, Frame: 64×64
columns = 512 / 64 = 8
```

```json
"columns": 8  // 8 frames per row
```

### spacing (optional, default: 0)

**Type**: `number` (pixels)  
**Purpose**: Gap between frames horizontally and vertically

Use spacing when your spritesheet has padding between frames.

**Visualization**:
```
No Spacing (spacing: 0)
┌──┬──┬──┐
│ 1│ 2│ 3│
├──┼──┼──┤
│ 4│ 5│ 6│
└──┴──┴──┘

With Spacing (spacing: 2)
┌──┐ ┌──┐ ┌──┐
│ 1│ │ 2│ │ 3│
└──┘ └──┘ └──┘
┌──┐ ┌──┐ ┌──┐
│ 4│ │ 5│ │ 6│
└──┘ └──┘ └──┘
```

**Examples**:
```json
"spacing": 0    // Frames are tightly packed
"spacing": 2    // 2 pixels between frames
"spacing": 4    // 4 pixels between frames
```

### margin (optional, default: 0)

**Type**: `number` (pixels)  
**Purpose**: Border pixels around the entire grid

Use margin when your spritesheet has padding around the edges.

**Visualization**:
```
No Margin (margin: 0)
┌──┬──┬──┐
│ 1│ 2│ 3│
├──┼──┼──┤
│ 4│ 5│ 6│
└──┴──┴──┘

With Margin (margin: 4)
┌─────────┐
│ ┌──┬──┐ │
│ │ 1│ 2│ │
│ ├──┼──┤ │
│ │ 3│ 4│ │
│ └──┴──┘ │
└─────────┘
```

**Examples**:
```json
"margin": 0     // No border
"margin": 4     // 4-pixel border around grid
"margin": 8     // 8-pixel border
```

## Animation Properties

Each animation in the `animations` array has these properties:

### name (required)

**Type**: `string`  
**Purpose**: Unique identifier for this animation within the bank

Used to:
- Play animations: `AnimationSystem::PlayAnimation(entity, "walk")`
- Reference in animation graphs: `"animation": "walk"`
- Set initial state: `"currentAnimName": "idle"`

**Rules**:
- Must be unique within the bank
- Case-sensitive
- Use alphanumeric and underscores
- Should be descriptive

**Common Names**:
```json
"name": "idle"       // Standing still
"name": "walk"       // Walking
"name": "run"        // Running
"name": "jump"       // Jumping
"name": "fall"       // Falling
"name": "attack"     // Generic attack
"name": "attack1"    // First attack in combo
"name": "death"      // Death animation
"name": "hit"        // Taking damage
"name": "cast"       // Casting spell
```

### loop (required)

**Type**: `boolean`  
**Purpose**: Whether the animation repeats

- `true`: Animation loops forever (idle, walk, run)
- `false`: Animation plays once and stops (attack, death, jump)

**Examples**:
```json
"loop": true   // Looping animations
"loop": false  // One-shot animations
```

**Use Cases**:

| Animation Type | Loop Value | Reason |
|---------------|------------|--------|
| idle | `true` | Character constantly breathes/sways |
| walk | `true` | Walking is continuous |
| attack | `false` | Attack completes then stops |
| death | `false` | Character stays dead |
| jump | `false` | Jump has a start and end |
| run | `true` | Running is continuous |

### speed (optional, default: 1.0)

**Type**: `number`  
**Purpose**: Playback speed multiplier

- `1.0`: Normal speed
- `0.5`: Half speed (slow motion)
- `2.0`: Double speed (fast forward)
- `0.0`: Paused (not recommended, use `PauseAnimation()` instead)

**Examples**:
```json
"speed": 1.0    // Normal playback
"speed": 1.5    // 50% faster (energetic run)
"speed": 0.8    // 20% slower (heavy character)
"speed": 2.0    // 2× speed (rapid attack)
```

**Note**: Speed can also be controlled at runtime with `SetPlaybackSpeed()`, which multiplies this value.

### nextAnimation (optional)

**Type**: `string`  
**Purpose**: Animation to auto-transition to when this one completes

Only applies to **non-looping** animations (`loop: false`).

**Examples**:
```json
{
  "name": "attack",
  "loop": false,
  "nextAnimation": "idle"  // Return to idle after attack
}

{
  "name": "jump",
  "loop": false,
  "nextAnimation": "fall"  // Transition to fall after jump
}

{
  "name": "cast",
  "loop": false,
  "nextAnimation": "cast_hold"  // Hold pose after casting
}
```

**Use Cases**:
- Attack → Idle
- Jump → Fall
- Death → (none, character stays on last frame)
- Hurt → Idle

**Validation**:
- `nextAnimation` must reference a valid animation name in the same bank
- Circular chains are allowed: `A → B → C → A`
- Missing references will log warnings

### Frame Definition: frameRange vs frames

Each animation must define frames using **one** of two methods:

#### Method 1: frameRange (Recommended)

Use when frames are **consecutive** in the spritesheet.

**Structure**:
```json
"frameRange": {
  "start": number,
  "end": number,
  "frameDuration": number
}
```

**Properties**:
- **start**: First frame index (inclusive)
- **end**: Last frame index (inclusive)
- **frameDuration**: Duration of each frame in seconds

**Examples**:
```json
"frameRange": {
  "start": 0,
  "end": 7,
  "frameDuration": 0.1
}
// Frames: [0, 1, 2, 3, 4, 5, 6, 7]
// Duration: 0.1s per frame (total 0.8s)

"frameRange": {
  "start": 16,
  "end": 23,
  "frameDuration": 0.08
}
// Frames: [16, 17, 18, 19, 20, 21, 22, 23]
// Duration: 0.08s per frame (total 0.64s)
```

**Frame Index Calculation**:
```
frameIndex = row × columns + column
```

Example with 8 columns:
- Row 0: Frames 0–7
- Row 1: Frames 8–15
- Row 2: Frames 16–23

#### Method 2: frames (Advanced)

Use when:
- Frames are **non-consecutive**
- You need **per-frame** durations
- You want **frame events** (sounds, effects)
- You need custom **hot spots** (pivot points)

**Structure**:
```json
"frames": [
  {
    "srcRect": { "x": number, "y": number, "w": number, "h": number },
    "duration": number,
    "hotSpot": { "x": number, "y": number },
    "events": ["string", ...]
  }
]
```

**Properties**:
- **srcRect**: Source rectangle in spritesheet (pixels)
- **duration**: Frame duration in seconds
- **hotSpot**: Pivot point offset from top-left
- **events**: Array of event names to trigger

**Examples**:

**Simple explicit frames**:
```json
"frames": [
  {
    "srcRect": { "x": 0, "y": 0, "w": 64, "h": 64 },
    "duration": 0.1
  },
  {
    "srcRect": { "x": 64, "y": 0, "w": 64, "h": 64 },
    "duration": 0.1
  },
  {
    "srcRect": { "x": 128, "y": 0, "w": 64, "h": 64 },
    "duration": 0.1
  }
]
```

**Variable frame durations** (attack animation):
```json
"frames": [
  { "srcRect": { "x": 0, "y": 128, "w": 64, "h": 64 }, "duration": 0.2 },   // Wind-up
  { "srcRect": { "x": 64, "y": 128, "w": 64, "h": 64 }, "duration": 0.05 }, // Strike!
  { "srcRect": { "x": 128, "y": 128, "w": 64, "h": 64 }, "duration": 0.15 } // Recovery
]
```

**Frame events** (footsteps):
```json
"frames": [
  { "srcRect": { "x": 0, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] },
  { "srcRect": { "x": 64, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": ["footstep_left"] },
  { "srcRect": { "x": 128, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] },
  { "srcRect": { "x": 192, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": ["footstep_right"] }
]
```

**Custom hot spots** (weapon pivot):
```json
"frames": [
  {
    "srcRect": { "x": 0, "y": 192, "w": 96, "h": 96 },
    "duration": 0.1,
    "hotSpot": { "x": 48, "y": 80 }  // Pivot at character's hand
  }
]
```

## Multi-Spritesheet Support

Different animations can use different spritesheets by overriding `spritesheetPath` per animation:

```json
{
  "id": "boss",
  "spritesheetPath": "Gamedata/Sprites/Bosses/boss_phase1.png",
  "frameWidth": 128,
  "frameHeight": 128,
  "columns": 6,
  "animations": [
    {
      "name": "idle_phase1",
      "loop": true,
      "frameRange": { "start": 0, "end": 5, "frameDuration": 0.15 }
    },
    {
      "name": "idle_phase2",
      "loop": true,
      "spritesheetPath": "Gamedata/Sprites/Bosses/boss_phase2.png",
      "frameRange": { "start": 0, "end": 5, "frameDuration": 0.15 }
    }
  ]
}
```

This allows one bank to manage animations from multiple PNG files.

## Best Practices

### Organizing Banks

**Option 1: One bank per character**
```
thesee_animations.json     (all Thésée animations)
goblin_animations.json     (all goblin animations)
dragon_animations.json     (all dragon animations)
```

**Option 2: One bank per animation category**
```
player_movement.json       (idle, walk, run, jump)
player_combat.json         (attack1, attack2, block)
player_magic.json          (cast, channel, summon)
```

**Recommendation**: One bank per character for simplicity.

### Common Animation Patterns

**Standard Character Set**:
```json
{
  "id": "character",
  "animations": [
    { "name": "idle", "loop": true, "frameRange": { ... } },
    { "name": "walk", "loop": true, "frameRange": { ... } },
    { "name": "run", "loop": true, "frameRange": { ... } },
    { "name": "jump", "loop": false, "nextAnimation": "fall", "frameRange": { ... } },
    { "name": "fall", "loop": true, "frameRange": { ... } },
    { "name": "land", "loop": false, "nextAnimation": "idle", "frameRange": { ... } },
    { "name": "attack", "loop": false, "nextAnimation": "idle", "frameRange": { ... } },
    { "name": "hurt", "loop": false, "nextAnimation": "idle", "frameRange": { ... } },
    { "name": "death", "loop": false, "frameRange": { ... } }
  ]
}
```

**Enemy Set**:
```json
{
  "id": "enemy",
  "animations": [
    { "name": "idle", "loop": true, "frameRange": { ... } },
    { "name": "patrol", "loop": true, "frameRange": { ... } },
    { "name": "alert", "loop": false, "nextAnimation": "chase", "frameRange": { ... } },
    { "name": "chase", "loop": true, "frameRange": { ... } },
    { "name": "attack", "loop": false, "nextAnimation": "idle", "frameRange": { ... } },
    { "name": "hurt", "loop": false, "nextAnimation": "idle", "frameRange": { ... } },
    { "name": "death", "loop": false, "frameRange": { ... } }
  ]
}
```

## Complete Example: Thésée Animations

```json
{
  "id": "thesee",
  "spritesheetPath": "Gamedata/Sprites/Characters/thesee_spritesheet.png",
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
    },
    {
      "name": "walk",
      "loop": true,
      "speed": 1.0,
      "frameRange": {
        "start": 8,
        "end": 15,
        "frameDuration": 0.08
      }
    },
    {
      "name": "run",
      "loop": true,
      "speed": 1.0,
      "frameRange": {
        "start": 16,
        "end": 23,
        "frameDuration": 0.06
      }
    },
    {
      "name": "hit",
      "loop": false,
      "speed": 1.0,
      "nextAnimation": "idle",
      "frameRange": {
        "start": 24,
        "end": 27,
        "frameDuration": 0.1
      }
    }
  ]
}
```

## Validation and Error Checking

The `AnimationManager` validates banks on load:

### Common Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| "Bank ID 'X' already loaded" | Duplicate bank ID | Use unique IDs |
| "Spritesheet not found: X" | File doesn't exist | Check path relative to executable |
| "Invalid frameRange: start > end" | start=10, end=5 | Swap start and end values |
| "Animation 'X' has no frames" | Missing frameRange/frames | Add frame definition |
| "nextAnimation 'X' not found" | Invalid reference | Check spelling, add animation |
| "columns must be > 0" | columns=0 or missing | Set correct column count |

### Debug Loading

Enable verbose logging:
```cpp
AnimationManager::Get().SetVerbose(true);
AnimationManager::Get().LoadAnimationBank("...");
```

Output:
```
[AnimationManager] Loading bank from 'thesee_animations.json'
[AnimationManager]   ID: thesee
[AnimationManager]   Spritesheet: 512×256 (8 columns)
[AnimationManager]   Animation 'idle': 8 frames (0.8s loop)
[AnimationManager]   Animation 'walk': 8 frames (0.64s loop)
[AnimationManager]   Animation 'run': 8 frames (0.48s loop)
[AnimationManager]   Animation 'hit': 4 frames (0.4s once)
[AnimationManager] Loaded bank 'thesee' successfully
```

## See Also

- [Quick Start Guide](animation-system-quick-start.md) - Create your first animation
- [Animation Graphs Reference](animation-graphs.md) - Control animation flow with FSM
- [Examples Gallery](examples.md) - See practical examples
- [API Reference](../../technical-reference/animation/api-reference.md) - AnimationManager API
