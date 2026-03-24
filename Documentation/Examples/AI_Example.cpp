/*
Olympe Engine V2 - 2025
AI System Example

This example demonstrates how to create an NPC with AI behavior using the new ECS-friendly
AI architecture with EventQueue integration.

To use this in your game:
1. Load behavior tree assets at game initialization
2. Create NPC entities with AI components
3. Emit events to trigger AI reactions
*/

#include "../Source/World.h"
#include "../Source/ECS_Components.h"
#include "../Source/ECS_Components_AI.h"
#include "../Source/AI/BehaviorTree.h"
#include "../Source/AI/AIEvents.h"

// Example: Creating a Guard NPC
EntityID CreateGuardNPC(const Vector& position)
{
    EntityID guard = World::Get().CreateEntity();
    
    // Core components
    World::Get().AddComponent<Position_data>(guard, position);
    World::Get().AddComponent<Movement_data>(guard);
    World::Get().AddComponent<PhysicsBody_data>(guard, 1.0f, 120.0f);
    World::Get().AddComponent<Health_data>(guard, 100, 100);
    World::Get().AddComponent<BoundingBox_data>(guard);
    
    // AI components
    World::Get().AddComponent<AIBlackboard_data>(guard);
    World::Get().AddComponent<AISenses_data>(guard);
    World::Get().AddComponent<AIState_data>(guard);
    World::Get().AddComponent<BehaviorTreeRuntime_data>(guard);
    World::Get().AddComponent<MoveIntent_data>(guard);
    World::Get().AddComponent<AttackIntent_data>(guard);
    
    // Configure AI
    AISenses_data& senses = World::Get().GetComponent<AISenses_data>(guard);
    senses.visionRadius = 400.0f;
    senses.hearingRadius = 600.0f;
    
    // Configure patrol
    AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(guard);
    blackboard.patrolPoints[0] = position + Vector(-50.0f, -50.0f, 0.0f);
    blackboard.patrolPoints[1] = position + Vector(50.0f, -50.0f, 0.0f);
    blackboard.patrolPoints[2] = position + Vector(50.0f, 50.0f, 0.0f);
    blackboard.patrolPoints[3] = position + Vector(-50.0f, 50.0f, 0.0f);
    blackboard.patrolPointCount = 4;
    
    // Activate behavior tree
    BehaviorTreeRuntime_data& btRuntime = World::Get().GetComponent<BehaviorTreeRuntime_data>(guard);
    btRuntime.treeAssetId = 2; // Patrol tree
    btRuntime.isActive = true;
    
    AIState_data& state = World::Get().GetComponent<AIState_data>(guard);
    state.currentMode = AIMode::Patrol;
    
    return guard;
}

// Example: Load AI behavior trees
void LoadAIBehaviorTrees()
{
    BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_patrol.json", 2);
    BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_combat.json", 3);
}

// Example: Emit AI events
void EmitExplosionEvent(const Vector& position, float radius)
{
    AIEvents::EmitExplosion(position, radius);
}

void EmitNoiseEvent(const Vector& position)
{
    AIEvents::EmitNoise(position, 1.0f);
}

void EmitDamageEvent(EntityID victim, EntityID attacker, float damage)
{
    AIEvents::EmitDamageDealt(victim, attacker, damage);
}
