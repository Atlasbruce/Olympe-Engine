#pragma once

#include "../../third_party/imgui/imgui.h"

namespace Olympe
{
    /**
     * @class CanvasGridRenderer
     * @brief Shared utility for rendering grid backgrounds on any canvas editor
     * @details Provides configurable grid rendering with support for:
     *   - Major and minor grid lines
     *   - Zoom-aware scaling
     *   - Pan offset handling
     *   - Customizable colors and spacing
     *   - Professional grid styles (VisualScript, Compact, Spacious)
     *   - C++14 compatible
     */
    class CanvasGridRenderer
    {
    public:
        /**
         * @enum GridStylePreset
         * @brief Pre-configured grid styles matching professional editors
         */
        enum GridStylePreset
        {
            Style_VisualScript,  // Dark blue bg, subtle gray grid (imnodes professional default)
            Style_Compact,       // Tight grid for dense layouts
            Style_Spacious,      // Wide grid for spacious layouts
        };

        /**
         * @struct GridConfig
         * @brief Configuration parameters for grid rendering
         */
        struct GridConfig
        {
            // Canvas positioning
            ImVec2 canvasPos;        // Top-left screen position of canvas
            ImVec2 canvasSize;       // Width and height of canvas area

            // Canvas transformation
            float zoom = 1.0f;       // Canvas zoom level (1.0 = 100%)
            float offsetX = 0.0f;    // Pan offset X (canvas space)
            float offsetY = 0.0f;    // Pan offset Y (canvas space)

            // Grid parameters
            float majorSpacing = 24.0f;   // Distance between major grid lines (canvas units)
            float minorDivisor = 1.0f;    // Minor lines = majorSpacing / minorDivisor

            // Colors (ImVec4: r, g, b, a)
            ImVec4 backgroundColor = ImVec4(0.157f, 0.157f, 0.196f, 0.784f);  // (40,40,50,200) - Dark blue
            ImVec4 majorLineColor = ImVec4(0.941f, 0.941f, 0.941f, 0.235f);   // (240,240,240,60)
            ImVec4 minorLineColor = ImVec4(0.784f, 0.784f, 0.784f, 0.157f);   // (200,200,200,40)

            // Line weights
            float majorLineThickness = 1.0f;   // Major line thickness in pixels
            float minorLineThickness = 0.5f;   // Minor line thickness in pixels
        };

        /**
         * @brief Get pre-configured grid style (VisualScript/Compact/Spacious)
         * @param preset The style preset to use
         * @return GridConfig initialized with preset values
         */
        static GridConfig GetStylePreset(GridStylePreset preset);

        /**
         * @brief Render a grid on the given ImDrawList
         * @param drawList ImGui draw list to render to
         * @param config Grid configuration parameters
         */
        static void RenderGrid(ImDrawList* drawList, const GridConfig& config);

        /**
         * @brief Convenience overload using ImGui's current context
         * @param config Grid configuration parameters
         */
        static void RenderGrid(const GridConfig& config);
    };
}
