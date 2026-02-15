/**
 * @file GridPanel.cpp
 * @brief ImGui panel for real-time grid system control - Implementation
 * @author Olympe Engine Team
 * @date 2025
 */

#include "GridPanel.h"
#include "ECS_Components.h"
#include "World.h"
#include "system/system_utils.h"

#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
#include "third_party/imgui/imgui.h"
#endif

GridPanel::GridPanel()
    : m_visible(false)
    , m_name("Grid Settings")
    , m_selectedProjection(0)
    , m_cellSizeX(32.0f)
    , m_cellSizeY(32.0f)
    , m_hexRadius(16.0f)
    , m_maxLines(1200)
    , m_lodZoomThreshold(0.5f)
    , m_lodSkipFactor(10)
    , m_showCollisionOverlay(false)
    , m_showNavigationOverlay(false)
    , m_activeCollisionLayer(0)
    , m_activeNavigationLayer(0)
{
    // Initialize color to light gray
    m_gridColorRGB[0] = 180.0f / 255.0f;
    m_gridColorRGB[1] = 180.0f / 255.0f;
    m_gridColorRGB[2] = 180.0f / 255.0f;
    m_gridColorRGB[3] = 1.0f;
}

GridPanel::~GridPanel()
{
}

void GridPanel::Initialize()
{
    SYSTEM_LOG << "GridPanel: Initialized\n";
    LoadSettingsFromECS();
}

void GridPanel::Toggle()
{
    SetVisible(!m_visible);
}

void GridPanel::SetVisible(bool visible)
{
    if (m_visible == visible) return;
    m_visible = visible;
    
    if (m_visible)
    {
        // Reload settings when panel is opened
        LoadSettingsFromECS();
        SYSTEM_LOG << "GridPanel: Opened\n";
    }
    else
    {
        SYSTEM_LOG << "GridPanel: Closed\n";
    }
}

void GridPanel::Render()
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    if (!m_visible) return;
    
    ImGui::SetNextWindowSize(ImVec2(420, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Grid Settings (F4)", &m_visible, ImGuiWindowFlags_None))
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ECS Grid System Configuration");
        ImGui::Separator();
        
        // Quick enable/disable toggle
        bool enabled = true;
        try {
            // Find GridSettings entity
            for (EntityID e : World::Get().GetEntities())
            {
                if (World::Get().HasComponent<GridSettings_data>(e))
                {
                    GridSettings_data& settings = World::Get().GetComponent<GridSettings_data>(e);
                    enabled = settings.enabled;
                    break;
                }
            }
        }
        catch (const std::exception& ex)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", ex.what());
        }
        
        if (ImGui::Checkbox("Grid Enabled", &enabled))
        {
            ApplySettingsToECS();
            // Update enabled state
            try {
                for (EntityID e : World::Get().GetEntities())
                {
                    if (World::Get().HasComponent<GridSettings_data>(e))
                    {
                        World::Get().GetComponent<GridSettings_data>(e).enabled = enabled;
                        break;
                    }
                }
            }
            catch (const std::exception&) {}
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Toggle with TAB)");
        
        ImGui::Spacing();
        
        // Projection type selector
        RenderProjectionSelector();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Size settings
        RenderSizeSettings();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Visual settings
        RenderVisualSettings();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Overlay settings
        RenderOverlaySettings();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Quick actions
        RenderQuickActions();
    }
    ImGui::End();
#endif
}

void GridPanel::RenderProjectionSelector()
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    ImGui::Text("Grid Projection Type:");
    
    const char* projectionTypes[] = { "Orthogonal", "Isometric", "Hexagonal (Axial)" };
    if (ImGui::Combo("##Projection", &m_selectedProjection, projectionTypes, 3))
    {
        ApplySettingsToECS();
    }
    
    // Help text
    ImGui::TextDisabled("Ortho: Standard square grid");
    ImGui::TextDisabled("Iso: Diamond-shaped isometric grid");
    ImGui::TextDisabled("Hex: Hexagonal axial grid (pointy-top)");
#endif
}

