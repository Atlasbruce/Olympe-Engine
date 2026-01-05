# 🤖 AI Systems

Olympe Engine's AI systems provide robust, data-driven artificial intelligence for NPCs and enemies using Behavior Trees and ECS architecture.

---

## 📖 Documentation in This Section

| Document | Description | Level |
|----------|-------------|-------|
| **[AI Architecture](AI_ARCHITECTURE.md)** | Overall AI system design and components | Intermediate |
| **[NPC AI System](NPC_AI_SYSTEM.md)** | Complete NPC AI implementation guide | Intermediate |
| **[Behavior Trees](BEHAVIOR_TREES.md)** | Behavior Tree system and JSON format | Advanced |
| **[BT Visual Architecture](BT_VISUAL_ARCHITECTURE.md)** | Visual architecture diagrams for BT system | Advanced |
| **[BT Developer Guide](BT_DEVELOPER_GUIDE.md)** | Creating custom behavior trees | Advanced |

---

## 🎯 AI System Overview

The Olympe Engine AI system is built on:

- **ECS Integration** - AI components fit seamlessly into the ECS architecture
- **Behavior Trees** - Data-driven, reusable AI behaviors
- **State Machines** - Hierarchical Finite State Machine (HFSM) for high-level states
- **Perception System** - Vision and hearing for NPCs
- **Event-Driven** - Reacts to game events via EventQueue

---

## 🧩 AI Components

### AIBlackboard_data
Stores AI reasoning data (targets, last known positions, etc.)

```cpp
struct AIBlackboard_data {
    EntityID currentTarget;
    Vector lastKnownTargetPosition;
    bool hasTarget;
    bool heardNoise;
    Vector noiseLocation;
};
```

### AISenses_data
Vision and hearing parameters with timeslicing

```cpp
struct AISenses_data {
    float visionRange;
    float visionAngle;
    float hearingRange;
    int timesliceGroup;  // For performance optimization
};
```

### AIState_data
High-level state machine with 6 modes

```cpp
enum class AIMode {
    Idle,
    Patrol,
    Combat,
    Flee,
    Investigate,
    Dead
};

struct AIState_data {
    AIMode currentMode;
    float timeSinceStateChange;
};
```

### BehaviorTreeRuntime_data
Per-entity behavior tree execution state

```cpp
struct BehaviorTreeRuntime_data {
    int treeID;
    BTNodeState rootState;
    std::unordered_map<int, BTNodeState> nodeStates;
};
```

### MoveIntent_data
Movement intent from AI decisions

```cpp
struct MoveIntent_data {
    Vector goalPosition;
    bool hasGoal;
};
```

### AttackIntent_data
Attack intent from AI decisions

```cpp
struct AttackIntent_data {
    bool wantsToAttack;
    EntityID targetEntity;
};
```

---

## ⚙️ AI Systems

### 1. AIStimuliSystem
Consumes EventQueue events and writes to blackboard

- Processes gameplay events (PlayerFired, NoiseMade, etc.)
- Updates AI blackboards with relevant information
- First in AI system execution order

### 2. AIPerceptionSystem
Timesliced target detection (5 Hz default, configurable)

- Implements vision and hearing
- Updates blackboard with detected targets
- Optimized with timeslicing for performance

### 3. AIStateTransitionSystem
HFSM state machine with automatic behavior tree selection

- Manages high-level AI states (Idle → Combat → Flee, etc.)
- Assigns appropriate behavior trees for each state
- Handles state transitions based on blackboard data

### 4. BehaviorTreeSystem
Ticks data-driven behavior trees (10 Hz default, configurable)

- Executes behavior tree logic
- Supports 6 node types: Selector, Sequence, Condition, Action, Inverter, Repeater
- Generates movement and attack intents

### 5. AIMotionSystem
Converts intents to Movement_data (uses PhysicsBody_data speed)

- Translates AI decisions into actual movement
- Uses physics properties for realistic motion
- Last in AI system execution order

---

## 🌲 Behavior Tree System

### Node Types

#### Composite Nodes
- **Selector** - Executes children until one succeeds (OR logic)
- **Sequence** - Executes children until one fails (AND logic)

#### Decorator Nodes
- **Inverter** - Inverts child result (Success ↔ Failure)
- **Repeater** - Repeats child N times or until failure

#### Leaf Nodes
- **Condition** - Checks blackboard/component state
- **Action** - Performs an action, modifies state

### Available Conditions

1. **TargetVisible** - Is target in blackboard visible?
2. **TargetInRange** - Is target within attack range?
3. **HealthBelow** - Is health below threshold?
4. **HasMoveGoal** - Does entity have a movement goal?
5. **CanAttack** - Can entity attack right now?
6. **HeardNoise** - Did entity hear a noise?

### Available Actions

1. **SetMoveGoalToTarget** - Set movement goal to current target
2. **SetMoveGoalToLastKnownTargetPos** - Move to last known position
3. **SetMoveGoalToPatrolPoint** - Set goal to next patrol waypoint
4. **MoveToGoal** - Execute movement toward goal
5. **AttackIfClose** - Attack if target in range
6. **PatrolPickNextPoint** - Select next patrol waypoint
7. **ClearTarget** - Remove current target
8. **Idle** - Do nothing (stand still)

---

## 📋 Example Behavior Trees

### Idle Behavior

```json
{
    "tree_id": 1,
    "name": "idle",
    "root_node_id": 1,
    "nodes": [
        {
            "id": 1,
            "type": "Action",
            "action_type": "Idle"
        }
    ]
}
```

### Guard Patrol

