# 🧩 Entity Component System (ECS)

Olympe Engine's ECS architecture provides a data-oriented, high-performance foundation for game logic.

---

## 📖 What is ECS?

**Entity Component System (ECS)** is an architectural pattern that emphasizes:

- **Composition over inheritance** - Entities are composed of components
- **Data-oriented design** - Data and logic are separated
- **Cache-friendly** - Components stored contiguously in memory
- **Scalable** - Easy to add new features without breaking existing code

---

## 🎯 Core Concepts

### Entities

Entities are simple integer IDs that serve as handles to collections of components:

```cpp
EntityID player = World::Get().CreateEntity();
EntityID enemy = World::Get().CreateEntity();
```

- Just an ID (typically `uint32_t`)
- No data or logic
- Acts as a "container" for components

### Components

Components are pure data structures with no logic:

```cpp
struct Position_data {
    Vector position;  // x, y, z coordinates
};

struct Health_data {
    int currentHealth;
    int maxHealth;
};

struct Velocity_data {
    Vector velocity;  // Movement vector
};
```

- Plain Old Data (POD) structures
- No methods (except constructors)
- Stored in contiguous arrays for cache efficiency

### Systems

Systems contain logic that operates on entities with specific components:

```cpp
class MovementSystem {
public:
    void Update(float deltaTime) {
        // Get all entities with Position AND Velocity
        auto entities = World::Get().GetEntitiesWithComponents<Position_data, Velocity_data>();
        
        for (EntityID entity : entities) {
            auto* pos = World::Get().GetComponent<Position_data>(entity);
            auto* vel = World::Get().GetComponent<Velocity_data>(entity);
            
            // Update position based on velocity
            pos->position.x += vel->velocity.x * deltaTime;
            pos->position.y += vel->velocity.y * deltaTime;
        }
    }
};
```

- Pure logic, no state (or minimal state)
- Query for entities with specific components
- Process matched entities

---

## 📦 Available Components

### Core Components

#### Position_data
```cpp
struct Position_data {
    Vector position;  // x, y, z in world space
};
```

#### BoundingBox_data
```cpp
struct BoundingBox_data {
    SDL_FRect boundingBox;  // x, y, w, h collision rectangle
};
```

#### VisualSprite_data
```cpp
struct VisualSprite_data {
    std::string spritePath;
    SDL_FRect srcRect;     // Source rectangle in texture
    Vector hotSpot;        // Sprite origin point
};
```

### Physics Components

#### Movement_data
```cpp
struct Movement_data {
    Vector direction;  // Normalized direction vector
    Vector velocity;   // Current velocity
};
```

#### PhysicsBody_data
```cpp
struct PhysicsBody_data {
    float mass;
    float speed;
};
```

### Input Components

#### Controller_data
```cpp
struct Controller_data {
    int controllerIndex;
    bool isConnected;
    // Button states...
};
```

#### PlayerBinding_data
```cpp
struct PlayerBinding_data {
    int playerIndex;
    int controllerID;  // -1 for keyboard
};
```

#### PlayerController_data
```cpp
struct PlayerController_data {
    bool isJumping;
    bool isShooting;
    bool isRunning;
    bool isInteracting;
};
```

### AI Components

#### AIBlackboard_data
```cpp
struct AIBlackboard_data {
    EntityID currentTarget;
    Vector lastKnownTargetPosition;
    bool hasTarget;
    bool heardNoise;
    Vector noiseLocation;
};
```

#### AIState_data
```cpp
struct AIState_data {
    AIMode currentMode;  // Idle, Patrol, Combat, Flee, Investigate, Dead
    float timeSinceStateChange;
};
```

#### BehaviorTreeRuntime_data
```cpp
struct BehaviorTreeRuntime_data {
    int treeID;
    BTNodeState rootState;
    std::unordered_map<int, BTNodeState> nodeStates;
};
```

### Gameplay Components

#### Health_data
```cpp
struct Health_data {
    int currentHealth;
    int maxHealth;
};
```

See `Source/ECS_Components.h` for the complete component reference.

---

## ⚙️ Core Systems

### Input Systems
- **InputSystem** - Processes SDL input events
- **PlayerControllerSystem** - Updates player controller states

### AI Systems
- **AIStimuliSystem** - Processes AI stimuli from events
- **AIPerceptionSystem** - Detects targets and threats
- **AIStateTransitionSystem** - Manages AI state machines
- **BehaviorTreeSystem** - Executes behavior trees
- **AIMotionSystem** - Converts AI intents to movement

### Physics Systems
- **MovementSystem** - Applies velocity to position
- **CollisionSystem** - Detects and resolves collisions
- **TriggerSystem** - Handles trigger zones

### Rendering Systems
- **RenderSystem** - Draws sprites and visuals
- **CameraSystem** - Updates camera position

See `Source/ECS_Systems.h` for system implementations.

---

## 🔄 System Execution Order

Systems execute in a specific order each frame for correct behavior:

