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

        // Creation and adding of the component using move semantics
        pool->AddComponent(entity, T{ std::forward<Args>(args)... });

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

private:

    std::vector<std::unique_ptr<Level>> m_levels;
};
