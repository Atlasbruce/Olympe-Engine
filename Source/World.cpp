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
#include "PrefabScanner.h"
#include "prefabfactory.h"
#include "DataManager.h"
#include "GameEngine.h"
#include "../SDL/include/SDL3_image/SDL_image.h"
#include <chrono>
#include <iostream>

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
    // Unload current level
    UnloadCurrentLevel();

    std::cout << "\n";
    std::cout << "/======================================================================\\n";
    std::cout << "|         PHASE 1: PARSING & VISUAL ANALYSIS                           |\n";
    std::cout << "\======================================================================/\n";
    
    // =======================================================================
    // PHASE 1: PARSING & VISUAL ANALYSIS
    // =======================================================================
    
    Olympe::Tiled::LevelParser parser;
    Olympe::Tiled::LevelParseResult phase1Result = parser.ParseAndAnalyze(tiledMapPath);
    
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
    
    Phase2Result phase2Result = ExecutePhase2(phase1Result);
    
    if (!phase2Result.success)
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Phase 2 failed (non-critical, continuing)\n";
    }
    
    // =======================================================================
    // PHASE 3: INSTANTIATION (5-Pass Pipeline)
    // =======================================================================
    
    std::cout << "\n";
    std::cout << "/======================================================================\\n";
    std::cout << "|         PHASE 3: INSTANTIATION (5-Pass Pipeline)                     |\n";
    std::cout << "\======================================================================/\n\n";
    
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
        
    // Execute 5-pass instantiation
    InstantiationResult instResult;
    
    std::cout << "→ Pass 1: Visual Layers (Parallax)\n";
    InstantiatePass1_VisualLayers(levelDef, instResult);
    
    std::cout << "→ Pass 2: Spatial Structure (Sectors, Collision)\n";
    InstantiatePass2_SpatialStructure(levelDef, instResult);
    
    std::cout << "→ Pass 3: Static Objects (Items, Waypoints)\n";
    InstantiatePass3_StaticObjects(levelDef, instResult);
    
    std::cout << "→ Pass 4: Dynamic Objects (Player, NPCs, Enemies)\n";
    InstantiatePass4_DynamicObjects(levelDef, instResult);
    
    std::cout << "→ Pass 5: Relationships (Patrol Paths, AI Links)\n";
    InstantiatePass5_Relationships(levelDef, instResult);
    
    instResult.success = true;
    
    // Final summary
    std::cout << "\n";
    std::cout << "/======================================================================\\n";
    std::cout << "| LEVEL LOADING COMPLETE                                               |\n";
    std::cout << "|======================================================================|\n";
    std::cout << "| Phase 1: ✓ Parse & Analysis                                          |\n";
    std::cout << "| Phase 2: " << (phase2Result.success ? "✓" : "⊙") << " Prefab Discovery & Preload"
              << std::string(37, ' ') << "|\n";
    std::cout << "| Phase 3: ✓ Instantiation Complete                                    |\n";
    std::cout << "|                                                                      |\n";
    std::cout << "| Entities Created: " << instResult.GetTotalCreated()
              << std::string(std::max(0, 48 - static_cast<int>(std::to_string(instResult.GetTotalCreated()).length())), ' ') << "|\n";
    std::cout << "| Entities Failed:  " << instResult.GetTotalFailed()
              << std::string(std::max(0, 48 - static_cast<int>(std::to_string(instResult.GetTotalFailed()).length())), ' ') << "|\n";
    std::cout << "\======================================================================/\n\n";
    
    SYSTEM_LOG << "World::LoadLevelFromTiled - Level loaded successfully\n";
    return true;
}

void World::UnloadCurrentLevel()
{
    SYSTEM_LOG << "World::UnloadCurrentLevel - Unloading current level\n";
    
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

World::Phase2Result World::ExecutePhase2(const Olympe::Tiled::LevelParseResult& phase1Result)
{
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\\\n";
    SYSTEM_LOG << "|         PHASE 2: PREFAB DISCOVERY & PRELOADING                       |\n";
    SYSTEM_LOG << "\\======================================================================/\n\n";
    
    Phase2Result result;
    
    // Step 1: Scan prefab directory
    SYSTEM_LOG << "→ Scanning prefab directory...\n";
    PrefabScanner scanner;
    std::vector<PrefabBlueprint> blueprints = scanner.ScanDirectory("GameData\\EntityPrefab");
    
    // Build the registry from blueprints
    for (const auto& blueprint : blueprints)
    {
        result.prefabRegistry.Register(blueprint);
    }
    
    // Step 2: Cross-check level requirements vs available prefabs
    SYSTEM_LOG << "\n→ Cross-checking level requirements...\n";
    for (const auto& type : phase1Result.objectCensus.uniqueTypes)
    {
        if (!result.prefabRegistry.Find(type))
        {
            SYSTEM_LOG << "  ✗ Missing prefab: " << type << "\n";
        }
        else
        {
            SYSTEM_LOG << "  ✓ Found prefab: " << type << "\n";
        }
    }
    
    // Step 3: Preload visual resources
    SYSTEM_LOG << "\n→ Preloading visual resources...\n";
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
    
    // Preload prefab resources
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
    }
    
    if (!audioPaths.empty())
    {
        result.preloadResult.audio = dataManager.PreloadAudioFiles(audioPaths, true);
    }
    
    result.preloadResult.success = result.preloadResult.IsComplete() || 
                                   result.preloadResult.GetTotalLoaded() > 0.5f;
    result.success = true;
    
    // Summary
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\\\n";
    SYSTEM_LOG << "| PHASE 2 COMPLETE                                                     |\n";
    SYSTEM_LOG << "|======================================================================|\n";
    SYSTEM_LOG << "| Prefabs Found:    " << result.prefabRegistry.GetCount()
              << std::string(49 - std::to_string(result.prefabRegistry.GetCount()).length(), ' ') << "|\n";
    SYSTEM_LOG << "| Resources Loaded: " << result.preloadResult.GetTotalLoaded()
              << std::string(49 - std::to_string(result.preloadResult.GetTotalLoaded()).length(), ' ') << "|\n";
    SYSTEM_LOG << "| Resources Failed: " << result.preloadResult.GetTotalFailed()
              << std::string(49 - std::to_string(result.preloadResult.GetTotalFailed()).length(), ' ') << "|\n";
    SYSTEM_LOG << "\\======================================================================/\n";
    
    return result;
}

