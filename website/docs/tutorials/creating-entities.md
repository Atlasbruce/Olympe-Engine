---
id: creating-entities
title: Creating Entities from Scratch
sidebar_label: Creating Entities
sidebar_position: 2
---

# Creating Entities from Scratch

Master entity creation in Olympe Engine using both the C++ ECS API and the JSON prefab system. This tutorial covers everything from basic entity instantiation to complex prefab blueprints with multiple components.

## What You'll Learn

- Understanding the Entity Component System (ECS) architecture
- Creating entities programmatically in C++
- Adding and configuring components
- Designing entity prefab blueprints in JSON
- Using the PrefabFactory for entity instantiation
- Best practices for entity design and organization
- Debugging entity creation issues

## Prerequisites

- Basic C++ programming knowledge
- Understanding of component-based architecture
- Olympe Engine development environment set up
- Familiarity with JSON format

---

## Part 1: Understanding the ECS Architecture

Olympe Engine uses an Entity Component System (ECS) architecture where:

- **Entities**: Unique IDs that represent game objects
- **Components**: Data structures that define entity properties
- **Systems**: Logic that processes entities with specific component combinations

### Entity ID

An entity is just a unique identifier:

```cpp
using EntityID = uint64_t;
const EntityID INVALID_ENTITY_ID = 0;
```

Entities have no behavior or data themselves—they're just IDs that link components together.

### Components

Components are plain data structures (POD types):

```cpp
struct Position_data {
    Vector position;  // 3D position (x, y, z)
};

struct Health_data {
    int currentHealth;
    int maxHealth;
};

struct VisualSprite_data {
    std::string spritePath;
    SDL_FRect srcRect;
    Vector hotSpot;
};
```

### Systems

Systems process entities that have specific component combinations:

```cpp
class RenderSystem : public ECS_System {
    void Process() override {
        // Process all entities with Position + VisualSprite components
        for (EntityID entity : m_entities) {
            auto* pos = World::GetInstance().GetComponent<Position_data>(entity);
            auto* sprite = World::GetInstance().GetComponent<VisualSprite_data>(entity);
            // Render sprite at position...
        }
    }
};
```

---

## Part 2: Creating Entities in C++

### Step 1: Basic Entity Creation

Create a simple entity with position:

```cpp
#include "World.h"
#include "ECS_Components.h"

void CreateSimpleEntity()
{
    // Get World singleton
    World& world = World::GetInstance();
    
    // Create new entity (returns unique EntityID)
    EntityID entity = world.CreateEntity();
    
    SYSTEM_LOG << "Created entity: " << entity << std::endl;
}
```

### Step 2: Adding Components

Add components to give the entity properties:

```cpp
void CreateEntityWithComponents()
{
    World& world = World::GetInstance();
    EntityID entity = world.CreateEntity();
    
    // Add Position component
    Position_data position;
    position.position = Vector(100.0f, 200.0f, 0.0f);
    world.AddComponent<Position_data>(entity, position);
    
    // Add BoundingBox component
    BoundingBox_data bbox;
    bbox.boundingBox = SDL_FRect{0.0f, 0.0f, 32.0f, 32.0f};
    world.AddComponent<BoundingBox_data>(entity, bbox);
    
    // Add Identity component
    Identity_data identity;
    identity.name = "MyEntity";
    identity.tag = "Player";
    identity.entityType = EntityType::Player;
    world.AddComponent<Identity_data>(entity, identity);
    
    SYSTEM_LOG << "Created entity with 3 components" << std::endl;
}
```

### Step 3: Complete Player Entity Example

Create a fully-functional player entity:

