---
id: behavior-trees-overview
title: "Behavior Trees Overview"
sidebar_label: "Overview"
---

# Behavior Trees Overview

Behavior Trees provide a flexible, hierarchical system for programming AI decision-making in Olympe Engine. They allow you to define complex AI behaviors in JSON files without writing code.

## What are Behavior Trees?

Behavior Trees (BTs) are tree structures where:
- **Nodes** represent decisions (conditions) or behaviors (actions)
- **Execution** flows from root to leaves based on node results
- **Structure** defines the AI's decision-making process

Unlike state machines, behavior trees are modular, composable, and easier to visualize and debug.

## Why Use Behavior Trees?

### Advantages

1. **Visual Structure**: Tree hierarchy is easy to understand and visualize
2. **Modularity**: Reuse subtrees across different AI types
3. **Data-Driven**: Define behaviors in JSON, no code changes needed
4. **Composability**: Build complex behaviors from simple nodes
5. **Maintainability**: Changes don't require recompilation
6. **Designer-Friendly**: Non-programmers can create and modify AI

### When to Use Behavior Trees

- **NPCs**: Guards, merchants, quest givers
- **Enemies**: Combat AI, patrol behaviors
- **Companions**: Follower AI, allied characters
- **Wildlife**: Animals with natural behaviors
- **Bosses**: Complex, multi-phase behaviors

## Node Types

### Composite Nodes

Composite nodes control execution flow through their children.

#### Selector (OR Node)
Succeeds if **any** child succeeds. Tries children left-to-right until one succeeds or all fail.

```
Selector: Attack or Patrol
├── Sequence: Attack if player nearby
│   ├── Condition: HasTarget
│   └── Action: AttackMelee
└── Action: Patrol
```

Use for: Choosing between alternative behaviors.

#### Sequence (AND Node)
Succeeds if **all** children succeed. Executes children left-to-right, stops on first failure.

```
Sequence: Chase and Attack
├── Condition: HasTarget
├── Action: MoveTo target
└── Action: AttackMelee
```

Use for: Sequential steps that must all complete.

### Leaf Nodes

Leaf nodes perform the actual work.

#### Condition Nodes
Check a condition and return Success or Failure.

**Available Conditions:**
- `TargetVisible`: Is there a valid target?
- `TargetInRange`: Is target within specified range?
- `HealthBelow`: Is health below threshold?
- `HasMoveGoal`: Is there a movement destination?
- `CanAttack`: Can entity attack now?
- `IsWaitTimerExpired`: Has wait timer finished?
- `HasNavigableDestination`: Is destination reachable?
- `HasValidPath`: Is navigation path valid?
- `HasReachedDestination`: Reached destination?

#### Action Nodes
Perform an action and return Running, Success, or Failure.

**Available Actions:**
- `MoveTo`: Move to target or goal position
- `AttackMelee`: Perform melee attack
- `SetMoveGoalToTarget`: Set movement goal to current target
- `SetMoveGoalToLastKnownTargetPos`: Move to last seen target position
- `SetMoveGoalToPatrolPoint`: Set goal to next patrol waypoint
- `PatrolPickNextPoint`: Advance to next patrol point
- `ClearTarget`: Clear current target
- `Idle`: Do nothing (always succeeds)
- `WaitRandomTime`: Wait for random duration
- `ChooseRandomNavigablePoint`: Pick random walkable destination
- `RequestPathfinding`: Request navigation path
- `FollowPath`: Follow computed navigation path

### Decorator Nodes

Decorators modify child node behavior.

#### Inverter
Inverts child result (Success ↔ Failure).

```json
{
    "type": "Inverter",
    "child": {
        "type": "Condition",
        "conditionType": "HasTarget"
    }
}
```

Use for: Negating conditions (e.g., "if NOT has target").

#### Repeater
Repeats child N times or indefinitely.

```json
{
    "type": "Repeater",
    "repeatCount": 3,
    "child": {
        "type": "Action",
        "actionType": "Patrol"
    }
}
```

