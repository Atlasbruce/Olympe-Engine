#pragma once

#include "../third_party/imgui/imgui.h"
#include "../AI/BTGraphLayoutEngine.h"

namespace Olympe
{
    namespace NodeGraphShared
    {
        // Compute a pin center in screen-space for a node given its ImNodes
        // screen-space node position and dimensions. If isOutput is true the
        // pin will be placed at the right-side inside the box; otherwise left.
        ImVec2 ComputePinCenterScreen(int nodeId, const BTNodeLayout* layout, bool isOutput, float pinOffset, float headerPx, float currentZoom);

        // Draw a pin (filled circle with outline) at screen position
        void DrawPinCircle(ImDrawList* drawList, const ImVec2& center, float radius, ImU32 fillColor, ImU32 outlineColor, float outlineThickness);
    }
}
