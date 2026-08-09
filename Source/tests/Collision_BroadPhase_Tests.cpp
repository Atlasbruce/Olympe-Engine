#include "../CollisionSystems/Collision_BroadPhase.h"

#include "../system/system_utils.h"

#include <vector>

static bool AssertTrue(bool condition, const char* label)
{
    if (!condition)
    {
        SYSTEM_LOG << "[CollisionBroadPhaseTest] FAILED: " << label << std::endl;
        return false;
    }

    return true;
}

static Olympe::Collision_AABB MakeAABB(float minX, float minY, float maxX, float maxY)
{
    Olympe::Collision_AABB box;
    box.min.x = minX;
    box.min.y = minY;
    box.min.z = 0.0f;
    box.max.x = maxX;
    box.max.y = maxY;
    box.max.z = 0.0f;
    return box;
}

static bool RejectSpecificPair(const Olympe::Collision_SpatialProxy& lhs,
                               const Olympe::Collision_SpatialProxy& rhs,
                               void* userData)
{
    const EntityID* ids = static_cast<const EntityID*>(userData);
    const EntityID first = (lhs.entity < rhs.entity) ? lhs.entity : rhs.entity;
    const EntityID second = (lhs.entity < rhs.entity) ? rhs.entity : lhs.entity;
    return !(first == ids[0] && second == ids[1]);
}

static bool SamePairs(const std::vector<Olympe::Collision_CandidatePair>& lhs,
                      const std::vector<Olympe::Collision_CandidatePair>& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (size_t index = 0; index < lhs.size(); ++index)
    {
        if (lhs[index].a != rhs[index].a || lhs[index].b != rhs[index].b)
        {
            return false;
        }
    }

    return true;
}

