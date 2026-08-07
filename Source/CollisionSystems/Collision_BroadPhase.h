#pragma once

#include "Collision_SpatialProxyRegistry.h"
#include "../ECS_Entity.h"

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

// Computes broad-phase candidate pairs from spatial proxies.
// Contract:
// - outPairs is cleared on entry.
// - pairs are normalized (a < b), self-pairs are skipped, duplicates are removed.
// - sweep uses epsilon-expanded overlap checks.
// - output order is deterministic: sorted by a ascending, then b ascending.
void Collision_ComputeBroadPhasePairs(const std::vector<Collision_SpatialProxy>& proxies,
                                      std::vector<Collision_CandidatePair>& outPairs);

} // namespace Olympe
