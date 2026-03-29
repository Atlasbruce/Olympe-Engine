---
id: component-lifecycle
title: Component Lifecycle Management
sidebar_label: Component Lifecycle
sidebar_position: 4
---

# Component Lifecycle Management

Understanding how components are created, modified, and destroyed is essential for working with Olympe Engine's ECS architecture.

## Entity Creation

### Basic Entity Creation

```cpp
// Create empty entity
EntityID entity = World::Get().CreateEntity();

// Entity ID is a unique identifier (unsigned int)
std::cout << "Created entity: " << entity << std::endl;
```

### Entity with Components

```cpp
// Create entity and immediately add components
EntityID player = World::Get().CreateEntity();

Identity_data identity;
identity.name = "Player";
identity.entityType = EntityType::Player;
World::Get().AddComponent<Identity_data>(player, identity);

Position_data position;
position.position = Vector(100, 200, 0);
World::Get().AddComponent<Position_data>(player, position);

Movement_data movement;
movement.maxSpeed = 150.0f;
World::Get().AddComponent<Movement_data>(player, movement);
```

### Using Prefabs

The preferred method for creating complex entities:

```cpp
// Load from prefab JSON
EntityID enemy = PrefabFactory::CreateFromFile(
    "Blueprints/Enemies/Orc.json"
);

// Prefab automatically adds all configured components
```

## Component Addition

### Adding Components

```cpp
// Create component instance
Health_data health;
health.maxHealth = 100.0f;
health.currentHealth = 100.0f;

// Add to entity
World::Get().AddComponent<Health_data>(entity, health);
```

### Adding Multiple Components

```cpp
// Add several components at once
Sprite_data sprite;
sprite.texturePath = "Resources/Textures/Character.png";
World::Get().AddComponent<Sprite_data>(entity, sprite);

Animation_data animation;
animation.totalFrames = 8;
World::Get().AddComponent<Animation_data>(entity, animation);

Collision_data collision;
collision.solid = true;
World::Get().AddComponent<Collision_data>(entity, collision);
```

### Component Addition Rules

1. **One instance per type** - Each entity can have only one instance of each component type
2. **Duplicate prevention** - Adding the same component type twice will replace the first
3. **No dependencies** - Components are independent; add in any order

```cpp
// This replaces the first Health component
Health_data health1;
health1.maxHealth = 100.0f;
World::Get().AddComponent<Health_data>(entity, health1);

Health_data health2;
health2.maxHealth = 200.0f;
World::Get().AddComponent<Health_data>(entity, health2);  // Replaces health1
```

## Component Access

### Getting Components

```cpp
// Get pointer to component (returns nullptr if not found)
Health_data* health = World::Get().GetComponent<Health_data>(entity);

if (health) {
    std::cout << "Health: " << health->currentHealth << std::endl;
    
    // Modify component
    health->currentHealth -= 10.0f;
} else {
    std::cout << "Entity has no Health component" << std::endl;
}
```

### Checking for Components

```cpp
// Check if entity has specific component
if (World::Get().HasComponent<Movement_data>(entity)) {
    // Entity has movement
    auto* movement = World::Get().GetComponent<Movement_data>(entity);
    movement->velocity.x += 100.0f;
}
```

### Getting Multiple Components

```cpp
// Get several components for one entity
auto* position = World::Get().GetComponent<Position_data>(entity);
auto* movement = World::Get().GetComponent<Movement_data>(entity);
auto* sprite = World::Get().GetComponent<Sprite_data>(entity);

// Check all exist before using
if (position && movement && sprite) {
    // Process entity with all three components
}
```

## Component Modification

### Direct Modification

```cpp
// Get and modify
auto* health = World::Get().GetComponent<Health_data>(entity);
if (health) {
    health->currentHealth -= damageAmount;
    
    if (health->currentHealth <= 0.0f) {
        health->currentHealth = 0.0f;
        // Handle death
    }
}
```

### Set Component (Replace)

```cpp
// Completely replace component
Health_data newHealth;
newHealth.maxHealth = 200.0f;
newHealth.currentHealth = 200.0f;
newHealth.regeneration = 1.0f;

World::Get().SetComponent<Health_data>(entity, newHealth);
// Old health data is replaced
```

### Conditional Modification

