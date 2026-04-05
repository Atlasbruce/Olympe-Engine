/**
 * @file ICanvasEditor.h
 * @brief Standardized Canvas Editor interface for unified architecture
 * @details Abstraction layer for pan/zoom/grid management across VisualScript and PrefabCanvas
 *
 * @design
 * - Unifies coordinate systems (Screen, Canvas, Editor, Grid spaces)
 * - Abstracts pan/zoom control (some implementations expose zoom, others don't)
 * - Provides grid rendering integration
 * - Separates lifecycle (BeginRender/EndRender) from node/link domain logic
 *
 * @usage
 * // In any editor's render loop:
 * m_canvasEditor->BeginRender();
 * {
 *   // Render domain-specific content (nodes, links, etc.)
 *   RenderMyNodes();
 * }
 * m_canvasEditor->EndRender();
 *
 * @compatibility C++14
 */

#pragma once

#include "../../third_party/imgui/imgui.h"
#include "CanvasGridRenderer.h"

namespace Olympe
{
    /**
     * @class ICanvasEditor
     * @brief Abstract interface for canvas editors with unified pan/zoom/grid management
     *
     * Implementations:
     * - ImNodesCanvasEditor: Wraps ImNodes::BeginNodeEditor/EndNodeEditor
     * - CustomCanvasEditor: Manual pan/zoom implementation with zoom support
     *
     * Key contract:
     * 1. BeginRender() must be called once per frame at start of canvas rendering
     * 2. EndRender() must be called once per frame at end of canvas rendering
     * 3. All coordinate transformations maintain consistency across pan/zoom states
     * 4. Pan/zoom state is persistent across frames (for smooth interaction)
     */
    class ICanvasEditor
    {
    public:
        virtual ~ICanvasEditor() = default;

        // ====================================================================
        // Lifecycle Management
        // ====================================================================

        /**
         * @brief Begin rendering cycle for this canvas
         * @details Must be called at the start of each frame's canvas rendering
         * - Initializes draw state
         * - Updates pan/zoom from input
         * - Prepares coordinate systems
         *
         * Implementations:
         * - ImNodesCanvasEditor: Calls ImNodes::BeginNodeEditor()
         * - CustomCanvasEditor: Updates m_canvasOffset and m_canvasZoom from input
         */
        virtual void BeginRender() = 0;

        /**
         * @brief End rendering cycle for this canvas
         * @details Must be called at the end of each frame's canvas rendering
         * - Finalizes draw state
         * - Captures post-render input events (hover detection, selection, etc.)
         *
         * Implementations:
         * - ImNodesCanvasEditor: Calls ImNodes::EndNodeEditor()
         * - CustomCanvasEditor: Performs hover/selection detection, context menu logic
         */
        virtual void EndRender() = 0;

        // ====================================================================
        // Coordinate Transformation Systems
        // ====================================================================

        /**
         * @brief Transform screen space coordinates to canvas space
         * @param screenPos Coordinates in screen space (pixels from top-left of window)
         * @return Coordinates in canvas space (logical coordinates after pan/zoom)
         *
         * Canvas space = screen space transformed by:
         * - Subtract canvas origin (where canvas starts on screen)
         * - Subtract pan offset (where user panned to)
         * - Divide by zoom level (scale to logical units)
         *
         * Formula: canvas = (screen - canvasOrigin - pan) / zoom
         */
        virtual ImVec2 ScreenToCanvas(const ImVec2& screenPos) const = 0;

        /**
         * @brief Transform canvas space coordinates to screen space
         * @param canvasPos Coordinates in canvas space (logical coordinates)
         * @return Coordinates in screen space (pixels on screen)
         *
         * Inverse of ScreenToCanvas:
         * screen = canvasPos * zoom + pan + canvasOrigin
         */
        virtual ImVec2 CanvasToScreen(const ImVec2& canvasPos) const = 0;

        /**
         * @brief Transform editor space to grid space
         * @param editorPos Coordinates in editor space
         * @return Coordinates in grid space (pan-independent)
         *
         * Grid space removes the pan offset, allowing positions to be stored
         * pan-independently. Useful for serialization.
         *
         * For imnodes: grid space = SetNodeGridSpacePos (pan-independent storage)
         * For custom: grid space = editorPos - pan offset
         */
        virtual ImVec2 EditorToGrid(const ImVec2& editorPos) const = 0;

        /**
         * @brief Transform grid space to editor space
         * @param gridPos Coordinates in grid space
         * @return Coordinates in editor space
         *
         * Inverse of EditorToGrid, adds back the pan offset.
         */
        virtual ImVec2 GridToEditor(const ImVec2& gridPos) const = 0;

        // ====================================================================
        // Pan Management
        // ====================================================================

        /**
         * @brief Get current pan offset
         * @return Offset vector representing where canvas origin is panned to
         * @note For imnodes: reads EditorContextGetPanning()
         * @note For custom: returns m_canvasOffset
         */
        virtual ImVec2 GetPan() const = 0;