Use for: Looping behaviors.

## Built-in Behaviors

Olympe Engine provides several ready-to-use behavior patterns.

### Wander Behavior

Random exploration within an area.

**How it works:**
1. Wait for random time
2. Choose random navigable destination
3. Request pathfinding to destination
4. Follow path to destination
5. Repeat

**Example:**
```json
{
    "type": "Sequence",
    "children": [
        {"type": "Action", "actionType": "WaitRandomTime", "actionParam1": 1.0, "actionParam2": 3.0},
        {"type": "Action", "actionType": "ChooseRandomNavigablePoint", "actionParam1": 200.0},
        {"type": "Action", "actionType": "RequestPathfinding"},
        {"type": "Action", "actionType": "FollowPath"}
    ]
}
```

**Use for:** Wildlife, idle NPCs, background characters.

### Patrol Behavior

Follow a predefined route of waypoints.

**How it works:**
1. Set goal to current patrol point
2. Move to patrol point
3. Pick next patrol point
4. Repeat

**Example:**
```json
{
    "type": "Sequence",
    "children": [
        {"type": "Action", "actionType": "SetMoveGoalToPatrolPoint"},
        {"type": "Action", "actionType": "MoveTo"},
        {"type": "Action", "actionType": "PatrolPickNextPoint"}
    ]
}
```

**Use for:** Guards, sentries, NPCs with routes.

### Chase Behavior

Pursue and attack a target.

**How it works:**
1. Check if target visible
2. Move toward target
3. Attack when in range

**Example:**
```json
{
    "type": "Selector",
    "children": [
        {
            "type": "Sequence",
            "children": [
                {"type": "Condition", "conditionType": "TargetVisible"},
                {"type": "Condition", "conditionType": "TargetInRange", "conditionParam": 50.0},
                {"type": "Action", "actionType": "AttackMelee"}
            ]
        },
        {
            "type": "Sequence",
            "children": [
                {"type": "Condition", "conditionType": "TargetVisible"},
                {"type": "Action", "actionType": "SetMoveGoalToTarget"},
                {"type": "Action", "actionType": "MoveTo"}
            ]
        }
    ]
}
```

**Use for:** Aggressive enemies, pursuing NPCs.

### Flee Behavior

Escape from danger.

**How it works:**
1. Check if threatened (target too close)
2. Move away from target
3. Find safe location

**Example:**
```json
{
    "type": "Sequence",
    "children": [
        {"type": "Condition", "conditionType": "TargetInRange", "conditionParam": 100.0},
        {"type": "Action", "actionType": "ChooseRandomNavigablePoint", "actionParam1": 300.0},
        {"type": "Action", "actionType": "MoveTo"}
    ]
}
```

**Use for:** Cowardly NPCs, low-health enemies, fleeing civilians.

## Assigning Behavior Trees to Entities

### In Blueprints

Add `AIBehaviorTree` and `AIBlackboard` components:

```json
{
    "name": "PatrolGuard",
    "components": [
        {
            "type": "AIBlackboard",
            "properties": {
                "detectionRadius": 200.0,
                "attackRadius": 50.0,
                "patrolPoints": [
                    {"x": 100, "y": 100},
                    {"x": 300, "y": 100},
                    {"x": 300, "y": 300},
                    {"x": 100, "y": 300}
                ]
            }
        },
        {
            "type": "AIBehaviorTree",
            "properties": {
                "behaviorTreePath": "Blueprints/AI/patrol_behavior.json"
            }
        }
    ]
}
```

### In Code

```cpp
// Create entity
EntityID guard = World::GetInstance().CreateEntity();

// Add AI blackboard
AIBlackboard_data blackboard;
blackboard.detectionRadius = 200.0f;
blackboard.attackRadius = 50.0f;
World::GetInstance().AddComponent<AIBlackboard_data>(guard, blackboard);

// Load and add behavior tree
AIBehaviorTree_data btData;
btData.behaviorTreePath = "Blueprints/AI/patrol_behavior.json";
// Load tree from JSON...
World::GetInstance().AddComponent<AIBehaviorTree_data>(guard, btData);
```

