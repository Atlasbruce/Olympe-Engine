---
id: input-architecture
title: Input System Architecture
sidebar_label: Architecture
---

# Input System Architecture

**Version:** 2.0  
**Last Updated:** 2026-02-03

## Overview

The Olympe Engine input system provides a flexible, multi-device input management architecture that supports:
- Multiple simultaneous input devices (joysticks, keyboard-mouse)
- Context-aware input handling (gameplay, editor, system)
- Per-device input profiles with customizable bindings
- JSON-based configuration with runtime override support
- ECS integration for entity-based input processing

## Architecture Components

### 1. Input Device Abstraction

#### InputDeviceSlot

An `InputDeviceSlot` represents a logical input device that can be assigned to a player or game entity. Each slot has:

```cpp
enum class InputDeviceType {
    None,
    Joystick,
    KeyboardMouse
};

struct InputDeviceSlot {
    InputDeviceType type;
    int deviceIndex;        // SDL_JoystickID for joysticks, -1 for keyboard-mouse
    short assignedPlayerID; // -1 if unassigned
    bool isConnected;
    std::string deviceName;
    InputProfile* profile;  // Device-specific configuration
};
```

**Key Features:**
- Auto-assignment prefers joysticks over keyboard-mouse
- Handles hot-plugging (connect/disconnect events)
- Maintains binding state across device disconnects

#### InputDeviceManager

Manages all device slots and assignment logic:

```cpp
class InputDeviceManager {
public:
    // Device registration
    void RegisterDevice(InputDeviceSlot slot);
    void UnregisterDevice(int deviceIndex);
    
    // Auto-assignment (prefers joysticks, then keyboard-mouse)
    InputDeviceSlot* AutoAssignDevice(short playerID);
    
    // Manual assignment
    bool AssignDeviceToPlayer(int deviceIndex, short playerID);
    bool UnassignDevice(short playerID);
    
    // Query
    InputDeviceSlot* GetDeviceForPlayer(short playerID);
    std::vector<InputDeviceSlot*> GetAvailableDevices();
};
```

### 2. Input Profiles

#### InputProfile

Device-specific configuration defining button/key mappings and behavior:

```cpp
struct InputProfile {
    std::string profileName;
    InputDeviceType deviceType;
    
    // Action mappings (action name -> input binding)
    std::unordered_map<std::string, InputBinding> actionMappings;
    
    // Settings
    float deadzone = 0.15f;
    float sensitivity = 1.0f;
    bool invertYAxis = false;
    
    // Validation
    bool ValidateNoOverlaps() const; // For keyboard profiles
};

struct InputBinding {
    InputType type;           // Button, Key, Axis, MouseButton
    int primaryInput;         // SDL_Scancode, button index, etc.
    int alternateInput = -1;  // Optional alternate binding
    
    // For axis bindings
    float axisScale = 1.0f;
    float axisDeadzone = 0.15f;
};
```

**Profile Types:**
- **Gameplay Profile**: Standard game controls (movement, actions)
- **Editor Profile**: Editor-specific shortcuts and tools
- **System Profile**: System-level inputs (pause, screenshot, debug)

#### Keyboard Overlap Validation

Keyboard profiles validate that no two actions share the same key binding within the same context:

```cpp
bool InputProfile::ValidateNoOverlaps() const {
    std::unordered_set<int> usedKeys;
    for (const auto& [action, binding] : actionMappings) {
        if (usedKeys.count(binding.primaryInput) > 0) {
            SYSTEM_LOG << "ERROR: Key overlap detected for action: " << action << "\n";
            return false;
        }
        usedKeys.insert(binding.primaryInput);
        
        if (binding.alternateInput != -1) {
            if (usedKeys.count(binding.alternateInput) > 0) {
                SYSTEM_LOG << "ERROR: Key overlap detected for alternate binding: " << action << "\n";
                return false;
            }
            usedKeys.insert(binding.alternateInput);
        }
    }
    return true;
}
```

### 3. Action Maps

#### ActionMap System

Action maps define logical groupings of input actions by context:

```cpp
enum class ActionMapContext {
    Gameplay,  // Core gameplay actions (move, jump, shoot)
    Editor,    // Editor-specific actions (select, place, delete)
    System,    // System actions (pause, screenshot, debug toggle)
    UI         // UI navigation (confirm, cancel, navigate)
};

struct ActionMap {
    ActionMapContext context;
    std::string name;
    std::vector<std::string> actions; // List of action names
    int priority = 0;                 // Higher priority consumes input first
    bool exclusive = false;            // If true, blocks lower priority maps
};
```

