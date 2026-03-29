---
id: input-configuration
title: "Input System Configuration"
sidebar_label: "Configuration"
---

# Input System Configuration

Olympe Engine's input system supports keyboards, mice, and gamepads with flexible JSON-based configuration. This allows you to define custom control schemes without modifying code.

## Configuration Files

### Main Configuration

**File**: `Config/olympe-config.json`

Main engine configuration including input system settings:

```json
{
  "version": "2.0",
  "screen_width": 1280,
  "screen_height": 720,
  "fullscreen": false,
  "vsync": true,
  "editor_enabled": false,
  "input_config_path": "Config/Inputs.json",
  "input_log_level": "info"
}
```

Key settings:
- **`input_config_path`**: Path to input profile configuration file
- **`input_log_level`**: Logging verbosity (`debug`, `info`, `warning`, `error`)

### Input Profiles

**File**: `Config/Inputs.json`

Defines input profiles, action mappings, and device settings.

## Input Profile Structure

### Profile Definition

Each profile defines bindings for a specific device type:

```json
{
  "version": "2.0",
  "profiles": [
    {
      "name": "default_keyboard",
      "device_type": "KeyboardMouse",
      "description": "Default keyboard and mouse bindings",
      "actions": { /* ... */ },
      "settings": { /* ... */ }
    },
    {
      "name": "default_gamepad",
      "device_type": "Joystick",
      "description": "Default gamepad bindings",
      "actions": { /* ... */ },
      "settings": { /* ... */ }
    }
  ]
}
```

### Device Types

- **`KeyboardMouse`**: Keyboard and mouse input
- **`Joystick`**: Gamepad/controller input

## Keyboard Configuration

### Key Bindings

```json
{
  "name": "default_keyboard",
  "device_type": "KeyboardMouse",
  "actions": {
    "move_up": {
      "type": "key",
      "primary": "W",
      "alternate": "UP",
      "comment": "Move character forward/up"
    },
    "move_down": {
      "type": "key",
      "primary": "S",
      "alternate": "DOWN"
    },
    "jump": {
      "type": "key",
      "primary": "SPACE"
    },
    "interact": {
      "type": "key",
      "primary": "E"
    },
    "sprint": {
      "type": "key",
      "primary": "LSHIFT"
    },
    "menu_open": {
      "type": "key",
      "primary": "ESCAPE"
    }
  }
}
```

#### Key Action Properties

- **`type`**: Must be `"key"`
- **`primary`**: Main key binding (required)
- **`alternate`**: Alternative key binding (optional)
- **`comment`**: Description for documentation

#### Supported Key Names

Common keys:
- Letters: `A`-`Z`
- Numbers: `0`-`9`
- Function keys: `F1`-`F12`
- Modifiers: `LSHIFT`, `RSHIFT`, `LCTRL`, `RCTRL`, `LALT`, `RALT`
- Special: `SPACE`, `RETURN`, `ESCAPE`, `TAB`, `BACKSPACE`, `DELETE`
- Arrows: `UP`, `DOWN`, `LEFT`, `RIGHT`

See SDL3 `SDL_Scancode` documentation for complete list.

### Mouse Bindings

```json
{
  "shoot": {
    "type": "mouse_button",
    "button": 1,
    "comment": "Left mouse button"
  },
  "aim": {
    "type": "mouse_button",
    "button": 3,
    "comment": "Right mouse button"
  }
}
```

#### Mouse Button Numbers

- **1**: Left button
- **2**: Middle button
- **3**: Right button
- **4+**: Additional mouse buttons

### Keyboard Settings

```json
{
  "settings": {
    "validate_overlaps": true,
    "mouse_sensitivity": 1.0
  }
}
```

- **`validate_overlaps`**: Check for duplicate key bindings
- **`mouse_sensitivity`**: Mouse movement multiplier

## Gamepad Configuration

### Button Bindings

```json
{
  "name": "default_gamepad",
  "device_type": "Joystick",
  "actions": {
    "jump": {
      "type": "button",
      "button": 0,
      "comment": "A button (Xbox) / Cross (PlayStation)"
    },
    "interact": {
      "type": "button",
      "button": 2,
      "comment": "X button (Xbox) / Square (PlayStation)"
    },
    "crouch": {
      "type": "button",
      "button": 1,
      "comment": "B button (Xbox) / Circle (PlayStation)"
    }
  }
}
```