```cpp
// Modify only if component exists
if (auto* movement = World::Get().GetComponent<Movement_data>(entity)) {
    // Boost speed temporarily
    movement->maxSpeed *= 1.5f;
}
```

## Component Removal

### Removing Components

```cpp
// Remove specific component
World::Get().RemoveComponent<Movement_data>(entity);

// Entity still exists, just without Movement component
```

### Removing Multiple Components

```cpp
// Remove several components
World::Get().RemoveComponent<Health_data>(entity);
World::Get().RemoveComponent<Collision_data>(entity);
World::Get().RemoveComponent<AIBehavior_data>(entity);
```

### Conditional Removal

```cpp
// Remove component if it exists
if (World::Get().HasComponent<TriggerZone_data>(entity)) {
    World::Get().RemoveComponent<TriggerZone_data>(entity);
}
```

## Entity Destruction

### Destroying Entities

```cpp
// Mark entity for destruction
World::Get().DestroyEntity(entity);

// Entity and ALL its components will be removed
// Destruction happens at end of frame
```

### Deferred Destruction

Entities are not destroyed immediately to prevent issues during system iteration:

```cpp
void DamageSystem::Process() {
    auto entities = World::Get().GetEntitiesWithComponents<Health_data>();
    
    for (EntityID entity : entities) {
        auto* health = World::Get().GetComponent<Health_data>(entity);
        
        if (health->currentHealth <= 0.0f) {
            // Mark for destruction
            World::Get().DestroyEntity(entity);
            // Entity still exists during this frame
            // Will be removed at frame end
        }
    }
    // Safe to continue iteration
}
```

### Cleanup

```cpp
// At end of frame, World removes all marked entities
void World::ProcessDestroyQueue() {
    for (EntityID entity : m_destroyQueue) {
        // Remove all components
        RemoveAllComponents(entity);
        
        // Free entity ID for reuse
        FreeEntity(entity);
    }
    m_destroyQueue.clear();
}
```

## Component Queries

### Single Component Query

```cpp
// Get all entities with Health component
auto entities = World::Get().GetEntitiesWithComponent<Health_data>();

for (EntityID entity : entities) {
    auto* health = World::Get().GetComponent<Health_data>(entity);
    // Process health
}
```

### Multiple Component Query

```cpp
// Get entities with BOTH Position AND Sprite
auto entities = World::Get().GetEntitiesWithComponents<
    Position_data, 
    Sprite_data
>();

for (EntityID entity : entities) {
    auto* pos = World::Get().GetComponent<Position_data>(entity);
    auto* sprite = World::Get().GetComponent<Sprite_data>(entity);
    
    // Both components guaranteed to exist
    RenderSprite(sprite, pos->position);
}
```

### Complex Queries

```cpp
// Get entities with Position, Movement, and Health
auto movingEntities = World::Get().GetEntitiesWithComponents<
    Position_data, 
    Movement_data, 
    Health_data
>();

// Filter further in code
for (EntityID entity : movingEntities) {
    auto* health = World::Get().GetComponent<Health_data>(entity);
    
    if (health->currentHealth > 0.0f) {
        // Process only living, moving entities
    }
}
```

## Component Lifecycle Hooks

### Creation Hooks

Components can have initialization logic:

```cpp
struct CustomComponent_data {
    int value = 0;
    
    // Called when component is added
    void OnCreate(EntityID owner) {
        value = CalculateInitialValue(owner);
        LogDebug("Created custom component for entity " + owner);
    }
};
```

### Destruction Hooks

Clean up resources when component is removed:

```cpp
struct ResourceComponent_data {
    SDL_Texture* texture = nullptr;
    
    // Called before component is removed
    void OnDestroy() {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
};
```

## Best Practices

### Entity Initialization Pattern

```cpp
EntityID CreateEnemy(const Vector& position) {
    // Create entity
    EntityID enemy = World::Get().CreateEntity();
    
    // Add all components together
    Identity_data id;
    id.name = "Enemy";
    id.entityType = EntityType::Enemy;
    World::Get().AddComponent<Identity_data>(enemy, id);
    
    Position_data pos;
    pos.position = position;
    World::Get().AddComponent<Position_data>(enemy, pos);
    
    Health_data health;
    health.maxHealth = 100.0f;
    health.currentHealth = 100.0f;
    World::Get().AddComponent<Health_data>(enemy, health);
    
    return enemy;
}
```