```cpp
EntityID CreatePlayer(float x, float y)
{
    World& world = World::GetInstance();
    EntityID player = world.CreateEntity();
    
    // Identity
    Identity_data identity("Player", "Player", "Player");
    identity.entityType = EntityType::Player;
    identity.isPersistent = true;  // Don't destroy on level change
    world.AddComponent<Identity_data>(player, identity);
    
    // Position
    Position_data position;
    position.position = Vector(x, y, LayerToZ(RenderLayer::Characters));
    world.AddComponent<Position_data>(player, position);
    
    // Bounding box for collision
    BoundingBox_data bbox;
    bbox.boundingBox = SDL_FRect{-16.0f, -16.0f, 32.0f, 32.0f};
    world.AddComponent<BoundingBox_data>(player, bbox);
    
    // Visual sprite
    VisualSprite_data sprite;
    sprite.spritePath = "Resources/player.png";
    sprite.srcRect = SDL_FRect{0.0f, 0.0f, 32.0f, 32.0f};
    sprite.hotSpot = Vector(16.0f, 16.0f);  // Center of sprite
    world.AddComponent<VisualSprite_data>(player, sprite);
    
    // Movement
    Movement_data movement;
    movement.direction = Vector(0.0f, 0.0f);
    movement.velocity = Vector(0.0f, 0.0f);
    world.AddComponent<Movement_data>(player, movement);
    
    // Physics
    PhysicsBody_data physics;
    physics.mass = 1.0f;
    physics.speed = 200.0f;  // Pixels per second
    world.AddComponent<PhysicsBody_data>(player, physics);
    
    // Health
    Health_data health;
    health.maxHealth = 100;
    health.currentHealth = 100;
    world.AddComponent<Health_data>(player, health);
    
    // Player controller
    PlayerController_data controller;
    controller.isJumping = false;
    controller.isShooting = false;
    world.AddComponent<PlayerController_data>(player, controller);
    
    SYSTEM_LOG << "Created player at (" << x << ", " << y << ")" << std::endl;
    
    return player;
}
```

### Step 4: Creating an Enemy Entity

```cpp
EntityID CreateEnemy(float x, float y, const std::string& behaviorTree)
{
    World& world = World::GetInstance();
    EntityID enemy = world.CreateEntity();
    
    // Identity
    Identity_data identity("Enemy", "Enemy", "Enemy");
    identity.entityType = EntityType::Enemy;
    world.AddComponent<Identity_data>(enemy, identity);
    
    // Position
    Position_data position;
    position.position = Vector(x, y, LayerToZ(RenderLayer::Characters));
    world.AddComponent<Position_data>(enemy, position);
    
    // Bounding box
    BoundingBox_data bbox;
    bbox.boundingBox = SDL_FRect{-16.0f, -24.0f, 32.0f, 48.0f};
    world.AddComponent<BoundingBox_data>(enemy, bbox);
    
    // Visual sprite
    VisualSprite_data sprite;
    sprite.spritePath = "Resources/enemy_skeleton.png";
    sprite.srcRect = SDL_FRect{0.0f, 0.0f, 32.0f, 48.0f};
    sprite.hotSpot = Vector(16.0f, 40.0f);  // Feet position
    world.AddComponent<VisualSprite_data>(enemy, sprite);
    
    // Movement
    Movement_data movement;
    world.AddComponent<Movement_data>(enemy, movement);
    
    // Physics
    PhysicsBody_data physics;
    physics.mass = 1.5f;
    physics.speed = 100.0f;
    world.AddComponent<PhysicsBody_data>(enemy, physics);
    
    // Health
    Health_data health;
    health.maxHealth = 50;
    health.currentHealth = 50;
    world.AddComponent<Health_data>(enemy, health);
    
    // AI Blackboard (memory for AI)
    AIBlackboard_data blackboard;
    blackboard.detectionRadius = 200.0f;
    blackboard.attackRadius = 50.0f;
    blackboard.currentPatrolIndex = 0;
    world.AddComponent<AIBlackboard_data>(enemy, blackboard);
    
    // AI Behavior Tree
    AIBehaviorTree_data bt;
    bt.behaviorTreePath = behaviorTree;
    // Load behavior tree from JSON file
    // (automatic loading happens in AISystem)
    world.AddComponent<AIBehaviorTree_data>(enemy, bt);
    
    SYSTEM_LOG << "Created enemy at (" << x << ", " << y << ")" << std::endl;
    
    return enemy;
}
```

### Step 5: Accessing and Modifying Components

