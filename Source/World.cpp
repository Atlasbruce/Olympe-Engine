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
    SYSTEM_LOG << "World Initialized\n";
}
//---------------------------------------------------------------------------------------------
World::~World()
{
    // Clean up all objects
    /*DEPRECATED OBJECT MANAGEMENT*/
    {
        for (auto obj : m_objectlist)
        {
            delete obj;
        }
        m_objectlist.clear();
    }
    SYSTEM_LOG << "World Destroyed\n";
}
//---------------------------------------------------------------------------------------------
void World::Initialize_ECS_Systems()
{
	// Initialization of ECS Systems
	// WARNING THE ORDER OF SYSTEMS MATTERS!

    /*
	Order of processing systems:
	- InputSystem
	- InputMappingSystem (NEW: maps hardware input to gameplay actions)
	- PlayerControlSystem
	- AI MovementSystem
	- DetectionSystem
	- PhysicsSystem
	- CollisionSystem
	- TriggerSystem
	- AudioSystem

    - RenderingSystem
    */
	Add_ECS_System(std::make_unique<InputSystem>());
	Add_ECS_System(std::make_unique<InputMappingSystem>());
	Add_ECS_System(std::make_unique<PlayerControlSystem>());
    Add_ECS_System(std::make_unique<AISystem>());
    Add_ECS_System(std::make_unique<DetectionSystem>());
    Add_ECS_System(std::make_unique<PhysicsSystem>());
    Add_ECS_System(std::make_unique<CollisionSystem>());
	Add_ECS_System(std::make_unique<TriggerSystem>());
	Add_ECS_System(std::make_unique<MovementSystem>());
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
	static bool firstCall = true;

    // Mise � jour de tous les syst�mes enregistr�s dans l'ordre
    for (const auto& system : m_systems)
    {
        system->Process();
		if (firstCall)
        {
            SYSTEM_LOG << "ECS System processed: " << typeid(*system).name() << "\n";
        }
    }

	firstCall = false;
}
//---------------------------------------------------------------------------------------------
void World::Render_ECS_Systems()
{
    static bool firstCall = true;

    // Mise � jour de tous les syst�mes enregistr�s dans l'ordre
    for (const auto& system : m_systems)
    {
        system->Render();
        if (firstCall)
        {
            SYSTEM_LOG << "ECS System Rendered: " << typeid(*system).name() << "\n";
        }
    }

    firstCall = false;
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
    return newID;
}
//---------------------------------------------------------------------------------------------
void World::DestroyEntity(EntityID entity)
{
    if (entity == INVALID_ENTITY_ID || m_entitySignatures.find(entity) == m_entitySignatures.end())
    {
        return;
    }

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