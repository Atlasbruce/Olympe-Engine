/*
 * ParallaxLayerManager.cpp - Parallax layer management
 */

#include "../include/ParallaxLayerManager.h"
#include "../../ECS_Systems.h"
#include "../../GameEngine.h"
#include <SDL3/SDL.h>
#include <algorithm>

namespace Olympe {
namespace Tiled {

    ParallaxLayerManager& ParallaxLayerManager::Get()
    {
        static ParallaxLayerManager instance;
        return instance;
    }

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
        float& outX, float& outY) const
    {
        // Calculate position based on parallax scroll factors
        // A scroll factor of 0.0 means the layer doesn't move (distant background)
        // A scroll factor of 1.0 means the layer moves with the camera (normal)
        // A scroll factor > 1.0 means the layer moves faster (foreground)
        
        outX = layer.offsetX - (cameraX * layer.scrollFactorX);
        outY = layer.offsetY - (cameraY * layer.scrollFactorY);
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

        // Calculate parallax offset
        float scrollX, scrollY;
        CalculateRenderPosition(layer, cam.worldPosition.x, cam.worldPosition.y, scrollX, scrollY);

        int texW, texH;
        SDL_GetTextureSize(layer.texture, &texW, &texH);

        SDL_SetTextureAlphaMod(layer.texture, static_cast<Uint8>(layer.opacity * 255));

        int screenW = (int)cam.viewport.w;
        int screenH = (int)cam.viewport.h;

        if (layer.repeatX || layer.repeatY)
        {
            // Tiled rendering
            int startTileX = layer.repeatX ? ((int)scrollX / texW) - 1 : 0;
            int endTileX = layer.repeatX ? (((int)scrollX + screenW) / texW) + 1 : 0;
            int startTileY = layer.repeatY ? ((int)scrollY / texH) - 1 : 0;
            int endTileY = layer.repeatY ? (((int)scrollY + screenH) / texH) + 1 : 0;

            for (int tileY = startTileY; tileY <= endTileY; ++tileY)
            {
                for (int tileX = startTileX; tileX <= endTileX; ++tileX)
                {
                    SDL_FRect destRect = {
                        scrollX + tileX * texW,
                        scrollY + tileY * texH,
                        (float)texW,
                        (float)texH
                    };
                    SDL_RenderTexture(renderer, layer.texture, nullptr, &destRect);
                }
            }
        }
        else
        {
            // Single image
            SDL_FRect destRect = {scrollX, scrollY, (float)texW, (float)texH};
            SDL_RenderTexture(renderer, layer.texture, nullptr, &destRect);
        }
    }

    void ParallaxLayerManager::RenderAllLayers(const CameraTransform& cam) const
    {
        // Create a sorted copy of layers by z-order
        std::vector<const ParallaxLayer*> sortedLayers;
        sortedLayers.reserve(layers_.size());
        
        for (const auto& layer : layers_)
        {
            sortedLayers.push_back(&layer);
        }

        // Sort by z-order (back to front)
        std::sort(sortedLayers.begin(), sortedLayers.end(),
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
