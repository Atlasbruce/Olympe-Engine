/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

AI Components purpose: Define all AI-related components for the ECS architecture.

*/

#pragma once

#include "ECS_Entity.h"
#include "vector.h"
#include <cstdint>
#include "ECS_Systems_AI.h"

// --- AI Blackboard Component ---
// Typed blackboard with explicit fields for performance (no std::map/std::string keys in hot path)
struct AIBlackboard_data
{
    // Target tracking
    EntityID targetEntity = INVALID_ENTITY_ID;
    Vector lastKnownTargetPosition;
    float timeSinceTargetSeen = 0.0f;
    bool hasTarget = false;
    
    // Perception state
    float distanceToTarget = 0.0f;
    bool targetVisible = false;
    bool targetInRange = false;
    
    // Movement goals
    Vector moveGoal;
    bool hasMoveGoal = false;
    
    // Patrol state
    int currentPatrolPoint = 0;
    int currentPatrolIndex = 0;
    Vector patrolPoints[8]; // Fixed size for performance
    int patrolPointCount = 0;
    bool hasPatrolPath = false; 
    
    // Combat state
    float lastAttackTime = 0.0f;
    float attackCooldown = 1.0f;
    bool canAttack = true;
    
    // Stimulus tracking
    float lastDamageTaken = 0.0f;
    float damageAmount = 0.0f;
    Vector lastNoisePosition;
    bool heardNoise = false;
    float noiseCooldown = 0.0f;
};

// --- AI Senses Component ---
// Perception parameters for the AI entity
struct AISenses_data
{
    float visionRadius = 300.0f;      // How far the entity can see
    float visionAngle = 180.0f;       // Field of view in degrees (360 = omnidirectional)
    float hearingRadius = 500.0f;     // How far the entity can hear sounds
    
    // Timeslicing parameters for performance
    float perceptionHz = 5.0f;        // Perception updates per second (default: 5 Hz = every 0.2s)
    float thinkHz = 10.0f;            // Decision/BT updates per second (default: 10 Hz = every 0.1s)
    
    // Internal timers (updated by systems)
    float nextPerceptionTime = 0.0f;
    float nextThinkTime = 0.0f;
    
    // Constructors
    AISenses_data() = default;
    AISenses_data(float vision, float hearing) 
        : visionRadius(vision), hearingRadius(hearing) {}
};

// --- AI State Component (HFSM) ---
// Hierarchical Finite State Machine mode/state
enum class AIMode : uint8_t
{
    Idle = 0,
    Patrol,
    Combat,
    Flee,
    Investigate,
    Dead
};

struct AIState_data
{
    AIMode currentMode = AIMode::Idle;
    AIMode previousMode = AIMode::Idle;
    float timeInCurrentMode = 0.0f;
    
    // State transition thresholds
    float combatEngageDistance = 250.0f;
    float fleeHealthThreshold = 0.2f;      // Flee when health below 20%
    float investigateTimeout = 5.0f;        // Time to investigate before returning to patrol
};

// --- Behavior Tree Runtime Component ---
// Per-entity behavior tree execution state
struct BehaviorTreeRuntime_data
{
    // Tree identification
    uint32_t treeAssetId = 0;           // ID of the behavior tree asset to execute
    
    // Execution state
    uint32_t currentNodeIndex = 0;       // Index of the currently executing node
    uint8_t lastStatus = 0;              // Last node execution status (0=Running, 1=Success, 2=Failure)
    
    // Timeslicing state
    float nextThinkTime = 0.0f;          // When to next tick the behavior tree
    
    // Tree execution control
    bool isActive = true;                // Enable/disable tree execution
    bool needsRestart = false;           // Flag to restart tree from root
    
    // Constructors
    BehaviorTreeRuntime_data() = default;
    BehaviorTreeRuntime_data(uint32_t treeId, bool active) 
        : treeAssetId(treeId), isActive(active) {}
};

// --- Move Intent Component ---
// Movement intent that will be converted to Movement_data by AIMotionSystem
struct MoveIntent_data
{
    Vector targetPosition;               // Where to move
    float desiredSpeed = 100.0f;          // Speed multiplier (0.0 to 1.0, can exceed 1.0 for sprint)
    bool hasIntent = false;              // Whether this intent is active
    float arrivalThreshold = 5.0f;       // Distance at which we consider arrived
    
    // Pathfinding flags (for future expansion)
    bool usePathfinding = false;
    bool avoidObstacles = false;
};

// --- Attack Intent Component ---
// Attack intent for combat actions
struct AttackIntent_data
{
    EntityID targetEntity = INVALID_ENTITY_ID;
    Vector targetPosition;               // Attack position if no specific entity
    float damage = 10.0f;                // Damage to deal
    float range = 50.0f;                 // Attack range
    bool hasIntent = false;              // Whether this intent is active
    
    // Attack type (can be extended)
    enum class AttackType : uint8_t
    {
        Melee = 0,
        Ranged,
        Area
    };
    AttackType attackType = AttackType::Melee;
};