**Action Map Stack:**
```
System Map (priority: 100, exclusive: false)
  ↓
Editor Map (priority: 50, exclusive: true)  ← Active when editor_enabled
  ↓
UI Map (priority: 30, exclusive: true)      ← Active when menu open
  ↓
Gameplay Map (priority: 0, exclusive: false)
```

### 4. Context Switching

#### Editor Context Switching

The system reads `editor_enabled` from `olympe-config.json` to determine context:

```cpp
class InputContextManager {
public:
    void Initialize();
    void LoadConfig(const std::string& configPath);
    
    // Context stack operations
    void PushContext(ActionMapContext ctx);
    void PopContext();
    ActionMapContext GetActiveContext() const;
    
    // Configuration
    bool IsEditorEnabled() const { return m_editorEnabled; }
    void SetEditorEnabled(bool enabled);
    
private:
    bool m_editorEnabled = false;
    std::vector<ActionMapContext> m_contextStack;
};
```

**Context Switching Flow:**
1. Load `olympe-config.json` at startup
2. If `editor_enabled: true`, push Editor context onto stack
3. Systems query active context before processing input
4. Editor actions only processed when Editor context is active

### 5. JSON Configuration System

#### Configuration Files

**olympe-config.json** (Engine configuration):
```json
{
    "screen_width": 1280,
    "screen_height": 720,
    "editor_enabled": false,
    "input_config_path": "Config/Inputs.json",
    "log_level": "info"
}
```

**Inputs.json** (Input configuration):
See `INPUT_SCHEMAS.md` for complete schema documentation.

#### Configuration Loading

```cpp
class InputConfigLoader {
public:
    // Load complete input configuration
    bool LoadInputConfig(const std::string& path);
    
    // Override specific profiles
    bool LoadProfileOverride(const std::string& path, const std::string& profileName);
    
    // Save current configuration
    bool SaveInputConfig(const std::string& path);
    
private:
    InputDeviceManager* m_deviceManager;
    std::vector<InputProfile> m_profiles;
    std::vector<ActionMap> m_actionMaps;
};
```

**Override System:**
- Base profiles loaded from `Inputs.json`
- User overrides loaded from `Inputs.user.json` (if exists)
- Override values replace base values
- Useful for per-user customization without modifying base config

### 6. ECS Integration

#### Component Compatibility

The new input system maintains compatibility with existing ECS components:

**Controller_data** (Raw hardware state):
```cpp
struct Controller_data {
    short controllerID;      // Maps to InputDeviceSlot.deviceIndex
    bool isConnected;        // Synced with InputDeviceSlot.isConnected
    Vector leftStick;        // Populated by InputDeviceSlot's profile
    Vector rightStick;
    float leftTrigger;
    float rightTrigger;
    bool buttons[16];
};
```

**PlayerController_data** (Gameplay actions):
```cpp
struct PlayerController_data {
    Vector Joydirection;     // Mapped from action "move"
    bool isJumping;          // Mapped from action "jump"
    bool isShooting;         // Mapped from action "shoot"
    // ... other gameplay actions
};
```

**PlayerBinding_data** (Player-device association):
```cpp
struct PlayerBinding_data {
    short playerIndex;       // Player ID
    short controllerID;      // Links to InputDeviceSlot.deviceIndex
};
```

#### System Integration

**InputMappingSystem** (Action mapping):
- Queries active `ActionMapContext` from `InputContextManager`
- Reads raw input from `Controller_data`
- Applies action mappings from `InputProfile`
- Updates `PlayerController_data` with mapped actions
- Respects context priority (editor actions override gameplay when active)

**InputEventConsumeSystem** (Event processing):
- Handles SDL device connect/disconnect events
- Updates `Controller_data.isConnected` state
- Triggers auto-assignment for new devices
- Maintains device hot-plug support

**InputsManager** (Legacy integration):
- Wraps `InputDeviceManager` for backward compatibility
- Maintains existing `BindControllerToPlayer()` API
- Delegates to new system internally
- Provides helper methods for common operations

### 7. Logging System

#### Logging Levels

```cpp
enum class InputLogLevel {
    Error,    // Critical errors (failed to load config, device errors)
    Warning,  // Non-critical issues (overlap detection, missing bindings)
    Info,     // General information (device connected, context switched)
    Debug     // Verbose debugging (every input event, action mapping)
};
```

#### Log Categories

```
[InputDevice]   Device connection, assignment, hot-plug events
[InputProfile]  Profile loading, validation, overlap detection
[InputContext]  Context switching, action map priority resolution
[InputAction]   Action mapping, input consumption
[InputConfig]   Configuration loading, parsing, override application
```

