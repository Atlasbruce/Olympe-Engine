/*
 * Olympe Engine V2 - 2025
 * Isometric Renderer
 * 
 * ✅ REFACTORED: Now provides utility functions only
 * Batching and sorting moved to unified rendering pipeline in ECS_Systems.cpp
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

    // ✅ DEPRECATED: Legacy tile structure - kept for compatibility
    // New unified pipeline uses RenderItem in ECS_Systems.cpp
    struct IsometricTile
    {
        int worldX;           // Grid X coordinate
        int worldY;           // Grid Y coordinate
        uint32_t tileGID;     // Tile Global ID (with flip flags)
        SDL_Texture* texture; // Texture to render
        SDL_Rect srcRect;     // Source rectangle in texture
        int zOrder;           // Layer z-order for sorting
        int tileoffsetX;
        int tileoffsetY;
        
        // Entity sprite support (DEPRECATED - now in unified pipeline)
        bool isEntitySprite;
        float worldPosX;
        float worldPosY;
        SDL_FRect destRect;
        uint32_t colorTint;
        SDL_FPoint hotSpot;
        
        IsometricTile()
            : worldX(0), worldY(0), tileGID(0), texture(nullptr), 
              srcRect{0, 0, 0, 0}, zOrder(0), tileoffsetX(0), tileoffsetY(0),
              isEntitySprite(false), worldPosX(0.0f), worldPosY(0.0f),
              destRect{0.0f, 0.0f, 0.0f, 0.0f}, colorTint(0xFFFFFFFF),
              hotSpot{0.0f, 0.0f} {}
    };

    // ✅ REFACTORED: Isometric utility functions (coordinate conversion, culling)
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
        
        // ✅ Utility functions (static-like, can be used without instance)
        Vector WorldToScreen(float worldX, float worldY) const;
        Vector ScreenToWorld(float screenX, float screenY) const;
        
        // Culling utilities
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
        
        // Rendering constants
        static constexpr float ISOMETRIC_OFFSET_Y = 200.0f;
        static constexpr float CULL_MARGIN = 100.0f;
        static constexpr float TALL_TILE_MULTIPLIER = 5.0f;
        static constexpr int VISIBLE_TILE_PADDING = 5;
        
        // Helper functions
        float CalculateCullingMargin() const;
    };

} // namespace Rendering
} // namespace Olympe
