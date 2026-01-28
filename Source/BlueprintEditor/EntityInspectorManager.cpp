/*
 * Olympe Blueprint Editor - Entity Inspector Manager Implementation
 */

#include "EntityInspectorManager.h"
#include "../World.h"
#include "../ECS_Components.h"
#include <iostream>
#include <algorithm>

namespace Olympe
{
    EntityInspectorManager& EntityInspectorManager::Instance()
    {
        static EntityInspectorManager instance;
        return instance;
    }

    EntityInspectorManager::EntityInspectorManager()
    {
    }

    EntityInspectorManager::~EntityInspectorManager()
    {
        Shutdown();
    }

    void EntityInspectorManager::Initialize()
    {
        if (m_Initialized)
            return;

        std::cout << "[EntityInspectorManager] Initializing...\n";

        // Initial sync with World
        SyncWithWorld();

        m_Initialized = true;
        std::cout << "[EntityInspectorManager] Initialized with " << m_EntityList.size() << " entities\n";
    }

    void EntityInspectorManager::Shutdown()
    {
        if (!m_Initialized)
            return;

        std::cout << "[EntityInspectorManager] Shutting down...\n";

        m_EntityList.clear();
        m_EntityInfoCache.clear();
        m_SelectedEntity = INVALID_ENTITY_ID;

        m_Initialized = false;
    }

    void EntityInspectorManager::Update()
    {
        if (!m_Initialized)
            return;

        // Event-driven tracking via OnEntityCreated/Destroyed hooks
        // No polling needed - sync only on explicit request via ForceSyncWithWorld()
    }

    void EntityInspectorManager::OnEntityCreated(EntityID entity)
    {
        if (!m_Initialized)
            return;

        // Check if already tracked
        if (std::find(m_EntityList.begin(), m_EntityList.end(), entity) != m_EntityList.end())
            return;

        m_EntityList.push_back(entity);

        string name = "Entity_" + std::to_string(entity);
        if (World::Get().HasComponent<Identity_data>(entity))
        {
            Identity_data& identity = World::Get().GetComponent<Identity_data>(entity);
			name = identity.name;
        }

        EntityInfo info(entity);
        info.name = name;
        info.isActive = true;
        m_EntityInfoCache[entity] = info;

        std::cout << "[EntityInspectorManager] Entity created: " << entity << "\n";
    }

    void EntityInspectorManager::OnEntityDestroyed(EntityID entity)
    {
        if (!m_Initialized)
            return;

        auto it = std::find(m_EntityList.begin(), m_EntityList.end(), entity);
        if (it != m_EntityList.end())
        {
            m_EntityList.erase(it);
            m_EntityInfoCache.erase(entity);

            if (m_SelectedEntity == entity)
                m_SelectedEntity = INVALID_ENTITY_ID;

            std::cout << "[EntityInspectorManager] Entity destroyed: " << entity << "\n";
        }
    }

    void EntityInspectorManager::OnComponentAdded(EntityID entity, const std::string& componentType)
    {
        if (!m_Initialized)
            return;

        auto it = m_EntityInfoCache.find(entity);
        if (it != m_EntityInfoCache.end())
        {
            auto& components = it->second.componentTypes;
            if (std::find(components.begin(), components.end(), componentType) == components.end())
            {
                components.push_back(componentType);
            }
        }
    }

    void EntityInspectorManager::OnComponentRemoved(EntityID entity, const std::string& componentType)
    {
        if (!m_Initialized)
            return;

        auto it = m_EntityInfoCache.find(entity);
        if (it != m_EntityInfoCache.end())
        {
            auto& components = it->second.componentTypes;
            auto compIt = std::find(components.begin(), components.end(), componentType);
            if (compIt != components.end())
            {
                components.erase(compIt);
            }
        }
    }

    std::vector<EntityID> EntityInspectorManager::GetAllEntities() const
    {
        return m_EntityList;
    }

