# Input System

The Olympe Engine input system provides flexible, multi-device input management with support for keyboards, mice, and game controllers.

## Quick Start

### 1. Initialize the Input System

In your game initialization code:

```cpp
// Initialize the input system with configuration
InputsManager::Get().InitializeInputSystem("Config/olympe-config.json");
```

### 2. Configure Input Profiles

Create or edit `Config/Inputs.json` to define your input profiles:

```json
{
  "version": "2.0",
  "profiles": [
    {
      "name": "default_keyboard",
      "device_type": "KeyboardMouse",
      "actions": {
        "move_up": {"type": "key", "primary": "W", "alternate": "UP"},
        "jump": {"type": "key", "primary": "SPACE"},
        "shoot": {"type": "mouse_button", "button": 1}
      }
    },
    {
      "name": "default_gamepad",
      "device_type": "Joystick",
      "actions": {
        "move": {"type": "stick", "stick": "left", "deadzone": 0.15},
        "jump": {"type": "button", "button": 0}
      }
    }
  ]
}
```

### 3. Assign Devices to Players

Devices are automatically assigned to players when they join:

```cpp
// Auto-assign first available device to player
InputsManager::Get().AutoBindControllerToPlayer(playerID);

// Or use new device manager directly
InputDeviceManager::Get().AutoAssignDevice(playerID);
```

### 4. Process Input in ECS Systems

The `InputMappingSystem` automatically reads input profiles and updates player controller components:

```cpp
// In your gameplay code, just read the mapped actions:
if (playerCtrl.isJumping) {
    // Handle jump
}

if (playerCtrl.Joydirection.x != 0 || playerCtrl.Joydirection.y != 0) {
    // Handle movement
}
```

## Features

### Multi-Device Support
- Simultaneous keyboard, mouse, and gamepad support
- Hot-plugging support (connect/disconnect at runtime)
- Auto-assignment with preference: joysticks → keyboard-mouse

### Context-Aware Input
- **Gameplay Context**: Normal game controls
- **UI Context**: Menu navigation (blocks gameplay input)
- **Editor Context**: Editor tools (requires `editor_enabled: true`)
- **System Context**: Always-active controls (screenshots, debug)

### Input Profiles
- Device-specific configurations
- Per-action bindings with primary and alternate keys
- Deadzone and sensitivity settings
- Keyboard overlap validation

### Action Maps
- Logical grouping of actions by context
- Priority-based processing (higher priority first)
- Exclusive mode (blocks lower priority maps)

### Configuration System
- JSON-based configuration
- User overrides (`Inputs.user.json`)
- Runtime profile switching
- Hot-reload support (planned)

## Documentation

### For Users
- **[User Guide](../Documentation/Input/INPUT_USER_GUIDE.md)** - How to configure and use the input system
- **[JSON Schemas](../Documentation/Input/INPUT_SCHEMAS.md)** - Complete reference for configuration files

### For Developers
- **[Architecture](../Documentation/Input/INPUT_ARCHITECTURE.md)** - Technical architecture and design
- **[API Reference](../Documentation/Input/INPUT_API.md)** - Code-level API documentation (coming soon)

## Example Configuration Files

### Minimal Setup (`Config/Inputs.json`)

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
        "jump": {"type": "key", "primary": "SPACE"}
      }
    }
  ],
  "default_profile_assignment": {
    "KeyboardMouse": "default_keyboard"
  }
}
```

### Engine Configuration (`Config/olympe-config.json`)

```json
{
  "screen_width": 1280,
  "screen_height": 720,
  "editor_enabled": false,
  "input_config_path": "Config/Inputs.json",
  "input_log_level": "info"
}
```

### User Overrides (`Config/Inputs.user.json`)

```json
{
  "version": "2.0",
  "profile_overrides": [
    {
      "profile": "default_keyboard",
      "actions": {
        "jump": {"type": "key", "primary": "LSHIFT"}
      }
    }
  ]
}
```

## API Overview

### InputDeviceManager

```cpp
// Get device for a player
InputDeviceSlot* device = InputDeviceManager::Get().GetDeviceForPlayer(playerID);

// Auto-assign device
InputDeviceSlot* assigned = InputDeviceManager::Get().AutoAssignDevice(playerID);

// Manual assignment
InputDeviceManager::Get().AssignDeviceToPlayer(deviceIndex, playerID);