Once created, you can access and modify components:

```cpp
void DamageEntity(EntityID entity, int damage)
{
    World& world = World::GetInstance();
    
    // Get component (returns pointer or nullptr)
    Health_data* health = world.GetComponent<Health_data>(entity);
    
    if (health) {
        health->currentHealth -= damage;
        
        SYSTEM_LOG << "Entity " << entity << " took " << damage 
                   << " damage. Health: " << health->currentHealth 
                   << "/" << health->maxHealth << std::endl;
        
        if (health->currentHealth <= 0) {
            // Entity died
            world.DestroyEntity(entity);
            SYSTEM_LOG << "Entity " << entity << " destroyed!" << std::endl;
        }
    }
}

void MoveEntity(EntityID entity, float dx, float dy)
{
    World& world = World::GetInstance();
    
    Position_data* position = world.GetComponent<Position_data>(entity);
    
    if (position) {
        position->position.x += dx;
        position->position.y += dy;
    }
}

void SetEntityVelocity(EntityID entity, float vx, float vy)
{
    World& world = World::GetInstance();
    
    Movement_data* movement = world.GetComponent<Movement_data>(entity);
    
    if (movement) {
        movement->velocity = Vector(vx, vy);
    }
}
```

### Step 6: Checking for Components

Check if an entity has specific components:

```cpp
bool IsEntityAlive(EntityID entity)
{
    World& world = World::GetInstance();
    
    // Check if entity exists and has Health component
    if (!world.EntityExists(entity)) {
        return false;
    }
    
    Health_data* health = world.GetComponent<Health_data>(entity);
    return health && health->currentHealth > 0;
}

bool CanEntityMove(EntityID entity)
{
    World& world = World::GetInstance();
    
    // Entity needs both Position and Movement components to move
    return world.GetComponent<Position_data>(entity) != nullptr &&
           world.GetComponent<Movement_data>(entity) != nullptr;
}
```

### Step 7: Destroying Entities

Remove entities from the world:

```cpp
void DestroyEntity(EntityID entity)
{
    World& world = World::GetInstance();
    
    // This removes entity and all its components
    world.DestroyEntity(entity);
    
    SYSTEM_LOG << "Entity " << entity << " destroyed" << std::endl;
}

void DestroyAllEnemies()
{
    World& world = World::GetInstance();
    
    // Find all entities with Enemy tag
    std::vector<EntityID> enemies = world.FindEntitiesByTag("Enemy");
    
    for (EntityID enemy : enemies) {
        world.DestroyEntity(enemy);
    }
    
    SYSTEM_LOG << "Destroyed " << enemies.size() << " enemies" << std::endl;
}
```

---

## Part 3: Creating Entity Prefabs (JSON System)

Prefabs are reusable entity templates defined in JSON files. They're easier to edit than C++ and can be modified without recompiling.

### Step 1: Understanding Prefab Structure

