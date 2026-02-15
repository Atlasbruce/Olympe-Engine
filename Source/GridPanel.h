/**
 * @file GridPanel.h
 * @brief ImGui panel for real-time grid system control
 * @author Olympe Engine Team
 * @date 2025
 * 
 * Purpose: Provides an ImGui overlay panel to control the ECS Grid system
 * (GridSettings_data component) in real-time. Supports orthogonal, isometric,
 * and hexagonal grid types with overlay visualization options.
 */

#pragma once

#include <string>

/**
 * @brief GridPanel - ImGui overlay for grid system control
 * 
 * This class provides a user-friendly interface to configure the ECS Grid system
 * in real-time. It allows toggling grid visibility, changing projection types,
 * adjusting cell sizes, colors, and enabling collision/navigation overlays.
 */
class GridPanel
{
public:
    GridPanel();
    ~GridPanel();

    /**
     * @brief Initialize the panel
     */
    void Initialize();

    /**
     * @brief Render the ImGui panel
     * Should be called between ImGui::NewFrame() and ImGui::Render()
     */
    void Render();

    /**
     * @brief Toggle panel visibility
     */
    void Toggle();

    /**
     * @brief Set panel visibility
     * @param visible True to show, false to hide
     */
    void SetVisible(bool visible);

    /**
     * @brief Check if panel is visible
     * @return True if visible, false otherwise
     */
    bool IsVisible() const { return m_visible; }

private:
    bool m_visible;
    std::string m_name;
    
    // UI state
    int m_selectedProjection;    // 0=Ortho, 1=Iso, 2=Hex
    float m_cellSizeX;
    float m_cellSizeY;
    float m_hexRadius;
    float m_gridColorRGB[4];     // RGBA normalized [0,1]
    int m_maxLines;
    float m_lodZoomThreshold;
    int m_lodSkipFactor;
    
    // Overlay settings
    bool m_showCollisionOverlay;
    bool m_showNavigationOverlay;
    int m_activeCollisionLayer;
    int m_activeNavigationLayer;
    
    /**
     * @brief Load settings from ECS GridSettings_data component
     */
    void LoadSettingsFromECS();
    
    /**
     * @brief Apply current UI settings to ECS GridSettings_data component
     */
    void ApplySettingsToECS();
    
    /**
     * @brief Render projection type selection UI
     */
    void RenderProjectionSelector();
    
    /**
     * @brief Render cell/hex size configuration UI
     */
    void RenderSizeSettings();
    
    /**
     * @brief Render visual settings UI (color, LOD, etc.)
     */
    void RenderVisualSettings();
    
    /**
     * @brief Render overlay settings UI (collision, navigation)
     */
    void RenderOverlaySettings();
    
    /**
     * @brief Render quick action buttons
     */
    void RenderQuickActions();
};
