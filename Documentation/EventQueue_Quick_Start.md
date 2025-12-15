# EventQueue ECS - Quick Start Guide

## 5-Minute Introduction

The **EventQueue ECS system** replaces generic message passing with **strongly-typed events**. Instead of casting generic `Message` structs, you work with specific event types like `CollisionEvent` or `InputButtonEvent`.

## Quick Example

### Post an Event (Producer)

```cpp
void MySystem::Process()
{
    // Get the event queue
    EventQueueSystem* eq = World::Get().GetEventQueueSystem();
    
    // Create a typed event
    CollisionEvent collision;
    collision.entityA = playerEntity;
    collision.entityB = enemyEntity;
    collision.started = true;
    collision.timestamp = SDL_GetTicks() / 1000.0f;
    
    // Post it
    Event evt = Event::CreateCollision(collision);
    eq->PostEvent(ECSEventType::CollisionStarted, evt);
}
```

### Consume Events (Consumer)

```cpp
void DamageSystem::Process()
{
    // Get the event queue
    EventQueueSystem* eq = World::Get().GetEventQueueSystem();
    
    // Consume all collision events
    eq->ConsumeEvents(ECSEventType::CollisionStarted,
        [this](const Event& evt) {
            const CollisionEvent& collision = evt.data.collision;
            ApplyDamage(collision.entityA, collision.entityB);
        });
}
```

## Available Event Types

### Input Events
- `InputButtonEvent` - Button press/release
- `InputAxisEvent` - Analog axis movement
- `InputActionEvent` - Abstract gameplay action

### Entity Lifecycle
- `EntityJoinedEvent` - Player joins
- `EntityLeftEvent` - Player leaves
- `EntitySpawnedEvent` - Entity created
- `EntityDestroyedEvent` - Entity destroyed

### Gameplay Events
- `CollisionEvent` - Entity collision
- `TriggerEvent` - Trigger zone entry/exit
- `HealthChangedEvent` - HP change
- `ItemCollectedEvent` - Item pickup

### System Events
- `LevelEvent` - Level load/unload
- `GameStateEvent` - Pause/resume

## Event Queue API

### Post Events
```cpp
EventQueueSystem* eq = World::Get().GetEventQueueSystem();
bool success = eq->PostEvent(ECSEventType::CollisionStarted, evt);
if (!success) {
    // Queue is full (64 event capacity)
}
```

### Consume Events (removes from queue)
```cpp
eq->ConsumeEvents(ECSEventType::TriggerEntered,
    [](const Event& evt) {
        // Process event
    });
```

### Peek Events (keeps in queue)
```cpp
eq->PeekEvents(ECSEventType::InputButton,
    [](const Event& evt) {
        // Just observe, don't remove
    });
```

### Queue Management
```cpp
size_t count = eq->GetEventCount(ECSEventType::CollisionStarted);
eq->ClearEvents(ECSEventType::CollisionStarted);  // Clear one type
eq->ClearAllEvents();  // Clear everything
```

## Entity-Local Queues

Attach an event queue to a specific entity:

```cpp
EntityID entity = World::Get().CreateEntity();
EventQueue_data& queue = World::Get().AddComponent<EventQueue_data>(entity);

// Post event to entity's queue
Event evt = Event::CreateTrigger(triggerEvt);
queue.PushEvent(evt);

// Consume from entity's queue
Event evt;
while (queue.PopEvent(evt)) {
    // Process entity-specific event
}
```

## Best Practices

### ✅ DO
- Check return value of `PostEvent` (queue might be full)
- Use timestamps for event ordering and replay
- Consume events every frame to prevent accumulation
- Use `PeekEvents` for logging/debugging without disrupting flow
- Create specific event types for your game logic

### ❌ DON'T
- Don't assume queue has infinite capacity (max 64 events)
- Don't post events during event consumption (post next frame instead)
- Don't use generic `Message` for new code (use typed events)
- Don't share event queue pointers (always get fresh via World)

## Real System Example

Here's a complete working system:

