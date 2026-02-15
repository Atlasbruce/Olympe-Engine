# ⚡ ECS Systems - Quick Reference

**One-page reference for all engine systems** | [📖 Full Guide](Adding_New_System_Guide.md)

---

## 🎯 System Execution Order (CRITICAL!)

Systems execute in **fixed order** every frame. Order matters for correctness!

```
Frame Start
    ↓
1. INPUT SYSTEMS
   └─ InputEventConsumeSystem     (Keyboard/mouse events)
   └─ GameEventConsumeSystem       (Custom game events)
   └─ InputMappingSystem           (Map keys to actions)
   └─ PlayerControlSystem          (Player input → MoveIntent)
    ↓
2. AI PIPELINE
   └─ AIStimuliSystem             (Detect noise, damage)
   └─ AIPerceptionSystem          (Vision, hearing)
   └─ AIStateTransitionSystem     (Mode changes: Idle→Combat)
   └─ BehaviorTreeSystem          (Execute BT logic)
   └─ AIMotionSystem              (AI → MoveIntent)
    ↓
3. NAVIGATION
   └─ NavigationSystem            (A* pathfinding)
    ↓
4. PHYSICS & MOVEMENT
   └─ PhysicsSystem               (Apply velocity)
   └─ CollisionSystem             (Detect collisions)
   └─ MovementSystem              (Execute MoveIntents)
    ↓
5. CAMERA
   └─ CameraSystem                (Update camera position)
    ↓
6. RENDERING
   └─ RenderingSystem             (Draw sprites)
   └─ GridSystem                  (Debug grid overlay)
   └─ UIRenderingSystem           (UI elements)
    ↓
Frame End
```

**⚠️ ORDER RULES:**
- Input MUST run first (to capture user actions)
- AI MUST run after input (to react to player)
- Navigation MUST run after AI (to calculate paths)
- Physics MUST run after navigation (to move entities)
- Rendering MUST run last (to display final state)

---

## 📋 System Registry

| System | Required Components | Purpose |
|--------|---------------------|---------|
| **InputEventConsumeSystem** | None | Process keyboard/mouse input |
| **PlayerControlSystem** | `PlayerControlled_data`, `MoveIntent_data` | Convert input to movement |
| **AIStimuliSystem** | `AIBlackboard_data` | Detect stimuli (noise, damage) |
| **AIPerceptionSystem** | `AIBlackboard_data`, `AISenses_data` | Vision & hearing checks |
| **AIStateTransitionSystem** | `AIState_data`, `AIBlackboard_data` | Switch AI modes |
| **BehaviorTreeSystem** | `BehaviorTreeRuntime_data`, `AIBlackboard_data` | Execute behavior trees |
| **AIMotionSystem** | `AIBlackboard_data`, `MoveIntent_data` | AI movement decisions |
| **NavigationSystem** | `NavigationAgent_data` | A* pathfinding |
| **PhysicsSystem** | `Position_data`, `Velocity_data` | Apply velocity to position |
| **CollisionSystem** | `Position_data`, `Collider_data` | Detect collisions |
| **MovementSystem** | `Position_data`, `MoveIntent_data` | Execute movement intents |
| **CameraSystem** | `Camera_data` | Update viewport |
| **RenderingSystem** | `Position_data`, `Sprite_data` | Draw entities |
| **GridSystem** | `GridSettings_data` | Debug overlays |

---

## 🆕 Adding a New System - Checklist

### 1. Create System Class
```cpp
// In ECS_Systems_*.cpp
class MyNewSystem : public ECS_System
{
public:
    MyNewSystem();  // Define required signature
    void Process() override;  // Main logic
};
```

### 2. Define Required Signature
```cpp
MyNewSystem::MyNewSystem()
{
    // Entities MUST have these components to be processed
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<MyComponent_data>(), true);
}
```

### 3. Implement Process()
```cpp
void MyNewSystem::Process()
{
    for (EntityID entity : m_entities)  // m_entities auto-populated by ECS
    {
        // Get components (safe - signature ensures they exist)
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        MyComponent_data& my = World::Get().GetComponent<MyComponent_data>(entity);
        
        // Your logic here
        pos.position.x += my.velocity * GameEngine::fDt;
    }
}
```

