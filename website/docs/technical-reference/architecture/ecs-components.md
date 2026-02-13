---
id: ecs-components
title: ECS Components Reference
sidebar_label: ECS Components
sidebar_position: 2
---

# ECS Components Reference

Olympe Engine uses an Entity-Component-System (ECS) architecture. This reference documents all built-in components available in the engine.

## Core Components

### Identity Component

**File:** `Source/ECS_Components.h`

Stores basic entity identification information.

```cpp
struct Identity_data {
    std::string name = "Entity";        // Entity name identifier
    std::string tag = "Untagged";       // Entity tag/category
    std::string type = "UnknownType";   // Entity type string
    EntityType entityType = EntityType::None;  // Entity type enum
    bool isPersistent = false;          // Persist across levels?
};
```

**Usage:**
```cpp
// Create entity with identity
EntityID entity = World::Get().CreateEntity();

Identity_data identity;
identity.name = "Player";
identity.tag = "Player";
identity.entityType = EntityType::Player;

World::Get().AddComponent<Identity_data>(entity, identity);
```

### Position Component

Stores entity position in world space.

```cpp
struct Position_data {
    Vector position;  // 2D/3D position vector (x, y, z)
};
```

**Usage:**
```cpp
Position_data pos;
pos.position = Vector(100.0f, 200.0f, 0.0f);
World::Get().AddComponent<Position_data>(entity, pos);

// Update position
auto* position = World::Get().GetComponent<Position_data>(entity);
if (position) {
    position->position.x += deltaTime * speed;
}
```

### BoundingBox Component

Defines rectangular collision area for the entity.

```cpp
struct BoundingBox_data {
    SDL_FRect boundingBox = {0.f, 0.f, 25.f, 25.f};  // x, y, width, height
};
```

**Usage:**
```cpp
BoundingBox_data bbox;
bbox.boundingBox = {0.f, 0.f, 32.f, 48.f};  // 32x48 collision box
World::Get().AddComponent<BoundingBox_data>(entity, bbox);
```

## Movement Components

### Movement Component

Controls entity movement with velocity, acceleration, and friction.

```cpp
struct Movement_data {
    Vector velocity;                    // Current velocity
    float maxSpeed = 100.0f;           // Maximum speed
    float acceleration = 400.0f;        // Acceleration rate
    float friction = 600.0f;            // Deceleration rate
};
```

**Usage:**
```cpp
Movement_data movement;
movement.maxSpeed = 150.0f;
movement.acceleration = 500.0f;
World::Get().AddComponent<Movement_data>(entity, movement);

// Apply movement in a system
auto* mov = World::Get().GetComponent<Movement_data>(entity);
auto* pos = World::Get().GetComponent<Position_data>(entity);
if (mov && pos) {
    // Apply velocity
    pos->position.x += mov->velocity.x * deltaTime;
    pos->position.y += mov->velocity.y * deltaTime;
    
    // Apply friction
    float friction = mov->friction * deltaTime;
    mov->velocity.x *= (1.0f - friction);
    mov->velocity.y *= (1.0f - friction);
}
```

### PhysicsBody Component

Adds physics simulation to entities.

```cpp
struct PhysicsBody_data {
    Vector velocity;                    // Current velocity
    Vector acceleration;                // Current acceleration
    float mass = 1.0f;                 // Mass (for physics)
    float drag = 0.1f;                 // Air resistance
    bool useGravity = false;           // Apply gravity?
};
```

## Visual Components

### VisualSprite Component

Renders a sprite for the entity.

```cpp
struct VisualSprite_data {
    std::string texturePath;            // Path to texture file
    int sourceX = 0, sourceY = 0;      // Source rectangle position
    int width = 32, height = 32;       // Sprite dimensions
    float scale = 1.0f;                // Uniform scale
    bool flipHorizontal = false;        // Flip on X-axis
    bool flipVertical = false;          // Flip on Y-axis
    SDL_Color tint = {255, 255, 255, 255};  // Color tint
    float opacity = 1.0f;              // Alpha transparency
    int renderLayer = 0;               // Z-order layer
};
```

**Usage:**
```cpp
VisualSprite_data sprite;
sprite.texturePath = "Resources/Textures/Characters/Knight.png";
sprite.width = 48;
sprite.height = 64;
sprite.scale = 1.5f;
sprite.renderLayer = static_cast<int>(RenderLayer::Characters);

World::Get().AddComponent<VisualSprite_data>(entity, sprite);
```

### Animation Component

Controls sprite animation.

```cpp
struct Animation_data {
    int currentFrame = 0;               // Current animation frame
    int totalFrames = 1;                // Total frames in animation
    float frameDuration = 0.1f;         // Duration per frame (seconds)
    float frameTimer = 0.0f;            // Internal timer
    bool loop = true;                   // Loop animation?
    bool playing = true;                // Is animation playing?
};
```

**Usage:**
```cpp
Animation_data anim;
anim.totalFrames = 8;          // 8-frame walk cycle
anim.frameDuration = 0.1f;     // 10 FPS
anim.loop = true;

World::Get().AddComponent<Animation_data>(entity, anim);
```

## Combat Components

### Health Component

Manages entity health and damage.

```cpp
struct Health_data {
    float maxHealth = 100.0f;          // Maximum health points
    float currentHealth = 100.0f;      // Current health
    float regeneration = 0.0f;         // Health regen per second
    bool invulnerable = false;         // Immune to damage?
};
```

