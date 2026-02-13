/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

World purpose: Manage the lifecycle of Entities and their interaction with ECS Systems.

*/
#include "World.h"
#include "CollisionMap.h"
#include "VideoGame.h"
#include "InputsManager.h"
#include "system/ViewportManager.h"
#include "OlympeTilemapEditor/include/LevelManager.h"
#include "ECS_Systems_AI.h"
#include "BlueprintEditor/WorldBridge.h"
#include "OlympeTilemapEditor/include/LevelManager.h"
#include "TiledLevelLoader/include/ParallaxLayerManager.h"
#include "TiledLevelLoader/include/LevelParser.h"
#include "TiledLevelLoader/include/TiledToOlympe.h"
#include "TiledLevelLoader/include/TiledDecoder.h"
#include "TiledLevelLoader/include/TilesetCache.h"
#include "TiledLevelLoader/include/TilesetParser.h"
#include "TiledLevelLoader/include/TiledStructures.h"
#include "TiledLevelLoader/include/IsometricProjection.h"
#include "PrefabScanner.h"
#include "prefabfactory.h"
#include "ParameterResolver.h"
#include "ParameterSchema.h"
#include "ComponentDefinition.h"
#include "DataManager.h"
#include "GameEngine.h"
#include "../SDL/include/SDL3_image/SDL_image.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <set>
#include "TiledLevelLoader/include/tiled_constants.h"

//---------------------------------------------------------------------------------------------
// Helper function to register input entities with InputsManager
void RegisterInputEntityWithManager(EntityID e)
{
    InputsManager::Get().RegisterInputEntity(e);
}
//---------------------------------------------------------------------------------------------
World::World()
{
    Initialize_ECS_Systems();

    m_mapOrientation = "orthogonal"; 
    m_tileWidth = 32;
    m_tileHeight = 32;

    // Auto-create singleton GridSettings entity if missing
    bool hasGridSettings = false;
    for (const auto& kv : m_entitySignatures)
    {
        EntityID e = kv.first;
        if (HasComponent<GridSettings_data>(e))
        {
            hasGridSettings = true;
            break;
        }
    }
    if (!hasGridSettings)
    {
        EntityID e = CreateEntity();
        AddComponent<GridSettings_data>(e); // enabled=true by default
    }

    SYSTEM_LOG << "World Initialized\n";
}
//---------------------------------------------------------------------------------------------
World::~World()
{
    SYSTEM_LOG << "World Destroyed\n";
}
//---------------------------------------------------------------------------------------------
void World::Initialize_ECS_Systems()
{
	// Initialization of ECS Systems
	// WARNING THE ORDER OF SYSTEMS MATTERS!

    /*
	Order of processing systems:
	- InputEventConsumeSystem (consumes Input domain events from EventQueue, updates Controller_data)
	- GameEventConsumeSystem (consumes Gameplay domain events, handles game state and player add/remove)
	- UIEventConsumeSystem (consumes UI domain events, handles menu activation)
	- CameraEventConsumeSystem (consumes Camera domain events, forwards to CameraSystem)
	- InputSystem
	- InputMappingSystem (maps hardware input to gameplay actions)
	- PlayerControlSystem
	- AIStimuliSystem (consumes Gameplay/Detection/Collision events, writes to blackboard)
	- AIPerceptionSystem (timesliced perception updates)
	- AIStateTransitionSystem (HFSM state transitions)
	- BehaviorTreeSystem (tick behavior trees, write intents)
	- AIMotionSystem (convert intents to Movement_data)
	- AISystem (legacy)
	- DetectionSystem
	- PhysicsSystem
	- CollisionSystem
	- TriggerSystem
	- MovementSystem
	- AudioSystem
	- CameraSystem (manages ECS cameras)

    - RenderingSystem
    */
	
	// -> NOUVEAU : Précharger tous les prefabs AVANT de créer les systèmes
	SYSTEM_LOG << "\n";
	PrefabFactory::Get().PreloadAllPrefabs("Gamedata/EntityPrefab");
	SYSTEM_LOG << "\n";
	
	Add_ECS_System(std::make_unique<InputEventConsumeSystem>());
	Add_ECS_System(std::make_unique<GameEventConsumeSystem>());
	Add_ECS_System(std::make_unique<UIEventConsumeSystem>());
	Add_ECS_System(std::make_unique<CameraEventConsumeSystem>());
	Add_ECS_System(std::make_unique<InputSystem>());
	Add_ECS_System(std::make_unique<InputMappingSystem>());
	Add_ECS_System(std::make_unique<PlayerControlSystem>());
	
	// AI Systems (ECS-friendly NPC AI architecture)
	Add_ECS_System(std::make_unique<AIStimuliSystem>());
	Add_ECS_System(std::make_unique<AIPerceptionSystem>());
	Add_ECS_System(std::make_unique<AIStateTransitionSystem>());
	Add_ECS_System(std::make_unique<BehaviorTreeSystem>());
	Add_ECS_System(std::make_unique<AIMotionSystem>());
	
	// Navigation System (processes NavigationAgent_data)
	Add_ECS_System(std::make_unique<NavigationSystem>());
	
    Add_ECS_System(std::make_unique<AISystem>());
    Add_ECS_System(std::make_unique<DetectionSystem>());
    Add_ECS_System(std::make_unique<PhysicsSystem>());
    Add_ECS_System(std::make_unique<CollisionSystem>());
	Add_ECS_System(std::make_unique<TriggerSystem>());
	Add_ECS_System(std::make_unique<MovementSystem>());
    
    // Camera System (manages ECS cameras - added before rendering)
    Add_ECS_System(std::make_unique<CameraSystem>());
    
    // Rendering systems (order matters - defines render pass sequence!)
    Add_ECS_System(std::make_unique<RenderingSystem>());        // Pass 1: World (parallax, tiles, entities)
	Add_ECS_System(std::make_unique<GridSystem>());             // Grid overlay
    Add_ECS_System(std::make_unique<UIRenderingSystem>());      // -> Pass 2: UI/HUD/Menu (ALWAYS on top)

}
//---------------------------------------------------------------------------------------------
void World::Add_ECS_System(std::unique_ptr<ECS_System> system)
{
    // Enregistrement d'un syst�me
    m_systems.push_back(std::move(system));
}
//---------------------------------------------------------------------------------------------
void World::Process_ECS_Systems()
{
	// update all registered systems in order
    for (const auto& system : m_systems)
    {
        system->Process();
    }
}
//---------------------------------------------------------------------------------------------
void World::Render_ECS_Systems()
{
	// Render all registered systems in order
    for (const auto& system : m_systems)
    {
        system->Render();

    }
    ViewportManager::Get().Render();
}
//---------------------------------------------------------------------------------------------
void World::RenderDebug_ECS_Systems()
{

    // RenderDebug all registered systems in order
    for (const auto& system : m_systems)
    {
        system->RenderDebug();
    }
}
//---------------------------------------------------------------------------------------------
void World::Notify_ECS_Systems(EntityID entity, ComponentSignature signature)
{
    // V�rifie si l'Entit� correspond maintenant aux exigences d'un Syst�me
    for (const auto& system : m_systems)
    {
        // Utilisation de l'op�ration de bits AND pour la comparaison (tr�s rapide)
        if ((signature & system->requiredSignature) == system->requiredSignature)
        {
            // L'Entit� correspond : l'ajouter au Syst�me
            system->AddEntity(entity);
        }
        else
        {
            // L'Entit� ne correspond plus : la retirer du Syst�me
            system->RemoveEntity(entity);
        }
    }
}
//---------------------------------------------------------------------------------------------
EntityID World::CreateEntity()
{
    // --- UID Generation based on nanosecond timestamp ---
    using namespace std::chrono;

    // 1. Generate a unique ID (UID) based on current time in nanoseconds
    // This provides a globally unique ID suitable for serialization and persistence.
    auto now = system_clock::now();
    EntityID newID = static_cast<std::uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());

    // Check for potential collision (extremely rare but possible if two entities are created
    // in the exact same nanosecond or during deserialization without proper synchronization)
    while (m_entitySignatures.count(newID))
    {
        // If collision occurs (two entities created in the same nanosecond), increment the ID.
        // This is a simple conflict resolution mechanism.
        newID++;
    }
    // Initialize the Entity's signature as empty
    m_entitySignatures[newID] = ComponentSignature{};
    
    // Notify Blueprint Editor (if active)
    NotifyBlueprintEditorEntityCreated(newID);
    
    return newID;
}
//---------------------------------------------------------------------------------------------
void World::DestroyEntity(EntityID entity)
{
    if (entity == INVALID_ENTITY_ID || m_entitySignatures.find(entity) == m_entitySignatures.end())
    {
        return;
    }

    // Notify Blueprint Editor BEFORE destruction (if active)
    NotifyBlueprintEditorEntityDestroyed(entity);

    // 1. Supprimer les composants de tous les Pools o� l'Entit� existe
    ComponentSignature signature = m_entitySignatures[entity];
    for (const auto& pair : m_componentPools)
    {
        ComponentTypeID typeID = pair.first;
        if (signature.test(typeID))
        {
            // Utilise la m�thode virtuelle RemoveComponent (Phase 1.2)
            pair.second->RemoveComponent(entity);
        }
    }

    // 2. Notifier les syst�mes (pour la retirer de leurs listes)
    Notify_ECS_Systems(entity, ComponentSignature{}); // Signature vide pour forcer la suppression

    // 3. Nettoyer les maps
    m_entitySignatures.erase(entity);

    // 4. Recycler l'ID (gestion de l'information)
    m_freeEntityIDs.push(entity);
    std::cout << "Entit� " << entity << " d�truite et ID recycl�.\n";
}
//---------------------------------------------------------------------------------------------
// Layer Management Implementation
//---------------------------------------------------------------------------------------------
void World::SetEntityLayer(EntityID entity, RenderLayer layer)
{
    if (!HasComponent<Position_data>(entity))
    {
        SYSTEM_LOG << "/!\\  World::SetEntityLayer: Entity " << entity 
                   << " has no Position_data component\n";
        return;
    }
    
    Position_data& pos = GetComponent<Position_data>(entity);
    pos.position.z = LayerToZ(layer);
    
    SYSTEM_LOG << "World::SetEntityLayer: Entity " << entity 
               << " assigned to layer " << static_cast<int>(layer) 
               << " (z=" << pos.position.z << ")\n";
}

RenderLayer World::CalculateLayerFromZOrder(float zOrder) const
{
    // Map zOrder values to appropriate RenderLayer enum
    // This mapping is designed for Tiled levels where zOrder represents layer depth
    // Note: Tiled typically uses sequential integers (0, 1, 2, ...) for layer ordering
    
    // Direct mapping if zOrder matches RenderLayer values
    // Background layers: negative values
    if (zOrder <= -2.0f) return RenderLayer::Background_Far;
    if (zOrder <= -1.0f) return RenderLayer::Background_Near;
    
    // Main game layers: positive values
    if (zOrder <= 0.5f) return RenderLayer::Ground;
    if (zOrder <= 1.5f) return RenderLayer::Objects;
    if (zOrder <= 2.5f) return RenderLayer::Characters;
    if (zOrder <= 3.5f) return RenderLayer::Flying;
    if (zOrder <= 4.5f) return RenderLayer::Effects;
    if (zOrder <= 7.0f) return RenderLayer::UI_Near;
    if (zOrder <= 15.0f) return RenderLayer::Foreground_Near;
    
    // Very high z values
    return RenderLayer::Foreground_Far;
}

