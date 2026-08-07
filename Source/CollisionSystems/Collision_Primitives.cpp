#include "Collision_Primitives.h"

#include "../system/system_utils.h"

namespace Olympe {

static const float COLLISION_EPSILON = 0.0001f;

bool Collision_Is2DVector(const Vector& v)
{
    if (v.z < -COLLISION_EPSILON || v.z > COLLISION_EPSILON)
    {
        SYSTEM_LOG << "[Collision] Warning: Vector.z is not 0 in 2D collision path: z="
                   << v.z << std::endl;
        return false;
    }
    return true;
}

bool Collision_IsValidAABB(const Collision_AABB& box)
{
    const bool minXOk = box.min.x <= box.max.x;
    const bool minYOk = box.min.y <= box.max.y;
    const bool minZOk = box.min.z <= box.max.z;

    if (!minXOk || !minYOk || !minZOk)
    {
        SYSTEM_LOG << "[Collision] Invalid AABB: min must be <= max on all axes" << std::endl;
        return false;
    }

    return true;
}

Collision_AABB Collision_ToWorldAABB(const Collision_AABB& localBounds,
                                     const Vector& worldPosition,
                                     const Vector& offset)
{
    (void)Collision_Is2DVector(worldPosition);
    (void)Collision_Is2DVector(offset);

    Collision_AABB world;

    world.min.x = worldPosition.x + offset.x + localBounds.min.x;
    world.min.y = worldPosition.y + offset.y + localBounds.min.y;
    world.min.z = 0.0f;

    world.max.x = worldPosition.x + offset.x + localBounds.max.x;
    world.max.y = worldPosition.y + offset.y + localBounds.max.y;
    world.max.z = 0.0f;

    if (!Collision_IsValidAABB(world))
    {
        SYSTEM_LOG << "[Collision] Collision_ToWorldAABB produced invalid bounds" << std::endl;
    }

    return world;
}

} // namespace Olympe
