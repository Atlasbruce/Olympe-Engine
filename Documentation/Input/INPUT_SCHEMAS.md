# Input Configuration JSON Schemas

**Version:** 2.0  
**Last Updated:** 2026-02-03

## Overview

This document defines the JSON schemas for input configuration files used by the Olympe Engine.

## File Structure

### Inputs.json (Main Configuration)

Complete input system configuration including profiles and action maps.

```json
{
  "version": "2.0",
  "profiles": [ /* InputProfile[] */ ],
  "action_maps": [ /* ActionMap[] */ ],
  "default_profile_assignment": { /* DeviceTypeProfileMap */ },
  "settings": { /* GlobalInputSettings */ }
}
```

### Inputs.user.json (User Overrides)

User-specific overrides for customization.

```json
{
  "version": "2.0",
  "profile_overrides": [ /* ProfileOverride[] */ ]
}
```

### olympe-config.json (Engine Configuration)

Engine-wide configuration including input system settings.

```json
{
  "screen_width": 1280,
  "screen_height": 720,
  "editor_enabled": false,
  "input_config_path": "Config/Inputs.json",
  "input_log_level": "info"
}
```

## Schema Definitions

### InputProfile

Defines input bindings for a specific device type.

```json
{
  "name": "string",                    // Unique profile name
  "device_type": "Joystick|KeyboardMouse",
  "device_filter": {                   // Optional: Auto-select profile for specific devices
    "vendor": "string",                // Device vendor name
    "product": "string",               // Device product name
    "vendor_id": "number",             // USB vendor ID (hex)
    "product_id": "number"             // USB product ID (hex)
  },
  "actions": {
    "action_name": { /* InputBinding */ }
  },
  "settings": {
    "deadzone": 0.15,                  // Global deadzone for analog inputs
    "sensitivity": 1.0,                // Global sensitivity multiplier
    "validate_overlaps": true          // Enable keyboard overlap validation
  }
}
```

**Field Details:**

- `name` (required): Unique identifier for the profile (e.g., "default_keyboard", "xbox_controller")
- `device_type` (required): Type of device this profile is for
  - `"Joystick"` - Game controllers, gamepads
  - `"KeyboardMouse"` - Keyboard and mouse as unified device
- `device_filter` (optional): Automatically apply this profile to matching devices
- `actions` (required): Map of action names to input bindings
- `settings` (optional): Profile-specific settings

### InputBinding

Defines how a physical input maps to an action.

#### Button Binding

```json
{
  "type": "button",
  "button": 0,                        // Button index (0-15)
  "comment": "A button on Xbox"       // Optional: Human-readable description
}
```

#### Key Binding

```json
{
  "type": "key",
  "primary": "W",                     // SDL_Scancode name
  "alternate": "UP",                  // Optional: Alternate key
  "comment": "Move forward"
}
```

**Supported Key Names:** `A-Z`, `0-9`, `F1-F12`, `SPACE`, `RETURN`, `ESCAPE`, `TAB`, `LSHIFT`, `RSHIFT`, `LCTRL`, `RCTRL`, `LALT`, `RALT`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `PAGEUP`, `PAGEDOWN`, `HOME`, `END`, `INSERT`, `DELETE`, etc.

