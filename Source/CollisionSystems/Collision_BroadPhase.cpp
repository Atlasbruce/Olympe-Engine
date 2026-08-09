#include "Collision_BroadPhase.h"

#include <cstddef>
#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

namespace Olympe {

static const float COLLISION_BROAD_PHASE_EPSILON = 0.0001f;

bool Collision_BroadPhaseFilter::Allows(const Collision_SpatialProxy& lhs,
                                        const Collision_SpatialProxy& rhs) const
{
    if (predicate == 0)
    {
        return true;
    }

    return predicate(lhs, rhs, userData);
}

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

static bool Collision_ProxySortByPair(const std::pair<EntityID, EntityID>& lhs,
                                      const std::pair<EntityID, EntityID>& rhs)
{
    if (lhs.first < rhs.first)
    {
        return true;
    }

    if (lhs.first > rhs.first)
    {
        return false;
    }

    return lhs.second < rhs.second;
}

static bool Collision_OverlapAxisWithEpsilon(float minA,
                                             float maxA,
                                             float minB,
                                             float maxB,
                                             float epsilon)
{
    return (minA <= (maxB + epsilon)) &&
           ((maxA + epsilon) >= minB);
}

static bool Collision_OverlapWithEpsilon(const Collision_AABB& a,
                                         const Collision_AABB& b,
                                         float epsilon)
{
    return Collision_OverlapAxisWithEpsilon(a.min.x, a.max.x, b.min.x, b.max.x, epsilon) &&
           Collision_OverlapAxisWithEpsilon(a.min.y, a.max.y, b.min.y, b.max.y, epsilon);
}

static bool Collision_IsValidAABB_NoLog(const Collision_AABB& box)
{
    return (box.min.x <= box.max.x) &&
           (box.min.y <= box.max.y) &&
           (box.min.z <= box.max.z);
}

static void Collision_CollectValidProxies(const std::vector<Collision_SpatialProxy>& proxies,
                                          std::vector<Collision_SpatialProxy>& validProxies,
                                          Collision_BroadPhaseMetrics* metrics)
{
    validProxies.clear();
    validProxies.reserve(proxies.size());

    if (metrics)
    {
        metrics->inputProxyCount = proxies.size();
    }

    std::vector<Collision_SpatialProxy>::const_iterator it = proxies.begin();
    for (; it != proxies.end(); ++it)
    {
        if (!Collision_IsValidAABB_NoLog(it->worldAABB))
        {
            if (metrics)
            {
                ++metrics->invalidProxyCount;
            }
            continue;
        }

        validProxies.push_back(*it);
    }

    if (metrics)
    {
        metrics->validProxyCount = validProxies.size();
    }
}

static void Collision_FinalizePairs(std::vector<std::pair<EntityID, EntityID> >& pairs,
                                    std::vector<Collision_CandidatePair>& outPairs,
                                    Collision_BroadPhaseMetrics* metrics)
{
    std::sort(pairs.begin(), pairs.end(), Collision_ProxySortByPair);

    std::vector<std::pair<EntityID, EntityID> >::iterator uniqueEnd =
        std::unique(pairs.begin(), pairs.end());

    if (metrics)
    {
        metrics->pairsBeforeDedup = pairs.size();
        metrics->pairsAfterDedup = static_cast<size_t>(uniqueEnd - pairs.begin());
        metrics->duplicatePairs = metrics->pairsBeforeDedup - metrics->pairsAfterDedup;
    }

    pairs.erase(uniqueEnd, pairs.end());

    outPairs.clear();
    outPairs.reserve(pairs.size());

    std::vector<std::pair<EntityID, EntityID> >::const_iterator it = pairs.begin();
    for (; it != pairs.end(); ++it)
    {
        Collision_CandidatePair pair;
        pair.a = it->first;
        pair.b = it->second;
        outPairs.push_back(pair);
    }

    if (metrics)
    {
        metrics->pairsAfterDedup = outPairs.size();
    }
}

static void Collision_ComputeBroadPhasePairsInternal(const std::vector<Collision_SpatialProxy>& proxies,
                                                     std::vector<Collision_CandidatePair>& outPairs,
                                                     Collision_BroadPhaseScratch& scratch,
                                                     Collision_BroadPhaseMetrics* metrics,
                                                     const Collision_BroadPhaseFilter& filter)
{
    if (metrics)
    {
        metrics->Reset();
    }

    Collision_CollectValidProxies(proxies, scratch.sortedProxies, metrics);

    if (scratch.sortedProxies.empty())
    {
        outPairs.clear();
        return;
    }

    std::sort(scratch.sortedProxies.begin(), scratch.sortedProxies.end(), Collision_ProxySortByMinX);

    scratch.activeProxies.clear();
    scratch.activeProxies.reserve(scratch.sortedProxies.size());
    scratch.normalizedPairs.clear();
    scratch.normalizedPairs.reserve(scratch.sortedProxies.size());

    size_t activeStart = 0;

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

    std::vector<Collision_SpatialProxy>::const_iterator itCurrent = scratch.sortedProxies.begin();
    for (; itCurrent != scratch.sortedProxies.end(); ++itCurrent)
    {
        const Collision_SpatialProxy& current = *itCurrent;

        while (activeStart < scratch.activeProxies.size())
        {
            const Collision_SpatialProxy& front = scratch.activeProxies[activeStart];
            if (front.worldAABB.max.x < (current.worldAABB.min.x - COLLISION_BROAD_PHASE_EPSILON))
            {
                ++activeStart;
                continue;
            }
            break;
        }

        if (activeStart > 0 && activeStart * 2 >= scratch.activeProxies.size())
        {
            scratch.activeProxies.erase(scratch.activeProxies.begin(),
                                        scratch.activeProxies.begin() + static_cast<std::ptrdiff_t>(activeStart));
            activeStart = 0;
        }

        size_t activeCount = scratch.activeProxies.size() - activeStart;
        if (metrics && activeCount > metrics->maxActiveCount)
        {
            metrics->maxActiveCount = activeCount;
        }

        size_t activeIndex = activeStart;
        for (; activeIndex < scratch.activeProxies.size(); ++activeIndex)
        {
            const Collision_SpatialProxy& candidate = scratch.activeProxies[activeIndex];

            if (metrics)
            {
                ++metrics->comparisons;
            }

            if (candidate.entity == current.entity)
            {
                if (metrics)
                {
                    ++metrics->selfRejectedPairs;
                }
                continue;
            }

            if (!Collision_OverlapWithEpsilon(candidate.worldAABB,
                                              current.worldAABB,
                                              COLLISION_BROAD_PHASE_EPSILON))
            {
                if (metrics)
                {
                    ++metrics->axisRejectedPairs;
                }
                continue;
            }

            if (!filter.Allows(candidate, current))
            {
                if (metrics)
                {
                    ++metrics->filteredPairs;
                }
                continue;
            }

            const EntityID first = (candidate.entity < current.entity) ? candidate.entity : current.entity;
            const EntityID second = (candidate.entity < current.entity) ? current.entity : candidate.entity;
            scratch.normalizedPairs.push_back(std::make_pair(first, second));
        }

        scratch.activeProxies.push_back(current);
    }

    if (metrics)
    {
        metrics->elapsedMs = std::chrono::duration<double, std::milli>(
                                 std::chrono::steady_clock::now() - startTime).count();
    }

    Collision_FinalizePairs(scratch.normalizedPairs, outPairs, metrics);
}

static void Collision_ComputeBroadPhasePairsBruteForceInternal(const std::vector<Collision_SpatialProxy>& proxies,
                                                               std::vector<Collision_CandidatePair>& outPairs,
                                                               Collision_BroadPhaseScratch& scratch,
                                                               Collision_BroadPhaseMetrics* metrics,
                                                               const Collision_BroadPhaseFilter& filter)
{
    if (metrics)
    {
        metrics->Reset();
    }

    Collision_CollectValidProxies(proxies, scratch.sortedProxies, metrics);

    scratch.normalizedPairs.clear();
    scratch.normalizedPairs.reserve(scratch.sortedProxies.size());

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

    size_t i = 0;
    for (; i < scratch.sortedProxies.size(); ++i)
    {
        size_t j = i + 1;
        for (; j < scratch.sortedProxies.size(); ++j)
        {
            const Collision_SpatialProxy& lhs = scratch.sortedProxies[i];
            const Collision_SpatialProxy& rhs = scratch.sortedProxies[j];

            if (metrics)
            {
                ++metrics->comparisons;
            }

            if (lhs.entity == rhs.entity)
            {
                if (metrics)
                {
                    ++metrics->selfRejectedPairs;
                }
                continue;
            }

            if (!Collision_OverlapWithEpsilon(lhs.worldAABB, rhs.worldAABB, COLLISION_BROAD_PHASE_EPSILON))
            {
                if (metrics)
                {
                    ++metrics->axisRejectedPairs;
                }
                continue;
            }

            if (!filter.Allows(lhs, rhs))
            {
                if (metrics)
                {
                    ++metrics->filteredPairs;
                }
                continue;
            }

            const EntityID first = (lhs.entity < rhs.entity) ? lhs.entity : rhs.entity;
            const EntityID second = (lhs.entity < rhs.entity) ? rhs.entity : lhs.entity;
            scratch.normalizedPairs.push_back(std::make_pair(first, second));
        }
    }

    if (metrics)
    {
        metrics->elapsedMs = std::chrono::duration<double, std::milli>(
                                 std::chrono::steady_clock::now() - startTime).count();
    }

    Collision_FinalizePairs(scratch.normalizedPairs, outPairs, metrics);
}

void Collision_ComputeBroadPhasePairs(const std::vector<Collision_SpatialProxy>& proxies,
                                      std::vector<Collision_CandidatePair>& outPairs)
{
    Collision_BroadPhaseScratch scratch;
    Collision_ComputeBroadPhasePairs(proxies,
                                     outPairs,
                                     scratch,
                                     0,
                                     Collision_BroadPhaseFilter());
}

void Collision_ComputeBroadPhasePairs(const std::vector<Collision_SpatialProxy>& proxies,
                                      std::vector<Collision_CandidatePair>& outPairs,
                                      Collision_BroadPhaseScratch& scratch,
                                      Collision_BroadPhaseMetrics* metrics,
                                      const Collision_BroadPhaseFilter& filter)
{
    Collision_ComputeBroadPhasePairsInternal(proxies, outPairs, scratch, metrics, filter);
}

void Collision_ComputeBroadPhasePairsBruteForce(const std::vector<Collision_SpatialProxy>& proxies,
                                                std::vector<Collision_CandidatePair>& outPairs,
                                                Collision_BroadPhaseScratch& scratch,
                                                Collision_BroadPhaseMetrics* metrics,
                                                const Collision_BroadPhaseFilter& filter)
{
    Collision_ComputeBroadPhasePairsBruteForceInternal(proxies, outPairs, scratch, metrics, filter);
}

} // namespace Olympe
