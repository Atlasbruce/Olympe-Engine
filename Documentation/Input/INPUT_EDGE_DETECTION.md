# Input Edge Detection Guide

## Overview

The Controller_data component now includes built-in edge detection for button presses and releases. This allows game systems to distinguish between:

- **Pressed** - Button transitioned from up to down this frame
- **Released** - Button transitioned from down to up this frame  
- **Held** - Button is currently down (may have been pressed multiple frames ago)

## Architecture

### State Management

Each `Controller_data` component maintains:

```cpp
// Current frame state
bool buttons[MAX_BUTTONS];

// Previous frame state  
bool buttons_prev[MAX_BUTTONS];

// Edge detection (computed each frame)
bool buttonsPressed[MAX_BUTTONS];  // false->true transition
bool buttonsReleased[MAX_BUTTONS]; // true->false transition
```

### Frame Processing Order

The input system processes in this order each frame:

1. **InputSystem::Process()** - Calls `UpdateEdgeDetection()` on all Controller_data components
2. **InputMappingSystem::Process()** - Reads input and populates PlayerController_data
3. **Other Systems** - Use PlayerController_data for gameplay logic
4. **InputMappingSystem::Process() (end)** - Calls `CommitState()` to save current→previous

## API Reference

### Query Methods

```cpp
// Check if button was just pressed this frame (edge detection)
bool IsButtonPressed(int buttonIndex) const;

// Check if button was just released this frame (edge detection)  
bool IsButtonReleased(int buttonIndex) const;

// Check if button is currently held down
bool IsButtonHeld(int buttonIndex) const;
```

### State Management Methods

```cpp
// Update edge detection state - call at START of frame
void UpdateEdgeDetection();

// Commit current state to previous - call at END of frame
void CommitState();
```

## Usage Examples

### Example 1: Jump on Button Press

```cpp
void PlayerControlSystem::Process()
{
    for (EntityID entity : m_entities)
    {
        Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
        PlayerController_data& player = World::Get().GetComponent<PlayerController_data>(entity);
        
        // Jump only when button is first pressed (not while held)
        if (ctrl.IsButtonPressed(0))  // A button
        {
            player.isJumping = true;
            ApplyJumpImpulse(entity);
        }
    }
}
```

### Example 2: Charge Attack

```cpp
void WeaponSystem::Process()
{
    for (EntityID entity : m_entities)
    {
        Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
        WeaponState& weapon = World::Get().GetComponent<WeaponState>(entity);
        
        // Start charging when button pressed
        if (ctrl.IsButtonPressed(1))  // B button
        {
            weapon.chargeStartTime = GameEngine::time;
        }
        
        // Continue charging while held
        if (ctrl.IsButtonHeld(1))
        {
            weapon.chargeAmount = (GameEngine::time - weapon.chargeStartTime) / weapon.maxChargeTime;
        }
        
        // Release attack when button released
        if (ctrl.IsButtonReleased(1))
        {
            FireChargedAttack(entity, weapon.chargeAmount);
            weapon.chargeAmount = 0.0f;
        }
    }
}
```

### Example 3: Menu Navigation

```cpp
void MenuSystem::Process()
{
    Controller_data& ctrl = /* get controller */;
    
    // Use pressed for single-step menu navigation
    if (ctrl.IsButtonPressed(DPAD_DOWN))
    {
        selectedIndex = (selectedIndex + 1) % menuItems.size();
    }
    
    if (ctrl.IsButtonPressed(DPAD_UP))
    {
        selectedIndex = (selectedIndex - 1 + menuItems.size()) % menuItems.size();
    }
    
    // Use pressed for selection confirmation
    if (ctrl.IsButtonPressed(BUTTON_A))
    {
        ActivateMenuItem(selectedIndex);
    }
}
```

## Integration with InputMappingSystem

The InputMappingSystem automatically uses edge detection for action mappings:

```cpp
void InputMappingSystem::Process()
{
    // ... input processing ...
    
    // Gamepad actions use Controller_data edge detection
    pctrl.isJumping = ctrl.IsButtonHeld(0);      // Continuous
    pctrl.isInteracting = ctrl.IsButtonPressed(2); // One-shot
    
    // ... end of processing ...
    
    // Commit state for next frame's edge detection
    for (EntityID entity : m_entities)
    {
        Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
        ctrl.CommitState();
    }
}
```

## Network Serialization

The Controller_data structure is designed to be network-serializable:

### Minimal Network Packet

For network play, you only need to send:
- `buttons[MAX_BUTTONS]` - Current button states
- `leftStick`, `rightStick` - Analog stick positions
- `leftTrigger`, `rightTrigger` - Trigger values

The receiving client can reconstruct edge detection locally by maintaining its own previous state.

### Example Serialization

```cpp
struct NetworkInputPacket
{
    uint16_t buttons;  // Packed bit flags for 16 buttons
    int8_t leftStick_x, leftStick_y;    // Quantized -127..127
    int8_t rightStick_x, rightStick_y;  // Quantized -127..127
    uint8_t leftTrigger, rightTrigger;  // Quantized 0..255
};
// Total: 8 bytes per player per frame
```

## Performance Considerations

### Memory Usage

Per Controller_data component:
- Current state: 16 buttons + 6 axis values = ~40 bytes
- Previous state: 16 buttons + 6 axis values = ~40 bytes  
- Edge detection: 32 button flags = 32 bytes
- **Total: ~112 bytes per controller**

### CPU Cost

Edge detection computation per frame:
- 16 boolean comparisons per controller
- ~O(1) constant time
- Negligible overhead compared to input polling

### Optimization Tips

1. **Cache InputEntities** - Use InputsManager's entity cache to avoid iterating all entities
2. **Early Exit** - Skip disconnected controllers
3. **Batch Processing** - Process all input in InputSystem before gameplay systems

## Migration from Old Code

### Before (Manual Edge Detection)

```cpp
// Old approach - manual state tracking
struct PlayerState
{
    bool jumpButtonPrev = false;
};

void Update()
{
    bool jumpButton = GetButton(0);
    if (jumpButton && !state.jumpButtonPrev)
    {
        Jump();  // Button pressed
    }
    state.jumpButtonPrev = jumpButton;
}
```

### After (Built-in Edge Detection)

```cpp
// New approach - use Controller_data
void Update()
{
    Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
    
    if (ctrl.IsButtonPressed(0))
    {
        Jump();  // Button pressed
    }
    // No manual state tracking needed!
}
```

## Troubleshooting

### Issue: Buttons not detecting presses

**Cause**: Edge detection not called before reading
**Solution**: Ensure InputSystem runs before your gameplay systems

### Issue: Multiple presses detected per actual press

**Cause**: CommitState() not called at end of frame
**Solution**: Ensure InputMappingSystem calls CommitState() after all input processing

### Issue: Pressed state persists across frames

**Cause**: UpdateEdgeDetection() not called each frame
**Solution**: Verify InputSystem is in the system processing list and runs first

## Best Practices

1. **Use IsButtonPressed() for one-shot actions** (jump, shoot, interact)
2. **Use IsButtonHeld() for continuous actions** (run, charge, aim)
3. **Use IsButtonReleased() for end-of-action** (release charged attack)
4. **Don't mix edge detection with event handling** - Pick one approach per button
5. **Process input in consistent order** - InputSystem → InputMapping → Gameplay

## See Also

- [Input System Guide](INPUT_SYSTEM_GUIDE.md) - Overview of the input system
- [Input API Reference](INPUT_API_REFERENCE.md) - Complete API documentation
- [Input Examples](INPUT_EXAMPLES.md) - More practical examples
- [ECS Components](../../Source/ECS_Components.h) - Controller_data definition

---

*Olympe Engine V2 - 2025*
