#include "SelectionEffectRenderer.h"

namespace Olympe
{
    SelectionEffectRenderer::SelectionEffectRenderer()
        : m_glowColor(0.0f, 0.8f, 1.0f),
          m_glowAlpha(0.6f),
          m_baseGlowSize(8.0f),
          m_borderWidthMultiplier(3.0f),
          m_glowEnabled(true)
    {
    }

    void SelectionEffectRenderer::RenderSelectionGlow(
        const ImVec2& minScreen,
        const ImVec2& maxScreen,
        float canvasZoom,
        float nodeScale,
        float cornerRadius) const
    {
        if (!m_glowEnabled) { return; }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        // Compute glow size scaled by zoom and node scale
        float glowSize = m_baseGlowSize * nodeScale * canvasZoom;

        // Glow color with alpha
        ImU32 glowColor = ImGui::GetColorU32(ImVec4(
            m_glowColor.x,
            m_glowColor.y,
            m_glowColor.z,
            m_glowAlpha
        ));

        // Draw glow as expanded rounded rectangle behind the node
        drawList->AddRectFilled(
            ImVec2(minScreen.x - glowSize, minScreen.y - glowSize),
            ImVec2(maxScreen.x + glowSize, maxScreen.y + glowSize),
            glowColor,
            cornerRadius + glowSize
        );
    }

    void SelectionEffectRenderer::RenderSelectionBorder(
        const ImVec2& minScreen,
        const ImVec2& maxScreen,
        ImU32 borderColor,
        float baseWidth,
        float canvasZoom,
        float cornerRadius) const
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        // Apply multiplier to border width
        float borderWidth = baseWidth * m_borderWidthMultiplier * canvasZoom;
        borderWidth = (borderWidth < 1.0f) ? 1.0f : borderWidth;  // Minimum 1 pixel

        drawList->AddRect(
            minScreen,
            maxScreen,
            borderColor,
            cornerRadius,
            ImDrawFlags_RoundCornersAll,
            borderWidth
        );
    }

    void SelectionEffectRenderer::RenderCompleteSelection(
        const ImVec2& minScreen,
        const ImVec2& maxScreen,
        ImU32 borderColor,
        float baseWidth,
        float canvasZoom,
        float nodeScale,
        float cornerRadius) const
    {
        RenderSelectionGlow(minScreen, maxScreen, canvasZoom, nodeScale, cornerRadius);
        RenderSelectionBorder(minScreen, maxScreen, borderColor, baseWidth, canvasZoom, cornerRadius);
    }

    // ===== Preset Styles =====

    void SelectionEffectRenderer::ApplyStyle_OlympeBlue()
    {
        m_glowColor = {0.0f, 0.8f, 1.0f};
        m_glowAlpha = 0.6f;
        m_baseGlowSize = 8.0f;
        m_borderWidthMultiplier = 3.0f;
    }

    void SelectionEffectRenderer::ApplyStyle_GoldAccent()
    {
        m_glowColor = {1.0f, 0.84f, 0.0f};  // Gold
        m_glowAlpha = 0.6f;
        m_baseGlowSize = 9.0f;
        m_borderWidthMultiplier = 3.5f;
    }

    void SelectionEffectRenderer::ApplyStyle_GreenEnergy()
    {
        m_glowColor = {0.0f, 1.0f, 0.5f};   // Bright green
        m_glowAlpha = 0.65f;
        m_baseGlowSize = 8.5f;
        m_borderWidthMultiplier = 3.0f;
    }

    void SelectionEffectRenderer::ApplyStyle_PurpleMystery()
    {
        m_glowColor = {0.8f, 0.2f, 1.0f};   // Purple/Magenta
        m_glowAlpha = 0.6f;
        m_baseGlowSize = 8.5f;
        m_borderWidthMultiplier = 3.0f;
    }

    void SelectionEffectRenderer::ApplyStyle_RedAlert()
    {
        m_glowColor = {1.0f, 0.2f, 0.2f};   // Red
        m_glowAlpha = 0.65f;
        m_baseGlowSize = 9.0f;
        m_borderWidthMultiplier = 3.5f;
    }

} // namespace Olympe
