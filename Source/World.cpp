/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

World purpose: Manage the lifecycle of Entities and their interaction with ECS Systems.

*/
#pragma once
#include "World.h"
#include "InputsManager.h"
#include "system/ViewportManager.h"
#include "ECS_Systems_AI.h"
#include "BlueprintEditor/WorldBridge.h"
#include "TiledLevelLoader/include/ParallaxLayerManager.h"
#include "TiledLevelLoader/include/LevelParser.h"
#include "TiledLevelLoader/include/TiledToOlympe.h"
#include "TiledLevelLoader/include/TiledDecoder.h"
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
#include <iostream>
#include <iomanip>

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
	
    Add_ECS_System(std::make_unique<AISystem>());
    Add_ECS_System(std::make_unique<DetectionSystem>());
    Add_ECS_System(std::make_unique<PhysicsSystem>());
    Add_ECS_System(std::make_unique<CollisionSystem>());
	Add_ECS_System(std::make_unique<TriggerSystem>());
	Add_ECS_System(std::make_unique<MovementSystem>());
    
    // Camera System (manages ECS cameras - added before rendering)
    Add_ECS_System(std::make_unique<CameraSystem>());
	Add_ECS_System(std::make_unique<GridSystem>()); 
    Add_ECS_System(std::make_unique<RenderingSystem>());
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
// Tiled MapEditor Integration
//---------------------------------------------------------------------------------------------
#include "TiledLevelLoader/include/TiledLevelLoader.h"
#include "TiledLevelLoader/include/TiledToOlympe.h"
#include "OlympeTilemapEditor/include/LevelManager.h"
#include "prefabfactory.h"
#include "ECS_Components_AI.h"
#include <fstream>

bool World::LoadLevelFromTiled(const std::string& tiledMapPath)
{
    Olympe::Tiled::LevelParser parser;
    Olympe::Tiled::LevelParseResult phase1Result;
    Phase2Result phase2Result;


    // Unload current level
    UnloadCurrentLevel();

    std::cout << "\n";
    std::cout << "/======================================================================\ \n";
    std::cout << "|         PHASE 1: PARSING & VISUAL ANALYSIS                           | \n";
    std::cout << "\======================================================================/ \n\n";
    
    // =======================================================================
    // PHASE 1: PARSING & VISUAL ANALYSIS
    // =======================================================================
    
    phase1Result = parser.ParseAndAnalyze(tiledMapPath);
    
    if (!phase1Result.IsSuccess())
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Phase 1 failed\n";
        for (const auto& error : phase1Result.errors)
        {
            SYSTEM_LOG << "  ERROR: " << error << "\n";
        }
        return false;
    }
    
    // =======================================================================
    // PHASE 2: PREFAB DISCOVERY & PRELOADING
    // =======================================================================
    
    /*Phase2Result*/
    phase2Result = ExecutePhase2_PrefabDiscovery(phase1Result);
    
    if (!phase2Result.success)
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Phase 2 failed (non-critical, continuing)\n";
    }
    
    // =======================================================================
    // PHASE 3: INSTANTIATION (5-Pass Pipeline)
    // =======================================================================
    
    std::cout << "\n";
    std::cout << "/======================================================================\ \n";
    std::cout << "|         PHASE 3: INSTANTIATION (5-Pass Pipeline)                     | \n";
    std::cout << "\======================================================================/ \n\n";
    
    // Load and convert using existing TiledToOlympe
    Olympe::Tiled::TiledMap tiledMap;
    Olympe::Tiled::TiledLevelLoader loader;
    
    if (!loader.LoadFromFile(tiledMapPath, tiledMap))
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Failed to reload map for conversion\n";
        return false;
    }
    
    Olympe::Tiled::TiledToOlympe converter;
    //std::string mappingPath = "Config/tiled_prefab_mapping.json";
    //converter.LoadPrefabMapping(mappingPath);
    
    Olympe::Editor::LevelDefinition levelDef;
    if (!converter.Convert(tiledMap, levelDef))
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Failed to convert map\n";
        return false;
    }/**/

	// Clean and standardize Object.type following prefab registry
    PrefabFactory& factory = PrefabFactory::Get();
    factory.SetPrefabRegistry(phase2Result.prefabRegistry);
	for (const auto& entity : levelDef.entities)
    {
		entity->type = parser.ObjectTypeChecker(entity->type);
    }
            
    // Execute 5-pass instantiation
    InstantiationResult instResult;
    
    std::cout << "-> Pass 1: Visual Layers (Parallax)\n";
    InstantiatePass1_VisualLayers(levelDef, instResult);
    
    std::cout << "-> Pass 2: Spatial Structure (Sectors, Collision)\n";
    InstantiatePass2_SpatialStructure(levelDef, instResult);
    
    std::cout << "-> Pass 3: Static Objects (Items, Waypoints)\n";
    InstantiatePass3_StaticObjects(levelDef, instResult);
    
    std::cout << "-> Pass 4: Dynamic Objects (Player, NPCs, Enemies)\n";
    InstantiatePass4_DynamicObjects(levelDef, phase2Result, instResult);
    
    std::cout << "-> Pass 5: Relationships (Patrol Paths, AI Links)\n";

    InstantiatePass5_Relationships(levelDef, instResult);
    
    instResult.success = true;
    
    // Final summary
    std::cout << "\n";
    std::cout << "/======================================================================\ \n";
    std::cout << "| LEVEL LOADING COMPLETE                                               | \n";
    std::cout << "|======================================================================| \n";
    std::cout << "| Phase 1: -> Parse & Analysis                                          | \n";
    std::cout << "| Phase 2: " << (phase2Result.success ? "->" : "⊙") << " Prefab Discovery & Preload"
              << std::string(37, ' ') << "| \n";
    std::cout << "| Phase 3: -> Instantiation Complete                                    | \n";
    std::cout << "|                                                                      | \n";
    std::cout << "| Entities Created: " << instResult.GetTotalCreated()
              << std::string(std::max(0, 48 - static_cast<int>(std::to_string(instResult.GetTotalCreated()).length())), ' ') << "| \n";
    std::cout << "| Entities Failed:  " << instResult.GetTotalFailed()
              << std::string(std::max(0, 48 - static_cast<int>(std::to_string(instResult.GetTotalFailed()).length())), ' ') << "| \n";
    std::cout << "\======================================================================/ \n\n";
    
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
// Phase 2: Prefab Discovery & Preloading Implementation
//=============================================================================