        /**
         * @brief Set pan offset directly
         * @param offset New pan offset
         * @details Used for programmatic pan (keyboard shortcut, reset view, etc.)
         *
         * @note For imnodes: calls EditorContextResetPanning()
         * @note For custom: updates m_canvasOffset directly
         */
        virtual void SetPan(const ImVec2& offset) = 0;

        /**
         * @brief Pan by delta amount
         * @param delta Relative pan movement
         * @note Convenience method: SetPan(GetPan() + delta)
         */
        virtual void PanBy(const ImVec2& delta) = 0;

        /**
         * @brief Reset pan to origin (0, 0)
         * @note Convenience method: SetPan(ImVec2(0, 0))
         */
        virtual void ResetPan() = 0;

        // ====================================================================
        // Zoom Management
        // ====================================================================

        /**
         * @brief Get current zoom level
         * @return Zoom scale (1.0 = 100%, 2.0 = 200%, 0.5 = 50%)
         * @note For imnodes: always returns 1.0f (no zoom support)
         * @note For custom: returns m_canvasZoom (0.1f to 3.0f)
         */
        virtual float GetZoom() const = 0;

        /**
         * @brief Set zoom level directly
         * @param scale New zoom scale
         * @param zoomCenter Optional center point for zoom (in screen space)
         * @details If zoomCenter provided, zoom maintains visual position of that point
         *
         * @note For imnodes: this is a no-op (returns without change)
         * @note For custom: updates m_canvasZoom with zoom-center adjustment
         */
        virtual void SetZoom(float scale, const ImVec2* zoomCenter = nullptr) = 0;

        /**
         * @brief Zoom by multiplier
         * @param factor Zoom multiplier (1.1 = 10% zoom in, 0.9 = 10% zoom out)
         * @param zoomCenter Optional center point for zoom
         * @note Convenience: SetZoom(GetZoom() * factor, zoomCenter)
         */
        virtual void ZoomBy(float factor, const ImVec2* zoomCenter = nullptr) = 0;

        /**
         * @brief Get zoom limits (min, max)
         * @return {minZoom, maxZoom}
         * @note For imnodes: {1.0f, 1.0f} (no zoom support)
         * @note For custom: {0.1f, 3.0f}
         */
        virtual ImVec2 GetZoomLimits() const = 0;

        /**
         * @brief Reset zoom to 1.0x
         * @note Convenience: SetZoom(1.0f)
         */
        virtual void ResetZoom() = 0;

        /**
         * @brief Reset both pan and zoom to defaults
         * @note Convenience: ResetPan(); ResetZoom();
         */
        virtual void ResetView() = 0;

        // ====================================================================
        // Grid Management
        // ====================================================================

        /**
         * @brief Get current grid configuration
         * @return GridConfig struct with all grid parameters
         * @note Reflects current zoom state in scaledSpacing
         */
        virtual CanvasGridRenderer::GridConfig GetGridConfig() const = 0;

        /**
         * @brief Render grid using current configuration
         * @param preset Style preset to use
         * @details Delegates to CanvasGridRenderer::RenderGrid()
         */
        virtual void RenderGrid(CanvasGridRenderer::GridStylePreset preset) = 0;

        /**
         * @brief Enable/disable grid rendering
         * @param enabled True to show grid, false to hide
         */
        virtual void SetGridVisible(bool enabled) = 0;

        /**
         * @brief Check if grid is visible
         * @return True if grid will be rendered
         */
        virtual bool IsGridVisible() const = 0;

        // ====================================================================
        // Canvas Properties
        // ====================================================================

        /**
         * @brief Get canvas position on screen
         * @return Top-left corner of canvas in screen space (pixels)
         */
        virtual ImVec2 GetCanvasScreenPos() const = 0;

        /**
         * @brief Get canvas size in screen space
         * @return Width and height of canvas rendering area (pixels)
         */
        virtual ImVec2 GetCanvasSize() const = 0;

        /**
         * @brief Get canvas visible area as AABB in canvas space
         * @return {minCorner, maxCorner} in canvas logical coordinates
         * @details Useful for culling: which nodes are visible on screen?
         */
        virtual void GetCanvasVisibleBounds(ImVec2& outMin, ImVec2& outMax) const = 0;

        /**
         * @brief Check if point in screen space is within canvas bounds
         * @param screenPos Position in screen space
         * @return True if point is inside canvas area
         */
        virtual bool IsPointInCanvas(const ImVec2& screenPos) const = 0;

        // ====================================================================
        // Context Information
        // ====================================================================

        /**
         * @brief Check if canvas is being hovered by mouse
         * @return True if mouse is over canvas and canvas has focus
         */
        virtual bool IsCanvasHovered() const = 0;

        /**
         * @brief Get the name/identifier of this canvas
         * @return String identifier for debugging and logging
         */
        virtual const char* GetCanvasName() const = 0;
    };

} // namespace Olympe
