/*
 * Olympe Blueprint Editor - Entity Inspector Manager
 * 
 * Tracks runtime entities and provides inspection/editing capabilities
 * Synchronizes with World ECS to provide real-time entity access
 */

#pragma once

#include "../ECS_Entity.h"
#include "../ECS_Components.h"
#include <vector>
#include <string>
#include <map>
#include <functional>

namespace Olympe
{
    // Entity metadata for inspector
    struct EntityInfo
    {
        EntityID id = INVALID_ENTITY_ID;
        std::string name;
        bool isActive = true;
        std::vector<std::string> componentTypes;
        
        EntityInfo() = default;
        EntityInfo(EntityID entityId) : id(entityId) {}
    };

    // Component property metadata
    struct ComponentPropertyInfo
    {
        std::string name;
        std::string type;       // "float", "int", "bool", "string", "vec2", "vec3"
        std::string value;      // String representation of current value
        void* dataPtr = nullptr; // Pointer to actual data (for editing)
        
        ComponentPropertyInfo() = default;
        ComponentPropertyInfo(const std::string& n, const std::string& t)
            : name(n), type(t) {}
    };

    /**
     * EntityInspectorManager - Manages runtime entity tracking and inspection
     * Singleton that maintains a synchronized list of all entities in the world
     */
    class EntityInspectorManager
    {
    public:
        static EntityInspectorManager& Instance();
        static EntityInspectorManager& Get() { return Instance(); }

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();  // Called each frame to sync with World

        // Entity notifications (called by World hooks)
        void OnEntityCreated(EntityID entity);
        void OnEntityDestroyed(EntityID entity);
        void OnComponentAdded(EntityID entity, const std::string& componentType);
        void OnComponentRemoved(EntityID entity, const std::string& componentType);

        // Entity queries
        std::vector<EntityID> GetAllEntities() const;
        std::vector<EntityInfo> GetAllEntityInfo() const;
        EntityInfo GetEntityInfo(EntityID entity) const;
        bool IsEntityValid(EntityID entity) const;

        // Component queries
        std::vector<std::string> GetEntityComponents(EntityID entity) const;
        bool HasComponent(EntityID entity, const std::string& componentType) const;

        // Property queries and editing
        std::vector<ComponentPropertyInfo> GetComponentProperties(EntityID entity, const std::string& componentType);
        bool SetComponentProperty(EntityID entity, const std::string& componentType, const std::string& propertyName, const std::string& value);

        // Filtering
        std::vector<EntityID> FilterByName(const std::string& nameFilter) const;
        std::vector<EntityID> FilterByComponent(const std::string& componentType) const;

        // Selection
        void SetSelectedEntity(EntityID entity);
        EntityID GetSelectedEntity() const { return m_SelectedEntity; }
        bool HasSelection() const { return m_SelectedEntity != INVALID_ENTITY_ID; }

        // State
        bool IsInitialized() const { return m_Initialized; }
        size_t GetEntityCount() const { return m_EntityList.size(); }

        // Force manual sync (for initial load or error recovery)
        void ForceSyncWithWorld();

    private:
        EntityInspectorManager();
        ~EntityInspectorManager();
        
        EntityInspectorManager(const EntityInspectorManager&) = delete;
        EntityInspectorManager& operator=(const EntityInspectorManager&) = delete;

        // Sync with World ECS
        void SyncWithWorld();
        
        // Component name extraction helpers
        std::string GetComponentName(ComponentTypeID typeId) const;
        ComponentTypeID GetComponentTypeId(const std::string& name) const;

    private:
        bool m_Initialized = false;
        EntityID m_SelectedEntity = INVALID_ENTITY_ID;
        std::vector<EntityID> m_EntityList;
        std::map<EntityID, EntityInfo> m_EntityInfoCache;
    };
}
