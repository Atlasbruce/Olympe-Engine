#pragma once

#include "Collision_Primitives.h"
#include "../ECS_Entity.h"

#include <map>
#include <vector>

namespace Olympe {

struct Collision_SpatialProxy
{
    EntityID entity;
    Collision_AABB worldAABB;

    Collision_SpatialProxy()
        : entity(INVALID_ENTITY_ID)
    {
    }
};

class Collision_SpatialProxyRegistry
{
public:
    void RegisterOrUpdateProxy(EntityID id, const Collision_AABB& aabb);
    void RemoveProxy(EntityID id);
    void Clear();
    void GetSnapshot(std::vector<Collision_SpatialProxy>& out) const;

private:
    std::map<EntityID, Collision_AABB> m_proxies;
};

} // namespace Olympe
