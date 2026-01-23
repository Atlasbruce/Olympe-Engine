/*
 * Olympe Engine V2 - 2025
 * Isometric Renderer Implementation
 */

#include "IsometricRenderer.h"
#include "../system/system_utils.h"
#include <algorithm>
#include <cmath>

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

    void IsometricRenderer::BeginFrame()
    {
        m_tileBatch.clear();
    }

    void IsometricRenderer::RenderTile(const IsometricTile& tile)
    {
        // Add tile to batch for depth sorting
        m_tileBatch.push_back(tile);
    }

    void IsometricRenderer::EndFrame()
    {
        // Sort tiles back-to-front (painter's algorithm)
        // In isometric view, tiles with lower (worldX + worldY) are rendered first
        std::sort(m_tileBatch.begin(), m_tileBatch.end(),
            [](const IsometricTile& a, const IsometricTile& b) {
                int sumA = a.worldX + a.worldY;
                int sumB = b.worldX + b.worldY;
                if (sumA != sumB) return sumA < sumB;
                // If on same diagonal, sort by X coordinate
                return a.worldX < b.worldX;
            });
        
        // Render all tiles
        for (const auto& tile : m_tileBatch)
        {
            if (!tile.texture) continue;
            RenderTileImmediate(tile);
        }
        
        m_tileBatch.clear();
    }

    void IsometricRenderer::RenderTileImmediate(const IsometricTile& tile)
    {
        if (!m_renderer || !tile.texture) return;
        
        // Extract tile ID and flip flags
        uint32_t tileId = tile.tileGID & TILE_ID_MASK;
        if (tileId == 0) return; // Empty tile
        
        bool flipH, flipV, flipD;
        ExtractFlipFlags(tile.tileGID, flipH, flipV, flipD);
        
        // Convert world coordinates to screen position (already includes camera and viewport transforms)
        Vector screenPos = WorldToScreen(static_cast<float>(tile.worldX), 
                                         static_cast<float>(tile.worldY));
        
        // Calculate destination rectangle
        SDL_FRect destRect;
        destRect.w = static_cast<float>(tile.srcRect.w) * m_zoom;
        destRect.h = static_cast<float>(tile.srcRect.h) * m_zoom;
        
        // Adjust for tile anchor point (typically bottom-center for isometric tiles)
        destRect.x = screenPos.x - destRect.w / 2.0f;
        destRect.y = screenPos.y - destRect.h;
        
        // Get SDL flip flags
        SDL_FlipMode flip = GetSDLFlip(flipH, flipV, flipD);
        
		// Source rectangle conversion
		SDL_FRect srcFRect = {(float)tile.srcRect.x, (float)tile.srcRect.y, (float)tile.srcRect.w, (float)tile.srcRect.h};

        // Render the tile
        SDL_RenderTextureRotated(m_renderer, tile.texture, &srcFRect, &destRect, 
                                  0.0, nullptr, flip);
    }

    Vector IsometricRenderer::WorldToScreen(float worldX, float worldY) const
    {
        // Isometric projection: world grid coordinates to screen coordinates
        // screenX = (worldX - worldY) * (tileWidth / 2)
        // screenY = (worldX + worldY) * (tileHeight / 2)
        
        float isoX = (worldX - worldY) * (m_tileWidth / 2.0f);
        float isoY = (worldX + worldY) * (m_tileHeight / 2.0f);
        
        // Apply camera transform and center in viewport
        float screenX = (isoX - m_cameraX) * m_zoom + m_screenWidth / 2.0f;
        float screenY = (isoY - m_cameraY) * m_zoom + m_screenHeight / 2.0f;
        
        // Add isometric Y offset to ensure tiles with negative world coordinates are visible
        // This is necessary for infinite maps that start at negative world coordinates
        screenY += ISOMETRIC_OFFSET_Y;
        
        Vector screen;
        screen.x = screenX;
        screen.y = screenY;
        screen.z = 0.0f;
        return screen;
    }

    Vector IsometricRenderer::ScreenToWorld(float screenX, float screenY) const
    {
        // Inverse isometric projection: screen coordinates to world grid coordinates
        // First, undo the isometric offset applied in WorldToScreen
        screenY -= ISOMETRIC_OFFSET_Y;
        
        // Then undo viewport centering and camera transform
        float isoX = (screenX - m_screenWidth / 2.0f) / m_zoom + m_cameraX;
        float isoY = (screenY - m_screenHeight / 2.0f) / m_zoom + m_cameraY;
        
        // Finally, inverse isometric projection
        // worldX = (isoX / (tileWidth/2) + isoY / (tileHeight/2)) / 2
        // worldY = (isoY / (tileHeight/2) - isoX / (tileWidth/2)) / 2
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
        // Convert tile position to screen coordinates (already includes camera and viewport transforms)
        Vector screenPos = WorldToScreen(static_cast<float>(worldX), 
                                         static_cast<float>(worldY));
        
        // Check if tile is within screen bounds (with padding for tile size and safety margin)
        float totalMargin = CalculateCullingMargin();
        
        bool visible = (screenPos.x >= -totalMargin && screenPos.x <= m_screenWidth + totalMargin &&
                        screenPos.y >= -totalMargin && screenPos.y <= m_screenHeight + totalMargin);
        
        if (!visible)
        {
            // Only log first 5 culled tiles to avoid spam
            // Note: Static counter is used for debug purposes only - not thread-safe
            static int culledCount = 0;
            if (culledCount < 5)
            {
                SYSTEM_LOG << "[ISO CULL] Tile culled: world(" << worldX << "," << worldY 
                           << ") screen(" << screenPos.x << "," << screenPos.y << ")\n";
                culledCount++;
            }
        }
        
        return visible;
    }

    void IsometricRenderer::GetVisibleTileRange(int& minX, int& minY, int& maxX, int& maxY) const
    {
        // Calculate visible tile range based on camera position and screen size
        // This is an approximation; exact calculation would require diamond-shaped culling
        
        // Get screen corners in world coordinates (using absolute screen coordinates)
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
        int padding = 2;
        
        minX = static_cast<int>(std::floor(worldMinX)) - padding;
        minY = static_cast<int>(std::floor(worldMinY)) - padding;
        maxX = static_cast<int>(std::ceil(worldMaxX)) + padding;
        maxY = static_cast<int>(std::ceil(worldMaxY)) + padding;
    }

    void IsometricRenderer::ExtractFlipFlags(uint32_t gid, bool& flipH, bool& flipV, bool& flipD) const
    {
        flipH = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
        flipV = (gid & FLIPPED_VERTICALLY_FLAG) != 0;
        flipD = (gid & FLIPPED_DIAGONALLY_FLAG) != 0;
    }

    SDL_FlipMode IsometricRenderer::GetSDLFlip(bool flipH, bool flipV, bool flipD) const
    {
        // SDL3 only supports horizontal and vertical flips, not diagonal
        // Diagonal flip requires rotation, which we don't handle here for simplicity
        // TODO: Implement diagonal flip using SDL_RenderTextureRotated with angle parameter
        
        int flip = SDL_FLIP_NONE;
        if (flipH) flip |= SDL_FLIP_HORIZONTAL;
        if (flipV) flip |= SDL_FLIP_VERTICAL;
        
        return static_cast<SDL_FlipMode>(flip);
    }

    float IsometricRenderer::CalculateCullingMargin() const
    {
        // Calculate total culling margin: tile size (with zoom) + safety margin
        float padding = std::max(m_tileWidth, m_tileHeight) * m_zoom;
        return padding + CULL_MARGIN;
    }

} // namespace Rendering
} // namespace Olympe