//---------------------------------------------------------------------------------------------
RenderLayer World::GetEntityLayer(EntityID entity) const
{
    if (!HasComponent<Position_data>(entity))
    {
        return RenderLayer::Ground;
    }

    const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
    return ZToLayer(pos.position.z);
}
//---------------------------------------------------------------------------------------------
// Blueprint Editor notification hooks
//---------------------------------------------------------------------------------------------
void World::NotifyBlueprintEditorEntityCreated(EntityID entity)
{
    // Only notify if BlueprintEditor is active (avoid linking issues when editor is not included)
    #ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    NotifyEditorEntityCreated((uint64_t)entity);
    #endif
}
//---------------------------------------------------------------------------------------------
void World::NotifyBlueprintEditorEntityDestroyed(EntityID entity)
{
    #ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    NotifyEditorEntityDestroyed((uint64_t)entity);
    #endif
}
//---------------------------------------------------------------------------------------------
// Grid Management
//---------------------------------------------------------------------------------------------
void World::SyncGridWithLevel(const Olympe::Editor::LevelDefinition& levelDef)
{
    // Find GridSettings entity
    bool foundGridSettings = false;
    for (const auto& kv : m_entitySignatures)
    {
        EntityID e = kv.first;
        if (HasComponent<GridSettings_data>(e))
        {
            foundGridSettings = true;
            GridSettings_data& settings = GetComponent<GridSettings_data>(e);
            
            // Extract orientation from mapConfig (primary source)
            std::string orientation = levelDef.mapConfig.orientation;
            if (orientation.empty())
            {
                orientation = "orthogonal";  // Default fallback
                SYSTEM_LOG << "World::SyncGridWithLevel: Warning - No orientation specified, using orthogonal\n";
            }
            
            // Extract tile dimensions
            int tileWidth = levelDef.mapConfig.tileWidth > 0 ? levelDef.mapConfig.tileWidth : 32;
            int tileHeight = levelDef.mapConfig.tileHeight > 0 ? levelDef.mapConfig.tileHeight : 32;
            
            // Update projection mode
            if (orientation == "orthogonal")
            {
                settings.projection = GridProjection::Ortho;
                settings.cellSize = Vector(static_cast<float>(tileWidth), 
                                          static_cast<float>(tileHeight), 0.f);
            }
            else if (orientation == "isometric")
            {
                settings.projection = GridProjection::Iso;
                settings.cellSize = Vector(static_cast<float>(tileWidth), 
                                          static_cast<float>(tileHeight), 0.f);
            }
            else if (orientation == "hexagonal")
            {
                settings.projection = GridProjection::HexAxial;
                // Note: This assumes "pointy-top" hexagonal orientation
                // Radius is half the tile width for pointy-top hexagons
                settings.hexRadius = static_cast<float>(tileWidth) / 2.0f;
            }
            else if (orientation == "staggered")
            {
                // Staggered orientation is not fully supported yet
                // Fall back to orthogonal with a warning
                SYSTEM_LOG << "World::SyncGridWithLevel: Warning - Staggered orientation not fully supported, using orthogonal\n";
                settings.projection = GridProjection::Ortho;
                settings.cellSize = Vector(static_cast<float>(tileWidth), 
                                          static_cast<float>(tileHeight), 0.f);
            }
            else
            {
                // Unknown orientation - fallback to orthogonal
                SYSTEM_LOG << "World::SyncGridWithLevel: Warning - Unknown orientation '" 
                           << orientation << "', using orthogonal\n";
                settings.projection = GridProjection::Ortho;
                settings.cellSize = Vector(static_cast<float>(tileWidth), 
                                          static_cast<float>(tileHeight), 0.f);
            }
            
            SYSTEM_LOG << "World::SyncGridWithLevel: Grid synced with level\n"
                       << "  Orientation: " << orientation << "\n"
                       << "  Tile size: " << tileWidth << "x" << tileHeight << "\n";
            
            break;
        }
    }
    
    if (!foundGridSettings)
    {
        SYSTEM_LOG << "World::SyncGridWithLevel: Warning - No GridSettings entity found, grid sync skipped\n";
    }
}
//---------------------------------------------------------------------------------------------
// Generate collision and navigation maps from TMJ/TMX level data
//---------------------------------------------------------------------------------------------

// Helper function to check if a layer is a collision layer
namespace {
	bool IsCollisionLayer(const std::shared_ptr<Olympe::Tiled::TiledLayer>& layer)
	{
		if (!layer) return false;
		
		// Check by name (case-insensitive)
		std::string layerNameLower = layer->name;
		std::transform(layerNameLower.begin(), layerNameLower.end(), 
		              layerNameLower.begin(), 
		              [](unsigned char c) { return std::tolower(c); });
		
		if (layerNameLower.find("collision") != std::string::npos ||
		    layerNameLower.find("walls") != std::string::npos)
		{
			return true;
		}
		
		// Check by property
		for (std::map<std::string, Olympe::Tiled::TiledProperty>::const_iterator propIt = layer->properties.begin();
		     propIt != layer->properties.end(); ++propIt)
		{
			if (propIt->second.name == "collision" && propIt->second.boolValue == true)
			{
				return true;
			}
		}
		
		return false;
	}
}

void World::GenerateCollisionAndNavigationMaps(const Olympe::Tiled::TiledMap& tiledMap,
                                                const Olympe::Editor::LevelDefinition& levelDef)
{
	// Constants for isometric tile offset heuristic
	// These bounds allow ±10% deviation from theoretical 2:1 ratio (i.e., 1.8 to 2.2)
	// to accommodate asset variations in isometric tilesets (e.g., 58x27 vs exact 54x27)
	constexpr float ISO_ASPECT_RATIO_MIN = 1.8f;  // Minimum aspect ratio for standard 2:1 isometric
	constexpr float ISO_ASPECT_RATIO_MAX = 2.2f;  // Maximum aspect ratio for standard 2:1 isometric
	constexpr float STANDARD_ISO_OFFSET_RATIO = 0.5f;  // Standard isometric vertical offset (tileHeight/2)
	
	SYSTEM_LOG << "\n";
	SYSTEM_LOG << "+==========================================================+\n";
	SYSTEM_LOG << "| COLLISION & NAVIGATION MAP GENERATION                    |\n";
	SYSTEM_LOG << "+==========================================================+\n";
	
	// Debug TMJ structure
	SYSTEM_LOG << "  [DEBUG] TMJ Structure Analysis:\n";
	SYSTEM_LOG << "    TMJ width=" << tiledMap.width << ", height=" << tiledMap.height << "\n";
	SYSTEM_LOG << "    TMJ tilewidth=" << tiledMap.tilewidth << ", tileheight=" << tiledMap.tileheight << "\n";
	SYSTEM_LOG << "    TMJ infinite=" << (tiledMap.infinite ? "YES" : "NO") << "\n";
	SYSTEM_LOG << "    TMJ orientation=" << tiledMap.orientation << "\n";
	SYSTEM_LOG << "    LevelDef mapWidth=" << levelDef.mapConfig.mapWidth 
	           << ", mapHeight=" << levelDef.mapConfig.mapHeight << "\n";
	SYSTEM_LOG << "    LevelDef tileWidth=" << levelDef.mapConfig.tileWidth 
	           << ", tileHeight=" << levelDef.mapConfig.tileHeight << "\n";
	
	// Scan layers to find actual tile grid dimensions
	int mapWidth = 0;
	int mapHeight = 0;
	
	SYSTEM_LOG << "  [DEBUG] Scanning tile layers for dimensions:\n";
	
	for (size_t layerIdx = 0; layerIdx < tiledMap.layers.size(); ++layerIdx)
	{
		const std::shared_ptr<Olympe::Tiled::TiledLayer>& layer = tiledMap.layers[layerIdx];
		if (layer && layer->type == Olympe::Tiled::LayerType::TileLayer)
		{
			SYSTEM_LOG << "    Layer '" << layer->name << "': "
			           << layer->width << "x" << layer->height 
			           << " (data size: " << layer->data.size() << ")\n";
			
			mapWidth = std::max(mapWidth, layer->width);
			mapHeight = std::max(mapHeight, layer->height);
		}
	}
	
	// Fallback if no layers found
	if (mapWidth == 0 || mapHeight == 0)
	{
		// Try using tilewidth/tileheight as dimensions (some TMJ files encode it this way)
		mapWidth = levelDef.mapConfig.tileWidth;
		mapHeight = levelDef.mapConfig.tileHeight;
		
		SYSTEM_LOG << "  WARNING: No tile layers found, using tileWidth/tileHeight as fallback\n";
	}
	
	SYSTEM_LOG << "  TMJ declared dimensions (metadata): " 
	           << levelDef.mapConfig.mapWidth << "x" << levelDef.mapConfig.mapHeight << "\n";
	SYSTEM_LOG << "  Actual tile grid dimensions (calculated): " 
	           << mapWidth << "x" << mapHeight << "\n";
	
	if (mapWidth == 0 || mapHeight == 0)
	{
		SYSTEM_LOG << "  X Invalid map dimensions, skipping collision/navigation generation\n\n";
		return;
	}
	
	// Determine projection type
	std::string orientation = levelDef.mapConfig.orientation;
	GridProjectionType projection = GridProjectionType::Ortho;
	if (orientation == "isometric")
		projection = GridProjectionType::Iso;
	else if (orientation == "hexagonal")
		projection = GridProjectionType::HexAxial;
	
	// Extract tile pixel dimensions from TMJ
	// The TMJ file's tilewidth/tileheight fields contain the actual pixel dimensions
	// These are already correctly stored in levelDef.mapConfig by TiledToOlympe::ExtractMapConfiguration()
	float tilePixelWidth = static_cast<float>(tiledMap.tilewidth);
	float tilePixelHeight = static_cast<float>(tiledMap.tileheight);
	
	// Validate tile pixel dimensions (reasonable range: 4-1024 pixels)
	if (tilePixelWidth <= 0.0f || tilePixelHeight <= 0.0f ||
	    tilePixelWidth > 1024.0f || tilePixelHeight > 1024.0f)
	{
		SYSTEM_LOG << "  X Invalid or unreasonable tile pixel dimensions from TMJ (" 
		           << tilePixelWidth << "x" << tilePixelHeight << "), using defaults\n";
		tilePixelWidth = 32.0f;
		tilePixelHeight = 32.0f;
	}
	
	SYSTEM_LOG << "  Tile pixel size (from TMJ): " << tilePixelWidth << "x" << tilePixelHeight << " px\n";
	SYSTEM_LOG << "  Projection: " << orientation << " (type=" << static_cast<int>(projection) << ")\n";
	
	// Extract tileset offset for isometric alignment
	float tileOffsetX = 0.0f;
	float tileOffsetY = 0.0f;
	
	if (projection == GridProjectionType::Iso && !tiledMap.tilesets.empty())
	{
		// Get offset from the first loaded tileset
		// NOTE: We use only the first tileset's offset as collision/navigation overlays
		// are unified across the entire map. If different layers use different tilesets
		// with different offsets, they should share the same base tile dimensions for
		// consistent collision detection. Multi-tileset maps with varying offsets are
		// not currently supported for overlay alignment.
		const Olympe::Tiled::TiledTileset& firstTileset = tiledMap.tilesets[0];
		tileOffsetX = static_cast<float>(firstTileset.tileoffsetX);
		tileOffsetY = static_cast<float>(firstTileset.tileoffsetY);
		
		if (tileOffsetX != 0.0f || tileOffsetY != 0.0f)
		{
			SYSTEM_LOG << "  -> Found tileset offset from '" << firstTileset.name << "': (" 
			           << tileOffsetX << ", " << tileOffsetY << ")\n";
		}
		else
		{
			// Fallback heuristic for standard isometric tiles without explicit offset
			// Most isometric tilesets use tileHeight/2 as vertical offset
			// Note: Division is safe - tilePixelHeight is validated in the dimension check
			// earlier in this function and set to 32.0f if invalid or zero
			float aspectRatio = tilePixelWidth / tilePixelHeight;
			
			if (aspectRatio >= ISO_ASPECT_RATIO_MIN && aspectRatio <= ISO_ASPECT_RATIO_MAX)
			{
				// Looks like standard 2:1 isometric
				tileOffsetY = tilePixelHeight * STANDARD_ISO_OFFSET_RATIO;
				SYSTEM_LOG << "  -> Applying standard isometric offset heuristic: (0, " 
				           << tileOffsetY << ")\n";
			}
		}
	}
	
	// Initialize collision map (single layer for now, can be extended later)
	CollisionMap& collMap = CollisionMap::Get();
	collMap.Initialize(mapWidth, mapHeight, projection, tilePixelWidth, tilePixelHeight, 1,
	                   tileOffsetX, tileOffsetY);
	
	// Initialize navigation map
	NavigationMap& navMap = NavigationMap::Get();
	navMap.Initialize(mapWidth, mapHeight, projection, tilePixelWidth, tilePixelHeight, 1);
	
	SYSTEM_LOG << "  -> CollisionMap initialized: " << mapWidth << "x" << mapHeight 
	           << " (" << (mapWidth * mapHeight) << " tiles)\n";
	SYSTEM_LOG << "  -> NavigationMap initialized: " << mapWidth << "x" << mapHeight << "\n";
	
	// ========================================================================
	// PHASE 1: Build navigation map from tile layers with custom properties
	// ========================================================================
	
	SYSTEM_LOG << "  [1/2] Processing tile layers for navigation...\n";
	
	int totalNavigableTiles = 0;
	int totalBlockedTiles = 0;
	int totalBorderTiles = 0;
	int layersProcessed = 0;
	int layersSkipped = 0;
	
	// 8-directional neighbor offsets (constant for all layers and projections)
	const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
	const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
	
	for (size_t layerIdx = 0; layerIdx < tiledMap.layers.size(); ++layerIdx)
	{
		const std::shared_ptr<Olympe::Tiled::TiledLayer>& layer = tiledMap.layers[layerIdx];
		if (!layer) continue;

		// Skip non-tile layers
		if (layer->type != Olympe::Tiled::LayerType::TileLayer) continue;

		// Parse layer navigation properties
		Olympe::Tiled::LayerProperties props = Olympe::Tiled::ParseLayerProperties(layer->properties);
		
		// Skip layers without navigation properties (graphic-only layers)
		if (!props.hasNavigationProperties)
		{
			SYSTEM_LOG << "    Skipping graphic-only layer: " << layer->name << "\n";
			layersSkipped++;
			continue;
		}

		SYSTEM_LOG << "    Layer '" << layer->name << "' navigation properties:\n";
		SYSTEM_LOG << "      - isTilesetWalkable: " << (props.isTilesetWalkable ? "true" : "false") << "\n";
		SYSTEM_LOG << "      - useTilesetBorder: " << (props.useTilesetBorder ? "true" : "false") << "\n";

		// Use layer dimensions, clamped to map bounds
		int layerW = std::min(layer->width, mapWidth);
		int layerH = std::min(layer->height, mapHeight);

		// Process tile data for this layer (data is 1D array: index = y * width + x)
		int navigableCount = 0;
		int blockedCount = 0;
		int borderCount = 0;
		
		// First pass: Mark non-empty tiles based on isTilesetWalkable
		int index = 0;
		for (int y = 0; y < layerH; ++y)
		{
			for (int x = 0; x < layerW; ++x)
			{
				if (index >= static_cast<int>(layer->data.size()))
				{
					SYSTEM_LOG << "    WARNING: Layer data truncated at index " << index 
					           << " (expected " << (layerW * layerH) << ")\n";
					break;
				}

				uint32_t gid = layer->data[index];
				uint32_t tileId = gid & ~TILE_FLIP_FLAGS_MASK; // Remove flip flags

				if (tileId > 0) // Non-empty tile
				{
					// Get current tile properties (may have been set by previous layers)
					TileProperties tileProps = collMap.GetTileProperties(x, y);
					
					if (props.isTilesetWalkable)
					{
						// Walkable layer: mark non-empty tiles as navigable (unless already blocked)
						if (!tileProps.isBlocked)
						{
							tileProps.isNavigable = true;
							tileProps.isBlocked = false;
							tileProps.traversalCost = 1.0f;
							collMap.SetTileProperties(x, y, tileProps);
							navigableCount++;
						}
					}
					else
					{
						// Collision layer: mark non-empty tiles as blocked (override previous settings)
						tileProps.isBlocked = true;
						tileProps.isNavigable = false;
						tileProps.traversalCost = 999.0f;
						collMap.SetTileProperties(x, y, tileProps);
						blockedCount++;
					}
				}
				
				++index;
			}
		}
		
		// Second pass: Mark borders if useTilesetBorder is true
		if (props.useTilesetBorder)
		{
			index = 0;
			for (int y = 0; y < layerH; ++y)
			{
				for (int x = 0; x < layerW; ++x)
				{
					if (index >= static_cast<int>(layer->data.size())) break;

					uint32_t gid = layer->data[index];
					uint32_t tileId = gid & ~TILE_FLIP_FLAGS_MASK;

					// Check if this is an empty tile
					if (tileId == 0)
					{
						// Check all 8 directions for non-empty tiles
						bool hasNonEmptyNeighbor = false;
						
						for (int dir = 0; dir < 8; ++dir)
						{
							int nx = x + dx[dir];
							int ny = y + dy[dir];
							
							// Check bounds
							if (nx < 0 || nx >= layerW || ny < 0 || ny >= layerH)
								continue;
							
							// Check if neighbor has a tile in this layer
							int neighborIndex = ny * layerW + nx;
							if (neighborIndex >= 0 && neighborIndex < static_cast<int>(layer->data.size()))
							{
								uint32_t neighborGid = layer->data[neighborIndex];
								uint32_t neighborTileId = neighborGid & ~TILE_FLIP_FLAGS_MASK;
								
								if (neighborTileId > 0)
								{
									hasNonEmptyNeighbor = true;
									break;
								}
							}
						}
						
						// If this empty tile is adjacent to a non-empty tile, mark it as a border
						if (hasNonEmptyNeighbor)
						{
							TileProperties tileProps = collMap.GetTileProperties(x, y);
							tileProps.isBlocked = true;
							tileProps.isNavigable = false;
							tileProps.traversalCost = 999.0f;
							collMap.SetTileProperties(x, y, tileProps);
							borderCount++;
						}
					}
					
					++index;
				}
			}
		}
		
		SYSTEM_LOG << "        -> Non-empty tiles: " << navigableCount << " navigable, " 
		           << blockedCount << " blocked\n";
		if (props.useTilesetBorder)
		{
			SYSTEM_LOG << "        -> Border tiles: " << borderCount << " marked as blocked\n";
		}
		
		totalNavigableTiles += navigableCount;
		totalBlockedTiles += blockedCount;
		totalBorderTiles += borderCount;
		layersProcessed++;
	}

	SYSTEM_LOG << "    -> Summary: " << layersProcessed << " layers processed, " 
	           << layersSkipped << " skipped (graphic only)\n";
	SYSTEM_LOG << "    -> Total navigable tiles: " << totalNavigableTiles << "\n";
	SYSTEM_LOG << "    -> Total blocked tiles: " << (totalBlockedTiles + totalBorderTiles) 
	           << " (" << totalBlockedTiles << " from obstacles + " 
	           << totalBorderTiles << " from borders)\n";

	
	// ========================================================================
	// PHASE 2: Process explicit collision object layers (optional)
	// ========================================================================
	
	SYSTEM_LOG << "  [2/2] Processing explicit collision object layers (if any)...\n";
	
	int objectCollisionTiles = 0;
	
	// B) Collision object layers (optional, legacy support)
	for (size_t layerIdx = 0; layerIdx < tiledMap.layers.size(); ++layerIdx)
	{
		const std::shared_ptr<Olympe::Tiled::TiledLayer>& layer = tiledMap.layers[layerIdx];
		if (!layer) continue;

		// Only process object group layers
		if (layer->type != Olympe::Tiled::LayerType::ObjectGroup) continue;

		// Check if this object group is marked as a collision layer (legacy pattern matching)
		if (!IsCollisionLayer(layer)) continue;

		SYSTEM_LOG << "    Processing collision objects from layer: " << layer->name << "\n";

		// Process objects in this group
		int objectsProcessed = 0;
		for (size_t objIdx = 0; objIdx < layer->objects.size(); ++objIdx)
		{
			const Olympe::Tiled::TiledObject& obj = layer->objects[objIdx];

			// Convert object bounds to grid tiles
			int gridX, gridY;
			collMap.WorldToGrid(obj.x, obj.y, gridX, gridY);
			
			int gridW = static_cast<int>(std::ceil(obj.width / tilePixelWidth));
			int gridH = static_cast<int>(std::ceil(obj.height / tilePixelHeight));

			// Mark tiles within object bounds as blocked
			for (int dy = 0; dy < gridH; ++dy)
			{
				for (int dx = 0; dx < gridW; ++dx)
				{
					int cx = gridX + dx;
					int cy = gridY + dy;
					
					if (collMap.IsValidGridPosition(cx, cy))
					{
						TileProperties objProps = collMap.GetTileProperties(cx, cy);
						objProps.isBlocked = true;
						objProps.isNavigable = false;
						objProps.traversalCost = 999.0f;
						collMap.SetTileProperties(cx, cy, objProps);
						objectCollisionTiles++;
					}
				}
			}
			objectsProcessed++;
		}
		
		SYSTEM_LOG << "      -> Processed " << objectsProcessed << " collision objects\n";
	}
	
	if (objectCollisionTiles > 0)
	{
		SYSTEM_LOG << "    -> Object collision tiles: " << objectCollisionTiles << "\n";
	}
	
	// ========================================================================
	// Summary
	// ========================================================================
	
	SYSTEM_LOG << "\n";
	SYSTEM_LOG << "  -> Collision & Navigation maps ready\n";
	SYSTEM_LOG << "     Grid dimensions: " << mapWidth << "x" << mapHeight 
	           << " (" << (mapWidth * mapHeight) << " total tiles)\n";
	SYSTEM_LOG << "     Navigable tiles: " << totalNavigableTiles << "\n";
	SYSTEM_LOG << "     Blocked tiles: " << (totalBlockedTiles + totalBorderTiles + objectCollisionTiles)
	           << " (obstacles: " << totalBlockedTiles 
	           << ", borders: " << totalBorderTiles
	           << ", objects: " << objectCollisionTiles << ")\n";
	
	// DEBUG: Verify that tiles can be read back correctly
	SYSTEM_LOG << "\n";
	SYSTEM_LOG << "  DEBUG: Verifying tile data can be read back...\n";
	constexpr int VERIFY_SCAN_SIZE = 10; // Sample grid size for verification
	int verifyNavigable = 0;
	int verifyBlocked = 0;
	for (int y = 0; y < std::min(VERIFY_SCAN_SIZE, mapHeight); ++y)
	{
		for (int x = 0; x < std::min(VERIFY_SCAN_SIZE, mapWidth); ++x)
		{
			const TileProperties& tile = collMap.GetTileProperties(x, y);
			if (tile.isNavigable && !tile.isBlocked)
			{
				verifyNavigable++;
				if (verifyNavigable <= 3)
				{
					SYSTEM_LOG << "    -> Sample navigable tile at (" << x << "," << y << ")\n";
				}
			}
			else if (tile.isBlocked)
			{
				verifyBlocked++;
				if (verifyBlocked <= 3)
				{
					SYSTEM_LOG << "    -> Sample blocked tile at (" << x << "," << y << ")\n";
				}
			}
		}
	}
	SYSTEM_LOG << "    -> In first " << VERIFY_SCAN_SIZE << "x" << VERIFY_SCAN_SIZE 
	           << " grid: " << verifyNavigable << " navigable, " 
	           << verifyBlocked << " blocked\n";
	
	SYSTEM_LOG << "+==========================================================+\n";
	SYSTEM_LOG << "\n";
}
//---------------------------------------------------------------------------------------------
// Tiled MapEditor Integration
//---------------------------------------------------------------------------------------------
#include "TiledLevelLoader/include/TiledLevelLoader.h"
#include "TiledLevelLoader/include/TiledToOlympe.h"
#include "OlympeTilemapEditor/include/LevelManager.h"
#include "prefabfactory.h"
#include "ECS_Components_AI.h"
#include "AI/BehaviorTreeDependencyScanner.h"
#include "AI/BehaviorTree.h"
#include <fstream>

