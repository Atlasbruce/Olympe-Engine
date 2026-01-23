/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

World purpose: Manage the overall game world, including object management, level handling, and ECS architecture.

*/
#pragma once

#include "system/EventQueue.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
//#include <array>
//#include <algorithm>
#include <type_traits>

#include "Level.h" // add Level management
#include "GameState.h"

// Include ECS related headers
#include "Ecs_Entity.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"
#include "ECS_Register.h" // Include the implementation of ComponentPool
#include "PrefabScanner.h"

// JSON library for tile layer loading
#include "third_party/nlohmann/json.hpp"

// Forward declarations for 3-Phase Level Loading
//struct PrefabRegistry;
namespace Olympe { 
    namespace Tiled { struct LevelParseResult; }
    namespace Editor { struct LevelDefinition; }
}

class World 
{
public:
    World();
    virtual ~World();

    static World& GetInstance()
    {
        static World instance;
        return instance;
    }
    static World& Get() { return GetInstance(); }

    //---------------------------------------------------------------
	// ECS Systems
	void Initialize_ECS_Systems();
    void Add_ECS_System(std::unique_ptr<ECS_System> system);
    void Process_ECS_Systems();
    void Render_ECS_Systems();
    void RenderDebug_ECS_Systems();
    
    // Get a specific system by type
    template <typename T>
    T* GetSystem()
    {
        for (auto& system : m_systems)
        {
            T* castedSystem = dynamic_cast<T*>(system.get());
            if (castedSystem)
                return castedSystem;
        }
        return nullptr;
    }

    //---------------------------------------------------------------
    // Main processing loop called each frame: events are processed first (async), then stages in order
    void Process()
    {
        //0) Swap EventQueue buffers to make previous frame's events readable
        // This is the single point per frame where write buffer becomes read buffer
        EventQueue::Get().BeginFrame();

        // check global game state
        GameState state = GameStateManager::GetState();
        bool paused = (state == GameState::GameState_Paused);

		// ECS Processing Systems
        Process_ECS_Systems();
    }
    //---------------------------------------------------------------------------------------------
    void Render()
    {
  		// ECS Rendering Systems
        Render_ECS_Systems();
        RenderDebug_ECS_Systems();
	}
	//---------------------------------------------------------------------------------------------
    // Level management
    void AddLevel(std::unique_ptr<Level> level)
    {
        if (level) m_levels.push_back(std::move(level));
    }

    const std::vector<std::unique_ptr<Level>>& GetLevels() const { return m_levels; }
    
    // Tiled MapEditor integration
    bool LoadLevelFromTiled(const std::string& tiledMapPath);
    void UnloadCurrentLevel();
    
    // ========================================================================
    // PHASE 2 & 3: Advanced Level Loading Structures
    // ========================================================================
    
    struct Phase2Result
    {
        struct PreloadStats
        {
            int spritesPreloaded;
            int audioPreloaded;
            int prefabsLoaded;
            
            PreloadStats() : spritesPreloaded(0), audioPreloaded(0), prefabsLoaded(0) {}
        };
        
        PrefabRegistry prefabRegistry;
        DataManager::LevelPreloadResult preloadResult;
        PreloadStats stats;
        std::vector<std::string> missingPrefabs;
        std::vector<std::string> errors;
        bool success;
        
        Phase2Result() : success(false) {}
    };
    
    struct InstantiationResult
    {
        struct PassStats
        {
            int totalObjects;
            int successfullyCreated;
            int failed;
            std::vector<std::string> failedObjects;
            
            PassStats() : totalObjects(0), successfullyCreated(0), failed(0) {}
            
            bool IsSuccess() const { return failed == 0; }
        };
        
        bool success;
        PassStats pass1_visualLayers;
        PassStats pass2_spatialStructure;
        PassStats pass3_staticObjects;
        PassStats pass4_dynamicObjects;
        PassStats pass5_relationships;
        
