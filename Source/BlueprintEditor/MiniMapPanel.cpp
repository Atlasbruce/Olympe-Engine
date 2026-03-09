/**
 * @file MiniMapPanel.cpp
 * @brief MiniMapPanel implementation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "MiniMapPanel.h"

#include <cstddef>

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

MiniMapPanel& MiniMapPanel::Get()
{
    static MiniMapPanel s_Instance;
    return s_Instance;
}

MiniMapPanel::MiniMapPanel()
    : m_GraphMinX(0.0f), m_GraphMaxX(1000.0f)
    , m_GraphMinY(0.0f), m_GraphMaxY(1000.0f)
    , m_ViewMinX(0.0f),  m_ViewMaxX(800.0f)
    , m_ViewMinY(0.0f),  m_ViewMaxY(600.0f)
    , m_IsVisible(false)
{
}

// ============================================================================
// Helpers
// ============================================================================

float MiniMapPanel::Normalise(float value, float minVal, float maxVal) const
{
    float range = maxVal - minVal;
    if (range <= 0.0f)
        return 0.0f;
    float n = (value - minVal) / range;
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return n;
}

// ============================================================================
// Configuration
// ============================================================================

void MiniMapPanel::SetGraphBounds(float minX, float maxX, float minY, float maxY)
{
    m_GraphMinX = minX;
    m_GraphMaxX = maxX;
    m_GraphMinY = minY;
    m_GraphMaxY = maxY;
}

void MiniMapPanel::SetViewport(float minX, float maxX, float minY, float maxY)
{
    m_ViewMinX = minX;
    m_ViewMaxX = maxX;
    m_ViewMinY = minY;
    m_ViewMaxY = maxY;
}

void MiniMapPanel::UpdateNodePositions(
    const std::vector<std::pair<int, std::pair<float, float>>>& rawPositions)
{
    m_Nodes.clear();
    m_Nodes.reserve(rawPositions.size());

    for (size_t i = 0; i < rawPositions.size(); ++i)
    {
        MiniMapNodeEntry entry;
        entry.id = rawPositions[i].first;
        entry.nx = Normalise(rawPositions[i].second.first,  m_GraphMinX, m_GraphMaxX);
        entry.ny = Normalise(rawPositions[i].second.second, m_GraphMinY, m_GraphMaxY);
        m_Nodes.push_back(entry);
    }

    m_IsVisible = !m_Nodes.empty();
}

// ============================================================================
// Accessors
// ============================================================================

const std::vector<MiniMapNodeEntry>& MiniMapPanel::GetNodes() const
{
    return m_Nodes;
}

void MiniMapPanel::GetViewportRect(float& outX, float& outY,
                                   float& outW, float& outH) const
{
    outX = Normalise(m_ViewMinX, m_GraphMinX, m_GraphMaxX);
    outY = Normalise(m_ViewMinY, m_GraphMinY, m_GraphMaxY);

    float graphW = m_GraphMaxX - m_GraphMinX;
    float graphH = m_GraphMaxY - m_GraphMinY;

    outW = (graphW > 0.0f) ? ((m_ViewMaxX - m_ViewMinX) / graphW) : 1.0f;
    outH = (graphH > 0.0f) ? ((m_ViewMaxY - m_ViewMinY) / graphH) : 1.0f;

    if (outW > 1.0f) outW = 1.0f;
    if (outH > 1.0f) outH = 1.0f;
}

bool MiniMapPanel::IsVisible() const
{
    return m_IsVisible;
}

// ============================================================================
// Interaction
// ============================================================================

bool MiniMapPanel::HandleClick(float clickNX, float clickNY,
                                float& outScrollX, float& outScrollY) const
{
    if (clickNX < 0.0f || clickNX > 1.0f || clickNY < 0.0f || clickNY > 1.0f)
        return false;

    float graphW = m_GraphMaxX - m_GraphMinX;
    float graphH = m_GraphMaxY - m_GraphMinY;

    outScrollX = m_GraphMinX + clickNX * graphW;
    outScrollY = m_GraphMinY + clickNY * graphH;
    return true;
}

} // namespace Olympe
