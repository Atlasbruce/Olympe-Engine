/**
 * @file CanvasMinimapRenderer.cpp
 * @brief Centralized minimap rendering implementation
 * @author Olympe Engine
 * @date 2026-03-15
 */

#include "CanvasMinimapRenderer.h"
#include "../../third_party/imnodes/imnodes.h"
#include <algorithm>
#include <cmath>

namespace Olympe
{

CanvasMinimapRenderer::CanvasMinimapRenderer()
    : m_visible(false)
    , m_sizeRatio(0.15f)
    , m_position(MinimapPosition::TopRight)
    , m_backgroundColor(IM_COL32(40, 40, 50, 200))
    , m_nodeColor(IM_COL32(100, 150, 255, 255))
    , m_viewportColor(IM_COL32(200, 200, 200, 100))
{
}

// ============================================================================
// Configuration
// ============================================================================

void CanvasMinimapRenderer::SetVisible(bool visible)
{
    m_visible = visible;
}

bool CanvasMinimapRenderer::IsVisible() const
{
    return m_visible;
}

void CanvasMinimapRenderer::SetSize(float ratio)
{
    // Clamp between 0.05 (5%) and 0.5 (50%)
    m_sizeRatio = std::max(0.05f, std::min(0.5f, ratio));
}

float CanvasMinimapRenderer::GetSize() const
{
    return m_sizeRatio;
}

void CanvasMinimapRenderer::SetPosition(MinimapPosition pos)
{
    m_position = pos;
}

MinimapPosition CanvasMinimapRenderer::GetPosition() const
{
    return m_position;
}

// ============================================================================
// Data Updates
// ============================================================================

float CanvasMinimapRenderer::NormaliseCoord(float value, float minVal, float maxVal)
{
    float range = maxVal - minVal;
    if (range <= 0.0f)
        return 0.0f;
    
    float norm = (value - minVal) / range;
    
    // Clamp to [0, 1]
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    
    return norm;
}

void CanvasMinimapRenderer::UpdateNodes(
    const std::vector<std::tuple<int, float, float, float, float>>& nodes,
    float graphMinX, float graphMaxX, float graphMinY, float graphMaxY)
{
    m_normalisedNodes.clear();
    m_normalisedNodes.reserve(nodes.size());

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        int   nodeId = std::get<0>(nodes[i]);
        float posX   = std::get<1>(nodes[i]);
        float posY   = std::get<2>(nodes[i]);
        float width  = std::get<3>(nodes[i]);
        float height = std::get<4>(nodes[i]);

        MinimapNodeData normNode;
        normNode.nodeId = nodeId;
        normNode.normX = NormaliseCoord(posX, graphMinX, graphMaxX);
        normNode.normY = NormaliseCoord(posY, graphMinY, graphMaxY);
        normNode.normW = NormaliseCoord(posX + width, graphMinX, graphMaxX) - normNode.normX;
        normNode.normH = NormaliseCoord(posY + height, graphMinY, graphMaxY) - normNode.normY;

        // Clamp size to avoid negative dimensions
        if (normNode.normW < 0.0f) normNode.normW = 0.0f;
        if (normNode.normH < 0.0f) normNode.normH = 0.0f;

        m_normalisedNodes.push_back(normNode);
    }
}

void CanvasMinimapRenderer::UpdateViewport(
    float viewMinX, float viewMaxX, float viewMinY, float viewMaxY,
    float graphMinX, float graphMaxX, float graphMinY, float graphMaxY)
{
    m_normalisedViewport.x = NormaliseCoord(viewMinX, graphMinX, graphMaxX);
    m_normalisedViewport.y = NormaliseCoord(viewMinY, graphMinY, graphMaxY);
    m_normalisedViewport.w = NormaliseCoord(viewMaxX, graphMinX, graphMaxX) - m_normalisedViewport.x;
    m_normalisedViewport.h = NormaliseCoord(viewMaxY, graphMinY, graphMaxY) - m_normalisedViewport.y;

    // Clamp viewport size
    if (m_normalisedViewport.w < 0.0f) m_normalisedViewport.w = 0.0f;
    if (m_normalisedViewport.h < 0.0f) m_normalisedViewport.h = 0.0f;
    if (m_normalisedViewport.w > 1.0f) m_normalisedViewport.w = 1.0f;
    if (m_normalisedViewport.h > 1.0f) m_normalisedViewport.h = 1.0f;
}

// ============================================================================
// Rendering
// ============================================================================

