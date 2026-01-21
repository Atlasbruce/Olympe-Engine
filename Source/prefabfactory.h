/*
	Olympe Engine V2 - 2025
	Nicolas Chereau
	nchereau@gmail.com

	Purpose:
	- Header file for PrefabFactory class, responsible for creating game object prefabs.
	- Supports both legacy function-based prefabs and modern component-agnostic instantiation.
*/
#pragma once
#include "World.h"
#include "system/system_utils.h"
#include "ComponentDefinition.h"
#include "PrefabScanner.h"
#include <map>

using PrefabBuilder = std::function<void(EntityID)>;

class PrefabFactory
{
public:
    static PrefabFactory& Get()
    {
        static PrefabFactory instance;
        return instance;
    }
    
    // ========================================================================
    // Legacy API (for backward compatibility)
    // ========================================================================
    
    // Example: Register("MiliceGuard", [](World& w, EntityID id) { ... });
    void RegisterPrefab(const std::string& name, PrefabBuilder builder)
    {
        m_prefabs[name] = builder;
       SYSTEM_LOG << "PrefabFactory::RegisteredPrefab has registered: " << name << "\n";
    }

    // create an Entity 
    EntityID CreateEntity(const std::string& prefabName)
    {
        if (m_prefabs.find(prefabName) == m_prefabs.end())
        {
            SYSTEM_LOG << "PrefabFactory::RegisteredPrefab"  << "Error: Prefab '" << prefabName << "' unknown.\n";
            return INVALID_ENTITY_ID;
        }

        // 1. Cr�er l'ID unique via le World (UID Nanoseconde)
        World& world = World::Get();
        EntityID newEntity = world.CreateEntity();

        // 2. Appliquer la recette (ajouter les composants)
        m_prefabs[prefabName](newEntity);

        SYSTEM_LOG << "PrefabFactory::CreateEntity '" << prefabName << "' created (ID: " << newEntity << ")\n";
        return newEntity;
    }
    
    // ========================================================================
    // Modern Component-Agnostic API
    // ========================================================================
    
    // Set the prefab registry (cache for lookups)
    void SetPrefabRegistry(const PrefabRegistry& registry);
    
    // Get the cached prefab registry
    const PrefabRegistry& GetPrefabRegistry() const { return m_prefabRegistry; }
    
    // Create entity from a parsed blueprint (no re-parsing)
    EntityID CreateEntityFromBlueprint(const PrefabBlueprint& blueprint);
    
    // Instantiate a single component on an entity (component-agnostic)
    bool InstantiateComponent(EntityID entity, const ComponentDefinition& componentDef);

private:
    std::map<std::string, PrefabBuilder> m_prefabs;
    PrefabRegistry m_prefabRegistry;

    PrefabFactory() = default;
    
    // ========================================================================
    // Component-specific instantiation helpers
    // ========================================================================
    
    bool InstantiateIdentity(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePosition(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePhysicsBody(EntityID entity, const ComponentDefinition& def);
    bool InstantiateVisualSprite(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAIBehavior(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAIBlackboard(EntityID entity, const ComponentDefinition& def);
    bool InstantiateBoundingBox(EntityID entity, const ComponentDefinition& def);
    bool InstantiateMovement(EntityID entity, const ComponentDefinition& def);
    bool InstantiateHealth(EntityID entity, const ComponentDefinition& def);
    bool InstantiateTriggerZone(EntityID entity, const ComponentDefinition& def);
    bool InstantiateCollisionZone(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAnimation(EntityID entity, const ComponentDefinition& def);
    bool InstantiateFX(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAudioSource(EntityID entity, const ComponentDefinition& def);
    bool InstantiateController(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePlayerController(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePlayerBinding(EntityID entity, const ComponentDefinition& def);
    bool InstantiateNPC(EntityID entity, const ComponentDefinition& def);
    bool InstantiateInventory(EntityID entity, const ComponentDefinition& def);
    bool InstantiateCamera(EntityID entity, const ComponentDefinition& def);
    bool InstantiateCameraTarget(EntityID entity, const ComponentDefinition& def);
    bool InstantiateCameraEffects(EntityID entity, const ComponentDefinition& def);
    bool InstantiateCameraBounds(EntityID entity, const ComponentDefinition& def);
    bool InstantiateCameraInputBinding(EntityID entity, const ComponentDefinition& def);
    bool InstantiateInputMapping(EntityID entity, const ComponentDefinition& def);
};

// Macro pour faciliter l'enregistrement automatique (similaire � votre ancienne m�thode)
#define REGISTER_PREFAB(Name, BuilderLambda) \
    namespace { \
        struct AutoRegister##Name { \
            AutoRegister##Name() { \
                PrefabFactory::Get().RegisterPrefab(#Name, BuilderLambda); \
            } \
        }; \
        AutoRegister##Name global_##Name; \
    }