/**
 * @file ViewportCulling.cpp
 * @brief Viewport culling implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "ViewportCulling.h"

#include <cstddef>

namespace Olympe {

// ============================================================================
// IsNodeVisible
// ============================================================================

bool ViewportCulling::IsNodeVisible(float nodeX, float nodeY,
                                    float nodeW, float nodeH,
                                    const ViewRect& viewport,
                                    float margin)
{
    // Expand the viewport by margin on all sides.
    float vpLeft   = viewport.x - margin;
    float vpTop    = viewport.y - margin;
    float vpRight  = viewport.x + viewport.w + margin;
    float vpBottom = viewport.y + viewport.h + margin;

    // Node AABB
    float nLeft   = nodeX;
    float nTop    = nodeY;
    float nRight  = nodeX + nodeW;
    float nBottom = nodeY + nodeH;

    // AABB overlap test: no overlap when one rect is entirely outside the other.
    if (nRight  < vpLeft   ) return false;
    if (nLeft   > vpRight  ) return false;
    if (nBottom < vpTop    ) return false;
    if (nTop    > vpBottom ) return false;

    return true;
}

// ============================================================================
// FilterVisibleNodes
// ============================================================================

std::vector<int> ViewportCulling::FilterVisibleNodes(
    const std::vector<std::pair<int, ViewRect>>& nodeRects,
    const ViewRect& viewport,
    float margin)
{
    std::vector<int> visible;
    visible.reserve(nodeRects.size());

    for (size_t i = 0; i < nodeRects.size(); ++i)
    {
        const ViewRect& r = nodeRects[i].second;

        if (IsNodeVisible(r.x, r.y, r.w, r.h, viewport, margin))
            visible.push_back(nodeRects[i].first);
    }

    return visible;
}

} // namespace Olympe