void GridPanel::RenderSizeSettings()
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    ImGui::Text("Grid Dimensions:");
    
    if (m_selectedProjection == 0 || m_selectedProjection == 1) // Ortho or Iso
    {
        ImGui::Text("Cell Size (World Units):");
        if (ImGui::DragFloat("Width##CellX", &m_cellSizeX, 1.0f, 8.0f, 256.0f, "%.1f"))
        {
            ApplySettingsToECS();
        }
        if (ImGui::DragFloat("Height##CellY", &m_cellSizeY, 1.0f, 8.0f, 256.0f, "%.1f"))
        {
            ApplySettingsToECS();
        }
    }
    else if (m_selectedProjection == 2) // Hexagonal
    {
        ImGui::Text("Hexagon Radius (World Units):");
        if (ImGui::DragFloat("##HexRadius", &m_hexRadius, 0.5f, 4.0f, 128.0f, "%.1f"))
        {
            ApplySettingsToECS();
        }
    }
#endif
}

void GridPanel::RenderVisualSettings()
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    ImGui::Text("Visual Settings:");
    
    // Grid color
    if (ImGui::ColorEdit4("Grid Color", m_gridColorRGB, ImGuiColorEditFlags_None))
    {
        ApplySettingsToECS();
    }
    
    // Max lines budget
    if (ImGui::DragInt("Max Lines", &m_maxLines, 10.0f, 100, 5000))
    {
        ApplySettingsToECS();
    }
    ImGui::TextDisabled("Performance budget for line rendering");
    
    // LOD settings
    ImGui::Spacing();
    ImGui::Text("Level of Detail (LOD):");
    if (ImGui::DragFloat("LOD Zoom Threshold", &m_lodZoomThreshold, 0.01f, 0.1f, 2.0f, "%.2f"))
    {
        ApplySettingsToECS();
    }
    ImGui::TextDisabled("Apply LOD when zoom is below this value");
    
    if (ImGui::DragInt("LOD Skip Factor", &m_lodSkipFactor, 1.0f, 1, 50))
    {
        ApplySettingsToECS();
    }
    ImGui::TextDisabled("Draw 1 line every N when LOD active");
#endif
}

void GridPanel::RenderOverlaySettings()
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    ImGui::Text("Overlay Visualization:");
    
    // Collision overlay
    if (ImGui::Checkbox("Show Collision Overlay", &m_showCollisionOverlay))
    {
        ApplySettingsToECS();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Toggle with C)");
    
    if (m_showCollisionOverlay)
    {
        ImGui::Indent();
        if (ImGui::SliderInt("Collision Layer", &m_activeCollisionLayer, 0, 7))
        {
            ApplySettingsToECS();
        }
        ImGui::TextDisabled("Layer 0: Ground, 1: Sky, 2: Underground, 3: Volume");
        ImGui::Unindent();
    }
    
    ImGui::Spacing();
    
    // Navigation overlay
    if (ImGui::Checkbox("Show Navigation Overlay", &m_showNavigationOverlay))
    {
        ApplySettingsToECS();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Toggle with N)");
    
    if (m_showNavigationOverlay)
    {
        ImGui::Indent();
        if (ImGui::SliderInt("Navigation Layer", &m_activeNavigationLayer, 0, 7))
        {
            ApplySettingsToECS();
        }
        ImGui::TextDisabled("Layer 0: Ground, 1: Sky, 2: Underground, 3: Volume");
        ImGui::Unindent();
    }
#endif
}

void GridPanel::RenderQuickActions()
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    ImGui::Text("Quick Actions:");
    
    if (ImGui::Button("Reset to Defaults", ImVec2(150, 0)))
    {
        m_selectedProjection = 0;
        m_cellSizeX = 32.0f;
        m_cellSizeY = 32.0f;
        m_hexRadius = 16.0f;
        m_gridColorRGB[0] = 180.0f / 255.0f;
        m_gridColorRGB[1] = 180.0f / 255.0f;
        m_gridColorRGB[2] = 180.0f / 255.0f;
        m_gridColorRGB[3] = 1.0f;
        m_maxLines = 1200;
        m_lodZoomThreshold = 0.5f;
        m_lodSkipFactor = 10;
        m_showCollisionOverlay = false;
        m_showNavigationOverlay = false;
        m_activeCollisionLayer = 0;
        m_activeNavigationLayer = 0;
        
        ApplySettingsToECS();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Reload from ECS", ImVec2(150, 0)))
    {
        LoadSettingsFromECS();
    }
    
    ImGui::Spacing();
    ImGui::TextDisabled("Keyboard shortcuts:");
    ImGui::TextDisabled("  TAB: Toggle grid");
    ImGui::TextDisabled("  C: Toggle collision overlay");
    ImGui::TextDisabled("  N: Toggle navigation overlay");
    ImGui::TextDisabled("  F4: Toggle this panel");
#endif
}

