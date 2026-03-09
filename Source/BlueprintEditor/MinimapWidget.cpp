/**
 * @file MinimapWidget.cpp
 * @brief Minimap widget implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "MinimapWidget.h"

#include <cstddef>

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

MinimapWidget& MinimapWidget::Get()
{
    static MinimapWidget s_Instance;
    return s_Instance;
}

// ============================================================================
// Construction
// ============================================================================

MinimapWidget::MinimapWidget()
{
    m_Viewport.x = 0.0f;
    m_Viewport.y = 0.0f;
    m_Viewport.w = 1.0f;
    m_Viewport.h = 1.0f;
}

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Safely normalise a value within a range, clamped to [0,1].
float Normalise(float value, float minVal, float maxVal)
{
    float range = maxVal - minVal;
    if (range <= 0.0f)
        return 0.0f;
    float n = (value - minVal) / range;
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return n;
}

} // anonymous namespace

// ============================================================================
// UpdateNodes
// ============================================================================

void MinimapWidget::UpdateNodes(const std::vector<MinimapNode>& nodes,
                                float graphMinX, float graphMinY,
                                float graphMaxX, float graphMaxY)
{
    m_MinimapNodes.clear();
    m_MinimapNodes.reserve(nodes.size());

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        MinimapNode mn;
        mn.id = nodes[i].id;
        mn.x  = Normalise(nodes[i].x, graphMinX, graphMaxX);
        mn.y  = Normalise(nodes[i].y, graphMinY, graphMaxY);
        m_MinimapNodes.push_back(mn);
    }
}

// ============================================================================
// UpdateViewport
// ============================================================================

void MinimapWidget::UpdateViewport(float viewX,  float viewY,
                                   float viewW,  float viewH,
                                   float graphMinX, float graphMinY,
                                   float graphMaxX, float graphMaxY)
{
    float rangeX = graphMaxX - graphMinX;
    float rangeY = graphMaxY - graphMinY;

    m_Viewport.x = Normalise(viewX, graphMinX, graphMaxX);
    m_Viewport.y = Normalise(viewY, graphMinY, graphMaxY);
    m_Viewport.w = (rangeX > 0.0f) ? (viewW / rangeX) : 1.0f;
    m_Viewport.h = (rangeY > 0.0f) ? (viewH / rangeY) : 1.0f;

    // Clamp width/height so the viewport rect does not exceed 1.0
    if (m_Viewport.w > 1.0f) m_Viewport.w = 1.0f;
    if (m_Viewport.h > 1.0f) m_Viewport.h = 1.0f;
}

// ============================================================================
// Accessors
// ============================================================================

const std::vector<MinimapNode>& MinimapWidget::GetMinimapNodes() const
{
    return m_MinimapNodes;
}

const MinimapViewport& MinimapWidget::GetViewport() const
{
    return m_Viewport;
}

// ============================================================================
// OnDrag
// ============================================================================

void MinimapWidget::OnDrag(float deltaX,  float deltaY,
                           float graphW,  float graphH,
                           float& outGraphDeltaX, float& outGraphDeltaY)
{
    // Map minimap pixel delta → normalised delta → graph-space delta
    outGraphDeltaX = (WIDTH  > 0.0f) ? (deltaX / WIDTH  * graphW) : 0.0f;
    outGraphDeltaY = (HEIGHT > 0.0f) ? (deltaY / HEIGHT * graphH) : 0.0f;
}

} // namespace Olympe
