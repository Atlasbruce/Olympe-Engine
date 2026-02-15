/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

AI Systems implementation: NPC AI behavior systems.

*/

#include "ECS_Systems_AI.h"
#include "ECS_Components.h"
#include "ECS_Components_AI.h"
#include "AI/BehaviorTree.h"
#include "AI/BehaviorTreeDebugWindow.h"
#include "World.h"
#include "GameEngine.h"
#include "system/EventQueue.h"
#include "system/system_utils.h"
#include <cmath>
#include <algorithm>

// Forward declaration of global debugger instance (defined in OlympeEngine.cpp)
extern Olympe::BehaviorTreeDebugWindow* g_btDebugWindow;

// --- AIStimuliSystem Implementation ---

AIStimuliSystem::AIStimuliSystem()
{
    // Requires AIBlackboard component
    requiredSignature.set(GetComponentTypeID_Static<AIBlackboard_data>(), true);
}

void AIStimuliSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    const EventQueue& queue = EventQueue::Get();
    float currentTime = GameEngine::fDt;
    
    // Process Gameplay domain events for AI stimuli
    queue.ForEachDomainEvent(EventDomain::Gameplay, [&](const Message& msg) {
        
        // Handle damage/hit events
        if (msg.msg_type == EventType::EventType_Hit)
        {
            // Find AI entities that should react to this damage
            for (EntityID entity : m_entities)
            {
                try
                {
                    AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
                    
                    // If this entity was hit (targetUid matches)
                    if (msg.targetUid == entity)
                    {
                        blackboard.lastDamageTaken = currentTime;
                        blackboard.damageAmount = msg.param1;
                        
                        // Set attacker as target if we don't have one
                        if (!blackboard.hasTarget && msg.deviceId > 0)
                        {
                            EntityID attacker = static_cast<EntityID>(msg.deviceId);
                            if (World::Get().IsEntityValid(attacker))
                            {
                                blackboard.targetEntity = attacker;
                                blackboard.hasTarget = true;
                                
                                if (World::Get().HasComponent<Position_data>(attacker))
                                {
                                    const Position_data& attackerPos = World::Get().GetComponent<Position_data>(attacker);
                                    blackboard.lastKnownTargetPosition = attackerPos.position;
                                }
                            }
                        }
                    }
                }
                catch (...)
                {
                    // Entity may have been destroyed
                }
            }
        }
        
        // Handle explosion/noise events (using Game_TakeScreenshot as proxy for explosion)
        // In a real implementation, you'd add specific event types
        if (msg.msg_type == EventType::Olympe_EventType_Game_TakeScreenshot)
        {
            // Treat as explosion/noise event
            Vector noisePos(msg.param1, msg.param2, 0.0f);
            float noiseRadius = 500.0f; // Default hearing radius
            
            for (EntityID entity : m_entities)
            {
                try
                {
                    if (!World::Get().HasComponent<Position_data>(entity)) continue;
                    if (!World::Get().HasComponent<AISenses_data>(entity)) continue;
                    
                    Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                    AISenses_data& senses = World::Get().GetComponent<AISenses_data>(entity);
                    AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
                    
                    float distance = (pos.position - noisePos).Magnitude();
                    if (distance <= senses.hearingRadius)
                    {
                        blackboard.heardNoise = true;
                        blackboard.lastNoisePosition = noisePos;
                        blackboard.noiseCooldown = 3.0f; // Hear noise for 3 seconds
                    }
                }
                catch (...)
                {
                    // Entity may have been destroyed
                }
            }
        }
    });
    
    // Update noise cooldowns
    for (EntityID entity : m_entities)
    {
        try
        {
            AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
            
            if (blackboard.heardNoise)
            {
                blackboard.noiseCooldown -= GameEngine::fDt;
                if (blackboard.noiseCooldown <= 0.0f)
                {
                    blackboard.heardNoise = false;
                }
            }
        }
        catch (...)
        {
            // Entity may have been destroyed
        }
    }
}

// --- AIPerceptionSystem Implementation ---

AIPerceptionSystem::AIPerceptionSystem()
{
    // Requires Position, AIBlackboard, and AISenses
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<AIBlackboard_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<AISenses_data>(), true);
}

void AIPerceptionSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    float currentTime = GameEngine::fDt;
    
    for (EntityID entity : m_entities)
    {
        try
        {
            AISenses_data& senses = World::Get().GetComponent<AISenses_data>(entity);
            AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
            const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            
            // Timeslicing: only update perception at specified Hz
            if (currentTime < senses.nextPerceptionTime)
                continue;
            
            senses.nextPerceptionTime = currentTime + (1.0f / senses.perceptionHz);
            
            // Update target tracking if we have a target
            if (blackboard.hasTarget && blackboard.targetEntity != INVALID_ENTITY_ID)
            {
                if (!World::Get().IsEntityValid(blackboard.targetEntity))
                {
                    // Target was destroyed
                    blackboard.hasTarget = false;
                    blackboard.targetEntity = INVALID_ENTITY_ID;
                    blackboard.targetVisible = false;
                    continue;
                }
                
                // Check if target is still visible
                bool visible = IsTargetVisible(entity, blackboard.targetEntity, 
                                                senses.visionRadius, senses.visionAngle);
                
                blackboard.targetVisible = visible;
                
                if (visible)
                {
                    // Update last known position
                    if (World::Get().HasComponent<Position_data>(blackboard.targetEntity))
                    {
                        Position_data& targetPos = World::Get().GetComponent<Position_data>(blackboard.targetEntity);
                        blackboard.lastKnownTargetPosition = targetPos.position;
                        blackboard.timeSinceTargetSeen = 0.0f;
                        
                        // Update distance
                        blackboard.distanceToTarget = Vector(targetPos.position - pos.position).Magnitude();
                    }
                }
                else
                {
                    blackboard.timeSinceTargetSeen += (1.0f / senses.perceptionHz);
                    
                    // Lose target after 5 seconds of not seeing them
                    if (blackboard.timeSinceTargetSeen > 5.0f)
                    {
                        blackboard.hasTarget = false;
                        blackboard.targetEntity = INVALID_ENTITY_ID;
                    }
                }
            }
            else
            {
                // No current target - scan for potential targets
                // Naive scan: check all entities with PlayerBinding_data (players are potential targets)
                for (const auto& kv : World::Get().m_entitySignatures)
                {
                    EntityID potentialTarget = kv.first;
                    if (potentialTarget == entity) continue;
                    
                    // Only target entities with PlayerBinding_data (players)
                    if (!World::Get().HasComponent<PlayerBinding_data>(potentialTarget))
                        continue;
                    
                    if (IsTargetVisible(entity, potentialTarget, senses.visionRadius, senses.visionAngle))
                    {
                        // Found a visible target!
                        blackboard.hasTarget = true;
                        blackboard.targetEntity = potentialTarget;
                        blackboard.targetVisible = true;
                        blackboard.timeSinceTargetSeen = 0.0f;
                        
                        if (World::Get().HasComponent<Position_data>(potentialTarget))
                        {
                            Position_data& targetPos = World::Get().GetComponent<Position_data>(potentialTarget);
                            blackboard.lastKnownTargetPosition = targetPos.position;
                            blackboard.distanceToTarget = (targetPos.position - pos.position).Magnitude();
                        }
                        
                        break; // Only acquire one target at a time
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "AIPerceptionSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}

bool AIPerceptionSystem::IsTargetVisible(EntityID entity, EntityID target, float visionRadius, float visionAngle)
{
    if (!World::Get().HasComponent<Position_data>(entity)) return false;
    if (!World::Get().HasComponent<Position_data>(target)) return false;
    
    Position_data& entityPos = World::Get().GetComponent<Position_data>(entity);
    Position_data& targetPos = World::Get().GetComponent<Position_data>(target);
    
    // Check distance
    Vector toTarget = targetPos.position - entityPos.position;
    float distance = toTarget.Magnitude();
    
    if (distance > visionRadius)
        return false;
    
    // TODO: Check angle when entity has a facing direction
    // For now, assume omnidirectional vision (360 degrees)
    
    // TODO: Add line-of-sight raycasting when collision system is available
    
    return true;
}

// --- AIStateTransitionSystem Implementation ---

AIStateTransitionSystem::AIStateTransitionSystem()
{
    // Requires AIBlackboard and AIState
    requiredSignature.set(GetComponentTypeID_Static<AIBlackboard_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<AIState_data>(), true);
}

void AIStateTransitionSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    for (EntityID entity : m_entities)
    {
        try
        {
            // Always sync AIMode to blackboard first (ensures initial sync)
            AIState_data& state = World::Get().GetComponent<AIState_data>(entity);
            AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
            blackboard.AIMode = static_cast<int>(state.currentMode);
            
            UpdateAIState(entity);
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "AIStateTransitionSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}

void AIStateTransitionSystem::UpdateAIState(EntityID entity)
{
    AIState_data& state = World::Get().GetComponent<AIState_data>(entity);
    AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
    
    state.timeInCurrentMode += GameEngine::fDt;
    
    AIMode newMode = state.currentMode;
    
    // Check for flee condition (low health)
    if (World::Get().HasComponent<Health_data>(entity))
    {
        const Health_data& health = World::Get().GetComponent<Health_data>(entity);
        float healthPercent = static_cast<float>(health.currentHealth) / static_cast<float>(health.maxHealth);
        
        if (healthPercent <= state.fleeHealthThreshold)
        {
            newMode = AIMode::Flee;
        }
        else if (healthPercent <= 0.0f)
        {
            newMode = AIMode::Dead;
        }
    }
    
    // State machine logic (only if not fleeing or dead)
    if (newMode != AIMode::Flee && newMode != AIMode::Dead)
    {
        switch (state.currentMode)
        {
            case AIMode::Idle:
                if (blackboard.hasTarget)
                {
                    newMode = AIMode::Combat;
                }
                else if (blackboard.heardNoise)
                {
                    newMode = AIMode::Investigate;
                }
                else if (blackboard.patrolPointCount > 0)
                {
                    newMode = AIMode::Patrol;
                }
                break;
            
            case AIMode::Patrol:
                if (blackboard.hasTarget)
                {
                    newMode = AIMode::Combat;
                }
                else if (blackboard.heardNoise)
                {
                    newMode = AIMode::Investigate;
                }
                break;
            
            case AIMode::Combat:
                if (!blackboard.hasTarget)
                {
                    // Lost target
                    if (blackboard.timeSinceTargetSeen > 2.0f)
                    {
                        newMode = AIMode::Investigate;
                    }
                }
                break;
            
            case AIMode::Investigate:
                if (blackboard.hasTarget)
                {
                    newMode = AIMode::Combat;
                }
                else if (state.timeInCurrentMode > state.investigateTimeout)
                {
                    // Investigation timeout - return to patrol or idle
                    newMode = (blackboard.patrolPointCount > 0) ? AIMode::Patrol : AIMode::Idle;
                }
                break;
            
            case AIMode::Flee:
                // Can transition out of flee if health recovers
                if (World::Get().HasComponent<Health_data>(entity))
                {
                    const Health_data& health = World::Get().GetComponent<Health_data>(entity);
                    float healthPercent = static_cast<float>(health.currentHealth) / static_cast<float>(health.maxHealth);
                    
                    if (healthPercent > state.fleeHealthThreshold + 0.2f)
                    {
                        newMode = AIMode::Idle;
                    }
                }
                break;
            
            case AIMode::Dead:
                // No transitions from dead state
                break;
        }
    }
    
    // Apply state change
    if (newMode != state.currentMode)
    {
        state.previousMode = state.currentMode;
        state.currentMode = newMode;
        state.timeInCurrentMode = 0.0f;
        
        // IMPORTANT: Restart tree execution when mode changes
        // The unified BT will handle mode-specific behavior via CheckBlackboardValue conditions
        if (World::Get().HasComponent<BehaviorTreeRuntime_data>(entity))
        {
            BehaviorTreeRuntime_data& btRuntime = World::Get().GetComponent<BehaviorTreeRuntime_data>(entity);
            
            // Handle Dead state - disable tree execution
            if (newMode == AIMode::Dead)
            {
                btRuntime.isActive = false;
            }
            
            // DO NOT change AITreeAssetId here! It's set once from the prefab.
            // The unified BT handles all modes internally via condition checks.
            btRuntime.needsRestart = true;
        }
    }
    // Note: AIMode sync to blackboard is now done at start of Process() for all entities
}

// --- BehaviorTreeSystem Implementation ---

BehaviorTreeSystem::BehaviorTreeSystem()
{
    // Requires BehaviorTreeRuntime and AIBlackboard
    requiredSignature.set(GetComponentTypeID_Static<BehaviorTreeRuntime_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<AIBlackboard_data>(), true);
}

void BehaviorTreeSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    float currentTime = GameEngine::fDt;
    
    for (EntityID entity : m_entities)
    {
        try
        {
            BehaviorTreeRuntime_data& btRuntime = World::Get().GetComponent<BehaviorTreeRuntime_data>(entity);
            AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
			Identity_data& identity = World::Get().GetComponent<Identity_data>(entity);
            
            if (!btRuntime.isActive)
                continue;
            
            // Get think frequency from AISenses if available
            float thinkHz = 10.0f; // Default
            if (World::Get().HasComponent<AISenses_data>(entity))
            {
                const AISenses_data& senses = World::Get().GetComponent<AISenses_data>(entity);
                thinkHz = senses.thinkHz;
                
                // Timeslicing: only update BT at specified Hz
                if (currentTime < senses.nextThinkTime)
                    continue;
                
                World::Get().GetComponent<AISenses_data>(entity).nextThinkTime = currentTime + (1.0f / thinkHz);
            }
            
            // Get the behavior tree asset
            const BehaviorTreeAsset* tree = nullptr;

            if (!btRuntime.AITreePath.empty())
            {
                // Load by path (preferred method)
                tree = BehaviorTreeManager::Get().GetTreeByPath(btRuntime.AITreePath);

                if (!tree)
                {
                    std::cerr << "[BehaviorTreeSystem] WARNING: Tree not found: "
                        << btRuntime.AITreePath << " for entity " << identity.name << std::endl;
                }
            }
            else if (btRuntime.AITreeAssetId != 0)
            {
                // Fallback to ID lookup
                tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
            }

            if (!tree)
            {
                // Tree not found, skip this entity
                continue;
            }
            
            // Restart tree if needed
            if (btRuntime.needsRestart)
            {
                btRuntime.AICurrentNodeIndex = tree->rootNodeId;
                btRuntime.needsRestart = false;
            }
            
            // Get the current node
            const BTNode* node = tree->GetNode(btRuntime.AICurrentNodeIndex);
            if (!node)
            {
                // Invalid node - restart from root
                btRuntime.AICurrentNodeIndex = tree->rootNodeId;
                node = tree->GetNode(btRuntime.AICurrentNodeIndex);
            }
            
            if (node)
            {
                // Execute the node
                BTStatus status = ExecuteBTNode(*node, entity, blackboard, *tree);
                btRuntime.lastStatus = static_cast<uint8_t>(status);
                
                // Notify debugger if active
                if (g_btDebugWindow && g_btDebugWindow->IsVisible())
                {
                    g_btDebugWindow->AddExecutionEntry(entity, node->id, node->name, status);
                }
                
                // Debug logging (every 2 seconds to avoid spam)
                static float lastLogTime = 0.0f;
                if (currentTime - lastLogTime > 2.0f)
                {
                    if (World::Get().HasComponent<AIState_data>(entity))
                    {
                        const AIState_data& state = World::Get().GetComponent<AIState_data>(entity);
                        const char* modeName = "Unknown";
                        switch (state.currentMode)
                        {
                            case AIMode::Idle: modeName = "Idle"; break;
                            case AIMode::Patrol: modeName = "Patrol"; break;
                            case AIMode::Combat: modeName = "Combat"; break;
                            case AIMode::Flee: modeName = "Flee"; break;
                            case AIMode::Investigate: modeName = "Investigate"; break;
                            case AIMode::Dead: modeName = "Dead"; break;
                        }
                        
                        const char* statusName = "Unknown";
                        switch (status)
                        {
                            case BTStatus::Running: statusName = "Running"; break;
                            case BTStatus::Success: statusName = "Success"; break;
                            case BTStatus::Failure: statusName = "Failure"; break;
                        }
                        
                        SYSTEM_LOG << "BT[Entity " << entity << "]: Mode=" << modeName 
                                   << ", Tree=" << btRuntime.AITreeAssetId
                                   << ", Node=" << node->name 
                                   << ", Status=" << statusName;
                        
                        if (blackboard.hasTarget)
                            SYSTEM_LOG << ", Target=" << blackboard.targetEntity 
                                       << ", Dist=" << blackboard.distanceToTarget;
                        
                        SYSTEM_LOG << "\n";
                    }
                    lastLogTime = currentTime;
                }
                
                // If node completed (success or failure), restart tree next frame
                if (status != BTStatus::Running)
                {
                    btRuntime.needsRestart = true;
                }
            }
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "BehaviorTreeSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}

// --- AIMotionSystem Implementation ---

AIMotionSystem::AIMotionSystem()
{
    // Requires Position, MoveIntent, and Movement
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<MoveIntent_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<Movement_data>(), true);
}

void AIMotionSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    for (EntityID entity : m_entities)
    {
        try
        {
            MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            Movement_data& movement = World::Get().GetComponent<Movement_data>(entity);
            
            if (!intent.hasIntent)
            {
                // No intent - stop moving
                movement.direction = Vector(0.0f, 0.0f, 0.0f);
                movement.velocity = Vector(0.0f, 0.0f, 0.0f);
                continue;
            }
            
            // Calculate direction to target
            Vector toTarget = intent.targetPosition - pos.position;
            float distance = toTarget.Magnitude();
            
            if (distance < 1.f)
            {
                // Already at target
                movement.direction = Vector(0.0f, 0.0f, 0.0f);
                movement.velocity = Vector(0.0f, 0.0f, 0.0f);


                continue;
            }
            
            // Normalize direction
            Vector direction = toTarget * (1.0f / distance);
            
            // Get speed from PhysicsBody if available
            float speed = 100.0f; // Default speed
            if (World::Get().HasComponent<PhysicsBody_data>(entity))
            {
                const PhysicsBody_data& physics = World::Get().GetComponent<PhysicsBody_data>(entity);
                speed = physics.speed;
            }
            
            // Apply desired speed multiplier
            speed *= intent.desiredSpeed;
            
            // Set movement direction and velocity
            movement.direction = direction;
            movement.velocity = direction * speed;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "AIMotionSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}

void AIMotionSystem::RenderDebug()
{
	// Optional: render debug info for AI motion (e.g., target positions)

}