World::Phase2Result World::ExecutePhase2_PrefabDiscovery(const Olympe::Tiled::LevelParseResult& phase1Result)
{
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\ \n";
    SYSTEM_LOG << "|         PHASE 2: PREFAB DISCOVERY & PRELOADING                       | \n";
    SYSTEM_LOG << "\\======================================================================/ \n\n";
    
    Phase2Result result;
    
    // Step 1: Scan prefab directory
    SYSTEM_LOG << "-> Step 1: Scanning prefab directory...\n";
    PrefabScanner scanner;
    std::vector<PrefabBlueprint> blueprints = scanner.ScanDirectory("GameData\\EntityPrefab");
    
    if (blueprints.empty())
    {
        SYSTEM_LOG << "  /!\ No prefabs found in directory\n";
        result.errors.push_back("No prefabs found in GameData\\EntityPrefab");
    }
    
    // Build the registry from blueprints
    for (const auto& blueprint : blueprints)
    {
        result.prefabRegistry.Register(blueprint);
        result.stats.prefabsLoaded++;
    }
    
    SYSTEM_LOG << "  -> Loaded " << result.stats.prefabsLoaded << " prefab blueprints\n";
    
    // Step 2: Cross-check level requirements vs available prefabs
    SYSTEM_LOG << "\n-> Step 2: Cross-checking level requirements...\n";
    
    for (const auto& type : phase1Result.objectCensus.uniqueTypes)
    {
        std::vector<const PrefabBlueprint*> blueprints = result.prefabRegistry.FindByType(type);
        
        if (blueprints.empty())
        {
            SYSTEM_LOG << "  x Missing prefab for type: " << type << "\n";
            result.missingPrefabs.push_back(type);
        }
        else
        {
            SYSTEM_LOG << "  -> Found prefab: " << blueprints[0]->prefabName << " (type: " << type << ")\n";
        }
    }
    
    if (!result.missingPrefabs.empty())
    {
        SYSTEM_LOG << "  /!\ " << result.missingPrefabs.size() << " missing prefabs detected\n";
    }
    
    // Step 3: Preload visual resources
    SYSTEM_LOG << "\n-> Step 3: Preloading visual resources...\n";
    DataManager& dataManager = DataManager::Get();
    
    // Preload tilesets
    std::vector<DataManager::TilesetInfo> tilesets;
    for (const auto& tilesetRef : phase1Result.visualManifest.tilesets)
    {
        DataManager::TilesetInfo info;
        info.sourceFile = tilesetRef.sourceFile;
        info.imageFile = tilesetRef.imageFile;
        info.individualImages = tilesetRef.individualImages;
        info.isCollection = tilesetRef.isCollection;
        tilesets.push_back(info);
    }
    result.preloadResult.tilesets = dataManager.PreloadTilesets(tilesets, true);
    
    // Preload parallax layers
    std::vector<std::string> parallaxPaths(
        phase1Result.visualManifest.parallaxLayers.begin(),
        phase1Result.visualManifest.parallaxLayers.end()
    );
    result.preloadResult.textures = dataManager.PreloadTextures(parallaxPaths, ResourceCategory::Level, true);
    
    // Step 4: Preload prefab resources (sprites and audio)
    SYSTEM_LOG << "\n-> Step 4: Preloading prefab resources...\n";
    
    std::vector<std::string> spritePaths;
    std::vector<std::string> audioPaths;
    
    for (const auto& name : result.prefabRegistry.GetAllPrefabNames())
    {
        const PrefabBlueprint* blueprint = result.prefabRegistry.Find(name);
        if (blueprint)
        {
            for (const auto& spritePath : blueprint->resources.spriteRefs)
            {
                spritePaths.push_back(spritePath);
            }
            for (const auto& audioPath : blueprint->resources.audioRefs)
            {
                audioPaths.push_back(audioPath);
            }
        }
    }
    
    if (!spritePaths.empty())
    {
        result.preloadResult.sprites = dataManager.PreloadSprites(spritePaths, ResourceCategory::GameEntity, true);
        result.stats.spritesPreloaded = result.preloadResult.sprites.totalRequested;
        SYSTEM_LOG << "  -> Preloaded " << result.preloadResult.sprites.successfullyLoaded 
                   << " sprites (" << result.preloadResult.sprites.completelyFailed << " failed)\n";
    }
    
    if (!audioPaths.empty())
    {
        result.preloadResult.audio = dataManager.PreloadAudioFiles(audioPaths, true);
        result.stats.audioPreloaded = result.preloadResult.audio.totalRequested;
        SYSTEM_LOG << "  -> Preloaded " << result.preloadResult.audio.successfullyLoaded 
                   << " audio files (" << result.preloadResult.audio.completelyFailed << " failed)\n";
    }
    
    result.preloadResult.success = result.preloadResult.IsComplete() || 
                                   result.preloadResult.GetTotalLoaded() > 0.5f;
    result.success = true;
    
    // Summary
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\ \n";
    SYSTEM_LOG << "| PHASE 2 COMPLETE                                                     | \n";
    SYSTEM_LOG << "|======================================================================| \n";
    SYSTEM_LOG << "| Prefabs Loaded:      " << std::setw(3) << result.stats.prefabsLoaded
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "| Missing Prefabs:     " << std::setw(3) << result.missingPrefabs.size()
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "| Sprites Preloaded:   " << std::setw(3) << result.stats.spritesPreloaded
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "| Audio Preloaded:     " << std::setw(3) << result.stats.audioPreloaded
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "| Resources Loaded:    " << std::setw(3) << result.preloadResult.GetTotalLoaded()
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "| Resources Failed:    " << std::setw(3) << result.preloadResult.GetTotalFailed()
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "\======================================================================/ \n";
    
    return result;
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
    
    std::cout << "-> Map configuration: " << m_mapOrientation 
               << " (" << m_tileWidth << "x" << m_tileHeight << ")\n";
    
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
    
    std::cout << "    ✓ Loaded chunk at (" << chunkX << ", " << chunkY 
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
        info.name = tilesetJson.value("name", "");
        info.tilewidth = tilesetJson.value("tilewidth", 32);
        info.tileheight = tilesetJson.value("tileheight", 32);
        info.columns = tilesetJson.value("columns", 0);
        info.imagewidth = tilesetJson.value("imagewidth", 0);
        info.imageheight = tilesetJson.value("imageheight", 0);
        info.margin = tilesetJson.value("margin", 0);
        info.spacing = tilesetJson.value("spacing", 0);
        
        // Parse tileoffset
        if (tilesetJson.contains("tileoffset"))
        {
            info.tileoffsetX = tilesetJson["tileoffset"].value("x", 0);
            info.tileoffsetY = tilesetJson["tileoffset"].value("y", 0);
        }
        
        uint32_t tilecount = tilesetJson.value("tilecount", 0);
        info.lastgid = info.firstgid + tilecount - 1;
        
        // Detect collection tileset by columns == 0
        info.isCollection = (info.columns == 0);
        
        if (!info.isCollection && tilesetJson.contains("image"))
        {
            // Image-based tileset
            std::string imagePath = tilesetJson["image"].get<std::string>();
            
            // Extract filename
            size_t lastSlash = imagePath.find_last_of("/\\");
            std::string filename = (lastSlash != std::string::npos) ? imagePath.substr(lastSlash + 1) : imagePath;
            
            // Find resource recursively
            std::string fullPath = DataManager::Get().FindResourceRecursive(filename);
            
            if (!fullPath.empty())
            {
                info.texture = IMG_LoadTexture(GameEngine::renderer, fullPath.c_str());
                if (info.texture)
                {
                    SYSTEM_LOG << "  ✓ Loaded tileset texture: " << filename << " (gid: " 
                              << info.firstgid << "-" << info.lastgid << ")\n";
                }
                else
                {
                    SYSTEM_LOG << "  x Failed to load tileset texture: " << fullPath << "\n";
                }
            }
        }
        else if (info.isCollection && tilesetJson.contains("tiles"))
        {
            // Collection tileset (individual tile images)
            const auto& tilesArray = tilesetJson["tiles"];
            
            for (const auto& tileJson : tilesArray)
            {
                uint32_t tileId = tileJson["id"].get<uint32_t>();
                std::string imagePath = tileJson["image"].get<std::string>();
                
                // Extract filename
                size_t lastSlash = imagePath.find_last_of("/\\");
                std::string filename = (lastSlash != std::string::npos) ? imagePath.substr(lastSlash + 1) : imagePath;
                
                // Find resource recursively
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
                        srcRect.w = tileJson.value("imagewidth", info.tilewidth);
                        srcRect.h = tileJson.value("imageheight", info.tileheight);
                        info.individualSrcRects[tileId] = srcRect;
                    }
                }
            }
            
            SYSTEM_LOG << "  ✓ Loaded collection tileset: " << info.name 
                      << " (" << info.individualTiles.size() << " tiles)\n";
        }
        
        m_tilesets.push_back(info);
    }
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
    
    // Find the tileset containing this GID
    for (const auto& tileset : m_tilesets)
    {
        if (cleanGid >= tileset.firstgid && cleanGid <= tileset.lastgid)
        {
            uint32_t localId = cleanGid - tileset.firstgid;
            
            // ✅ CRITICAL: Set the tileset pointer BEFORE any return
            outTileset = &tileset;
            
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
    }
    
    // GID not found in any tileset
    outTileset = nullptr;
    return false;
}

