# Component System

Components in Olympe Engine are the **data layer** of the ECS architecture. They hold state but contain no logic.

## Built-in Components

### Transform
```cpp
struct TransformComponent {
    float x, y;        // World position
    float rotation;    // Radians
    float scaleX, scaleY;
};
```

### Sprite
```cpp
struct SpriteComponent {
    std::string texturePath;
    int layer;
    bool visible;
};
```

### Health
```cpp
struct HealthComponent {
    int current;
    int max;
    bool isDead() const { return current <= 0; }
};
```

### AIBlackboard
```cpp
struct AIBlackboardComponent {
    std::string blackboardFile;  // Path to JSON
    BlackboardSystem data;       // Runtime data
};
```

### BehaviorTree
```cpp
struct BehaviorTreeComponent {
    std::string graphFile;   // Path to .bt JSON
    bool isRunning;
};
```

## Component Registration

All components must be registered at startup:

```cpp
// ECS_Components_Registration.cpp
REGISTER_COMPONENT(TransformComponent)
REGISTER_COMPONENT(SpriteComponent)
REGISTER_COMPONENT(HealthComponent)
// ...
```

## Adding a Custom Component

1. Define the struct in a header
2. Register it in `ECS_Components_Registration.cpp`
3. Add a corresponding entry in `ComponentsParameters.json` if it should appear in the Prefab Editor

## Related

- [ECS System](ECS-System.md)
- [Entity Prefab System](../04-Entity-Prefabs/Prefab-System.md)