bool World::LoadLevelDependencies(const nlohmann::json& levelJson)
{
    std::cout << "\n+===========================================================+\n";
    std::cout << "| LEVEL DEPENDENCY LOADING                                  |\n";
    std::cout << "+===========================================================+\n";
    
    // Step 1: Extract all prefab types used in the level
    std::cout << "Step 1/3: Extracting prefab types from level...\n";
    std::set<std::string> prefabNames = BehaviorTreeDependencyScanner::ExtractPrefabsFromLevel(levelJson);
    
    if (prefabNames.empty())
    {
        std::cout << "  -> No prefabs found in level (this may be normal for tile-only levels)\n";
        std::cout << "+===========================================================+\n\n";
        return true;
    }
    
    std::cout << "  -> Found " << prefabNames.size() << " unique prefab type(s):\n";
    for (const auto& name : prefabNames)
    {
        std::cout << "     - " << name << "\n";
    }
    
    // Step 2: Scan prefabs for behavior tree dependencies
    std::cout << "\nStep 2/3: Scanning prefabs for behavior tree dependencies...\n";
    std::vector<std::string> prefabList(prefabNames.begin(), prefabNames.end());
    auto btDeps = BehaviorTreeDependencyScanner::ScanPrefabs(prefabList);
    
    if (btDeps.empty())
    {
        std::cout << "  -> No behavior trees required for this level\n";
        std::cout << "+===========================================================+\n\n";
        return true;
    }
    
    // Step 3: Load all required behavior trees
    std::cout << "\nStep 3/3: Loading required behavior trees...\n";
    int loaded = 0;
    int alreadyLoaded = 0;
    int failed = 0;
    
    for (const auto& dep : btDeps)
    {
        // Check if already loaded
        if (BehaviorTreeManager::Get().IsTreeLoadedByPath(dep.treePath))
        {
            std::cout << "  [CACHED] " << dep.treePath << " (ID=" << dep.suggestedTreeId << ")\n";
            alreadyLoaded++;
            continue;
        }
        
        // Load the tree
        std::cout << "  [LOADING] " << dep.treePath << " (ID=" << dep.suggestedTreeId << ")... ";
        
        if (BehaviorTreeManager::Get().LoadTreeFromFile(dep.treePath, dep.suggestedTreeId))
        {
            std::cout << "SUCCESS\n";
            loaded++;
        }
        else
        {
            std::cout << "FAILED\n";
            std::cerr << "  [ERROR] Failed to load behavior tree: " << dep.treePath << "\n";
            failed++;
        }
    }
    
    // Summary
    std::cout << "\n+===========================================================+\n";
    std::cout << "| DEPENDENCY LOADING SUMMARY                                |\n";
    std::cout << "+===========================================================+\n";
    std::cout << "| Behavior Trees Required:   " << btDeps.size() << "\n";
    std::cout << "| Loaded This Session:       " << loaded << "\n";
    std::cout << "| Already Cached:            " << alreadyLoaded << "\n";
    std::cout << "| Failed:                    " << failed << "\n";
    std::cout << "+===========================================================+\n\n";
    
    return (failed == 0);
}

