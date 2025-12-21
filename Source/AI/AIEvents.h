/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

AI Events purpose: Helper functions for emitting AI-related gameplay events via EventQueue.

*/

#pragma once

#include "../system/EventQueue.h"
#include "../system/message.h"
#include "../system/system_consts.h"
#include "../vector.h"
#include "../ECS_Entity.h"

namespace AIEvents
{
    // Emit an explosion event at a position
    // This will be heard by NPCs within their hearing radius
    inline void EmitExplosion(const Vector& position, float radius)
    {
        Message msg = Message::Create(
            EventType::Olympe_EventType_AI_Explosion,
            EventDomain::Gameplay
        );
        msg.param1 = position.x;
        msg.param2 = position.y;
        msg.state = static_cast<int>(radius);
        
        EventQueue::Get().Push(msg);
    }
    
    // Emit a noise/sound event at a position
    // Intensity affects how far the sound travels
    inline void EmitNoise(const Vector& position, float intensity = 1.0f)
    {
        Message msg = Message::Create(
            EventType::Olympe_EventType_AI_Noise,
            EventDomain::Gameplay
        );
        msg.param1 = position.x;
        msg.param2 = position.y;
        msg.state = static_cast<int>(intensity * 100.0f);
        
        EventQueue::Get().Push(msg);
    }
    
    // Emit a damage dealt event
    // This notifies NPCs when damage is dealt, allowing them to react
    inline void EmitDamageDealt(EntityID victim, EntityID attacker, float damage)
    {
        Message msg = Message::Create(
            EventType::EventType_Hit,
            EventDomain::Gameplay
        );
        msg.targetUid = victim;
        msg.deviceId = static_cast<int>(attacker);
        msg.param1 = damage;
        
        EventQueue::Get().Push(msg);
    }
}
