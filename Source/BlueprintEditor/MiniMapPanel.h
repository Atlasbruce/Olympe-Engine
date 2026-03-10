/**
 * @file MiniMapPanel.h
 * @brief Navigation mini-map panel for the VS graph editor (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * MiniMapPanel tracks the full graph bounds and the current viewport, and
 * exposes normalised (0..1) coordinates for rendering a thumbnail overview.
 * It also converts click/drag events on the mini-map into graph-scroll deltas.
 *
 * No ImGui dependency — UI code reads the data and renders however it likes.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <utility>
#include <vector>

namespace Olympe {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @struct MiniMapNodeEntry
 * @brief A node represented in the mini-map (normalised coords).
 */
struct MiniMapNodeEntry {
    int   id;   ///< Graph node ID
    float nx;   ///< Normalised X [0..1]
    float ny;   ///< Normalised Y [0..1]
};

// ============================================================================
// MiniMapPanel
// ============================================================================

/**
 * @class MiniMapPanel
 * @brief Singleton mini-map providing normalised graph overview data.
 *
 * Typical usage:
 * @code
 *   auto& mm = MiniMapPanel::Get();
 *   mm.SetGraphBounds(minX, maxX, minY, maxY);
 *   mm.SetViewport(viewMinX, viewMaxX, viewMinY, viewMaxY);
 *   mm.UpdateNodePositions(rawPositions);
 *
 *   // Render thumbnail using GetNodes() and GetViewportRect()
 *   auto [vpX, vpY, vpW, vpH] = mm.GetViewportRect();  // C++14: use structured float4
 * @endcode
 */
class MiniMapPanel {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /** @brief Returns the single shared instance. */
    static MiniMapPanel& Get();

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the full bounds of the graph in graph space.
     */
    void SetGraphBounds(float minX, float maxX, float minY, float maxY);

    /**
     * @brief Sets the currently visible viewport in graph space.
     */
    void SetViewport(float minX, float maxX, float minY, float maxY);

    /**
     * @brief Updates the normalised node positions from raw (id, x, y) triples.
     * @param rawPositions  Each entry: (nodeId, graphX, graphY).
     */
    void UpdateNodePositions(const std::vector<std::pair<int, std::pair<float, float>>>& rawPositions);

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /** @brief Returns the normalised node positions for rendering. */
    const std::vector<MiniMapNodeEntry>& GetNodes() const;

    /**
     * @brief Returns the normalised viewport rectangle.
     * @param outX  Left edge [0..1]
     * @param outY  Top edge [0..1]
     * @param outW  Width [0..1]
     * @param outH  Height [0..1]
     */
    void GetViewportRect(float& outX, float& outY, float& outW, float& outH) const;

    /** @brief Returns true when the mini-map has valid data to display. */
    bool IsVisible() const;

    // -----------------------------------------------------------------------
    // Interaction
    // -----------------------------------------------------------------------

    /**
     * @brief Converts a click on the mini-map into a graph-scroll target.
     *
     * @param clickNX  Normalised X of the click on the mini-map [0..1].
     * @param clickNY  Normalised Y of the click on the mini-map [0..1].
     * @param outScrollX Target scroll X in graph space.
     * @param outScrollY Target scroll Y in graph space.
     */
    bool HandleClick(float clickNX, float clickNY,
                     float& outScrollX, float& outScrollY) const;

    static constexpr float PANEL_WIDTH  = 200.0f;  ///< Default panel width in screen px
    static constexpr float PANEL_HEIGHT = 150.0f;  ///< Default panel height in screen px

private:

    MiniMapPanel();

    float Normalise(float value, float minVal, float maxVal) const;

    float m_GraphMinX;
    float m_GraphMaxX;
    float m_GraphMinY;
    float m_GraphMaxY;

    float m_ViewMinX;
    float m_ViewMaxX;
    float m_ViewMinY;
    float m_ViewMaxY;

    std::vector<MiniMapNodeEntry> m_Nodes;
    bool                          m_IsVisible;
};

} // namespace Olympe
