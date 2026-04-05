/**
 * @file ImNodesCanvasEditor.cpp
 * @brief Implementation of ImNodesCanvasEditor adapter
 * @details Wraps imnodes with standardized ICanvasEditor interface
 */

#include "ImNodesCanvasEditor.h"

namespace Olympe
{
    ImNodesCanvasEditor::ImNodesCanvasEditor(
        const char* name,
        ImVec2 canvasScreenPos,
        ImVec2 canvasSize,
        ImNodesEditorContext* imnodesContext
    )
        : m_name(name)
        , m_canvasScreenPos(canvasScreenPos)
        , m_canvasSize(canvasSize)
        , m_imnodesContext(imnodesContext)
        , m_gridVisible(true)
    {
    }

    // ========================================================================
    // Lifecycle Management
    // ========================================================================

    void ImNodesCanvasEditor::BeginRender()
    {
        // Switch to this canvas's imnodes context
        if (m_imnodesContext)
        {
            ImNodes::EditorContextSet(m_imnodesContext);
        }

        // Start imnodes rendering cycle
        ImNodes::BeginNodeEditor();
    }

    void ImNodesCanvasEditor::EndRender()
    {
        // End imnodes rendering cycle
        ImNodes::EndNodeEditor();

        // Note: Grid rendering is called separately via RenderGrid()
        // This happens before EndNodeEditor in the render loop
    }

    // ========================================================================
    // Coordinate Transformation Systems
    // ========================================================================

    ImVec2 ImNodesCanvasEditor::ScreenToCanvas(const ImVec2& screenPos) const
    {
        ImVec2 pan = GetPan();
        ImVec2 canvasPos = ImVec2(screenPos.x - m_canvasScreenPos.x, screenPos.y - m_canvasScreenPos.y);
        ImVec2 result = ImVec2(canvasPos.x - pan.x, canvasPos.y - pan.y);
        return result;
    }

    ImVec2 ImNodesCanvasEditor::CanvasToScreen(const ImVec2& canvasPos) const
    {
        ImVec2 pan = GetPan();
        ImVec2 result = ImVec2(canvasPos.x + pan.x + m_canvasScreenPos.x, canvasPos.y + pan.y + m_canvasScreenPos.y);
        return result;
    }

    ImVec2 ImNodesCanvasEditor::EditorToGrid(const ImVec2& editorPos) const
    {
        // Grid space = editor space - pan offset
        ImVec2 pan = GetPan();
        return ImVec2(editorPos.x - pan.x, editorPos.y - pan.y);
    }

    ImVec2 ImNodesCanvasEditor::GridToEditor(const ImVec2& gridPos) const
    {
        // Editor space = grid space + pan offset
        ImVec2 pan = GetPan();
        return ImVec2(gridPos.x + pan.x, gridPos.y + pan.y);
    }

    // ========================================================================
    // Pan Management
    // ========================================================================

    ImVec2 ImNodesCanvasEditor::GetPan() const
    {
        return ImNodes::EditorContextGetPanning();
    }

    void ImNodesCanvasEditor::SetPan(const ImVec2& offset)
    {
        ImNodes::EditorContextResetPanning(offset);
    }

    void ImNodesCanvasEditor::PanBy(const ImVec2& delta)
    {
        ImVec2 newPan = ImVec2(GetPan().x + delta.x, GetPan().y + delta.y);
        SetPan(newPan);
    }

    void ImNodesCanvasEditor::ResetPan()
    {
        SetPan(ImVec2(0.0f, 0.0f));
    }

    // ========================================================================
    // Grid Management
    // ========================================================================

    CanvasGridRenderer::GridConfig ImNodesCanvasEditor::GetGridConfig() const
    {
        // Get the VisualScript style preset
        CanvasGridRenderer::GridConfig config = 
            CanvasGridRenderer::GetStylePreset(CanvasGridRenderer::Style_VisualScript);

        // Apply canvas-specific parameters
        config.canvasPos = m_canvasScreenPos;
        config.canvasSize = m_canvasSize;
        config.zoom = 1.0f;  // imnodes always 1.0x
        config.offsetX = GetPan().x;
        config.offsetY = GetPan().y;

        return config;
    }

    void ImNodesCanvasEditor::RenderGrid(CanvasGridRenderer::GridStylePreset preset)
    {
        if (!m_gridVisible)
            return;

        CanvasGridRenderer::GridConfig config = 
            CanvasGridRenderer::GetStylePreset(preset);

        config.canvasPos = m_canvasScreenPos;
        config.canvasSize = m_canvasSize;
        config.zoom = 1.0f;  // imnodes always 1.0x
        config.offsetX = GetPan().x;
        config.offsetY = GetPan().y;

        CanvasGridRenderer::RenderGrid(config);
    }

    void ImNodesCanvasEditor::SetGridVisible(bool enabled)
    {
        m_gridVisible = enabled;
    }

    // ========================================================================
    // Canvas Properties
    // ========================================================================

    void ImNodesCanvasEditor::GetCanvasVisibleBounds(ImVec2& outMin, ImVec2& outMax) const
    {
        // Canvas visible area in screen space
        ImVec2 screenMin = m_canvasScreenPos;
        ImVec2 screenMax = ImVec2(m_canvasScreenPos.x + m_canvasSize.x, m_canvasScreenPos.y + m_canvasSize.y);

        // Convert to canvas space
        outMin = ScreenToCanvas(screenMin);
        outMax = ScreenToCanvas(screenMax);
    }

    bool ImNodesCanvasEditor::IsPointInCanvas(const ImVec2& screenPos) const
    {
        return screenPos.x >= m_canvasScreenPos.x && 
               screenPos.x <= m_canvasScreenPos.x + m_canvasSize.x &&
               screenPos.y >= m_canvasScreenPos.y && 
               screenPos.y <= m_canvasScreenPos.y + m_canvasSize.y;
    }

    // ========================================================================
    // Context Information
    // ========================================================================

    bool ImNodesCanvasEditor::IsCanvasHovered() const
    {
        return ImNodes::IsEditorHovered();
    }

} // namespace Olympe
