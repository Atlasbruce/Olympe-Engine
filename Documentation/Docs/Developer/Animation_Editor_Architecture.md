# Animation Editor - Technical Architecture

## Overview

The Animation Editor (`AnimationEditorWindow`) is a standalone tool for creating and editing AnimationBank files in the Olympe Engine. It provides a comprehensive visual interface for managing multi-spritesheet animation banks with real-time preview capabilities.

The editor renders in a **separate SDL3 window** with its own ImGui context, following the same standalone window pattern as the BT Debugger. This allows animation artists to work in a dedicated workspace while the game continues running in the main window.

**Hotkey**: Press **F9** to toggle the Animation Editor window.

## Core Architecture

### Main Components

#### 1. AnimationEditorWindow
**Location**: `Source/Editor/AnimationEditorWindow.h/cpp`

The main editor class providing all animation authoring features.

```cpp
class AnimationEditorWindow {
    bool m_isOpen;                  // Window visibility
    
    // Current bank state
    AnimationBank m_currentBank;    // Loaded animation bank
    std::string m_currentFilePath;  // File path for save
    bool m_isDirty;                 // Has unsaved changes
    
    // UI state
    int m_selectedSpritesheetIndex; // Selected spritesheet
    int m_selectedSequenceIndex;    // Selected sequence
    bool m_isPlayingPreview;        // Preview playback state
    float m_previewSpeed;           // Playback speed multiplier
    
    // Standalone window
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    ImGuiContext* m_imguiContext;
    
    // Spritesheet viewer
    float m_viewerZoom;             // Zoom level
    Vector m_viewerPan;             // Pan offset
    bool m_showGrid;                // Grid overlay toggle
};
```

**Key Methods**:
- `Toggle()` - Show/hide window
- `Update(deltaTime)` - Main update loop
- `ProcessEvent(SDL_Event*)` - Handle input events
- `Render()` - Render UI panels
- `UpdatePreview(deltaTime)` - Advance preview animation

#### 2. AnimationBank
**Location**: `Source/Animation/AnimationTypes.h`

Data model for a collection of animations sharing multiple spritesheets.

**Schema v2 Structure**:
```cpp
struct AnimationBank {
    std::string bankId;             // Unique bank identifier
    std::string name;               // Display name
    std::string description;        // Bank description
    
    // Multi-spritesheet support
    std::vector<SpritesheetInfo> spritesheets;
    
    // Animation sequences (new format)
    std::vector<AnimationSequence> sequences;
    
    // DEPRECATED: Old single-spritesheet fields
    std::string spritesheetPath;    // DEPRECATED
    int frameWidth;                 // DEPRECATED
    int frameHeight;                // DEPRECATED
    std::unordered_map<std::string, Animation> animations; // DEPRECATED
};
```

#### 3. SpritesheetInfo
**Location**: `Source/Animation/AnimationTypes.h`

Per-spritesheet metadata within a bank.

```cpp
struct SpritesheetInfo {
    std::string id;                 // Unique ID (e.g., "player_idle")
    std::string path;               // Relative path to image
    std::string description;
    
    // Grid layout
    int frameWidth;                 // Width of each frame (px)
    int frameHeight;                // Height of each frame (px)
    int columns;                    // Number of columns
    int rows;                       // Number of rows
    int totalFrames;                // Total frame count
    int spacing;                    // Spacing between frames (px)
    int margin;                     // Margin around edges (px)
    
    SDL_FPoint hotspot;             // Default pivot point (0-1 normalized)
};
```

**Grid Auto-Detection**:
The editor can automatically detect frame dimensions by analyzing the first row and column of the spritesheet image using pixel comparison.

#### 4. AnimationSequence
**Location**: `Source/Animation/AnimationTypes.h`

Animation definition using frame ranges (new format).