bool World::LoadLevelFromTiled(const std::string& tiledMapPath)
{
    // Unload current level
    UnloadCurrentLevel();

    std::cout << "\n";
    std::cout << "+==========================================================+\n";
    std::cout << "| LEVEL LOADING PIPELINE (6 PHASES)                        |\n";
    std::cout << "+==========================================================+\n\n";
    
    // =======================================================================
    // PHASE 1: PREFAB SYSTEM INITIALIZATION (Already Done at Startup)
    // =======================================================================
    
    PrefabFactory& factory = PrefabFactory::Get();
    SYSTEM_LOG << "Phase 1: -> Prefab System Ready (" << factory.GetPrefabCount() << " prefabs)\n\n";
    
    // =======================================================================
    // PHASE 2: LEVEL PARSING (JSON -> Memory, Normalize Immediately)
    // =======================================================================
    
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "| PHASE 2: LEVEL PARSING                                   |\n";
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "File: " << tiledMapPath << "\n\n";
    
    // Load TMJ (ONCE)
    Olympe::Tiled::TiledMap tiledMap;
    Olympe::Tiled::TiledLevelLoader loader;
    
    if (!loader.LoadFromFile(tiledMapPath, tiledMap))
    {
        SYSTEM_LOG << "  X Failed to load TMJ file\n";
        return false;
    }
    
    // Convert to LevelDefinition (includes immediate normalization)
    Olympe::Tiled::TiledToOlympe converter;
    Olympe::Editor::LevelDefinition levelDef;
    if (!converter.Convert(tiledMap, levelDef))
    {
        SYSTEM_LOG << "  X Failed to convert map\n";
        return false;
    }
    
    SYSTEM_LOG << "  -> Parsed " << levelDef.entities.size() << " entities (types normalized)\n";
    SYSTEM_LOG << "  -> Static objects: " << levelDef.categorizedObjects.staticObjects.size() << "\n";
    SYSTEM_LOG << "  -> Dynamic objects: " << levelDef.categorizedObjects.dynamicObjects.size() << "\n";
    SYSTEM_LOG << "  -> Patrol paths: " << levelDef.categorizedObjects.patrolPaths.size() << "\n\n";
    
    // Generate collision and navigation maps
    SYSTEM_LOG << "[Phase 5/6] Generating Collision & Navigation Maps...\n";
    GenerateCollisionAndNavigationMaps(tiledMap, levelDef);
    
    // Synchronize grid settings with loaded level
    SyncGridWithLevel(levelDef);
    
    // Validate prefabs (after normalization)
    ValidateLevelPrefabs(levelDef);
    
    // =======================================================================
    // PHASE 2.5: BEHAVIOR TREE DEPENDENCY LOADING (NEW!)
    // =======================================================================
    
    // Note: We reload the JSON here to access the raw layer data for dependency scanning.
    // This is intentional - the TiledMap structure is already converted to LevelDefinition,
    // and creating a new API to extract the raw JSON would require larger refactoring.
    // The performance impact is negligible for typical level sizes.
    nlohmann::json levelJsonRaw;
    std::ifstream jsonFile(tiledMapPath);
    if (jsonFile.is_open())
    {
        try 
        {
            jsonFile >> levelJsonRaw;
            jsonFile.close();
            
            if (!LoadLevelDependencies(levelJsonRaw))
            {
                std::cerr << "[World] ERROR: Failed to load level dependencies\n";
                return false;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[World] WARNING: Failed to parse level JSON: " << e.what() << "\n";
            // Continue anyway - not all levels may have behavior trees
        }
    }
    else
    {
        std::cerr << "[World] WARNING: Could not open level JSON file for dependency scanning\n";
        // Continue anyway - not all levels may have behavior trees
    }
    
    // =======================================================================
    // PHASE 3: RESOURCE PRELOADING (Centralized)
    // =======================================================================
    
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "| PHASE 3: RESOURCE PRELOADING                             |\n";
    SYSTEM_LOG << "+==========================================================+\n\n";
    
    int resourcesLoaded = 0;
    int resourcesFailed = 0;
    
    DataManager& dataManager = DataManager::Get();
    
    // Step 1: Tilesets
    SYSTEM_LOG << "  Step 1/4: Loading tilesets...\n";
    if (!levelDef.resources.tilesetPaths.empty())
    {
        auto tilesetResult = dataManager.PreloadTextures(
            levelDef.resources.tilesetPaths, 
            ResourceCategory::Level, 
            true);
        resourcesLoaded += tilesetResult.successfullyLoaded;
        resourcesFailed += tilesetResult.completelyFailed;
        SYSTEM_LOG << "    -> Loaded " << tilesetResult.successfullyLoaded << " tilesets\n";
    }
    
    // Step 2: Parallax layers
    SYSTEM_LOG << "  Step 2/4: Loading parallax layers...\n";
    std::vector<std::string> parallaxPaths;
    if (levelDef.metadata.customData.contains("parallaxLayers") && levelDef.metadata.customData["parallaxLayers"].is_array())
    {
        const auto& parallaxLayersJson = levelDef.metadata.customData["parallaxLayers"];
        for (const auto& layerJson : parallaxLayersJson)
        {
            std::string imagePath = layerJson.value("imagePath", "");
            if (!imagePath.empty())
            {
                parallaxPaths.push_back(imagePath);
            }
        }
    }
    if (!parallaxPaths.empty())
    {
        auto parallaxResult = dataManager.PreloadTextures(parallaxPaths, ResourceCategory::Level, true);
        resourcesLoaded += parallaxResult.successfullyLoaded;
        resourcesFailed += parallaxResult.completelyFailed;
        SYSTEM_LOG << "    -> Loaded " << parallaxResult.successfullyLoaded << " parallax layers\n";
    }
    
    // Step 3: Prefab sprites
    SYSTEM_LOG << "  Step 3/4: Loading prefab sprites...\n";
    std::vector<std::string> spritePaths;
    std::set<std::string> uniqueTypes;
    for (const auto& entity : levelDef.entities)
    {
        if (entity)
        {
            uniqueTypes.insert(entity->type);
        }
    }
    
    for (const auto& type : uniqueTypes)
    {
        const PrefabBlueprint* blueprint = factory.GetPrefabRegistry().Find(type);
        if (blueprint)
        {
            for (const auto& sprite : blueprint->resources.spriteRefs)
            {
                spritePaths.push_back(sprite);
            }
        }
    }
    
    if (!spritePaths.empty())
    {
        auto spriteResult = dataManager.PreloadSprites(spritePaths, ResourceCategory::GameEntity, true);
        resourcesLoaded += spriteResult.successfullyLoaded;
        resourcesFailed += spriteResult.completelyFailed;
        SYSTEM_LOG << "    -> Loaded " << spriteResult.successfullyLoaded << " sprites\n";
    }
    
    // Step 4: Audio
    SYSTEM_LOG << "  Step 4/4: Loading audio files...\n";
    if (!levelDef.resources.audioPaths.empty())
    {
        auto audioResult = dataManager.PreloadAudioFiles(levelDef.resources.audioPaths, true);
        resourcesLoaded += audioResult.successfullyLoaded;
        resourcesFailed += audioResult.completelyFailed;
        SYSTEM_LOG << "    -> Loaded " << audioResult.successfullyLoaded << " audio files\n";
    }
    
    SYSTEM_LOG << "\n  -> Total resources loaded: " << resourcesLoaded;
    if (resourcesFailed > 0)
    {
        SYSTEM_LOG << " (" << resourcesFailed << " failed)";
    }
    SYSTEM_LOG << "\n\n";
    
    // =======================================================================
    // PHASE 4: VISUAL STRUCTURE CREATION
    // =======================================================================
    
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "| PHASE 4: VISUAL STRUCTURE CREATION                       |\n";
    SYSTEM_LOG << "+==========================================================+\n\n";
    
    InstantiationResult instResult;
    
    // Create parallax layers and tiles
    SYSTEM_LOG << "  Pass 1/2: Parallax & Visual Layers...\n";
    InstantiatePass1_VisualLayers(levelDef, instResult);
    
    SYSTEM_LOG << "  Pass 2/2: Spatial Structures...\n";
    InstantiatePass2_SpatialStructure(levelDef, instResult);
    
    SYSTEM_LOG << "  -> Created " << m_tileChunks.size() << " tile chunks\n\n";
    
    // =======================================================================
    // PHASE 5: ENTITY INSTANTIATION (Unified)
    // =======================================================================
    
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "| PHASE 5: ENTITY INSTANTIATION                            |\n";
    SYSTEM_LOG << "+==========================================================+\n\n";
    
    // Pass 1: Static objects
    SYSTEM_LOG << "  Pass 1/3: Static objects...\n";
    for (auto& entityInstancePtr : levelDef.categorizedObjects.staticObjects)
    {
        std::shared_ptr<Olympe::Editor::EntityInstance> entityInstance = std::move(entityInstancePtr);
        EntityID entity = InstantiateEntity(entityInstance, factory, instResult.pass3_staticObjects);
        if (entity != INVALID_ENTITY_ID && entityInstance)
        {
            instResult.entityRegistry[entityInstance->name] = entity;
        }
    }
    SYSTEM_LOG << "    -> Created " << instResult.pass3_staticObjects.successfullyCreated << " objects\n\n";
    
    // Pass 2: Dynamic objects
    SYSTEM_LOG << "  Pass 2/3: Dynamic objects...\n";
    for (auto& entityInstancePtr : levelDef.categorizedObjects.dynamicObjects)
    {
        std::shared_ptr<Olympe::Editor::EntityInstance> entityInstance = std::move(entityInstancePtr);
        EntityID entity = InstantiateEntity(entityInstance, factory, instResult.pass4_dynamicObjects);
        
        if (entity != INVALID_ENTITY_ID && entityInstance)
        {
            instResult.entityRegistry[entityInstance->name] = entity;
            // Note: Post-processing (player registration, AI init) is now handled in InstantiateEntity
        }
    }
    SYSTEM_LOG << "    -> Created " << instResult.pass4_dynamicObjects.successfullyCreated << " objects\n\n";
    
    // Pass 3: Spatial structures (patrol paths, waypoints)
    SYSTEM_LOG << "  Pass 3/3: Spatial structures (patrol paths)...\n";
    for (auto& entityInstancePtr : levelDef.categorizedObjects.patrolPaths)
    {
        std::shared_ptr<Olympe::Editor::EntityInstance> entityInstance = std::move(entityInstancePtr);
        EntityID entity = InstantiateEntity(entityInstance, factory, instResult.pass5_relationships);
        if (entity != INVALID_ENTITY_ID && entityInstance)
        {
            instResult.entityRegistry[entityInstance->name] = entity;
        }
    }
    SYSTEM_LOG << "    -> Created " << instResult.pass5_relationships.successfullyCreated << " objects\n\n";
    
    // =======================================================================
    // PHASE 6: ENTITY RELATIONSHIPS
    // =======================================================================
    
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "| PHASE 6: ENTITY RELATIONSHIPS                            |\n";
    SYSTEM_LOG << "+==========================================================+\n\n";
    
    InstantiatePass5_Relationships(levelDef, instResult);
    
    SYSTEM_LOG << "  -> Linked " << instResult.pass5_relationships.linkedObjects << " relationships\n\n";
    
    instResult.success = true;
    
    // =======================================================================
    // FINAL SUMMARY
    // =======================================================================
    
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "| LEVEL LOADING COMPLETE                                   |\n";
    SYSTEM_LOG << "+==========================================================+\n";
    SYSTEM_LOG << "| Entities Created:    " << std::setw(3) << instResult.GetTotalCreated()
               << std::string(31, ' ') << "|\n";
    SYSTEM_LOG << "| Entities Failed:     " << std::setw(3) << instResult.GetTotalFailed()
               << std::string(31, ' ') << "|\n";
    SYSTEM_LOG << "| Resources Loaded:    " << std::setw(3) << resourcesLoaded
               << std::string(31, ' ') << "|\n";
    SYSTEM_LOG << "| Resources Failed:    " << std::setw(3) << resourcesFailed
               << std::string(31, ' ') << "|\n";
    SYSTEM_LOG << "+==========================================================+\n\n";
    
    SYSTEM_LOG << "World::LoadLevelFromTiled - Level loaded successfully\n";
    return true;
}

void World::UnloadCurrentLevel()
{
    SYSTEM_LOG << "World::UnloadCurrentLevel - Unloading current level\n";
    
    // Clear tile chunks and tilesets
    m_tileChunks.clear();
    m_tilesetManager.Clear();
    
    // Destroy all entities except system entities (like GridSettings)
    std::vector<EntityID> entitiesToDestroy;
    
    for (const auto& kv : m_entitySignatures)
    {
        EntityID e = kv.first;
        
        // Keep system entities (GridSettings, Camera settings, etc.)
        if (HasComponent<GridSettings_data>(e))
        {
            continue; // Keep grid settings
        }
        
        // Keep camera entities if they exist as system entities
        if (HasComponent<Camera_data>(e))
        {
            // Check if it's a player camera or system camera
            // For now, we'll keep all cameras
            continue;
        }

		//keep persistent entities
		if (HasComponent<Identity_data>(e))
		{
			Identity_data& id = GetComponent<Identity_data>(e);
			if (id.isPersistent)
			{
				SYSTEM_LOG << "World::UnloadCurrentLevel - Keeping persistent entity: " << e << " (" << id.name << ")\n";
				continue; // Keep persistent entities
			}
		}
        
        // Mark all other entities for destruction
        entitiesToDestroy.push_back(e);
    }
    
    // Destroy marked entities
    for (EntityID e : entitiesToDestroy)
    {
        DestroyEntity(e);
    }
    
    SYSTEM_LOG << "World::UnloadCurrentLevel - Destroyed " << entitiesToDestroy.size() 
               << " entities\n";
}

//=============================================================================
// Validation & Unified Entity Instantiation Helpers
//=============================================================================

void World::ValidateLevelPrefabs(const Olympe::Editor::LevelDefinition& levelDef)
{
    PrefabFactory& factory = PrefabFactory::Get();
    std::set<std::string> missingTypes;
    
    for (const auto& entity : levelDef.entities)
    {
        if (!entity) continue;
        
        const PrefabBlueprint* blueprint = factory.GetPrefabRegistry().Find(entity->type);
        if (!blueprint)
        {
            missingTypes.insert(entity->type);
        }
    }
    
    if (!missingTypes.empty())
    {
        SYSTEM_LOG << "  /!\\ Warning: " << missingTypes.size() << " missing prefabs:\n";
        for (const auto& type : missingTypes)
        {
            SYSTEM_LOG << "    - " << type << "\n";
        }
    }
}

EntityID World::InstantiateEntity(
    const std::shared_ptr<Olympe::Editor::EntityInstance>& entityInstance,
    PrefabFactory& factory,
    InstantiationResult::PassStats& stats)
{
    if (!entityInstance)
    {
        stats.failed++;
        return INVALID_ENTITY_ID;
    }
    
    stats.totalObjects++;
    
    // -> Type ALREADY normalized - direct lookup (no matching logic needed)
    const PrefabBlueprint* blueprint = factory.GetPrefabRegistry().Find(entityInstance->type);
    
    if (!blueprint || !blueprint->isValid)
    {
        SYSTEM_LOG << "    X No prefab for type '" << entityInstance->type << "'\n";
        EntityID placeholder = CreateMissingPrefabPlaceholder(*entityInstance, stats);
        return placeholder;
    }
    
    // Check input device availability for Player prefabs before creation
    if (blueprint->HasCategory("RequiresRegistration"))
    {
        if (!VideoGame::Get().IsInputDeviceAvailable())
        {
            stats.failed++;
            stats.failedObjects.push_back(entityInstance->name);
            SYSTEM_LOG << "    X Cannot create player '" << entityInstance->name 
                       << "': no input device available (all joysticks and keyboard already assigned)\n";
            return INVALID_ENTITY_ID;
        }
    }
    
    // Build instance parameters
    LevelInstanceParameters instanceParams(entityInstance->name, entityInstance->type);
    instanceParams.position = entityInstance->position;
    ExtractCustomProperties(entityInstance->overrides, instanceParams, entityInstance.get(), blueprint);
    
    // Create entity with overrides
    // CRITICAL: Disable autoAssignLayer to preserve position.z from Tiled level
    // The zOrder from the Tiled level is already stored in position.z and should not be overridden
    EntityID entity = factory.CreateEntityWithOverrides(*blueprint, instanceParams, false);
    
    if (entity == INVALID_ENTITY_ID)
    {
        stats.failed++;
        stats.failedObjects.push_back(entityInstance->name);
        SYSTEM_LOG << "    X Failed: " << entityInstance->name << "\n";
        return INVALID_ENTITY_ID;
    }
    
    // Update identity
    if (HasComponent<Identity_data>(entity))
    {
        Identity_data& id = GetComponent<Identity_data>(entity);
        id.name = entityInstance->name;
    }
    
    // Category-based post-processing
    if (blueprint->HasCategory("RequiresRegistration"))
    {
        RegisterPlayerEntity(entity);
        SYSTEM_LOG << "    -> Registered player entity: " << entityInstance->name << "\n";
    }
    
    if (blueprint->HasCategory("HasAI"))
    {
        // Initialize AI systems if needed
        SYSTEM_LOG << "    -> Entity has AI: " << entityInstance->name << "\n";
    }
    
    // Log entity creation with layer information
    if (HasComponent<Position_data>(entity))
    {
        const Position_data& pos = GetComponent<Position_data>(entity);
        RenderLayer layer = CalculateLayerFromZOrder(pos.position.z);
        SYSTEM_LOG << "    ✓ " << entityInstance->name 
                   << " [" << blueprint->prefabName << "]"
                   << " (layer: " << static_cast<int>(layer) << ", z: " << pos.position.z << ")"
                   << std::endl;
    }
    else
    {
        SYSTEM_LOG << "    ✓ " << entityInstance->name << " [" << blueprint->prefabName << "]\n";
    }
    
    stats.successfullyCreated++;
    
    return entity;
}

//=============================================================================
// Phase 3: Instantiation Pass Implementations
//=============================================================================

bool World::InstantiatePass1_VisualLayers(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    // Extract map orientation and tile size from metadata
    m_mapOrientation = levelDef.metadata.customData.value("orientation", "orthogonal");
    m_tileWidth = levelDef.metadata.customData.value("tilewidth", 32);
    m_tileHeight = levelDef.metadata.customData.value("tileheight", 32);
    
    // Extract map bounds and chunk origin for coordinate system alignment
    int minTileX = levelDef.metadata.customData.value("minTileX", 0);
    int minTileY = levelDef.metadata.customData.value("minTileY", 0);
    int maxTileX = levelDef.metadata.customData.value("maxTileX", 0);
    int maxTileY = levelDef.metadata.customData.value("maxTileY", 0);
    int chunkOriginX = levelDef.metadata.customData.value("chunkOriginX", 0);
    int chunkOriginY = levelDef.metadata.customData.value("chunkOriginY", 0);
    
    SetMapBounds(minTileX, minTileY, maxTileX, maxTileY, chunkOriginX, chunkOriginY);
    
    std::cout << "-> Map configuration: " << m_mapOrientation 
               << " (" << m_tileWidth << "x" << m_tileHeight << ")\n";
    
    // Log isometric origin for verification
    if (m_mapOrientation == "isometric")
    {
        SYSTEM_LOG << "[World] Isometric origin calculated: (" 
                   << GetIsometricOriginX() << ", " << GetIsometricOriginY() << ")\n";
    }
    
    // ===== PART 1: Parallax Layers =====
    if (levelDef.metadata.customData.contains("parallaxLayers"))
    {
        Olympe::Tiled::ParallaxLayerManager& parallaxMgr = Olympe::Tiled::ParallaxLayerManager::Get();
        parallaxMgr.Clear();
        
        const auto& parallaxLayersJson = levelDef.metadata.customData["parallaxLayers"];
        if (parallaxLayersJson.is_array())
        {
            result.pass1_visualLayers.totalObjects = static_cast<int>(parallaxLayersJson.size());
            
            for (const auto& layerJson : parallaxLayersJson)
            {
                std::string imagePath = layerJson.value("imagePath", "");
                if (imagePath.empty())
                {
                    result.pass1_visualLayers.failed++;
                    result.pass1_visualLayers.failedObjects.push_back("<missing imagePath>");
                    std::cout << "  x Failed: parallax layer missing imagePath\n";
                    continue;
                }
                
                std::string filename;

				// extract only filename if full path is given
				size_t lastSlash = imagePath.find_last_of("/\\");
				if (lastSlash != std::string::npos)
                    filename = imagePath.substr(lastSlash + 1);
				//recursive search file from Gamedata folder
				imagePath = DataManager::Get().FindResourceRecursive(filename);

                SDL_Texture* texture = IMG_LoadTexture(GameEngine::renderer, imagePath.c_str());
                
                if (!texture)
                {
                    result.pass1_visualLayers.failed++;
                    result.pass1_visualLayers.failedObjects.push_back(imagePath);
                    std::cout << "  x Failed to load parallax layer: " << imagePath << "\n";
                    continue;
                }
                
                Olympe::Tiled::ParallaxLayer layer;
                layer.name = layerJson.value("name", "unnamed_parallax");
                layer.imagePath = imagePath;
                layer.texture = texture;
                layer.scrollFactorX = layerJson.value("scrollFactorX", 1.0f);
                layer.scrollFactorY = layerJson.value("scrollFactorY", 0.0f);
                layer.repeatX = layerJson.value("repeatX", false);
                layer.repeatY = layerJson.value("repeatY", false);
                layer.offsetX = layerJson.value("offsetX", 0.0f);
                layer.offsetY = layerJson.value("offsetY", 0.0f);
                layer.opacity = layerJson.value("opacity", 1.0f);
                layer.zOrder = layerJson.value("zOrder", 0);
                layer.visible = layerJson.value("visible", true);
                layer.tintColor = layerJson.value("tintColor", 0xFFFFFFFF);

                parallaxMgr.AddLayer(layer);
                result.pass1_visualLayers.successfullyCreated++;
                std::cout << "  -> Loaded parallax layer: " << layer.name << "\n";
            }
        }
    }
    
    // ===== PART 2: Tilesets =====
    if (levelDef.metadata.customData.contains("tilesets"))
    {
        std::cout << "-> Loading tilesets...\n";
        m_tilesetManager.LoadTilesets(levelDef.metadata.customData["tilesets"]);
    }
    
    // ===== PART 3: Tile Layers =====
    if (levelDef.metadata.customData.contains("tileLayers"))
    {
        const auto& tileLayersJson = levelDef.metadata.customData["tileLayers"];
        
        if (tileLayersJson.is_array())
        {
            std::cout << "-> Loading " << tileLayersJson.size() << " tile layers...\n";
            
            for (size_t i = 0; i < tileLayersJson.size(); ++i)
            {
                const auto& layerJson = tileLayersJson[i];
                std::string layerType = layerJson.value("type", "");
                
                if (layerType == "tilelayer")
                {
                    LoadTileLayer(layerJson, result);
                }
            }
        }
    }
    
    // NOTE: Tile chunks are now loaded and stored in m_tileChunks.
    // To fully render these tiles, the following work is still needed:
    // 
    // 1. Store tileset metadata (firstgid, tilewidth, tileheight, source texture)
    //    during level loading in Phase 1 or Phase 2
    // 
    // 2. Create a helper method to map tile GID to (texture, srcRect):
    //    - Find which tileset the GID belongs to (using firstgid ranges)
    //    - Calculate source rectangle within the tileset texture
    //    - Handle collection vs image-based tilesets differently
    // 
    // 3. Integrate tile rendering into RenderMultiLayerForCamera():
    //    - Add TileLayer render items to the depth-sorted render queue
    //    - Use IsometricRenderer for isometric maps
    //    - Use simple quad rendering for orthogonal maps
    //
    // See: Source/Rendering/IsometricRenderer.h for the rendering interface
    // See: Source/DataManager.h - PreloadTilesets() for texture loading
    
    return true;
}

//=============================================================================
// Tile Layer Loading Helper Methods
//=============================================================================

void World::LoadTileLayer(const nlohmann::json& layerJson, InstantiationResult& result)
{
    std::string layerName = layerJson.value("name", "unnamed_layer");
    int zOrder = layerJson.value("zOrder", 0);
    bool visible = layerJson.value("visible", true);
    float opacity = layerJson.value("opacity", 1.0f);
    std::string encoding = layerJson.value("encoding", "");  // Extract encoding from layer
    
    if (!visible)
    {
        std::cout << "  ⊙ Skipping invisible layer: " << layerName << "\n";
        return;
    }
    
    result.pass1_visualLayers.totalObjects++;
    
    // Handle infinite maps with chunks
    if (layerJson.contains("chunks") && layerJson["chunks"].is_array())
    {
        const auto& chunks = layerJson["chunks"];
        
        std::cout << "  -> Tile Layer (Infinite): " << layerName 
                  << " (" << chunks.size() << " chunks, encoding: " << encoding << ", z: " << zOrder << ")\n";
        
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            LoadTileChunk(chunks[i], layerName, zOrder, encoding);  // Pass encoding
        }
        
        result.pass1_visualLayers.successfullyCreated++;
    }
    // Handle finite maps with regular data
    else if (layerJson.contains("data"))
    {
        int width = layerJson.value("width", 0);
        int height = layerJson.value("height", 0);
        
        std::cout << "  -> Tile Layer (Finite): " << layerName 
                  << " (" << width << "x" << height << ", z: " << zOrder << ")\n";
        
        LoadTileData(layerJson["data"], layerName, width, height, zOrder, encoding);  // Pass encoding
        result.pass1_visualLayers.successfullyCreated++;
    }
    else
    {
        std::cout << "  x Tile layer missing data: " << layerName << "\n";
        result.pass1_visualLayers.failed++;
    }
}

