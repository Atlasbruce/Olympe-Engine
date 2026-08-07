#include "../CollisionSystems/Collision_Primitives.h"
#include "../CollisionSystems/Collision_Intersections.h"

#include <iostream>

static bool AssertTrue(bool condition, const char* label)
{
    if (!condition)
    {
        std::cerr << "[CollisionTest] FAILED: " << label << std::endl;
        return false;
    }
    return true;
}

int main()
{
    bool ok = true;

    {
        Olympe::Collision_Point a;
        a.position.x = 10.0f;
        a.position.y = 20.0f;

        Olympe::Collision_Point b = a;
        ok = AssertTrue(Olympe::Collision_Intersects_PointPoint(a, b), "PointPoint equal") && ok;
    }

    {
        Olympe::Collision_Circle c;
        c.center.x = 0.0f;
        c.center.y = 0.0f;
        c.radius = 5.0f;

        Olympe::Collision_Point p;
        p.position.x = 3.0f;
        p.position.y = 4.0f;

        ok = AssertTrue(Olympe::Collision_Intersects_PointCircle(p, c), "PointCircle inside") && ok;
    }

    {
        Olympe::Collision_Circle a;
        a.center.x = 0.0f;
        a.center.y = 0.0f;
        a.radius = 4.0f;

        Olympe::Collision_Circle b;
        b.center.x = 7.0f;
        b.center.y = 0.0f;
        b.radius = 3.0f;

        ok = AssertTrue(Olympe::Collision_Intersects_CircleCircle(a, b), "CircleCircle touching") && ok;
    }

    {
        Olympe::Collision_AABB box;
        box.min.x = -1.0f;
        box.min.y = -1.0f;
        box.max.x = 1.0f;
        box.max.y = 1.0f;

        Olympe::Collision_Circle c;
        c.center.x = 2.0f;
        c.center.y = 0.0f;
        c.radius = 1.1f;

        ok = AssertTrue(Olympe::Collision_Intersects_CircleAABB(c, box), "CircleAABB overlap") && ok;
    }

    {
        Olympe::Collision_AABB a;
        a.min.x = 0.0f; a.min.y = 0.0f;
        a.max.x = 2.0f; a.max.y = 2.0f;

        Olympe::Collision_AABB b;
        b.min.x = 2.0f; b.min.y = 2.0f;
        b.max.x = 4.0f; b.max.y = 4.0f;

        ok = AssertTrue(Olympe::Collision_Intersects_AABBAABB(a, b), "AABBAABB edge touch") && ok;
    }

    {
        Olympe::Collision_AABB local;
        local.min.x = -1.0f; local.min.y = -2.0f;
        local.max.x = 1.0f;  local.max.y = 2.0f;

        Vector pos;
        pos.x = 10.0f; pos.y = 20.0f; pos.z = 0.0f;

        Vector offset;
        offset.x = 3.0f; offset.y = 4.0f; offset.z = 0.0f;

        Olympe::Collision_AABB world = Olympe::Collision_ToWorldAABB(local, pos, offset);

        ok = AssertTrue(world.min.x == 12.0f && world.min.y == 22.0f &&
                        world.max.x == 14.0f && world.max.y == 26.0f,
                        "ToWorldAABB values") && ok;
    }

    if (!ok)
    {
        std::cerr << "[CollisionTest] FAIL" << std::endl;
        return 1;
    }

    std::cout << "[CollisionTest] PASS" << std::endl;
    return 0;
}