### 4. Register in World
```cpp
// In World::Initialize_ECS_Systems()
void World::Initialize_ECS_Systems()
{
    // ... existing systems ...
    
    // Add in CORRECT ORDER!
    RegisterSystem(std::make_shared<MyNewSystem>());
}
```

---

## ⚠️ Common Pitfalls

### ❌ Wrong Execution Order
```cpp
// BAD: Rendering before Physics
RegisterSystem(renderingSystem);
RegisterSystem(physicsSystem);  // Renders old positions!
```

**Fix:** Physics → Movement → Rendering

### ❌ Forgetting to Register
```cpp
MyNewSystem mySystem;  // Created but never registered!
```

**Fix:** Call `RegisterSystem()` in `World::Initialize_ECS_Systems()`

### ❌ Missing Component in Signature
```cpp
MyNewSystem::MyNewSystem()
{
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    // Missing MyComponent_data!
}

void MyNewSystem::Process()
{
    MyComponent_data& comp = GetComponent<MyComponent_data>(entity);  // CRASH!
}
```

**Fix:** Add ALL required components to signature

### ❌ Modifying Other Systems' State
```cpp
void MyNewSystem::Process()
{
    PhysicsSystem::someGlobalState = true;  // ❌ Bad coupling
}
```

**Fix:** Use components/intents for inter-system communication

---

## 🔄 System Communication Patterns

### Pattern 1: Intent-Based
```cpp
// System A: Sets intent
MoveIntent_data& intent = GetComponent<MoveIntent_data>(entity);
intent.targetPosition = goal;
intent.hasIntent = true;

// System B: Reads intent (runs later)
if (intent.hasIntent)
    ApplyMovement(entity, intent);
```

### Pattern 2: Blackboard Sharing
```cpp
// Perception System: Writes to blackboard
AIBlackboard_data& bb = GetComponent<AIBlackboard_data>(entity);
bb.targetEntity = detectedEnemy;
bb.distanceToTarget = distance;

// BT System: Reads from blackboard
if (bb.distanceToTarget < attackRange)
    Attack(bb.targetEntity);
```

### Pattern 3: Event Broadcasting
```cpp
// System A: Broadcast event
Event evt;
evt.type = EventType::EntityDied;
evt.entityId = deadEntity;
GameEngine::Get().BroadcastEvent(evt);

// System B: Listen for events (next frame)
for (const Event& evt : consumedEvents)
{
    if (evt.type == EventType::EntityDied)
        HandleDeath(evt.entityId);
}
```

---

## 🧪 Testing Systems

```cpp
// Test system registration
TEST(MyNewSystemTest, IsRegistered)
{
    World& world = World::Get();
    world.Initialize_ECS_Systems();
    
    // Verify system exists
    auto systems = world.GetAllSystems();
    bool found = false;
    for (auto& sys : systems)
    {
        if (dynamic_cast<MyNewSystem*>(sys.get()))
        {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
}

// Test system processes correct entities
TEST(MyNewSystemTest, ProcessesMatchingEntities)
{
    World& world = World::Get();
    world.Initialize_ECS_Systems();
    
    // Create entity with required components
    EntityID entity = world.CreateEntity();
    world.AddComponent(entity, Position_data{});
    world.AddComponent(entity, MyComponent_data{});
    
    // Run systems
    world.Update();
    
    // Verify system executed
    MyComponent_data& comp = world.GetComponent<MyComponent_data>(entity);
    EXPECT_TRUE(comp.wasProcessed);  // Assuming system sets this flag
}
```

---

## 📚 See Also

- [Full System Guide](Adding_New_System_Guide.md) - Detailed implementation steps
- [Component Quick Ref](../01_ECS_Components/_QuickRef_Components.md) - Component reference
- [Architecture Overview](../00_Architecture_Overview.md) - High-level design

---

**💡 Pro Tip:** When debugging, add logging to system `Process()` methods to trace execution order!