void World::LoadTileChunk(const nlohmann::json& chunkJson, const std::string& layerName, 
                          int zOrder, const std::string& encoding)
{
    int chunkX = chunkJson.value("x", 0);
    int chunkY = chunkJson.value("y", 0);
    int chunkW = chunkJson.value("width", 0);
    int chunkH = chunkJson.value("height", 0);
    
    // Decode tile data
    std::vector<uint32_t> tileGIDs;
    
    if (chunkJson.contains("data"))
    {
        if (chunkJson["data"].is_string())
        {
            // Base64 encoded data
            std::string dataStr = chunkJson["data"].get<std::string>();
            tileGIDs = Olympe::Tiled::TiledDecoder::DecodeTileData(
                dataStr, 
                encoding,  // Use layer encoding
                ""         // No compression
            );
        }
        else if (chunkJson["data"].is_array())
        {
            // Direct array of GIDs
            for (const auto& gid : chunkJson["data"])
            {
                tileGIDs.push_back(gid.get<uint32_t>());
            }
        }
    }
    
    if (tileGIDs.empty())
    {
        std::cout << "    x Failed to decode chunk at (" << chunkX << ", " << chunkY << ")\n";
        return;
    }
    
    // Store chunk for rendering
    TileChunk chunk;
    chunk.layerName = layerName;
    chunk.x = chunkX;
    chunk.y = chunkY;
    chunk.width = chunkW;
    chunk.height = chunkH;
    chunk.zOrder = zOrder;
    chunk.tileGIDs = tileGIDs;
    
    m_tileChunks.push_back(chunk);
    
    std::cout << "    ok - Loaded chunk at (" << chunkX << ", " << chunkY 
              << ") - " << tileGIDs.size() << " tiles\n";
}

void World::LoadTileData(const nlohmann::json& dataJson, const std::string& layerName, 
                         int width, int height, int zOrder, const std::string& encoding)
{
    std::vector<uint32_t> tileGIDs;
    
    if (dataJson.is_string())
    {
        // Base64 encoded
        std::string dataStr = dataJson.get<std::string>();
        tileGIDs = Olympe::Tiled::TiledDecoder::DecodeTileData(dataStr, encoding, "");
    }
    else if (dataJson.is_array())
    {
        // Direct array of tile IDs
        for (const auto& tile : dataJson)
        {
            tileGIDs.push_back(tile.get<uint32_t>());
        }
    }
    
    if (tileGIDs.empty())
    {
        std::cout << "    x No tile data for layer: " << layerName << "\n";
        return;
    }
    
    // Store as a single chunk
    TileChunk chunk;
    chunk.layerName = layerName;
    chunk.x = 0;
    chunk.y = 0;
    chunk.width = width;
    chunk.height = height;
    chunk.zOrder = zOrder;
    chunk.tileGIDs = tileGIDs;
    
    m_tileChunks.push_back(chunk);
}

