#pragma once

#include "../../third_party/imgui/imgui.h"

namespace Olympe
{
    namespace CanvasHitTesting
    {
        inline bool ContainsPoint(
            const ImVec2& point,
            const ImVec2& boundsMin,
            const ImVec2& boundsMax)
        {
            return point.x >= boundsMin.x && point.x <= boundsMax.x &&
                   point.y >= boundsMin.y && point.y <= boundsMax.y;
        }

        inline bool ContainsPointInCircle(
            const ImVec2& point,
            const ImVec2& center,
            float radius)
        {
            const float deltaX = point.x - center.x;
            const float deltaY = point.y - center.y;
            return deltaX * deltaX + deltaY * deltaY <= radius * radius;
        }

        inline bool IntersectsRectangle(
            const ImVec2& firstMin,
            const ImVec2& firstMax,
            const ImVec2& secondMin,
            const ImVec2& secondMax)
        {
            return !(firstMax.x < secondMin.x || firstMin.x > secondMax.x ||
                     firstMax.y < secondMin.y || firstMin.y > secondMax.y);
        }
    }
}