```cpp
void World::Update(float deltaTime) {
    // 1. Input - Collect player input
    inputSystem.Update(deltaTime);
    
    // 2. AI - AI decision making
    aiStimuliSystem.Update(deltaTime);
    aiPerceptionSystem.Update(deltaTime);
    aiStateTransitionSystem.Update(deltaTime);
    behaviorTreeSystem.Update(deltaTime);
    aiMotionSystem.Update(deltaTime);
    
    // 3. Physics - Movement and collisions
    movementSystem.Update(deltaTime);
    collisionSystem.Update(deltaTime);
    triggerSystem.Update(deltaTime);
    
    // 4. Animation - Update sprite animations
    animationSystem.Update(deltaTime);
    
    // 5. Camera - Update camera position
    cameraSystem.Update(deltaTime);
    
    // 6. Render - Draw everything
    renderSystem.Update(deltaTime);
    
    // 7. Events - Process deferred events
    eventSystem.Update(deltaTime);
}
```

---

## 🎮 Usage Examples

### Creating an Entity with Components

```cpp
#include "World.h"
#include "ECS_Components.h"

// Create a simple enemy
EntityID enemy = World::Get().CreateEntity();

// Add position
Position_data pos;
pos.position = {100.0f, 200.0f, 0.0f};
World::Get().AddComponent<Position_data>(enemy, pos);

// Add health
Health_data health;
health.currentHealth = 50;
health.maxHealth = 50;
World::Get().AddComponent<Health_data>(enemy, health);

// Add AI
AIState_data aiState;
aiState.currentMode = AIMode::Patrol;
World::Get().AddComponent<AIState_data>(enemy, aiState);
```

### Querying Entities

```cpp
// Get all entities with Position and Health
auto entities = World::Get().GetEntitiesWithComponents<Position_data, Health_data>();

for (EntityID entity : entities) {
    auto* pos = World::Get().GetComponent<Position_data>(entity);
    auto* health = World::Get().GetComponent<Health_data>(entity);
    
    std::cout << "Entity at " << pos->position.x << ", " << pos->position.y 
              << " has " << health->currentHealth << " HP" << std::endl;
}
```

### Modifying Components

```cpp
// Damage an entity
void DamageEntity(EntityID entity, int damage) {
    if (auto* health = World::Get().GetComponent<Health_data>(entity)) {
        health->currentHealth -= damage;
        
        if (health->currentHealth <= 0) {
            health->currentHealth = 0;
            // Entity is dead, handle death logic
        }
    }
}
```

### Removing Components

```cpp
// Remove AI from an entity (make it static)
World::Get().RemoveComponent<AIState_data>(entity);
World::Get().RemoveComponent<BehaviorTreeRuntime_data>(entity);
```

---

## 🏗️ Creating Custom Components

### Step 1: Define the Component

In `Source/ECS_Components.h`:

```cpp
struct MyCustom_data {
    int myValue;
    float myFloat;
    std::string myString;
};
```

### Step 2: Register the Component

Components are automatically registered when you use them with `AddComponent<>`.

### Step 3: Create a System (Optional)

If you need logic, create a system in `Source/ECS_Systems.h`:

```cpp
class MyCustomSystem {
public:
    void Update(float deltaTime) {
        auto entities = World::Get().GetEntitiesWithComponents<MyCustom_data>();
        
        for (EntityID entity : entities) {
            auto* custom = World::Get().GetComponent<MyCustom_data>(entity);
            // Your logic here
        }
    }
};
```

### Step 4: Register the System

Add to `World::Initialize_ECS_Systems()`:

```cpp
void World::Initialize_ECS_Systems() {
    // ... existing systems ...
    myCustomSystem = std::make_unique<MyCustomSystem>();
}
```

---

## 📊 Performance Considerations

### Memory Layout

Components are stored in contiguous arrays for optimal cache performance:

```
Memory Layout (Good Cache Locality)
───────────────────────────────────

Position Components:  [P1][P2][P3][P4][P5]...
Health Components:    [H1][H2][H3][H4][H5]...
Velocity Components:  [V1][V2][V3][V4][V5]...
```

### Best Practices

✅ **Keep components small** - Small components = better cache usage  
✅ **Use POD types** - Avoid complex types in components  
✅ **Minimize component queries** - Cache entity lists when possible  
✅ **Batch operations** - Process many entities at once  
✅ **Avoid component churn** - Don't add/remove components frequently  

❌ **Don't store logic in components** - Keep them data-only  
❌ **Don't use inheritance** - Use composition instead  
❌ **Don't access components across systems** - Use events instead  

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../../README.md)
- 🏗️ [Architecture Overview](../../02-Architecture/README.md)
- 📖 [General Architecture](../../02-Architecture/ARCHITECTURE_GENERALE.md)
- 🎮 [Input System Guide](../Input/INPUT_SYSTEM_GUIDE.md)
- 🤖 [AI Systems](../../04-AI-Systems/README.md)

---

## 📖 Further Reading

### Internal Documentation
- `Source/ECS_Components.h` - All component definitions
- `Source/ECS_Systems.h` - System implementations
- `Source/World.h` - ECS World management

### External Resources
- [Data-Oriented Design Book](https://www.dataorienteddesign.com/dodbook/)
- [ECS FAQ](https://github.com/SanderMertens/ecs-faq)
- [Game Programming Patterns - Component](https://gameprogrammingpatterns.com/component.html)

---

[← Back to Core Systems](../README.md) | [Up to Documentation Hub ↑](../../README.md)