**New Range-Based Format** (Recommended):
```cpp
struct AnimationSequence {
    std::string name;               // Sequence name (e.g., "walk")
    std::string spritesheetId;      // Reference to SpritesheetInfo.id
    
    // Frame range
    int startFrame;                 // Starting frame index
    int frameCount;                 // Number of frames
    
    // Playback
    float frameDuration;            // Duration per frame (seconds)
    bool loop;                      // Loop animation?
    float speed;                    // Speed multiplier
    std::string nextAnimation;      // Auto-transition target
    
    // Events
    std::vector<OlympeAnimation::AnimationEventData> events;
};
```

**DEPRECATED Frame-by-Frame Format**:
```cpp
struct AnimationSequence {
    std::string name;
    std::vector<AnimationFrame> frames;  // DEPRECATED: individual frames
    float speed;
    bool loop;
};

struct AnimationFrame {             // DEPRECATED
    SDL_Rect srcRect;               // Source rectangle on spritesheet
    float duration;                 // Frame duration (seconds)
    SDL_FPoint hotSpot;             // Per-frame hotspot
    std::string eventName;          // Triggered event
};
```

The animation system supports both formats for backward compatibility. The ECS animation system checks `sequence->spritesheetId.empty()` to determine format.

#### 5. AnimationGraph (Simple FSM)
**Location**: `Source/Animation/AnimationGraph.h` (namespace `Olympe`)

Simple finite state machine for basic animation transitions.

```cpp
namespace Olympe {
    struct AnimationGraph {
        std::string graphId;
        std::string defaultState;
        std::unordered_map<std::string, std::string> states; // state -> animation
        
        bool CanTransition(const std::string& from, const std::string& to) const;
    };
}
```

#### 6. OlympeAnimation::AnimationGraph (Advanced FSM)
**Location**: `Source/Animation/AnimationGraph.h` (namespace `OlympeAnimation`)

Advanced state machine with parameter-based conditions and smooth transitions.

**Components**:

**Condition** - Parameter-based condition evaluation:
```cpp
struct Condition {
    std::string parameter;          // Parameter name
    ComparisonOperator op;          // ==, !=, >, >=, <, <=
    ParameterValue value;           // Comparison value
};
```

**Transition** - State transition definition:
```cpp
struct Transition {
    std::string fromState;          // Source state (or "ANY")
    std::string toState;            // Target state
    float transitionTime;           // Blend duration (seconds)
    std::vector<Condition> conditions; // All must be true
};
```

**AnimationState** - State in the graph:
```cpp
struct AnimationState {
    std::string name;               // State name
    std::string animationName;      // Animation to play
    BlendMode blendMode;            // Override, Additive, Blend
    int priority;                   // State priority
};
```

**AnimationGraph Methods**:
```cpp
// Parameter management
void SetParameter(const std::string& name, bool/float/int/string value);
bool/float/int/string GetParameter(...) const;

// State management
void SetCurrentState(const std::string& stateName);
const std::string& GetCurrentState() const;
std::string GetCurrentAnimationName() const;

// Update (checks transitions)
bool Update(float deltaTime);  // Returns true if state changed

// Load from file
bool LoadFromFile(const std::string& filePath);
```

**Animation Types**:
- `BlendMode`: Override (replace), Additive (add), Blend (smooth)
- `TransitionType`: Immediate, Smooth, AfterComplete
- `ParameterType`: Bool, Float, Int, String
- `ComparisonOperator`: Equal, NotEqual, Greater, GreaterOrEqual, Less, LessOrEqual

**Animation Events**:
```cpp
struct AnimationEventData {
    std::string type;               // "sound", "hitbox", "vfx", "gamelogic"
    int frame;                      // Frame number to trigger
    std::string dataJson;           // JSON event data
};
```

Common event types:
- **sound**: Play audio on specific frame
- **hitbox**: Activate attack hitbox
- **vfx**: Spawn visual effect
- **gamelogic**: Trigger custom game logic

## Editor Features

### UI Panel Layout

The editor uses a 3-column ImGui layout with dockable panels:

#### Main Menu Bar
**Method**: `RenderMainMenu()`

File operations:
- **New**: Create new animation bank
- **Open**: Load existing bank from JSON
- **Save** (Ctrl+S): Save to current file
- **Save As** (Ctrl+Shift+S): Save to new file
- **Import JSON**: Import bank from JSON
- **Export JSON**: Export bank to JSON
- **Close**: Close editor window