bool World::InstantiatePass2_SpatialStructure(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    // Create collision zones and sectors
    PrefabFactory& factory = PrefabFactory::Get();
    
    for (const auto& entityInstance : levelDef.entities)
    {
        if (!entityInstance) continue;
        
        // Improved: Use case-insensitive substring matching for collision/sector types
        std::string typeLower = entityInstance->type;
        std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
        
        bool isCollision = (typeLower.find("collision") != std::string::npos);
        bool isSector = (typeLower.find("sector") != std::string::npos || 
                        typeLower.find("zone") != std::string::npos);
        
        if (isCollision)
        {
            result.pass2_spatialStructure.totalObjects++;
            
            EntityID eid = CreateEntity();
            
            AddComponent<Identity_data>(eid, entityInstance->name, "Collision", entityInstance->type);
            AddComponent<Position_data>(eid, Vector(
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                0.0f
            ));
            
            float width = 64.0f;
            float height = 64.0f;
            float rotation = entityInstance->rotation;
            
            if (!entityInstance->overrides.is_null())
            {
                if (entityInstance->overrides.contains("width"))
                    width = entityInstance->overrides["width"].get<float>();
                if (entityInstance->overrides.contains("height"))
                    height = entityInstance->overrides["height"].get<float>();
            }
            
            // Handle polygon collision if present
            if (entityInstance->overrides.contains("CollisionPolygon") &&
                entityInstance->overrides["CollisionPolygon"].contains("points"))
            {
                // TODO: Create polygon collision component when available
                // For now, create bounding box as fallback
                std::cout << "  -> Created collision polygon (using bbox fallback): " << entityInstance->name << "\n";
            }
            
            AddComponent<CollisionZone_data>(eid, SDL_FRect{
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                width, height
            }, true);
            
            result.pass2_spatialStructure.successfullyCreated++;
            result.entityRegistry[entityInstance->name] = eid;
            std::cout << "  -> Created collision zone: " << entityInstance->name << "\n";
        }
        else if (isSector)
        {
            // Add sector instantiation support
            result.pass2_spatialStructure.totalObjects++;
            
            EntityID eid = CreateEntity();
            AddComponent<Identity_data>(eid, entityInstance->name, "Sector", entityInstance->type);
            AddComponent<Position_data>(eid, Vector(
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                0.0f
            ));
            
            // TODO: Add SectorZone_data component when available
            
            result.pass2_spatialStructure.successfullyCreated++;
            result.sectors.push_back(eid);
            result.entityRegistry[entityInstance->name] = eid;
            std::cout << "  -> Created sector: " << entityInstance->name << "\n";
        }
    }
    
    return true;
}

