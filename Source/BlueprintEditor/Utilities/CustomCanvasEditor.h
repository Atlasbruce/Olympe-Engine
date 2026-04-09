/**
 * @file CustomCanvasEditor.h
 * @brief ICanvasEditor implementation for custom canvas with zoom support (PrefabCanvas)
 * @details Provides pan, zoom, and coordinate transformation without imnodes dependency
 *
 * @design
 * - Full manual control of pan/zoom
 * - Supports zoom range 0.1x - 3.0x (configurable)
 * - Grid delegated to CanvasGridRenderer with zoom-aware scaling
 * - Coordinate transformation includes zoom scaling
 * - Input handling (middle-mouse pan, scroll zoom)
 *
 * @usage
 * // In EntityPrefabRenderer::Render()
 * m_canvasEditor = std::make_unique<CustomCanvasEditor>("ComponentCanvas", canvasScreenPos, canvasSize);
 *
 * m_canvasEditor->BeginRender();
 * {
 *   // Render custom nodes
 *   RenderMyComponentNodes();
 * }
 * m_canvasEditor->EndRender();
 *
 * @compatibility C++14
 */

#pragma once

#include "ICanvasEditor.h"
#include "CanvasMinimapRenderer.h"
#include <string>
#include <memory>

namespace Olympe
{
    class CustomCanvasEditor : public ICanvasEditor
    {
    public:
        /**
         * @brief Construct custom canvas editor with zoom support
         * @param name Identifier for this editor (e.g., "ComponentCanvas")
         * @param canvasScreenPos Top-left corner of canvas on screen (pixels)
         * @param canvasSize Width/height of canvas area (pixels)
         * @param initialZoom Initial zoom level (default 1.0f)
         * @param minZoom Minimum zoom allowed (default 0.1f)
         * @param maxZoom Maximum zoom allowed (default 3.0f)
         */
        CustomCanvasEditor(
            const char* name,
            ImVec2 canvasScreenPos,
            ImVec2 canvasSize,
            float initialZoom = 1.0f,
            float minZoom = 0.1f,
            float maxZoom = 3.0f
        );

        virtual ~CustomCanvasEditor() = default;

        // ====================================================================
        // Lifecycle Management
        // ====================================================================

        /**
         * @brief Begin rendering cycle
         * @details Updates pan/zoom from input (mouse/scroll)
         */
        virtual void BeginRender() override;

        /**
         * @brief End rendering cycle
         * @details Finalizes input state for frame
         */
        virtual void EndRender() override;

        // ====================================================================
        // Coordinate Transformation Systems
        // ====================================================================

        /**
         * @brief Transform screen space to canvas space
         * @details Includes zoom scaling:
         *   canvas = (screen - canvasScreenPos - pan) / zoom
         */
        virtual ImVec2 ScreenToCanvas(const ImVec2& screenPos) const override;

        /**
         * @brief Transform canvas space to screen space
         * @details Includes zoom scaling:
         *   screen = canvas * zoom + pan + canvasScreenPos
         */
        virtual ImVec2 CanvasToScreen(const ImVec2& canvasPos) const override;

        /**
         * @brief Transform editor space to grid space
         * @details Grid space removes pan offset (but keeps zoom):
         *   grid = editor - pan
         */
        virtual ImVec2 EditorToGrid(const ImVec2& editorPos) const override;

        /**
         * @brief Transform grid space to editor space
         * @details Adds pan offset back (keeps zoom):
         *   editor = grid + pan
         */
        virtual ImVec2 GridToEditor(const ImVec2& gridPos) const override;

        // ====================================================================
        // Pan Management
        // ====================================================================

        /**
         * @brief Get current pan offset
         * @return Current pan offset vector
         */
        virtual ImVec2 GetPan() const override { return m_canvasOffset; }

        /**
         * @brief Set pan offset directly
         * @param offset New pan offset
         */
        virtual void SetPan(const ImVec2& offset) override { m_canvasOffset = offset; }

        /**
         * @brief Pan by delta
         */
        virtual void PanBy(const ImVec2& delta) override 
        { 
            m_canvasOffset = ImVec2(m_canvasOffset.x + delta.x, m_canvasOffset.y + delta.y); 
        }