View options:
- Toggle panels visibility
- Reset layout
- Zoom controls

Help:
- User guide
- Keyboard shortcuts
- About

#### Left Panel: Bank List
**Method**: `RenderBankListPanel()`

Features:
- **Directory Scanning**: Scans `Gamedata/Animations/` for `.json` banks
- **Bank Preview**: Shows bank metadata (ID, name, spritesheet count)
- **Quick Load**: Double-click to load bank
- **Search/Filter**: Filter by name or ID
- **Create New**: Button to create new bank

#### Middle Panel: Spritesheet/Sequence Tabs
**Method**: `RenderSpritesheetPanel()`, `RenderSequencePanel()`

**Spritesheet Tab**:
- **List View**: Shows all spritesheets in bank
- **Add/Remove**: Buttons to manage spritesheets
- **Properties**:
  - ID (unique identifier)
  - Path (relative to Gamedata)
  - Description
  - Grid Layout: Frame dimensions, columns, rows
  - Spacing & Margin
  - Hotspot (pivot point)
- **Auto-Detect Grid**: Analyzes image to detect frame dimensions
- **Image Viewer**: 
  - Zoom/Pan controls
  - Grid overlay toggle
  - Frame index overlay
  - Hotspot visualization

**Sequence Tab**:
- **List View**: Shows all animation sequences
- **Add/Remove**: Buttons to manage sequences
- **Properties**:
  - Name (unique within bank)
  - Spritesheet Reference (dropdown)
  - Frame Range: Start frame, frame count
  - Timing: Frame duration, speed multiplier
  - Playback: Loop toggle, next animation
- **Event Editor**: Add frame events (sounds, hitboxes, VFX)
- **Visual Timeline**: Frame-by-frame timeline with events

#### Right Panel: Preview
**Method**: `RenderPreviewPanel()`

Real-time animation preview:
- **Playback Controls**:
  - Play/Pause button
  - Stop button (resets to frame 0)
  - Reset button (reload sequence)
  - Frame scrubber (manual frame selection)
- **Speed Control**: Slider (0.1x - 3.0x)
- **Loop Toggle**: Enable/disable looping
- **Preview Window**: Renders current frame with:
  - Hotspot visualization (crosshair)
  - Frame border
  - Frame index display
  - FPS counter
- **Frame Info**: Shows current frame, total frames, elapsed time
- **Spritesheet View**: Option to show full spritesheet with current frame highlighted

#### Bottom Panel: Properties
**Method**: `RenderPropertiesPanel()`

Bank-level metadata:
- **Bank ID**: Unique identifier
- **Name**: Display name
- **Description**: Text description
- **Author**: Creator name
- **Version**: Version string
- **Tags**: Comma-separated tags

## Data Format

### Unified Schema v2 JSON

The editor saves animation banks in a unified JSON format:

```json
{
  "schema_version": 2,
  "type": "animation_bank",
  "bankId": "player_animations",
  "metadata": {
    "name": "Player Character Animations",
    "description": "All animations for the main player character",
    "author": "Artist Name",
    "version": "1.0.0",
    "tags": ["player", "character", "combat"]
  },
  "spritesheets": [
    {
      "id": "player_idle",
      "path": "Gamedata/Sprites/player_idle.png",
      "description": "Idle animation spritesheet",
      "frameWidth": 64,
      "frameHeight": 64,
      "columns": 8,
      "rows": 1,
      "totalFrames": 8,
      "spacing": 0,
      "margin": 0,
      "hotspot": {"x": 0.5, "y": 0.9}
    },
    {
      "id": "player_walk",
      "path": "Gamedata/Sprites/player_walk.png",
      "description": "Walk cycle spritesheet",
      "frameWidth": 64,
      "frameHeight": 64,
      "columns": 10,
      "rows": 1,
      "totalFrames": 10,
      "spacing": 2,
      "margin": 1,
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
    },
    {
      "name": "walk",
      "spritesheetId": "player_walk",
      "startFrame": 0,
      "frameCount": 10,
      "frameDuration": 0.08,
      "loop": true,
      "speed": 1.0,
      "nextAnimation": "",
      "events": [
        {
          "type": "sound",
          "frame": 3,
          "dataJson": "{\"soundId\": \"footstep_left\"}"
        },
        {
          "type": "sound",
          "frame": 8,
          "dataJson": "{\"soundId\": \"footstep_right\"}"
        }
      ]
    }
  ]
}
```