**Usage:**
```cpp
Health_data health;
health.maxHealth = 150.0f;
health.currentHealth = 150.0f;
health.regeneration = 0.5f;  // 0.5 HP/sec
World::Get().AddComponent<Health_data>(entity, health);

// Damage entity
auto* hp = World::Get().GetComponent<Health_data>(entity);
if (hp && !hp->invulnerable) {
    hp->currentHealth -= damageAmount;
    if (hp->currentHealth <= 0.0f) {
        // Entity died
        World::Get().DestroyEntity(entity);
    }
}
```

## AI Components

### AIBehavior Component

Basic AI behavior flags.

```cpp
struct AIBehavior_data {
    bool aggressive = false;            // Attacks on sight?
    bool patrol = false;                // Uses patrol behavior?
    float detectionRange = 100.0f;     // Detection radius
};
```

### NPC Component

NPC-specific data.

```cpp
struct NPC_data {
    std::string dialogue;               // NPC dialogue text
    bool canInteract = true;           // Can player interact?
    float interactionRadius = 50.0f;   // Interaction range
};
```

## Gameplay Components

### TriggerZone Component

Creates trigger areas for events.

```cpp
struct TriggerZone_data {
    float radius = 15.0f;              // Detection radius
    bool triggered = false;             // Is something detected?
};
```

### Inventory Component

Manages item inventory.

```cpp
struct Inventory_data {
    std::vector<EntityID> items;        // List of item entities
    int maxItems = 10;                  // Inventory capacity
};
```

## Input Components

### Controller Component

Generic controller input.

```cpp
struct Controller_data {
    Vector inputDirection;              // Input vector (normalized)
    bool actionPressed = false;         // Action button state
    bool jumpPressed = false;           // Jump button state
    bool interactPressed = false;       // Interact button state
};
```

### PlayerController Component

Player-specific input handling.

```cpp
struct PlayerController_data {
    float moveSpeed = 150.0f;          // Movement speed
    bool canMove = true;               // Movement enabled?
    bool canJump = false;              // Jump enabled?
};
```

### InputMapping Component

Maps input keys to actions.

```cpp
struct InputMapping_data {
    std::unordered_map<std::string, SDL_Keycode> keyBindings;
    // Maps action names to keyboard keys
};
```

## Editor Components

### VisualEditor Component

Editor visualization settings.

```cpp
struct VisualEditor_data {
    SDL_Color debugColor = {255, 0, 0, 128};  // Debug render color
    bool showBoundingBox = false;             // Render collision box?
    bool showOrigin = false;                  // Show origin point?
    std::string editorLabel;                  // Label in editor
};
```

### GridSettings Component

Grid visualization settings.

```cpp
struct GridSettings_data {
    bool gridEnabled = false;           // Show grid?
    int gridSpacing = 32;              // Grid cell size
    SDL_Color gridColor = {128, 128, 128, 64};  // Grid color
};
```

## Audio Components

### AudioSource Component

Plays audio from entity.

```cpp
struct AudioSource_data {
    std::string audioPath;              // Path to audio file
    bool loop = false;                  // Loop audio?
    float volume = 1.0f;               // Volume (0.0 to 1.0)
    bool playing = false;               // Is audio playing?
};
```

## Effects Components

### FX Component

Visual effects settings.

```cpp
struct FX_data {
    std::string effectType;             // Effect identifier
    float lifetime = 1.0f;             // Effect duration
    float age = 0.0f;                  // Current age
    bool autoDestroy = true;           // Destroy after lifetime?
};
```

## Component Usage Patterns

### Adding Components

```cpp
// Create entity
EntityID entity = World::Get().CreateEntity();

// Add components
Identity_data id;
id.name = "Enemy";
World::Get().AddComponent<Identity_data>(entity, id);

Position_data pos;
pos.position = Vector(100, 200, 0);
World::Get().AddComponent<Position_data>(entity, pos);
```

### Removing Components

```cpp
World::Get().RemoveComponent<Movement_data>(entity);
```

### Checking for Components

```cpp
if (World::Get().HasComponent<Health_data>(entity)) {
    auto* health = World::Get().GetComponent<Health_data>(entity);
    // Use health component
}
```

### Querying Entities by Component

```cpp
// Get all entities with both Position and Sprite components
auto entities = World::Get().GetEntitiesWithComponents<Position_data, VisualSprite_data>();

for (EntityID entity : entities) {
    auto* pos = World::Get().GetComponent<Position_data>(entity);
    auto* sprite = World::Get().GetComponent<VisualSprite_data>(entity);
    
    // Process entity
    RenderSprite(sprite, pos->position);
}
```

## Component Registration

Components are automatically registered using macros:

```cpp
// In ECS_Components_Registration.cpp
AUTO_REGISTER_COMPONENT(Identity_data);
AUTO_REGISTER_COMPONENT(Position_data);
AUTO_REGISTER_COMPONENT(Movement_data);
// ... etc
```

This enables:
- Dynamic component creation from JSON
- Prefab system support
- Component serialization

## Performance Considerations

### Memory Layout
- Components are stored in contiguous memory arrays
- Fast iteration over entities with specific components
- Cache-friendly design

### Best Practices
- Keep components data-only (no methods)
- Use smaller components for better cache performance
- Query specific components, not all entities
- Remove unused components to save memory

## See Also

- [ECS Systems](./ecs-systems.md) - Component processing systems
- [Component Lifecycle](./component-lifecycle.md) - Component management
- [ECS Overview](./ecs-overview.md) - Architecture overview
- [Prefab Properties](../../user-guide/blueprints/prefab-properties.md) - Component properties in prefabs
