#include "NodeGraphShared.h"
#include "../third_party/imnodes/imnodes.h"

namespace Olympe
{
    namespace NodeGraphShared
    {
        ImVec2 ComputePinCenterScreen(int nodeId, const BTNodeLayout* layout, bool isOutput, float pinOffset, float headerPx, float currentZoom)
        {
            ImVec2 nPos = ImNodes::GetNodeScreenSpacePos(nodeId);
            ImVec2 nDim = ImNodes::GetNodeDimensions(nodeId);

            // Clamp headerPx
            float h = headerPx * currentZoom;
            h = std::max(4.0f, std::min(h, nDim.y - 4.0f));
            float pinY = nPos.y + h;

            if (isOutput)
            {
                // inside right side
                return ImVec2(nPos.x + nDim.x - pinOffset, pinY);
            }
            else
            {
                // inside left side
                return ImVec2(nPos.x + pinOffset, pinY);
            }
        }

        void DrawPinCircle(ImDrawList* drawList, const ImVec2& center, float radius, ImU32 fillColor, ImU32 outlineColor, float outlineThickness)
        {
            if (!drawList) return;
            drawList->AddCircleFilled(center, radius + outlineThickness, outlineColor);
            drawList->AddCircleFilled(center, radius, fillColor);
        }
    }
}
