# ECS System

Olympe Engine uses an **Entity-Component-System (ECS)** architecture as its core data model.

## Concepts

| Concept | Description |
|---------|-------------|
| **Entity** | A unique integer ID (`EntityId`) |
| **Component** | Plain data struct (no logic) |
| **System** | Logic that operates on component groups |

## Entity

```cpp
using EntityId = uint32_t;
const EntityId InvalidEntity = 0;
```

Entities are created and destroyed via the **ECS World**:

```cpp
EntityId entity = world.CreateEntity();
world.DestroyEntity(entity);
```

## Components

Components are pure data:

```cpp
struct TransformComponent {
    float x, y;
    float rotation;
    float scaleX, scaleY;
};

struct HealthComponent {
    int current;
    int max;
};
```

Attach/detach via world:

```cpp
world.AddComponent<TransformComponent>(entity, {100.f, 200.f, 0.f, 1.f, 1.f});
auto& t = world.GetComponent<TransformComponent>(entity);
world.RemoveComponent<TransformComponent>(entity);
```

## Systems

Systems iterate over entities that have specific components:

```cpp
class MovementSystem : public ISystem {
public:
    void Update(float dt) override {
        for (auto entity : world.View<TransformComponent, MovementComponent>()) {
            auto& t = world.GetComponent<TransformComponent>(entity);
            auto& m = world.GetComponent<MovementComponent>(entity);
            t.x += m.velocityX * dt;
            t.y += m.velocityY * dt;
        }
    }
};
```

## Registration

All components are registered in `ECS_Components_Registration.cpp`:

```cpp
world.RegisterComponent<TransformComponent>();
world.RegisterComponent<HealthComponent>();
// ...
```

## Related

- [Component System](Component-System.md)
- [Blackboard Architecture](Blackboard-Architecture.md)