// ========================================================================
// TilesetManager Implementation
// ========================================================================

void TilesetManager::Clear()
{
    // Cleanup textures - these are owned by TilesetManager and loaded directly
    // via IMG_LoadTexture, not through DataManager's texture cache
    for (auto& tileset : m_tilesets)
    {
        if (tileset.texture)
        {
            SDL_DestroyTexture(tileset.texture);
            tileset.texture = nullptr;
        }
        
        for (auto& pair : tileset.individualTiles)
        {
            if (pair.second)
            {
                SDL_DestroyTexture(pair.second);
            }
        }
        tileset.individualTiles.clear();
    }
    
    m_tilesets.clear();
}

void TilesetManager::LoadTilesets(const nlohmann::json& tilesetsJson)
{
    Clear();
    
    if (!tilesetsJson.is_array()) return;
    
    for (const auto& tilesetJson : tilesetsJson)
    {
        TilesetInfo info;
        info.firstgid = tilesetJson.value("firstgid", 0);
        
        // ========================================================================
        // CRITICAL: Check if tileset is external (has "source" property)
        // ========================================================================
        if (tilesetJson.contains("source") && !tilesetJson["source"].get<std::string>().empty())
        {
            std::string sourceFile = tilesetJson["source"].get<std::string>();
            
            SYSTEM_LOG << "  -> Loading external tileset: " << sourceFile << "\n";
            
            // Build full path relative to Levels directory
            std::string baseDir = "Gamedata/Levels/";
            std::string fullPath = baseDir + sourceFile;
            
            // ========================================================================
            // CRITICAL: Use TilesetCache to load and parse external tileset file
            // ========================================================================
            auto cachedTileset = Olympe::Tiled::TilesetCache::GetInstance().GetTileset(fullPath);
            
            if (cachedTileset)
            {
                // Copy properties from TiledTileset to TilesetInfo
                info.name = cachedTileset->name;
                info.tilewidth = cachedTileset->tilewidth;
                info.tileheight = cachedTileset->tileheight;
                info.columns = cachedTileset->columns;
                info.imagewidth = cachedTileset->imagewidth;
                info.imageheight = cachedTileset->imageheight;
                info.margin = cachedTileset->margin;
                info.spacing = cachedTileset->spacing;
                
                // ========================================================================
                // CRITICAL: Extract tileoffset from external tileset file
                // These offsets come directly from the .tsx/.tsj file's <tileoffset> element
                // and must be applied to ALL tiles in this tileset during rendering.
                // ========================================================================
                info.tileoffsetX = cachedTileset->tileoffsetX;
                info.tileoffsetY = cachedTileset->tileoffsetY;
                
                SYSTEM_LOG << "[TilesetManager] ========================================+\n";
                SYSTEM_LOG << "[TilesetManager] Loading external tileset: " << info.name << "\n";
                SYSTEM_LOG << "[TilesetManager] Parsed global offset from cache: (" 
                          << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
                SYSTEM_LOG << "[TilesetManager] GID range: " << info.firstgid << " - " 
                          << (info.firstgid + cachedTileset->tilecount - 1) << "\n";
                
                // Determine if collection tileset
                info.isCollection = !cachedTileset->tiles.empty() && cachedTileset->image.empty();
                
                // Calculate lastgid
                uint32_t tilecount = cachedTileset->tilecount;
                info.lastgid = info.firstgid + tilecount - 1;
                
                // ========================================================================
                // Load textures based on tileset type
                // ========================================================================
                if (!info.isCollection && !cachedTileset->image.empty())
                {
                    // Image-based tileset
                    std::string imagePath = cachedTileset->image;
                    
                    // Extract filename
                    size_t lastSlash = imagePath.find_last_of("/\\");
                    std::string filename = (lastSlash != std::string::npos) ? 
                        imagePath.substr(lastSlash + 1) : imagePath;
                    
                    // Find resource recursively
                    std::string fullImagePath = DataManager::Get().FindResourceRecursive(filename);
                    
                    if (!fullImagePath.empty())
                    {
                        info.texture = IMG_LoadTexture(GameEngine::renderer, fullImagePath.c_str());
                        if (info.texture)
                        {
                            SYSTEM_LOG << "[TilesetManager] ok - Loaded atlas texture: " << filename << "\n";
                            SYSTEM_LOG << "[TilesetManager] Image-based tileset - All " 
                                      << tilecount << " tiles will use offset (" 
                                      << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
                            SYSTEM_LOG << "[TilesetManager] ========================================+\n";
                        }
                        else
                        {
                            SYSTEM_LOG << "  x Failed to load tileset texture: " << fullImagePath << "\n";
                        }
                    }
                }
                else if (info.isCollection)
                {
                    // ====================================================================
                    // Collection tileset (individual tile images)
                    // Each tile in a collection inherits the tileset's global tileoffset
                    // ====================================================================
                    
                    int loadedCount = 0;
                    for (const auto& tile : cachedTileset->tiles)
                    {
                        if (tile.image.empty()) continue;
                        
                        uint32_t tileId = tile.id;
                        uint32_t gid = info.firstgid + tileId;
                        std::string imagePath = tile.image;
                        
                        // Extract filename
                        size_t lastSlash = imagePath.find_last_of("/\\");
                        std::string filename = (lastSlash != std::string::npos) ? 
                            imagePath.substr(lastSlash + 1) : imagePath;
                        
                        // Find resource recursively
                        std::string fullImagePath = DataManager::Get().FindResourceRecursive(filename);
                        
                        if (!fullImagePath.empty())
                        {
                            SDL_Texture* tex = IMG_LoadTexture(GameEngine::renderer, fullImagePath.c_str());
                            if (tex)
                            {
                                info.individualTiles[tileId] = tex;
                                
                                SDL_Rect srcRect;
                                srcRect.x = 0;
                                srcRect.y = 0;
                                srcRect.w = tile.imagewidth > 0 ? tile.imagewidth : info.tilewidth;
                                srcRect.h = tile.imageheight > 0 ? tile.imageheight : info.tileheight;
                                info.individualSrcRects[tileId] = srcRect;
                                
                                loadedCount++;
                                
                                // Log first few tiles and any in Trees GID range (127-135)
                                if (loadedCount <= 3 || (gid >= 127 && gid <= 135))
                                {
                                    SYSTEM_LOG << "[TilesetManager] GID " << gid 
                                              << " (" << srcRect.w << "x" << srcRect.h << ")"
                                              << " - STORED with tileset offset: (" 
                                              << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
                                }
                            }
                        }
                    }
                    
                    SYSTEM_LOG << "[TilesetManager] ok - Loaded collection tileset: " << info.name 
                              << " (" << info.individualTiles.size() << " tiles)\n";
                    SYSTEM_LOG << "[TilesetManager] All tiles stored with global offset: (" 
                              << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
                    SYSTEM_LOG << "[TilesetManager] ========================================+\n";
                }
            }
            else
            {
                SYSTEM_LOG << "  x Failed to load external tileset: " << sourceFile << "\n";
                continue;
            }
        }
        else
        {
            // ========================================================================
            // Embedded tileset (inline in TMJ) - LEGACY CODE PATH
            // ========================================================================
            info.name = tilesetJson.value("name", "");
            info.tilewidth = tilesetJson.value("tilewidth", 32);
            info.tileheight = tilesetJson.value("tileheight", 32);
            info.columns = tilesetJson.value("columns", 0);
            info.imagewidth = tilesetJson.value("imagewidth", 0);
            info.imageheight = tilesetJson.value("imageheight", 0);
            info.margin = tilesetJson.value("margin", 0);
            info.spacing = tilesetJson.value("spacing", 0);
            
            // ====================================================================
            // CRITICAL: Parse tileoffset from embedded tileset
            // These offsets come from the embedded tileset's tileoffset property
            // ====================================================================
            if (tilesetJson.contains("tileoffset"))
            {
                info.tileoffsetX = tilesetJson["tileoffset"].value("x", 0);
                info.tileoffsetY = tilesetJson["tileoffset"].value("y", 0);
                
                SYSTEM_LOG << "  [TilesetManager] Embedded tileset '" << info.name << "'"
                          << " - Global tileoffset: (" << info.tileoffsetX 
                          << ", " << info.tileoffsetY << ")\n";
            }
            else
            {
                // Explicit default values
                info.tileoffsetX = 0;
                info.tileoffsetY = 0;
            }
            
            uint32_t tilecount = tilesetJson.value("tilecount", 0);
            info.lastgid = info.firstgid + tilecount - 1;
            
            info.isCollection = (info.columns == 0);
            
            // Load embedded tileset textures (existing code)
            if (!info.isCollection && tilesetJson.contains("image"))
            {
                std::string imagePath = tilesetJson["image"].get<std::string>();
                
                size_t lastSlash = imagePath.find_last_of("/\\");
                std::string filename = (lastSlash != std::string::npos) ? 
                    imagePath.substr(lastSlash + 1) : imagePath;
                
                std::string fullPath = DataManager::Get().FindResourceRecursive(filename);
                
                if (!fullPath.empty())
                {
                    info.texture = IMG_LoadTexture(GameEngine::renderer, fullPath.c_str());
                    if (info.texture)
                    {
                        SYSTEM_LOG << "  ok - Loaded embedded tileset texture: " << filename 
                                  << " (gid: " << info.firstgid << "-" << info.lastgid << ")\n";
                    }
                    else
                    {
                        SYSTEM_LOG << "  x Failed to load embedded tileset texture: " << fullPath << "\n";
                    }
                }
            }
            else if (info.isCollection && tilesetJson.contains("tiles"))
            {
                const auto& tilesArray = tilesetJson["tiles"];
                
                for (const auto& tileJson : tilesArray)
                {
                    uint32_t tileId = tileJson["id"].get<uint32_t>();
                    std::string imagePath = tileJson["image"].get<std::string>();
                    
                    size_t lastSlash = imagePath.find_last_of("/\\");
                    std::string filename = (lastSlash != std::string::npos) ? 
                        imagePath.substr(lastSlash + 1) : imagePath;
                    
                    std::string fullPath = DataManager::Get().FindResourceRecursive(filename);
                    
                    if (!fullPath.empty())
                    {
                        SDL_Texture* tex = IMG_LoadTexture(GameEngine::renderer, fullPath.c_str());
                        if (tex)
                        {
                            info.individualTiles[tileId] = tex;
                            
                            SDL_Rect srcRect;
                            srcRect.x = 0;
                            srcRect.y = 0;
                            srcRect.w = tileJson.value("width", info.tilewidth);
                            srcRect.h = tileJson.value("height", info.tileheight);
                            info.individualSrcRects[tileId] = srcRect;
                        }
                    }
                }
                
                SYSTEM_LOG << "  ok - Loaded embedded collection tileset: " << info.name 
                          << " (" << info.individualTiles.size() << " tiles)\n";
            }
        }
        
        m_tilesets.push_back(info);
        
        // ====================================================================
        // POST-INSERTION VERIFICATION: Check that offset values survived storage
        // ====================================================================
        const TilesetInfo& storedInfo = m_tilesets.back();
        if (storedInfo.tileoffsetX != info.tileoffsetX || storedInfo.tileoffsetY != info.tileoffsetY)
        {
            SYSTEM_LOG << "[TilesetManager] X ERROR: Offset LOST during vector storage!\n";
            SYSTEM_LOG << "                  Expected: (" << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
            SYSTEM_LOG << "                  Got: (" << storedInfo.tileoffsetX << ", " << storedInfo.tileoffsetY << ")\n";
        }
        else if (info.tileoffsetX != 0 || info.tileoffsetY != 0)
        {
            SYSTEM_LOG << "[TilesetManager] ok - POST-INSERT verification: offset=(" 
                      << storedInfo.tileoffsetX << ", " << storedInfo.tileoffsetY << ") preserved\n";
        }
    }
    
    // ========================================================================
    // CRITICAL FIX: Sort tilesets by firstgid in DESCENDING order
    // This ensures that when searching for a GID in GetTileTexture(), we check
    // the most specific (highest firstgid) tileset first.
    // 
    // Example: If we have overlapping ranges:
    //   - tiles-iso-1: [1-396]
    //   - Tiles iso cube: [109-126] (embedded in tiles-iso-1)
    //   - Trees: [127-205] (embedded in tiles-iso-1)
    // 
    // Without sorting, GID 127 would match tiles-iso-1 first (wrong).
    // With descending sort, GID 127 matches Trees first (correct).
    // ========================================================================
    std::sort(m_tilesets.begin(), m_tilesets.end(), 
        [](const TilesetInfo& a, const TilesetInfo& b) {
            return a.firstgid > b.firstgid;  // Descending order
        });
    
    SYSTEM_LOG << "\n[TilesetManager] ========================================+\n";
    SYSTEM_LOG << "[TilesetManager] Tileset load complete. Final ordering (by firstgid DESC):\n";
    for (const auto& tileset : m_tilesets)
    {
        SYSTEM_LOG << "  - " << tileset.name 
                  << " [" << tileset.firstgid << " - " << tileset.lastgid << "]"
                  << " offset=(" << tileset.tileoffsetX << ", " << tileset.tileoffsetY << ")\n";
    }
    
    // ========================================================================
    // VALIDATION: Detect overlapping GID ranges (warning only)
    // This helps identify potential configuration issues in Tiled maps
    // ========================================================================
    bool hasOverlaps = false;
    for (size_t i = 0; i < m_tilesets.size(); ++i)
    {
        for (size_t j = i + 1; j < m_tilesets.size(); ++j)
        {
            const auto& ts1 = m_tilesets[i];
            const auto& ts2 = m_tilesets[j];
            
            // Check if ranges overlap
            // After sorting by descending firstgid:
            // ts1.firstgid >= ts2.firstgid, so we only need to check if ts1.firstgid <= ts2.lastgid
            if (ts1.firstgid <= ts2.lastgid)
            {
                hasOverlaps = true;
                SYSTEM_LOG << "  [WARNING] GID range overlap detected!\n";
                SYSTEM_LOG << "      Tileset '" << ts1.name 
                          << "' [" << ts1.firstgid << "-" << ts1.lastgid << "]\n";
                SYSTEM_LOG << "      overlaps with '" << ts2.name 
                          << "' [" << ts2.firstgid << "-" << ts2.lastgid << "]\n";
                SYSTEM_LOG << "      -> GetTileTexture() will prioritize '" << ts1.name 
                          << "' due to higher firstgid\n";
            }
        }
    }
    
    if (!hasOverlaps)
    {
        SYSTEM_LOG << "  [OK] No GID range overlaps detected\n";
    }
    SYSTEM_LOG << "[TilesetManager] ========================================+\n";
}

