#include "Level.h"
#include "CollisionMap.h"

bool LevelCollisionMapResource::IsSolidCell(int x, int y) const
{
 if (x < 0 || y < 0 || x >= width || y >= height)
 {
  return true;
 }
 const size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
 if (index >= solidCells.size())
 {
  return true;
 }
 return solidCells[index] != 0;
}

bool LevelCollisionMapResource::TestAABB(const AABB& bounds) const
{
 int minX = static_cast<int>(bounds.min.x);
 int minY = static_cast<int>(bounds.min.y);
 int maxX = static_cast<int>(bounds.max.x);
 int maxY = static_cast<int>(bounds.max.y);
 for (int y = minY; y <= maxY; ++y)
 {
  for (int x = minX; x <= maxX; ++x)
  {
   if (IsSolidCell(x, y))
   {
    return true;
   }
  }
 }
 return false;
}

bool LevelNavigationMapResource::IsWalkableCell(int x, int y) const
{
 if (x < 0 || y < 0 || x >= width || y >= height)
 {
  return false;
 }
 const size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
 if (index >= walkableCells.size())
 {
  return false;
 }
 return walkableCells[index] != 0;
}
