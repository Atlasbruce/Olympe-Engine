# ⚡ AI Systems - Quick Reference

**One-page reference for AI pipeline** | [📖 Full Guide](AI_Blackboard_Guide.md)

---

## 🤖 AI System Pipeline

AI systems execute in **strict order** to ensure correct data flow:

```
1. AIStimuliSystem       → Detect stimuli (noise, damage, events)
2. AIPerceptionSystem    → Update vision, hearing, target tracking
3. AIStateTransitionSystem → Switch modes (Idle → Combat → Flee)
4. BehaviorTreeSystem    → Execute behavior tree logic
5. AIMotionSystem        → Convert AI decisions to MoveIntent
```

**Critical Rule:** Systems MUST run in this order for AI to work correctly!

---

## 📊 AI System Overview

| System | Reads | Writes | Purpose |
|--------|-------|--------|---------|
| **AIStimuliSystem** | Events | `AIBlackboard.heardNoise` | Detect noise, damage |
| **AIPerceptionSystem** | `Position`, `AISenses` | `AIBlackboard.targetEntity` | Vision & hearing checks |
| **AIStateTransitionSystem** | `AIBlackboard` | `AIState.currentMode` | Mode switching logic |
| **BehaviorTreeSystem** | `BehaviorTreeRuntime`, `AIBlackboard` | `MoveIntent`, `AttackIntent` | Execute BT nodes |
| **AIMotionSystem** | `AIBlackboard.moveGoal` | `MoveIntent` | Movement decisions |

---

## 🧠 AIBlackboard_data - Central Memory

The blackboard is shared state for all AI systems:

### Target Tracking
```cpp
EntityID targetEntity;           // Current target
Vector lastKnownTargetPosition;  // Last seen position
float timeSinceTargetSeen;       // Time since last contact
bool hasTarget;                  // Target exists?
```

### Perception State
```cpp
float distanceToTarget;          // Distance to target
bool targetVisible;              // In line of sight?
bool targetInRange;              // Within attack range?
```

### Movement Goals
```cpp
Vector moveGoal;                 // Where to move
bool hasMoveGoal;                // Goal set?
```

### Wander State
```cpp
Vector wanderDestination;        // Random wander target
bool hasWanderDestination;       // Destination valid?
float wanderWaitTimer;           // Current wait time
float wanderTargetWaitTime;      // Target wait duration
```

### Combat State
```cpp
float lastAttackTime;            // Last attack timestamp
float attackCooldown;            // Time between attacks
bool canAttack;                  // Ready to attack?
```

---

## 🎯 Common AI Patterns

### Pattern 1: Target Acquisition

```cpp
// AIPerceptionSystem: Detect target
if (CanSeeEntity(entity, targetEntity))
{
    blackboard.targetEntity = targetEntity;
    blackboard.targetVisible = true;
    blackboard.lastKnownTargetPosition = targetPos;
    blackboard.timeSinceTargetSeen = 0.0f;
    blackboard.hasTarget = true;
}
```

### Pattern 2: State Transitions

```cpp
// AIStateTransitionSystem: Switch to combat
if (blackboard.hasTarget && blackboard.targetVisible)
{
    if (state.currentMode == AIMode::Idle || state.currentMode == AIMode::Patrol)
    {
        state.currentMode = AIMode::Combat;
        SYSTEM_LOG << "Entity " << entity << " entering combat\n";
    }
}
```

### Pattern 3: Behavior Tree Decision

```cpp
// BehaviorTreeSystem: Attack decision
if (blackboard.distanceToTarget < attackRange && blackboard.canAttack)
{
    AttackIntent_data& intent = GetComponent<AttackIntent_data>(entity);
    intent.targetEntity = blackboard.targetEntity;
    intent.hasIntent = true;
    
    blackboard.canAttack = false;  // Set cooldown
    blackboard.lastAttackTime = currentTime;
}
```

### Pattern 4: Movement Execution

