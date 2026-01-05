/*
 * Olympe Blueprint Editor - World Bridge
 * 
 * Provides hooks between World ECS and Blueprint Editor
 * Allows World to notify editor of entity/component changes without direct dependency
 */

#include "WorldBridge.h"
#include "EntityInspectorManager.h"
#include "../ECS_Entity.h"
#include <cstdint>  // Pour uint64_t

// Bridge functions called by World (via extern declarations)
// These avoid circular dependencies between World and BlueprintEditor

extern "C" void NotifyEditorEntityCreated(uint64_t entity)
{
    if (Olympe::EntityInspectorManager::Get().IsInitialized())
    {
        Olympe::EntityInspectorManager::Get().OnEntityCreated(static_cast<EntityID>(entity));
    }
}

extern "C" void NotifyEditorEntityDestroyed(uint64_t entity)
{
    if (Olympe::EntityInspectorManager::Get().IsInitialized())
    {
        Olympe::EntityInspectorManager::Get().OnEntityDestroyed(static_cast<EntityID>(entity));
    }
}
