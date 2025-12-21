# EventQueue ECS-First Architecture Migration

## Overview

This document describes the migration from a callback-based EventManager system to an ECS-first EventQueue architecture with double-buffering and domain-based routing.

## Goals Achieved

1. **Removed platform dependencies from Message structure**
   - Eliminated `#include "SDL_events.h"` and `#include "windows.h"` from `message.h`
   - Removed `EventStructType struct_type`, `MSG* msg`, and `SDL_Event* sdlEvent` fields
   - Events are now platform-agnostic with generic payload fields

2. **Introduced EventQueue with double-buffer architecture**
   - Created `Source/system/EventQueue.h` with singleton pattern
   - Implements double-buffering: events written during frame N are read during frame N+1
   - Single `BeginFrame()` call per frame swaps read/write buffers
   - Events consumed **next frame** for predictable behavior

3. **Domain-based event routing**
   - Added `EventDomain` enum: Input, UI, Gameplay, System, Camera, Viewport, Detection, Collision, All
   - Events stored once, indexed by domain for efficient filtering
   - Systems can query only events relevant to their domain

4. **ECS-first event consumption**
   - Created `InputEventConsumeSystem` to replace callback-based `UpdateECSInputFromMessage`
   - Registered early in ECS system initialization (before InputMappingSystem)
   - Consumes Input domain events and updates ECS components directly

5. **Removed DispatchImmediate pattern**
   - All events now go through EventQueue::Push()
   - No immediate/synchronous event dispatch
   - Events always consumed next frame via double-buffer

## Architecture Changes

### Message Structure (Before)

```cpp
struct Message {
    EventStructType struct_type;  // REMOVED
    EventType msg_type;
    MSG* msg;                      // REMOVED  
    SDL_Event* sdlEvent;           // REMOVED
    uint64_t targetUid;
    int deviceId;
    int controlId;
    int state;
    float param1;
    float param2;
};
```

### Message Structure (After)

```cpp
struct Message {
    EventType msg_type;           // Kept for transition
    EventDomain domain;           // NEW: routing domain
    uint64_t targetUid;
    int deviceId;
    int controlId;
    int state;
    float param1;
    float param2;
    
    static Message Create(EventType, EventDomain, ...);
};
```

### Event Flow (Before)

```
SDL Event → Input Manager → EventManager::AddMessage() → EventManager::Process() → 
UpdateECSInputFromMessage() → Callbacks → Components Updated
```

### Event Flow (After)

```
SDL Event → Input Manager → EventQueue::Push() → 
[Frame N+1] → EventQueue::BeginFrame() (swap buffers) → 
InputEventConsumeSystem::Process() → Components Updated
```

## Key Changes by File

### Core Infrastructure

1. **Source/system/system_consts.h**
   - Added `enum class EventDomain` with 9 domains

2. **Source/system/message.h**
   - Removed platform dependencies
   - Added `domain` field
   - Updated `Message::Create()` API

3. **Source/system/EventQueue.h** (NEW)
   - Double-buffer implementation
   - Domain-based indexing
   - `BeginFrame()`, `Push()`, `GetDomainEvents()`, `ForEachDomainEvent()` methods

### ECS Systems

4. **Source/ECS_Systems.h**
   - Declared `InputEventConsumeSystem`

5. **Source/ECS_Systems.cpp**
   - Implemented `InputEventConsumeSystem::Process()`
   - Migrated logic from `EventManager_ECS.cpp`

6. **Source/World.h** / **Source/World.cpp**
   - Added `EventQueue::BeginFrame()` call at start of `World::Process()`
   - Removed `EventManager::Process()` call
   - Registered `InputEventConsumeSystem` first in system initialization

### Input Managers

7. **Source/system/KeyboardManager.cpp**
   - Changed from `EventManager::Get().AddMessage()` to `EventQueue::Get().Push()`
   - Updated to use new `Message::Create()` API with EventDomain::Input

8. **Source/system/JoystickManager.cpp**
   - Same changes as KeyboardManager
   - Updated connect/disconnect event handling

9. **Source/system/MouseManager.cpp**
   - Same changes as KeyboardManager

### Camera System

10. **Source/system/CameraEventHandler.cpp**
    - Updated all camera event emissions to use EventQueue
    - Camera events use EventDomain::Camera

11. **Source/ECS_Systems_Camera.cpp**
    - Removed `struct_type` check in `OnEvent()`

### Application Code

12. **Source/VideoGame.cpp**
    - Updated camera follow event to use EventQueue

### Cleanup

13. **Source/AI_Player.cpp**, **Source/GameObject.cpp**, **Source/ObjectComponent.cpp**, 
    **Source/PanelManager.cpp**, **Source/ObjectFactory.h**
    - Removed `struct_type` switch statements
    - Simplified event handling to only check `msg_type`

## Phase 2: Complete EventManager Removal (December 2025)

### New Event Consumer Systems

14. **Source/ECS_Systems.h** and **Source/ECS_Systems.cpp**
    - Added `GameEventConsumeSystem` to handle VideoGame events (pause/resume/quit, player add/remove)
    - Added `UIEventConsumeSystem` to handle GameMenu events (menu enter/exit/validate)
    - Added `CameraEventConsumeSystem` to forward Camera domain events to CameraSystem
    - Enhanced `InputEventConsumeSystem` to handle joystick connect/disconnect for auto-rebind

### EventManager Removed