```cpp
// AIMotionSystem: Execute move
if (blackboard.hasMoveGoal)
{
    MoveIntent_data& intent = GetComponent<MoveIntent_data>(entity);
    intent.targetPosition = blackboard.moveGoal;
    intent.desiredSpeed = 1.0f;
    intent.hasIntent = true;
}
```

---

## 🔄 Data Flow Diagram

```
[Game Events]
      ↓
[AIStimuliSystem] → Updates blackboard.heardNoise
      ↓
[AIPerceptionSystem] → Updates blackboard.targetEntity, distanceToTarget
      ↓
[AIStateTransitionSystem] → Updates state.currentMode based on blackboard
      ↓
[BehaviorTreeSystem] → Reads blackboard, sets MoveIntent/AttackIntent
      ↓
[AIMotionSystem] → Reads blackboard.moveGoal, sets MoveIntent
      ↓
[NavigationSystem] → Pathfinding
      ↓
[MovementSystem] → Applies movement
```

---

## 🎭 AI Modes (AIState_data)

| Mode | Behavior | Transitions |
|------|----------|-------------|
| **Idle** | Stand still, idle animation | → Patrol (patrol path set)<br>→ Combat (target detected)<br>→ Investigate (heard noise) |
| **Patrol** | Follow patrol path | → Idle (no patrol points)<br>→ Combat (target detected) |
| **Combat** | Attack target | → Flee (low health)<br>→ Investigate (target lost)<br>→ Idle (target dead) |
| **Flee** | Run away | → Idle (safe distance)<br>→ Dead (health depleted) |
| **Investigate** | Check noise location | → Combat (target found)<br>→ Idle (nothing found) |
| **Dead** | Inactive | (No transitions) |

---

## ⚙️ AISenses_data - Perception Config

```cpp
struct AISenses_data
{
    float sightRange = 300.0f;       // Vision distance
    float hearingRange = 500.0f;     // Sound detection distance
    float sightAngle = 90.0f;        // Vision cone (degrees)
    float thinkHz = 10.0f;           // AI updates per second
    float nextThinkTime = 0.0f;      // Next scheduled update
};
```

**Performance:** `thinkHz` controls AI update frequency. Lower values improve performance but reduce responsiveness.

---

## 🧪 Testing AI Systems

```cpp
// Test perception
EntityID npc = CreateNPC();
EntityID target = CreateTarget();

// Position target in sight range
SetPosition(target, npcPos + Vector(100, 0));  // 100 units away

// Run AI pipeline
World::Get().Update();

// Verify target detected
AIBlackboard_data& bb = GetComponent<AIBlackboard_data>(npc);
ASSERT_TRUE(bb.hasTarget);
ASSERT_EQ(bb.targetEntity, target);
ASSERT_TRUE(bb.targetVisible);
```

---

## ⚠️ Common Mistakes

### ❌ Wrong System Order
```cpp
// BAD: BT before perception
RegisterSystem(std::make_shared<BehaviorTreeSystem>());
RegisterSystem(std::make_shared<AIPerceptionSystem>());  // Too late!
```

**Fix:** Perception → State → BT → Motion

### ❌ Forgetting to Clear Flags
```cpp
// BAD: Attack intent never cleared
intent.hasIntent = true;  // Set but never cleared → attacks every frame!
```

**Fix:** Clear intents after execution or on cooldown

### ❌ Direct State Modification
```cpp
// BAD: Bypassing state machine
state.currentMode = AIMode::Combat;  // Skips transition logic!
```

**Fix:** Use `AIStateTransitionSystem` for all mode changes

---

## 📚 See Also

- [Full AI Guide](AI_Blackboard_Guide.md) - Detailed blackboard patterns
- [Behavior Trees](../04_Behavior_Trees/_QuickRef_BehaviorTrees.md) - BT reference
- [Systems Quick Ref](../02_ECS_Systems/_QuickRef_Systems.md) - All systems

---

**💡 Pro Tip:** Enable AI debug logging with `SYSTEM_LOG` in `AIStimuliSystem::Process()` to trace AI decisions!
