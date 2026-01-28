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
#include "TiledLevelLoader/include/TilesetCache.h"
#include "TiledLevelLoader/include/TilesetParser.h"
#include "TiledLevelLoader/include/TiledStructures.h"
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
	
	// ✅ NOUVEAU : Précharger tous les prefabs AVANT de créer les systèmes
	SYSTEM_LOG << "\n";
	PrefabFactory::Get().PreloadAllPrefabs("Blueprints/EntityPrefab");
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
    Add_ECS_System(std::make_unique<UIRenderingSystem>());      // ✅ Pass 2: UI/HUD/Menu (ALWAYS on top)
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
    
    // Step 1: Reuse PrefabFactory's cached registry if available
    SYSTEM_LOG << "-> Step 1: Loading prefab registry...\n";
    result.prefabRegistry = PrefabFactory::Get().GetPrefabRegistry();
    
    if (result.prefabRegistry.GetCount() == 0)
    {
        SYSTEM_LOG << "  ⚠️  PrefabFactory registry is empty, scanning now...\n";
        PrefabScanner scanner;
        std::vector<PrefabBlueprint> blueprints = scanner.ScanDirectory("Blueprints/EntityPrefab");
        
        if (blueprints.empty())
        {
            SYSTEM_LOG << "  /!\ No prefabs found in directory\n";
            result.errors.push_back("No prefabs found in Blueprints/EntityPrefab");
        }
        
        // Build the registry from blueprints
        for (const auto& blueprint : blueprints)
        {
            result.prefabRegistry.Register(blueprint);
            result.stats.prefabsLoaded++;
        }
    }
    else
    {
        SYSTEM_LOG << "  ✅ Using " << result.prefabRegistry.GetCount() 
                   << " prefabs from PrefabFactory cache\n";
        result.stats.prefabsLoaded = result.prefabRegistry.GetCount();
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
                
                SYSTEM_LOG << "[TilesetManager] ========================================\n";
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
                            SYSTEM_LOG << "[TilesetManager] ✓ Loaded atlas texture: " << filename << "\n";
                            SYSTEM_LOG << "[TilesetManager] Image-based tileset - All " 
                                      << tilecount << " tiles will use offset (" 
                                      << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
                            SYSTEM_LOG << "[TilesetManager] ========================================\n";
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
                    
                    SYSTEM_LOG << "[TilesetManager] ✓ Loaded collection tileset: " << info.name 
                              << " (" << info.individualTiles.size() << " tiles)\n";
                    SYSTEM_LOG << "[TilesetManager] All tiles stored with global offset: (" 
                              << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
                    SYSTEM_LOG << "[TilesetManager] ========================================\n";
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
                        SYSTEM_LOG << "  ✓ Loaded embedded tileset texture: " << filename 
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
                
                SYSTEM_LOG << "  ✓ Loaded embedded collection tileset: " << info.name 
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
            SYSTEM_LOG << "[TilesetManager] ❌ ERROR: Offset LOST during vector storage!\n";
            SYSTEM_LOG << "                  Expected: (" << info.tileoffsetX << ", " << info.tileoffsetY << ")\n";
            SYSTEM_LOG << "                  Got: (" << storedInfo.tileoffsetX << ", " << storedInfo.tileoffsetY << ")\n";
        }
        else if (info.tileoffsetX != 0 || info.tileoffsetY != 0)
        {
            SYSTEM_LOG << "[TilesetManager] ✓ POST-INSERT verification: offset=(" 
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
    
    SYSTEM_LOG << "\n[TilesetManager] ========================================\n";
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
    SYSTEM_LOG << "[TilesetManager] ========================================\n\n";
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
        
        // ✅ CRITICAL: Set the tileset pointer BEFORE any return
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
    SYSTEM_LOG << "[TilesetManager::GetTileTexture] ❌ GID " << cleanGid 
              << " NOT FOUND in any tileset (total tilesets: " << m_tilesets.size() << ")\n";
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
            // Use position directly - already a Vector, no conversion needed
            AddComponent<Position_data>(eid, entityInstance->position);
            
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
                entityInstance->position.x,
                entityInstance->position.y,
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
            // Use position directly - already a Vector, no conversion needed
            AddComponent<Position_data>(eid, entityInstance->position);
            
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
    
    // ✅ ADD DEBUG LOGGING
    SYSTEM_LOG << "[DEBUG] Pass 3 Entry:\n";
    SYSTEM_LOG << "  Total entities in levelDef: " << levelDef.entities.size() << "\n";
    SYSTEM_LOG << "  Categorized static objects: " << levelDef.categorizedObjects.staticObjects.size() << "\n";
    SYSTEM_LOG << "  Categorized dynamic objects: " << levelDef.categorizedObjects.dynamicObjects.size() << "\n";
    
    // ✅ USE CATEGORIZED STATIC OBJECTS (no manual filtering needed)
    for (const auto& entityInstance : levelDef.categorizedObjects.staticObjects)
    {
        if (!entityInstance) continue;
        
        SYSTEM_LOG << "  [DEBUG] Processing static object: " << entityInstance->name 
                   << " (type: " << entityInstance->type << ")\n";
        
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
            // Fallback: create basic entity with red placeholder for missing prefab
            entity = CreateEntity();
            AddComponent<Identity_data>(entity, entityInstance->name, entityInstance->type, entityInstance->type);
            
            // Use position directly - already a Vector, no conversion needed
            AddComponent<Position_data>(entity, entityInstance->position);
            
            // Add visual editor marker with red color for missing prefabs
            VisualEditor_data editorData;
            editorData.sprite = DataManager::Get().GetSprite("location-32.png", ".\\Resources\\Icons\\location-32.png");
            editorData.color = { 255, 0, 0, 255 };  // Bright red (RGBA)
            editorData.isVisible = true;
            if (editorData.sprite) {
                editorData.srcRect = { 0, 0, static_cast<float>(editorData.sprite->w), static_cast<float>(editorData.sprite->h) };
                editorData.hotSpot = Vector(editorData.srcRect.w / 2.0f, editorData.srcRect.h / 2.0f, 0.0f);
            }
            AddComponent<VisualEditor_data>(entity, editorData);
            
            SYSTEM_LOG << "  /!\  PLACEHOLDER: Created red marker for missing prefab '" 
                       << entityInstance->type << "' (name: " << entityInstance->name 
                       << ") at position: " << entityInstance->position << "\n";
        }
        
        if (entity != INVALID_ENTITY_ID)
        {
            if (HasComponent<Position_data>(entity))
            {
                // Use position directly - already a Vector, no conversion needed
                GetComponent<Position_data>(entity).position = entityInstance->position;
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
    
    // ✅ ADD DEBUG LOGGING
    SYSTEM_LOG << "[DEBUG] Pass 4 Entry - Analyzing categorized objects:\n";
    SYSTEM_LOG << "  Total entities in levelDef: " << levelDef.entities.size() << "\n";
    SYSTEM_LOG << "  Categorized dynamic objects: " << levelDef.categorizedObjects.dynamicObjects.size() << "\n";
    SYSTEM_LOG << "  Categorized static objects: " << levelDef.categorizedObjects.staticObjects.size() << "\n";
    SYSTEM_LOG << "  Categorized patrol paths: " << levelDef.categorizedObjects.patrolPaths.size() << "\n";
    
    if (!levelDef.categorizedObjects.dynamicObjects.empty()) {
        SYSTEM_LOG << "\n[DEBUG] Dynamic objects to instantiate:\n";
        for (const auto& entity : levelDef.categorizedObjects.dynamicObjects) {
            if (entity) {
                SYSTEM_LOG << "  - " << entity->name << " (type: " << entity->type << ")\n";
            }
        }
    } else {
        SYSTEM_LOG << "  ⚠️ WARNING: No dynamic objects in categorized list!\n";
    }
    SYSTEM_LOG << "\n";
    
    PrefabFactory& factory = PrefabFactory::Get();
    factory.SetPrefabRegistry(phase2Result.prefabRegistry);
    
    ParameterResolver resolver;
    
    // ✅ USE CATEGORIZED DYNAMIC OBJECTS (no manual filtering needed)
    for (const auto& entityInstance : levelDef.categorizedObjects.dynamicObjects)
    {
        if (!entityInstance) continue;
        
        result.pass4_dynamicObjects.totalObjects++;
        
        SYSTEM_LOG << "  [DEBUG] Processing dynamic object: " << entityInstance->name 
                   << " (type: " << entityInstance->type << ")\n";
        
        // Find prefab blueprint by type
        std::vector<const PrefabBlueprint*> blueprints = phase2Result.prefabRegistry.FindByType(entityInstance->type);
        
        if (blueprints.empty())
        {
            // Create red placeholder for missing prefab
            EntityID entity = CreateEntity();
            if (entity != INVALID_ENTITY_ID)
            {
                AddComponent<Identity_data>(entity, entityInstance->name, entityInstance->type, entityInstance->type);
                
                // Use position directly - already a Vector, no conversion needed
                AddComponent<Position_data>(entity, entityInstance->position);
                
                // Add visual editor marker with red color for missing prefabs
                VisualEditor_data editorData;
                editorData.sprite = DataManager::Get().GetSprite("location-32.png", ".\\Resources\\Icons\\location-32.png");
                editorData.color = { 255, 0, 0, 255 };  // Bright red (RGBA)
                editorData.isVisible = true;
                if (editorData.sprite) {
                    editorData.srcRect = { 0, 0, static_cast<float>(editorData.sprite->w), static_cast<float>(editorData.sprite->h) };
                    editorData.hotSpot = Vector(editorData.srcRect.w / 2.0f, editorData.srcRect.h / 2.0f, 0.0f);
                }
                AddComponent<VisualEditor_data>(entity, editorData);
                
                result.pass4_dynamicObjects.successfullyCreated++;
                result.entityRegistry[entityInstance->name] = entity;
                
                SYSTEM_LOG << "  /!\  PLACEHOLDER: Created red marker for missing prefab '" 
                           << entityInstance->type << "' (name: " << entityInstance->name 
                           << ") at position: " << entityInstance->position << "\n";
            }
            else
            {
                result.pass4_dynamicObjects.failed++;
                result.pass4_dynamicObjects.failedObjects.push_back(entityInstance->name + " (type: " + entityInstance->type + ")");
                SYSTEM_LOG << "  x Failed: No prefab found for type '" << entityInstance->type 
                           << "' (instance: " << entityInstance->name << ") and couldn't create placeholder\n";
            }
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
        // Use position directly - already a Vector, no conversion needed
        instanceParams.position = entityInstance->position;
        
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
        
        // ✅ CRITICAL: Override position INCLUDING z component (zOrder) to preserve layer depth
        if (HasComponent<Position_data>(entity)) {
            auto& pos = GetComponent<Position_data>(entity);
            pos.position = entityInstance->position;  // Includes x, y, AND z!
            SYSTEM_LOG << "    -> Position: (" << pos.position.x << ", " << pos.position.y 
                       << ", " << pos.position.z << ") [zOrder preserved]\n";
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
                    
                    // Copy patrol points from path to guard
                    guardBlackboard.patrolPointCount = pathData.patrolPointCount;
                    for (int i = 0; i < pathData.patrolPointCount && i < 8; ++i) {
                        guardBlackboard.patrolPoints[i] = pathData.patrolPoints[i];
                    }
                    guardBlackboard.currentPatrolIndex = 0;
                    guardBlackboard.hasPatrolPath = true;
                    
                    SYSTEM_LOG << "  ✓ Linked guard '" << link.sourceObjectName 
                               << "' → patrol '" << link.targetObjectName 
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
    
    SYSTEM_LOG << "  ✓ Created " << linksCreated << " object relationships\n";
    return true;
}
