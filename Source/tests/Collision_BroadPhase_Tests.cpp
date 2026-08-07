#include "../CollisionSystems/Collision_SpatialProxyRegistry.h"
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

        std::vector<Olympe::Collision_CandidatePair> pairs;
        Olympe::Collision_ComputeBroadPhasePairs(proxies, pairs);

        ok = AssertTrue(pairs.empty(), "No overlap -> 0 pair") && ok;
    }

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;

        Olympe::Collision_SpatialProxy a;
        a.entity = 10;
        a.worldAABB = MakeAABB(0.0f, 0.0f, 2.0f, 2.0f);
        proxies.push_back(a);

        Olympe::Collision_SpatialProxy b;
        b.entity = 20;
        b.worldAABB = MakeAABB(1.0f, 1.0f, 3.0f, 3.0f);
        proxies.push_back(b);

        std::vector<Olympe::Collision_CandidatePair> pairs;
        Olympe::Collision_ComputeBroadPhasePairs(proxies, pairs);

        ok = AssertTrue(pairs.size() == 1, "Single overlap -> 1 pair") && ok;
        ok = AssertTrue(pairs[0].a == 10 && pairs[0].b == 20,
                        "Single overlap expected pair") && ok;
    }

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;
        for (EntityID i = 1; i <= 128; ++i)
        {
            Olympe::Collision_SpatialProxy proxy;
            proxy.entity = i;

            const float left = static_cast<float>(i - 1) * 0.75f;
            const float right = left + 1.0f;

            proxy.worldAABB = MakeAABB(left, 0.0f, right, 1.0f);
            proxies.push_back(proxy);
        }

        std::vector<Olympe::Collision_CandidatePair> pairs;
        Olympe::Collision_ComputeBroadPhasePairs(proxies, pairs);

        ok = AssertTrue(pairs.size() == 127, "Chain overlap count is deterministic") && ok;

        bool ordered = true;
        for (size_t index = 0; index < pairs.size(); ++index)
        {
            const EntityID expectedA = static_cast<EntityID>(index + 1);
            const EntityID expectedB = static_cast<EntityID>(index + 2);
            if (pairs[index].a != expectedA || pairs[index].b != expectedB)
            {
                ordered = false;
                break;
            }
        }

        ok = AssertTrue(ordered, "Chain overlap pair ordering deterministic") && ok;
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

        std::vector<Olympe::Collision_CandidatePair> pairs;
        Olympe::Collision_ComputeBroadPhasePairs(proxies, pairs);

        ok = AssertTrue(pairs.size() == 1 && pairs[0].a == 100 && pairs[0].b == 200,
                        "Edge touching with epsilon -> pair exists") && ok;
    }

    {
        Olympe::Collision_SpatialProxyRegistry registry;

        registry.RegisterOrUpdateProxy(1, MakeAABB(5.0f, 5.0f, 3.0f, 6.0f));
        registry.RegisterOrUpdateProxy(2, MakeAABB(0.0f, 0.0f, 1.0f, 1.0f));

        std::vector<Olympe::Collision_SpatialProxy> snapshot;
        registry.GetSnapshot(snapshot);

        ok = AssertTrue(snapshot.size() == 1, "Invalid AABB ignored by registry") && ok;
        ok = AssertTrue(snapshot[0].entity == 2, "Registry snapshot keeps valid entity") && ok;
    }

    {
        std::vector<Olympe::Collision_SpatialProxy> proxies;

        Olympe::Collision_SpatialProxy first;
        first.entity = 10;
        first.worldAABB = MakeAABB(0.0f, 0.0f, 2.0f, 2.0f);
        proxies.push_back(first);

        Olympe::Collision_SpatialProxy duplicateSameEntity;
        duplicateSameEntity.entity = 10;
        duplicateSameEntity.worldAABB = MakeAABB(0.5f, 0.0f, 2.5f, 2.0f);
        proxies.push_back(duplicateSameEntity);

        Olympe::Collision_SpatialProxy other;
        other.entity = 20;
        other.worldAABB = MakeAABB(1.0f, 0.0f, 3.0f, 2.0f);
        proxies.push_back(other);

        std::vector<Olympe::Collision_CandidatePair> pairs;
        Olympe::Collision_ComputeBroadPhasePairs(proxies, pairs);

        ok = AssertTrue(pairs.size() == 1, "Dedup/self-pair not emitted") && ok;
        ok = AssertTrue(pairs[0].a == 10 && pairs[0].b == 20,
                        "Dedup/self-pair expected output") && ok;
    }

    if (!ok)
    {
        SYSTEM_LOG << "[CollisionBroadPhaseTest] FAIL" << std::endl;
        return 1;
    }

    SYSTEM_LOG << "[CollisionBroadPhaseTest] PASS" << std::endl;
    return 0;
}
