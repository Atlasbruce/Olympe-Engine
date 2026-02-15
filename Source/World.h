/**
 * @file World.h
 * @brief World and ECS Manager for Olympe Engine
 * @author Nicolas Chereau
 * @date 2025
 * @version 2.0
 * 
 * @details
 * This file contains the World class which manages the Entity-Component-System
 * architecture, level loading, and game state. The World is a singleton that
 * coordinates all game entities, components, and systems.
 * 
 * Key responsibilities:
 * - Entity lifecycle management (create, destroy, query)
 * - Component storage and retrieval
 * - System execution coordination
 * - Level loading from Tiled maps
 * - Tile rendering and tileset management
 * 
 * @note World purpose: Manage the overall game world, including object management,
 * level handling, and ECS architecture.
 */
#pragma once

#include "system/EventQueue.h"
#include "system/system_utils.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
#include <type_traits>

#include "Level.h"
#include "GameState.h"

// Include ECS related headers
#include "Ecs_Entity.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"
#include "ECS_Register.h" // Include the implementation of ComponentPool
#include "PrefabScanner.h"
#include "PrefabFactory.h"

// JSON library for tile layer loading
#include "third_party/nlohmann/json.hpp"
#include "ParameterResolver.h"

// Forward declarations for 3-Phase Level Loading
//struct PrefabRegistry;
namespace Olympe { 
    namespace Tiled { 
        struct LevelParseResult;
        struct TiledMap;
    }
    namespace Editor { 
        struct LevelDefinition;
        struct EntityInstance;
    }
}

// ========================================================================
// TILE LAYER SUPPORT
// ========================================================================

/**
 * @struct TileChunk
 * @brief Represents a chunk of tiles for rendering
 * 
 * Tile chunks are used to batch tile rendering for performance.
 * Each chunk contains a grid of tiles from a single layer.
 */
struct TileChunk
{
    std::string layerName;       ///< Name of the source layer
    int x;                       ///< Chunk X position (in tiles)
    int y;                       ///< Chunk Y position (in tiles)
    int width;                   ///< Chunk width (in tiles)
    int height;                  ///< Chunk height (in tiles)
    int zOrder;                  ///< Render order (Z-coordinate)
    std::vector<uint32_t> tileGIDs;  ///< Tile Global IDs (with flip flags)

    /** @brief Default constructor */
    TileChunk() : x(0), y(0), width(0), height(0), zOrder(0) {}
};

/**
 * @class TilesetManager
 * @brief Manages tilesets loaded from Tiled maps
 * 
 * Handles both image-based tilesets (single texture atlas) and
 * collection tilesets (individual tile images).
 * 
 * Provides tile lookup by Global ID (GID) and texture access.
 */
class TilesetManager
{
public:
    /**
     * @struct TilesetInfo
     * @brief Information about a loaded tileset
     * 
     * Stores tileset properties including dimensions, offsets, and textures.
     * Critical for proper tile rendering with correct positioning.
     */
    struct TilesetInfo
    {
        uint32_t firstgid;       ///< First Global ID in this tileset
        uint32_t lastgid;        ///< Last Global ID (firstgid + tilecount - 1)
        std::string name;        ///< Tileset name
        int tilewidth;           ///< Width of each tile
        int tileheight;          ///< Height of each tile
        int columns;             ///< Number of columns in atlas
        int imagewidth;          ///< Total atlas width
        int imageheight;         ///< Total atlas height
        int margin;              ///< Margin around atlas
        int spacing;             ///< Spacing between tiles
        bool isCollection;       ///< True if collection tileset
        
        // ====================================================================
        // CRITICAL: Global tile offset for this tileset
        // These values come from the .tsx/.tsj file's <tileoffset> element
        // ALL tiles in this tileset inherit these offset values for rendering
        // Examples from Olympe tilesets:
        // - Trees.tsj: tileoffsetX = -100, tileoffsetY = 0
        // - Tiles iso cube.tsx: tileoffsetX = 0, tileoffsetY = 26
        // - tiles-iso-1.tsx: tileoffsetX = 0, tileoffsetY = 0 (default)
        // ====================================================================
        int tileoffsetX;         ///< Global X offset for all tiles
        int tileoffsetY;         ///< Global Y offset for all tiles

        // Image-based tileset
        SDL_Texture* texture;    ///< Atlas texture (image-based)

        // Collection tileset (individual tiles)
        std::map<uint32_t, SDL_Texture*> individualTiles;      ///< Per-tile textures
        std::map<uint32_t, SDL_Rect> individualSrcRects;       ///< Per-tile source rects

