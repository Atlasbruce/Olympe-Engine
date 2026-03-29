# Animation Editor - User Guide

> **📖 Latest Documentation**: For the most up-to-date version of this guide, visit the [official Olympe Engine documentation site](https://atlasbruce.github.io/Olympe-Engine/editors/animation-editor).

## Overview

The Animation Editor is a powerful tool for creating and editing sprite-based animations in the Olympe Engine. It provides an intuitive visual interface for managing animation banks with multi-spritesheet support, frame-based animation sequencing, and real-time preview.

## Getting Started

### Opening the Editor

1. Launch the Olympe Engine
2. Press **F9** to open the Animation Editor window
3. The editor opens in a separate window with its own interface

### Creating Your First Animation Bank

1. Click **File → New** in the menu bar
2. Enter a unique Bank ID (e.g., "player_animations")
3. Fill in the bank metadata:
   - **Name**: Display name (e.g., "Player Character Animations")
   - **Description**: Brief description of the bank's purpose
   - **Author**: Your name or team name
   - **Version**: Version number (e.g., "1.0.0")
4. Click **Create** to create the empty bank

## Managing Spritesheets

### Adding a Spritesheet

1. Navigate to the **Spritesheet** tab in the middle panel
2. Click the **Add Spritesheet** button
3. Fill in the spritesheet properties:
   - **ID**: Unique identifier (e.g., "player_idle")
   - **Path**: Relative path to the image (e.g., "Gamedata/Sprites/player_idle.png")
   - **Description**: Brief description

### Configuring Frame Dimensions

#### Manual Configuration

Set the grid layout manually:
- **Frame Width**: Width of each frame in pixels
- **Frame Height**: Height of each frame in pixels
- **Columns**: Number of frames horizontally
- **Rows**: Number of frames vertically
- **Spacing**: Pixels between frames (default: 0)
- **Margin**: Pixels around spritesheet edges (default: 0)

#### Auto-Detect Grid

For uniform grids, use auto-detection:
1. Click the **Auto-Detect Grid** button
2. The editor analyzes the image and detects frame boundaries
3. Review the detected dimensions in the properties panel
4. Adjust manually if needed

**Note**: Auto-detection works best with:
- Uniform grid spacing
- Clear frame boundaries
- Consistent frame sizes

### Setting the Hotspot

The hotspot (pivot point) determines where the sprite is anchored:

1. In the spritesheet properties, locate **Hotspot**
2. Set X and Y values (0.0 to 1.0):
   - `(0.5, 0.5)` - Center of the sprite
   - `(0.5, 1.0)` - Bottom-center (common for characters)
   - `(0.0, 0.0)` - Top-left corner
3. The hotspot is visualized as a crosshair in the preview

### Viewing Spritesheet

The spritesheet viewer shows the loaded image with:
- **Grid Overlay**: Toggle with the **Show Grid** checkbox
- **Frame Numbers**: Displays frame indices (0-based)
- **Zoom Controls**: Use mouse wheel or +/- buttons
- **Pan**: Click and drag with middle mouse button

### Removing a Spritesheet

1. Select the spritesheet from the list
2. Click the **Remove** button
3. Confirm deletion (this doesn't delete the image file, only removes from bank)

**Warning**: Removing a spritesheet will invalidate any sequences that reference it.

## Creating Animations

### Adding a Sequence

1. Navigate to the **Sequence** tab
2. Click the **Add Sequence** button
3. Enter a unique sequence name (e.g., "idle", "walk", "attack")
4. Select the spritesheet from the dropdown
5. Configure the frame range and playback settings

### Defining Frame Ranges

Specify which frames to use from the spritesheet:

- **Start Frame**: First frame index (0-based)
- **Frame Count**: Number of frames in the sequence

**Example**: For a walk cycle using frames 10-19 of a spritesheet:
- Start Frame: 10
- Frame Count: 10

### Setting Timing

Control animation playback speed:

- **Frame Duration**: Time each frame displays (seconds)
  - Typical values: 0.08 to 0.15 seconds
  - Lower values = faster animation
- **Speed Multiplier**: Additional speed control (default: 1.0)
  - 0.5 = half speed
  - 2.0 = double speed

**Effective Frame Rate** = 1 / (frameDuration × speed)

### Loop Settings

- **Loop**: Check to repeat animation continuously
- **Next Animation**: Specify animation to play after this one completes (leave empty for none)

**Use Cases**:
- Loop enabled: Idle, walk, run animations
- Loop disabled, Next Animation set: Attack → idle transition
- Loop disabled, Next Animation empty: One-shot animations (death, spawn)

### Adding Animation Events

Events trigger actions on specific frames:

1. In the sequence properties, click **Add Event**
2. Set the **Frame** number where the event should trigger
3. Choose the **Event Type**:
   - **sound**: Play audio (footsteps, weapon sounds)
   - **hitbox**: Activate attack hitbox
   - **vfx**: Spawn visual effect (dust, sparks)
   - **gamelogic**: Custom game logic trigger
4. Enter event-specific data in JSON format

**Example - Footstep Sound**:
```json
{
  "type": "sound",
  "frame": 3,
  "dataJson": "{\"soundId\": \"footstep_left\", \"volume\": 0.8}"
}
```

**Example - Attack Hitbox**:
```json
{
  "type": "hitbox",
  "frame": 5,
  "dataJson": "{\"damage\": 10, \"radius\": 32, \"knockback\": 5}"
}
```

## Preview System

### Playback Controls

The preview panel provides real-time animation playback:

- **Play/Pause** (Space): Start or pause animation
- **Stop**: Reset to first frame
- **Frame Scrubber**: Drag to manually select frame

### Speed Control

Adjust playback speed with the slider:
- Range: 0.1x to 3.0x
- 1.0x = Normal speed
- Useful for inspecting fast animations

### Preview Display

The preview window shows:
- Current frame rendered at actual size
- Hotspot indicator (crosshair)
- Frame border
- Frame counter (e.g., "3 / 10")
- Elapsed time

### Preview Options

- **Loop**: Toggle looping for preview (independent of sequence loop setting)
- **Show Full Spritesheet**: Display entire spritesheet with current frame highlighted
- **Show Hotspot**: Toggle hotspot visualization

## Saving & Exporting

### Save Operations

**Save** (Ctrl+S):
- Saves to the current file path
- Overwrites existing file
- Marks bank as "clean" (no unsaved changes)

**Save As** (Ctrl+Shift+S):
- Prompts for new file name and location
- Creates a new copy
- Updates current file path to new location

### File Format

The editor saves in **JSON Schema v2** format with this structure:

```json
{
  "schema_version": 2,
  "type": "animation_bank",
  "bankId": "unique_id",
  "metadata": {
    "name": "Display Name",
    "description": "Description",
    "author": "Author Name",
    "version": "1.0.0"
  },
  "spritesheets": [
    {
      "id": "spritesheet_id",
      "path": "Gamedata/Sprites/image.png",
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
      "name": "animation_name",
      "spritesheetId": "spritesheet_id",
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

### Import/Export

**Import JSON**:
- Load animation data from external JSON file
- Useful for:
  - Sharing animations between projects
  - Version control workflows
  - Batch editing via scripts

**Export JSON**:
- Export bank to standalone JSON file
- Useful for:
  - Creating backups
  - Sharing with team members
  - Versioning specific animation states

## Animation Graph Integration

Animation banks work with the Animation Graph system for state-based animation control.

### Simple Animation Graph

Define state transitions in code:
```cpp
AnimationGraph graph;
graph.SetDefaultState("idle");
graph.AddState("idle", "idle");      // state name -> animation name
graph.AddState("walk", "walk");
graph.AddState("attack", "attack");
```

### Advanced Animation Graph

Create parameter-driven state machines:

1. Create animation graph JSON file (separate from animation bank)
2. Define states, transitions, and conditions
3. Load graph in code:
   ```cpp
   OlympeAnimation::AnimationGraph graph;
   graph.LoadFromFile("Gamedata/Animations/player_graph.json");
   ```

**Example Graph**:
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
    {
      "name": "idle",
      "animationName": "idle",
      "blendMode": "Override"
    },
    {
      "name": "walk",
      "animationName": "walk",
      "blendMode": "Override"
    }
  ],
  "transitions": [
    {
      "fromState": "idle",
      "toState": "walk",
      "conditions": [
        {"parameter": "speed", "op": "Greater", "value": 0.1}
      ]
    },
    {
      "fromState": "walk",
      "toState": "idle",
      "conditions": [
        {"parameter": "speed", "op": "LessOrEqual", "value": 0.1}
      ]
    }
  ]
}
```

**In Game Code**:
```cpp
// Update parameter to trigger transition
animGraph.SetParameter("speed", playerVelocity.Length());
animGraph.Update(deltaTime);

// Get current animation
std::string currentAnim = animGraph.GetCurrentAnimationName();
```

## Troubleshooting

### Spritesheet Not Loading

**Problem**: Spritesheet path shows as invalid or image doesn't display.

**Solutions**:
1. Verify path is relative to the `Gamedata/` directory
2. Check file extension (PNG and JPG supported)
3. Ensure file exists at the specified location
4. Try using forward slashes (/) in paths

### Animation Plays Incorrectly

**Problem**: Animation is too fast, too slow, or shows wrong frames.

**Solutions**:
1. Check **Frame Duration**: Typical range is 0.08-0.15 seconds
2. Verify **Speed Multiplier** is set to 1.0 (unless intentionally adjusted)
3. Confirm **Start Frame** and **Frame Count** are correct
4. Ensure spritesheet **Columns** and **Rows** match actual image layout

### Grid Auto-Detect Fails

**Problem**: Auto-detect doesn't find frame boundaries correctly.

**Solutions**:
1. Manually set frame dimensions if grid is non-uniform
2. Ensure spacing and margin values are correct
3. Check that spritesheet has clear frame boundaries
4. Verify image doesn't have compression artifacts

### Preview Shows Black Screen

**Problem**: Preview window is black or shows corrupted image.

**Solutions**:
1. Verify spritesheet is loaded successfully
2. Check that selected sequence references valid spritesheet
3. Ensure frame index is within valid range
4. Try reloading the bank (Close and Open)

### Changes Not Saved

**Problem**: Editor shows unsaved changes but Save doesn't work.

**Solutions**:
1. Check file permissions on target directory
2. Verify sufficient disk space
3. Try **Save As** to a different location
4. Check for JSON syntax errors (shouldn't happen with editor, but worth checking)

## Tips & Best Practices

### Organization

1. **Consistent Naming**: Use lowercase with underscores (e.g., "player_walk", not "PlayerWalk")
2. **Logical Grouping**: Group related animations in single bank (e.g., all player animations in one bank)
3. **Spritesheet IDs**: Use descriptive IDs that indicate content (e.g., "player_combat" not "sprites_01")

### Performance

1. **Spritesheet Size**: Keep spritesheets under 2048x2048 for broad compatibility
2. **Frame Count**: Aim for 6-12 frames per animation (smooth enough, not excessive)
3. **Reuse**: Use same spritesheet for multiple sequences when possible
4. **Compression**: Use PNG with appropriate compression (balance quality vs file size)

### Animation Quality

1. **Timing**: Reference real-world motion speeds
   - Walk: ~0.08-0.10 seconds per frame
   - Run: ~0.06-0.08 seconds per frame
   - Idle: ~0.12-0.15 seconds per frame (slower for calm feeling)
2. **Loop Points**: Ensure first and last frames flow smoothly for loops
3. **Hotspot Consistency**: Use same hotspot for related animations to prevent "sliding"
4. **Events**: Place footstep sounds on contact frames for realism

### Workflow

1. **Start Simple**: Create idle animation first to establish baseline
2. **Test Early**: Use preview frequently to catch issues early
3. **Iterate**: Don't aim for perfection on first pass
4. **Version Control**: Save intermediate versions (use "Save As" with version numbers)
5. **Backup**: Keep backup copies of working animations before major changes

## Keyboard Shortcuts

### File Operations
- `Ctrl+N` - New bank
- `Ctrl+O` - Open bank
- `Ctrl+S` - Save
- `Ctrl+Shift+S` - Save As
- `Ctrl+W` - Close editor

### Preview Controls
- `Space` - Play/Pause
- `Left Arrow` - Previous frame
- `Right Arrow` - Next frame
- `Home` - First frame
- `End` - Last frame
- `R` - Reset preview

### View Controls
- `Ctrl++` - Zoom in
- `Ctrl+-` - Zoom out
- `Ctrl+0` - Reset zoom
- `Ctrl+G` - Toggle grid overlay

### Editor
- `F9` - Toggle Animation Editor window
- `Delete` - Remove selected item
- `Escape` - Cancel operation

## Next Steps

### Learning More

- [Animation Editor Architecture](Developer/Animation_Editor_Architecture.md) - Technical details
- [Animation System Documentation](../website/docs/technical-reference/animation/animation-system.md) - Complete system reference
- [Animation Graph Guide](../website/docs/user-guide/animation-system/animation-graphs.md) - State machine usage

### Getting Help

- Check the [official documentation site](https://atlasbruce.github.io/Olympe-Engine/)
- Review example animation banks in `Examples/Animations/`
- Consult the source code: `Source/Editor/AnimationEditorWindow.cpp`

## Changelog

### Version 1.0 (2025)
- Initial release
- Multi-spritesheet support
- Unified Schema v2 format
- Real-time preview
- Grid auto-detection
- Animation events
- Standalone window (F9 hotkey)
