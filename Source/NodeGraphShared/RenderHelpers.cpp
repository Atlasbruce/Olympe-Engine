/**
 * @file RenderHelpers.cpp
 * @brief Implementation of rendering helper functions for node graphs
 */

#include "RenderHelpers.h"
#include "../AI/BehaviorTree.h"
#include "../third_party/imnodes/imnodes.h"

namespace Olympe {
namespace NodeGraphShared {

ImVec2 ComputePinCenterScreen(
    int nodeId,
    const BTNodeLayout* layout,
    bool isOutput,
    float pinOffset,
    float headerHeight,
    float zoom
)
{
    // Get node's screen-space position and dimensions from ImNodes
    ImVec2 nodeScreenPos = ImNodes::GetNodeScreenSpacePos(nodeId);
    ImVec2 nodeDimensions = ImNodes::GetNodeDimensions(nodeId);
    
    // Calculate pin Y position: below the header, centered on the "In"/"Out" text row
    // The header height determines where the first row of content begins
    float pinY = nodeScreenPos.y + headerHeight;
    
    float pinX;
    if (isOutput)
    {
        // Output pin: right edge of node + offset
        pinX = nodeScreenPos.x + nodeDimensions.x + pinOffset;
    }
    else
    {
        // Input pin: left edge of node - offset
        pinX = nodeScreenPos.x - pinOffset;
    }
    
    return ImVec2(pinX, pinY);
}

void DrawPinCircle(
    ImDrawList* drawList,
    const ImVec2& center,
    float radius,
    uint32_t color,
    uint32_t outlineColor,
    float outlineThickness
)
{
    if (!drawList)
        return;
    
    // Draw outline first (larger circle)
    float outerRadius = radius + outlineThickness;
    drawList->AddCircleFilled(center, outerRadius, outlineColor);
    
    // Draw inner circle on top
    drawList->AddCircleFilled(center, radius, color);
}

} // namespace NodeGraphShared
} // namespace Olympe
