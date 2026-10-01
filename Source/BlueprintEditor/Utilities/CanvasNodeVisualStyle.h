#pragma once

#include "../../third_party/imgui/imgui.h"

namespace Olympe
{
    /** Shared two-tone node frame: semantic title color over a neutral body. */
    struct CanvasNodeVisualStyle
    {
        ImU32 titleBarColor = IM_COL32(75, 105, 150, 255);
        ImU32 bodyColor = IM_COL32(48, 48, 48, 255);
        ImU32 borderColor = IM_COL32(130, 130, 130, 255);
        ImU32 hoveredBorderColor = IM_COL32(255, 200, 0, 255);
        ImU32 selectedBorderColor = IM_COL32(0, 255, 255, 255);
        ImU32 selectedGlowColor = IM_COL32(0, 200, 255, 100);
        ImU32 hoveredGlowColor = IM_COL32(255, 200, 0, 150);
        float titleBarHeight = 25.0f;
        float rounding = 4.0f;
    };

    /** Draw the generic readability-focused node frame used by custom canvases. */
    inline void RenderTwoToneNodeFrame(
        ImDrawList* drawList,
        const ImVec2& min,
        const ImVec2& max,
        const CanvasNodeVisualStyle& style,
        bool selected,
        bool hovered)
    {
        if (!drawList)
            return;

        if (selected)
        {
            drawList->AddRect(
                ImVec2(min.x - 3.0f, min.y - 3.0f),
                ImVec2(max.x + 3.0f, max.y + 3.0f),
                style.selectedGlowColor, style.rounding,
                ImDrawFlags_RoundCornersAll, 1.0f);
        }
        else if (hovered)
        {
            drawList->AddRect(
                ImVec2(min.x - 2.0f, min.y - 2.0f),
                ImVec2(max.x + 2.0f, max.y + 2.0f),
                style.hoveredGlowColor, style.rounding,
                ImDrawFlags_RoundCornersAll, 1.0f);
        }

        drawList->AddRectFilled(min, max, style.bodyColor, style.rounding);
        const ImVec2 titleMax(max.x, (min.y + style.titleBarHeight < max.y) ? min.y + style.titleBarHeight : max.y);
        drawList->AddRectFilled(min, titleMax, style.titleBarColor, style.rounding,
            ImDrawFlags_RoundCornersTop);

        const ImU32 borderColor = selected ? style.selectedBorderColor :
            (hovered ? style.hoveredBorderColor : style.borderColor);
        const float borderWidth = selected ? 3.0f : (hovered ? 2.5f : 1.5f);
        drawList->AddRect(min, max, borderColor, style.rounding,
            ImDrawFlags_RoundCornersAll, borderWidth);
    }
}