### Required Components

Entities with behavior trees must have:
- **`AIBlackboard_data`**: AI state and memory
- **`AIBehaviorTree_data`**: Behavior tree structure and execution state
- **`Position_data`**: Required for movement actions
- **`Velocity_data`**: Optional, for smooth movement

## AI Blackboard

The blackboard stores AI state and shared data:

```cpp
struct AIBlackboard_data {
    EntityID targetEntity;           // Current target
    Vector moveGoal;                 // Movement destination
    std::vector<Vector> patrolPoints; // Patrol route waypoints
    int currentPatrolIndex;          // Current patrol point
    float detectionRadius;           // How far can see
    float attackRadius;              // Attack range
    float health;                    // Current health
    Vector lastKnownTargetPos;       // Last seen target position
    float waitTimer;                 // Timer for wait actions
    // ... more state variables
};
```

Conditions and actions read/write blackboard data.

## Behavior Tree JSON Format

```json
{
    "schema_version": 1,
    "type": "BehaviorTree",
    "name": "GuardBehavior",
    "description": "Guard that patrols and chases intruders",
    "root": {
        "type": "Selector",
        "children": [
            {
                "type": "Sequence",
                "children": [
                    {
                        "type": "Condition",
                        "conditionType": "TargetVisible"
                    },
                    {
                        "type": "Action",
                        "actionType": "SetMoveGoalToTarget"
                    },
                    {
                        "type": "Action",
                        "actionType": "MoveTo"
                    }
                ]
            },
            {
                "type": "Action",
                "actionType": "Patrol"
            }
        ]
    }
}
```

## System Processing

The `AISystem` processes all entities with behavior trees every frame:

```cpp
class AISystem : public ECS_System {
    virtual void Process() override {
        for (EntityID entity : m_entities) {
            AIBehaviorTree_data* bt = GetComponent<AIBehaviorTree_data>(entity);
            AIBlackboard_data* blackboard = GetComponent<AIBlackboard_data>(entity);
            
            // Evaluate behavior tree
            BTStatus status = EvaluateNode(bt->rootNode, blackboard, entity);
            
            // Update based on status
            // ...
        }
    }
};
```

## Debugging Behavior Trees

### Visual Debugging
- Use Blueprint Editor to visualize tree structure
- Enable AI debug rendering in engine (shows paths, targets, radii)

### Logging
```cpp
// Enable AI logging
SYSTEM_LOG << "AI Entity " << entity << " status: " << BTStatusToString(status) << "\n";
```

### Common Issues
- **AI stuck**: Check if pathfinding is blocked, increase `detectionRadius`
- **AI not detecting**: Ensure `targetEntity` is set and in range
- **Actions failing**: Verify required components exist (Position, Velocity, etc.)

## Best Practices

1. **Start Simple**: Begin with basic Selector/Sequence trees
2. **Reuse Subtrees**: Factor common patterns into reusable JSON files
3. **Test Incrementally**: Add one behavior at a time
4. **Use Descriptive Names**: Clear node names help debugging
5. **Balance Tree Depth**: Deep trees are hard to understand; prefer wider trees
6. **Cache Conditions**: Don't repeat expensive checks
7. **Profile Performance**: Monitor frame time with many AI entities

## Related Documentation

- [Dependency Loading](./dependency-loading.md) - How behavior tree JSON files are loaded
- [ECS Overview](../architecture/ecs-overview.md) - Understanding the component system
- [Blueprint System](../../user-guide/blueprints/overview.md) - Creating AI entities
- [AI Components](../../api-reference/components.md) - AIBlackboard and AIBehaviorTree details

## Example Files

Explore example behavior trees in:
- `Blueprints/AI/wander_behavior.json`
- `Blueprints/AI/patrol_behavior.json`
- `Blueprints/AI/chase_behavior.json`

Study these to understand real-world behavior tree design.
