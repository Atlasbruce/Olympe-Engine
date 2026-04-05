/**
 * @file CustomCanvasEditor.cpp
 * @brief Implementation of CustomCanvasEditor with pan/zoom support
 */

#include "CustomCanvasEditor.h"
#include "../../third_party/imgui/imgui.h"

namespace Olympe
{
    CustomCanvasEditor::CustomCanvasEditor(
        const char* name,
        ImVec2 canvasScreenPos,
        ImVec2 canvasSize,
        float initialZoom,
        float minZoom,
        float maxZoom
    )
        : m_name(name)
        , m_canvasScreenPos(canvasScreenPos)
        , m_canvasSize(canvasSize)
        , m_canvasOffset(0.0f, 0.0f)
        , m_canvasZoom(initialZoom)
        , m_minZoom(minZoom)
        , m_maxZoom(maxZoom)
        , m_gridVisible(true)
        , m_lastMousePos(ImGui::GetMousePos())
        , m_isPanning(false)
        , m_middleMousePanEnabled(true)
        , m_scrollZoomEnabled(true)
        , m_scrollZoomSpeed(1.1f)
    {
        m_canvasZoom = ClampZoom(m_canvasZoom);
    }

    // ========================================================================
    // Lifecycle Management
    // ========================================================================

    void CustomCanvasEditor::BeginRender()
    {
        // Update pan/zoom from input before rendering
        UpdateInputState();
    }

    void CustomCanvasEditor::EndRender()
    {
        // No-op: input handling is done in BeginRender
        // Grid rendering happens separately
    }

    // ========================================================================
    // Coordinate Transformation Systems
    // ========================================================================

    ImVec2 CustomCanvasEditor::ScreenToCanvas(const ImVec2& screenPos) const
    {
        // screen → canvas: (screen - canvasScreenPos - pan) / zoom
        ImVec2 relativeToCanvas = ImVec2(screenPos.x - m_canvasScreenPos.x, screenPos.y - m_canvasScreenPos.y);
        ImVec2 result = ImVec2((relativeToCanvas.x - m_canvasOffset.x) / m_canvasZoom,
                              (relativeToCanvas.y - m_canvasOffset.y) / m_canvasZoom);
        return result;
    }

    ImVec2 CustomCanvasEditor::CanvasToScreen(const ImVec2& canvasPos) const
    {
        // canvas → screen: canvas * zoom + pan + canvasScreenPos
        ImVec2 scaledByZoom = ImVec2(canvasPos.x * m_canvasZoom, canvasPos.y * m_canvasZoom);
        ImVec2 result = ImVec2(scaledByZoom.x + m_canvasOffset.x + m_canvasScreenPos.x,
                              scaledByZoom.y + m_canvasOffset.y + m_canvasScreenPos.y);
        return result;
    }

    ImVec2 CustomCanvasEditor::EditorToGrid(const ImVec2& editorPos) const
    {
        // Grid space = editor space - pan (no zoom applied to grid space)
        return ImVec2(editorPos.x - m_canvasOffset.x, editorPos.y - m_canvasOffset.y);
    }

    ImVec2 CustomCanvasEditor::GridToEditor(const ImVec2& gridPos) const
    {
        // Editor space = grid space + pan
        return ImVec2(gridPos.x + m_canvasOffset.x, gridPos.y + m_canvasOffset.y);
    }

    // ========================================================================
    // Zoom Management
    // ========================================================================

    void CustomCanvasEditor::SetZoom(float scale, const ImVec2* zoomCenter)
    {
        float oldZoom = m_canvasZoom;
        m_canvasZoom = ClampZoom(scale);

        // If zoom center provided, adjust pan to keep that point visually stationary
        if (zoomCenter && oldZoom != m_canvasZoom)
        {
            // In screen space, zoomCenter is the point we want to keep stationary
            // Convert to canvas space at old zoom
            float centerX = zoomCenter->x - m_canvasScreenPos.x - m_canvasOffset.x;
            float centerY = zoomCenter->y - m_canvasScreenPos.y - m_canvasOffset.y;
            ImVec2 canvasPointAtOldZoom = ImVec2(centerX / oldZoom, centerY / oldZoom);

            // Calculate where this point would be at new zoom with current pan
            ImVec2 screenPointAtNewZoom = ImVec2(
                canvasPointAtOldZoom.x * m_canvasZoom + m_canvasOffset.x + m_canvasScreenPos.x,
                canvasPointAtOldZoom.y * m_canvasZoom + m_canvasOffset.y + m_canvasScreenPos.y
            );

            // Adjust pan so the point ends up back at zoomCenter
            ImVec2 delta = ImVec2(zoomCenter->x - screenPointAtNewZoom.x, 
                                 zoomCenter->y - screenPointAtNewZoom.y);
            m_canvasOffset = ImVec2(m_canvasOffset.x + delta.x, m_canvasOffset.y + delta.y);
        }
    }