Basic prefab JSON structure:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "PrefabName",
    "description": "Description of what this entity does",
    "components": [
        {
            "type": "ComponentType",
            "properties": {
                "propertyName": "value"
            }
        }
    ]
}
```

### Step 2: Creating a Simple Item Prefab

Create `Blueprints/EntityPrefab/HealthPotion.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "HealthPotion",
    "description": "A health potion that restores 50 HP",
    "components": [
        {
            "type": "Identity",
            "properties": {
                "name": "Health Potion",
                "tag": "Item",
                "type": "HealthPotion",
                "entityType": "Item"
            }
        },
        {
            "type": "Position",
            "properties": {
                "position": {
                    "x": 0.0,
                    "y": 0.0,
                    "z": 10000.0
                }
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {
                    "x": -8.0,
                    "y": -8.0,
                    "w": 16.0,
                    "h": 16.0
                }
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/Items/potion_health.png",
                "srcRect": {
                    "x": 0.0,
                    "y": 0.0,
                    "w": 16.0,
                    "h": 16.0
                },
                "hotSpot": {
                    "x": 8.0,
                    "y": 8.0
                }
            }
        },
        {
            "type": "ItemData",
            "properties": {
                "itemType": "Consumable",
                "healAmount": 50,
                "stackSize": 1
            }
        }
    ]
}
```

### Step 3: Creating a Complex NPC Prefab

Create `Blueprints/EntityPrefab/Merchant.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "Merchant",
    "description": "A friendly merchant NPC who sells items",
    "components": [
        {
            "type": "Identity",
            "properties": {
                "name": "Merchant",
                "tag": "NPC",
                "type": "Merchant",
                "entityType": "NPC"
            }
        },
        {
            "type": "Position",
            "properties": {
                "position": {
                    "x": 0.0,
                    "y": 0.0,
                    "z": 20000.0
                }
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {
                    "x": -16.0,
                    "y": -32.0,
                    "w": 32.0,
                    "h": 64.0
                }
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/NPCs/merchant.png",
                "srcRect": {
                    "x": 0.0,
                    "y": 0.0,
                    "w": 32.0,
                    "h": 64.0
                },
                "hotSpot": {
                    "x": 16.0,
                    "y": 56.0
                }
            }
        },
        {
            "type": "Movement",
            "properties": {
                "direction": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 0.0, "y": 0.0}
            }
        },
        {
            "type": "PhysicsBody",
            "properties": {
                "mass": 1.0,
                "speed": 50.0
            }
        },
        {
            "type": "Health",
            "properties": {
                "currentHealth": 100,
                "maxHealth": 100
            }
        },
        {
            "type": "AIBlackboard",
            "properties": {
                "detectionRadius": 100.0,
                "attackRadius": 0.0,
                "currentPatrolIndex": 0,
                "patrolPoints": []
            }
        },
        {
            "type": "AIBehaviorTree",
            "properties": {
                "behaviorTreePath": "Blueprints/AI/merchant_idle.json"
            }
        },
        {
            "type": "Dialogue",
            "properties": {
                "dialogueFile": "Dialogues/merchant_greeting.json",
                "currentNode": 0
            }
        },
        {
            "type": "Shop",
            "properties": {
                "shopInventory": [
                    "HealthPotion",
                    "ManaPotion",
                    "Sword",
                    "Shield"
                ],
                "goldAmount": 1000
            }
        }
    ]
}
```

### Step 4: Creating a Complex Enemy Prefab

Create `Blueprints/EntityPrefab/SkeletonWarrior.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "SkeletonWarrior",
    "description": "An undead warrior that patrols and attacks on sight",
    "components": [
        {
            "type": "Identity",
            "properties": {
                "name": "Skeleton Warrior",
                "tag": "Enemy",
                "type": "SkeletonWarrior",
                "entityType": "Enemy"
            }
        },
        {
            "type": "Position",
            "properties": {
                "position": {
                    "x": 0.0,
                    "y": 0.0,
                    "z": 20000.0
                }
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {
                    "x": -12.0,
                    "y": -28.0,
                    "w": 24.0,
                    "h": 48.0
                }
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/Enemies/skeleton_warrior.png",
                "srcRect": {
                    "x": 0.0,
                    "y": 0.0,
                    "w": 32.0,
                    "h": 48.0
                },
                "hotSpot": {
                    "x": 16.0,
                    "y": 40.0
                }
            }
        },
        {
            "type": "Animation",
            "properties": {
                "animationID": "skeleton_idle",
                "currentFrame": 0,
                "frameDuration": 0.15,
                "elapsedTime": 0.0
            }
        },
        {
            "type": "Movement",
            "properties": {
                "direction": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 0.0, "y": 0.0}
            }
        },
        {
            "type": "PhysicsBody",
            "properties": {
                "mass": 2.0,
                "speed": 80.0
            }
        },
        {
            "type": "Health",
            "properties": {
                "currentHealth": 75,
                "maxHealth": 75
            }
        },
        {
            "type": "Combat",
            "properties": {
                "attackDamage": 15,
                "attackCooldown": 1.5,
                "lastAttackTime": 0.0
            }
        },
        {
            "type": "AIBlackboard",
            "properties": {
                "detectionRadius": 250.0,
                "attackRadius": 40.0,
                "currentPatrolIndex": 0,
                "patrolPoints": [
                    {"x": 100.0, "y": 100.0},
                    {"x": 300.0, "y": 100.0},
                    {"x": 300.0, "y": 300.0},
                    {"x": 100.0, "y": 300.0}
                ]
            }
        },
        {
            "type": "AIBehaviorTree",
            "properties": {
                "behaviorTreePath": "Blueprints/AI/guard_patrol.json"
            }
        },
        {
            "type": "Loot",
            "properties": {
                "lootTable": "skeleton_loot",
                "dropChance": 0.75
            }
        }
    ]
}
```

### Step 5: Using Prefabs in Code

Load and instantiate prefabs:

```cpp
#include "PrefabFactory.h"

