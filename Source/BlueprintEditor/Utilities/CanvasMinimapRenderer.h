/**
 * @file CanvasMinimapRenderer.h
 * @brief Centralized minimap rendering helper for all canvas types
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * Unified minimap renderer supporting:
 * - ImNodes native MiniMap (BehaviorTree, VisualScript)
 * - Custom minimap rendering (EntityPrefab, future custom canvases)
 *
 * This class handles:
 * - Normalised coordinate computation (0..1 range)
 * - Minimap visibility, size, and position control
 * - Integration with ImGui for rendering
 *
 * C++14 compliant.
 */

#pragma once

#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imnodes/imnodes.h"
#include <vector>
#include <tuple>
#include <utility>

namespace Olympe
{
    /**
     * @struct MinimapNode
     * @brief Node representation in minimap (normalised coordinates)
     */
    struct MinimapNodeData
    {
        int   nodeId;   ///< Original graph node ID
        float normX;    ///< Normalised X position [0..1]
        float normY;    ///< Normalised Y position [0..1]
        float normW;    ///< Normalised width [0..1]
        float normH;    ///< Normalised height [0..1]
    };

    /**
     * @struct MinimapViewportData
     * @brief Viewport region in normalised coordinates
     */
    struct MinimapViewportData
    {
        float x;  ///< Left edge [0..1]
        float y;  ///< Top edge [0..1]
        float w;  ///< Width [0..1]
        float h;  ///< Height [0..1]
    };

    /**
     * @enum MinimapPosition
     * @brief Screen corner positions for minimap overlay
     */
    enum class MinimapPosition
    {
        TopLeft     = 0,
        TopRight    = 1,
        BottomLeft  = 2,
        BottomRight = 3
    };

    /**
     * @class CanvasMinimapRenderer
     * @brief Unified minimap renderer for standardized appearance
     *
     * Typical usage:
     * @code
     *   CanvasMinimapRenderer minimap;
     *   minimap.SetSize(0.15f);  // 15% of canvas
     *   minimap.SetPosition(MinimapPosition::TopRight);
     *
     *   // Before rendering:
     *   minimap.UpdateNodes(nodesList, graphMinX, graphMaxX, graphMinY, graphMaxY);
     *   minimap.UpdateViewport(viewX, viewY, viewW, viewH, graphBounds);
     *
     *   // During render:
     *   if (minimap.IsVisible())
     *   {
     *       minimap.RenderCustom(drawList, canvasScreenPos, canvasSize);
     *       // OR for ImNodes:
     *       minimap.RenderImNodes();
     *   }
     * @endcode
     */
    class CanvasMinimapRenderer
    {
    public:
        // Default dimensions
        static constexpr float DEFAULT_WIDTH  = 200.0f;
        static constexpr float DEFAULT_HEIGHT = 150.0f;

        CanvasMinimapRenderer();
        virtual ~CanvasMinimapRenderer() = default;

        // ====================================================================
        // Configuration
        // ====================================================================

        /**
         * @brief Set minimap visibility
         */
        void SetVisible(bool visible);

        /**
         * @brief Check if minimap should be rendered
         */
        bool IsVisible() const;

        /**
         * @brief Set minimap size as ratio of canvas (0.05 - 0.5)
         */
        void SetSize(float ratio);

        /**
         * @brief Get current size ratio
         */
        float GetSize() const;

        /**
         * @brief Set minimap corner position
         */
        void SetPosition(MinimapPosition pos);

        /**
         * @brief Get current position
         */
        MinimapPosition GetPosition() const;

        // ====================================================================
        // Data Updates
        // ====================================================================

        /**
         * @brief Update node positions from raw graph data
         * @param nodes Vector of (nodeId, posX, posY, width, height) tuples
         * @param graphMinX Graph bounds left edge
         * @param graphMaxX Graph bounds right edge
         * @param graphMinY Graph bounds top edge
         * @param graphMaxY Graph bounds bottom edge
         */
        void UpdateNodes(
            const std::vector<std::tuple<int, float, float, float, float>>& nodes,
            float graphMinX, float graphMaxX, float graphMinY, float graphMaxY
        );

        /**
         * @brief Update visible viewport
         * @param viewMinX Left edge of visible area (graph space)
         * @param viewMaxX Right edge of visible area (graph space)
         * @param viewMinY Top edge of visible area (graph space)
         * @param viewMaxY Bottom edge of visible area (graph space)
         * @param graphMinX Graph bounds left edge
         * @param graphMaxX Graph bounds right edge
         * @param graphMinY Graph bounds top edge
         * @param graphMaxY Graph bounds bottom edge
         */
        void UpdateViewport(
            float viewMinX, float viewMaxX, float viewMinY, float viewMaxY,
            float graphMinX, float graphMaxX, float graphMinY, float graphMaxY
        );

        // ====================================================================
        // Rendering
        // ====================================================================

        /**
         * @brief Render minimap using ImNodes native API
         * @details Must be called BEFORE ImNodes::EndNodeEditor()
         */
        void RenderImNodes() const;

        /**
         * @brief Render minimap using custom ImGui drawing
         * @param canvasScreenPos Top-left of canvas on screen
         * @param canvasSize Canvas dimensions in screen pixels
         */
        void RenderCustom(const ImVec2& canvasScreenPos, const ImVec2& canvasSize) const;

        // ====================================================================
        // Data Access
        // ====================================================================

        /**
         * @brief Get normalised node positions for custom rendering
         */
        const std::vector<MinimapNodeData>& GetNormalisedNodes() const;

        /**
         * @brief Get normalised viewport rectangle for custom rendering
         */
        const MinimapViewportData& GetNormalisedViewport() const;

        // ====================================================================
        // Colors (customizable for different canvas types)
        // ====================================================================

        /**
         * @brief Set minimap background color (RGBA)
         */
        void SetBackgroundColor(ImU32 color);

        /**
         * @brief Set minimap node color (RGBA)
         */
        void SetNodeColor(ImU32 color);

        /**
         * @brief Set minimap viewport rectangle color (RGBA)
         */
        void SetViewportColor(ImU32 color);

    private:

        // Normalisation helper
        static float NormaliseCoord(float value, float minVal, float maxVal);

        // Configuration
        bool m_visible;
        float m_sizeRatio;              ///< 0.05 - 0.5 ratio of canvas
        MinimapPosition m_position;

        // Normalised data
        std::vector<MinimapNodeData> m_normalisedNodes;
        MinimapViewportData m_normalisedViewport;

        // Colors
        ImU32 m_backgroundColor;
        ImU32 m_nodeColor;
        ImU32 m_viewportColor;
    };

} // namespace Olympe
