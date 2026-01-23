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
        // Debug logging before sorting and rendering
        SYSTEM_LOG << "[ISO RENDERER] ======================================\n";
        SYSTEM_LOG << "[ISO RENDERER] EndFrame() called\n";
        SYSTEM_LOG << "[ISO RENDERER]   Batch size before culling: " << m_tileBatch.size() << " tiles\n";
        SYSTEM_LOG << "[ISO RENDERER]   Camera: (" << m_cameraX << ", " << m_cameraY << ") zoom=" << m_zoom << "\n";
        SYSTEM_LOG << "[ISO RENDERER]   Viewport: " << m_screenWidth << "x" << m_screenHeight << "\n";
        
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
        
        SYSTEM_LOG << "[ISO RENDERER]   Sorted batch size: " << m_tileBatch.size() << "\n";
        
        // Log first tile
        if (!m_tileBatch.empty())
        {
            const auto& firstTile = m_tileBatch.front();
            Vector screenPos = WorldToScreen((float)firstTile.worldX, (float)firstTile.worldY);
            SYSTEM_LOG << "[ISO RENDERER]   First tile: world(" << firstTile.worldX << "," << firstTile.worldY 
                       << ") -> screen(" << screenPos.x << "," << screenPos.y << ")\n";
            SYSTEM_LOG << "[ISO RENDERER]      texture=" << (firstTile.texture ? "VALID" : "NULL") 
                       << " srcRect=(" << firstTile.srcRect.x << "," << firstTile.srcRect.y 
                       << " " << firstTile.srcRect.w << "x" << firstTile.srcRect.h << ")\n";
        }
        
        // Render all tiles and track rendered count
        int renderedCount = 0;
        for (const auto& tile : m_tileBatch)
        {
            if (tile.texture != nullptr)
            {
                renderedCount++;
                // Only log first 3 rendered tiles
                if (renderedCount <= 3)
                {
                    Vector screenPos = WorldToScreen((float)tile.worldX, (float)tile.worldY);
                    SYSTEM_LOG << "[ISO RENDERER]   Rendering tile #" << renderedCount 
                               << ": world(" << tile.worldX << "," << tile.worldY 
                               << ") screen(" << screenPos.x << "," << screenPos.y << ")\n";
                }
            }
            RenderTileImmediate(tile);
        }
        
        SYSTEM_LOG << "[ISO RENDERER]   Total tiles rendered: " << renderedCount << "\n";
        SYSTEM_LOG << "[ISO RENDERER] ======================================\n";
        
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
        
        // Convert world coordinates to screen position
        Vector screenPos = WorldToScreen(static_cast<float>(tile.worldX), 
                                         static_cast<float>(tile.worldY));
        
        // Apply camera offset and zoom
        float screenX = (screenPos.x - m_cameraX) * m_zoom + m_screenWidth / 2.0f;
        float screenY = (screenPos.y - m_cameraY) * m_zoom + m_screenHeight / 2.0f;
        
        // Calculate destination rectangle
        SDL_FRect destRect;
        destRect.x = screenX;
        destRect.y = screenY;
        destRect.w = static_cast<float>(tile.srcRect.w) * m_zoom;
        destRect.h = static_cast<float>(tile.srcRect.h) * m_zoom;
        
        // Adjust for tile anchor point (typically bottom-center for isometric tiles)
        destRect.x -= destRect.w / 2.0f;
        destRect.y -= destRect.h;
        
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
        
        Vector screen;
        screen.x = (worldX - worldY) * (m_tileWidth / 2.0f);
        screen.y = (worldX + worldY) * (m_tileHeight / 2.0f);
        screen.z = 0.0f;
        return screen;
    }

    Vector IsometricRenderer::ScreenToWorld(float screenX, float screenY) const
    {
        // Inverse isometric projection: screen coordinates to world grid coordinates
        // worldX = (screenX / (tileWidth/2) + screenY / (tileHeight/2)) / 2
        // worldY = (screenY / (tileHeight/2) - screenX / (tileWidth/2)) / 2
        
        Vector world;
        float halfTileW = m_tileWidth / 2.0f;
        float halfTileH = m_tileHeight / 2.0f;
        
        world.x = (screenX / halfTileW + screenY / halfTileH) / 2.0f;
        world.y = (screenY / halfTileH - screenX / halfTileW) / 2.0f;
        world.z = 0.0f;
        return world;
    }

    bool IsometricRenderer::IsTileVisible(int worldX, int worldY) const
    {
        // Convert tile position to screen coordinates
        Vector screenPos = WorldToScreen(static_cast<float>(worldX), 
                                         static_cast<float>(worldY));
        
        // Apply camera offset and zoom
        float screenX = (screenPos.x - m_cameraX) * m_zoom + m_screenWidth / 2.0f;
        float screenY = (screenPos.y - m_cameraY) * m_zoom + m_screenHeight / 2.0f;
        
        // Check if tile is within screen bounds (with padding for tile size)
        float padding = std::max(m_tileWidth, m_tileHeight) * m_zoom;
        
        bool visible = (screenX >= -padding && screenX <= m_screenWidth + padding &&
                        screenY >= -padding && screenY <= m_screenHeight + padding);
        
        if (!visible)
        {
            // Only log first 5 culled tiles to avoid spam
            // Note: Static counter is used for debug purposes only - not thread-safe
            static int culledCount = 0;
            if (culledCount < 5)
            {
                SYSTEM_LOG << "[ISO CULL] Tile culled: world(" << worldX << "," << worldY 
                           << ") screen(" << screenX << "," << screenY << ")\n";
                culledCount++;
            }
        }
        
        return visible;
    }

    void IsometricRenderer::GetVisibleTileRange(int& minX, int& minY, int& maxX, int& maxY) const
    {
        // Calculate visible tile range based on camera position and screen size
        // This is an approximation; exact calculation would require diamond-shaped culling
        
        // Get screen corners in world coordinates
        Vector topLeft = ScreenToWorld(-m_screenWidth / 2.0f, -m_screenHeight / 2.0f);
        Vector topRight = ScreenToWorld(m_screenWidth / 2.0f, -m_screenHeight / 2.0f);
        Vector bottomLeft = ScreenToWorld(-m_screenWidth / 2.0f, m_screenHeight / 2.0f);
        Vector bottomRight = ScreenToWorld(m_screenWidth / 2.0f, m_screenHeight / 2.0f);
        
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

} // namespace Rendering
} // namespace Olympe