bool TilesetManager::GetTileTexture(uint32_t gid, SDL_Texture*& outTexture, SDL_Rect& outSrcRect, const TilesetInfo*& outTileset)
{
    // Strip flip flags (top 3 bits)
    uint32_t cleanGid = gid & 0x1FFFFFFF;
    
    if (cleanGid == 0)
    {
        outTileset = nullptr;
        return false;  // Empty tile
    }
    
    // ====================================================================
    // Find the tileset with the HIGHEST firstgid that contains this GID.
    // This ensures we get the most specific tileset when ranges overlap.
    // For example, if tiles-iso-1 has [1-396] and Trees has [127-205],
    // GID 127 should match Trees (firstgid=127) not tiles-iso-1 (firstgid=1).
    // ====================================================================
    const TilesetInfo* bestMatch = nullptr;
    
    for (const auto& tileset : m_tilesets)
    {
        if (cleanGid >= tileset.firstgid && cleanGid <= tileset.lastgid)
        {
            // Select tileset with highest firstgid
            if (!bestMatch || tileset.firstgid > bestMatch->firstgid)
            {
                bestMatch = &tileset;
            }
        }
    }
    
    if (bestMatch)
    {
        const TilesetInfo& tileset = *bestMatch;
        uint32_t localId = cleanGid - tileset.firstgid;
        
        // -> CRITICAL: Set the tileset pointer BEFORE any return
        outTileset = bestMatch;
        
        if (tileset.isCollection)
        {
            // Collection tileset - lookup individual tile
            auto it = tileset.individualTiles.find(localId);
            if (it != tileset.individualTiles.end())
            {
                auto srcIt = tileset.individualSrcRects.find(localId);
                if (srcIt != tileset.individualSrcRects.end())
                {
                    outTexture = it->second;
                    outSrcRect = srcIt->second;
                    
                    if (outTexture == nullptr)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
                            "[TILESET] NULL texture for collection tile GID=%u, localId=%u", gid, localId);
                        return false;
                    }
                    
                    return true;
                }
            }
            
            // Collection tile not found
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
                "[TILESET] Collection tile not found: GID=%u, localId=%u", gid, localId);
            return false;
        }
        else
        {
            // Image-based tileset - calculate source rect
            if (!tileset.texture)
            {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
                    "[TILESET] NULL texture for tileset '%s' (GID=%u)", tileset.name.c_str(), gid);
                return false;
            }
            
            outTexture = tileset.texture;
            
            // Calculate source rect with margin and spacing
            int col = localId % tileset.columns;
            int row = localId / tileset.columns;
            
            outSrcRect.x = tileset.margin + col * (tileset.tilewidth + tileset.spacing);
            outSrcRect.y = tileset.margin + row * (tileset.tileheight + tileset.spacing);
            outSrcRect.w = tileset.tilewidth;
            outSrcRect.h = tileset.tileheight;
            
            return true;
        }
    }
    
    // GID not found in any tileset
    SYSTEM_LOG << "[TilesetManager::GetTileTexture] X GID " << cleanGid 
              << " NOT FOUND in any tileset (total tilesets: " << m_tilesets.size() << ")\n";
    outTileset = nullptr;
    return false;
}

bool World::InstantiatePass2_SpatialStructure(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    // =========================================================================
    // REMOVED: Legacy collision and sector creation
    // =========================================================================
    // All entities (including Collision and Sector types) are now instantiated
    // via PrefabFactory in Phase 5 (unified entity instantiation).
    // This ensures consistency and eliminates double instantiation issues.
    // =========================================================================
    
    return true;
}

//=============================================================================
// DEPRECATED: Pass3 and Pass4 (Replaced by Unified InstantiateEntity)
//=============================================================================
// These methods are no longer used in the 6-phase pipeline
// Entity instantiation is now handled by the unified InstantiateEntity() helper
// in Phase 5 of LoadLevelFromTiled()

bool World::InstantiatePass5_Relationships(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    SYSTEM_LOG << "\n[Pass 5] Linking object relationships...\n";
    
    int linksCreated = 0;
    
    // Process object links from level definition
    for (const auto& link : levelDef.objectLinks) {
        if (link.linkType == "patrol_path") {
            // Find guard entity
            auto guardIt = result.entityRegistry.find(link.sourceObjectName);
            if (guardIt == result.entityRegistry.end()) {
                SYSTEM_LOG << "  x Guard '" << link.sourceObjectName << "' not found in registry\n";
                result.pass5_relationships.failed++;
                continue;
            }
            
            // Find patrol path entity
            auto pathIt = result.entityRegistry.find(link.targetObjectName);
            if (pathIt == result.entityRegistry.end()) {
                SYSTEM_LOG << "  x Patrol path '" << link.targetObjectName << "' not found in registry\n";
                result.pass5_relationships.failed++;
                continue;
            }
            
            EntityID guard = guardIt->second;
            EntityID patrolPath = pathIt->second;
            
            // Link guard to patrol path via AIBlackboard_data
            if (HasComponent<AIBlackboard_data>(guard)) {
                AIBlackboard_data& guardBlackboard = GetComponent<AIBlackboard_data>(guard);
                
                // Get patrol points from patrol path entity
                if (HasComponent<AIBlackboard_data>(patrolPath)) {
                    const AIBlackboard_data& pathData = GetComponent<AIBlackboard_data>(patrolPath);
                    
                    // Copy patrol points from path to guard (bounded)
                    const int maxPoints = static_cast<int>(std::size(guardBlackboard.patrolPoints));
                    const int count = std::min(pathData.patrolPointCount, maxPoints);
                    guardBlackboard.patrolPointCount = count;
                    for (int i = 0; i < count; ++i) {
                        guardBlackboard.patrolPoints[i] = pathData.patrolPoints[i];
                    }
                    guardBlackboard.currentPatrolIndex = 0;
                    guardBlackboard.hasPatrolPath = (count > 0);
                    
                    SYSTEM_LOG << "  ok - Linked guard '" << link.sourceObjectName 
                               << "' -> patrol '" << link.targetObjectName 
                               << "' (" << guardBlackboard.patrolPointCount << " points)\n";
                    
                    result.pass5_relationships.linkedObjects++;
                    linksCreated++;
                } else {
                    SYSTEM_LOG << "  x Patrol path '" << link.targetObjectName 
                               << "' missing AIBlackboard_data\n";
                    result.pass5_relationships.failed++;
                }
            } else {
                SYSTEM_LOG << "  x Guard '" << link.sourceObjectName 
                           << "' missing AIBlackboard_data\n";
                result.pass5_relationships.failed++;
            }
        }
        // TODO: Handle other link types (trigger_target, etc.) here
    }
    
    // Legacy: Assign patrol paths from entity overrides (backward compatibility)
    for (const auto& entityInstance : levelDef.entities)
    {
        if (!entityInstance) continue;
        
        // Look for entity in registry
        auto it = result.entityRegistry.find(entityInstance->name);
        if (it == result.entityRegistry.end()) continue;
        
        EntityID entity = it->second;
        
        // Handle patrol paths embedded in overrides
        if (!entityInstance->overrides.is_null() && entityInstance->overrides.contains("patrolPath"))
        {
            if (HasComponent<AIBlackboard_data>(entity))
            {
                result.pass5_relationships.totalObjects++;
                
                AIBlackboard_data& blackboard = GetComponent<AIBlackboard_data>(entity);
                const auto& patrolPath = entityInstance->overrides["patrolPath"];
                
                if (patrolPath.is_array())
                {
                    const int maxPoints = static_cast<int>(std::size(blackboard.patrolPoints));
                    blackboard.patrolPointCount = 0;
                    for (size_t i = 0; i < patrolPath.size() && blackboard.patrolPointCount < maxPoints; ++i)
                    {
                        if (patrolPath[i].contains("x") && patrolPath[i].contains("y"))
                        {
                            blackboard.patrolPoints[blackboard.patrolPointCount].x = 
                                static_cast<float>(patrolPath[i]["x"].get<float>());
                            blackboard.patrolPoints[blackboard.patrolPointCount].y = 
                                static_cast<float>(patrolPath[i]["y"].get<float>());
                            blackboard.patrolPoints[blackboard.patrolPointCount].z = 0.0f;
                            blackboard.patrolPointCount++;
                        }
                    }
                    
                    result.pass5_relationships.successfullyCreated++;
                    std::cout << "  -> Assigned " << blackboard.patrolPointCount 
                              << " patrol points to: " << entityInstance->name << "\n";
                }
            }
        }
    }
    
    SYSTEM_LOG << "  ok - Created " << linksCreated << " object relationships\n";
    return true;
}

// ========================================================================
// Helper Methods for Entity Instantiation
// ========================================================================

// Helper function to convert JSON value to ComponentParameter consistently
namespace {
    ComponentParameter JsonValueToComponentParameter(const nlohmann::json& value)
    {
        ComponentParameter param;
        
        if (value.is_number_float())
        {
            param = ComponentParameter::FromFloat(value.get<float>());
        }
        else if (value.is_number_integer())
        {
            param = ComponentParameter::FromInt(value.get<int>());
        }
        else if (value.is_boolean())
        {
            param = ComponentParameter::FromBool(value.get<bool>());
        }
        else if (value.is_string())
        {
            param = ComponentParameter::FromString(value.get<std::string>());
        }
        else if (value.is_array())
        {
            // Check if this is a numeric array that could be a Vector
            if (value.size() >= 2 && value.size() <= 3 && value[0].is_number())
            {
                // Handle vector types with validation
                bool hasInvalidElements = false;
                
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                
                if (value[0].is_number()) {
                    x = value[0].get<float>();
                } else {
                    hasInvalidElements = true;
                }
                
                if (value[1].is_number()) {
                    y = value[1].get<float>();
                } else {
                    hasInvalidElements = true;
                }
                
                if (value.size() >= 3) {
                    if (value[2].is_number()) {
                        z = value[2].get<float>();
                    } else {
                        hasInvalidElements = true;
                    }
                }
                
                if (hasInvalidElements) {
                    SYSTEM_LOG << "[World] WARNING: Vector array contains non-numeric elements: " 
                               << value.dump() << ". Non-numeric values defaulted to 0.0f." << std::endl;
                }
                
                param = ComponentParameter::FromVector3(x, y, z);
            }
            else
            {
                // Preserve as array (for patrol paths, waypoints, etc.)
                param = ComponentParameter::FromArray(value);
            }
        }
        else
        {
            // Default to string representation
            param = ComponentParameter::FromString(value.dump());
        }
        
        return param;
    }
}

