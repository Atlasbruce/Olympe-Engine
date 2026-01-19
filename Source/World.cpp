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
#include "GameEngine.h"
#include "../SDL/include/SDL3_image/SDL_image.h"
#include <chrono>

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
    SYSTEM_LOG << "World::LoadLevelFromTiled - Loading: " << tiledMapPath << "\n";
    
    Olympe::Tiled::TiledMap tiledMap;

    // 1. Load the Tiled map
    Olympe::Tiled::TiledLevelLoader loader;
    if (!loader.LoadFromFile(tiledMapPath, tiledMap))
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Failed to load Tiled map\n";
        return false;
    }
    
    //const Olympe::Tiled::TiledMap& tiledMap = loader.GetMap();
    std::string orientationStr;
    switch (tiledMap.orientation)
    {
        case Olympe::Tiled::MapOrientation::Orthogonal: orientationStr = "Orthogonal"; break;
        case Olympe::Tiled::MapOrientation::Isometric: orientationStr = "Isometric"; break;
        case Olympe::Tiled::MapOrientation::Staggered: orientationStr = "Staggered"; break;
        case Olympe::Tiled::MapOrientation::Hexagonal: orientationStr = "Hexagonal"; break;
        default: orientationStr = "Unknown"; break;
    }
    SYSTEM_LOG << "World::LoadLevelFromTiled - Map loaded: " << tiledMap.width << "x" << tiledMap.height 
               << " (orientation: " << orientationStr << ")\n";
    
    // 2. Convert to Olympe format
    Olympe::Tiled::TiledToOlympe converter;
    
    // Load prefab mapping from Config/tiled_prefab_mapping.json
    std::string mappingPath = "Config/tiled_prefab_mapping.json";
    if (!converter.LoadPrefabMapping(mappingPath))
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Warning: Could not load prefab mapping from " 
                   << mappingPath << ", using defaults\n";
    }
    
    // Convert the map
    Olympe::Editor::LevelDefinition levelDef;
    if (!converter.Convert(tiledMap, levelDef))
    {
        SYSTEM_LOG << "World::LoadLevelFromTiled - Failed to convert Tiled map to Olympe format\n";
        return false;
    }
    
    SYSTEM_LOG << "World::LoadLevelFromTiled - Converted level with " 
               << levelDef.entities.size() << " entities\n";
    
    // 3. Unload current level if any
    UnloadCurrentLevel();
    
    // 4. Create entities from level definition
    PrefabFactory& factory = PrefabFactory::Get();
    
    for (const auto& entityInstance : levelDef.entities)
    {
        if (!entityInstance) continue;
        
        SYSTEM_LOG << "World::LoadLevelFromTiled - Creating entity: " << entityInstance->name 
                   << " from prefab: " << entityInstance->prefabPath << "\n";
        
        // Handle collision objects manually (not via prefab)
        if (entityInstance->type == "collision")
        {
            EntityID eid = CreateEntity();
            
            AddComponent<Identity_data>(eid, entityInstance->name, "Collision", EntityType::Collision);
            AddComponent<Position_data>(eid, Vector(
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                0.0f
            ));
            
            // Extract collision bounds from overrides
            float width = 64.0f;
            float height = 64.0f;
            if (!entityInstance->overrides.is_null())
            {
                if (entityInstance->overrides.contains("width"))
                {
                    width = entityInstance->overrides["width"].get<float>();
                }
                if (entityInstance->overrides.contains("height"))
                {
                    height = entityInstance->overrides["height"].get<float>();
                }
            }
            
            AddComponent<CollisionZone_data>(eid, SDL_FRect{
                static_cast<float>(entityInstance->position.x),
                static_cast<float>(entityInstance->position.y),
                width,
                height
            }, true);
            
            SYSTEM_LOG << "World::LoadLevelFromTiled - Created collision zone '" << entityInstance->name 
                       << "' at (" << entityInstance->position.x << "," << entityInstance->position.y 
                       << ") size (" << width << "x" << height << ")\n";
            
            continue; // Skip prefab creation for collision objects
        }
        
        // Extract prefab name from path (e.g., "Blueprints/EntityPrefab/player.json" -> "player")
        // or use directly if it's already a short name (e.g., "PlayerEntity")
        std::string prefabName = entityInstance->prefabPath;
        size_t lastSlash = prefabName.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            prefabName = prefabName.substr(lastSlash + 1);
        }
        size_t lastDot = prefabName.find_last_of(".");
        if (lastDot != std::string::npos)
        {
            prefabName = prefabName.substr(0, lastDot);
        }
        
        // Create entity using prefab factory
        EntityID entity = factory.CreateEntity(prefabName);
        if (entity == INVALID_ENTITY_ID)
        {
            SYSTEM_LOG << "World::LoadLevelFromTiled - Warning: Failed to create entity from prefab: " 
                       << prefabName << "\n";
            continue;
        }
        
        // Set position if entity has Position_data component
        if (HasComponent<Position_data>(entity))
        {
            Position_data& pos = GetComponent<Position_data>(entity);
            pos.position.x = static_cast<float>(entityInstance->position.x);
            pos.position.y = static_cast<float>(entityInstance->position.y);
            pos.position.z = 0.0f;
        }
        
        // Apply overrides from entityInstance->overrides (component property overrides)
        // This would require reflection or manual parsing based on override structure
        // For now, we'll handle special cases like patrol paths
        
        // Handle patrol paths from custom properties
        if (!entityInstance->overrides.is_null() && entityInstance->overrides.contains("patrolPath"))
        {
            if (HasComponent<AIBlackboard_data>(entity))
            {
                AIBlackboard_data& blackboard = GetComponent<AIBlackboard_data>(entity);
                
                // Extract patrol points from overrides
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
                    
                    if (blackboard.patrolPointCount > 0)
                    {
                        SYSTEM_LOG << "World::LoadLevelFromTiled - Assigned " 
                                   << blackboard.patrolPointCount << " patrol points to entity " 
                                   << entityInstance->name << "\n";
                    }
                }
            }
        }
    }
    
    // 5. Load parallax layers from metadata
    if (levelDef.metadata.customData.contains("parallaxLayers"))
    {
        Olympe::Tiled::ParallaxLayerManager& parallaxMgr = Olympe::Tiled::ParallaxLayerManager::Get();
        parallaxMgr.Clear();
        
        const auto& parallaxLayersJson = levelDef.metadata.customData["parallaxLayers"];
        if (parallaxLayersJson.is_array())
        {
            for (const auto& layerJson : parallaxLayersJson)
            {
                std::string imagePath = layerJson["imagePath"].get<std::string>();
                SDL_Texture* texture = IMG_LoadTexture(GameEngine::renderer, imagePath.c_str());
                
                if (!texture)
                {
                    SYSTEM_LOG << "World::LoadLevelFromTiled - Warning: Failed to load parallax image: " 
                               << imagePath << " - " << SDL_GetError() << "\n";
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
                
                SYSTEM_LOG << "World::LoadLevelFromTiled - Loaded parallax layer '" << layer.name 
                           << "' (zOrder=" << layer.zOrder << ", parallaxX=" << layer.scrollFactorX << ")\n";
            }
            
            SYSTEM_LOG << "World::LoadLevelFromTiled - Total parallax layers loaded: " 
                       << parallaxMgr.GetLayerCount() << "\n";
        }
    }
    
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