**Backward Compatibility**:
The system still loads old format banks (with `animations` object instead of `sequences` array). The ECS animation system checks `spritesheetId.empty()` to determine format:
- New format: Uses spritesheet reference and frame ranges
- Old format: Uses individual frame definitions

## Standalone Window Pattern

The Animation Editor uses the same pattern as the BT Debugger:

### Window Lifecycle

1. **Creation** (`CreateSeparateWindow()`):
   - Create SDL_Window (1280x720, resizable, initially hidden)
   - Create SDL_Renderer (accelerated, VSync enabled)
   - Create dedicated ImGuiContext
   - Initialize ImGui_ImplSDL3_InitForSDLRenderer()
   - Initialize ImGui_ImplSDLRenderer3_Init()
   - Load ImGui style and fonts
   - Show window

2. **Rendering** (`RenderInSeparateWindow()`):
   - Switch to Animation Editor's ImGui context
   - Process SDL events for this window
   - Begin ImGui frame (NewFrame for both backends)
   - Render UI with full-screen docking space
   - End frame (Render, GetDrawData, RenderClear, RenderDrawData)
   - Present renderer

3. **Destruction** (`DestroySeparateWindow()`):
   - Shutdown ImGui backends (SDL3, SDLRenderer3)
   - Destroy ImGui context
   - Destroy SDL_Renderer
   - Destroy SDL_Window
   - Set pointers to nullptr

### Event Handling

`ProcessEvent(SDL_Event*)` handles:
- **Window Events**:
  - SDL_EVENT_WINDOW_CLOSE_REQUESTED: Close editor
  - SDL_EVENT_WINDOW_RESIZED: Update viewport
- **Keyboard Events**:
  - Ctrl+S: Save
  - Ctrl+Shift+S: Save As
  - Space: Toggle preview playback
  - Left/Right Arrow: Previous/next frame
- **Mouse Events**:
  - Zoom: Mouse wheel in spritesheet viewer
  - Pan: Middle mouse drag
  - Selection: Click on frames or sequences

Events are routed to Animation Editor **before** BT Debugger in `OlympeEngine.cpp` main loop (line 137-143).

## Integration with Animation System

The editor creates/modifies animation banks that are loaded by:

**AnimationManager** (`Source/Animation/AnimationManager.h/cpp`):
```cpp
class AnimationManager {
    bool LoadAnimationBank(const std::string& bankId, const std::string& path);
    AnimationBank* GetAnimationBank(const std::string& bankId);
    
    void PlayAnimation(EntityID entity, const std::string& animName);
    void SetAnimationSpeed(EntityID entity, float speed);
    void PauseAnimation(EntityID entity);
    void ResumeAnimation(EntityID entity);
};
```

**ECS Animation System** (`Source/ECS_Systems_Animation.cpp/h`):
```cpp
void UpdateEntity(EntityID id, float dt) {
    // Check format
    if (sequence->spritesheetId.empty()) {
        // Old format: frame-by-frame
        UpdateFrameByFrame(id, sequence, dt);
    } else {
        // New format: spritesheet + frame range
        UpdateRangeBased(id, sequence, dt);
    }
}
```

**Integration Flow**:
1. Artist creates animation bank in Animation Editor
2. Save to `Gamedata/Animations/bankId.json`
3. Game loads bank via `AnimationManager::LoadAnimationBank()`
4. Entities reference animations via `AnimationComponent`
5. ECS system updates animation frames each frame
6. AnimationGraph controls state transitions

## File Operations

### New Bank
**Method**: `NewBank()`