        std::map<std::string, EntityID> entityRegistry;      // name -> entity ID
        std::map<int, EntityID> objectIdToEntity;            // Tiled object ID -> entity ID
        std::vector<EntityID> sectors;
        
        InstantiationResult() : success(false) {}
        
        int GetTotalCreated() const
        {
            return pass1_visualLayers.successfullyCreated + 
                   pass2_spatialStructure.successfullyCreated +
                   pass3_staticObjects.successfullyCreated +
                   pass4_dynamicObjects.successfullyCreated +
                   pass5_relationships.successfullyCreated;
        }
        
        int GetTotalFailed() const
        {
            return pass1_visualLayers.failed + pass2_spatialStructure.failed +
                   pass3_staticObjects.failed + pass4_dynamicObjects.failed +
                   pass5_relationships.failed;
        }
        
        bool IsComplete() const
        {
            return success && GetTotalFailed() == 0;
        }
    };
    
    // -------------------------------------------------------------
    // ECS Entity Management
    EntityID CreateEntity();
    void DestroyEntity(EntityID entity);
    bool IsEntityValid(EntityID entity) const
    {
        return m_entitySignatures.find(entity) != m_entitySignatures.end();
	}
    // -------------------------------------------------------------
    // Component Management (Pool Facade)

    // Add Component: takes type (T) and constructor arguments
    template <typename T, typename... Args>
    T& AddComponent(EntityID entity, Args&&... args)
    {
        const ComponentTypeID typeID = GetComponentTypeID_Static<T>();

        // 1. Instantiate the pool if it's the first time we add this type
        if (m_componentPools.find(typeID) == m_componentPools.end())
        {
            m_componentPools[typeID] = std::make_unique<ComponentPool<T>>();
        }

        // 2. Get the pool and add the component
        auto* pool = static_cast<ComponentPool<T>*>(m_componentPools[typeID].get());

        // Creation and adding of the component using perfect forwarding
        pool->AddComponent(entity, std::forward<Args>(args)...);

        // 3. Update the Entity's Signature
        m_entitySignatures[entity].set(typeID, true);

        // 4. Notify Systems about the signature change
        Notify_ECS_Systems(entity, m_entitySignatures[entity]);

        // 5. Special handling: Register input entities with InputsManager
        HandleSpecialComponentRegistration<T>(entity);

        return pool->GetComponent(entity);
    }

    template <typename T>
    void RemoveComponent(EntityID entity)
    {
        const ComponentTypeID typeID = GetComponentTypeID_Static<T>();
        if (m_componentPools.find(typeID) == m_componentPools.end()) return;

        // 1. Remove from the pool
        m_componentPools[typeID]->RemoveComponent(entity);

        // 2. Update the Entity's Signature
        m_entitySignatures[entity].set(typeID, false);

        // 3. Notify Systems
        Notify_ECS_Systems(entity, m_entitySignatures[entity]);
    }

    template <typename T>
    T& GetComponent(EntityID entity)
    {
        const ComponentTypeID typeID = GetComponentTypeID_Static<T>();
        if (m_componentPools.find(typeID) == m_componentPools.end())
        {
            throw std::runtime_error("Component pool not registered.");
        }

        auto* pool = static_cast<ComponentPool<T>*>(m_componentPools[typeID].get());
        return pool->GetComponent(entity);
    }

    template <typename T>
    bool HasComponent(EntityID entity) const
    {
        const ComponentTypeID typeID = GetComponentTypeID_Static<T>();

        // Fast check using the signature
        if (m_entitySignatures.count(entity) && m_entitySignatures.at(entity).test(typeID))
        {
            // Delegate the final check to the specific Pool
            if (m_componentPools.count(typeID)) {
                auto* pool = static_cast<ComponentPool<T>*>(m_componentPools.at(typeID).get());
                return pool->HasComponent(entity);
            }
        }
        return false;
    }