//=============================================================================
// Phase 3: Instantiation Pass Implementations
//=============================================================================

bool World::InstantiatePass1_VisualLayers(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    // Load parallax layers from metadata
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
                std::string imagePath = layerJson["imagePath"].get<std::string>();
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
                    std::cout << "  ✗ Failed to load parallax layer: " << imagePath << "\n";
                    continue;
                }
                
                Olympe::Tiled::ParallaxLayer layer;
                layer.name = layerJson["name"].get<std::string>();
                layer.imagePath = imagePath;
                layer.texture = texture;
                layer.scrollFactorX = layerJson["scrollFactorX"].get<float>();
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
                std::cout << "  ✓ Loaded parallax layer: " << layer.name << "\n";
            }
        }
    }
    
    return true;
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
        
        if (entityInstance->type == "collision")
        {
            result.pass2_spatialStructure.totalObjects++;
            
            EntityID eid = CreateEntity();
            
            AddComponent<Identity_data>(eid, entityInstance->name, "Collision", EntityType::Collision);
            AddComponent<Position_data>(eid, Vector(
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                0.0f
            ));
            
            float width = 64.0f;
            float height = 64.0f;
            if (!entityInstance->overrides.is_null())
            {
                if (entityInstance->overrides.contains("width"))
                    width = entityInstance->overrides["width"].get<float>();
                if (entityInstance->overrides.contains("height"))
                    height = entityInstance->overrides["height"].get<float>();
            }
            
            AddComponent<CollisionZone_data>(eid, SDL_FRect{
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                width, height
            }, true);
            
            result.pass2_spatialStructure.successfullyCreated++;
            result.entityRegistry[entityInstance->name] = eid;
            std::cout << "  ✓ Created collision zone: " << entityInstance->name << "\n";
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
        
        // Skip collision (handled in Pass 2)
        if (entityInstance->type == "collision") continue;
        
        // Check if it's a static object (items, waypoints, etc.)
        if (entityInstance->type == "item" || entityInstance->type == "waypoint" || 
            entityInstance->type == "trigger" || entityInstance->type == "spawn")
        {
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
                result.pass3_staticObjects.failed++;
                result.pass3_staticObjects.failedObjects.push_back(prefabName);
                std::cout << "  ✗ Failed to create static object: " << prefabName << "\n";
                continue;
            }
            
            if (HasComponent<Position_data>(entity))
            {
                Position_data& pos = GetComponent<Position_data>(entity);
                pos.position.x = static_cast<float>(entityInstance->position.x);
                pos.position.y = static_cast<float>(entityInstance->position.y);
            }
            
            result.pass3_staticObjects.successfullyCreated++;
            result.entityRegistry[entityInstance->name] = entity;
            std::cout << "  ✓ Created static object: " << entityInstance->name << "\n";
        }
    }
    
    return true;
}

bool World::InstantiatePass4_DynamicObjects(
    const Olympe::Editor::LevelDefinition& levelDef,
    InstantiationResult& result)
{
    // Create player, NPCs, enemies, and other dynamic objects
    PrefabFactory& factory = PrefabFactory::Get();
    
    for (const auto& entityInstance : levelDef.entities)
    {
        if (!entityInstance) continue;
        
        // Skip already handled types
        if (entityInstance->type == "collision" || entityInstance->type == "item" || 
            entityInstance->type == "waypoint" || entityInstance->type == "trigger" || 
            entityInstance->type == "spawn")
        {
            continue;
        }
        
        result.pass4_dynamicObjects.totalObjects++;
        
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
            result.pass4_dynamicObjects.failed++;
            result.pass4_dynamicObjects.failedObjects.push_back(prefabName);
            std::cout << "  ✗ Failed to create dynamic object: " << prefabName << "\n";
            continue;
        }
        
        if (HasComponent<Position_data>(entity))
        {
            Position_data& pos = GetComponent<Position_data>(entity);
            pos.position.x = static_cast<float>(entityInstance->position.x);
            pos.position.y = static_cast<float>(entityInstance->position.y);
        }
        
        result.pass4_dynamicObjects.successfullyCreated++;
        result.entityRegistry[entityInstance->name] = entity;
        std::cout << "  ✓ Created dynamic object: " << entityInstance->name << "\n";
    }
    
    return true;
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
                    std::cout << "  ✓ Assigned " << blackboard.patrolPointCount 
                              << " patrol points to: " << entityInstance->name << "\n";
                }
            }
        }
    }
    
    return true;
}
