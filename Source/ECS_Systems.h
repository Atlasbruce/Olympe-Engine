/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Systems purpose: Define systems that operate on entities with specific components.

*/

#pragma once

#include "ECS_Entity.h"
#include <set>

// Execution phases for ECS systems
// Systems are executed in the order defined here to ensure consistent and predictable behavior
enum class ECS_ExecutionPhase
{
    // Input collection phase: Raw input from hardware devices
    InputCollect = 0,
    
    // Input mapping phase: Map hardware input to gameplay actions
    InputMap = 1,
    
    // Gameplay phase: Player control, AI behaviors, game logic
    Gameplay = 2,
    
    // Physics phase: Movement, collision detection, physics simulation
    Physics = 3,
    
    // Camera update phase: Update camera positions based on targets
    CameraUpdate = 4,
    
    // Render phase: Visual effects and rendering
    Render = 5,
    
    // Future phases (hooks for extensibility):
    // Network = 6,      // Network synchronization
    // Debug = 7,        // Debug visualization and tools
    // Resources = 8     // Resource loading and management
};

// The System class handles game logic over entities with specific components.
class ECS_System
{
public:
    // The signature required for an Entity to be processed by this System
    ComponentSignature requiredSignature;

    // The set of Entities this System processes in its Update loop
    std::set<EntityID> m_entities;

    // Execution phase for this system (determines order of execution)
    ECS_ExecutionPhase executionPhase;

    ECS_System() : requiredSignature(), executionPhase(ECS_ExecutionPhase::Gameplay) {}

    // The core logic of the System
    virtual void Process() {}
	virtual void Render() {}

    void AddEntity(EntityID entity) { m_entities.insert(entity); }
    void RemoveEntity(EntityID entity) { m_entities.erase(entity); }
};


// Input System: processes entities with Input_data
class InputSystem : public ECS_System
{
    public:
    InputSystem();
	virtual void Process() override;
 };
// AI System: processes entities with AIBehavior_data and Movement_data
class AISystem : public ECS_System
{
    public:
    AISystem();
    virtual void Process() override;
};
// Detection System: processes entities with TriggerZone_data and Position_data
class DetectionSystem : public ECS_System
{
    public:
    DetectionSystem();
    virtual void Process() override;
};
// Physics System: processes entities with Transform_data and PhysicsBody_data
class PhysicsSystem : public ECS_System
{
    public:
    PhysicsSystem();
    virtual void Process() override;
};
// Collision System: processes entities with BoundingBox_data and Position_data
class CollisionSystem : public ECS_System
{
    public:
    CollisionSystem();
    virtual void Process() override;
};
// Trigger System: processes entities with TriggerZone_data and Position_data
class TriggerSystem : public ECS_System
{
    public:
    TriggerSystem();
    virtual void Process() override;
};
// Movement System: processes entities with Transform_data and Movement_data
class MovementSystem : public ECS_System
{
    public:
    MovementSystem();
    virtual void Process() override;
};
// Rendering System: processes entities with Transform_data and VisualSprite_data
class RenderingSystem : public ECS_System
{
public:
    RenderingSystem();
	virtual void Render() override;
};
// Player Control System: processes entities with PlayerBinding_data and Controller_data
class PlayerControlSystem : public ECS_System
{
    public:
    PlayerControlSystem();
    virtual void Process() override;
};
// Input Mapping System: maps hardware input to gameplay actions
class InputMappingSystem : public ECS_System
{
    public:
    InputMappingSystem();
    virtual void Process() override;
};

//-------------------------------------------------------------
// Olympe Effect System: Plasma bloom background with glowing orbs
// No entity required - autonomous visual effect system
class OlympeEffectSystem : public ECS_System
{
public:
    OlympeEffectSystem();
    virtual ~OlympeEffectSystem();
    
    virtual void Process() override;
    virtual void Render() override;
    
    void Initialize();

private:
    // Implementation details hidden - defined in OlympeEffectSystem.cpp
    struct Implementation;
    Implementation* pImpl;
};