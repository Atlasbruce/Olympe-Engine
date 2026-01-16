/*
 * ParallaxLayerManager.cpp - Parallax layer management
 */

#include "ParallaxLayerManager.h"

namespace Olympe {
namespace Tiled {

    ParallaxLayerManager::ParallaxLayerManager()
    {
    }

    ParallaxLayerManager::~ParallaxLayerManager()
    {
    }

    void ParallaxLayerManager::AddLayer(const ParallaxLayer& layer)
    {
        layers_.push_back(layer);
    }

    void ParallaxLayerManager::Clear()
    {
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
        
        outX = layer.offsetX + (cameraX * layer.scrollFactorX);
        outY = layer.offsetY + (cameraY * layer.scrollFactorY);
    }

    const ParallaxLayer* ParallaxLayerManager::GetLayer(size_t index) const
    {
        if (index >= layers_.size()) {
            return nullptr;
        }
        return &layers_[index];
    }

} // namespace Tiled
} // namespace Olympe