bool World::InstantiatePass3_StaticObjects(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    // Create items, waypoints, and other static objects
    PrefabFactory& factory = PrefabFactory::Get();
    
    for (const auto& entityInstance : levelDef.entities)
    {
        if (!entityInstance) continue;
        
        // Skip collision/sectors (handled in Pass 2)
        std::string typeLower = entityInstance->type;
        std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
        
        if (typeLower.find("collision") != std::string::npos ||
            typeLower.find("sector") != std::string::npos ||
            typeLower.find("zone") != std::string::npos)
        {
            continue;
        }
        
        // Extensible static type list
        std::vector<std::string> staticTypes = {
            "item", "waypoint", "trigger", "spawn",
            "key", "door", "exit", "pickup", "interactable",
            "checkpoint", "portal", "teleporter", "switch"
        };
        
        // Check if it's a dynamic type (will be handled in Pass 4)
        std::vector<std::string> dynamicTypes = {
            "player", "npc", "guard", "enemy", "zombie", "ambiant"
        };
        
        bool isDynamic = false;
        for (const auto& dynamicType : dynamicTypes)
        {
            if (typeLower == dynamicType)
            {
                isDynamic = true;
                break;
            }
        }
        
        if (isDynamic) continue;
        
        // Check if it's a static type
        bool isStatic = false;
        for (const auto& staticType : staticTypes)
        {
            if (typeLower == staticType)
            {
                isStatic = true;
                break;
            }
        }
        
        if (!isStatic) continue;
        
        result.pass3_staticObjects.totalObjects++;
        
        std::string prefabName = entityInstance->prefabPath;
        size_t lastSlash = prefabName.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            prefabName = prefabName.substr(lastSlash + 1);
        size_t lastDot = prefabName.find_last_of(".");
        if (lastDot != std::string::npos)
            prefabName = prefabName.substr(0, lastDot);
        
        EntityID entity = factory.CreateEntity(prefabName);
        if (entity == INVALID_ENTITY_ID)
        {
            // Fallback: create basic entity even if prefab missing
            entity = CreateEntity();
            AddComponent<Identity_data>(entity, entityInstance->name, entityInstance->type, entityInstance->type);
            AddComponent<Position_data>(entity, Vector(
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                0.0f
            ));
            
            std::cout << "  ! Created fallback entity (prefab missing): " << entityInstance->name << "\n";
        }
        
        if (entity != INVALID_ENTITY_ID)
        {
            if (HasComponent<Position_data>(entity))
            {
                Position_data& pos = GetComponent<Position_data>(entity);
                pos.position.x = static_cast<float>(entityInstance->position.x);
                pos.position.y = static_cast<float>(entityInstance->position.y);
            }

            if (HasComponent<Identity_data>(entity))
            {
                Identity_data& id = GetComponent<Identity_data>(entity);
                id.name = entityInstance->name;
                id.type = entityInstance->type;
			}

   //         if (HasComponent<VisualSprite_data>(entity))
   //         {
   //             VisualSprite_data& spriteComp = GetComponent<VisualSprite_data>(entity);
   //             spriteComp.sprite = DataManager::Get().GetSprite(entityInstance->spritePath, entityInstance->spritePath);
   //             spriteComp.UpdateRect();
			//}

   //         if (HasComponent<VisualEditor_data>(entity))
   //         {
   //             VisualEditor_data& editorComp = GetComponent<VisualEditor_data>(entity);
   //             editorComp.sprite = DataManager::Get().GetSprite(entityInstance->spritePath, entityInstance->spritePath);
   //             editorComp.UpdateRect();
   //         }
            
            result.pass3_staticObjects.successfullyCreated++;
            result.entityRegistry[entityInstance->name] = entity;
            std::cout << "  -> Created static object: " << entityInstance->name << "\n";
        }
        else
        {
            result.pass3_staticObjects.failed++;
            result.pass3_staticObjects.failedObjects.push_back(entityInstance->name);
        }
    }
    
    return true;
}

