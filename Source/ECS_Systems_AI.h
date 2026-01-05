/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

AI Systems purpose: Define ECS systems for NPC AI behavior.

*/

#pragma once

#include "ECS_Entity.h"
#include "ECS_Systems.h"
#include <set>

// --- AIStimuliSystem ---
// Consumes EventQueue domain events (Gameplay, Detection, Collision) and writes
// relevant stimulus results into AIBlackboard_data
class AIStimuliSystem : public ECS_System
{
public:
    AIStimuliSystem();
    virtual void Process() override;
};

// --- AIPerceptionSystem ---
// Timesliced perception system that updates blackboard perception state
// For now uses naive scan; structured for future spatial partition integration
class AIPerceptionSystem : public ECS_System
{
public:
    AIPerceptionSystem();
    virtual void Process() override;
    
private:
    // Helper: Check if target is visible from entity position
    bool IsTargetVisible(EntityID entity, EntityID target, float visionRadius, float visionAngle);
};

// --- AIStateTransitionSystem ---
// HFSM that selects AI mode and chooses which behavior tree to run based on blackboard + state
class AIStateTransitionSystem : public ECS_System
{
public:
    AIStateTransitionSystem();
    virtual void Process() override;
    
private:
    // Helper: Determine next state based on current state and blackboard
    void UpdateAIState(EntityID entity);
};

// --- BehaviorTreeSystem ---
// Ticks data-driven Behavior Trees loaded from JSON and writes intents (MoveIntent/AttackIntent)
// and/or blackboard updates
class BehaviorTreeSystem : public ECS_System
{
public:
    BehaviorTreeSystem();
    virtual void Process() override;
};

// --- AIMotionSystem ---
// Converts intents into Movement_data (direction/velocity) and uses PhysicsBody_data speed if present
class AIMotionSystem : public ECS_System
{
public:
    AIMotionSystem();
    virtual void Process() override;
	virtual void RenderDebug() override;
};
