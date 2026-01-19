/*
 * ParallaxLayerManager.h - Parallax scrolling system for image layers
 * 
 * Manages parallax background/foreground layers with support for:
 * - Parallax scroll factors (parallaxx, parallaxy)
 * - Texture repeating (repeatx, repeaty)
 * - Layer offsets and opacity
 * - Camera-relative positioning
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations
struct SDL_Texture;
struct SDL_Renderer;

namespace Olympe {
namespace Tiled {

    // Forward declare CameraTransform
    struct CameraTransform;

    struct ParallaxLayer
    {
        std::string name;
        std::string imagePath;
        float scrollFactorX;
        float scrollFactorY;
        float offsetX;
        float offsetY;
        float opacity;
        bool repeatX;
        bool repeatY;
        bool visible;
        int tintColor;  // ARGB
        int zOrder;     // Z-order for sorting (lower = background, higher = foreground)
        SDL_Texture* texture; // Runtime texture

        ParallaxLayer()
            : scrollFactorX(1.0f), scrollFactorY(1.0f),
              offsetX(0.0f), offsetY(0.0f), opacity(1.0f),
              repeatX(false), repeatY(false), visible(true),
              tintColor(0xFFFFFFFF), zOrder(0), texture(nullptr) {}
    };

    class ParallaxLayerManager
    {
    public:
        // Singleton access
        static ParallaxLayerManager& Get();

        // Add a parallax layer
        void AddLayer(const ParallaxLayer& layer);

        // Get all layers
        const std::vector<ParallaxLayer>& GetLayers() const { return layers_; }

        // Clear all layers
        void Clear();

        // Calculate render position for a layer based on camera position
        void CalculateRenderPosition(
            const ParallaxLayer& layer,
            float cameraX, float cameraY,
            float& outX, float& outY) const;

        // Get number of layers
        size_t GetLayerCount() const { return layers_.size(); }

        // Get layer by index
        const ParallaxLayer* GetLayer(size_t index) const;

        // Render a specific layer
        void RenderLayer(const ParallaxLayer& layer, const CameraTransform& cam) const;

        // Render all layers in z-order
        void RenderAllLayers(const CameraTransform& cam) const;

    private:
        ParallaxLayerManager();
        ~ParallaxLayerManager();
        
        // Prevent copying
        ParallaxLayerManager(const ParallaxLayerManager&) = delete;
        ParallaxLayerManager& operator=(const ParallaxLayerManager&) = delete;

        std::vector<ParallaxLayer> layers_;
    };

} // namespace Tiled
} // namespace Olympe
