/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Events purpose: Define typed event structures for the EventQueue ECS system.
This replaces ad-hoc messaging with strongly-typed event structs.

*/

#pragma once

#include "ECS_Entity.h"
#include "vector.h"
#include <string>
#include <SDL3/SDL.h>

// ============================================================================
// Event Type Enumeration
// ============================================================================

enum class ECSEventType
{
    // Input Events
    InputButton,
    InputAxis,
    InputAction,
    
    // Entity Lifecycle Events
    EntityJoined,
    EntityLeft,
    EntitySpawned,
    EntityDestroyed,
    
    // Gameplay Events
    CollisionStarted,
    CollisionEnded,
    TriggerEntered,
    TriggerExited,
    HealthChanged,
    ItemCollected,
    
    // System Events
    LevelLoaded,
    LevelUnloaded,
    GamePaused,
    GameResumed,
    
    MAX_EVENT_TYPES
};

// ============================================================================
// Typed Event Structures
// ============================================================================

// --- Input Events ---

struct InputButtonEvent
{
    EntityID sourceEntity = INVALID_ENTITY_ID;
    int deviceId = -1;
    int buttonId = -1;
    bool pressed = false;
    float timestamp = 0.0f;
};

struct InputAxisEvent
{
    EntityID sourceEntity = INVALID_ENTITY_ID;
    int deviceId = -1;
    int axisId = -1;
    float value = 0.0f;
    float timestamp = 0.0f;
};

struct InputActionEvent
{
    EntityID sourceEntity = INVALID_ENTITY_ID;
    std::string actionName;
    bool activated = false;
    float strength = 1.0f;
    float timestamp = 0.0f;
};

// --- Entity Lifecycle Events ---

struct EntityJoinedEvent
{
    EntityID entity = INVALID_ENTITY_ID;
    int playerIndex = -1;
    int controllerID = -1;
    float timestamp = 0.0f;
};

struct EntityLeftEvent
{
    EntityID entity = INVALID_ENTITY_ID;
    int playerIndex = -1;
    float timestamp = 0.0f;
};

struct EntitySpawnedEvent
{
    EntityID entity = INVALID_ENTITY_ID;
    Vector position;
    std::string entityType;
    float timestamp = 0.0f;
};

struct EntityDestroyedEvent
{
    EntityID entity = INVALID_ENTITY_ID;
    std::string reason;
    float timestamp = 0.0f;
};

// --- Gameplay Events ---

struct CollisionEvent
{
    EntityID entityA = INVALID_ENTITY_ID;
    EntityID entityB = INVALID_ENTITY_ID;
    Vector contactPoint;
    Vector normal;
    bool started = true;  // true = started, false = ended
    float timestamp = 0.0f;
};

struct TriggerEvent
{
    EntityID trigger = INVALID_ENTITY_ID;
    EntityID other = INVALID_ENTITY_ID;
    bool entered = true;  // true = entered, false = exited
    float timestamp = 0.0f;
};

struct HealthChangedEvent
{
    EntityID entity = INVALID_ENTITY_ID;
    int oldHealth = 0;
    int newHealth = 0;
    int delta = 0;
    EntityID instigator = INVALID_ENTITY_ID;
    float timestamp = 0.0f;
};

struct ItemCollectedEvent
{
    EntityID collector = INVALID_ENTITY_ID;
    EntityID item = INVALID_ENTITY_ID;
    std::string itemType;
    float timestamp = 0.0f;
};

// --- System Events ---

struct LevelEvent
{
    std::string levelName;
    bool loaded = true;  // true = loaded, false = unloaded
    float timestamp = 0.0f;
};

struct GameStateEvent
{
    bool paused = false;
    float timestamp = 0.0f;
};

// ============================================================================
// Event Union for Storage
// ============================================================================

union EventData
{
    InputButtonEvent inputButton;
    InputAxisEvent inputAxis;
    InputActionEvent inputAction;
    EntityJoinedEvent entityJoined;
    EntityLeftEvent entityLeft;
    EntitySpawnedEvent entitySpawned;
    EntityDestroyedEvent entityDestroyed;
    CollisionEvent collision;
    TriggerEvent trigger;
    HealthChangedEvent healthChanged;
    ItemCollectedEvent itemCollected;
    LevelEvent level;
    GameStateEvent gameState;
    
    EventData() { /* POD types, no initialization needed */ }
};

// ============================================================================
// Event Wrapper
// ============================================================================

struct Event
{
    ECSEventType type = ECSEventType::MAX_EVENT_TYPES;
    EventData data;
    
    Event() = default;
    
    // Helper constructors for each event type
    static Event CreateInputButton(const InputButtonEvent& evt)
    {
        Event e;
        e.type = ECSEventType::InputButton;
        e.data.inputButton = evt;
        return e;
    }
    
    static Event CreateInputAxis(const InputAxisEvent& evt)
    {
        Event e;
        e.type = ECSEventType::InputAxis;
        e.data.inputAxis = evt;
        return e;
    }
    
    static Event CreateInputAction(const InputActionEvent& evt)
    {
        Event e;
        e.type = ECSEventType::InputAction;
        e.data.inputAction = evt;
        return e;
    }
    
    static Event CreateEntityJoined(const EntityJoinedEvent& evt)
    {
        Event e;
        e.type = ECSEventType::EntityJoined;
        e.data.entityJoined = evt;
        return e;
    }
    
    static Event CreateEntityLeft(const EntityLeftEvent& evt)
    {
        Event e;
        e.type = ECSEventType::EntityLeft;
        e.data.entityLeft = evt;
        return e;
    }
    
    static Event CreateEntitySpawned(const EntitySpawnedEvent& evt)
    {
        Event e;
        e.type = ECSEventType::EntitySpawned;
        e.data.entitySpawned = evt;
        return e;
    }
    
    static Event CreateEntityDestroyed(const EntityDestroyedEvent& evt)
    {
        Event e;
        e.type = ECSEventType::EntityDestroyed;
        e.data.entityDestroyed = evt;
        return e;
    }
    
    static Event CreateCollision(const CollisionEvent& evt)
    {
        Event e;
        e.type = ECSEventType::CollisionStarted;
        e.data.collision = evt;
        return e;
    }
    
    static Event CreateTrigger(const TriggerEvent& evt)
    {
        Event e;
        e.type = ECSEventType::TriggerEntered;
        e.data.trigger = evt;
        return e;
    }
    
    static Event CreateHealthChanged(const HealthChangedEvent& evt)
    {
        Event e;
        e.type = ECSEventType::HealthChanged;
        e.data.healthChanged = evt;
        return e;
    }
    
    static Event CreateItemCollected(const ItemCollectedEvent& evt)
    {
        Event e;
        e.type = ECSEventType::ItemCollected;
        e.data.itemCollected = evt;
        return e;
    }
    
    static Event CreateLevel(const LevelEvent& evt)
    {
        Event e;
        e.type = evt.loaded ? ECSEventType::LevelLoaded : ECSEventType::LevelUnloaded;
        e.data.level = evt;
        return e;
    }
    
    static Event CreateGameState(const GameStateEvent& evt)
    {
        Event e;
        e.type = evt.paused ? ECSEventType::GamePaused : ECSEventType::GameResumed;
        e.data.gameState = evt;
        return e;
    }
};
