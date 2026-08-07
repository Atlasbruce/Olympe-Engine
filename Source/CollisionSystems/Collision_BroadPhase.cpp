#include "Collision_BroadPhase.h"

#include <algorithm>
#include <deque>
#include <set>
#include <utility>
#include <vector>

namespace Olympe {

static const float COLLISION_BROAD_PHASE_EPSILON = 0.0001f;

static bool Collision_ProxySortByMinX(const Collision_SpatialProxy& lhs,
                                      const Collision_SpatialProxy& rhs)
{
    if (lhs.worldAABB.min.x < rhs.worldAABB.min.x)
    {
        return true;
    }

    if (lhs.worldAABB.min.x > rhs.worldAABB.min.x)
    {
        return false;
    }

    return lhs.entity < rhs.entity;
}

static bool Collision_OverlapYWithEpsilon(const Collision_AABB& a,
                                          const Collision_AABB& b,
                                          float epsilon)
{
    return (a.min.y <= (b.max.y + epsilon)) &&
           ((a.max.y + epsilon) >= b.min.y);
}

void Collision_ComputeBroadPhasePairs(const std::vector<Collision_SpatialProxy>& proxies,
                                      std::vector<Collision_CandidatePair>& outPairs)
{
    outPairs.clear();

    if (proxies.empty())
    {
        return;
    }

    std::vector<Collision_SpatialProxy> sorted = proxies;
    std::sort(sorted.begin(), sorted.end(), Collision_ProxySortByMinX);

    std::deque<Collision_SpatialProxy> active;

    std::set<std::pair<EntityID, EntityID> > uniquePairs;

    std::vector<Collision_SpatialProxy>::const_iterator itCurrent = sorted.begin();
    for (; itCurrent != sorted.end(); ++itCurrent)
    {
        const Collision_SpatialProxy& current = *itCurrent;

        while (!active.empty())
        {
            const Collision_SpatialProxy& front = active.front();
            if (front.worldAABB.max.x < (current.worldAABB.min.x - COLLISION_BROAD_PHASE_EPSILON))
            {
                active.pop_front();
                continue;
            }
            break;
        }

        std::deque<Collision_SpatialProxy>::const_iterator itActive = active.begin();
        for (; itActive != active.end(); ++itActive)
        {
            const Collision_SpatialProxy& candidate = *itActive;

            if (!Collision_OverlapYWithEpsilon(candidate.worldAABB,
                                               current.worldAABB,
                                               COLLISION_BROAD_PHASE_EPSILON))
            {
                continue;
            }

            if (candidate.entity == current.entity)
            {
                continue;
            }

            const EntityID first = (candidate.entity < current.entity) ? candidate.entity : current.entity;
            const EntityID second = (candidate.entity < current.entity) ? current.entity : candidate.entity;
            uniquePairs.insert(std::make_pair(first, second));
        }

        active.push_back(current);
    }

    outPairs.reserve(uniquePairs.size());

    std::set<std::pair<EntityID, EntityID> >::const_iterator itPair = uniquePairs.begin();
    for (; itPair != uniquePairs.end(); ++itPair)
    {
        Collision_CandidatePair pair;
        pair.a = itPair->first;
        pair.b = itPair->second;
        outPairs.push_back(pair);
    }
}

} // namespace Olympe