void CanvasMinimapRenderer::RenderImNodes() const
{
    if (!m_visible)
        return;

    // Use ImNodes native MiniMap
    // Position enum matches: TopLeft=0, TopRight=1, BottomLeft=2, BottomRight=3
    ImNodes::MiniMap(m_sizeRatio, static_cast<ImNodesMiniMapLocation>(m_position));
}

void CanvasMinimapRenderer::RenderCustom(const ImVec2& canvasScreenPos, const ImVec2& canvasSize) const
{
    if (!m_visible || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList)
        return;

    // Compute minimap dimensions
    float minimapW = canvasSize.x * m_sizeRatio;
    float minimapH = canvasSize.y * m_sizeRatio;

    // Compute minimap position based on m_position
    ImVec2 minimapPos = canvasScreenPos;
    switch (m_position)
    {
    case MinimapPosition::TopRight:
        minimapPos.x = canvasScreenPos.x + canvasSize.x - minimapW - 5.0f;
        minimapPos.y = canvasScreenPos.y + 5.0f;
        break;
    case MinimapPosition::TopLeft:
        minimapPos.x = canvasScreenPos.x + 5.0f;
        minimapPos.y = canvasScreenPos.y + 5.0f;
        break;
    case MinimapPosition::BottomLeft:
        minimapPos.x = canvasScreenPos.x + 5.0f;
        minimapPos.y = canvasScreenPos.y + canvasSize.y - minimapH - 5.0f;
        break;
    case MinimapPosition::BottomRight:
        minimapPos.x = canvasScreenPos.x + canvasSize.x - minimapW - 5.0f;
        minimapPos.y = canvasScreenPos.y + canvasSize.y - minimapH - 5.0f;
        break;
    }

    ImVec2 minimapMax(minimapPos.x + minimapW, minimapPos.y + minimapH);

    // Draw background
    drawList->AddRectFilled(minimapPos, minimapMax, m_backgroundColor, 4.0f);
    drawList->AddRect(minimapPos, minimapMax, IM_COL32(150, 150, 150, 255), 4.0f, 0, 1.0f);

    // Draw normalised nodes as small rectangles
    for (const auto& node : m_normalisedNodes)
    {
        float nodeScreenX = minimapPos.x + node.normX * minimapW;
        float nodeScreenY = minimapPos.y + node.normY * minimapH;
        float nodeScreenW = node.normW * minimapW;
        float nodeScreenH = node.normH * minimapH;

        // Clamp node dimensions (minimum 2 pixels for visibility)
        if (nodeScreenW < 2.0f) nodeScreenW = 2.0f;
        if (nodeScreenH < 2.0f) nodeScreenH = 2.0f;

        ImVec2 nodeMin(nodeScreenX, nodeScreenY);
        ImVec2 nodeMax(nodeScreenX + nodeScreenW, nodeScreenY + nodeScreenH);

        drawList->AddRectFilled(nodeMin, nodeMax, m_nodeColor, 1.0f);
        drawList->AddRect(nodeMin, nodeMax, IM_COL32(200, 200, 255, 255), 1.0f);
    }

    // Draw viewport rectangle (with transparency)
    {
        float vpScreenX = minimapPos.x + m_normalisedViewport.x * minimapW;
        float vpScreenY = minimapPos.y + m_normalisedViewport.y * minimapH;
        float vpScreenW = m_normalisedViewport.w * minimapW;
        float vpScreenH = m_normalisedViewport.h * minimapH;

        ImVec2 vpMin(vpScreenX, vpScreenY);
        ImVec2 vpMax(vpScreenX + vpScreenW, vpScreenY + vpScreenH);

        drawList->AddRectFilled(vpMin, vpMax, m_viewportColor, 1.0f);
        drawList->AddRect(vpMin, vpMax, IM_COL32(255, 255, 255, 255), 1.0f, 0, 2.0f);
    }
}

// ============================================================================
// Data Access
// ============================================================================

const std::vector<MinimapNodeData>& CanvasMinimapRenderer::GetNormalisedNodes() const
{
    return m_normalisedNodes;
}

const MinimapViewportData& CanvasMinimapRenderer::GetNormalisedViewport() const
{
    return m_normalisedViewport;
}

// ============================================================================
// Colors
// ============================================================================

void CanvasMinimapRenderer::SetBackgroundColor(ImU32 color)
{
    m_backgroundColor = color;
}

void CanvasMinimapRenderer::SetNodeColor(ImU32 color)
{
    m_nodeColor = color;
}

void CanvasMinimapRenderer::SetViewportColor(ImU32 color)
{
    m_viewportColor = color;
}

} // namespace Olympe
