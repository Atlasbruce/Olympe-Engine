---
id: ecs-systems
title: ECS Systems Reference
sidebar_label: ECS Systems
sidebar_position: 3
---

# ECS Systems Reference

Systems in Olympe Engine process entities with specific component combinations. Each system runs every frame and updates component data based on game logic.

## System Architecture

Systems follow a consistent pattern:

```cpp
class ExampleSystem {
public:
    static void Process(float deltaTime);
};
```

Systems:
- Are **stateless** (use static methods)
- Process entities with **specific component combinations**
- Run in a **defined order** each frame
- Have access to **World** for entity queries

## Core Systems

### Movement System

**File:** `Source/ECS_Systems.cpp`

Processes entities with Position and Movement components.

```cpp
// Processes: Position_data + Movement_data
void MovementSystem::Process(float deltaTime) {
    auto entities = World::Get().GetEntitiesWithComponents<
        Position_data, Movement_data
    >();
    
    for (EntityID entity : entities) {
        auto* pos = World::Get().GetComponent<Position_data>(entity);
        auto* mov = World::Get().GetComponent<Movement_data>(entity);
        
        // Apply velocity
        pos->position.x += mov->velocity.x * deltaTime;
        pos->position.y += mov->velocity.y * deltaTime;
        
        // Apply friction
        float frictionFactor = 1.0f - (mov->friction * deltaTime);
        mov->velocity.x *= frictionFactor;
        mov->velocity.y *= frictionFactor;
        
        // Clamp to max speed
        float speed = Vector::Length(mov->velocity);
        if (speed > mov->maxSpeed) {
            mov->velocity = Vector::Normalize(mov->velocity) * mov->maxSpeed;
        }
    }
}
```

**Purpose:** Updates entity positions based on velocity and handles friction.

### Physics System

Applies physics simulation to entities.

```cpp
// Processes: Position_data + PhysicsBody_data
void PhysicsSystem::Process(float deltaTime) {
    auto entities = World::Get().GetEntitiesWithComponents<
        Position_data, PhysicsBody_data
    >();
    
    for (EntityID entity : entities) {
        auto* pos = World::Get().GetComponent<Position_data>(entity);
        auto* physics = World::Get().GetComponent<PhysicsBody_data>(entity);
        
        // Apply gravity
        if (physics->useGravity) {
            physics->acceleration.y += 980.0f * deltaTime;  // 9.8 m/s²
        }
        
        // Apply acceleration to velocity
        physics->velocity.x += physics->acceleration.x * deltaTime;
        physics->velocity.y += physics->acceleration.y * deltaTime;
        
        // Apply drag
        physics->velocity.x *= (1.0f - physics->drag);
        physics->velocity.y *= (1.0f - physics->drag);
        
        // Update position
        pos->position.x += physics->velocity.x * deltaTime;
        pos->position.y += physics->velocity.y * deltaTime;
        
        // Reset acceleration
        physics->acceleration = Vector(0, 0, 0);
    }
}
```

**Purpose:** Simulates physics forces like gravity and drag.

## Rendering Systems

### Sprite Rendering System

Renders all visible sprites in the world.

```cpp
// Processes: Position_data + VisualSprite_data
void SpriteRenderSystem::Process(SDL_Renderer* renderer) {
    auto entities = World::Get().GetEntitiesWithComponents<
        Position_data, VisualSprite_data
    >();
    
    // Sort by render layer (Z-order)
    std::vector<EntityID> sortedEntities(entities.begin(), entities.end());
    std::sort(sortedEntities.begin(), sortedEntities.end(),
        [](EntityID a, EntityID b) {
            auto* spriteA = World::Get().GetComponent<VisualSprite_data>(a);
            auto* spriteB = World::Get().GetComponent<VisualSprite_data>(b);
            return spriteA->renderLayer < spriteB->renderLayer;
        });
    
    // Render in layer order
    for (EntityID entity : sortedEntities) {
        auto* pos = World::Get().GetComponent<Position_data>(entity);
        auto* sprite = World::Get().GetComponent<VisualSprite_data>(entity);
        
        // Create source and destination rectangles
        SDL_Rect src = {
            sprite->sourceX, sprite->sourceY,
            sprite->width, sprite->height
        };
        
        SDL_FRect dest = {
            pos->position.x,
            pos->position.y,
            sprite->width * sprite->scale,
            sprite->height * sprite->scale
        };
        
        // Apply flip
        SDL_FlipMode flip = SDL_FLIP_NONE;
        if (sprite->flipHorizontal) flip |= SDL_FLIP_HORIZONTAL;
        if (sprite->flipVertical) flip |= SDL_FLIP_VERTICAL;
        
        // Render
        SDL_Texture* texture = TextureManager::Get(sprite->texturePath);
        SDL_RenderTextureEx(renderer, texture, &src, &dest, 0, nullptr, flip);
    }
}
```

**Purpose:** Renders all sprite components with proper Z-ordering.

### Animation System

Updates sprite animations.