```cpp
class HealthSystem : public ECS_System
{
public:
    HealthSystem() {
        requiredSignature.set(GetComponentTypeID_Static<Health_data>(), true);
    }
    
    void Process() override
    {
        EventQueueSystem* eq = World::Get().GetEventQueueSystem();
        
        // Consume damage events
        eq->ConsumeEvents(ECSEventType::CollisionStarted,
            [this, eq](const Event& evt) {
                const CollisionEvent& col = evt.data.collision;
                
                // Apply damage if entity has health
                if (World::Get().HasComponent<Health_data>(col.entityA)) {
                    Health_data& hp = World::Get().GetComponent<Health_data>(col.entityA);
                    int oldHp = hp.currentHealth;
                    hp.currentHealth -= 10;
                    
                    // Post health changed event
                    HealthChangedEvent hpEvt;
                    hpEvt.entity = col.entityA;
                    hpEvt.oldHealth = oldHp;
                    hpEvt.newHealth = hp.currentHealth;
                    hpEvt.delta = -10;
                    hpEvt.instigator = col.entityB;
                    hpEvt.timestamp = SDL_GetTicks() / 1000.0f;
                    
                    Event newEvt = Event::CreateHealthChanged(hpEvt);
                    eq->PostEvent(ECSEventType::HealthChanged, newEvt);
                    
                    // Check for death
                    if (hp.currentHealth <= 0) {
                        EntityDestroyedEvent deathEvt;
                        deathEvt.entity = col.entityA;
                        deathEvt.reason = "health_depleted";
                        deathEvt.timestamp = SDL_GetTicks() / 1000.0f;
                        
                        Event destroyEvt = Event::CreateEntityDestroyed(deathEvt);
                        eq->PostEvent(ECSEventType::EntityDestroyed, destroyEvt);
                    }
                }
            });
    }
};
```

## Testing Your Events

Run the test suite:

```cpp
// Compile and run EventQueue_Example.cpp
// It tests all features:
// - Basic post/consume
// - Multiple events
// - Peek without consuming
// - Ring buffer overflow
// - Entity-local queues
// - All event types
```

## Migration from Old System

### Old Way (Generic Message)
```cpp
Message msg;
msg.msg_type = EventType::Olympe_EventType_Object_CollideEvent;
msg.deviceId = entityA;
msg.controlId = entityB;
EventManager::Get().AddMessage(msg);

// Consumer needs to cast and check types
void OnEvent(const Message& msg) {
    if (msg.msg_type == EventType::Olympe_EventType_Object_CollideEvent) {
        int entityA = msg.deviceId;  // Type-unsafe
        int entityB = msg.controlId;
    }
}
```

### New Way (Typed Events)
```cpp
CollisionEvent collision;
collision.entityA = entityA;
collision.entityB = entityB;
collision.started = true;

Event evt = Event::CreateCollision(collision);
World::Get().GetEventQueueSystem()->PostEvent(ECSEventType::CollisionStarted, evt);

// Consumer has type safety
eq->ConsumeEvents(ECSEventType::CollisionStarted,
    [](const Event& evt) {
        const CollisionEvent& col = evt.data.collision;  // Type-safe access
        // IDE autocomplete works here!
    });
```

## Performance

- **Ring Buffer**: O(1) push/pop operations
- **Fixed Allocation**: No malloc during gameplay (64 events pre-allocated)
- **Cache Friendly**: Sequential memory access
- **Separate Queues**: Each event type has its own queue (no interference)

## Common Patterns

### Producer-Consumer Chain
```cpp
// System 1: Detect collision → post CollisionEvent
// System 2: Handle collision → post HealthChangedEvent
// System 3: Handle death → post EntityDestroyedEvent
// System 4: Spawn particle effect for destroyed entity
```

### Event Recording for Replay
```cpp
std::vector<Event> recording;

// Record
eq->PeekEvents(ECSEventType::InputButton,
    [&](const Event& evt) {
        recording.push_back(evt);
    });

// Replay
for (const Event& evt : recording) {
    eq->PostEvent(evt.type, evt);
}
```

### Debug Logging
```cpp
// Log all events without consuming them
eq->PeekEvents(ECSEventType::TriggerEntered,
    [](const Event& evt) {
        const TriggerEvent& t = evt.data.trigger;
        std::cout << "DEBUG: Trigger " << t.trigger 
                  << " entered by " << t.other << std::endl;
    });
```

## Next Steps

1. Read full guide: `Documentation/EventQueue_ECS_Guide.md`
2. See working examples: `Source/ECS_Systems.cpp` (TriggerSystem, DetectionSystem)
3. Run tests: `Source/EventQueue_Example.cpp`
4. Check event definitions: `Source/ECS_Events.h`

## Need Help?

- **Event Types**: See `ECSEventType` enum in `Source/ECS_Events.h`
- **Event Structs**: See individual event definitions in `Source/ECS_Events.h`
- **API Reference**: See `EventQueueSystem` class in `Source/ECS_Systems.h`
- **Examples**: See `Documentation/EventQueue_ECS_Guide.md` (6 detailed examples)

---

**Quick Reference Card**

```cpp
// Get queue
auto* eq = World::Get().GetEventQueueSystem();

// Post event
Event evt = Event::CreateCollision(collisionEvt);
eq->PostEvent(ECSEventType::CollisionStarted, evt);

// Consume events
eq->ConsumeEvents(ECSEventType::CollisionStarted,
    [](const Event& evt) { /* handle */ });

// Check queue
size_t count = eq->GetEventCount(ECSEventType::CollisionStarted);

// Clear
eq->ClearEvents(ECSEventType::CollisionStarted);
```

---

*Olympe Engine V2 - 2025*