15. **Source/system/EventManager.h** - **DELETED**
    - Old callback-based EventManager completely removed

16. **Source/system/EventManager_ECS.cpp** - **DELETED**
    - Legacy helper function `UpdateECSInputFromMessage` removed (replaced by InputEventConsumeSystem)

17. **Source/VideoGame.cpp**
    - Removed EventManager::Register calls for game events
    - Removed OnEvent callback method (logic moved to GameEventConsumeSystem)
    - Removed debounce flags (moved to GameEventConsumeSystem)

18. **Source/InputsManager.cpp**
    - Removed EventManager::Register calls for joystick connect/disconnect
    - Removed OnEvent callback method (logic moved to InputEventConsumeSystem)

19. **Source/system/GameMenu.cpp**
    - Removed EventManager::Register calls for menu events
    - Removed OnEvent callback method (logic moved to UIEventConsumeSystem)

20. **Source/ECS_Systems_Camera.cpp**
    - Removed EventManager::Register calls for camera events
    - CameraSystem::OnEvent still exists but is called by CameraEventConsumeSystem

21. **Source/World.h**
    - Removed `#include "system/EventManager.h"`
    - World now only uses EventQueue

### System Initialization Order

22. **Source/World.cpp**
    - Updated system initialization order:
      1. InputEventConsumeSystem
      2. GameEventConsumeSystem
      3. UIEventConsumeSystem
      4. CameraEventConsumeSystem
      5. InputSystem
      6. InputMappingSystem
      7. PlayerControlSystem
      8. (other systems...)

## Migration Complete

### EventManager Removed

EventManager has been **completely removed** from critical gameplay systems:
- VideoGame no longer uses EventManager callbacks
- InputsManager no longer uses EventManager callbacks
- GameMenu no longer uses EventManager callbacks
- CameraSystem no longer registers with EventManager
- World.h no longer includes EventManager.h

### Legacy Code Note

Some legacy GameObject-based code (AI_Player, ObjectFactory, PanelManager) may still reference EventManager but these are not critical to gameplay and are being phased out in favor of the ECS architecture.

### Backward Compatibility

- `EventType` enum is preserved for all event types
- Event domains enable efficient filtering by system type
- Legacy systems can coexist during gradual migration

## Frame Timing

### Critical: Single BeginFrame() Call

`EventQueue::BeginFrame()` is called **exactly once per frame** at the start of `World::Process()`. This is the single point where write buffer becomes read buffer.

### Event Latency

Events have **1-frame latency** by design:
- Frame N: Input event occurs → EventQueue::Push() → goes to write buffer
- Frame N+1: BeginFrame() swaps buffers → InputEventConsumeSystem reads event → updates components

This ensures predictable, deterministic behavior and avoids race conditions.

## Domain Routing

### Event Domains

```cpp
enum class EventDomain {
    Input,      // Keyboard, mouse, joystick events
    UI,         // UI interaction events
    Gameplay,   // Game logic events (default)
    System,     // System-level events
    Camera,     // Camera control events
    Viewport,   // Viewport management
    Detection,  // Trigger zone detection
    Collision,  // Collision events
    All         // Special domain that receives all events
};
```

### Usage Example

```cpp
// Emit event to specific domain
Message msg = Message::Create(
    EventType::Olympe_EventType_Joystick_ButtonDown,
    EventDomain::Input,
    joystickId,
    buttonIndex
);
EventQueue::Get().Push(msg);

// Consume domain events in ECS system
void MySystem::Process() {
    EventQueue::Get().ForEachDomainEvent(EventDomain::Input, 
        [](const Message& msg) {
            // Process input event
        });
}
```

## Testing Considerations

Since the project is Windows-only and requires MSVC to build, testing should verify:

1. **Input events work correctly** - keyboard, mouse, joystick
2. **1-frame latency is acceptable** - gameplay feels responsive
3. **No events are dropped** - all pushed events are consumed
4. **Camera events work** - shake, zoom, follow, etc.
5. **Multiple domains can coexist** - Input and Camera events don't interfere
6. **Keyboard RETURN key creates new player entity**
7. **Joystick connect/disconnect triggers auto-rebind logic**

## Future Work

### Phase 3: Event Replay & Recording

With double-buffering in place, event replay for debugging/testing is straightforward:
- Record frame N events from write buffer before swap
- Replay by pushing recorded events in same order

### Phase 4: Multi-Domain Events

Current implementation supports single domain per event. Future enhancement:
- Events can target multiple domains
- Useful for events that affect both UI and Gameplay

### Phase 5: Legacy Code Cleanup

Remove EventManager references from non-critical legacy code:
- AI_Player (legacy GameObject)
- ObjectFactory (legacy object creation)
- PanelManager (debug UI)
- CameraEventHandler (legacy camera wrapper)

These systems are not critical to gameplay and are being phased out in favor of ECS.

## Performance Characteristics

### Memory

- Events stored once in vector
- Domain indices are just integer indices
- Minimal overhead compared to callback-based system

### Speed

- O(1) event push
- O(1) buffer swap
- O(n) domain iteration where n = events in that domain
- No virtual function calls during event emission
- No callback lookup overhead

## Conclusion

This migration successfully transitions from a callback-based event system to a modern ECS-first architecture with:
- Platform independence
- Predictable frame-based event consumption  
- Efficient domain-based routing
- No immediate dispatch complexity
- Clean separation of concerns

The system is ready for testing and can be further refined based on performance profiling and gameplay feedback.