```cpp
// Processes: VisualSprite_data + Animation_data
void AnimationSystem::Process(float deltaTime) {
    auto entities = World::Get().GetEntitiesWithComponents<
        VisualSprite_data, Animation_data
    >();
    
    for (EntityID entity : entities) {
        auto* sprite = World::Get().GetComponent<VisualSprite_data>(entity);
        auto* anim = World::Get().GetComponent<Animation_data>(entity);
        
        if (!anim->playing) continue;
        
        // Update timer
        anim->frameTimer += deltaTime;
        
        // Advance frame
        if (anim->frameTimer >= anim->frameDuration) {
            anim->frameTimer -= anim->frameDuration;
            anim->currentFrame++;
            
            // Handle loop/completion
            if (anim->currentFrame >= anim->totalFrames) {
                if (anim->loop) {
                    anim->currentFrame = 0;
                } else {
                    anim->currentFrame = anim->totalFrames - 1;
                    anim->playing = false;
                }
            }
            
            // Update sprite source rect
            sprite->sourceX = (anim->currentFrame % 8) * sprite->width;
            sprite->sourceY = (anim->currentFrame / 8) * sprite->height;
        }
    }
}
```

**Purpose:** Advances sprite animations frame by frame.

## Input Systems

### Input Event System

Processes keyboard and mouse input events.

```cpp
// Processes: PlayerController_data + Controller_data
void InputEventSystem::Process(const SDL_Event& event) {
    auto entities = World::Get().GetEntitiesWithComponents<
        PlayerController_data, Controller_data
    >();
    
    for (EntityID entity : entities) {
        auto* controller = World::Get().GetComponent<Controller_data>(entity);
        
        if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                case SDLK_W: case SDLK_UP:
                    controller->inputDirection.y = -1.0f;
                    break;
                case SDLK_S: case SDLK_DOWN:
                    controller->inputDirection.y = 1.0f;
                    break;
                case SDLK_A: case SDLK_LEFT:
                    controller->inputDirection.x = -1.0f;
                    break;
                case SDLK_D: case SDLK_RIGHT:
                    controller->inputDirection.x = 1.0f;
                    break;
                case SDLK_SPACE:
                    controller->actionPressed = true;
                    break;
            }
        }
        else if (event.type == SDL_EVENT_KEY_UP) {
            switch (event.key.key) {
                case SDLK_W: case SDLK_UP:
                case SDLK_S: case SDLK_DOWN:
                    controller->inputDirection.y = 0.0f;
                    break;
                case SDLK_A: case SDLK_LEFT:
                case SDLK_D: case SDLK_RIGHT:
                    controller->inputDirection.x = 0.0f;
                    break;
                case SDLK_SPACE:
                    controller->actionPressed = false;
                    break;
            }
        }
    }
}
```

**Purpose:** Translates SDL input events to controller component state.

### Player Movement System

Converts input to movement.

```cpp
// Processes: PlayerController_data + Controller_data + Movement_data
void PlayerMovementSystem::Process(float deltaTime) {
    auto entities = World::Get().GetEntitiesWithComponents<
        PlayerController_data, Controller_data, Movement_data
    >();
    
    for (EntityID entity : entities) {
        auto* playerCtrl = World::Get().GetComponent<PlayerController_data>(entity);
        auto* controller = World::Get().GetComponent<Controller_data>(entity);
        auto* movement = World::Get().GetComponent<Movement_data>(entity);
        
        if (!playerCtrl->canMove) continue;
        
        // Normalize input direction
        Vector input = controller->inputDirection;
        float inputMagnitude = Vector::Length(input);
        if (inputMagnitude > 1.0f) {
            input = Vector::Normalize(input);
        }
        
        // Apply acceleration in input direction
        float targetSpeed = inputMagnitude * playerCtrl->moveSpeed;
        Vector targetVelocity = input * targetSpeed;
        
        // Lerp current velocity towards target
        float accelRate = movement->acceleration * deltaTime;
        movement->velocity.x += (targetVelocity.x - movement->velocity.x) * accelRate;
        movement->velocity.y += (targetVelocity.y - movement->velocity.y) * accelRate;
    }
}
```

**Purpose:** Converts player input to movement acceleration.

## Collision Systems

### Collision Detection System

Detects collisions between entities.

```cpp
// Processes: Position_data + BoundingBox_data
void CollisionSystem::Process() {
    auto entities = World::Get().GetEntitiesWithComponents<
        Position_data, BoundingBox_data
    >();
    
    std::vector<EntityID> entityList(entities.begin(), entities.end());
    
    // Check all pairs
    for (size_t i = 0; i < entityList.size(); i++) {
        for (size_t j = i + 1; j < entityList.size(); j++) {
            EntityID entityA = entityList[i];
            EntityID entityB = entityList[j];
            
            auto* posA = World::Get().GetComponent<Position_data>(entityA);
            auto* bboxA = World::Get().GetComponent<BoundingBox_data>(entityA);
            auto* posB = World::Get().GetComponent<Position_data>(entityB);
            auto* bboxB = World::Get().GetComponent<BoundingBox_data>(entityB);
            
            // Create world-space rectangles
            SDL_FRect rectA = {
                posA->position.x + bboxA->boundingBox.x,
                posA->position.y + bboxA->boundingBox.y,
                bboxA->boundingBox.w,
                bboxA->boundingBox.h
            };
            
            SDL_FRect rectB = {
                posB->position.x + bboxB->boundingBox.x,
                posB->position.y + bboxB->boundingBox.y,
                bboxB->boundingBox.w,
                bboxB->boundingBox.h
            };
            
            // Test intersection
            if (SDL_RectsIntersect(&rectA, &rectB)) {
                // Handle collision
                OnCollision(entityA, entityB);
            }
        }
    }
}
```

