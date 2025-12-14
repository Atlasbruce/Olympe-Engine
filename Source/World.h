/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

World purpose: Manage the overall game world, including object management, level handling, and ECS architecture.

*/
#pragma once

#include "object.h"
#include "ObjectComponent.h"
#include "system/EventManager.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <type_traits>

#include "Level.h" // add Level management
#include "GameState.h"
#include "OptionsManager.h"
#include "system/CameraManager.h"

// Include ECS related headers
#include "Ecs_Entity.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"
#include "ECS_Register.h" // Include the implementation of ComponentPool

class World : public Object
{
public:
    World();
    virtual ~World();
    virtual ObjectType GetObjectType() const { return ObjectType::Singleton; }

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

    //---------------------------------------------------------------
    // Main processing loop called each frame: events are processed first (async), then stages in order
    void Process()
    {
        //0) Ensure queued events are dispatched before the update stages
        EventManager::Get().Process();

        // check global game state
        GameState state = GameStateManager::GetState();
        bool paused = (state == GameState::GameState_Paused);

        Process_ECS_Systems();

        /*DEPRECATED OBJECT MANAGEMENT*/
        {
            //1) Physics
            if (!paused)
            {
                for (auto* prop : array_component_lists_bytypes[static_cast<size_t>(ComponentType::Physics)])
                {
                    if (prop) prop->Process();
                }
            }

            //2) AI
            if (!paused)
            {
                for (auto* prop : array_component_lists_bytypes[static_cast<size_t>(ComponentType::AI)])
                {
                    if (prop) prop->Process();
                }
            }

            //3) Visual
            for (auto* prop : array_component_lists_bytypes[static_cast<size_t>(ComponentType::Visual)])
            {
                if (prop) prop->Process();
            }

            //4) Audio
            if (!paused)
            {
                for (auto* prop : array_component_lists_bytypes[static_cast<size_t>(ComponentType::Audio)])
                {
                    if (prop) prop->Process();
                }
            }
        }

		// Update Camera positions if needed after all objects have been processed
        CameraManager::Get().Process();
    }
    //---------------------------------------------------------------------------------------------
    void Render()
    {
        /*DEPRECATED OBJECT MANAGEMENT*/
        {

            // Render stage (note: actual drawing may require renderer context)
            for (auto* prop : array_component_lists_bytypes[static_cast<size_t>(ComponentType::Visual)])
            {
                if (prop) prop->Render();
            }
            if (OptionsManager::Get().IsSet(OptionFlags::ShowDebugInfo))
            {
                // Render debug for Visual components
                for (auto* prop : array_component_lists_bytypes[static_cast<size_t>(ComponentType::Visual)])
                {
                    if (prop) prop->RenderDebug();
                }
                for (auto* prop : array_component_lists_bytypes[static_cast<size_t>(ComponentType::AI)])
                {
                    if (prop) prop->RenderDebug();
                }
            }
        }

		// ECS Rendering Systems
        Render_ECS_Systems();
	}

	//---------------------------------------------------------------------------------------------
	// Objects & Entities management
    /*DEPRECATED OBJECT MANAGEMENT*/void StoreObject(Object* obj)
    {
        // Implementation to add object to the game engine
        m_objectlist.push_back(obj);
		SYSTEM_LOG << "World: Added object " + obj->name + " to World\n";
    }
	//---------------------------------------------------------------------------------------------
    Object*& GetObjectByUID(uint64_t uid)
    {
        static Object* ObjPtr = nullptr;
        for (auto* obj : m_objectlist)
        {
            if (obj && obj->GetUID() == uid)
            {
				ObjPtr = obj;
                return ObjPtr;
            }
        }
        ObjPtr = nullptr;
		return ObjPtr;
   	}
    // provide access to object list for other systems (Factory)
    /*DEPRECATED OBJECT MANAGEMENT*/std::vector<Object*>& GetObjectList() { return m_objectlist; }
    /*DEPRECATED OBJECT MANAGEMENT*/const std::vector<Object*>& GetObjectList() const { return m_objectlist; }

    //---------------------------------------------------------------------------------------------
    // Objects' Components management
    /*DEPRECATED OBJECT MANAGEMENT*/void StoreComponent(ObjectComponent* objectComponent)
    {
        if (!objectComponent)
        {
            SYSTEM_LOG << "Error: World::AddComponent called with null component!\n";
            return;
        }

        try
        {
            //add Component to the right type list in the array
            array_component_lists_bytypes[static_cast<size_t>(objectComponent->GetComponentType())].push_back(objectComponent);
            SYSTEM_LOG << "World: Added component " + objectComponent->name + " of type " << static_cast<int>(objectComponent->GetComponentType()) << " to World\n";
        }
        catch (const std::exception&)
        {
			SYSTEM_LOG << "Error: World::AddComponent failed to add component of type " << static_cast<int>(objectComponent->GetComponentType()) << "\n";
        }
    }

    //---------------------------------------------------------------------------------------------
    // Level management
    void AddLevel(std::unique_ptr<Level> level)
    {
        if (level) m_levels.push_back(std::move(level));
    }

    const std::vector<std::unique_ptr<Level>>& GetLevels() const { return m_levels; }

    // -------------------------------------------------------------
    // ECS Entity Management
    EntityID CreateEntity();
    void DestroyEntity(EntityID entity);

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
    // Helper function pour le traitement spécial de PlayerBinding_data
    template <typename T>
    void HandleSpecialComponentRegistration(EntityID entity, typename std::enable_if<std::is_same<T, PlayerBinding_data>::value>::type* = nullptr)
    {
        extern void RegisterInputEntityWithManager(EntityID e);
        RegisterInputEntityWithManager(entity);
    }

    template <typename T>
    void HandleSpecialComponentRegistration(EntityID entity, typename std::enable_if<!std::is_same<T, PlayerBinding_data>::value>::type* = nullptr)
    {
        // Ne fait rien pour les autres types
    }

private:

    /*DEPRECATED OBJECT MANAGEMENT*/std::vector<Object*> m_objectlist;
    /*DEPRECATED OBJECT MANAGEMENT*/std::array<std::vector<ObjectComponent*>, static_cast<size_t>(ComponentType::Count)> array_component_lists_bytypes;


    std::vector<std::unique_ptr<Level>> m_levels;
};