        /**
         * @brief Reset pan to origin
         */
        virtual void ResetPan() override { m_canvasOffset = ImVec2(0.0f, 0.0f); }

        // ====================================================================
        // Zoom Management
        // ====================================================================

        /**
         * @brief Get current zoom level
         * @return Current zoom scale (1.0 = 100%)
         */
        virtual float GetZoom() const override { return m_canvasZoom; }

        /**
         * @brief Set zoom level directly
         * @param scale New zoom scale (clamped to min/max)
         * @param zoomCenter Optional screen-space point to maintain visual position during zoom
         * @details If zoomCenter is provided, adjusts pan so that the point at zoomCenter
         *          maintains its visual position on screen after zoom changes.
         */
        virtual void SetZoom(float scale, const ImVec2* zoomCenter = nullptr) override;

        /**
         * @brief Zoom by multiplier
         * @param factor Zoom factor (1.1 = 10% zoom in, 0.9 = 10% zoom out)
         * @param zoomCenter Optional screen-space point to maintain visual position
         */
        virtual void ZoomBy(float factor, const ImVec2* zoomCenter = nullptr) override;

        /**
         * @brief Get zoom limits
         * @return {minZoom, maxZoom}
         */
        virtual ImVec2 GetZoomLimits() const override { return ImVec2(m_minZoom, m_maxZoom); }

        /**
         * @brief Reset zoom to 1.0x
         */
        virtual void ResetZoom() override { m_canvasZoom = 1.0f; }

        /**
         * @brief Reset view (pan and zoom)
         */
        virtual void ResetView() override
        {
            ResetPan();
            ResetZoom();
        }

        // ====================================================================
        // Grid Management
        // ====================================================================

        /**
         * @brief Get current grid configuration
         * @return GridConfig with zoom-aware scaling
         */
        virtual CanvasGridRenderer::GridConfig GetGridConfig() const override;

        /**
         * @brief Render grid
         * @param preset Style preset
         */
        virtual void RenderGrid(CanvasGridRenderer::GridStylePreset preset) override;

        /**
         * @brief Set grid visibility
         * @param enabled True to show grid
         */
        virtual void SetGridVisible(bool enabled) override;

        /**
         * @brief Check if grid is visible
         * @return True if grid will be rendered
         */
        virtual bool IsGridVisible() const override { return m_gridVisible; }

        // ====================================================================
        // Canvas Properties
        // ====================================================================

        /**
         * @brief Get canvas screen position
         * @return Stored canvasScreenPos
         */
        virtual ImVec2 GetCanvasScreenPos() const override { return m_canvasScreenPos; }

        /**
         * @brief Get canvas size
         * @return Stored canvasSize
         */
        virtual ImVec2 GetCanvasSize() const override { return m_canvasSize; }

        /**
         * @brief Set canvas screen position (updates each frame)
         * @param screenPos New screen position
         */
        virtual void SetCanvasScreenPos(const ImVec2& screenPos) override { m_canvasScreenPos = screenPos; }

        /**
         * @brief Set canvas size (updates each frame)
         * @param size New canvas size
         */
        virtual void SetCanvasSize(const ImVec2& size) override { m_canvasSize = size; }

        /**
         * @brief Get canvas visible bounds in canvas space
         * @param outMin Top-left corner
         * @param outMax Bottom-right corner
         */
        virtual void GetCanvasVisibleBounds(ImVec2& outMin, ImVec2& outMax) const override;

        /**
         * @brief Check if point is in canvas
         * @param screenPos Position in screen space
         * @return True if within canvas bounds
         */
        virtual bool IsPointInCanvas(const ImVec2& screenPos) const override;

        // ====================================================================
        // Context Information
        // ====================================================================

        /**
         * @brief Check if canvas is hovered
         * @return True if mouse is over canvas
         */
        virtual bool IsCanvasHovered() const override;

        /**
         * @brief Get canvas name
         * @return Stored canvas name
         */
        virtual const char* GetCanvasName() const override { return m_name.c_str(); }

        // ====================================================================
        // Minimap Management
        // ====================================================================