void World::ExtractCustomProperties(
    const nlohmann::json& overrides, 
    LevelInstanceParameters& instanceParams,
    const Olympe::Editor::EntityInstance* entityInstance,
    const PrefabBlueprint* prefab)
{
    // REQUIREMENT A: Automatic TMJ field mapping (NO cross-component overwrites)
    // Map TMJ object fields (x, y, width, height) to component-scoped overrides
    // ONLY if entityInstance and prefab are provided, and ONLY if not already set
    
    if (entityInstance && prefab)
    {
        // Helper lambda to check if prefab has a component
        auto prefabHasComponent = [&](const std::string& componentType) -> bool {
            for (const auto& comp : prefab->components) {
                if (comp.componentType == componentType)
                    return true;
            }
            return false;
        };
        
        // Helper lambda to check if component-scoped override already set
        auto hasComponentOverride = [&](const std::string& componentType, const std::string& paramName) -> bool {
            auto compIt = instanceParams.componentOverrides.find(componentType);
            if (compIt != instanceParams.componentOverrides.end()) {
                return compIt->second.find(paramName) != compIt->second.end();
            }
            return false;
        };
        
        // 1) Position_data: Always map x/y/z from entityInstance.position (TMJ position)
        // NOTE: This is already done via instanceParams.position = entityInstance->position
        // No need to create component override here as it's handled by ParameterResolver
        
        // Check if this is a point object (spawners, waypoints)
        bool isPointObject = false;
        if (!overrides.is_null())
        {
            if (overrides.contains("point") && overrides["point"].is_boolean())
                isPointObject = overrides["point"].get<bool>();
            // Also check if both width and height are explicitly 0
            if (!isPointObject && overrides.contains("width") && overrides.contains("height"))
            {
                if (overrides["width"].is_number() && overrides["height"].is_number())
                {
                    float w = overrides["width"].get<float>();
                    float h = overrides["height"].get<float>();
                    if (w == 0.0f && h == 0.0f)
                        isPointObject = true;
                }
            }
        }
        
        // 2) BoundingBox_data: Map TMJ width/height AND x/y (offset) if prefab has it
        // Skip width/height for point objects
        if (prefabHasComponent("BoundingBox_data"))
        {
            // Get TMJ object dimensions (these come from TMJ object's width/height properties)
            // Note: For point objects, width/height may be 0
            float tmjWidth = 0.0f;
            float tmjHeight = 0.0f;
            float tmjX = 0.0f;
            float tmjY = 0.0f;
            
            // Extract from entityInstance metadata if available
            // TMJ width/height are typically stored in the object definition
            // For now, we'll check if they're in overrides or try to extract from context
            if (!overrides.is_null())
            {
                // Check if TMJ provided width/height as flat properties
                if (overrides.contains("width") && overrides["width"].is_number())
                    tmjWidth = overrides["width"].get<float>();
                if (overrides.contains("height") && overrides["height"].is_number())
                    tmjHeight = overrides["height"].get<float>();
                if (overrides.contains("x") && overrides["x"].is_number())
                    tmjX = overrides["x"].get<float>();
                if (overrides.contains("y") && overrides["y"].is_number())
                    tmjY = overrides["y"].get<float>();
            }
            
            // Only apply if dimensions are non-zero and not already overridden
            // NOTE: BoundingBox_data uses Int type for width/height per schema
            // TMJ dimensions (float) are rounded down to match schema type
            // SKIP width/height for point objects (spawners)
            if (!isPointObject && tmjWidth > 0.0f && !hasComponentOverride("BoundingBox_data", "width"))
            {
                instanceParams.componentOverrides["BoundingBox_data"]["width"] = 
                    ComponentParameter::FromInt(static_cast<int>(tmjWidth));
            }
            if (!isPointObject && tmjHeight > 0.0f && !hasComponentOverride("BoundingBox_data", "height"))
            {
                instanceParams.componentOverrides["BoundingBox_data"]["height"] = 
                    ComponentParameter::FromInt(static_cast<int>(tmjHeight));
            }
            if (tmjX != 0.0f && !hasComponentOverride("BoundingBox_data", "x"))
            {
                instanceParams.componentOverrides["BoundingBox_data"]["x"] = 
                    ComponentParameter::FromFloat(tmjX);
            }
            if (tmjY != 0.0f && !hasComponentOverride("BoundingBox_data", "y"))
            {
                instanceParams.componentOverrides["BoundingBox_data"]["y"] = 
                    ComponentParameter::FromFloat(tmjY);
            }
        }
        
        // 3) CollisionZone_data: Map TMJ x/y/width/height if prefab has it
        // Skip width/height for point objects
        if (prefabHasComponent("CollisionZone_data"))
        {
            float tmjWidth = 0.0f;
            float tmjHeight = 0.0f;
            float tmjX = 0.0f;
            float tmjY = 0.0f;
            int overridesApplied = 0;
            
            if (!overrides.is_null())
            {
                if (overrides.contains("width") && overrides["width"].is_number())
                    tmjWidth = overrides["width"].get<float>();
                if (overrides.contains("height") && overrides["height"].is_number())
                    tmjHeight = overrides["height"].get<float>();
                if (overrides.contains("x") && overrides["x"].is_number())
                    tmjX = overrides["x"].get<float>();
                if (overrides.contains("y") && overrides["y"].is_number())
                    tmjY = overrides["y"].get<float>();
            }
            
            // Apply if not already overridden (component-scoped overrides take precedence)
            if (tmjX != 0.0f && !hasComponentOverride("CollisionZone_data", "x"))
            {
                instanceParams.componentOverrides["CollisionZone_data"]["x"] = 
                    ComponentParameter::FromFloat(tmjX);
                overridesApplied++;
            }
            if (tmjY != 0.0f && !hasComponentOverride("CollisionZone_data", "y"))
            {
                instanceParams.componentOverrides["CollisionZone_data"]["y"] = 
                    ComponentParameter::FromFloat(tmjY);
                overridesApplied++;
            }
            // SKIP width/height for point objects
            if (!isPointObject && tmjWidth > 0.0f && !hasComponentOverride("CollisionZone_data", "width"))
            {
                instanceParams.componentOverrides["CollisionZone_data"]["width"] = 
                    ComponentParameter::FromFloat(tmjWidth);
                overridesApplied++;
            }
            if (!isPointObject && tmjHeight > 0.0f && !hasComponentOverride("CollisionZone_data", "height"))
            {
                instanceParams.componentOverrides["CollisionZone_data"]["height"] = 
                    ComponentParameter::FromFloat(tmjHeight);
                overridesApplied++;
            }
            
            // Log when CollisionZone_data overrides are injected
            if (overridesApplied > 0)
            {
                SYSTEM_LOG << "[World] CollisionZone_data: Applied " << overridesApplied 
                           << " TMJ overrides for '" << entityInstance->name << "'" << std::endl;
            }
        }
        
        // 4) PhysicsBody_data: Map TMJ rotation if prefab has it
        if (prefabHasComponent("PhysicsBody_data"))
        {
            float tmjRotation = 0.0f;
            
            if (!overrides.is_null())
            {
                if (overrides.contains("rotation") && overrides["rotation"].is_number())
                    tmjRotation = overrides["rotation"].get<float>();
            }
            
            // Apply rotation if non-zero and not already overridden
            if (tmjRotation != 0.0f && !hasComponentOverride("PhysicsBody_data", "rotation"))
            {
                instanceParams.componentOverrides["PhysicsBody_data"]["rotation"] = 
                    ComponentParameter::FromFloat(tmjRotation);
            }
        }
        
        // 5) VisualSprite_data: Map TMJ visible field if prefab has it
        if (prefabHasComponent("VisualSprite_data"))
        {
            bool tmjVisible = true;  // Default to visible
            
            if (!overrides.is_null())
            {
                if (overrides.contains("visible") && overrides["visible"].is_boolean())
                    tmjVisible = overrides["visible"].get<bool>();
            }
            
            // Apply visible if not already overridden
            // Note: We always apply this even if true, to ensure explicit TMJ visibility is respected
            if (!hasComponentOverride("VisualSprite_data", "visible"))
            {
                instanceParams.componentOverrides["VisualSprite_data"]["visible"] = 
                    ComponentParameter::FromBool(tmjVisible);
            }
        }
        
        // NOTE: Do NOT map TMJ width/height to VisualSprite_data or VisualEditor_data automatically
        // These should only be set via explicit component-scoped overrides (e.g., "VisualSprite_data.width")
    }
    
    // Now process existing overrides from JSON (explicit overrides take precedence)
    if (overrides.is_null())
        return;
    
    for (auto it = overrides.begin(); it != overrides.end(); ++it)
    {
        const std::string& key = it.key();
        const auto& value = it.value();
        
        // Skip TMJ metadata fields that were already processed
        if (key == "width" || key == "height" || key == "x" || key == "y" || key == "rotation" || key == "visible")
        {
            // These were handled in automatic TMJ mapping above
            // Don't store them as flat properties to avoid confusion
            continue;
        }
        
        // Check if this is a component-scoped override (nested object)
        // Example: overrides["Transform"] = {"width": 32, "height": 64}
        if (value.is_object())
        {
            // This is a component-level override - extract all parameters
            std::map<std::string, ComponentParameter> componentParams;
            
            for (auto paramIt = value.begin(); paramIt != value.end(); ++paramIt)
            {
                const std::string& paramName = paramIt.key();
                const auto& paramValue = paramIt.value();
                
                // Use helper function for consistent conversion
                componentParams[paramName] = JsonValueToComponentParameter(paramValue);
            }
            
            // Merge with component-scoped overrides (explicit overrides take precedence)
            auto& targetParams = instanceParams.componentOverrides[key];
            for (const auto& pair : componentParams)
            {
                // Use insert to check existence and add in one operation
                targetParams.insert(pair);
            }
        }
        else
        {
            // This is a flat property - use helper for consistent conversion
            instanceParams.properties[key] = JsonValueToComponentParameter(value);
        }
    }
}

EntityID World::CreateMissingPrefabPlaceholder(
    const Olympe::Editor::EntityInstance& entityInstance,
    InstantiationResult::PassStats& stats)
{
    EntityID entity = World::Get().CreateEntity();
    if (entity == INVALID_ENTITY_ID)
    {
        stats.failed++;
        stats.failedObjects.push_back(entityInstance.name + " (type: " + entityInstance.type + ")");
        return INVALID_ENTITY_ID;
    }

    World::Get().AddComponent<Identity_data>(entity, entityInstance.name, entityInstance.type, entityInstance.type);
    World::Get().AddComponent<Position_data>(entity, entityInstance.position);
    
    // Add visual editor marker with red color for missing prefabs
    VisualEditor_data editorData;
    editorData.sprite = DataManager::Get().GetSprite("location-32.png", ".\\Resources\\Icons\\location-32.png");
    editorData.color = { 255, 0, 0, 255 };  // Bright red (RGBA)
    editorData.isVisible = true;
    if (editorData.sprite)
    {
        editorData.srcRect = { 0, 0, static_cast<float>(editorData.sprite->w), static_cast<float>(editorData.sprite->h) };
        editorData.hotSpot = Vector(editorData.srcRect.w / 2.0f, editorData.srcRect.h / 2.0f, 0.0f);
    }
    World::Get().AddComponent<VisualEditor_data>(entity, editorData);
    
    stats.successfullyCreated++;
    
    SYSTEM_LOG << "  /!\\  PLACEHOLDER: Created red marker for missing prefab '" 
               << entityInstance.type << "' (name: " << entityInstance.name 
               << ") at position: " << entityInstance.position << "\n";

    return entity;
}

std::string World::ExtractPrefabName(const std::string& prefabPath)
{
    std::string prefabName = prefabPath;
    
    // Remove path
    size_t lastSlash = prefabName.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        prefabName = prefabName.substr(lastSlash + 1);
    
    // Remove extension
    size_t lastDot = prefabName.find_last_of(".");
    if (lastDot != std::string::npos)
        prefabName = prefabName.substr(0, lastDot);
    
    return prefabName;
}

void World::RegisterPlayerEntity(EntityID entity)
{
    // Validate that the entity has required player components
    if (!HasComponent<PlayerBinding_data>(entity) || !HasComponent<Controller_data>(entity))
    {
        SYSTEM_LOG << "X World::RegisterPlayerEntity: Entity " << entity 
                   << " missing required player components (PlayerBinding_data, Controller_data)\n";
        return;
    }
    
    // Delegate to VideoGame for full player registration
    VideoGame::Get().RegisterLoadedPlayerEntity(entity);
}

//=============================================================================
// Isometric Origin Calculation (for tile/entity alignment)
//=============================================================================

void World::SetMapBounds(int minTileX, int minTileY, int maxTileX, int maxTileY, int chunkOriginX, int chunkOriginY)
{
    m_minTileX = minTileX;
    m_minTileY = minTileY;
    m_maxTileX = maxTileX;
    m_maxTileY = maxTileY;
    m_chunkOriginX = chunkOriginX;
    m_chunkOriginY = chunkOriginY;
    
    // Invalidate cached isometric origin (will be recalculated on next access)
    m_isometricOriginCached = false;
    
    SYSTEM_LOG << "[World] Map bounds set: tiles(" << minTileX << "," << minTileY 
               << ") to (" << maxTileX << "," << maxTileY << "), chunk origin: ("
               << chunkOriginX << "," << chunkOriginY << ")\n";
}

float World::GetIsometricOriginX() const
{
    // For isometric maps, calculate TMJ origin from the 4 map corners
    // using shared utility function to avoid code duplication
    // 
    // NOTE: This caching assumes single-threaded access (typical for game engines).
    // If multi-threaded access is needed, synchronization should be added.
    if (m_mapOrientation == "isometric")
    {
        // Cache both X and Y values together to avoid inconsistency
        if (!m_isometricOriginCached)
        {
            Olympe::Tiled::IsometricProjection::CalculateTMJOrigin(
                m_minTileX, m_minTileY, m_maxTileX, m_maxTileY,
                m_tileWidth, m_tileHeight,
                m_cachedIsometricOriginX, m_cachedIsometricOriginY);
            m_isometricOriginCached = true;
        }
        return m_cachedIsometricOriginX;
    }
    return 0.0f;
}

float World::GetIsometricOriginY() const
{
    // For isometric maps, calculate TMJ origin from the 4 map corners
    // (See GetIsometricOriginX for detailed explanation)
    // 
    // NOTE: This caching assumes single-threaded access (typical for game engines).
    // If multi-threaded access is needed, synchronization should be added.
    if (m_mapOrientation == "isometric")
    {
        // Cache both X and Y values together to avoid inconsistency
        if (!m_isometricOriginCached)
        {
            Olympe::Tiled::IsometricProjection::CalculateTMJOrigin(
                m_minTileX, m_minTileY, m_maxTileX, m_maxTileY,
                m_tileWidth, m_tileHeight,
                m_cachedIsometricOriginX, m_cachedIsometricOriginY);
            m_isometricOriginCached = true;
        }
        return m_cachedIsometricOriginY;
    }
    return 0.0f;
}