```json
{
    "tree_id": 2,
    "name": "guard_patrol",
    "root_node_id": 1,
    "nodes": [
        {
            "id": 1,
            "type": "Sequence",
            "children": [2, 3]
        },
        {
            "id": 2,
            "type": "Action",
            "action_type": "PatrolPickNextPoint"
        },
        {
            "id": 3,
            "type": "Action",
            "action_type": "MoveToGoal"
        }
    ]
}
```

### Guard Combat

```json
{
    "tree_id": 3,
    "name": "guard_combat",
    "root_node_id": 1,
    "nodes": [
        {
            "id": 1,
            "type": "Selector",
            "children": [2, 6]
        },
        {
            "id": 2,
            "type": "Sequence",
            "children": [3, 4]
        },
        {
            "id": 3,
            "type": "Condition",
            "condition_type": "TargetInRange"
        },
        {
            "id": 4,
            "type": "Action",
            "action_type": "AttackIfClose"
        },
        {
            "id": 6,
            "type": "Sequence",
            "children": [7, 8]
        },
        {
            "id": 7,
            "type": "Action",
            "action_type": "SetMoveGoalToTarget"
        },
        {
            "id": 8,
            "type": "Action",
            "action_type": "MoveToGoal"
        }
    ]
}
```

### Investigate Noise

```json
{
    "tree_id": 4,
    "name": "investigate",
    "root_node_id": 1,
    "nodes": [
        {
            "id": 1,
            "type": "Sequence",
            "children": [2, 3]
        },
        {
            "id": 2,
            "type": "Action",
            "action_type": "SetMoveGoalToLastKnownTargetPos"
        },
        {
            "id": 3,
            "type": "Action",
            "action_type": "MoveToGoal"
        }
    ]
}
```

See `Blueprints/AI/` for complete examples.

---

## 🔄 AI Data Flow

```
Game Event                EventQueue
  (e.g., Noise)                │
                               │
                               ▼
                      AIStimuliSystem
                               │
                               ▼
                        AIBlackboard
                    (heardNoise = true)
                               │
                               ▼
                    AIPerceptionSystem
                   (updates target info)
                               │
                               ▼
                  AIStateTransitionSystem
                (Idle → Investigate state)
                               │
                               ▼
                   BehaviorTreeSystem
              (execute investigate tree)
                               │
                               ▼
                      MoveIntent_data
                      AttackIntent_data
                               │
                               ▼
                      AIMotionSystem
                 (convert to Movement_data)
                               │
                               ▼
                     MovementSystem
                   (update position)
```

---

## 🎮 Creating AI Entities

### Simple Patrolling Guard

```cpp
#include "World.h"
#include "ECS_Components.h"

// Create entity
EntityID guard = World::Get().CreateEntity();

// Add position
Position_data pos;
pos.position = {100.0f, 100.0f, 0.0f};
World::Get().AddComponent<Position_data>(guard, pos);

// Add AI components
AIBlackboard_data blackboard;
World::Get().AddComponent<AIBlackboard_data>(guard, blackboard);

AISenses_data senses;
senses.visionRange = 150.0f;
senses.visionAngle = 90.0f;
senses.hearingRange = 200.0f;
senses.timesliceGroup = 0;
World::Get().AddComponent<AISenses_data>(guard, senses);

AIState_data state;
state.currentMode = AIMode::Patrol;
World::Get().AddComponent<AIState_data>(guard, state);

// Add behavior tree
BehaviorTreeRuntime_data btRuntime;
btRuntime.treeID = 2; // guard_patrol tree
World::Get().AddComponent<BehaviorTreeRuntime_data>(guard, btRuntime);

// Add movement
PhysicsBody_data physics;
physics.speed = 50.0f;
physics.mass = 1.0f;
World::Get().AddComponent<PhysicsBody_data>(guard, physics);

MoveIntent_data moveIntent;
World::Get().AddComponent<MoveIntent_data>(guard, moveIntent);
```

---

## 📊 Performance Optimization

### Timeslicing

AI systems use timeslicing to spread expensive operations across frames:

```cpp
// AIPerceptionSystem - 5 Hz default (every 0.2 seconds)
senses.timesliceGroup = entityID % 5;  // Distribute across 5 groups

// BehaviorTreeSystem - 10 Hz default (every 0.1 seconds)
// Automatically timesliced based on entity count
```

### Configuration

Adjust update frequencies for performance:

```cpp
// In system update
if (timeSinceLastUpdate < UPDATE_INTERVAL) {
    return;  // Skip this frame
}
```

Typical frequencies:
- **Perception**: 5 Hz (0.2s) - Vision/hearing checks
- **Behavior Trees**: 10 Hz (0.1s) - Decision making
- **Motion**: Every frame - Movement execution

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../README.md)
- 🏗️ [Architecture Overview](../02-Architecture/README.md)
- 🧩 [ECS System](../03-Core-Systems/ECS/README.md)
- 📋 [Blueprint System](../03-Core-Systems/Blueprint/README.md) - For AI behavior blueprints

---

## 🎓 Learning Path

### For Beginners
1. ✅ Read this README overview
2. 📖 Study [AI Architecture](AI_ARCHITECTURE.md)
3. 🎮 Create a simple patrolling NPC (example above)
4. 📝 Modify existing behavior tree JSON files

### For Advanced Users
1. 📊 Study [BT Visual Architecture](BT_VISUAL_ARCHITECTURE.md)
2. 🔧 Read [BT Developer Guide](BT_DEVELOPER_GUIDE.md)
3. 🏗️ Create custom behavior trees
4. 🚀 Extend with custom conditions and actions

---

[← Back to Documentation Hub](../README.md) | [Next: AI Architecture →](AI_ARCHITECTURE.md)