void SpawnEntitiesFromPrefabs()
{
    // Preload all prefabs (do this once at startup)
    PrefabFactory::Get().PreloadAllPrefabs("Blueprints/EntityPrefab");
    
    SYSTEM_LOG << "Loaded " << PrefabFactory::Get().GetPrefabCount() 
               << " prefabs" << std::endl;
    
    // Create entity from prefab
    EntityID potion = PrefabFactory::Get().CreateEntityFromPrefabName("HealthPotion");
    
    if (potion != INVALID_ENTITY_ID) {
        SYSTEM_LOG << "Spawned health potion: " << potion << std::endl;
        
        // Override position after creation
        World& world = World::GetInstance();
        Position_data* pos = world.GetComponent<Position_data>(potion);
        if (pos) {
            pos->position = Vector(500.0f, 300.0f, 10000.0f);
        }
    }
    
    // Spawn multiple enemies
    for (int i = 0; i < 5; i++) {
        EntityID enemy = PrefabFactory::Get().CreateEntityFromPrefabName("SkeletonWarrior");
        
        if (enemy != INVALID_ENTITY_ID) {
            // Position them in a line
            Position_data* pos = World::GetInstance().GetComponent<Position_data>(enemy);
            if (pos) {
                pos->position.x = 200.0f + (i * 100.0f);
                pos->position.y = 400.0f;
            }
        }
    }
    
    // Spawn merchant
    EntityID merchant = PrefabFactory::Get().CreateEntityFromPrefabName("Merchant");
    if (merchant != INVALID_ENTITY_ID) {
        SYSTEM_LOG << "Spawned merchant: " << merchant << std::endl;
    }
}
```

### Step 6: Prefab Helper Function

Create a helper for spawning prefabs at specific positions:

```cpp
EntityID SpawnPrefabAt(const std::string& prefabName, float x, float y, float z = -1.0f)
{
    EntityID entity = PrefabFactory::Get().CreateEntityFromPrefabName(prefabName);
    
    if (entity == INVALID_ENTITY_ID) {
        SYSTEM_LOG << "ERROR: Failed to spawn prefab: " << prefabName << std::endl;
        return INVALID_ENTITY_ID;
    }
    
    World& world = World::GetInstance();
    Position_data* pos = world.GetComponent<Position_data>(entity);
    
    if (pos) {
        pos->position.x = x;
        pos->position.y = y;
        
        if (z >= 0.0f) {
            pos->position.z = z;
        }
    }
    
    return entity;
}