    std::vector<EntityInfo> EntityInspectorManager::GetAllEntityInfo() const
    {
        std::vector<EntityInfo> result;
        for (EntityID entity : m_EntityList)
        {
            auto it = m_EntityInfoCache.find(entity);
            if (it != m_EntityInfoCache.end())
                result.push_back(it->second);
        }
        return result;
    }

    EntityInfo EntityInspectorManager::GetEntityInfo(EntityID entity) const
    {
        auto it = m_EntityInfoCache.find(entity);
        if (it != m_EntityInfoCache.end())
            return it->second;

        return EntityInfo();
    }

    bool EntityInspectorManager::IsEntityValid(EntityID entity) const
    {
        return std::find(m_EntityList.begin(), m_EntityList.end(), entity) != m_EntityList.end();
    }

    std::vector<std::string> EntityInspectorManager::GetEntityComponents(EntityID entity) const
    {
        auto it = m_EntityInfoCache.find(entity);
        if (it != m_EntityInfoCache.end())
            return it->second.componentTypes;

        return {};
    }

    bool EntityInspectorManager::HasComponent(EntityID entity, const std::string& componentType) const
    {
        auto components = GetEntityComponents(entity);
        return std::find(components.begin(), components.end(), componentType) != components.end();
    }

    std::vector<ComponentPropertyInfo> EntityInspectorManager::GetComponentProperties(EntityID entity, const std::string& componentType)
    {
        std::vector<ComponentPropertyInfo> properties;

        // Access World to get actual component data
        World& world = World::Get();

        if (!world.IsEntityValid(entity))
            return properties;

        // Based on component type, extract properties
        // This is a simplified version - in a real implementation, you'd use reflection or type traits

        if (componentType == "Position_data")
        {
            if (world.HasComponent<Position_data>(entity))
            {
                auto& comp = world.GetComponent<Position_data>(entity);
                
                ComponentPropertyInfo prop;
                prop.name = "x";
                prop.type = "float";
                prop.value = std::to_string(comp.position.x);
                prop.dataPtr = &comp.position.x;
                properties.push_back(prop);

                prop.name = "y";
                prop.value = std::to_string(comp.position.y);
                prop.dataPtr = &comp.position.y;
                properties.push_back(prop);

                prop.name = "z";
                prop.value = std::to_string(comp.position.z);
                prop.dataPtr = &comp.position.z;
                properties.push_back(prop);
            }
        }
        else if (componentType == "Velocity_data")
        {
            if (world.HasComponent<Movement_data>(entity))
            {
                auto& comp = world.GetComponent<Movement_data>(entity);
                
                ComponentPropertyInfo prop;
                prop.name = "dx";
                prop.type = "float";
                prop.value = std::to_string(comp.velocity.x);
                prop.dataPtr = &comp.velocity.x;
                properties.push_back(prop);

                prop.name = "dy";
                prop.value = std::to_string(comp.velocity.y);
                prop.dataPtr = &comp.velocity.y;
                properties.push_back(prop);

                prop.name = "dz";
                prop.value = std::to_string(comp.velocity.z);
                prop.dataPtr = &comp.velocity.z;
                properties.push_back(prop);
            }
        }
        // Add more component types as needed...

        return properties;
    }

