#pragma once

#include "../third_party/imgui/imgui.h"
#include "../AI/BTGraphLayoutEngine.h"
#include "../third_party/imnodes/imnodes.h"
#include "../BlueprintEditor/NodeStyleRegistry.h"
#include <algorithm>
#include <string>

namespace Olympe
{
    namespace NodeGraphShared
    {
        inline ImVec2 ComputePinCenterScreen(int nodeId, const BTNodeLayout* layout, bool isOutput, float pinOffset, float headerPx, float currentZoom)
        {
            ImVec2 nPos = ImNodes::GetNodeScreenSpacePos(nodeId);
            ImVec2 nDim = ImNodes::GetNodeDimensions(nodeId);

            // Clamp headerPx
            float h = headerPx * currentZoom;
            h = std::max(4.0f, std::min(h, nDim.y - 4.0f));
            float pinY = nPos.y + h;

            if (isOutput)
            {
                return ImVec2(nPos.x + nDim.x - pinOffset, pinY);
            }
            else
            {
                return ImVec2(nPos.x + pinOffset, pinY);
            }
        }

        inline void DrawPinCircle(ImDrawList* drawList, const ImVec2& center, float radius, ImU32 fillColor, ImU32 outlineColor, float outlineThickness)
        {
            if (!drawList) return;
            drawList->AddCircleFilled(center, radius + outlineThickness, outlineColor);
            drawList->AddCircleFilled(center, radius, fillColor);
        }

        inline void RenderNodeHeader(int nodeId, const NodeStyle& style, const char* icon, const std::string& title, bool isCurrentNode, float pulseTimer)
        {
            // NOTE: Do not call BeginNode/BeginNodeTitleBar here. The caller
            // (`RenderNodeVisual`) is responsible for opening the node and title
            // bar scopes. Calling them here caused nested/duplicate scope
            // transitions and assertion failures.

            ImU32 headerColor = style.headerColor;
            ImU32 headerHoveredColor = style.headerHoveredColor;
            ImU32 headerSelectedColor = style.headerSelectedColor;

            if (isCurrentNode)
            {
                float t = 0.5f + 0.5f * sinf(pulseTimer * 2.0f * 3.14159265f * 2.0f);
                ImU32 r = static_cast<ImU32>(180 + static_cast<int>(t * 75.0f));
                ImU32 g = static_cast<ImU32>(140 + static_cast<int>(t * 115.0f));
                ImU32 b = static_cast<ImU32>(10);
                headerColor = IM_COL32(r, g, b, 255);
                headerHoveredColor = IM_COL32(r, g, b, 230);
                headerSelectedColor = IM_COL32(r, g, b, 210);
            }

            ImNodes::PushColorStyle(ImNodesCol_TitleBar, headerColor);
            ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, headerHoveredColor);
            ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, headerSelectedColor);

            if (icon && icon[0] != '\0')
                ImGui::Text("%s %s", icon, title.c_str());
            else
                ImGui::TextUnformatted(title.c_str());

            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();

            // The caller must call EndNodeTitleBar().
        }
    }
}
