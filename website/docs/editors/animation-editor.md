---
id: animation-editor
title: Animation Editor
sidebar_label: Animation Editor
---

# Animation Editor

## Overview

The Animation Editor is a visual tool for creating and editing sprite-based animations in the Olympe Engine. It provides an intuitive interface for managing animation banks with multi-spritesheet support, frame-based sequencing, and real-time preview.

**Hotkey**: Press **F9** to open the Animation Editor window.

## Quick Start

### Creating Your First Animation

1. Press **F9** to open the Animation Editor
2. Click **File → New** to create a new animation bank
3. Fill in bank metadata (ID, name, description)
4. Add a spritesheet (see below)
5. Create animation sequences from spritesheet frames
6. Preview and save

## Managing Spritesheets

### Adding a Spritesheet

1. Go to **Spritesheet** tab
2. Click **Add Spritesheet**
3. Configure properties:
   - **ID**: Unique identifier (e.g., "player_idle")
   - **Path**: Relative path (e.g., "Gamedata/Sprites/player_idle.png")
   - **Grid Layout**: Frame dimensions (width, height, columns, rows)
   - **Spacing**: Pixels between frames
   - **Margin**: Pixels around edges
   - **Hotspot**: Pivot point (0.0-1.0, e.g., 0.5, 0.9 for bottom-center)

### Auto-Detect Grid

For uniform grids:
1. Click **Auto-Detect Grid** button
2. Editor analyzes image and detects frame boundaries
3. Review detected values and adjust if needed

**Best for**: Uniform grids with clear frame boundaries

### Viewing Spritesheet

The viewer displays:
- **Grid Overlay**: Toggle with "Show Grid" checkbox
- **Frame Numbers**: 0-based indices
- **Zoom**: Mouse wheel or +/- buttons
- **Pan**: Middle mouse drag

## Creating Animations

### Adding a Sequence

1. Go to **Sequence** tab
2. Click **Add Sequence**
3. Enter unique name (e.g., "walk", "attack")
4. Select spritesheet from dropdown
5. Configure frame range and timing

### Frame Range Configuration

**Start Frame**: First frame index (0-based)
**Frame Count**: Number of frames in sequence

*Example*: Walk cycle using frames 10-19
- Start Frame: 10
- Frame Count: 10

### Timing Settings

**Frame Duration**: Time per frame in seconds
- Typical: 0.08 to 0.15 seconds
- Lower = faster animation

**Speed Multiplier**: Additional speed control
- 1.0 = normal
- 0.5 = half speed
- 2.0 = double speed

**Effective FPS** = 1 / (frameDuration × speed)

### Loop Settings

**Loop**: Enable for continuous animations (idle, walk)
**Next Animation**: Auto-transition target (e.g., attack → idle)

### Animation Events

Add frame-triggered events:
1. Click **Add Event** in sequence properties
2. Set **Frame** number
3. Choose **Event Type**:
   - **sound**: Play audio (footsteps, weapon sounds)
   - **hitbox**: Activate attack hitbox
   - **vfx**: Spawn particle effects
   - **gamelogic**: Custom triggers
4. Enter event data as JSON

**Example - Footstep**:
```json
{
  "type": "sound",
  "frame": 3,
  "dataJson": "{\"soundId\": \"footstep_left\", \"volume\": 0.8}"
}
```

## Preview System

### Playback Controls

- **Play/Pause** (Space): Start/pause playback
- **Stop**: Reset to first frame
- **Frame Scrubber**: Manual frame selection

### Speed Control

Slider adjusts playback speed (0.1x - 3.0x)
- 1.0x = Normal
- Useful for inspecting fast animations

### Preview Display

Shows:
- Current frame at actual size
- Hotspot indicator (crosshair)
- Frame counter (e.g., "3 / 10")
- Elapsed time

### Preview Options

- **Loop**: Toggle looping
- **Show Full Spritesheet**: View entire spritesheet with frame highlighted
- **Show Hotspot**: Toggle hotspot visualization

## Saving & Exporting

### Save Operations

**Save** (Ctrl+S):
- Saves to current file
- Overwrites existing
- Marks as "clean" (no unsaved changes)

**Save As** (Ctrl+Shift+S):
- Prompts for new file location
- Creates new copy
- Updates current path

### File Format

Saves in **JSON Schema v2** format:

```json
{
  "schema_version": 2,
  "type": "animation_bank",
  "bankId": "player_animations",
  "metadata": {
    "name": "Player Animations",
    "description": "Main character animations",
    "author": "Artist Name",
    "version": "1.0.0"
  },
  "spritesheets": [
    {
      "id": "player_idle",
      "path": "Gamedata/Sprites/player_idle.png",
      "frameWidth": 64,
      "frameHeight": 64,
      "columns": 8,
      "rows": 1,
      "totalFrames": 8,
      "hotspot": {"x": 0.5, "y": 0.9}
    }
  ],
  "sequences": [
    {
      "name": "idle",
      "spritesheetId": "player_idle",
      "startFrame": 0,
      "frameCount": 8,
      "frameDuration": 0.1,
      "loop": true,
      "speed": 1.0
    }
  ]
}
```

### Recommended File Organization

```
Gamedata/
├── Sprites/
│   ├── player_idle.png
│   ├── player_walk.png
│   └── enemy_slime.png
└── Animations/
    ├── player_animations.json
    └── enemy_slime.json
```

## Animation Graph Integration

Animation banks work with Animation Graphs for state-based control.

### Simple State Machine

```cpp
AnimationGraph graph;
graph.SetDefaultState("idle");
graph.AddState("idle", "idle");
graph.AddState("walk", "walk");
```

### Advanced State Machine

Create parameter-driven FSM with JSON graph files:

```json
{
  "graphName": "PlayerAnimations",
  "animationBankPath": "Gamedata/Animations/player_animations.json",
  "defaultState": "idle",
  "parameters": {
    "speed": 0.0,
    "isAttacking": false
  },
  "states": [
    {"name": "idle", "animationName": "idle"},
    {"name": "walk", "animationName": "walk"}
  ],
  "transitions": [
    {
      "fromState": "idle",
      "toState": "walk",
      "conditions": [
        {"parameter": "speed", "op": "Greater", "value": 0.1}
      ]
    }
  ]
}
```

## Keyboard Shortcuts

### File Operations
- `Ctrl+N` - New bank
- `Ctrl+O` - Open bank
- `Ctrl+S` - Save
- `Ctrl+Shift+S` - Save As
- `Ctrl+W` - Close

### Preview Controls
- `Space` - Play/Pause
- `Left/Right Arrow` - Previous/Next frame
- `Home/End` - First/Last frame
- `R` - Reset preview

### View Controls
- `Ctrl++` - Zoom in
- `Ctrl+-` - Zoom out
- `Ctrl+0` - Reset zoom
- `Ctrl+G` - Toggle grid
- `F9` - Toggle editor

## Troubleshooting

### Spritesheet Not Loading
**Problem**: Image doesn't display

**Solutions**:
- Verify path is relative to `Gamedata/`
- Check file extension (PNG, JPG supported)
- Ensure file exists
- Use forward slashes (/) in paths

### Animation Plays Incorrectly
**Problem**: Too fast/slow or wrong frames

**Solutions**:
- Check frame duration (typical: 0.08-0.15s)
- Verify speed multiplier is 1.0
- Confirm start frame and frame count
- Check spritesheet columns/rows match image

### Grid Auto-Detect Fails
**Problem**: Incorrect frame detection

**Solutions**:
- Manually set dimensions for non-uniform grids
- Verify spacing and margin values
- Check for clear frame boundaries
- Ensure no compression artifacts

### Preview Shows Black Screen
**Problem**: Preview window is black

**Solutions**:
- Verify spritesheet loaded successfully
- Check sequence references valid spritesheet
- Ensure frame index within range
- Try reloading (Close and Open)

## Best Practices

### Organization
- Use lowercase with underscores: "player_walk"
- Group related animations in one bank
- Use descriptive spritesheet IDs: "player_combat"

### Performance
- Keep spritesheets under 2048×2048
- Aim for 6-12 frames per animation
- Reuse spritesheets across sequences
- Use PNG with appropriate compression

### Animation Quality
- **Walk**: 0.08-0.10s per frame
- **Run**: 0.06-0.08s per frame
- **Idle**: 0.12-0.15s per frame
- Ensure smooth loop points
- Use consistent hotspots

### Workflow
1. Start with idle animation
2. Use preview frequently
3. Iterate, don't aim for perfection
4. Version control (Save As with versions)
5. Backup before major changes

## Related Documentation

- [Animation Editor Architecture](/technical-reference/animation/animation-editor-architecture) - Technical details
- [Animation System](/technical-reference/animation/animation-system) - Complete system reference
- [Animation Graphs](/user-guide/animation-system/animation-graphs) - State machine usage

## Support

- [GitHub Issues](https://github.com/Atlasbruce/Olympe-Engine/issues)
- [Documentation Site](https://atlasbruce.github.io/Olympe-Engine/)
