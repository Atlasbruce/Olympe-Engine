# ⚡ ECS Components - Quick Reference

**One-page reference for all engine components** | [📖 Full Guide](Adding_New_Component_Guide.md)

---

## 🎯 Core Components (ECS_Components.h)

| Component | Purpose | Key Fields | Systems Using It |
|-----------|---------|------------|------------------|
| **Identity_data** | Entity metadata | `name`, `archetype`, `isActive` | All systems (filtering) |
| **Position_data** | World position | `position`, `rotation` | Movement, Rendering, Camera |
| **Velocity_data** | Movement speed | `velocity`, `acceleration` | PhysicsSystem, MovementSystem |
| **Sprite_data** | Visual representation | `textureId`, `width`, `height`, `tint` | RenderingSystem |
| **Collider_data** | Collision detection | `type`, `width`, `height`, `layer` | CollisionSystem |
| **Health_data** | HP tracking | `currentHealth`, `maxHealth` | CombatSystem, AI conditions |
| **MoveIntent_data** | Movement request | `targetPosition`, `desiredSpeed`, `usePathfinding` | NavigationSystem, MovementSystem |
| **AttackIntent_data** | Combat request | `targetEntity`, `damage`, `range` | CombatSystem |
| **GridSettings_data** | Visual debug | `showGrid`, `showCollision`, `showNavigation` | GridSystem, Overlays |

---

## 🤖 AI Components (ECS_Components_AI.h)

| Component | Purpose | Key Fields | Systems Using It |
|-----------|---------|------------|------------------|
| **AIBlackboard_data** | AI memory | `targetEntity`, `moveGoal`, `wanderDestination` | All AI systems |
| **AISenses_data** | Perception config | `sightRange`, `hearingRange`, `thinkHz` | AIPerceptionSystem |
| **AIState_data** | Behavior mode | `currentMode` (Idle/Patrol/Combat/Flee) | AIStateTransitionSystem |
| **BehaviorTreeRuntime_data** | BT execution | `treeAssetId`, `currentNodeIndex`, `isActive` | BehaviorTreeSystem |
| **NavigationAgent_data** | Pathfinding | `currentPath`, `pathRequested`, `targetPosition` | NavigationSystem |

---

## 🎨 Rendering Components (ECS_Components_Rendering.h)

| Component | Purpose | Key Fields |
|-----------|---------|------------|
| **AnimationState_data** | Animation playback | `currentAnimation`, `frameIndex`, `elapsed` |
| **RenderLayer_data** | Draw order | `layer`, `sortOrder` |
| **Camera_data** | Viewport control | `zoom`, `targetEntity`, `offset` |

---

## 📝 Adding a New Component - Checklist

```cpp
// 1. Define struct in appropriate header
struct MyComponent_data
{
    float myValue = 0.0f;
    bool isActive = true;
};

// 2. Auto-register (REQUIRED!)
AUTO_REGISTER_COMPONENT(MyComponent_data);

// 3. Add to PrefabFactory (if needed in prefabs)
void PrefabFactory::InstantiateMyComponent(EntityID entity, const json& data)
{
    MyComponent_data comp;
    comp.myValue = JsonHelper::GetFloat(data, "myValue", 0.0f);
    World::Get().AddComponent(entity, comp);
}

// 4. Register factory in PrefabFactory::RegisterAllComponentFactories()
RegisterComponentFactory("MyComponent", [](EntityID e, const json& d) {
    InstantiateMyComponent(e, d);
});
```

---

## ⚠️ Common Mistakes

### ❌ Forgetting AUTO_REGISTER_COMPONENT
```cpp
struct MyComponent_data { ... };
// Missing: AUTO_REGISTER_COMPONENT(MyComponent_data);
```
**Result:** Component not recognized by ECS → crashes

### ❌ Adding Logic to Components
```cpp
struct Position_data
{
    Vector position;
    void MoveBy(Vector delta) { position += delta; } // ❌ NO!
};
```
**Solution:** Put logic in Systems, keep components as pure data

### ❌ Using std::string in Hot Path
```cpp
struct AIBlackboard_data
{
    std::string currentState; // ❌ Slow for frequent access
};
```
**Solution:** Use enums or fixed-size char arrays

---

## 🔍 Component Relationships

### Intent Pattern
```
[AI System] → [MoveIntent_data] → [MovementSystem]
             ↓
         (sets intent)
```

**Example:**
```cpp
// AI decides to move
MoveIntent_data& intent = GetComponent<MoveIntent_data>(entity);
intent.targetPosition = goal;
intent.hasIntent = true;

// MovementSystem reads and executes
if (intent.hasIntent)
{
    MoveTowards(entity, intent.targetPosition);
}
```

### Blackboard Pattern
```
[AIPerceptionSystem] → [AIBlackboard_data] ← [BehaviorTreeSystem]
                       ↓
                 (shared state)
```

**Example:**
```cpp
// Perception updates blackboard
blackboard.targetEntity = detectedEntity;
blackboard.distanceToTarget = distance;

// BT reads from blackboard
if (blackboard.distanceToTarget < attackRange)
    Attack(blackboard.targetEntity);
```

---

## 🎯 Component Design Guidelines

### ✅ DO
- Keep components small (< 200 bytes ideal)
- Use primitive types when possible
- Initialize all fields with defaults
- Document units (pixels, seconds, etc.)
- Group related fields together

### ❌ DON'T
- Store pointers to other components
- Add virtual functions (breaks POD)
- Use dynamic containers (std::vector, std::map) unless necessary
- Reference external systems
- Put game logic in components

---

## 🧪 Testing Components

```cpp
// Create test entity
EntityID testEntity = World::Get().CreateEntity();

// Add component
Position_data pos;
pos.position = Vector(100, 200);
World::Get().AddComponent(testEntity, pos);

// Verify
ASSERT(World::Get().HasComponent<Position_data>(testEntity));
Position_data& retrieved = World::Get().GetComponent<Position_data>(testEntity);
ASSERT(retrieved.position.x == 100);
```

---

## 📚 See Also

- [Full Component Guide](Adding_New_Component_Guide.md) - Step-by-step instructions
- [System Quick Ref](../02_ECS_Systems/_QuickRef_Systems.md) - How systems use components
- [Prefab Quick Ref](../03_Prefabs/_QuickRef_Prefabs.md) - Using components in prefabs

---

**💡 Pro Tip:** When unsure about a component's structure, look at similar existing components in the codebase!
