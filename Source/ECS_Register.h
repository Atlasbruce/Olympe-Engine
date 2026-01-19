/*

*/
#pragma once
#include "ECS_Entity.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"

#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>

// --- Component Pool Implementation (Contiguous Storage) ---
template <typename T>
class ComponentPool : public IComponentPool
{
public:
    // The actual data container (contiguous storage for cache efficiency)
    std::vector<T> m_data;
    // Map from EntityID to index in the data vector
    std::unordered_map<EntityID, size_t> m_entityToIndex;
    // Map from index to EntityID (needed for swap-and-pop)
    std::vector<EntityID> m_indexToEntity;

    ComponentPool()
    {
		// DEPRECATED LIMITATION
        // m_data.reserve(MAX_ENTITIES);
        // m_indexToEntity.reserve(MAX_ENTITIES);
    }

    // Virtual function implementation: removes a component using swap-and-pop
    void RemoveComponent(EntityID entity) override
    {
        if (!m_entityToIndex.count(entity)) return;

        size_t indexOfRemoved = m_entityToIndex[entity];
        size_t indexOfLast = m_data.size() - 1;

        // Swap-and-pop optimization: maintains data contiguity
        if (indexOfRemoved != indexOfLast)
        {
            // 1. Move/Swap data in the vector
            m_data[indexOfRemoved] = std::move(m_data[indexOfLast]);

            // 2. Update the mapping for the moved Entity
            EntityID entityOfLast = m_indexToEntity[indexOfLast];
            m_entityToIndex[entityOfLast] = indexOfRemoved;
            m_indexToEntity[indexOfRemoved] = entityOfLast;
        }

        // 3. Remove the last element (which is the component to be deleted)
        m_data.pop_back();
        m_indexToEntity.pop_back();

        // 4. Clean up the mapping for the deleted Entity
        m_entityToIndex.erase(entity);
    }

    // Adds a component (T) for the given EntityID
    // Constructs the component in-place using perfect forwarding
    template<typename... Args>
    void AddComponent(EntityID entity, Args&&... args)
    {
        if (m_entityToIndex.count(entity)) return;

        size_t newIndex = m_data.size();

        m_data.emplace_back(std::forward<Args>(args)...);
        m_indexToEntity.push_back(entity);
        m_entityToIndex[entity] = newIndex;
    }

    // Fast access to the component by EntityID
    T& GetComponent(EntityID entity)
    {
        if (!m_entityToIndex.count(entity))
        {
            throw std::runtime_error("Component not found for entity.");
        }
        return m_data[m_entityToIndex[entity]];
    }

    bool HasComponent(EntityID entity) const
    {
        return m_entityToIndex.count(entity) > 0;
    }
};
