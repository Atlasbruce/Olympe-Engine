/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Systems purpose: Define systems that operate on entities with specific components.

*/

#pragma once

#include "ECS_Entity.h"
#include "ECS_Events.h"
#include "ECS_Components.h"
#include <set>


// The System class handles game logic over entities with specific components.
class ECS_System
{
public:
    // The signature required for an Entity to be processed by this System
    ComponentSignature requiredSignature;

    // The set of Entities this System processes in its Update loop
    std::set<EntityID> m_entities;

    ECS_System() : requiredSignature() {}

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
// EventQueue System: manages global event queues for decoupled event handling
// This system provides typed event queues for different event categories
// No entity required - autonomous event management system
class EventQueueSystem : public ECS_System
{
public:
    EventQueueSystem();
    virtual void Process() override;
    
    // Post an event to a specific queue
    bool PostEvent(ECSEventType type, const Event& event);
    
    // Consume events of a specific type (calls callback for each event)
    template<typename Func>
    void ConsumeEvents(ECSEventType type, Func callback);
    
    // Peek at events without consuming them
    template<typename Func>
    void PeekEvents(ECSEventType type, Func callback) const;
    
    // Clear all events of a specific type
    void ClearEvents(ECSEventType type);
    
    // Clear all events in all queues
    void ClearAllEvents();
    
    // Get number of events in a specific queue
    size_t GetEventCount(ECSEventType type) const;

private:
    // Separate event queues for different event types
    EventQueue_data m_eventQueues[static_cast<size_t>(ECSEventType::MAX_EVENT_TYPES)];
};

// Template method implementations (must be in header)
template<typename Func>
void EventQueueSystem::ConsumeEvents(ECSEventType type, Func callback)
{
    size_t queueIndex = static_cast<size_t>(type);
    if (queueIndex >= static_cast<size_t>(ECSEventType::MAX_EVENT_TYPES))
        return;
    
    EventQueue_data& queue = m_eventQueues[queueIndex];
    Event evt;
    
    // Process all events in the queue
    while (queue.PopEvent(evt))
    {
        callback(evt);
    }
}

template<typename Func>
void EventQueueSystem::PeekEvents(ECSEventType type, Func callback) const
{
    size_t queueIndex = static_cast<size_t>(type);
    if (queueIndex >= static_cast<size_t>(ECSEventType::MAX_EVENT_TYPES))
        return;
    
    const EventQueue_data& queue = m_eventQueues[queueIndex];
    
    // Peek at all events without removing them
    size_t currentRead = queue.readIndex;
    size_t remaining = queue.count;
    
    while (remaining > 0)
    {
        callback(queue.events[currentRead]);
        currentRead = (currentRead + 1) % EventQueue_data::QUEUE_CAPACITY;
        --remaining;
    }
}

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