        /**
         * @brief Default constructor with explicit initialization
         */
        TilesetInfo() : firstgid(0), lastgid(0), tilewidth(0), tileheight(0),
            columns(0), imagewidth(0), imageheight(0), margin(0), spacing(0),
            isCollection(false), tileoffsetX(0), tileoffsetY(0), texture(nullptr) {
        }
    };

    /**
     * @brief Clear all loaded tilesets
     */
    void Clear();
    
    /**
     * @brief Load tilesets from JSON data
     * @param tilesetsJson JSON array of tileset definitions
     */
    void LoadTilesets(const nlohmann::json& tilesetsJson);
    
    /**
     * @brief Get texture and source rect for a tile by GID
     * @param gid Global tile ID
     * @param outTexture Output texture pointer
     * @param outSrcRect Output source rectangle
     * @param outTileset Output tileset info pointer
     * @return True if tile was found
     */
    bool GetTileTexture(uint32_t gid, SDL_Texture*& outTexture, SDL_Rect& outSrcRect, const TilesetInfo*& outTileset);
    
    /**
     * @brief Get all loaded tilesets
     * @return Reference to tileset vector
     */
    const std::vector<TilesetInfo>& GetTilesets() const { return m_tilesets; }

private:
    std::vector<TilesetInfo> m_tilesets;  ///< All loaded tilesets
};

/**
 * @class World
 * @brief Core ECS manager and world coordinator
 * 
 * The World class is the central hub for the Entity-Component-System architecture.
 * It manages all entities, components, systems, and level loading.
 * 
 * Key features:
 * - Entity creation and destruction
 * - Component addition, removal, and queries
 * - System registration and execution
 * - Level loading from Tiled maps (.tmj/.tmx)
 * - Tile rendering with multiple tilesets
 * - Collision and navigation mesh management
 * 
 * @note Singleton class - use World::Get() to access
 * 
 * @example
 * @code
 * // Create entity
 * EntityID player = World::Get().CreateEntity();
 * 
 * // Add components
 * Position_data pos;
 * pos.position = Vector(100, 200, 0);
 * World::Get().AddComponent<Position_data>(player, pos);
 * 
 * // Query entities
 * auto entities = World::Get().GetEntitiesWithComponents<Position_data, Sprite_data>();
 * @endcode
 */
class World 
{
public:
    /** @brief Default constructor */
    World();
    
    /** @brief Destructor */
    virtual ~World();

    /**
     * @brief Get singleton instance
     * @return Reference to World singleton
     */
    static World& GetInstance()
    {
        static World instance;
        return instance;
    }
    
    /**
     * @brief Get singleton instance (short form)
     * @return Reference to World singleton
     */
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
    
    // NEW: Load and prepare all behavior tree dependencies for a level
    bool LoadLevelDependencies(const nlohmann::json& levelJson);
    
    // Generate collision and navigation maps from TMJ/TMX level data
    void GenerateCollisionAndNavigationMaps(const Olympe::Tiled::TiledMap& tiledMap,
                                            const Olympe::Editor::LevelDefinition& levelDef);
    
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
            int linkedObjects;

