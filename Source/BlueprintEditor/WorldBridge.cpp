/*
 * Olympe Blueprint Editor - World Bridge
 * 
 * Provides hooks between World ECS and Blueprint Editor
 * Allows World to notify editor of entity/component changes without direct dependency
 */

#include "EntityInspectorManager.h"
#include "../ECS_Entity.h"

// Bridge functions called by World (via extern declarations)
// These avoid circular dependencies between World and BlueprintEditor

extern "C" void NotifyEditorEntityCreated(EntityID entity)
{
    if (Olympe::EntityInspectorManager::Get().IsInitialized())
    {
        Olympe::EntityInspectorManager::Get().OnEntityCreated(entity);
    }
}

extern "C" void NotifyEditorEntityDestroyed(EntityID entity)
{
    if (Olympe::EntityInspectorManager::Get().IsInitialized())
    {
        Olympe::EntityInspectorManager::Get().OnEntityDestroyed(entity);
    }
}