bool World::InstantiatePass4_DynamicObjects(
    const Olympe::Editor::LevelDefinition& levelDef,
    const Phase2Result& phase2Result,
    InstantiationResult& result)
{
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\ \n";
    SYSTEM_LOG << "| PASS 4: DYNAMIC OBJECTS (Prefab-Based Instantiation)                 | \n";
    SYSTEM_LOG << "\======================================================================/ \n\n";
    
    PrefabFactory& factory = PrefabFactory::Get();
    factory.SetPrefabRegistry(phase2Result.prefabRegistry);
    
    ParameterResolver resolver;
    
    std::vector<std::string> dynamicTypes = {
        "player", "npc", "guard", "enemy", "zombie", "trigger", "ambiant"
    };

    std::string instancetypeLower;
	std::string dynamicTypeLower;
    
    for (const auto& entityInstance : levelDef.entities)
    {
        if (!entityInstance) continue;
        
        // Filter: Only dynamic types
        bool isDynamic = false;
        for (const auto& dynamicType : dynamicTypes)
        {
			instancetypeLower = entityInstance->type;
            std::transform(instancetypeLower.begin(), instancetypeLower.end(), instancetypeLower.begin(), ::tolower);

			dynamicTypeLower = dynamicType;
			std::transform(dynamicTypeLower.begin(), dynamicTypeLower.end(), dynamicTypeLower.begin(), ::tolower);

            if (instancetypeLower == dynamicTypeLower)
            {
                isDynamic = true;
                break;
            }
        }
        
        if (!isDynamic) continue;
        
        result.pass4_dynamicObjects.totalObjects++;
        
        // Find prefab blueprint by type
        std::vector<const PrefabBlueprint*> blueprints = phase2Result.prefabRegistry.FindByType(entityInstance->type);
        
        if (blueprints.empty())
        {
            result.pass4_dynamicObjects.failed++;
            result.pass4_dynamicObjects.failedObjects.push_back(entityInstance->name + " (type: " + entityInstance->type + ")");
            SYSTEM_LOG << "  x Failed: No prefab found for type '" << entityInstance->type 
                       << "' (instance: " << entityInstance->name << ")\n";
            continue;
        }
        
        const PrefabBlueprint* blueprint = blueprints[0];
        
        SYSTEM_LOG << "  -> Creating: " << entityInstance->name << " [" << blueprint->prefabName << "]\n";
        
        // Create entity
        EntityID entity = CreateEntity();
        if (entity == INVALID_ENTITY_ID)
        {
            result.pass4_dynamicObjects.failed++;
            result.pass4_dynamicObjects.failedObjects.push_back(entityInstance->name);
            SYSTEM_LOG << "    x Failed to create entity ID\n";
            continue;
        }
        
        // Build level instance parameters
        LevelInstanceParameters instanceParams(entityInstance->name, entityInstance->type);
        instanceParams.position = Vector(
            static_cast<float>(entityInstance->position.x),
            static_cast<float>(entityInstance->position.y),
            0.0f
        );
        
        // Extract custom properties from level instance
        if (!entityInstance->overrides.is_null())
        {
            for (auto it = entityInstance->overrides.begin(); it != entityInstance->overrides.end(); ++it)
            {
                ComponentParameter param;
                const auto& value = it.value();
                
                if (value.is_number_float())
                {
                    param.type = ComponentParameter::Type::Float;
                    param.floatValue = value.get<float>();
                }
                else if (value.is_number_integer())
                {
                    param.type = ComponentParameter::Type::Int;
                    param.intValue = value.get<int>();
                }
                else if (value.is_boolean())
                {
                    param.type = ComponentParameter::Type::Bool;
                    param.boolValue = value.get<bool>();
                }
                else if (value.is_string())
                {
                    param.type = ComponentParameter::Type::String;
                    param.stringValue = value.get<std::string>();
                }
                
                instanceParams.properties[it.key()] = param;
            }
        }
        
        // Resolve components using ParameterResolver
        std::vector<ResolvedComponentInstance> resolvedComponents = resolver.Resolve(*blueprint, instanceParams);
        
        // Instantiate components
        int componentCount = 0;
        int failedComponents = 0;
        
        for (const auto& resolved : resolvedComponents)
        {
            if (!resolved.isValid)
            {
                failedComponents++;
                SYSTEM_LOG << "    /!\ Invalid resolved component: " << resolved.componentType << "\n";
                continue;
            }
            
            ComponentDefinition compDef;
            compDef.componentType = resolved.componentType;
            compDef.parameters = resolved.parameters;
            
            if (factory.InstantiateComponent(entity, compDef))
            {
                componentCount++;
            }
            else
            {
                failedComponents++;
                SYSTEM_LOG << "    /!\ Failed to instantiate component: " << resolved.componentType << "\n";
            }
        }
        
        if (failedComponents > 0)
        {
            SYSTEM_LOG << "    /!\ Created with " << componentCount << " components (" 
                       << failedComponents << " failed)\n";
        }
        else
        {
            SYSTEM_LOG << "    -> Created with " << componentCount << " components\n";
        }
        
        result.pass4_dynamicObjects.successfullyCreated++;
        result.entityRegistry[entityInstance->name] = entity;
    }
    
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\ \n";
    SYSTEM_LOG << "| PASS 4 COMPLETE                                                      | \n";
    SYSTEM_LOG << "|======================================================================| \n";
    SYSTEM_LOG << "| Total Objects:       " << std::setw(3) << result.pass4_dynamicObjects.totalObjects
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "| Successfully Created: " << std::setw(3) << result.pass4_dynamicObjects.successfullyCreated
               << std::string(44, ' ') << "| \n";
    SYSTEM_LOG << "| Failed:              " << std::setw(3) << result.pass4_dynamicObjects.failed
               << std::string(45, ' ') << "| \n";
    SYSTEM_LOG << "\======================================================================/ \n";
    
    return result.pass4_dynamicObjects.IsSuccess();
}

bool World::InstantiatePass5_Relationships(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    // Assign patrol paths and other relationships
    for (const auto& entityInstance : levelDef.entities)
    {
        if (!entityInstance) continue;
        
        // Look for entity in registry
        auto it = result.entityRegistry.find(entityInstance->name);
        if (it == result.entityRegistry.end()) continue;
        
        EntityID entity = it->second;
        
        // Handle patrol paths
        if (!entityInstance->overrides.is_null() && entityInstance->overrides.contains("patrolPath"))
        {
            if (HasComponent<AIBlackboard_data>(entity))
            {
                result.pass5_relationships.totalObjects++;
                
                AIBlackboard_data& blackboard = GetComponent<AIBlackboard_data>(entity);
                const auto& patrolPath = entityInstance->overrides["patrolPath"];
                
                if (patrolPath.is_array())
                {
                    blackboard.patrolPointCount = 0;
                    for (size_t i = 0; i < patrolPath.size() && i < 8; ++i)
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
    
    return true;
}
