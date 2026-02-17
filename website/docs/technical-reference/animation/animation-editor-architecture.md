---
id: animation-editor-architecture
title: Animation Editor Architecture
sidebar_label: Editor Architecture
---

# Animation Editor - Architecture

## Overview

The Animation Editor (`AnimationEditorWindow`) is a standalone tool for creating and editing AnimationBank files. It renders in a separate SDL3 window following the standalone window pattern.

**Hotkey**: F9

For complete technical details, see [Animation_Editor_Architecture.md](https://github.com/Atlasbruce/Olympe-Engine/blob/main/Docs/Developer/Animation_Editor_Architecture.md) in the repository.

## Core Components

### 1. AnimationEditorWindow
**Location**: `Source/Editor/AnimationEditorWindow.h/cpp`

Main editor class with comprehensive authoring features.

Key members:
```cpp
bool m_isOpen;
AnimationBank m_currentBank;
std::string m_currentFilePath;
bool m_isDirty;                    // Unsaved changes
int m_selectedSpritesheetIndex;
int m_selectedSequenceIndex;
bool m_isPlayingPreview;
float m_previewSpeed;
SDL_Window* m_window;              // Separate window
SDL_Renderer* m_renderer;
ImGuiContext* m_imguiContext;
```

### 2. AnimationBank
**Location**: `Source/Animation/AnimationTypes.h`

Data model for multi-spritesheet animation collection:
```cpp
struct AnimationBank {
    std::string bankId;
    std::string name;
    std::vector<SpritesheetInfo> spritesheets;  // NEW: Multi-spritesheet
    std::vector<AnimationSequence> sequences;   // NEW: Range-based
    
    // DEPRECATED: Old format fields
    std::string spritesheetPath;
    std::unordered_map<std::string, Animation> animations;
};
```

### 3. SpritesheetInfo
Per-spritesheet metadata:
```cpp
struct SpritesheetInfo {
    std::string id;             // Unique identifier
    std::string path;           // Relative path
    int frameWidth, frameHeight;
    int columns, rows;
    int totalFrames;
    int spacing, margin;
    SDL_FPoint hotspot;         // Pivot point (0-1)
};
```

### 4. AnimationSequence
Range-based animation definition (NEW format):
```cpp
struct AnimationSequence {
    std::string name;
    std::string spritesheetId;  // Reference to SpritesheetInfo
    int startFrame;             // Starting frame index
    int frameCount;             // Number of frames
    float frameDuration;        // Time per frame
    bool loop;
    float speed;                // Speed multiplier
    std::string nextAnimation;  // Auto-transition
    std::vector<AnimationEventData> events;
};
```

**DEPRECATED**: Old frame-by-frame format with `AnimationFrame` vector.

### 5. AnimationGraph Systems

**Simple FSM** (`Olympe::AnimationGraph`):
```cpp
struct AnimationGraph {
    std::string graphId;
    std::string defaultState;
    std::unordered_map<std::string, std::string> states;
    bool CanTransition(const std::string& from, const std::string& to) const;
};
```

**Advanced FSM** (`OlympeAnimation::AnimationGraph`):
- Parameter-based conditions (`Condition`, `ParameterValue`, `ComparisonOperator`)
- State transitions (`Transition`, `TransitionType`)
- Animation states (`AnimationState`, `BlendMode`)
- Parameter management (bool, float, int, string)
- Events (`AnimationEventData`)

## UI Panel Layout

3-column ImGui docking layout:

### Main Menu Bar
File operations: New, Open, Save, Save As, Import, Export, Close

### Left Panel: Bank List
- Directory scanning (`Gamedata/Animations/`)
- Bank preview (ID, name, spritesheet count)
- Quick load (double-click)
- Search/filter

### Middle Panel: Spritesheet/Sequence Tabs

**Spritesheet Tab**:
- Add/Remove spritesheets
- Properties editor (ID, path, grid layout, hotspot)
- Auto-detect grid button
- Image viewer (zoom, pan, grid overlay)

**Sequence Tab**:
- Add/Remove sequences
- Properties editor (name, spritesheet ref, frame range, timing)
- Event editor (sounds, hitboxes, VFX)
- Visual timeline

### Right Panel: Preview
- Playback controls (play/pause/stop/reset)
- Speed slider (0.1x - 3.0x)
- Frame scrubber
- Loop toggle
- Preview window (current frame with hotspot)

### Bottom Panel: Properties
Bank metadata: ID, name, description, author, version, tags

## Data Format

### Unified Schema v2 JSON
```json
{
  "schema_version": 2,
  "type": "animation_bank",
  "bankId": "player_animations",
  "metadata": {
    "name": "Player Animations",
    "description": "Main character",
    "author": "Artist Name",
    "version": "1.0.0",
    "tags": ["player", "character"]
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
      "spacing": 0,
      "margin": 0,
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
      "speed": 1.0,
      "nextAnimation": "",
      "events": []
    }
  ]
}
```

## Standalone Window Pattern

Same pattern as BT Debugger:

### Lifecycle
1. **Creation**: `CreateSeparateWindow()` - SDL_Window (1280x720), SDL_Renderer, ImGuiContext
2. **Rendering**: `RenderInSeparateWindow()` - Switch context, render UI, present
3. **Destruction**: `DestroySeparateWindow()` - Shutdown ImGui, destroy SDL resources

### Event Handling
`ProcessEvent(SDL_Event*)` for:
- Window close (SDL_EVENT_WINDOW_CLOSE_REQUESTED)
- Keyboard shortcuts (Ctrl+S, Space, arrows)
- Mouse events (zoom, pan, selection)

Events routed before BT Debugger in `OlympeEngine.cpp` (line 137-143).

## Integration

### AnimationManager
```cpp
class AnimationManager {
    bool LoadAnimationBank(const std::string& bankId, const std::string& path);
    AnimationBank* GetAnimationBank(const std::string& bankId);
    void PlayAnimation(EntityID entity, const std::string& animName);
};
```

### ECS Animation System
`Source/ECS_Systems_Animation.cpp`:
```cpp
void UpdateEntity(EntityID id, float dt) {
    if (sequence->spritesheetId.empty()) {
        // Old format: frame-by-frame
        UpdateFrameByFrame(id, sequence, dt);
    } else {
        // New format: spritesheet + range
        UpdateRangeBased(id, sequence, dt);
    }
}
```

## File Operations

- **NewBank()**: Empty bank with auto-generated ID
- **OpenBank()**: Load JSON with schema validation
- **SaveBank()**: Serialize to Schema v2 JSON
- **SaveBankAs()**: Prompt for new path
- **ImportBankJSON()**: Merge from external JSON
- **ExportBankJSON()**: Export to specified path
- **ScanBankDirectory()**: Recursive scan for banks

## Animation Event Types

- **sound**: Play audio on frame
- **hitbox**: Activate attack hitbox
- **vfx**: Spawn particle effects
- **gamelogic**: Custom game logic trigger

Event data stored as JSON string in `AnimationEventData`.

## File Locations

- `Source/Editor/AnimationEditorWindow.h/cpp` - Main editor
- `Source/Animation/AnimationTypes.h` - Data structures
- `Source/Animation/AnimationGraph.h` - State machines
- `Source/Animation/AnimationManager.h/cpp` - Bank loading
- `Source/ECS_Systems_Animation.h/cpp` - ECS integration
- `Source/OlympeEngine.cpp` - Main loop (F9 hotkey)

## Related Documentation

- [Animation Editor User Guide](/editors/animation-editor) - User documentation
- [Animation System](/technical-reference/animation/animation-system) - Complete reference
- [Animation Graphs](/user-guide/animation-system/animation-graphs) - State machines

## See Also

- Complete architecture document: `Docs/Developer/Animation_Editor_Architecture.md`
- User guide: `Docs/Animation_Editor_User_Guide.md`
- Source code: `Source/Editor/AnimationEditorWindow.cpp`