**Example Logs:**
```
[InputDevice][Info] Joystick connected: Xbox Controller (ID: 0)
[InputDevice][Info] Auto-assigned Joystick 0 to Player 1
[InputProfile][Warning] Keyboard profile 'gameplay' has overlapping keys: 'jump' and 'interact' both use SPACE
[InputContext][Info] Context switched: Gameplay -> Editor (editor_enabled: true)
[InputAction][Debug] Action 'move' mapped to leftStick (0.75, -0.32)
[InputConfig][Info] Loaded input configuration from 'Config/Inputs.json'
[InputConfig][Info] Applied user overrides from 'Config/Inputs.user.json'
```

## Data Flow Diagram

```
                         ┌──────────────────┐
                         │  SDL Event Loop  │
                         └────────┬─────────┘
                                  │
                                  ▼
                    ┌─────────────────────────┐
                    │   InputsManager         │
                    │   HandleEvent()         │
                    └─────────┬───────────────┘
                              │
                 ┌────────────┼────────────┐
                 ▼            ▼            ▼
        ┌──────────────┐ ┌─────────┐ ┌──────────┐
        │ JoystickMgr  │ │  KeyMgr │ │ MouseMgr │
        └──────┬───────┘ └────┬────┘ └────┬─────┘
               │              │           │
               └──────────────┼───────────┘
                              ▼
                    ┌───────────────────┐
                    │ InputDeviceSlots  │
                    │ (with profiles)   │
                    └─────────┬─────────┘
                              │
                              ▼
                    ┌───────────────────┐
                    │ InputContextMgr   │
                    │ (Action Map Stack)│
                    └─────────┬─────────┘
                              │
                 ┌────────────┼────────────┐
                 ▼                         ▼
    ┌─────────────────────┐   ┌──────────────────────┐
    │InputEventConsume    │   │ InputMappingSystem   │
    │System               │   │                      │
    │                     │   │ - Reads context      │
    │- Update Controller_ │   │ - Apply profiles     │
    │  data               │   │ - Update Player      │
    │- Handle hotplug     │   │   Controller_data    │
    └─────────────────────┘   └──────────────────────┘
                              
```

## Best Practices

### For Game Developers

1. **Use Action Names, Not Raw Inputs**
   ```cpp
   // Good: Uses action mapping
   if (playerCtrl.isJumping) { ... }
   
   // Bad: Direct key checks
   if (KeyboardManager::Get().IsKeyHeld(SDL_SCANCODE_SPACE)) { ... }
   ```

2. **Define Actions in Profiles**
   - Always define actions in JSON profiles
   - Never hardcode input bindings in game code
   - Allow players to rebind actions

3. **Respect Context Priority**
   - Check active context before processing input
   - Don't process gameplay input when UI/Editor is active
   ```cpp
   if (InputContextManager::Get().GetActiveContext() != ActionMapContext::Gameplay) {
       return; // Skip gameplay input processing
   }
   ```

### For Engine Developers

1. **Validate Profiles on Load**
   - Always call `ValidateNoOverlaps()` for keyboard profiles
   - Log warnings for missing action mappings
   - Provide sensible defaults

2. **Log Device Changes**
   - Log all device connect/disconnect events
   - Log auto-assignment decisions
   - Log context switches with reasons

3. **Handle Edge Cases**
   - Device disconnected mid-game
   - All devices unassigned
   - Invalid configuration files
   - Overlapping keyboard bindings

## Performance Considerations

- **Device slot lookup**: O(1) hash map lookup by player ID
- **Action mapping**: O(1) hash map lookup by action name
- **Context switching**: O(1) vector push/pop operations
- **Profile validation**: O(n) where n = number of actions (done once at load)

## Thread Safety

- All manager classes use mutex locks for thread-safe access
- Device state updated only on main thread (SDL event thread)
- ECS systems process input on main thread only
- Configuration loading should be done during initialization or paused state

## Future Enhancements

- [ ] Hot-reload input configuration without restart
- [ ] Runtime profile editor (in-game rebinding UI)
- [ ] Analog button support (pressure-sensitive)
- [ ] Gesture recognition for touch input
- [ ] Input recording/playback for testing
- [ ] Network input synchronization for multiplayer

## See Also

- [INPUT_USER_GUIDE.md](INPUT_USER_GUIDE.md) - User-facing documentation
- [INPUT_SCHEMAS.md](INPUT_SCHEMAS.md) - JSON schema reference
- [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - Upgrading from v1.x input system
