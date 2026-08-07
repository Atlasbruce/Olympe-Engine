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

void Collision_ComputeBroadPhasePairs(const std::vector<Collision_SpatialProxy>& proxies,
                                      std::vector<Collision_CandidatePair>& outPairs);

} // namespace Olympe