### Component Validation

Always check component existence:

```cpp
// Bad: May crash if component doesn't exist
auto* health = World::Get().GetComponent<Health_data>(entity);
health->currentHealth -= 10.0f;  // UNSAFE!

// Good: Check before use
auto* health = World::Get().GetComponent<Health_data>(entity);
if (health) {
    health->currentHealth -= 10.0f;  // SAFE
}
```

### Efficient Queries

Cache query results when possible:

```cpp
class MySystem {
    static std::vector<EntityID> s_cachedEntities;
    
public:
    static void Process(float deltaTime) {
        // Update cache only when needed
        static bool needsUpdate = true;
        if (needsUpdate) {
            auto entities = World::Get().GetEntitiesWithComponents<
                Position_data, Movement_data
            >();
            s_cachedEntities.assign(entities.begin(), entities.end());
            needsUpdate = false;
        }
        
        // Use cached entities
        for (EntityID entity : s_cachedEntities) {
            // Process...
        }
    }
};
```

### Safe Iteration

Don't modify entity structure during iteration:

```cpp
// Bad: Modifying during iteration
auto entities = World::Get().GetEntitiesWithComponent<Health_data>();
for (EntityID entity : entities) {
    // DON'T DO THIS: may invalidate iterator
    World::Get().RemoveComponent<Health_data>(entity);
}

// Good: Collect first, modify after
std::vector<EntityID> toRemove;
auto entities = World::Get().GetEntitiesWithComponent<Health_data>();
for (EntityID entity : entities) {
    if (ShouldRemove(entity)) {
        toRemove.push_back(entity);
    }
}

// Now safe to modify
for (EntityID entity : toRemove) {
    World::Get().RemoveComponent<Health_data>(entity);
}
```

## Memory Management

### Component Storage

Components are stored in contiguous arrays for cache efficiency:

```cpp
// Internally, World manages component arrays
template<typename T>
class ComponentArray {
    std::vector<T> m_components;        // Dense array
    std::vector<EntityID> m_entities;   // Corresponding entities
    std::unordered_map<EntityID, size_t> m_entityToIndex;  // Fast lookup
};
```

### Memory Allocation

- Components are allocated when added
- Memory is reused when entities are destroyed
- Arrays grow as needed (std::vector reallocation)

### Performance Considerations

- **Adding components** - O(1) amortized
- **Removing components** - O(1) with swap-and-pop
- **Getting components** - O(1) hash lookup
- **Querying entities** - O(n) where n = entities with component

## Debugging

### Component Inspector

```cpp
void PrintEntityComponents(EntityID entity) {
    std::cout << "Entity " << entity << " components:\n";
    
    if (World::Get().HasComponent<Identity_data>(entity)) {
        auto* id = World::Get().GetComponent<Identity_data>(entity);
        std::cout << "  - Identity: " << id->name << "\n";
    }
    
    if (World::Get().HasComponent<Position_data>(entity)) {
        auto* pos = World::Get().GetComponent<Position_data>(entity);
        std::cout << "  - Position: (" << pos->position.x 
                  << ", " << pos->position.y << ")\n";
    }
    
    if (World::Get().HasComponent<Health_data>(entity)) {
        auto* hp = World::Get().GetComponent<Health_data>(entity);
        std::cout << "  - Health: " << hp->currentHealth 
                  << "/" << hp->maxHealth << "\n";
    }
}
```

### Lifecycle Logging

```cpp
#define DEBUG_COMPONENT_LIFECYCLE 1

#ifdef DEBUG_COMPONENT_LIFECYCLE
#define LOG_COMPONENT(msg) std::cout << "[Component] " << msg << std::endl
#else
#define LOG_COMPONENT(msg)
#endif

// Usage
World::Get().AddComponent<Health_data>(entity, health);
LOG_COMPONENT("Added Health to entity " + std::to_string(entity));
```

## See Also

- [ECS Components](./ecs-components.md) - Component reference
- [ECS Systems](./ecs-systems.md) - System reference
- [ECS Overview](./ecs-overview.md) - Architecture overview
- [Creating Prefabs](../../user-guide/blueprints/creating-prefabs.md) - Entity templates
