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
        
        // ✅ FIX: Calculate world position of layer (before camera transform)
        float worldX = layer.offsetX - (cam.worldPosition.x * layer.scrollFactorX);
        float worldY = layer.offsetY - (cam.worldPosition.y * layer.scrollFactorY);
        Vector worldPos(worldX, worldY, 0.0f);
        
        // ✅ FIX: Use WorldToScreen() for rotation + zoom
        Vector screenPos = cam.WorldToScreen(worldPos);
        
        // Apply zoom to dimensions
        texW *= cam.zoom;
        texH *= cam.zoom;

        SDL_SetTextureAlphaMod(layer.texture, static_cast<Uint8>(layer.opacity * 255));

        int screenW = (int)cam.viewport.w;
        int screenH = (int)cam.viewport.h;

        if (layer.repeatX || layer.repeatY)
        {
            // Tiled rendering
            float tileOffsetX = layer.repeatX ? fmod(screenPos.x, texW) : screenPos.x;
            float tileOffsetY = layer.repeatY ? fmod(screenPos.y, texH) : screenPos.y;
            
            // Adjust for negative values
            if (tileOffsetX > 0) tileOffsetX -= texW;
            if (tileOffsetY > 0) tileOffsetY -= texH;
            
            // Calculate number of tiles needed (using zoomed dimensions)
            int tilesX = layer.repeatX ? (int)ceil(screenW / texW) + 2 : 1;
            int tilesY = layer.repeatY ? (int)ceil(screenH / texH) + 2 : 1;
            
            // Render tiled pattern
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
                    // ✅ FIX: Apply SDL rotation for texture
                    SDL_FPoint center = {texW / 2.0f, texH / 2.0f};
                    SDL_RenderTextureRotated(renderer, layer.texture, nullptr, &destRect,
                                            cam.rotation, &center, SDL_FLIP_NONE);
                }
            }
        }
        else
        {
            // Single image
            SDL_FRect destRect = {screenPos.x, screenPos.y, texW, texH};
            SDL_FPoint center = {texW / 2.0f, texH / 2.0f};
            SDL_RenderTextureRotated(renderer, layer.texture, nullptr, &destRect,
                                    cam.rotation, &center, SDL_FLIP_NONE);
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
