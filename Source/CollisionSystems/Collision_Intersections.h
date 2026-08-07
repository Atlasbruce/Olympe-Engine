#pragma once

#include "Collision_Primitives.h"

namespace Olympe {

bool Collision_Intersects_PointPoint(const Collision_Point& a,
                                     const Collision_Point& b);

bool Collision_Intersects_PointCircle(const Collision_Point& point,
                                      const Collision_Circle& circle);

bool Collision_Intersects_CircleCircle(const Collision_Circle& a,
                                       const Collision_Circle& b);

bool Collision_Intersects_CircleAABB(const Collision_Circle& circle,
                                     const Collision_AABB& box);

bool Collision_Intersects_AABBAABB(const Collision_AABB& a,
                                   const Collision_AABB& b);

} // namespace Olympe
