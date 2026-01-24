/*
 * Olympe Engine V2 - 2025
 * Isometric Renderer
 * 
 * Renders isometric tilemaps with proper depth sorting, tile flipping, and culling.
 */

#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include "../vector.h"

namespace Olympe {
namespace Rendering {

    // Tile flip flags (from Tiled format)
    constexpr uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    constexpr uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
    constexpr uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
    constexpr uint32_t TILE_ID_MASK = 0x1FFFFFFF;

    // Tile rendering data
    struct IsometricTile
    {
        int worldX;           // Grid X coordinate
        int worldY;           // Grid Y coordinate
        uint32_t tileGID;     // Tile Global ID (with flip flags)
        SDL_Texture* texture; // Texture to render
        SDL_Rect srcRect;     // Source rectangle in texture
        
        // Tile offset from tileset (for proper alignment)
        int tileoffsetX;
        int tileoffsetY;
        
        IsometricTile()
            : worldX(0), worldY(0), tileGID(0), texture(nullptr), 
              srcRect{0, 0, 0, 0}, tileoffsetX(0), tileoffsetY(0) {}
    };

    // Isometric Renderer class
    class IsometricRenderer
    {
    public:
        IsometricRenderer();
        ~IsometricRenderer();

        // Setup
        void Initialize(SDL_Renderer* renderer, int tileWidth, int tileHeight);
        
        // Camera control
        void SetCamera(float camX, float camY, float zoom);
        void SetViewport(int screenWidth, int screenHeight);
        
        // Rendering
        void BeginFrame();
        void RenderTile(const IsometricTile& tile);
        void EndFrame();
        
        // Utility functions
        Vector WorldToScreen(float worldX, float worldY) const;
        Vector ScreenToWorld(float screenX, float screenY) const;
        
        // Culling
        bool IsTileVisible(int worldX, int worldY) const;
        void GetVisibleTileRange(int& minX, int& minY, int& maxX, int& maxY) const;

    private:
        SDL_Renderer* m_renderer;
        
        // Tile dimensions
        int m_tileWidth;
        int m_tileHeight;
        
        // Camera state
        float m_cameraX;
        float m_cameraY;
        float m_zoom;
        
        // Viewport
        int m_screenWidth;
        int m_screenHeight;
        
        // Tile batch for depth sorting
        std::vector<IsometricTile> m_tileBatch;
        
        // Rendering constants
        static constexpr float ISOMETRIC_OFFSET_Y = 200.0f; // Y offset for negative world coords
        static constexpr float CULL_MARGIN = 100.0f;        // Safety margin for tall tiles
        
        // Helper functions
        void RenderTileImmediate(const IsometricTile& tile);
        void ExtractFlipFlags(uint32_t gid, bool& flipH, bool& flipV, bool& flipD) const;
        SDL_FlipMode GetSDLFlip(bool flipH, bool flipV, bool flipD) const;
        float CalculateCullingMargin() const;
    };

} // namespace Rendering
} // namespace Olympe