**Purpose:** Broad-phase collision detection using AABB tests.

## AI Systems

### Behavior Tree System

Executes behavior trees for AI entities.

```cpp
// Processes: Position_data + AIBlackboard_data + BehaviorTree_data
void BehaviorTreeSystem::Process(float deltaTime) {
    auto entities = World::Get().GetEntitiesWithComponents<
        Position_data, AIBlackboard_data
    >();
    
    for (EntityID entity : entities) {
        // Update behavior tree
        BehaviorTree::UpdateEntity(entity, deltaTime);
    }
}
```

**Purpose:** Runs AI behavior logic for NPCs and enemies.

## System Execution Order

Systems execute in a specific order each frame:

```cpp
void GameEngine::Update(float deltaTime) {
    // 1. Input
    InputEventSystem::Process(events);
    
    // 2. AI
    BehaviorTreeSystem::Process(deltaTime);
    
    // 3. Player Control
    PlayerMovementSystem::Process(deltaTime);
    
    // 4. Physics
    PhysicsSystem::Process(deltaTime);
    MovementSystem::Process(deltaTime);
    
    // 5. Collision
    CollisionSystem::Process();
    
    // 6. Animation
    AnimationSystem::Process(deltaTime);
    
    // 7. Health/Combat
    HealthSystem::Process(deltaTime);
    
    // 8. Cleanup
    LifetimeSystem::Process(deltaTime);
}

void GameEngine::Render(SDL_Renderer* renderer) {
    // Render systems
    SpriteRenderSystem::Process(renderer);
    DebugRenderSystem::Process(renderer);
}
```

## Creating Custom Systems

### System Template

```cpp
// MyCustomSystem.h
#pragma once
#include "World.h"

class MyCustomSystem {
public:
    static void Process(float deltaTime);
};

// MyCustomSystem.cpp
#include "MyCustomSystem.h"

void MyCustomSystem::Process(float deltaTime) {
    // Query entities with required components
    auto entities = World::Get().GetEntitiesWithComponents<
        ComponentA, ComponentB
    >();
    
    // Process each entity
    for (EntityID entity : entities) {
        auto* compA = World::Get().GetComponent<ComponentA>(entity);
        auto* compB = World::Get().GetComponent<ComponentB>(entity);
        
        // Update component data
        compA->value += compB->delta * deltaTime;
    }
}
```

### Registering System

Add to game loop in `GameEngine.cpp`:

```cpp
void GameEngine::Update(float deltaTime) {
    // ... other systems
    
    MyCustomSystem::Process(deltaTime);
    
    // ... more systems
}
```

## System Best Practices

### Performance
- **Query once** - Store entity query results, don't re-query
- **Early exit** - Skip inactive entities early
- **Batch operations** - Process similar operations together
- **Avoid allocations** - Reuse containers

### Design
- **Single responsibility** - One system, one purpose
- **Component combinations** - Query specific component sets
- **Stateless** - Use static methods, store no state
- **Order matters** - Consider system dependencies

### Example: Optimized System

```cpp
void OptimizedSystem::Process(float deltaTime) {
    // Cache entity query (only query once)
    static std::vector<EntityID> entityCache;
    entityCache.clear();
    
    auto entities = World::Get().GetEntitiesWithComponents<
        Position_data, Movement_data
    >();
    
    // Copy to vector for indexed access
    entityCache.assign(entities.begin(), entities.end());
    
    // Process in batches
    const size_t batchSize = 64;
    for (size_t start = 0; start < entityCache.size(); start += batchSize) {
        size_t end = std::min(start + batchSize, entityCache.size());
        
        for (size_t i = start; i < end; i++) {
            EntityID entity = entityCache[i];
            // Process entity...
        }
    }
}
```

## Debugging Systems

### Enable System Logging

```cpp
#define DEBUG_SYSTEMS 1

void MovementSystem::Process(float deltaTime) {
    #ifdef DEBUG_SYSTEMS
    std::cout << "[MovementSystem] Processing " 
              << entities.size() << " entities\n";
    #endif
    
    // ... system logic
}
```

### Profile System Performance

```cpp
#include <chrono>

void PhysicsSystem::Process(float deltaTime) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // ... system logic
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start
    );
    
    std::cout << "[PhysicsSystem] Time: " << duration.count() << "μs\n";
}
```

## See Also

- [ECS Components](./ecs-components.md) - Component reference
- [ECS Overview](./ecs-overview.md) - Architecture overview
- [Component Lifecycle](./component-lifecycle.md) - Entity management
