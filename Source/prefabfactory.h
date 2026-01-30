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
#include "ParameterResolver.h"
#include <map>
#include <memory>

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
    // NEW: Centralized Prefab Management
    // ========================================================================
    
    /// Preload all prefabs from Blueprints/EntityPrefab directory
    /// This should be called once at engine startup (World::Initialize)
    void PreloadAllPrefabs(const std::string& prefabDirectory = "Blueprints/EntityPrefab");
    
    /// Create entity from prefab name (uses cached registry)
    /// This is the new recommended API for creating entities
    EntityID CreateEntityFromPrefabName(const std::string& prefabName);
    
    /// Create entity from prefab name with explicit layer override
    EntityID CreateEntityFromPrefabName(const std::string& prefabName, RenderLayer layer);
    
    /// Get prefab count
    int GetPrefabCount() const { return static_cast<int>(m_prefabRegistry.GetCount()); }
    
    /// Check if prefab exists
    bool HasPrefab(const std::string& prefabName) const 
    { 
        return m_prefabRegistry.Find(prefabName) != nullptr; 
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
        // Try legacy system first
        if (m_prefabs.find(prefabName) != m_prefabs.end())
        {
            World& world = World::Get();
            EntityID newEntity = world.CreateEntity();
            m_prefabs[prefabName](newEntity);
            SYSTEM_LOG << "PrefabFactory::CreateEntity '" << prefabName << "' created (ID: " << newEntity << ")\n";
            return newEntity;
        }
        
        // Fallback to modern system
        return CreateEntityFromPrefabName(prefabName);
    }
    
    // ========================================================================
    // Modern Component-Agnostic API
    // ========================================================================
    
    // Set the prefab registry (cache for lookups)
    void SetPrefabRegistry(const PrefabRegistry& registry);
    
    // Get the cached prefab registry
    const PrefabRegistry& GetPrefabRegistry() const { return m_prefabRegistry; }
    
    // Normalize a type string to canonical form (delegates to scanner)
    std::string NormalizeType(const std::string& type) const;
    
    // Check if two types are equivalent (delegates to scanner)
    bool AreTypesEquivalent(const std::string& type1, const std::string& type2) const;
    
    // Check if a type is registered (after normalization)
    bool IsTypeRegistered(const std::string& type) const;
    
    // Get canonical type info (for debugging)
    bool GetCanonicalInfo(const std::string& type, std::string& outCanonical, 
                          std::string& outPrefabFile) const;
    
    // Create entity from a parsed blueprint (no re-parsing)
    // autoAssignLayer: if true, automatically assign layer based on EntityType
    EntityID CreateEntityFromBlueprint(const PrefabBlueprint& blueprint, bool autoAssignLayer = true);
    
    // Create entity from blueprint with level instance parameter overrides
    // This is the unified method for both static and dynamic object creation
    EntityID CreateEntityWithOverrides(
        const PrefabBlueprint& blueprint,
        const LevelInstanceParameters& instanceParams,
        bool autoAssignLayer = true);
    
    // Instantiate a single component on an entity (component-agnostic)
    bool InstantiateComponent(EntityID entity, const ComponentDefinition& componentDef);

private:
    std::map<std::string, PrefabBuilder> m_prefabs;
    PrefabRegistry m_prefabRegistry;
    bool m_prefabsPreloaded = false;
    std::unique_ptr<PrefabScanner> m_scanner;  // For type normalization

    PrefabFactory() = default;
    
    // ========================================================================
    // Component-specific instantiation helpers
    // ========================================================================
    
    bool InstantiateIdentity(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePosition(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePhysicsBody(EntityID entity, const ComponentDefinition& def);
    bool InstantiateVisualSprite(EntityID entity, const ComponentDefinition& def);
    bool InstantiateVisualEditor(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAIBehavior(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAIBlackboard(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAISenses(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAIState(EntityID entity, const ComponentDefinition& def);
    bool InstantiateBehaviorTreeRuntime(EntityID entity, const ComponentDefinition& def);
    bool InstantiateMoveIntent(EntityID entity, const ComponentDefinition& def);
    bool InstantiateAttackIntent(EntityID entity, const ComponentDefinition& def);
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