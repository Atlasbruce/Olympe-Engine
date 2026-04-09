/**
 * @file ImNodesCanvasEditor.h
 * @brief ICanvasEditor adapter for ImNodes-based editors (VisualScript)
 * @details Wraps ImNodes::BeginNodeEditor/EndNodeEditor with standardized interface
 *
 * @design
 * - Minimal wrapper around imnodes
 * - Exposes pan control via EditorContextGetPanning/EditorContextResetPanning
 * - Zoom is read-only (always 1.0f - imnodes doesn't support zoom)
 * - Coordinates use grid space (pan-independent storage)
 * - Grid delegated to CanvasGridRenderer with Style_VisualScript preset
 *
 * @usage
 * // In VisualScriptEditorPanel::RenderCanvas()
 * m_canvasEditor = std::make_unique<ImNodesCanvasEditor>("VisualScript", canvasScreenPos, canvasSize);
 *
 * m_canvasEditor->BeginRender();
 * {
 *   // Render nodes using ImNodes API as usual
 *   ImNodes::BeginNode(nodeId);
 *   // ... node content
 *   ImNodes::EndNode();
 * }
 * m_canvasEditor->EndRender();
 *
 * @compatibility C++14
 */

#pragma once

#include "ICanvasEditor.h"
#include "../../third_party/imnodes/imnodes.h"
#include <string>
#include <vector>
#include <tuple>

namespace Olympe
{
    class ImNodesCanvasEditor : public ICanvasEditor
    {
    public:
        /**
         * @brief Construct imnodes-based canvas editor
         * @param name Identifier for this editor (e.g., "VisualScript")
         * @param canvasScreenPos Top-left corner of canvas on screen (pixels)
         * @param canvasSize Width/height of canvas area (pixels)
         * @param imnodesContext Optional ImNodes context (nullptr = use current)
         */
        ImNodesCanvasEditor(
            const char* name,
            ImVec2 canvasScreenPos,
            ImVec2 canvasSize,
            ImNodesEditorContext* imnodesContext = nullptr
        );

        virtual ~ImNodesCanvasEditor() = default;

        // ====================================================================
        // Lifecycle Management
        // ====================================================================

        /**
         * @brief Begin rendering cycle (calls ImNodes::BeginNodeEditor)
         */
        virtual void BeginRender() override;

        /**
         * @brief End rendering cycle (calls ImNodes::EndNodeEditor)
         */
        virtual void EndRender() override;

        // ====================================================================
        // Coordinate Transformation Systems
        // ====================================================================

        /**
         * @brief Transform screen space to canvas space
         * @details For imnodes (zoom=1.0):
         *   canvas = screen - canvasScreenPos - pan
         */
        virtual ImVec2 ScreenToCanvas(const ImVec2& screenPos) const override;

        /**
         * @brief Transform canvas space to screen space
         * @details For imnodes (zoom=1.0):
         *   screen = canvas + pan + canvasScreenPos
         */
        virtual ImVec2 CanvasToScreen(const ImVec2& canvasPos) const override;

        /**
         * @brief Transform editor space to grid space (removes pan)
         * @details For imnodes:
         *   grid = editor - pan
         */
        virtual ImVec2 EditorToGrid(const ImVec2& editorPos) const override;

        /**
         * @brief Transform grid space to editor space (adds pan)
         * @details For imnodes:
         *   editor = grid + pan
         */
        virtual ImVec2 GridToEditor(const ImVec2& gridPos) const override;

        // ====================================================================
        // Pan Management
        // ====================================================================

        /**
         * @brief Get current pan offset
         * @return EditorContextGetPanning() from imnodes
         */
        virtual ImVec2 GetPan() const override;

        /**
         * @brief Set pan offset
         * @param offset New pan offset
         * @details Calls EditorContextResetPanning()
         */
        virtual void SetPan(const ImVec2& offset) override;

        /**
         * @brief Pan by delta
         */
        virtual void PanBy(const ImVec2& delta) override;

        /**
         * @brief Reset pan to origin
         */
        virtual void ResetPan() override;

        // ====================================================================
        // Zoom Management
        // ====================================================================

        /**
         * @brief Get current zoom level
         * @return Always 1.0f (imnodes doesn't support zoom)
         */
        virtual float GetZoom() const override { return 1.0f; }

        /**
         * @brief Set zoom level (no-op for imnodes)
         * @details This is a no-op because imnodes doesn't expose zoom control.
         * If zoom is needed, use CustomCanvasEditor instead.
         */
        virtual void SetZoom(float scale, const ImVec2* zoomCenter = nullptr) override
        {
            (void)scale;
            (void)zoomCenter;
            // No-op: imnodes doesn't support zoom
        }