// Usage:
void SpawnLevelEntities()
{
    SpawnPrefabAt("Player", 100.0f, 100.0f);
    SpawnPrefabAt("SkeletonWarrior", 500.0f, 200.0f);
    SpawnPrefabAt("SkeletonWarrior", 600.0f, 250.0f);
    SpawnPrefabAt("Merchant", 300.0f, 400.0f);
    SpawnPrefabAt("HealthPotion", 450.0f, 150.0f);
}
```

---

## Part 4: Component Reference

### Core Components

#### Identity_data
Entity identification and metadata.

```cpp
struct Identity_data {
    std::string name;           // Display name
    std::string tag;            // Group/category
    std::string type;           // Type string
    EntityType entityType;      // Type enum
    bool isPersistent;          // Persist across levels?
};
```

**JSON:**
```json
{
    "type": "Identity",
    "properties": {
        "name": "MyEntity",
        "tag": "Enemy",
        "type": "Skeleton",
        "entityType": "Enemy",
        "isPersistent": false
    }
}
```

#### Position_data
2D/3D position in world space.

```cpp
struct Position_data {
    Vector position;  // x, y, z coordinates
};
```

**JSON:**
```json
{
    "type": "Position",
    "properties": {
        "position": {"x": 100.0, "y": 200.0, "z": 0.0}
    }
}
```

#### BoundingBox_data
Collision rectangle.

```cpp
struct BoundingBox_data {
    SDL_FRect boundingBox;  // x, y, w, h
};
```

**JSON:**
```json
{
    "type": "BoundingBox",
    "properties": {
        "boundingBox": {"x": -16.0, "y": -16.0, "w": 32.0, "h": 32.0}
    }
}
```

#### VisualSprite_data
Visual representation.

```cpp
struct VisualSprite_data {
    std::string spritePath;
    SDL_FRect srcRect;
    Vector hotSpot;
};
```

**JSON:**
```json
{
    "type": "VisualSprite",
    "properties": {
        "spritePath": "Resources/sprite.png",
        "srcRect": {"x": 0.0, "y": 0.0, "w": 32.0, "h": 32.0},
        "hotSpot": {"x": 16.0, "y": 16.0}
    }
}
```

### Physics Components

#### Movement_data
Movement direction and velocity.

```cpp
struct Movement_data {
    Vector direction;  // Normalized direction
    Vector velocity;   // Current velocity
};
```

#### PhysicsBody_data
Physical properties.

```cpp
struct PhysicsBody_data {
    float mass;
    float speed;
    float friction;
};
```

### Gameplay Components

#### Health_data
Health tracking.

```cpp
struct Health_data {
    int currentHealth;
    int maxHealth;
};
```

#### AIBlackboard_data
AI state and memory.

```cpp
struct AIBlackboard_data {
    EntityID targetEntity;
    Vector moveGoal;
    std::vector<Vector> patrolPoints;
    int currentPatrolIndex;
    float detectionRadius;
    float attackRadius;
    float waitTimer;
};
```

#### AIBehaviorTree_data
Behavior tree reference.

```cpp
struct AIBehaviorTree_data {
    std::string behaviorTreePath;
    // Runtime data...
};
```

---

## Part 5: Best Practices

### Entity Design Principles

1. **Composition over Inheritance**: Use components, not inheritance hierarchies
2. **Single Responsibility**: Each component should have one clear purpose
3. **Data-Driven**: Prefer JSON prefabs over hardcoded entities
4. **Reusability**: Create generic prefabs that can be customized

### Component Organization

```
Blueprints/
├── EntityPrefab/
│   ├── Player.json
│   ├── Enemies/
│   │   ├── SkeletonWarrior.json
│   │   ├── Goblin.json
│   │   └── Dragon.json
│   ├── NPCs/
│   │   ├── Merchant.json
│   │   ├── Villager.json
│   │   └── Guard.json
│   ├── Items/
│   │   ├── HealthPotion.json
│   │   ├── ManaPotion.json
│   │   └── Weapons/
│   │       ├── Sword.json
│   │       └── Bow.json
│   └── Environment/
│       ├── Tree.json
│       ├── Rock.json
│       └── Chest.json
```

### Naming Conventions

- **Prefab files**: PascalCase (e.g., `SkeletonWarrior.json`)
- **Component types**: PascalCase (e.g., `Position`, `AIBlackboard`)
- **Properties**: camelCase (e.g., `maxHealth`, `spritePath`)
- **Tags**: PascalCase (e.g., `"Player"`, `"Enemy"`)

### Performance Tips

1. **Preload prefabs** at startup, not during gameplay
2. **Pool entities** for frequently spawned objects (projectiles, particles)
3. **Limit component count**: Only add components you need
4. **Use tags** for efficient entity queries

---

## Part 6: Debugging

### Common Issues

#### Prefab Not Found
```
ERROR: Prefab not found: SkeletonWarrior
```

**Solution**:
- Check filename matches prefab name exactly
- Verify file is in `Blueprints/EntityPrefab/` directory
- Ensure `PreloadAllPrefabs()` was called

#### Component Not Applied
```
WARNING: Failed to create component: InvalidComponentType
```

**Solution**:
- Check component type spelling (case-sensitive)
- Verify component is registered in `ECS_Components_Registration.cpp`
- Check JSON syntax is valid

#### Entity Created But Not Visible
**Solution**:
- Verify `Position_data` and `VisualSprite_data` components exist
- Check sprite path is correct
- Verify z-order is appropriate for rendering layer
- Check entity is within camera view

### Debug Logging

Enable entity creation logging:

```cpp
void CreateEntityWithLogging()
{
    World& world = World::GetInstance();
    EntityID entity = world.CreateEntity();
    
    SYSTEM_LOG << "=== Entity " << entity << " ===" << std::endl;
    
    // Add components with logging
    Position_data pos;
    pos.position = Vector(100, 100, 0);
    world.AddComponent<Position_data>(entity, pos);
    SYSTEM_LOG << "  + Position: (" << pos.position.x << ", " 
               << pos.position.y << ")" << std::endl;
    
    Health_data health;
    health.maxHealth = 100;
    health.currentHealth = 100;
    world.AddComponent<Health_data>(entity, health);
    SYSTEM_LOG << "  + Health: " << health.currentHealth 
               << "/" << health.maxHealth << std::endl;
    
    SYSTEM_LOG << "Entity creation complete" << std::endl;
}
```

### Validation Helper

```cpp
bool ValidateEntity(EntityID entity)
{
    World& world = World::GetInstance();
    
    if (!world.EntityExists(entity)) {
        SYSTEM_LOG << "ERROR: Entity " << entity << " does not exist" << std::endl;
        return false;
    }
    
    bool valid = true;
    
    // Check required components
    if (!world.GetComponent<Position_data>(entity)) {
        SYSTEM_LOG << "WARNING: Entity " << entity << " missing Position" << std::endl;
        valid = false;
    }
    
    if (!world.GetComponent<Identity_data>(entity)) {
        SYSTEM_LOG << "WARNING: Entity " << entity << " missing Identity" << std::endl;
        valid = false;
    }
    
    return valid;
}
```

---

## Next Steps

You've learned how to create entities using both C++ and JSON prefabs! Continue with:

### Related Tutorials:
- [**Behavior Tree Basics**](./behavior-tree-basics.md) - Add AI to your entities
- [**First Tiled Map**](./first-tiled-map.md) - Spawn entities from Tiled maps
- [**Animation System**](../user-guide/animation/sprite-animation.md) - Animate your entities

### Advanced Topics:
- [**ECS Architecture**](../technical-reference/architecture/ecs-overview.md) - Deep dive into ECS
- [**Component Registration**](../technical-reference/architecture/ecs-components.md) - Creating custom components
- [**System Development**](../technical-reference/architecture/ecs-systems.md) - Writing custom systems

### Reference Documentation:
- [**Component API Reference**](../api-reference/components.md) - All available components
- [**Prefab System**](../user-guide/blueprints/overview.md) - Advanced prefab features
- [**World API**](../api-reference/world.md) - World class methods

---

## Summary

In this tutorial, you learned:

✅ Understanding ECS architecture (Entities, Components, Systems)  
✅ Creating entities programmatically in C++  
✅ Adding and configuring components  
✅ Creating JSON prefab blueprints  
✅ Using PrefabFactory for entity instantiation  
✅ Best practices for entity design  
✅ Debugging entity creation issues  

You can now create rich, complex game entities for your Olympe Engine projects!

---

## Additional Resources

- **Example Code**: `Examples/AI_Example.cpp`
- **Example Prefabs**: `Blueprints/example_entity_simple.json`
- **Component Source**: `Source/ECS_Components.h`
- **Factory Source**: `Source/prefabfactory.h`

**Happy Entity Creating! 🎮**
