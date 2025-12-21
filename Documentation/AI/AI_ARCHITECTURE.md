# AI Architecture Guide

## Overview

The Olympe Engine features a modern, performant hybrid AI architecture designed for NPC behavior. The system is ECS-first, data-oriented, and scalable to hundreds of NPCs.

See the full documentation at: [Complete AI Architecture Guide](https://github.com/Atlasbruce/Olympe-Engine/blob/main/Documentation/AI/AI_ARCHITECTURE.md)

## Quick Start

### Creating an NPC with AI

```cpp
// Create entity
EntityID guard = World::Get().CreateEntity();

// Add core components
World::Get().AddComponent<Position_data>(guard, Vector(200.0f, 200.0f, 0.0f));
World::Get().AddComponent<Movement_data>(guard);
World::Get().AddComponent<PhysicsBody_data>(guard, 1.0f, 120.0f);
World::Get().AddComponent<Health_data>(guard, 100, 100);

// Add AI components
World::Get().AddComponent<AIBlackboard_data>(guard);
World::Get().AddComponent<AISenses_data>(guard);
World::Get().AddComponent<AIState_data>(guard);
World::Get().AddComponent<BehaviorTreeRuntime_data>(guard);
World::Get().AddComponent<MoveIntent_data>(guard);
World::Get().AddComponent<AttackIntent_data>(guard);

// Load behavior trees
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_patrol.json", 2);
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_combat.json", 3);

// Configure and activate
BehaviorTreeRuntime_data& btRuntime = World::Get().GetComponent<BehaviorTreeRuntime_data>(guard);
btRuntime.treeAssetId = 2; // Patrol tree
btRuntime.isActive = true;
```

### Emitting AI Events

```cpp
#include "AI/AIEvents.h"

// Explosion
AIEvents::EmitExplosion(Vector(500.0f, 300.0f, 0.0f), 200.0f);

// Noise
AIEvents::EmitNoise(Vector(400.0f, 250.0f, 0.0f), 1.0f);

// Damage
AIEvents::EmitDamageDealt(victimEntity, attackerEntity, 25.0f);
```

## AI Pipeline

```
EventQueue → AIBlackboard → HFSM → Behavior Tree → Intents → Motion
     ↓            ↑           ↓          ↓            ↓         ↓
Stimuli      Perception    State      Actions     Intents  Movement
```

## Components

- **AIBlackboard_data**: Typed blackboard for AI reasoning
- **AISenses_data**: Vision/hearing parameters and timeslicing
- **AIState_data**: HFSM modes (Idle/Patrol/Combat/Flee/Investigate/Dead)
- **BehaviorTreeRuntime_data**: Behavior tree execution state
- **MoveIntent_data**: Movement intent
- **AttackIntent_data**: Attack intent

## Systems (Execution Order)

1. **AIStimuliSystem**: Consumes events → writes to blackboard
2. **AIPerceptionSystem**: Scans for targets → updates blackboard
3. **AIStateTransitionSystem**: HFSM transitions → selects behavior tree
4. **BehaviorTreeSystem**: Ticks tree → writes intents
5. **AIMotionSystem**: Converts intents → writes Movement_data
6. **MovementSystem**: Applies movement

## Scaling

- **Current**: 100-500 NPCs (5 Hz perception, 10 Hz thinking)
- **1000+ NPCs**: Use LOD, spatial partitioning, state-based optimization

## Example Behavior Trees

See `Blueprints/AI/guard_patrol.json` and `Blueprints/AI/guard_combat.json` for examples.

## References

- Full Documentation: `Documentation/AI/AI_ARCHITECTURE.md`
- AI Components: `Source/ECS_Components_AI.h`
- AI Systems: `Source/ECS_Systems_AI.h`
- Behavior Tree: `Source/AI/BehaviorTree.h`
- AI Events: `Source/AI/AIEvents.h`
