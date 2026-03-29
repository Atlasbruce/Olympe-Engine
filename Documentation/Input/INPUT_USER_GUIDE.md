# Input System User Guide

**Version:** 2.0  
**Last Updated:** 2026-02-03

## Introduction

Welcome to the Olympe Engine Input System! This guide will help you understand how to configure and use the input system for your game.

## Quick Start

### Basic Setup

1. **Create Input Configuration File**

   Create a file named `Inputs.json` in your `Config/` directory:

   ```json
   {
     "version": "2.0",
     "profiles": [
       {
         "name": "default_gamepad",
         "device_type": "Joystick",
         "actions": {
           "move_horizontal": {
             "type": "axis",
             "axis": 0,
             "deadzone": 0.15
           },
           "move_vertical": {
             "type": "axis",
             "axis": 1,
             "deadzone": 0.15
           },
           "jump": {
             "type": "button",
             "button": 0
           },
           "shoot": {
             "type": "button",
             "button": 1
           }
         }
       }
     ]
   }
   ```

2. **Enable Input System**

   In your `olympe-config.json`:

   ```json
   {
     "screen_width": 1280,
     "screen_height": 720,
     "editor_enabled": false,
     "input_config_path": "Config/Inputs.json"
   }
   ```

3. **Run Your Game**

   The input system will automatically load your configuration and detect connected devices!

## Understanding Input Concepts

### Devices

The input system supports two main device types:

- **Joystick/Gamepad**: Physical game controllers (Xbox, PlayStation, generic gamepads)
- **KeyboardMouse**: Combined keyboard and mouse input as a single device

### Profiles

An **Input Profile** defines how buttons, keys, and axes map to game actions for a specific device type.

Example: A "default_gamepad" profile might map:
- Left stick → Movement
- A button → Jump
- Right trigger → Shoot

### Actions

**Actions** are logical inputs in your game, independent of physical hardware:
- `move_horizontal`, `move_vertical` - Character movement
- `jump` - Jump action
- `shoot` - Fire weapon
- `interact` - Use/pick up objects
- `menu_open` - Open pause menu

### Contexts

**Contexts** determine which inputs are active at any time:
- **Gameplay**: Normal game controls
- **UI**: Menu navigation
- **Editor**: Editor tools (when `editor_enabled: true`)
- **System**: Always-active system controls (screenshot, quit)

## Configuring Input Profiles

### Keyboard Profile Example

```json
{
  "name": "default_keyboard",
  "device_type": "KeyboardMouse",
  "actions": {
    "move_up": {
      "type": "key",
      "primary": "W",
      "alternate": "UP"
    },
    "move_down": {
      "type": "key",
      "primary": "S",
      "alternate": "DOWN"
    },
    "move_left": {
      "type": "key",
      "primary": "A",
      "alternate": "LEFT"
    },
    "move_right": {
      "type": "key",
      "primary": "D",
      "alternate": "RIGHT"
    },
    "jump": {
      "type": "key",
      "primary": "SPACE"
    },
    "shoot": {
      "type": "key",
      "primary": "LCTRL"
    },
    "interact": {
      "type": "key",
      "primary": "E"
    }
  },
  "settings": {
    "validate_overlaps": true
  }
}
```

**Key Names**: Use SDL key names like `W`, `A`, `S`, `D`, `SPACE`, `LCTRL`, `LSHIFT`, `ESCAPE`, `UP`, `DOWN`, `LEFT`, `RIGHT`, etc.

### Gamepad Profile Example

```json
{
  "name": "xbox_controller",
  "device_type": "Joystick",
  "actions": {
    "move": {
      "type": "stick",
      "stick": "left",
      "deadzone": 0.15,
      "sensitivity": 1.0
    },
    "look": {
      "type": "stick",
      "stick": "right",
      "deadzone": 0.15,
      "sensitivity": 1.2,
      "invert_y": false
    },
    "jump": {
      "type": "button",
      "button": 0,
      "comment": "A button on Xbox"
    },
    "shoot": {
      "type": "button",
      "button": 1,
      "comment": "B button on Xbox"
    },
    "aim": {
      "type": "trigger",
      "trigger": "left",
      "threshold": 0.1
    },
    "fire_heavy": {
      "type": "trigger",
      "trigger": "right",
      "threshold": 0.5
    }
  },
  "settings": {
    "deadzone": 0.15,
    "sensitivity": 1.0
  }
}
```

