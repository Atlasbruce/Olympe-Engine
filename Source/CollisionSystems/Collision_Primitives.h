#pragma once

#include "../vector.h"

namespace Olympe {

struct Collision_Point
{
    Vector position;

    Collision_Point()
    {
        position.x = 0.0f;
        position.y = 0.0f;
        position.z = 0.0f;
    }
};

struct Collision_Circle
{
    Vector center;
    float radius;

    Collision_Circle()
        : radius(0.0f)
    {
        center.x = 0.0f;
        center.y = 0.0f;
        center.z = 0.0f;
    }
};

struct Collision_AABB
{
    Vector min;
    Vector max;

    Collision_AABB()
    {
        min.x = min.y = min.z = 0.0f;
        max.x = max.y = max.z = 0.0f;
    }
};

struct Collision_ContactResult
{
    bool intersects;
    Vector point;
    Vector normal;
    float penetration;

    Collision_ContactResult()
        : intersects(false)
        , penetration(0.0f)
    {
        point.x = point.y = point.z = 0.0f;
        normal.x = normal.y = normal.z = 0.0f;
    }
};

bool Collision_Is2DVector(const Vector& v);
bool Collision_IsValidAABB(const Collision_AABB& box);
Collision_AABB Collision_ToWorldAABB(const Collision_AABB& localBounds,
                                     const Vector& worldPosition,
                                     const Vector& offset);

} // namespace Olympe