    // Public for inspection/debug
    std::unordered_map<EntityID, ComponentSignature> m_entitySignatures;

private:
    // Mapping: TypeID -> Component Pool
    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentPool>> m_componentPools;

    // Entity ID management
    EntityID m_nextEntityID = 1;
    std::queue<EntityID> m_freeEntityIDs;

    // System management
    std::vector<std::unique_ptr<ECS_System>> m_systems;

    // Notifies systems when an Entity's signature changes
    void Notify_ECS_Systems(EntityID entity, ComponentSignature signature);

    // Helper functions for SFINAE-based special component registration (C++14 compatible)
	// Helper function for SFINAE-based special component registration
    template <typename T>
    void HandleSpecialComponentRegistration(EntityID entity, typename std::enable_if<std::is_same<T, PlayerBinding_data>::value>::type* = nullptr)
    {
        extern void RegisterInputEntityWithManager(EntityID e);
        RegisterInputEntityWithManager(entity);
    }

    template <typename T>
    void HandleSpecialComponentRegistration(EntityID entity, typename std::enable_if<!std::is_same<T, PlayerBinding_data>::value>::type* = nullptr)
    {
		// Do nothing for other types
    }
    
    // Blueprint Editor notification hooks
    void NotifyBlueprintEditorEntityCreated(EntityID entity);
    void NotifyBlueprintEditorEntityDestroyed(EntityID entity);

    // ========================================================================
    // PHASE 2 & 3: Helper Methods for 3-Phase Level Loading
    // ========================================================================
    
    // Phase 2: Prefab discovery and resource preloading
    Phase2Result ExecutePhase2_PrefabDiscovery(const Olympe::Tiled::LevelParseResult& phase1Result);
    
    // Phase 3: Instantiation passes (using existing TiledToOlympe for now)
    bool InstantiatePass1_VisualLayers(
        const Olympe::Editor::LevelDefinition& levelDef,
        InstantiationResult& result);
    
    bool InstantiatePass2_SpatialStructure(
        const Olympe::Editor::LevelDefinition& levelDef,
        InstantiationResult& result);
    
    bool InstantiatePass3_StaticObjects(
        const Olympe::Editor::LevelDefinition& levelDef,
        InstantiationResult& result);
    
    bool InstantiatePass4_DynamicObjects(
        const Olympe::Editor::LevelDefinition& levelDef,
        const Phase2Result& phase2Result,
        InstantiationResult& result);
    
    bool InstantiatePass5_Relationships(
        const Olympe::Editor::LevelDefinition& levelDef,
        InstantiationResult& result);
    
    // ========================================================================
    // TILE LAYER SUPPORT
    // ========================================================================
    
    // Tile chunk structure for rendering
    struct TileChunk
    {
        std::string layerName;
        int x;              // Chunk X position (in tiles)
        int y;              // Chunk Y position (in tiles)
        int width;          // Chunk width (in tiles)
        int height;         // Chunk height (in tiles)
        int zOrder;         // Render order
        std::vector<uint32_t> tileGIDs;  // Tile Global IDs (with flip flags)
        
        TileChunk() : x(0), y(0), width(0), height(0), zOrder(0) {}
    };
    
    // Get tile chunks (for rendering system)
    const std::vector<TileChunk>& GetTileChunks() const { return m_tileChunks; }

private:
    // Tile layer loading helper methods (internal use only)
    void LoadTileLayer(const nlohmann::json& layerJson, InstantiationResult& result);
    void LoadTileChunk(const nlohmann::json& chunkJson, const std::string& layerName, 
                       int zOrder, const std::string& encoding);
    void LoadTileData(const nlohmann::json& dataJson, const std::string& layerName, 
                      int width, int height, int zOrder, const std::string& encoding);
    
    std::vector<TileChunk> m_tileChunks;
    std::vector<std::unique_ptr<Level>> m_levels;
};