// Get available devices
auto devices = InputDeviceManager::Get().GetAvailableDevices();
```

### InputProfile

```cpp
// Get profile from device
if (device && device->profile) {
    auto jumpBinding = device->profile->GetActionBinding("jump");
    float deadzone = device->profile->deadzone;
}
```

### InputContextManager

```cpp
// Switch contexts
InputContextManager::Get().PushContext(ActionMapContext::UI);
InputContextManager::Get().PopContext();

// Check active context
if (InputContextManager::Get().GetActiveContext() == ActionMapContext::Gameplay) {
    // Process gameplay input
}

// Enable/disable editor
InputContextManager::Get().SetEditorEnabled(true);
```

### InputConfigLoader

```cpp
// Load configurations
InputConfigLoader::Get().LoadEngineConfig("Config/olympe-config.json");
InputConfigLoader::Get().LoadInputConfig("Config/Inputs.json");
InputConfigLoader::Get().LoadProfileOverride("Config/Inputs.user.json");
```

## Backward Compatibility

The new system maintains full backward compatibility with the old `InputMapping_data` component system:

```cpp
// Old way (still works)
InputMapping_data& mapping = World::Get().GetComponent<InputMapping_data>(entity);
if (km.IsKeyHeld(mapping.keyboardBindings["jump"])) { ... }

// New way (preferred)
InputDeviceSlot* device = InputDeviceManager::Get().GetDeviceForPlayer(playerID);
auto jumpBinding = device->profile->GetActionBinding("jump");
if (jumpBinding && km.IsKeyHeld(static_cast<SDL_Scancode>(jumpBinding->primaryInput))) { ... }
```

The `InputMappingSystem` automatically uses the new profile system when available, falling back to `InputMapping_data` if not.

## Migration Guide

### From Old System to New System

1. **Create Input Configuration File**
   ```bash
   cp Examples/Inputs.json Config/Inputs.json
   ```

2. **Update Initialization**
   ```cpp
   // Add to your game initialization
   InputsManager::Get().InitializeInputSystem();
   ```

3. **Remove Manual InputMapping_data Setup** (Optional)
   ```cpp
   // Old: Manually creating InputMapping_data components
   // New: Profiles loaded from JSON automatically
   ```

4. **Test Your Game**
   - Verify all inputs work as expected
   - Check logs for any warnings
   - Test with multiple devices

### Keeping Old System (Not Recommended)

If you don't call `InitializeInputSystem()`, the system falls back to the old `InputMapping_data` system automatically.

## Troubleshooting

### Problem: Input not responding

**Check:**
1. Is the input system initialized? `InputsManager::Get().InitializeInputSystem()`
2. Are devices registered? Check logs for `[InputDevice][Info] Device registered`
3. Is the correct context active? Check `InputContextManager::Get().GetActiveContext()`

**Solution:**
```bash
# Enable debug logging in olympe-config.json
"input_log_level": "debug"
```

### Problem: Keyboard overlap detected

**Check logs for:**
```
[InputProfile][Error] Keyboard profile 'default_keyboard' has overlapping keys
```

**Solution:**
Ensure each action uses a unique key in your `Inputs.json`:
```json
{
  "jump": {"type": "key", "primary": "SPACE"},
  "interact": {"type": "key", "primary": "E"}  // Don't use SPACE
}
```

### Problem: Editor shortcuts conflict with gameplay

**Solution:**
Use separate action maps with proper priorities:
```json
{
  "action_maps": [
    {
      "name": "editor_tools",
      "context": "Editor",
      "priority": 50,
      "exclusive": true
    },
    {
      "name": "gameplay",
      "context": "Gameplay",
      "priority": 0,
      "exclusive": false
    }
  ]
}
```

## Performance

- Device slot lookup: O(1) hash map
- Action binding lookup: O(1) hash map
- Context switching: O(1) vector operations
- Profile validation: O(n) where n = number of actions (done once at load)

## Future Enhancements

- [ ] Hot-reload input configuration
- [ ] Runtime profile editor UI
- [ ] Touch input support
- [ ] Network input synchronization
- [ ] Input recording/playback

## Support

For issues, questions, or contributions:
1. Check the [Documentation](../Documentation/Input/)
2. Review log output (`input_log_level: "debug"`)
3. Create an issue with complete configuration and logs

## License

Part of Olympe Engine V2 - 2026
