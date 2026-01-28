/*
 * Olympe Engine V2 - 2025
 * Isometric Renderer Implementation
 * 
 * ✅ REFACTORED: Now provides utility functions only
 * Batching and sorting moved to unified rendering pipeline in ECS_Systems.cpp
 */

#include "IsometricRenderer.h"
#include "../system/system_utils.h"
#include <algorithm>
#include <cmath>
#include "../drawing.h"

namespace Olympe {
namespace Rendering {

    IsometricRenderer::IsometricRenderer()
        : m_renderer(nullptr)
        , m_tileWidth(64)
        , m_tileHeight(32)
        , m_cameraX(0.0f)
        , m_cameraY(0.0f)
        , m_zoom(1.0f)
        , m_screenWidth(800)
        , m_screenHeight(600)
    {
    }

    IsometricRenderer::~IsometricRenderer()
    {
    }

    void IsometricRenderer::Initialize(SDL_Renderer* renderer, int tileWidth, int tileHeight)
    {
        m_renderer = renderer;
        m_tileWidth = tileWidth;
        m_tileHeight = tileHeight;
        
        SYSTEM_LOG << "IsometricRenderer: Initialized with tile size " 
                   << tileWidth << "x" << tileHeight << "\n";
    }

    void IsometricRenderer::SetCamera(float camX, float camY, float zoom)
    {
        m_cameraX = camX;
        m_cameraY = camY;
        m_zoom = zoom;
    }

    void IsometricRenderer::SetViewport(int screenWidth, int screenHeight)
    {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
    }

    Vector IsometricRenderer::WorldToScreen(float worldX, float worldY) const
    {
        // Isometric projection: world grid coordinates to screen coordinates
        float isoX = (worldX - worldY) * (m_tileWidth / 2.0f);
        float isoY = (worldX + worldY) * (m_tileHeight / 2.0f);
        
        // Apply camera transform and center in viewport
        float screenX = (isoX - m_cameraX) * m_zoom + m_screenWidth / 2.0f;
        float screenY = (isoY - m_cameraY) * m_zoom + m_screenHeight / 2.0f;
        
        // Add isometric Y offset to ensure tiles with negative world coordinates are visible
        screenY += ISOMETRIC_OFFSET_Y;
        
        Vector screen;
        screen.x = screenX;
        screen.y = screenY;
        screen.z = 0.0f;
        return screen;
    }

    Vector IsometricRenderer::ScreenToWorld(float screenX, float screenY) const
    {
        // Inverse isometric projection
        screenY -= ISOMETRIC_OFFSET_Y;
        
        float isoX = (screenX - m_screenWidth / 2.0f) / m_zoom + m_cameraX;
        float isoY = (screenY - m_screenHeight / 2.0f) / m_zoom + m_cameraY;
        
        Vector world;
        float halfTileW = m_tileWidth / 2.0f;
        float halfTileH = m_tileHeight / 2.0f;
        
        world.x = (isoX / halfTileW + isoY / halfTileH) / 2.0f;
        world.y = (isoY / halfTileH - isoX / halfTileW) / 2.0f;
        world.z = 0.0f;
        return world;
    }

    bool IsometricRenderer::IsTileVisible(int worldX, int worldY) const
    {
        // Convert tile position to screen coordinates
        Vector screenPos = WorldToScreen(static_cast<float>(worldX), 
                                         static_cast<float>(worldY));
        
        // Check if tile is within screen bounds with margin
        float totalMargin = CalculateCullingMargin();
        
        bool visible = (screenPos.x >= -totalMargin && screenPos.x <= m_screenWidth + totalMargin &&
                        screenPos.y >= -totalMargin && screenPos.y <= m_screenHeight + totalMargin);
        
        return visible;
    }

    void IsometricRenderer::GetVisibleTileRange(int& minX, int& minY, int& maxX, int& maxY) const
    {
        // Get screen corners in world coordinates
        Vector topLeft = ScreenToWorld(0.0f, 0.0f);
        Vector topRight = ScreenToWorld(static_cast<float>(m_screenWidth), 0.0f);
        Vector bottomLeft = ScreenToWorld(0.0f, static_cast<float>(m_screenHeight));
        Vector bottomRight = ScreenToWorld(static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));
        
        // Find bounding box in world coordinates
        float worldMinX = std::min({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x});
        float worldMaxX = std::max({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x});
        float worldMinY = std::min({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y});
        float worldMaxY = std::max({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y});
        
        // Add padding for tile size
        minX = static_cast<int>(std::floor(worldMinX)) - VISIBLE_TILE_PADDING;
        minY = static_cast<int>(std::floor(worldMinY)) - VISIBLE_TILE_PADDING;
        maxX = static_cast<int>(std::ceil(worldMaxX)) + VISIBLE_TILE_PADDING;
        maxY = static_cast<int>(std::ceil(worldMaxY)) + VISIBLE_TILE_PADDING;
    }

    float IsometricRenderer::CalculateCullingMargin() const
    {
        // Calculate total culling margin for tall tiles
        float padding = std::max(m_tileWidth, m_tileHeight) * m_zoom * TALL_TILE_MULTIPLIER;
        return padding + CULL_MARGIN;
    }

} // namespace Rendering
} // namespace Olympe