**Button Numbers** (Standard Gamepad):
- 0 = A (bottom face button)
- 1 = B (right face button)
- 2 = X (left face button)
- 3 = Y (top face button)
- 4 = Left shoulder
- 5 = Right shoulder
- 6 = Select/Back
- 7 = Start
- 8 = Left stick click
- 9 = Right stick click

## Action Maps

Action maps group related actions by context:

```json
{
  "action_maps": [
    {
      "name": "gameplay",
      "context": "Gameplay",
      "priority": 0,
      "exclusive": false,
      "actions": [
        "move_up", "move_down", "move_left", "move_right",
        "jump", "shoot", "interact"
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
        "tool_select", "tool_place", "tool_delete",
        "grid_toggle", "snap_toggle"
      ]
    },
    {
      "name": "system",
      "context": "System",
      "priority": 100,
      "exclusive": false,
      "actions": [
        "screenshot", "toggle_fps", "quit"
      ]
    }
  ]
}
```

**Priority**: Higher values process input first  
**Exclusive**: When true, prevents lower priority maps from receiving input

## Common Use Cases

### 1. Local Multiplayer Setup

For local multiplayer, the system auto-assigns devices to players:

1. **Player 1**: First connected gamepad (or keyboard if no gamepad)
2. **Player 2**: Second connected gamepad (or keyboard if Player 1 has gamepad)
3. **Player 3+**: Additional gamepads

**Auto-assignment preference**: Joysticks → KeyboardMouse

### 2. Custom Key Bindings

Allow players to rebind keys by creating a `Inputs.user.json` file:

```json
{
  "version": "2.0",
  "profile_overrides": [
    {
      "profile": "default_keyboard",
      "actions": {
        "jump": {
          "type": "key",
          "primary": "LSHIFT"
        },
        "shoot": {
          "type": "key",
          "primary": "MOUSE1"
        }
      }
    }
  ]
}
```

The system loads `Inputs.json` first, then applies overrides from `Inputs.user.json`.

### 3. Editor Mode

Enable editor controls by setting `editor_enabled: true` in `olympe-config.json`:

```json
{
  "editor_enabled": true,
  "input_config_path": "Config/Inputs.json"
}
```

When enabled:
- Editor action map becomes active
- Editor actions have higher priority than gameplay
- Can use editor shortcuts while game is running

### 4. Handling Device Disconnection

The input system automatically handles hot-plugging:

**Device Disconnected:**
- Player marked as "disconnected"
- Game can pause or show reconnect prompt
- Player's assignment preserved

**Device Reconnected:**
- System attempts to re-assign same device to player
- If specific device unavailable, prompts for manual assignment

**Logging Example:**
```
[InputDevice][Warning] Player 1 device disconnected: Xbox Controller (ID: 2)
[InputDevice][Info] Player 1 marked as disconnected, waiting for reconnect
[InputDevice][Info] Joystick connected: Xbox Controller (ID: 2)
[InputDevice][Info] Reconnected Player 1 to Xbox Controller (ID: 2)
```

## Input Validation

### Keyboard Overlap Detection

The system validates keyboard profiles to ensure no two actions use the same key:

```json
{
  "actions": {
    "jump": {
      "type": "key",
      "primary": "SPACE"
    },
    "interact": {
      "type": "key",
      "primary": "SPACE"  // ERROR: Conflicts with jump!
    }
  }
}
```

**Validation Error:**
```
[InputProfile][Error] Keyboard profile 'default_keyboard' has overlapping keys
[InputProfile][Error] Actions 'jump' and 'interact' both use key: SPACE
[InputProfile][Error] Profile validation failed, using default bindings
```

**Fix**: Use different keys or provide alternate bindings:
```json
{
  "jump": {
    "type": "key",
    "primary": "SPACE"
  },
  "interact": {
    "type": "key",
    "primary": "E",
    "alternate": "RETURN"
  }
}
```

## Complete Configuration Example