            PassStats() : totalObjects(0), successfullyCreated(0), failed(0), linkedObjects(0) {}

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
    /**
     * @brief Get all active entity IDs
     * @return Vector of all entity IDs in the world
     */
    const std::vector<EntityID>& GetAllEntities() const
    {
        return m_entities;
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

    // ========================================================================
    // Layer Management API
    // ========================================================================
    
    /// Get the default render layer for an entity type
    /// Automatically assigns appropriate layer based on entity classification
    RenderLayer GetDefaultLayerForType(EntityType type) const
    {
        switch (type)
        {
            case EntityType::Player:
            case EntityType::NPC:
                return RenderLayer::Characters;
            
            case EntityType::Enemy:
                return RenderLayer::Characters;  // Same layer as players for proper overlap
            
            case EntityType::Item:
            case EntityType::Collectible:
                return RenderLayer::Objects;
            
            case EntityType::Effect:
            case EntityType::Particle:
                return RenderLayer::Effects;
            
            case EntityType::UIElement:
                return RenderLayer::UI_Near;
            
            case EntityType::Background:
                return RenderLayer::Background_Near;
            
            case EntityType::Trigger:
            case EntityType::Waypoint:
                return RenderLayer::Ground;  // Invisible helpers at ground level
            
            case EntityType::Static:
            case EntityType::Dynamic:
            case EntityType::None:
            default:
                return RenderLayer::Ground;
        }
    }
    
    /// Set entity render layer (updates position.z)
    void SetEntityLayer(EntityID entity, RenderLayer layer);
    
    /// Calculate layer index from zOrder value (for Tiled levels)
    /// Maps zOrder ranges to layer indices for proper depth sorting
    RenderLayer CalculateLayerFromZOrder(float zOrder) const;
    
    /// Get entity render layer
    RenderLayer GetEntityLayer(EntityID entity) const;
    
    /// Get next available custom layer index (for dynamic layers)
    int GetNextCustomLayerIndex() 
    { 
        return m_nextCustomLayerIndex++; 
    }
    
    // ========================================================================
    // Grid Management
    // ========================================================================
    
    /// Toggle grid visibility
    void ToggleGrid()
    {
        for (const auto& kv : m_entitySignatures)
        {
            EntityID e = kv.first;
            if (HasComponent<GridSettings_data>(e))
            {
                GridSettings_data& settings = GetComponent<GridSettings_data>(e);
                settings.enabled = !settings.enabled;
                
                SYSTEM_LOG << "World::ToggleGrid: Grid " 
                           << (settings.enabled ? "enabled" : "disabled") << "\n";
                break;
            }
        }
    }
    
    /// Get current grid state
    bool IsGridEnabled()
    {
        for (const auto& kv : m_entitySignatures)
        {
            EntityID e = kv.first;
            if (HasComponent<GridSettings_data>(e))
            {
                const GridSettings_data& settings = GetComponent<GridSettings_data>(e);
                return settings.enabled;
            }
        }
        return false;  // Default if no GridSettings entity exists
    }
    
    /// Toggle collision overlay visibility
    void ToggleCollisionOverlay()
    {
        for (const auto& kv : m_entitySignatures)
        {
            EntityID e = kv.first;
            if (HasComponent<GridSettings_data>(e))
            {
                GridSettings_data& settings = GetComponent<GridSettings_data>(e);
                settings.showCollisionOverlay = !settings.showCollisionOverlay;
                
                SYSTEM_LOG << "World::ToggleCollisionOverlay: Collision overlay " 
                           << (settings.showCollisionOverlay ? "enabled" : "disabled") << "\n";
                break;
            }
        }
    }
    
    /// Toggle navigation overlay visibility
    void ToggleNavigationOverlay()
    {
        for (const auto& kv : m_entitySignatures)
        {
            EntityID e = kv.first;
            if (HasComponent<GridSettings_data>(e))
            {
                GridSettings_data& settings = GetComponent<GridSettings_data>(e);
                settings.showNavigationOverlay = !settings.showNavigationOverlay;
                
                SYSTEM_LOG << "World::ToggleNavigationOverlay: Navigation overlay " 
                           << (settings.showNavigationOverlay ? "enabled" : "disabled") << "\n";
                break;
            }
        }
    }
    
    /// Get collision overlay state
    bool IsCollisionOverlayVisible()
    {
        for (const auto& kv : m_entitySignatures)
        {
            EntityID e = kv.first;
            if (HasComponent<GridSettings_data>(e))
            {
                const GridSettings_data& settings = GetComponent<GridSettings_data>(e);
                return settings.showCollisionOverlay;
            }
        }
        return false;  // Default if no GridSettings entity exists
    }
    
    /// Get navigation overlay state
    bool IsNavigationOverlayVisible()
    {
        for (const auto& kv : m_entitySignatures)
        {
            EntityID e = kv.first;
            if (HasComponent<GridSettings_data>(e))
            {
                const GridSettings_data& settings = GetComponent<GridSettings_data>(e);
                return settings.showNavigationOverlay;
            }
        }
        return false;  // Default if no GridSettings entity exists
    }
    
    /// Synchronize grid settings with loaded level
    /// Extracts map orientation and tile dimensions from LevelDefinition
    /// and updates GridSettings_data (projection mode, cellSize, hexRadius)
    /// Supports: orthogonal, isometric, hexagonal orientations
    void SyncGridWithLevel(const Olympe::Editor::LevelDefinition& levelDef);

    // Public for inspection/debug
    std::unordered_map<EntityID, ComponentSignature> m_entitySignatures;

private:
    // Mapping: TypeID -> Component Pool
    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentPool>> m_componentPools;

    // Entity ID management
    EntityID m_nextEntityID = 1;
    std::queue<EntityID> m_freeEntityIDs;
    std::vector<EntityID> m_entities;

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
    // PHASE 2 & 3: Helper Methods for 6-Phase Level Loading
    // ========================================================================
    
    // Phase 2: Validate level prefabs (after normalization)
    void ValidateLevelPrefabs(const Olympe::Editor::LevelDefinition& levelDef);
    
    // Phase 4: Visual structure instantiation passes
    bool InstantiatePass1_VisualLayers(
        const Olympe::Editor::LevelDefinition& levelDef,
        InstantiationResult& result);
    
    bool InstantiatePass2_SpatialStructure(
        const Olympe::Editor::LevelDefinition& levelDef,
        InstantiationResult& result);
    
    // Phase 6: Relationship linking
    bool InstantiatePass5_Relationships(
        const Olympe::Editor::LevelDefinition& levelDef,
        InstantiationResult& result);
    
    /// Unified entity instantiation helper (used by all Phase 5 passes)
    /// Types are already normalized, performs direct lookup and instantiation
    EntityID InstantiateEntity(
        const std::shared_ptr<Olympe::Editor::EntityInstance>& entityInstance,
        PrefabFactory& factory,
        InstantiationResult::PassStats& stats);
    
    // ========================================================================
    // Player Entity Registration (for level-loaded players)
    // ========================================================================
    
    /// Register a player entity that was loaded from a level file
    /// Validates required components and delegates to VideoGame for full setup
    void RegisterPlayerEntity(EntityID entity);
    
    // ========================================================================
    // Helper Methods for Entity Instantiation
    // ========================================================================
    
    /// Extract custom properties from JSON overrides into LevelInstanceParameters
    void ExtractCustomProperties(
        const nlohmann::json& overrides, 
        LevelInstanceParameters& instanceParams,
        const Olympe::Editor::EntityInstance* entityInstance = nullptr,
        const PrefabBlueprint* prefab = nullptr);
    
    /// Create a red placeholder entity for missing prefabs
    EntityID CreateMissingPrefabPlaceholder(
        const Olympe::Editor::EntityInstance& entityInstance,
        InstantiationResult::PassStats& stats);
    
    /// Extract prefab name from prefab path (removes path and extension)
    std::string ExtractPrefabName(const std::string& prefabPath);
    

public:    
    // Get tile chunks (for rendering system)
    const std::vector<TileChunk>& GetTileChunks() const { return m_tileChunks; }
    
    // Get tileset manager (for rendering system)
    TilesetManager& GetTilesetManager() { return m_tilesetManager; }
    
    // Get map configuration for rendering
    const std::string& GetMapOrientation() const { return m_mapOrientation; }
    int GetTileWidth() const { return m_tileWidth; }
    int GetTileHeight() const { return m_tileHeight; }
    
    // Get isometric origin offset (computed from map bounds)
    // This offset is applied to tile rendering to align tiles and entities in the same world space
    float GetIsometricOriginX() const;
    float GetIsometricOriginY() const;
    
    // Set map bounds (for isometric origin calculation)
    void SetMapBounds(int minTileX, int minTileY, int maxTileX, int maxTileY, int chunkOriginX, int chunkOriginY);
    
    // Get chunk origin (for orthogonal/hex/staggered entity offset)
    int GetChunkOriginX() const { return m_chunkOriginX; }
    int GetChunkOriginY() const { return m_chunkOriginY; }


    // Tile layer loading helper methods (internal use only)
    void LoadTileLayer(const nlohmann::json& layerJson, InstantiationResult& result);
    void LoadTileChunk(const nlohmann::json& chunkJson, const std::string& layerName, 
                       int zOrder, const std::string& encoding);
    void LoadTileData(const nlohmann::json& dataJson, const std::string& layerName, 
                      int width, int height, int zOrder, const std::string& encoding);

private:
    TilesetManager m_tilesetManager;
    std::vector<TileChunk> m_tileChunks;
    std::string m_mapOrientation;  // "orthogonal" or "isometric"
    int m_tileWidth;
    int m_tileHeight;
    std::vector<std::unique_ptr<Level>> m_levels;
    
    // Map bounds (for isometric origin calculation)
    int m_minTileX = 0;
    int m_minTileY = 0;
    int m_maxTileX = 0;
    int m_maxTileY = 0;
    
    // Chunk origin (for orthogonal/hex/staggered entity offset)
    int m_chunkOriginX = 0;
    int m_chunkOriginY = 0;
    
    // Cached isometric origin (computed once, used many times during rendering)
    mutable float m_cachedIsometricOriginX = 0.0f;
    mutable float m_cachedIsometricOriginY = 0.0f;
    mutable bool m_isometricOriginCached = false;
    
    // Custom layer counter (starts after predefined layers)
    int m_nextCustomLayerIndex = static_cast<int>(RenderLayer::Foreground_Far) + 1;
};