Full list: [SDL_Scancode Documentation](https://wiki.libsdl.org/SDL3/SDL_Scancode)

#### Axis Binding

```json
{
  "type": "axis",
  "axis": 0,                          // Axis index (0-5)
  "deadzone": 0.15,                   // Deadzone threshold (0.0-1.0)
  "sensitivity": 1.0,                 // Sensitivity multiplier
  "invert": false,                    // Invert axis direction
  "scale": 1.0                        // Scale factor
}
```

**Standard Axis Indices:**
- 0 = Left stick X
- 1 = Left stick Y
- 2 = Right stick X
- 3 = Right stick Y
- 4 = Left trigger
- 5 = Right trigger

#### Stick Binding (Composite)

```json
{
  "type": "stick",
  "stick": "left|right",              // Which stick
  "deadzone": 0.15,
  "sensitivity": 1.0,
  "invert_x": false,
  "invert_y": false
}
```

Internally maps to two axes (X and Y).

#### Trigger Binding

```json
{
  "type": "trigger",
  "trigger": "left|right",            // Which trigger
  "threshold": 0.1,                   // Activation threshold (0.0-1.0)
  "sensitivity": 1.0
}
```

#### Mouse Button Binding

```json
{
  "type": "mouse_button",
  "button": 1,                        // 1=Left, 2=Middle, 3=Right
  "comment": "Primary fire"
}
```

#### Mouse Axis Binding

```json
{
  "type": "mouse_axis",
  "axis": "x|y|wheel_x|wheel_y",     // Mouse axis
  "sensitivity": 1.0,
  "invert": false
}
```

#### Composite Binding (Advanced)

```json
{
  "type": "composite",
  "inputs": [
    {"type": "button", "button": 4},
    {"type": "stick", "stick": "left", "threshold": 0.5}
  ],
  "timing": "simultaneous|sequential",
  "window_ms": 100                    // Timing window in milliseconds
}
```

### ActionMap

Groups actions by context and priority.

```json
{
  "name": "string",                   // Unique action map name
  "context": "Gameplay|UI|Editor|System",
  "priority": 0,                      // Higher = processed first (0-100)
  "exclusive": false,                 // Block lower priority maps when active
  "actions": [                        // List of action names in this map
    "action_name_1",
    "action_name_2"
  ],
  "enabled_by_default": true          // Active on startup
}
```

**Context Types:**

- `Gameplay` - Normal game controls (movement, combat)
- `UI` - Menu and UI navigation
- `Editor` - Editor tools and shortcuts (requires `editor_enabled: true`)
- `System` - Always-active system controls (screenshot, quit, debug)

**Priority Ranges:**
- 0-29: Gameplay
- 30-49: UI
- 50-79: Editor
- 80-100: System

**Exclusive Behavior:**
- `true`: Consumes all input, preventing lower priority maps from receiving it
- `false`: Allows input to pass through to lower priority maps

### DeviceTypeProfileMap

Maps device types to default profiles.

```json
{
  "Joystick": "default_gamepad",      // Profile name for joysticks
  "KeyboardMouse": "default_keyboard" // Profile name for keyboard-mouse
}
```

### GlobalInputSettings

Global input system settings.

```json
{
  "auto_assign_devices": true,        // Auto-assign devices to players
  "prefer_joysticks": true,           // Prefer joysticks over keyboard-mouse
  "enable_hot_reload": false,         // Hot-reload config on file change
  "log_level": "info",                // error|warning|info|debug
  "device_reconnect_timeout_ms": 5000 // Time to wait for device reconnect
}
```

### ProfileOverride (User Overrides)

Overrides specific actions in a profile.

```json
{
  "profile": "default_keyboard",      // Name of profile to override
  "actions": {
    "action_name": { /* InputBinding */ }
  }
}
```

## Complete Examples

### Example 1: Minimal Configuration

Minimal `Inputs.json` for single-player keyboard game:

```json
{
  "version": "2.0",
  "profiles": [
    {
      "name": "default_keyboard",
      "device_type": "KeyboardMouse",
      "actions": {
        "move_up": {"type": "key", "primary": "W"},
        "move_down": {"type": "key", "primary": "S"},
        "move_left": {"type": "key", "primary": "A"},
        "move_right": {"type": "key", "primary": "D"},
        "jump": {"type": "key", "primary": "SPACE"},
        "shoot": {"type": "mouse_button", "button": 1}
      }
    }
  ],
  "action_maps": [
    {
      "name": "gameplay",
      "context": "Gameplay",
      "priority": 0,
      "exclusive": false,
      "actions": ["move_up", "move_down", "move_left", "move_right", "jump", "shoot"]
    }
  ],
  "default_profile_assignment": {
    "KeyboardMouse": "default_keyboard"
  }
}
```

### Example 2: Multiplayer with Gamepad Support

`Inputs.json` for local multiplayer with keyboard and gamepad:

```json
{
  "version": "2.0",
  "profiles": [
    {
      "name": "default_keyboard",
      "device_type": "KeyboardMouse",
      "actions": {
        "move_up": {"type": "key", "primary": "W", "alternate": "UP"},
        "move_down": {"type": "key", "primary": "S", "alternate": "DOWN"},
        "move_left": {"type": "key", "primary": "A", "alternate": "LEFT"},
        "move_right": {"type": "key", "primary": "D", "alternate": "RIGHT"},
        "jump": {"type": "key", "primary": "SPACE"},
        "shoot": {"type": "key", "primary": "LCTRL"},
        "interact": {"type": "key", "primary": "E"}
      },
      "settings": {
        "validate_overlaps": true
      }
    },
    {
      "name": "default_gamepad",
      "device_type": "Joystick",
      "actions": {
        "move": {"type": "stick", "stick": "left", "deadzone": 0.15},
        "look": {"type": "stick", "stick": "right", "deadzone": 0.15},
        "jump": {"type": "button", "button": 0, "comment": "A button"},
        "shoot": {"type": "button", "button": 1, "comment": "B button"},
        "interact": {"type": "button", "button": 2, "comment": "X button"}
      },
      "settings": {
        "deadzone": 0.15,
        "sensitivity": 1.0
      }
    }
  ],
  "action_maps": [
    {
      "name": "gameplay",
      "context": "Gameplay",
      "priority": 0,
      "exclusive": false,
      "actions": ["move_up", "move_down", "move_left", "move_right", "move", "look", "jump", "shoot", "interact"]
    }
  ],
  "default_profile_assignment": {
    "Joystick": "default_gamepad",
    "KeyboardMouse": "default_keyboard"
  },
  "settings": {
    "auto_assign_devices": true,
    "prefer_joysticks": true,
    "log_level": "info"
  }
}
```

### Example 3: Full-Featured Configuration

Complete `Inputs.json` with gameplay, UI, editor, and system contexts:

```json
{
  "version": "2.0",
  "profiles": [
    {
      "name": "default_keyboard",
      "device_type": "KeyboardMouse",
      "actions": {
        "move_up": {"type": "key", "primary": "W", "alternate": "UP"},
        "move_down": {"type": "key", "primary": "S", "alternate": "DOWN"},
        "move_left": {"type": "key", "primary": "A", "alternate": "LEFT"},
        "move_right": {"type": "key", "primary": "D", "alternate": "RIGHT"},
        "jump": {"type": "key", "primary": "SPACE"},
        "shoot": {"type": "mouse_button", "button": 1},
        "aim": {"type": "mouse_button", "button": 3},
        "interact": {"type": "key", "primary": "E"},
        "reload": {"type": "key", "primary": "R"},
        "sprint": {"type": "key", "primary": "LSHIFT"},
        "crouch": {"type": "key", "primary": "LCTRL"},
        
        "ui_up": {"type": "key", "primary": "UP"},
        "ui_down": {"type": "key", "primary": "DOWN"},
        "ui_left": {"type": "key", "primary": "LEFT"},
        "ui_right": {"type": "key", "primary": "RIGHT"},
        "ui_confirm": {"type": "key", "primary": "RETURN"},
        "ui_cancel": {"type": "key", "primary": "ESCAPE"},
        
        "editor_select": {"type": "mouse_button", "button": 1},
        "editor_place": {"type": "mouse_button", "button": 1},
        "editor_delete": {"type": "key", "primary": "DELETE"},
        "editor_copy": {"type": "key", "primary": "C"},
        "editor_paste": {"type": "key", "primary": "V"},
        "editor_undo": {"type": "key", "primary": "Z"},
        "editor_redo": {"type": "key", "primary": "Y"},
        "editor_grid_toggle": {"type": "key", "primary": "G"},
        "editor_snap_toggle": {"type": "key", "primary": "N"},
        
        "screenshot": {"type": "key", "primary": "F12"},
        "toggle_fps": {"type": "key", "primary": "F3"},
        "toggle_debug": {"type": "key", "primary": "F1"},
        "menu_open": {"type": "key", "primary": "ESCAPE"},
        "quit": {"type": "key", "primary": "Q"}
      },
      "settings": {
        "validate_overlaps": true
      }
    },
    {
      "name": "xbox_controller",
      "device_type": "Joystick",
      "device_filter": {
        "product": "Xbox"
      },
      "actions": {
        "move": {"type": "stick", "stick": "left", "deadzone": 0.15},
        "look": {"type": "stick", "stick": "right", "deadzone": 0.15, "sensitivity": 1.2},
        "jump": {"type": "button", "button": 0},
        "shoot": {"type": "trigger", "trigger": "right", "threshold": 0.1},
        "aim": {"type": "trigger", "trigger": "left", "threshold": 0.1},
        "interact": {"type": "button", "button": 2},
        "reload": {"type": "button", "button": 3},
        "sprint": {"type": "button", "button": 8},
        "crouch": {"type": "button", "button": 1},
        
        "ui_up": {"type": "stick", "stick": "left", "axis": "y", "threshold": -0.5},
        "ui_down": {"type": "stick", "stick": "left", "axis": "y", "threshold": 0.5},
        "ui_left": {"type": "stick", "stick": "left", "axis": "x", "threshold": -0.5},
        "ui_right": {"type": "stick", "stick": "left", "axis": "x", "threshold": 0.5},
        "ui_confirm": {"type": "button", "button": 0},
        "ui_cancel": {"type": "button", "button": 1},
        
        "screenshot": {"type": "button", "button": 6},
        "menu_open": {"type": "button", "button": 7}
      },
      "settings": {
        "deadzone": 0.15,
        "sensitivity": 1.0
      }
    }
  ],
  "action_maps": [
    {
      "name": "gameplay",
      "context": "Gameplay",
      "priority": 0,
      "exclusive": false,
      "actions": [
        "move_up", "move_down", "move_left", "move_right", "move", "look",
        "jump", "shoot", "aim", "interact", "reload", "sprint", "crouch"
      ]
    },
    {
      "name": "ui_navigation",
      "context": "UI",
      "priority": 30,
      "exclusive": true,
      "actions": [
        "ui_up", "ui_down", "ui_left", "ui_right",
        "ui_confirm", "ui_cancel"
      ]
    },
    {
      "name": "editor_tools",
      "context": "Editor",
      "priority": 50,
      "exclusive": true,
      "actions": [
        "editor_select", "editor_place", "editor_delete",
        "editor_copy", "editor_paste", "editor_undo", "editor_redo",
        "editor_grid_toggle", "editor_snap_toggle"
      ]
    },
    {
      "name": "system",
      "context": "System",
      "priority": 100,
      "exclusive": false,
      "actions": [
        "screenshot", "toggle_fps", "toggle_debug", "menu_open", "quit"
      ]
    }
  ],
  "default_profile_assignment": {
    "Joystick": "xbox_controller",
    "KeyboardMouse": "default_keyboard"
  },
  "settings": {
    "auto_assign_devices": true,
    "prefer_joysticks": true,
    "enable_hot_reload": false,
    "log_level": "info",
    "device_reconnect_timeout_ms": 5000
  }
}
```

### Example 4: User Overrides

`Inputs.user.json` for player customization:

```json
{
  "version": "2.0",
  "profile_overrides": [
    {
      "profile": "default_keyboard",
      "actions": {
        "jump": {"type": "key", "primary": "LSHIFT"},
        "shoot": {"type": "mouse_button", "button": 1},
        "aim": {"type": "mouse_button", "button": 3},
        "crouch": {"type": "key", "primary": "C"},
        "sprint": {"type": "key", "primary": "LCTRL"}
      }
    },
    {
      "profile": "xbox_controller",
      "actions": {
        "look": {
          "type": "stick",
          "stick": "right",
          "deadzone": 0.12,
          "sensitivity": 1.5,
          "invert_y": true
        }
      }
    }
  ]
}
```

### Example 5: olympe-config.json

Complete engine configuration with input settings:

```json
{
  "version": "2.0",
  "screen_width": 1920,
  "screen_height": 1080,
  "fullscreen": false,
  "vsync": true,
  
  "editor_enabled": false,
  "input_config_path": "Config/Inputs.json",
  "input_log_level": "info",
  
  "log_panel_visible": true,
  "log_panel_height": 200,
  
  "audio_master_volume": 1.0,
  "audio_music_volume": 0.7,
  "audio_sfx_volume": 0.8
}
```

## Validation Rules

### Profile Validation

1. **Unique Profile Names**: Each profile must have a unique `name`
2. **Valid Device Type**: `device_type` must be `"Joystick"` or `"KeyboardMouse"`
3. **Valid Actions**: All `actions` must reference valid `InputBinding` objects
4. **Keyboard Overlap**: When `validate_overlaps: true`, keyboard profiles must not have overlapping key bindings

### Action Map Validation

1. **Unique Names**: Each action map must have a unique `name`
2. **Valid Context**: `context` must be one of: `Gameplay`, `UI`, `Editor`, `System`
3. **Valid Priority**: `priority` must be 0-100
4. **Action References**: All `actions` must reference actions defined in at least one profile

### Binding Validation

1. **Button Index**: 0-15 (must match `Controller_data::MAX_BUTTONS`)
2. **Axis Index**: 0-5 (must match `JoystickManager::MAX_AXES`)
3. **Key Names**: Must be valid SDL_Scancode names
4. **Deadzone**: 0.0-1.0
5. **Sensitivity**: > 0.0 (typically 0.5-2.0)

## Error Handling

### Load Errors

**Missing File:**
```
[InputConfig][Error] Failed to load input config: Config/Inputs.json not found
[InputConfig][Info] Using default input configuration
```

**Parse Error:**
```
[InputConfig][Error] JSON parse error at line 45: Unexpected token
[InputConfig][Error] Failed to parse input config: Config/Inputs.json
[InputConfig][Info] Using default input configuration
```

**Validation Error:**
```
[InputProfile][Error] Profile 'default_keyboard' validation failed
[InputProfile][Error] Overlapping keys detected: jump (SPACE) and interact (SPACE)
[InputConfig][Warning] Skipping invalid profile: default_keyboard
```

### Runtime Errors

**Missing Action:**
```
[InputAction][Warning] Action 'dodge_roll' not found in profile 'default_gamepad'
[InputAction][Info] Using default binding for action 'dodge_roll'
```

**Invalid Device:**
```
[InputDevice][Error] Cannot assign device 5 to player 1: device not connected
[InputDevice][Info] Attempting auto-assignment for player 1
```

## Best Practices

1. **Always Include Version**: Specify `"version": "2.0"` in all config files
2. **Use Comments**: Add `"comment"` fields to document bindings
3. **Provide Defaults**: Define sensible default profiles for all device types
4. **Test Validation**: Enable `validate_overlaps: true` during development
5. **Version Control**: Commit `Inputs.json`, ignore `Inputs.user.json`
6. **Document Actions**: Maintain a list of all actions in your game
7. **Consistent Naming**: Use descriptive, consistent action names (e.g., `move_up`, not `up_key`)

## See Also

- [INPUT_ARCHITECTURE.md](INPUT_ARCHITECTURE.md) - System architecture
- [INPUT_USER_GUIDE.md](INPUT_USER_GUIDE.md) - User documentation
- [JSON Schema Validator](https://www.jsonschemavalidator.net/) - Online validation tool
- [SDL Scancode Reference](https://wiki.libsdl.org/SDL3/SDL_Scancode) - Complete key code list
