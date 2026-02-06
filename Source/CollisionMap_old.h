/* CollisionMap.h
 Simple class holding collision/navigation data for a sector.
*/
#pragma once

#include <vector>
#include <iostream>
#include "system/system_utils.h"

class CollisionMap
{
public:
 CollisionMap() { SYSTEM_LOG << "CollisionMap created\n"; }
 ~CollisionMap() { SYSTEM_LOG << "CollisionMap destroyed\n"; }

 // Simple placeholder for collision data
 std::vector<int> data;

 void Clear() { data.clear(); }
};