Here's a complete `Inputs.json` with all features:

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
        "shoot": {"type": "key", "primary": "MOUSE1"},
        "aim": {"type": "key", "primary": "MOUSE2"},
        "interact": {"type": "key", "primary": "E"},
        "menu_open": {"type": "key", "primary": "ESCAPE"}
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
        "jump": {"type": "button", "button": 0},
        "shoot": {"type": "button", "button": 1},
        "interact": {"type": "button", "button": 2},
        "menu_open": {"type": "button", "button": 7},
        "aim": {"type": "trigger", "trigger": "left"},
        "sprint": {"type": "button", "button": 8}
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
      "actions": ["move_up", "move_down", "move_left", "move_right", "jump", "shoot", "interact"]
    },
    {
      "name": "ui",
      "context": "UI",
      "priority": 30,
      "exclusive": true,
      "actions": ["ui_up", "ui_down", "ui_confirm", "ui_cancel"]
    },
    {
      "name": "system",
      "context": "System",
      "priority": 100,
      "exclusive": false,
      "actions": ["screenshot", "menu_open"]
    }
  ],
  "default_profile_assignment": {
    "Joystick": "default_gamepad",
    "KeyboardMouse": "default_keyboard"
  }
}
```

## Troubleshooting

### Problem: Controller not detected

**Solution:**
1. Check controller is properly connected
2. Check system logs for device detection: `[InputDevice][Info] Joystick connected: ...`
3. Try unplugging and reconnecting controller
4. Verify SDL3 gamepad database is up to date

### Problem: Keys not responding

**Solution:**
1. Verify key names match SDL key codes (case-sensitive)
2. Check for overlap warnings in logs
3. Ensure correct context is active (gameplay input disabled in UI mode)
4. Verify profile is properly loaded

### Problem: Overlapping key bindings

**Solution:**
1. Check logs for `[InputProfile][Error]` messages
2. Update `Inputs.json` to use unique keys for each action
3. Enable `validate_overlaps: true` in profile settings

### Problem: Editor shortcuts conflict with gameplay

**Solution:**
1. Use different action maps for editor vs gameplay contexts
2. Set `exclusive: true` on editor action map
3. Ensure editor actions have higher priority than gameplay

## Best Practices

### For Players

1. **Backup Your Config**: Copy `Inputs.user.json` before making changes
2. **Use Alternate Bindings**: Define alternates for important actions
3. **Test Changes**: Verify bindings work before starting long play sessions

### For Developers

1. **Provide Defaults**: Always include sensible default profiles
2. **Document Actions**: Add comments explaining what each action does
3. **Support Rebinding**: Allow players to customize controls
4. **Test Multiple Devices**: Test with keyboard, Xbox, PlayStation controllers
5. **Log Clearly**: Use descriptive log messages for debugging

## Advanced Topics

### Custom Device Profiles

Create device-specific profiles for different controller brands:

```json
{
  "profiles": [
    {
      "name": "xbox_controller",
      "device_type": "Joystick",
      "device_filter": {
        "vendor": "Microsoft",
        "product": "Xbox"
      },
      "actions": { ... }
    },
    {
      "name": "playstation_controller",
      "device_type": "Joystick",
      "device_filter": {
        "vendor": "Sony",
        "product": "DualShock"
      },
      "actions": { ... }
    }
  ]
}
```

### Composite Actions

Combine multiple inputs for complex actions:

```json
{
  "dodge_roll": {
    "type": "composite",
    "inputs": [
      {"type": "button", "button": 4, "comment": "Hold shoulder button"},
      {"type": "stick", "stick": "left", "threshold": 0.5, "comment": "Flick stick"}
    ],
    "timing": "simultaneous"
  }
}
```

### Context-Specific Bindings

Different bindings for the same action in different contexts:

```json
{
  "profiles": [
    {
      "name": "default_keyboard",
      "actions": {
        "confirm": {
          "gameplay": {"type": "key", "primary": "E"},
          "ui": {"type": "key", "primary": "RETURN"},
          "editor": {"type": "key", "primary": "MOUSE1"}
        }
      }
    }
  ]
}
```

## See Also

- [INPUT_ARCHITECTURE.md](INPUT_ARCHITECTURE.md) - Technical architecture documentation
- [INPUT_SCHEMAS.md](INPUT_SCHEMAS.md) - Complete JSON schema reference
- [SDL Key Names](https://wiki.libsdl.org/SDL3/SDL_Scancode) - Full list of SDL key codes
- [Gamepad Button Mapping](https://wiki.libsdl.org/SDL3/SDL_GamepadButton) - Standard gamepad layout

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review log output for error messages
3. Consult technical documentation
4. Report bugs with complete log output and configuration files