Creates empty bank with default structure:
- Auto-generated bank ID (timestamp-based)
- Empty spritesheets array
- Empty sequences array
- Default metadata

### Open Bank
**Method**: `OpenBank(filepath)`

Loads existing JSON bank:
1. Parse JSON with schema validation
2. Load spritesheets (verify paths exist)
3. Load sequences (verify spritesheet references)
4. Set current file path for save
5. Mark as clean (not dirty)

### Save Bank
**Method**: `SaveBank()`

Saves to current file:
1. Serialize bank to JSON (Schema v2 format)
2. Pretty-print JSON with 2-space indent
3. Write to file
4. Mark as clean

### Save Bank As
**Method**: `SaveBankAs()`

Prompts for new file path:
1. Show native file dialog
2. Update current file path
3. Call `SaveBank()`

### Import Bank JSON
**Method**: `ImportBankJSON(filepath)`

Imports from external JSON:
1. Parse JSON
2. Merge with current bank (or replace)
3. Show import summary dialog
4. Mark as dirty

### Export Bank JSON
**Method**: `ExportBankJSON(filepath)`

Exports to specified path:
1. Serialize to JSON
2. Write to file
3. Keep current file path unchanged

### Directory Scanning
**Method**: `ScanBankDirectory(dirPath)`

Scans for animation banks:
1. Recursively scan directory for `.json` files
2. Parse each file to check schema version
3. Extract bank metadata (ID, name)
4. Return list of bank info

## Performance Considerations

- **Texture Loading**: Spritesheets loaded on-demand, cached in SDL_Texture
- **Preview Rendering**: Only renders current frame, not full animation
- **Grid Detection**: Caches results, only runs when user requests
- **File Watching**: Editor doesn't auto-reload on external changes
- **Undo/Redo**: Not implemented (future enhancement)

## Troubleshooting

### Common Issues

**Spritesheet not displaying**:
- Verify path is relative to `Gamedata/` directory
- Check file exists and is valid image format (PNG, JPG)
- Ensure frame dimensions don't exceed image size

**Animation plays too fast/slow**:
- Check frame duration (typical: 0.08-0.15 seconds)
- Verify speed multiplier (1.0 = normal)
- Ensure loop is enabled if intended

**Preview shows wrong frame**:
- Verify start frame and frame count
- Check spritesheet has enough frames (columns × rows)
- Ensure totalFrames matches actual spritesheet

**Grid auto-detect fails**:
- Image must have consistent grid spacing
- Frames must be uniformly sized
- Spacing and margin must be correctly set

## Keyboard Shortcuts

**File**:
- `Ctrl+N`: New bank
- `Ctrl+O`: Open bank
- `Ctrl+S`: Save
- `Ctrl+Shift+S`: Save As
- `Ctrl+W`: Close window

**Preview**:
- `Space`: Play/Pause
- `Left Arrow`: Previous frame
- `Right Arrow`: Next frame
- `Home`: First frame
- `End`: Last frame
- `R`: Reset preview

**View**:
- `Ctrl++`: Zoom in
- `Ctrl+-`: Zoom out
- `Ctrl+0`: Reset zoom
- `Ctrl+G`: Toggle grid

## File Locations

**Core Editor**:
- `Source/Editor/AnimationEditorWindow.h/cpp` - Main editor class

**Data Structures**:
- `Source/Animation/AnimationTypes.h` - AnimationBank, SpritesheetInfo, AnimationSequence
- `Source/Animation/AnimationGraph.h` - AnimationGraph (both simple and advanced)

**Animation System**:
- `Source/Animation/AnimationManager.h/cpp` - Bank loading and management
- `Source/ECS_Systems_Animation.h/cpp` - ECS animation update system

**Integration**:
- `Source/OlympeEngine.cpp` - Main loop event routing (F9 hotkey)

## Related Documentation

- [Animation Editor User Guide](../Animation_Editor_User_Guide.md) - User-facing guide
- [Animation System Overview](../Animation_System.md) - System architecture
- [Animation System Documentation](../../website/docs/technical-reference/animation/animation-system.md) - Complete technical reference
