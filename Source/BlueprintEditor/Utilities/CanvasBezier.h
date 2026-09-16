#pragma once

#include "../../third_party/imgui/imgui.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Olympe
{
    namespace CanvasBezier
    {
        struct CubicCurve
        {
            ImVec2 start;
            ImVec2 control1;
            ImVec2 control2;
            ImVec2 end;
        };

        inline CubicCurve MakeHorizontalCurve(
            const ImVec2& start,
            const ImVec2& end,
            float controlRatio = 0.4f)
        {
            const float controlOffset = (end.x - start.x) * controlRatio;
            return {
                start,
                ImVec2(start.x + controlOffset, start.y),
                ImVec2(end.x - controlOffset, end.y),
                end
            };
        }

        inline ImVec2 Evaluate(const CubicCurve& curve, float t)
        {
            const float inverseT = 1.0f - t;
            const float inverseTSquared = inverseT * inverseT;
            const float tSquared = t * t;
            return ImVec2(
                inverseTSquared * inverseT * curve.start.x + 3.0f * inverseTSquared * t * curve.control1.x + 3.0f * inverseT * tSquared * curve.control2.x + tSquared * t * curve.end.x,
                inverseTSquared * inverseT * curve.start.y + 3.0f * inverseTSquared * t * curve.control1.y + 3.0f * inverseT * tSquared * curve.control2.y + tSquared * t * curve.end.y);
        }

        inline float DistanceToCurve(const CubicCurve& curve, const ImVec2& point, int samples = 32)
        {
            float minDistance = std::numeric_limits<float>::max();
            const int sampleCount = samples > 0 ? samples : 1;
            for (int i = 0; i <= sampleCount; ++i) {
                const ImVec2 curvePoint = Evaluate(curve, static_cast<float>(i) / static_cast<float>(sampleCount));
                const float deltaX = point.x - curvePoint.x;
                const float deltaY = point.y - curvePoint.y;
                minDistance = std::min(minDistance, std::sqrt(deltaX * deltaX + deltaY * deltaY));
            }
            return minDistance;
        }

        inline void Render(ImDrawList* drawList, const CubicCurve& curve, ImU32 color, float thickness, int segments = 32)
        {
            if (!drawList) return;
            drawList->AddBezierCubic(curve.start, curve.control1, curve.control2, curve.end, color, thickness, segments);
        }
    }
}