int main()
{
    bool ok = true;

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;

        Olympe::Collision_SpatialProxy a;
        a.entity = 1;
        a.worldAABB = MakeAABB(0.0f, 0.0f, 1.0f, 1.0f);
        proxies.push_back(a);

        Olympe::Collision_SpatialProxy b;
        b.entity = 2;
        b.worldAABB = MakeAABB(2.0f, 0.0f, 3.0f, 1.0f);
        proxies.push_back(b);

        std::vector<Olympe::Collision_CandidatePair> sapPairs;
        std::vector<Olympe::Collision_CandidatePair> brutePairs;
        Olympe::Collision_BroadPhaseScratch scratch;
        Olympe::Collision_BroadPhaseMetrics sapMetrics;
        Olympe::Collision_BroadPhaseMetrics bruteMetrics;

        Olympe::Collision_ComputeBroadPhasePairs(proxies, sapPairs, scratch, &sapMetrics, Olympe::Collision_BroadPhaseFilter());
        Olympe::Collision_ComputeBroadPhasePairsBruteForce(proxies, brutePairs, scratch, &bruteMetrics, Olympe::Collision_BroadPhaseFilter());

        ok = AssertTrue(sapPairs.empty(), "No overlap -> SAP empty") && ok;
        ok = AssertTrue(brutePairs.empty(), "No overlap -> brute force empty") && ok;
        ok = AssertTrue(SamePairs(sapPairs, brutePairs), "No overlap -> same outputs") && ok;
        ok = AssertTrue(sapMetrics.invalidProxyCount == 0, "No overlap -> no invalid proxies") && ok;
    }

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;
        for (EntityID i = 1; i <= 32; ++i)
        {
            Olympe::Collision_SpatialProxy proxy;
            proxy.entity = i;

            const float left = static_cast<float>(i - 1) * 0.75f;
            const float right = left + 1.0f;
            proxy.worldAABB = MakeAABB(left, 0.0f, right, 1.0f);
            proxies.push_back(proxy);
        }

        std::vector<Olympe::Collision_CandidatePair> sapPairs;
        std::vector<Olympe::Collision_CandidatePair> brutePairs;
        Olympe::Collision_BroadPhaseScratch scratch;
        Olympe::Collision_BroadPhaseMetrics sapMetrics;
        Olympe::Collision_BroadPhaseMetrics bruteMetrics;

        Olympe::Collision_ComputeBroadPhasePairs(proxies, sapPairs, scratch, &sapMetrics, Olympe::Collision_BroadPhaseFilter());
        Olympe::Collision_ComputeBroadPhasePairsBruteForce(proxies, brutePairs, scratch, &bruteMetrics, Olympe::Collision_BroadPhaseFilter());

        ok = AssertTrue(SamePairs(sapPairs, brutePairs), "Chain overlap -> SAP matches brute force") && ok;
        ok = AssertTrue(sapPairs.size() == 31, "Chain overlap -> expected pair count") && ok;
        ok = AssertTrue(sapMetrics.comparisons > 0, "Chain overlap -> sweep comparisons recorded") && ok;
        ok = AssertTrue(sapMetrics.maxActiveCount > 0, "Chain overlap -> active window recorded") && ok;
        ok = AssertTrue(bruteMetrics.comparisons > sapMetrics.comparisons,
                        "Chain overlap -> brute force does more comparisons") && ok;
    }

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;

        Olympe::Collision_SpatialProxy a;
        a.entity = 100;
        a.worldAABB = MakeAABB(0.0f, 0.0f, 1.0f, 1.0f);
        proxies.push_back(a);

        Olympe::Collision_SpatialProxy b;
        b.entity = 200;
        b.worldAABB = MakeAABB(1.0f, 0.0f, 2.0f, 1.0f);
        proxies.push_back(b);

        EntityID rejectedPair[2] = {100, 200};
        Olympe::Collision_BroadPhaseFilter filter;
        filter.predicate = &RejectSpecificPair;
        filter.userData = rejectedPair;

        std::vector<Olympe::Collision_CandidatePair> sapPairs;
        std::vector<Olympe::Collision_CandidatePair> brutePairs;
        Olympe::Collision_BroadPhaseScratch scratch;
        Olympe::Collision_BroadPhaseMetrics sapMetrics;
        Olympe::Collision_BroadPhaseMetrics bruteMetrics;

        Olympe::Collision_ComputeBroadPhasePairs(proxies, sapPairs, scratch, &sapMetrics, filter);
        Olympe::Collision_ComputeBroadPhasePairsBruteForce(proxies, brutePairs, scratch, &bruteMetrics, filter);

        ok = AssertTrue(sapPairs.empty(), "Filter -> SAP rejects pair") && ok;
        ok = AssertTrue(brutePairs.empty(), "Filter -> brute force rejects pair") && ok;
        ok = AssertTrue(sapMetrics.filteredPairs == 1, "Filter -> metrics count rejected pair") && ok;
    }

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;

        Olympe::Collision_SpatialProxy valid;
        valid.entity = 2;
        valid.worldAABB = MakeAABB(0.0f, 0.0f, 1.0f, 1.0f);
        proxies.push_back(valid);

        Olympe::Collision_SpatialProxy invalid;
        invalid.entity = 3;
        invalid.worldAABB = MakeAABB(5.0f, 5.0f, 2.0f, 2.0f);
        proxies.push_back(invalid);

        std::vector<Olympe::Collision_CandidatePair> sapPairs;
        Olympe::Collision_BroadPhaseScratch scratch;
        Olympe::Collision_BroadPhaseMetrics metrics;
        Olympe::Collision_ComputeBroadPhasePairs(proxies, sapPairs, scratch, &metrics, Olympe::Collision_BroadPhaseFilter());

        ok = AssertTrue(sapPairs.empty(), "Invalid AABB -> no pairs") && ok;
        ok = AssertTrue(metrics.invalidProxyCount == 1, "Invalid AABB -> counted") && ok;
        ok = AssertTrue(metrics.validProxyCount == 1, "Invalid AABB -> one valid proxy") && ok;
    }

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;
        for (EntityID i = 1; i <= 8; ++i)
        {
            Olympe::Collision_SpatialProxy proxy;
            proxy.entity = i;
            proxy.worldAABB = MakeAABB(static_cast<float>(i - 1), 0.0f, static_cast<float>(i), 1.0f);
            proxies.push_back(proxy);
        }

        std::vector<Olympe::Collision_CandidatePair> pairs;
        Olympe::Collision_BroadPhaseScratch scratch;
        Olympe::Collision_BroadPhaseMetrics metrics;

        Olympe::Collision_ComputeBroadPhasePairs(proxies, pairs, scratch, &metrics, Olympe::Collision_BroadPhaseFilter());
        const size_t sortedCapacity = scratch.sortedProxies.capacity();
        const size_t pairCapacity = scratch.normalizedPairs.capacity();

        Olympe::Collision_ComputeBroadPhasePairs(proxies, pairs, scratch, &metrics, Olympe::Collision_BroadPhaseFilter());

        ok = AssertTrue(scratch.sortedProxies.capacity() == sortedCapacity, "Scratch reuse -> sorted capacity stable") && ok;
        ok = AssertTrue(scratch.normalizedPairs.capacity() == pairCapacity, "Scratch reuse -> pair capacity stable") && ok;
    }

    if (!ok)
    {
        SYSTEM_LOG << "[CollisionBroadPhaseTest] FAIL" << std::endl;
        return 1;
    }

    SYSTEM_LOG << "[CollisionBroadPhaseTest] PASS" << std::endl;
    return 0;
}