    bool EntityInspectorManager::SetComponentProperty(EntityID entity, const std::string& componentType, 
                                                       const std::string& propertyName, const std::string& value)
    {
        World& world = World::Get();

        if (!world.IsEntityValid(entity))
            return false;

        // Based on component type and property name, set the value
        // This is simplified - a real implementation would use reflection

        if (componentType == "Position_data" && world.HasComponent<Position_data>(entity))
        {
            auto& comp = world.GetComponent<Position_data>(entity);
            
            try
            {
                float floatValue = std::stof(value);
                
                if (propertyName == "x")
                    comp.position.x = floatValue;
                else if (propertyName == "y")
                    comp.position.y = floatValue;
                else if (propertyName == "z")
                    comp.position.z = floatValue;
                else
                    return false;

                return true;
            }
            catch (...)
            {
                return false;
            }
        }
        else if (componentType == "Movement_data" && world.HasComponent<Movement_data>(entity))
        {            
            try
            {
                auto& comp = world.GetComponent<Movement_data>(entity);

                float floatValue = std::stof(value);
                
                if (propertyName == "dx")
                    comp.velocity.x = floatValue;
                else if (propertyName == "dy")
                    comp.velocity.y = floatValue;
                else if (propertyName == "dz")
                    comp.velocity.z = floatValue;
                else
                    return false;

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        return false;
    }

    std::vector<EntityID> EntityInspectorManager::FilterByName(const std::string& nameFilter) const
    {
        std::vector<EntityID> result;

        if (nameFilter.empty())
            return m_EntityList;

        for (EntityID entity : m_EntityList)
        {
            auto it = m_EntityInfoCache.find(entity);
            if (it != m_EntityInfoCache.end())
            {
                if (it->second.name.find(nameFilter) != std::string::npos)
                    result.push_back(entity);
            }
        }

        return result;
    }

    std::vector<EntityID> EntityInspectorManager::FilterByComponent(const std::string& componentType) const
    {
        std::vector<EntityID> result;

        for (EntityID entity : m_EntityList)
        {
            if (HasComponent(entity, componentType))
                result.push_back(entity);
        }

        return result;
    }

    void EntityInspectorManager::SetSelectedEntity(EntityID entity)
    {
        if (IsEntityValid(entity) || entity == INVALID_ENTITY_ID)
            m_SelectedEntity = entity;
    }

    void EntityInspectorManager::ForceSyncWithWorld()
    {
        SyncWithWorld();
    }

    void EntityInspectorManager::SyncWithWorld()
    {
        World& world = World::Get();

        // Get all entities from World
        const auto& entitySignatures = world.m_entitySignatures;

        // Add any entities we don't have
        for (const auto& pair : entitySignatures)
        {
            EntityID entity = pair.first;
            
            if (std::find(m_EntityList.begin(), m_EntityList.end(), entity) == m_EntityList.end())
            {
                // New entity found
                OnEntityCreated(entity);
            }

            // Update component list
            auto& info = m_EntityInfoCache[entity];
            info.componentTypes.clear();

            // Extract component types from signature
            const ComponentSignature& sig = pair.second;
            
            // Check each component type
            // This is a simplified approach - ideally you'd have a component type registry
            if (sig.test(GetComponentTypeID_Static<Position_data>()))
                info.componentTypes.push_back("Position_data");
            if (sig.test(GetComponentTypeID_Static<Movement_data>()))
                info.componentTypes.push_back("Velocity_data");
            if (sig.test(GetComponentTypeID_Static<VisualSprite_data>()))
                info.componentTypes.push_back("Sprite_data");
            if (sig.test(GetComponentTypeID_Static<Health_data>()))
                info.componentTypes.push_back("Health_data");
            if (sig.test(GetComponentTypeID_Static<Controller_data>()))
                info.componentTypes.push_back("Controller_data");
            if (sig.test(GetComponentTypeID_Static<PlayerBinding_data>()))
                info.componentTypes.push_back("PlayerBinding_data");
            if (sig.test(GetComponentTypeID_Static<PlayerController_data>()))
                info.componentTypes.push_back("PlayerController_data");
            // Add more component types as needed...
        }

        // Remove entities that no longer exist in World
        m_EntityList.erase(
            std::remove_if(m_EntityList.begin(), m_EntityList.end(),
                [&entitySignatures](EntityID entity)
                {
                    return entitySignatures.find(entity) == entitySignatures.end();
                }),
            m_EntityList.end()
        );
    }

    std::string EntityInspectorManager::GetComponentName(ComponentTypeID typeId) const
    {
        // Map type IDs to names
        // This is a simplified approach
        return "UnknownComponent";
    }

    ComponentTypeID EntityInspectorManager::GetComponentTypeId(const std::string& name) const
    {
        // Map names to type IDs
        return 0;
    }
}