#### Standard Button Layout

**Xbox Controller / Generic Gamepad:**
- **0**: A / Cross
- **1**: B / Circle
- **2**: X / Square
- **3**: Y / Triangle
- **4**: Left Bumper (LB)
- **5**: Right Bumper (RB)
- **6**: Back / Select
- **7**: Start
- **8**: Left Stick Click (L3)
- **9**: Right Stick Click (R3)
- **12-15**: D-pad (Up, Down, Left, Right)

### Analog Stick Bindings

```json
{
  "move": {
    "type": "stick",
    "stick": "left",
    "deadzone": 0.15,
    "sensitivity": 1.0,
    "comment": "Left stick for movement"
  },
  "look": {
    "type": "stick",
    "stick": "right",
    "deadzone": 0.15,
    "sensitivity": 1.2,
    "invert_y": false,
    "comment": "Right stick for camera"
  }
}
```

#### Stick Properties

- **`stick`**: `"left"` or `"right"`
- **`deadzone`**: Ignore input below this value (0.0 - 1.0)
- **`sensitivity`**: Input multiplier
- **`invert_y`**: Invert vertical axis (optional)

### Trigger Bindings

```json
{
  "shoot": {
    "type": "trigger",
    "trigger": "right",
    "threshold": 0.1,
    "comment": "Right trigger"
  },
  "aim": {
    "type": "trigger",
    "trigger": "left",
    "threshold": 0.1
  }
}
```

#### Trigger Properties

- **`trigger`**: `"left"` or `"right"`
- **`threshold`**: Minimum value to register input (0.0 - 1.0)

### Gamepad Settings

```json
{
  "settings": {
    "deadzone": 0.15,
    "sensitivity": 1.0
  }
}
```

- **`deadzone`**: Global deadzone for all analog inputs
- **`sensitivity`**: Global sensitivity multiplier

## Action Mapping

Action maps group actions by context and priority:

```json
{
  "action_maps": [
    {
      "name": "gameplay",
      "context": "Gameplay",
      "priority": 0,
      "exclusive": false,
      "description": "Core gameplay actions",
      "actions": [
        "move_up", "move_down", "move_left", "move_right",
        "jump", "shoot", "aim", "interact", "sprint"
      ]
    },
    {
      "name": "ui_navigation",
      "context": "UI",
      "priority": 30,
      "exclusive": true,
      "description": "Menu navigation",
      "actions": [
        "ui_up", "ui_down", "ui_left", "ui_right",
        "ui_confirm", "ui_cancel"
      ]
    },
    {
      "name": "system",
      "context": "System",
      "priority": 100,
      "exclusive": false,
      "description": "System controls",
      "actions": [
        "screenshot", "toggle_fps", "toggle_debug", "menu_open"
      ]
    }
  ]
}
```

### Action Map Properties

- **`name`**: Unique identifier
- **`context`**: Input context (`Gameplay`, `UI`, `System`)
- **`priority`**: Higher priority maps are checked first
- **`exclusive`**: If true, blocks lower-priority maps when active
- **`actions`**: List of action names in this map

### Context Priority

When multiple contexts are active, priority determines processing order:

1. **System** (priority 100): Always processed
2. **UI** (priority 30): Active when menus are open (exclusive)
3. **Gameplay** (priority 0): Active during normal gameplay

Exclusive maps block lower-priority maps from processing input.

## Default Profile Assignment

Automatically assign profiles based on device type:

```json
{
  "default_profile_assignment": {
    "Joystick": "default_gamepad",
    "KeyboardMouse": "default_keyboard"
  }
}
```

When a device connects, it's automatically assigned the appropriate profile.

## System Settings

```json
{
  "settings": {
    "auto_assign_devices": true,
    "prefer_joysticks": true,
    "enable_hot_reload": false,
    "log_level": "info",
    "device_reconnect_timeout_ms": 5000
  }
}
```

- **`auto_assign_devices`**: Automatically assign profiles to new devices
- **`prefer_joysticks`**: Prefer gamepads over keyboard when available
- **`enable_hot_reload`**: Reload config when file changes (future feature)
- **`log_level`**: Logging verbosity
- **`device_reconnect_timeout_ms`**: Time to wait before removing disconnected device

