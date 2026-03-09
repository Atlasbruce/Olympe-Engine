/**
 * @file GridSnapping.cpp
 * @brief Grid-snapping helper implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "GridSnapping.h"

#include <cmath>

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

GridSnapping& GridSnapping::Get()
{
    static GridSnapping s_Instance;
    return s_Instance;
}

// ============================================================================
// Construction
// ============================================================================

GridSnapping::GridSnapping()
    : m_Enabled(true)
    , m_GridSize(16)
{
}

// ============================================================================
// State
// ============================================================================

bool GridSnapping::IsEnabled() const
{
    return m_Enabled;
}

void GridSnapping::SetEnabled(bool enabled)
{
    m_Enabled = enabled;
}

void GridSnapping::Toggle()
{
    m_Enabled = !m_Enabled;
}

// ============================================================================
// Grid size
// ============================================================================

int GridSnapping::GetGridSize() const
{
    return m_GridSize;
}

void GridSnapping::SetGridSize(int size)
{
    if (size > 0)
        m_GridSize = size;
}

// ============================================================================
// Snapping
// ============================================================================

float GridSnapping::SnapX(float x) const
{
    if (!m_Enabled || m_GridSize <= 0)
        return x;

    float gs = static_cast<float>(m_GridSize);
    return std::floor((x / gs) + 0.5f) * gs;
}

float GridSnapping::SnapY(float y) const
{
    if (!m_Enabled || m_GridSize <= 0)
        return y;

    float gs = static_cast<float>(m_GridSize);
    return std::floor((y / gs) + 0.5f) * gs;
}

void GridSnapping::Snap(float& x, float& y) const
{
    x = SnapX(x);
    y = SnapY(y);
}

} // namespace Olympe