void GridPanel::LoadSettingsFromECS()
{
    try
    {
        // Find GridSettings entity
        for (EntityID e : World::Get().GetEntities())
        {
            if (World::Get().HasComponent<GridSettings_data>(e))
            {
                const GridSettings_data& settings = World::Get().GetComponent<GridSettings_data>(e);
                
                // Load projection type
                m_selectedProjection = static_cast<int>(settings.projection);
                
                // Load cell sizes
                m_cellSizeX = settings.cellSize.x;
                m_cellSizeY = settings.cellSize.y;
                m_hexRadius = settings.hexRadius;
                
                // Load visual settings
                m_gridColorRGB[0] = settings.color.r / 255.0f;
                m_gridColorRGB[1] = settings.color.g / 255.0f;
                m_gridColorRGB[2] = settings.color.b / 255.0f;
                m_gridColorRGB[3] = settings.color.a / 255.0f;
                m_maxLines = settings.maxLines;
                m_lodZoomThreshold = settings.lodZoomThreshold;
                m_lodSkipFactor = settings.lodSkipFactor;
                
                // Load overlay settings
                m_showCollisionOverlay = settings.showCollisionOverlay;
                m_showNavigationOverlay = settings.showNavigationOverlay;
                m_activeCollisionLayer = settings.activeCollisionLayer;
                m_activeNavigationLayer = settings.activeNavigationLayer;
                
                SYSTEM_LOG << "GridPanel: Settings loaded from ECS\n";
                return;
            }
        }
        
        SYSTEM_LOG << "GridPanel: No GridSettings_data component found\n";
    }
    catch (const std::exception& ex)
    {
        SYSTEM_LOG << "GridPanel: Error loading settings: " << ex.what() << "\n";
    }
}

void GridPanel::ApplySettingsToECS()
{
    try
    {
        // Find GridSettings entity
        for (EntityID e : World::Get().GetEntities())
        {
            if (World::Get().HasComponent<GridSettings_data>(e))
            {
                GridSettings_data& settings = World::Get().GetComponent<GridSettings_data>(e);
                
                // Apply projection type
                settings.projection = static_cast<GridProjection>(m_selectedProjection);
                
                // Apply cell sizes
                settings.cellSize.x = m_cellSizeX;
                settings.cellSize.y = m_cellSizeY;
                settings.hexRadius = m_hexRadius;
                
                // Apply visual settings
                settings.color.r = static_cast<Uint8>(m_gridColorRGB[0] * 255.0f);
                settings.color.g = static_cast<Uint8>(m_gridColorRGB[1] * 255.0f);
                settings.color.b = static_cast<Uint8>(m_gridColorRGB[2] * 255.0f);
                settings.color.a = static_cast<Uint8>(m_gridColorRGB[3] * 255.0f);
                settings.maxLines = m_maxLines;
                settings.lodZoomThreshold = m_lodZoomThreshold;
                settings.lodSkipFactor = m_lodSkipFactor;
                
                // Apply overlay settings
                settings.showCollisionOverlay = m_showCollisionOverlay;
                settings.showNavigationOverlay = m_showNavigationOverlay;
                settings.activeCollisionLayer = static_cast<uint8_t>(m_activeCollisionLayer);
                settings.activeNavigationLayer = static_cast<uint8_t>(m_activeNavigationLayer);
                
                return;
            }
        }
    }
    catch (const std::exception& ex)
    {
        SYSTEM_LOG << "GridPanel: Error applying settings: " << ex.what() << "\n";
    }
}
