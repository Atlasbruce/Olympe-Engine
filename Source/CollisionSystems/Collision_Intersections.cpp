#include "Collision_Intersections.h"

#include "../system/system_utils.h"

namespace Olympe {

static const float COLLISION_EPSILON = 0.0001f;

static float Collision_Abs(float v)
{
    return (v < 0.0f) ? -v : v;
}

static float Collision_Max(float a, float b)
{
    return (a > b) ? a : b;
}

static float Collision_Min(float a, float b)
{
    return (a < b) ? a : b;
}

static float Collision_Clamp(float value, float minVal, float maxVal)
{
    return Collision_Max(minVal, Collision_Min(value, maxVal));
}

bool Collision_Intersects_PointPoint(const Collision_Point& a,
                                     const Collision_Point& b)
{
    (void)Collision_Is2DVector(a.position);
    (void)Collision_Is2DVector(b.position);

    const float dx = a.position.x - b.position.x;
    const float dy = a.position.y - b.position.y;

    return (Collision_Abs(dx) <= COLLISION_EPSILON) &&
           (Collision_Abs(dy) <= COLLISION_EPSILON);
}

bool Collision_Intersects_PointCircle(const Collision_Point& point,
                                      const Collision_Circle& circle)
{
    (void)Collision_Is2DVector(point.position);
    (void)Collision_Is2DVector(circle.center);

    if (circle.radius < 0.0f)
    {
        SYSTEM_LOG << "[Collision] Invalid circle radius < 0" << std::endl;
        return false;
    }

    const float dx = point.position.x - circle.center.x;
    const float dy = point.position.y - circle.center.y;
    const float distSq = (dx * dx) + (dy * dy);
    const float radiusSq = circle.radius * circle.radius;

    return distSq <= (radiusSq + COLLISION_EPSILON);
}

bool Collision_Intersects_CircleCircle(const Collision_Circle& a,
                                       const Collision_Circle& b)
{
    (void)Collision_Is2DVector(a.center);
    (void)Collision_Is2DVector(b.center);

    if (a.radius < 0.0f || b.radius < 0.0f)
    {
        SYSTEM_LOG << "[Collision] Invalid circle radius < 0" << std::endl;
        return false;
    }

    const float dx = a.center.x - b.center.x;
    const float dy = a.center.y - b.center.y;
    const float distSq = (dx * dx) + (dy * dy);
    const float sumR = a.radius + b.radius;

    return distSq <= ((sumR * sumR) + COLLISION_EPSILON);
}

bool Collision_Intersects_CircleAABB(const Collision_Circle& circle,
                                     const Collision_AABB& box)
{
    (void)Collision_Is2DVector(circle.center);

    if (!Collision_IsValidAABB(box))
    {
        return false;
    }

    if (circle.radius < 0.0f)
    {
        SYSTEM_LOG << "[Collision] Invalid circle radius < 0" << std::endl;
        return false;
    }

    const float closestX = Collision_Clamp(circle.center.x, box.min.x, box.max.x);
    const float closestY = Collision_Clamp(circle.center.y, box.min.y, box.max.y);

    const float dx = circle.center.x - closestX;
    const float dy = circle.center.y - closestY;

    const float distSq = (dx * dx) + (dy * dy);
    const float radiusSq = circle.radius * circle.radius;

    return distSq <= (radiusSq + COLLISION_EPSILON);
}

bool Collision_Intersects_AABBAABB(const Collision_AABB& a,
                                   const Collision_AABB& b)
{
    if (!Collision_IsValidAABB(a) || !Collision_IsValidAABB(b))
    {
        return false;
    }

    const bool overlapX = (a.min.x <= b.max.x + COLLISION_EPSILON) &&
                          (a.max.x + COLLISION_EPSILON >= b.min.x);

    const bool overlapY = (a.min.y <= b.max.y + COLLISION_EPSILON) &&
                          (a.max.y + COLLISION_EPSILON >= b.min.y);

    return overlapX && overlapY;
}

} // namespace Olympe
