/**
 * @file ViewportCulling.h
 * @brief Viewport culling helpers for the VS graph canvas (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * Provides static utility methods for determining whether a node rectangle
 * overlaps the current viewport (with an optional margin).  Call
 * FilterVisibleNodes() to get a list of node IDs that should be rendered.
 *
 * No state, no singletons — pure functions operating on plain data.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <vector>
#include <utility>

namespace Olympe {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @struct ViewRect
 * @brief An axis-aligned rectangle used to describe the viewport or a node bounds.
 */
struct ViewRect {
    float x; ///< Left edge
    float y; ///< Top edge
    float w; ///< Width
    float h; ///< Height
};

// ============================================================================
// ViewportCulling
// ============================================================================

/**
 * @class ViewportCulling
 * @brief Static helpers for AABB-based viewport culling.
 *
 * Typical usage:
 * @code
 *   ViewRect vp = { scrollX, scrollY, windowW, windowH };
 *   auto visible = ViewportCulling::FilterVisibleNodes(nodeRects, vp);
 *   for (int id : visible) RenderNode(id);
 * @endcode
 */
class ViewportCulling {
public:

    /**
     * @brief Tests whether a single node rectangle overlaps the viewport.
     *
     * @param nodeX     Node left edge in graph space.
     * @param nodeY     Node top edge in graph space.
     * @param nodeW     Node width.
     * @param nodeH     Node height.
     * @param viewport  Current visible rectangle in graph space.
     * @param margin    Extra padding applied to the viewport before testing.
     * @return true if the node is at least partially visible.
     */
    static bool IsNodeVisible(float nodeX, float nodeY,
                              float nodeW, float nodeH,
                              const ViewRect& viewport,
                              float margin = 50.0f);

    /**
     * @brief Filters a list of (id, rect) pairs and returns only the visible IDs.
     *
     * @param nodeRects  List of (nodeId, ViewRect) pairs.
     * @param viewport   Current visible rectangle in graph space.
     * @param margin     Extra padding applied to the viewport before testing.
     * @return           IDs of visible nodes.
     */
    static std::vector<int> FilterVisibleNodes(
        const std::vector<std::pair<int, ViewRect>>& nodeRects,
        const ViewRect& viewport,
        float margin = 50.0f);
};

} // namespace Olympe