        /**
         * @brief Render minimap overlay for custom canvas
         * @details Uses CanvasMinimapRenderer for unified appearance
         */
        virtual void RenderMinimap() override;

        /**
         * @brief Enable/disable minimap
         * @param enabled True to show minimap
         */
        virtual void SetMinimapVisible(bool enabled) override;

        /**
         * @brief Check if minimap is visible
         * @return True if minimap will be rendered
         */
        virtual bool IsMinimapVisible() const override;

        /**
         * @brief Set minimap size ratio
         * @param scale Size ratio (0.05 - 0.5)
         */
         virtual void SetMinimapSize(float scale) override;

        /**
         * @brief Get minimap size ratio
         * @return Current size ratio
         */
        virtual float GetMinimapSize() const override;

        /**
         * @brief Set minimap position (corner)
         * @param position Position enum (0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight)
         */
        virtual void SetMinimapPosition(int position) override;

        /**
         * @brief Get minimap position
         * @return Position enum value
         */
        virtual int GetMinimapPosition() const override;

        /**
         * @brief Update minimap with current graph data
         * @param nodes Vector of (nodeId, posX, posY, width, height) tuples
         * @param graphMinX Graph bounds left
         * @param graphMaxX Graph bounds right
         * @param graphMinY Graph bounds top
         * @param graphMaxY Graph bounds bottom
         */
        void UpdateMinimapNodes(
            const std::vector<std::tuple<int, float, float, float, float>>& nodes,
            float graphMinX, float graphMaxX, float graphMinY, float graphMaxY);

        /**
         * @brief Update minimap viewport from visible canvas area
         */
        void UpdateMinimapViewport(
            float viewMinX, float viewMaxX, float viewMinY, float viewMaxY,
            float graphMinX, float graphMaxX, float graphMinY, float graphMaxY);

        // ====================================================================
        // Input Configuration
        // ====================================================================

        /**
         * @brief Enable/disable middle-mouse panning
         * @param enabled True to allow pan via middle mouse
         * @note Default: enabled
         */
        void SetMiddleMousePanEnabled(bool enabled) { m_middleMousePanEnabled = enabled; }

        /**
         * @brief Enable/disable scroll wheel zooming
         * @param enabled True to allow zoom via scroll wheel
         * @note Default: enabled
         */
        void SetScrollZoomEnabled(bool enabled) { m_scrollZoomEnabled = enabled; }

        /**
         * @brief Set scroll zoom speed (multiplier)
         * @param speed Zoom factor per scroll notch (default 1.1)
         * @note Higher = faster zoom response
         */
        void SetScrollZoomSpeed(float speed) { m_scrollZoomSpeed = speed; }

    private:
        // Canvas properties
        std::string m_name;
        ImVec2 m_canvasScreenPos;
        ImVec2 m_canvasSize;

        // Pan and zoom state
        ImVec2 m_canvasOffset;  // Pan offset (where origin is panned to)
        float m_canvasZoom;     // Zoom level (1.0 = 100%)
        float m_minZoom;        // Minimum allowed zoom
        float m_maxZoom;        // Maximum allowed zoom

        // Grid state
        bool m_gridVisible;

        // Minimap state
        std::unique_ptr<CanvasMinimapRenderer> m_minimapRenderer;

        // Input state
        ImVec2 m_lastMousePos;           // Previous frame mouse position
        bool m_isPanning;                // True if currently middle-mouse dragging
        bool m_middleMousePanEnabled;    // Enable/disable middle-mouse pan
        bool m_scrollZoomEnabled;        // Enable/disable scroll zoom
        float m_scrollZoomSpeed;         // Zoom factor per scroll (default 1.1)

        // Internal helper methods
        /**
         * @brief Update pan/zoom from current frame input
         * @details Called in BeginRender()
         */
        void UpdateInputState();

        /**
         * @brief Handle middle-mouse panning
         */
        void HandlePanning();

        /**
         * @brief Handle scroll wheel zooming
         */
        void HandleZooming();

        /**
         * @brief Clamp zoom to valid range
         * @param zoom Value to clamp
         * @return Clamped value
         */
        float ClampZoom(float zoom) const;
    };

} // namespace Olympe