    void CustomCanvasEditor::ZoomBy(float factor, const ImVec2* zoomCenter)
    {
        SetZoom(m_canvasZoom * factor, zoomCenter);
    }

    float CustomCanvasEditor::ClampZoom(float zoom) const
    {
        if (zoom < m_minZoom) return m_minZoom;
        if (zoom > m_maxZoom) return m_maxZoom;
        return zoom;
    }

    // ========================================================================
    // Grid Management
    // ========================================================================

    CanvasGridRenderer::GridConfig CustomCanvasEditor::GetGridConfig() const
    {
        // Get default preset (VisualScript style for consistency)
        CanvasGridRenderer::GridConfig config =
            CanvasGridRenderer::GetStylePreset(CanvasGridRenderer::Style_VisualScript);

        // Apply canvas-specific and zoom-specific parameters
        config.canvasPos = m_canvasScreenPos;
        config.canvasSize = m_canvasSize;
        config.zoom = m_canvasZoom;        // Include zoom scaling
        config.offsetX = m_canvasOffset.x;
        config.offsetY = m_canvasOffset.y;

        return config;
    }

    void CustomCanvasEditor::RenderGrid(CanvasGridRenderer::GridStylePreset preset)
    {
        if (!m_gridVisible)
            return;

        CanvasGridRenderer::GridConfig config =
            CanvasGridRenderer::GetStylePreset(preset);

        config.canvasPos = m_canvasScreenPos;
        config.canvasSize = m_canvasSize;
        config.zoom = m_canvasZoom;        // Include zoom scaling
        config.offsetX = m_canvasOffset.x;
        config.offsetY = m_canvasOffset.y;

        CanvasGridRenderer::RenderGrid(config);
    }

    void CustomCanvasEditor::SetGridVisible(bool enabled)
    {
        m_gridVisible = enabled;
    }

    // ========================================================================
    // Canvas Properties
    // ========================================================================

    void CustomCanvasEditor::GetCanvasVisibleBounds(ImVec2& outMin, ImVec2& outMax) const
    {
        // Canvas visible area in screen space
        ImVec2 screenMin = m_canvasScreenPos;
        ImVec2 screenMax = ImVec2(m_canvasScreenPos.x + m_canvasSize.x, m_canvasScreenPos.y + m_canvasSize.y);

        // Convert to canvas space
        outMin = ScreenToCanvas(screenMin);
        outMax = ScreenToCanvas(screenMax);
    }

    bool CustomCanvasEditor::IsPointInCanvas(const ImVec2& screenPos) const
    {
        return screenPos.x >= m_canvasScreenPos.x &&
               screenPos.x <= m_canvasScreenPos.x + m_canvasSize.x &&
               screenPos.y >= m_canvasScreenPos.y &&
               screenPos.y <= m_canvasScreenPos.y + m_canvasSize.y;
    }

    // ========================================================================
    // Context Information
    // ========================================================================

    bool CustomCanvasEditor::IsCanvasHovered() const
    {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;

        // Check if mouse is over canvas
        bool inBounds = IsPointInCanvas(mousePos);

        // Check if not blocked by other windows
        bool hasWindowFocus = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        return inBounds && hasWindowFocus;
    }

    // ========================================================================
    // Input Handling
    // ========================================================================

    void CustomCanvasEditor::UpdateInputState()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Only handle input if canvas is hovered
        if (!IsCanvasHovered())
        {
            m_lastMousePos = io.MousePos;
            m_isPanning = false;
            return;
        }

        // Handle panning
        if (m_middleMousePanEnabled)
        {
            HandlePanning();
        }

        // Handle zooming
        if (m_scrollZoomEnabled)
        {
            HandleZooming();
        }

        m_lastMousePos = io.MousePos;
    }

    void CustomCanvasEditor::HandlePanning()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Middle mouse button (button 2)
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            if (!m_isPanning)
            {
                m_isPanning = true;
                m_lastMousePos = io.MousePos;
            }

            // Pan by mouse delta
            ImVec2 delta = io.MouseDelta;
            m_canvasOffset = ImVec2(m_canvasOffset.x + delta.x, m_canvasOffset.y + delta.y);
        }
        else
        {
            m_isPanning = false;
        }
    }

    void CustomCanvasEditor::HandleZooming()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Check for scroll wheel input
        if (io.MouseWheel != 0.0f)
        {
            // Calculate zoom factor from scroll
            float factor = (io.MouseWheel > 0.0f) ? m_scrollZoomSpeed : (1.0f / m_scrollZoomSpeed);

            // Get current mouse position (screen space) as zoom center
            ImVec2 zoomCenter = io.MousePos;

            // Apply zoom
            ZoomBy(factor, &zoomCenter);
        }
    }

} // namespace Olympe
