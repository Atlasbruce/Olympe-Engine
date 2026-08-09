#pragma once

#include "Collision_SpatialProxyRegistry.h"
#include "../ECS_Entity.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace Olympe {

struct Collision_CandidatePair
{
    EntityID a;
    EntityID b;

    Collision_CandidatePair()
        : a(INVALID_ENTITY_ID)
        , b(INVALID_ENTITY_ID)
    {
    }
};

struct Collision_BroadPhaseMetrics
{
    size_t inputProxyCount;
    size_t validProxyCount;
    size_t invalidProxyCount;
    size_t comparisons;
    size_t pairsBeforeDedup;
    size_t pairsAfterDedup;
    size_t duplicatePairs;
    size_t filteredPairs;
    size_t selfRejectedPairs;
    size_t axisRejectedPairs;
    size_t maxActiveCount;
    double elapsedMs;

    Collision_BroadPhaseMetrics()
    {
        Reset();
    }

    void Reset()
    {
        inputProxyCount = 0;
        validProxyCount = 0;
        invalidProxyCount = 0;
        comparisons = 0;
        pairsBeforeDedup = 0;
        pairsAfterDedup = 0;
        duplicatePairs = 0;
        filteredPairs = 0;
        selfRejectedPairs = 0;
        axisRejectedPairs = 0;
        maxActiveCount = 0;
        elapsedMs = 0.0;
    }
};

struct Collision_BroadPhaseScratch
{
    std::vector<Collision_SpatialProxy> sortedProxies;
    std::vector<Collision_SpatialProxy> activeProxies;
    std::vector<std::pair<EntityID, EntityID> > normalizedPairs;

    void Clear()
    {
        sortedProxies.clear();
        activeProxies.clear();
        normalizedPairs.clear();
    }
};

struct Collision_BroadPhaseFilter
{
    typedef bool (*Predicate)(const Collision_SpatialProxy& lhs,
                              const Collision_SpatialProxy& rhs,
                              void* userData);

    Predicate predicate;
    void* userData;

    Collision_BroadPhaseFilter()
        : predicate(0)
        , userData(0)
    {
    }

    bool Allows(const Collision_SpatialProxy& lhs,
                const Collision_SpatialProxy& rhs) const;
};

// Computes broad-phase candidate pairs from spatial proxies.
// Contract:
// - outPairs is cleared on entry.
// - pairs are normalized (a < b), self-pairs are skipped, duplicates are removed.
// - sweep uses epsilon-expanded overlap checks.
// - output order is deterministic: sorted by a ascending, then b ascending.
void Collision_ComputeBroadPhasePairs(const std::vector<Collision_SpatialProxy>& proxies,
                                      std::vector<Collision_CandidatePair>& outPairs);

void Collision_ComputeBroadPhasePairs(const std::vector<Collision_SpatialProxy>& proxies,
                                      std::vector<Collision_CandidatePair>& outPairs,
                                      Collision_BroadPhaseScratch& scratch,
                                      Collision_BroadPhaseMetrics* metrics,
                                      const Collision_BroadPhaseFilter& filter);

void Collision_ComputeBroadPhasePairsBruteForce(const std::vector<Collision_SpatialProxy>& proxies,
                                                std::vector<Collision_CandidatePair>& outPairs,
                                                Collision_BroadPhaseScratch& scratch,
                                                Collision_BroadPhaseMetrics* metrics,
                                                const Collision_BroadPhaseFilter& filter);

} // namespace Olympe
