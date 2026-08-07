#include "Collision_SpatialProxyRegistry.h"

#include "../system/system_utils.h"

namespace Olympe {

void Collision_SpatialProxyRegistry::RegisterOrUpdateProxy(EntityID id, const Collision_AABB& aabb)
{
    if (!Collision_IsValidAABB(aabb))
    {
        SYSTEM_LOG << "[Collision] Collision_SpatialProxyRegistry ignored invalid AABB for entity="
                   << id << std::endl;
        return;
    }

    m_proxies[id] = aabb;
}

void Collision_SpatialProxyRegistry::RemoveProxy(EntityID id)
{
    m_proxies.erase(id);
}

void Collision_SpatialProxyRegistry::Clear()
{
    m_proxies.clear();
}

void Collision_SpatialProxyRegistry::GetSnapshot(std::vector<Collision_SpatialProxy>& out) const
{
    out.clear();
    out.reserve(m_proxies.size());

    std::map<EntityID, Collision_AABB>::const_iterator it = m_proxies.begin();
    for (; it != m_proxies.end(); ++it)
    {
        Collision_SpatialProxy proxy;
        proxy.entity = it->first;
        proxy.worldAABB = it->second;
        out.push_back(proxy);
    }
}

} // namespace Olympe