        /**
         * @brief Zoom by multiplier (no-op for imnodes)
         */
        virtual void ZoomBy(float factor, const ImVec2* zoomCenter = nullptr) override
        {
            (void)factor;
            (void)zoomCenter;
            // No-op: imnodes doesn't support zoom
        }

        /**
         * @brief Get zoom limits
         * @return {1.0f, 1.0f} (fixed, no zoom support)
         */
        virtual ImVec2 GetZoomLimits() const override { return ImVec2(1.0f, 1.0f); }

        /**
         * @brief Reset zoom to 1.0x (no-op, already 1.0f)
         */
        virtual void ResetZoom() override
        {
            // No-op: already at 1.0f
        }

        /**
         * @brief Reset view to defaults (pan only, zoom already 1.0f)
         */
        virtual void ResetView() override
        {
            ResetPan();
            // Zoom is already 1.0f, no need to reset
        }

        // ====================================================================
        // Grid Management
        // ====================================================================

        /**
         * @brief Get current grid configuration
         * @return GridConfig from CanvasGridRenderer with current pan/zoom
         */
        virtual CanvasGridRenderer::GridConfig GetGridConfig() const override;

        /**
         * @brief Render grid
         * @param preset Style preset (typically Style_VisualScript)
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
         * @brief Set canvas screen position (call each frame to update)
         * @param screenPos Top-left corner position in screen space
         * @details Required for minimap coordinate calculations each frame
         */
        virtual void SetCanvasScreenPos(const ImVec2& screenPos) override { m_canvasScreenPos = screenPos; }

        /**
         * @brief Set canvas size (call each frame to update)
         * @param size Width and height in screen space
         * @details Required for minimap rendering calculations each frame
         */
        virtual void SetCanvasSize(const ImVec2& size) override { m_canvasSize = size; }

        /**
         * @brief Get canvas visible bounds in canvas space
         * @param outMin Top-left corner of visible area
         * @param outMax Bottom-right corner of visible area
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
         * @return Result of ImNodes::IsEditorHovered()
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
         * @brief Render minimap overlay
         * @details Calls ImNodes::MiniMap() with configured size and position
         */
        virtual void RenderMinimap() override;

        /**
         * @brief Enable/disable minimap rendering
         * @param enabled True to show minimap
         */
        virtual void SetMinimapVisible(bool enabled) override;

        /**
         * @brief Check if minimap is visible
         * @return True if minimap will be rendered
         */
        virtual bool IsMinimapVisible() const override { return m_minimapVisible; }

        /**
         * @brief Set minimap size scale
         * @param scale Size multiplier (0.1f to 0.3f typical)
         */
        virtual void SetMinimapSize(float scale) override;

        /**
         * @brief Get minimap size scale
         * @return Current size multiplier
         */
        virtual float GetMinimapSize() const override { return m_minimapSize; }

        /**
         * @brief Set minimap position
         * @param position ImNodesMiniMapLocation enum value
         */
        virtual void SetMinimapPosition(int position) override;

             /**
              * @brief Get minimap position
              * @return Current ImNodesMiniMapLocation value
              */
             virtual int GetMinimapPosition() const override { return m_minimapPosition; }

             /**
              * @brief Update minimap nodes (no-op for imnodes)
              * @details ImNodes manages minimap data internally, so this is unused
              */
             virtual void UpdateMinimapNodes(
                 const std::vector<std::tuple<int, float, float, float, float>>& nodes,
                 float graphMinX, float graphMaxX, float graphMinY, float graphMaxY) override
             {
                 // No-op: ImNodes::MiniMap() handles this internally
                 (void)nodes; (void)graphMinX; (void)graphMaxX; (void)graphMinY; (void)graphMaxY;
             }

             /**
              * @brief Update minimap viewport (no-op for imnodes)
              * @details ImNodes manages minimap viewport internally, so this is unused
              */
             virtual void UpdateMinimapViewport(
                 float viewMinX, float viewMaxX, float viewMinY, float viewMaxY,
                 float graphMinX, float graphMaxX, float graphMinY, float graphMaxY) override
             {
                 // No-op: ImNodes::MiniMap() handles this internally
                 (void)viewMinX; (void)viewMaxX; (void)viewMinY; (void)viewMaxY;
                 (void)graphMinX; (void)graphMaxX; (void)graphMinY; (void)graphMaxY;
             }

        private:
        std::string m_name;
        ImVec2 m_canvasScreenPos;
        ImVec2 m_canvasSize;
        ImNodesEditorContext* m_imnodesContext;
        bool m_gridVisible;

        // Minimap configuration
        bool m_minimapVisible = true;
        float m_minimapSize = 0.15f;
        int m_minimapPosition = ImNodesMiniMapLocation_BottomRight;
    };

} // namespace Olympe