## Loading Configuration

### In Code

```cpp
#include "InputsManager.h"

// Initialize with default config path
InputsManager::Get().InitializeInputSystem("Config/olympe-config.json");

// Or specify custom path
InputsManager::Get().InitializeInputSystem("Config/custom_inputs.json");
```

### Automatic Loading

The engine loads input configuration during startup:

```cpp
// In GameEngine::Initialize()
InputsManager::Get().InitializeInputSystem(configPath);
```

## Multi-Player Support

The input system supports multiple players with different devices:

- **Player 1**: Keyboard or first gamepad
- **Player 2+**: Additional gamepads

Each player gets a separate `Controller_data` component with their device bindings.

### Player Binding

```cpp
// Bind player to keyboard
InputsManager::Get().BindPlayerToDevice(0, DeviceType::Keyboard);

// Bind player to specific gamepad
InputsManager::Get().BindPlayerToDevice(1, joystickID);
```

## Runtime Input Queries

### Check Action State

```cpp
// Get controller component
Controller_data* controller = World::Get().GetComponent<Controller_data>(playerEntity);

// Check if action is pressed
if (controller->actions["jump"]) {
    // Jump logic
}

// Get analog input
float moveX = controller->axes["move_x"];
float moveY = controller->axes["move_y"];
```

### Input Events

The input system publishes events to `EventQueue`:

```cpp
// Subscribe to input events
EventQueue::Subscribe("Input", [](const Event& event) {
    if (event.type == "ButtonPressed") {
        std::string action = event.GetString("action");
        int playerID = event.GetInt("playerID");
        // Handle button press
    }
});
```

## Custom Profiles

### Creating a Custom Profile

1. Copy an existing profile in `Inputs.json`
2. Modify action bindings
3. Change `name` and `description`
4. Assign to device type

Example: Racing game profile

```json
{
  "name": "racing",
  "device_type": "KeyboardMouse",
  "actions": {
    "accelerate": {"type": "key", "primary": "W"},
    "brake": {"type": "key", "primary": "S"},
    "steer_left": {"type": "key", "primary": "A"},
    "steer_right": {"type": "key", "primary": "D"},
    "handbrake": {"type": "key", "primary": "SPACE"},
    "nitro": {"type": "key", "primary": "LSHIFT"}
  }
}
```

### Switching Profiles

```cpp
// Switch to custom profile
InputDeviceManager::Get().AssignProfileToDevice(deviceID, "racing");
```

## Troubleshooting

### Keys Not Responding
- Check key names match SDL scancode names
- Verify profile is assigned to device
- Check action map context is active

### Gamepad Not Detected
- Ensure gamepad is connected before starting engine
- Check `GetConnectedJoysticksCount()` > 0
- Try different USB port

### Actions Not Working
- Verify action name in profile matches component usage
- Check action map includes the action
- Ensure correct context is active

### Input Lag
- Reduce `device_reconnect_timeout_ms`
- Enable `vsync` for consistent frame timing
- Check for blocking operations in input handlers

## Best Practices

1. **Use Descriptive Names**: Clear action names improve maintainability
2. **Document Custom Bindings**: Add comments to explain non-obvious mappings
3. **Test All Devices**: Verify keyboard, mouse, and gamepad configurations
4. **Provide Alternatives**: Offer alternate key bindings for flexibility
5. **Respect Conventions**: Follow standard control schemes (WASD, Xbox layout)
6. **Context Separation**: Use action maps to separate gameplay/UI/system controls
7. **Deadzone Tuning**: Adjust deadzones for specific gamepads if needed

## Related Documentation

- [Input System Architecture](./input-architecture.md) - Technical implementation details
- [Input System User Guide](./input-user-guide.md) - How to use input in gameplay
- [ECS Overview](../../technical-reference/architecture/ecs-overview.md) - Controller_data component details

## Example Files

- **`Config/olympe-config.json`**: Main engine configuration
- **`Config/Inputs.json`**: Complete input profile example
- **`Source/InputConfigLoader.cpp`**: Profile loading implementation
- **`Source/InputsManager.cpp`**: Input system implementation
