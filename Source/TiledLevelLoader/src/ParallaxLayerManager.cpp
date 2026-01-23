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

        float texW, texH;
        SDL_GetTextureSize(layer.texture, &texW, &texH);

        SDL_SetTextureAlphaMod(layer.texture, static_cast<Uint8>(layer.opacity * 255));

        int screenW = (int)cam.viewport.w;
        int screenH = (int)cam.viewport.h;

        if (layer.repeatX || layer.repeatY)
        {
            // Tiled rendering with proper wrapping
            // Calculate wrapped offset for seamless tiling
            float tileOffsetX = layer.repeatX ? fmod(scrollX, texW) : scrollX;
            float tileOffsetY = layer.repeatY ? fmod(scrollY, texH) : scrollY;
            
            // Adjust for negative values (ensure offset is always negative or zero)
            if (tileOffsetX > 0) tileOffsetX -= texW;
            if (tileOffsetY > 0) tileOffsetY -= texH;
            
            // Calculate number of tiles needed to cover viewport
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
