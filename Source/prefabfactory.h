/**
 * @file prefabfactory.h
 * @brief Header file for PrefabFactory class, responsible for creating game object prefabs
 * @author Nicolas Chereau
 * @date 2025
 * 
 * Purpose:
 * - PrefabFactory class handles entity creation from blueprints
 * - Supports both legacy function-based prefabs and modern component-agnostic instantiation
 */
#pragma once
#include "system/system_utils.h"
#include "ComponentDefinition.h"
#include "PrefabScanner.h"
#include "ParameterResolver.h"
#include <map>
#include <memory>
#include <functional>
#include <vector>

using PrefabBuilder = std::function<void(EntityID)>;

/**
 * @brief Factory class for creating entities from prefab blueprints
 * 
 * PrefabFactory manages entity creation from JSON blueprint files.
 * It supports both modern component-agnostic instantiation and legacy function-based prefabs.
 * 
 * @see PrefabBlueprint
 * @see ComponentDefinition
 */
class PrefabFactory
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to PrefabFactory instance
     */
    static PrefabFactory& Get()
    {
        static PrefabFactory instance;
        return instance;
    }
    
    // ========================================================================
    // Component Factory Registry (Auto-Registration System)
    // ========================================================================
    
    /// Register a component factory (called by auto-registration system)
    void RegisterComponentFactory(const std::string& componentName, 
                                   std::function<bool(EntityID, const ComponentDefinition&)> factory);
    
    /// Check if a component is registered
    bool IsComponentRegistered(const std::string& componentName) const;
    
    /// Get list of all registered components (for debugging)
    std::vector<std::string> GetRegisteredComponents() const;
    
    // ========================================================================
    // NEW: Centralized Prefab Management
    // ========================================================================
    
    /**
     * @brief Preload all prefabs from directory
     * 
     * Scans and caches all prefab blueprints for fast entity creation.
     * Should be called once at engine startup (World::Initialize).
     * 
     * @param prefabDirectory Path to prefab directory (default: "Blueprints/EntityPrefab")
     */
    void PreloadAllPrefabs(const std::string& prefabDirectory = "Blueprints/EntityPrefab");
    
    /**
     * @brief Create entity from prefab name
     * 
     * Instantiates a new entity by loading and applying all components
     * defined in the prefab blueprint file.
     * 
     * @param prefabName Name of the prefab (without .json extension)
     * @return EntityID of created entity, or INVALID_ENTITY_ID on failure
     * 
     * @see PrefabBlueprint
     * @see InstantiateComponent
     */
    EntityID CreateEntityFromPrefabName(const std::string& prefabName);
    
    /// Create entity from prefab name with explicit layer override
   // UNUSED EntityID CreateEntityFromPrefabName(const std::string& prefabName, RenderLayer layer);
    
    /**
     * @brief Get prefab count
     * @return Number of loaded prefabs
     */
    int GetPrefabCount() const { return static_cast<int>(m_prefabRegistry.GetCount()); }
    
    /**
     * @brief Check if prefab exists
     * @param prefabName Name of the prefab to check
     * @return true if prefab exists in registry
     */
    bool HasPrefab(const std::string& prefabName) const 
    { 
        return m_prefabRegistry.Find(prefabName) != nullptr; 
    }
    
    // ========================================================================
    // Legacy API (for backward compatibility)
    // ========================================================================
    
    /**
     * @brief Register a legacy prefab builder function
     * @param name Prefab name
     * @param builder Function that builds the entity
     */
    void RegisterPrefab(const std::string& name, PrefabBuilder builder)
    {
        m_prefabs[name] = builder;
       SYSTEM_LOG << "PrefabFactory::RegisteredPrefab has registered: " << name << "\n";
    }

    /**
     * @brief Create an entity using legacy prefab system
     * @param prefabName Name of the prefab
     * @return EntityID of created entity
     */
    EntityID CreateEntity(const std::string& prefabName);
    
    // ========================================================================
    // Modern Component-Agnostic API
    // ========================================================================
    
    /**
     * @brief Set the prefab registry cache
     * @param registry PrefabRegistry to use for lookups
     */
    void SetPrefabRegistry(const PrefabRegistry& registry);
    
    /**
     * @brief Get the cached prefab registry
     * @return Reference to the prefab registry
     */
    const PrefabRegistry& GetPrefabRegistry() const { return m_prefabRegistry; }
    
    /**
     * @brief Normalize a type string to canonical form
     * @param type Type string to normalize
     * @return Normalized type string
     */
    std::string NormalizeType(const std::string& type) const;
    
    /**
     * @brief Check if two types are equivalent
     * @param type1 First type string
     * @param type2 Second type string
     * @return true if types are equivalent
     */
    bool AreTypesEquivalent(const std::string& type1, const std::string& type2) const;
    
    /**
     * @brief Check if a type is registered
     * @param type Type string to check (will be normalized)
     * @return true if type is registered
     */
    bool IsTypeRegistered(const std::string& type) const;
    
    /**
     * @brief Get canonical type info for debugging
     * @param type Type to query
     * @param outCanonical Output parameter for canonical type name
     * @param outPrefabFile Output parameter for prefab file path
     * @return true if type was found
     */
    bool GetCanonicalInfo(const std::string& type, std::string& outCanonical, 
                          std::string& outPrefabFile) const;
    
    /**
     * @brief Create entity from a parsed blueprint
     * 
     * @param blueprint Parsed prefab blueprint
     * @param autoAssignLayer If true, automatically assign layer based on EntityType
     * @return EntityID of created entity
     */
    EntityID CreateEntityFromBlueprint(const PrefabBlueprint& blueprint, bool autoAssignLayer = true);
    
    /**
     * @brief Create entity from blueprint with level instance parameter overrides
     * 
     * This is the unified method for both static and dynamic object creation.
     * 
     * @param blueprint Parsed prefab blueprint
     * @param instanceParams Level instance parameters to override blueprint defaults
     * @param autoAssignLayer If true, automatically assign layer based on EntityType
     * @return EntityID of created entity
     */
    EntityID CreateEntityWithOverrides(
        const PrefabBlueprint& blueprint,
        const LevelInstanceParameters& instanceParams,
        bool autoAssignLayer = true);
    
    /**
     * @brief Instantiate a single component on an entity
     * 
     * Component-agnostic instantiation method that works with any registered component type.
     * 
     * @param entity EntityID to add component to
     * @param componentDef Component definition with type and parameters
     * @return true if component was successfully instantiated
     */
    bool InstantiateComponent(EntityID entity, const ComponentDefinition& componentDef);

private:
    std::map<std::string, PrefabBuilder> m_prefabs;
    PrefabRegistry m_prefabRegistry;
    bool m_prefabsPreloaded = false;
    std::unique_ptr<PrefabScanner> m_scanner;  // For type normalization
    
    // Registry: component name -> factory function
    std::map<std::string, std::function<bool(EntityID, const ComponentDefinition&)>> m_componentFactories;

    PrefabFactory() = default;
    
    // ========================================================================
    // Component-specific instantiation helpers
    // ========================================================================
    
    bool InstantiateIdentity(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePosition(EntityID entity, const ComponentDefinition& def);
    bool InstantiatePhysicsBody(EntityID entity, const ComponentDefinition& def);
    bool InstantiateVisualSprite(EntityID entity, const ComponentDefinition& def);
    bool InstantiateVisualEditor(EntityID entity, const ComponentDefinition& def);
    bool InstantiateVisualAnimation(EntityID entity, const ComponentDefinition& def);
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
    bool InstantiateNavigationAgent(EntityID entity, const ComponentDefinition& def);
};

// Helper function for auto-registration (called by macro)
void RegisterComponentFactory_Internal(const char* componentName, 
                                       std::function<bool(EntityID, const ComponentDefinition&)> factory);

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