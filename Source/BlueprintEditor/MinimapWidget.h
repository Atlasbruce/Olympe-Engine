/**
 * @file MinimapWidget.h
 * @brief Data-only minimap widget for the VS graph editor (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * MinimapWidget converts absolute graph-space positions into normalised 0..1
 * coordinates suitable for rendering a miniature overview of the canvas.  It
 * also tracks the visible viewport rectangle and handles drag events.
 *
 * No ImGui dependency — the UI layer calls UpdateNodes(), UpdateViewport(), and
 * GetMinimapNodes() each frame and renders the result using whatever draw calls
 * it likes.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <vector>

namespace Olympe {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @struct MinimapNode
 * @brief A single node represented in the minimap.
 */
struct MinimapNode {
    float x;  ///< Normalised x position [0..1] in graph space
    float y;  ///< Normalised y position [0..1] in graph space
    int   id; ///< Corresponding node ID in the full graph
};

/**
 * @struct MinimapViewport
 * @brief Normalised rectangle [0..1] representing the visible portion of the graph.
 */
struct MinimapViewport {
    float x; ///< Left edge (normalised)
    float y; ///< Top edge (normalised)
    float w; ///< Width (normalised)
    float h; ///< Height (normalised)
};

// ============================================================================
// MinimapWidget
// ============================================================================

/**
 * @class MinimapWidget
 * @brief Singleton that maintains normalised minimap state.
 *
 * Typical usage:
 * @code
 *   auto& mm = MinimapWidget::Get();
 *   mm.UpdateNodes(graphNodes, minX, minY, maxX, maxY);
 *   mm.UpdateViewport(scrollX, scrollY, viewW, viewH, minX, minY, maxX, maxY);
 *   const auto& pts = mm.GetMinimapNodes();
 *   const auto& vp  = mm.GetViewport();
 *   // ... render pts and vp rectangle ...
 * @endcode
 */
class MinimapWidget {
public:

    /// Width of the minimap panel in screen pixels.
    static constexpr float WIDTH  = 200.0f;
    /// Height of the minimap panel in screen pixels.
    static constexpr float HEIGHT = 150.0f;

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the single shared instance.
     */
    static MinimapWidget& Get();

    // -----------------------------------------------------------------------
    // Update
    // -----------------------------------------------------------------------

    /**
     * @brief Recomputes normalised node positions from graph-space coordinates.
     *
     * @param nodes     Nodes to display (absolute graph-space positions).
     * @param graphMinX Leftmost graph boundary.
     * @param graphMinY Topmost graph boundary.
     * @param graphMaxX Rightmost graph boundary.
     * @param graphMaxY Bottommost graph boundary.
     */
    void UpdateNodes(const std::vector<MinimapNode>& nodes,
                     float graphMinX, float graphMinY,
                     float graphMaxX, float graphMaxY);

    /**
     * @brief Recomputes the normalised viewport rectangle.
     *
     * @param viewX     Current scroll / pan X in graph space.
     * @param viewY     Current scroll / pan Y in graph space.
     * @param viewW     Viewport width in graph space.
     * @param viewH     Viewport height in graph space.
     * @param graphMinX Leftmost graph boundary.
     * @param graphMinY Topmost graph boundary.
     * @param graphMaxX Rightmost graph boundary.
     * @param graphMaxY Bottommost graph boundary.
     */
    void UpdateViewport(float viewX,  float viewY,
                        float viewW,  float viewH,
                        float graphMinX, float graphMinY,
                        float graphMaxX, float graphMaxY);

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the last computed set of normalised minimap node positions.
     */
    const std::vector<MinimapNode>& GetMinimapNodes() const;

    /**
     * @brief Returns the last computed normalised viewport rectangle.
     */
    const MinimapViewport& GetViewport() const;

    // -----------------------------------------------------------------------
    // Interaction
    // -----------------------------------------------------------------------

    /**
     * @brief Converts a drag delta on the minimap into a graph-space delta.
     *
     * @param deltaX         Drag delta in minimap pixel space (x).
     * @param deltaY         Drag delta in minimap pixel space (y).
     * @param graphW         Full graph width in graph space.
     * @param graphH         Full graph height in graph space.
     * @param outGraphDeltaX Resulting graph-space x delta.
     * @param outGraphDeltaY Resulting graph-space y delta.
     */
    void OnDrag(float deltaX,  float deltaY,
                float graphW,  float graphH,
                float& outGraphDeltaX, float& outGraphDeltaY);

private:

    MinimapWidget();

    std::vector<MinimapNode> m_MinimapNodes;
    MinimapViewport          m_Viewport;
};

} // namespace Olympe
