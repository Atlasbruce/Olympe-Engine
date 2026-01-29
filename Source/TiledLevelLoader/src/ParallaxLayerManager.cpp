/*
 * ParallaxLayerManager.cpp - Parallax layer management
 */

#include "../include/ParallaxLayerManager.h"
#include "../../ECS_Systems.h"
#include "../../GameEngine.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace Olympe {
namespace Tiled {


    ParallaxLayerManager::ParallaxLayerManager()
    {
    }

    ParallaxLayerManager::~ParallaxLayerManager()
    {
        // Clean up textures
        for (auto& layer : layers_)
        {
            if (layer.texture)
            {
                SDL_DestroyTexture(layer.texture);
                layer.texture = nullptr;
            }
        }
    }

    ParallaxLayerManager& ParallaxLayerManager::Get()
    {
        static ParallaxLayerManager instance;
        return instance;
    }

    void ParallaxLayerManager::AddLayer(const ParallaxLayer& layer)
    {
        layers_.push_back(layer);
    }

    void ParallaxLayerManager::Clear()
    {
        // Clean up textures
        for (auto& layer : layers_)
        {
            if (layer.texture)
            {
                SDL_DestroyTexture(layer.texture);
                layer.texture = nullptr;
            }
        }
        layers_.clear();
    }

    void ParallaxLayerManager::CalculateRenderPosition(
        const ParallaxLayer& layer,
        float cameraX, float cameraY,
        float zoom,
        float& outX, float& outY) const
    {
        // Calculate position based on parallax scroll factors
        // A scroll factor of 0.0 means the layer doesn't move (distant background)
        // A scroll factor of 1.0 means the layer moves with the camera (normal)
        // A scroll factor > 1.0 means the layer moves faster (foreground)
        
        // Calculate world position of the layer (applying parallax factor)
        float worldX = layer.offsetX - (cameraX * layer.scrollFactorX);
        float worldY = layer.offsetY - (cameraY * layer.scrollFactorY);
        
        // Apply zoom to the relative position (consistent with entity rendering)
        outX = worldX * zoom;
        outY = worldY * zoom;
    }

    const ParallaxLayer* ParallaxLayerManager::GetLayer(size_t index) const
    {
        if (index >= layers_.size()) {
            return nullptr;
        }
        return &layers_[index];
    }

    void ParallaxLayerManager::RenderLayer(const ParallaxLayer& layer, const CameraTransform& cam) const
    {
        if (!layer.visible || !layer.texture) return;

        SDL_Renderer* renderer = GameEngine::renderer;
        if (!renderer) return;

        float texW, texH;
        SDL_GetTextureSize(layer.texture, &texW, &texH);
        
        // Calculate parallax-adjusted world position
        float worldX = layer.offsetX - (cam.worldPosition.x * layer.scrollFactorX);
        float worldY = layer.offsetY - (cam.worldPosition.y * layer.scrollFactorY);
        
        // Calculate screen position with zoom only (no rotation)
        // Rotation is applied separately via SDL_RenderTextureRotated with viewport-centered pivot
        float screenX = worldX * cam.zoom - cam.screenOffset.x + cam.viewport.w / 2.0f;
        float screenY = worldY * cam.zoom - cam.screenOffset.y + cam.viewport.h / 2.0f;
        
        // Apply zoom to dimensions
        texW *= cam.zoom;
        texH *= cam.zoom;

        SDL_SetTextureAlphaMod(layer.texture, static_cast<Uint8>(layer.opacity * 255));

        int screenW = (int)cam.viewport.w;
        int screenH = (int)cam.viewport.h;

        if (layer.repeatX || layer.repeatY)
        {
            // Tiled rendering with zoom-aware calculations
            float tileOffsetX = layer.repeatX ? fmod(screenX, texW) : screenX;
            float tileOffsetY = layer.repeatY ? fmod(screenY, texH) : screenY;
            
            // Adjust for negative values
            if (tileOffsetX > 0) tileOffsetX -= texW;
            if (tileOffsetY > 0) tileOffsetY -= texH;
            
            // Calculate number of tiles needed (using zoomed dimensions)
            int tilesX = layer.repeatX ? (int)ceil(screenW / texW) + 2 : 1;
            int tilesY = layer.repeatY ? (int)ceil(screenH / texH) + 2 : 1;
            
            // Render each tile WITH rotation around viewport center
            for (int tileY = 0; tileY < tilesY; ++tileY)
            {
                for (int tileX = 0; tileX < tilesX; ++tileX)
                {
                    SDL_FRect destRect = {
                        tileOffsetX + tileX * texW,
                        tileOffsetY + tileY * texH,
                        texW,
                        texH
                    };
                    
                    // Pivot = viewport center in tile's coordinate space
                    // This makes the tile rotate around the camera center (not its own center)
                    SDL_FPoint pivotInTileSpace = {
                        cam.viewport.w / 2.0f - destRect.x,
                        cam.viewport.h / 2.0f - destRect.y
                    };
                    
                    SDL_RenderTextureRotated(renderer, layer.texture, nullptr, &destRect,
                                            cam.rotation, &pivotInTileSpace, SDL_FLIP_NONE);
                }
            }
        }
        else
        {
            // Single image with zoom
            SDL_FRect destRect = {screenX, screenY, texW, texH};
            
            // Rotate around viewport center
            SDL_FPoint pivotInTileSpace = {
                cam.viewport.w / 2.0f - destRect.x,
                cam.viewport.h / 2.0f - destRect.y
            };
            
            SDL_RenderTextureRotated(renderer, layer.texture, nullptr, &destRect,
                                    cam.rotation, &pivotInTileSpace, SDL_FLIP_NONE);
        }
    }

    void ParallaxLayerManager::RenderAllLayers(const CameraTransform& cam) const
    {
        // Create a sorted copy of layers by z-order
        vector<const ParallaxLayer*> sortedLayers;
        sortedLayers.reserve(layers_.size());
        
        for (const auto& layer : layers_)
        {
            sortedLayers.push_back(&layer);
        }

        // Sort by z-order (back to front)
        sort(sortedLayers.begin(), sortedLayers.end(),
            [](const ParallaxLayer* a, const ParallaxLayer* b) {
                return a->zOrder < b->zOrder;
            });

        // Render in sorted order
        for (const auto* layer : sortedLayers)
        {
            RenderLayer(*layer, cam);
        }
    }

} // namespace Tiled
} // namespace Olympe
