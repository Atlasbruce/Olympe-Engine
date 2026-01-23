/*
 * TiledLevelLoader.h - Main loader for Tiled MapEditor files
 * 
 * Loads and parses Tiled .tmj (JSON map) files with full support for:
 * - Orthogonal and isometric orientations
 * - Finite and infinite (chunked) maps
 * - All layer types (tile, object, image, group)
 * - External tilesets (.tsx, .tsj)
 * - Embedded tilesets
 * - Custom properties
 * - Parallax scrolling
 */

#pragma once

#include "TiledStructures.h"
#include "TiledJsonHelper.h"
#include <string>
#include <memory>

namespace Olympe {
namespace Tiled {

    class TiledLevelLoader
    {
    public:
        TiledLevelLoader();
        ~TiledLevelLoader();

        // Load map from .tmj file
        bool LoadFromFile(const std::string& filepath, TiledMap& outMap);

        // Get last error message
        const std::string& GetLastError() const { return lastError_; }

    private:
        // Parse map JSON
        bool ParseMap(const json& j, TiledMap& map);

        // Parse layers
        bool ParseLayer(const json& j, std::shared_ptr<TiledLayer>& layer);
        bool ParseTileLayer(const json& j, TiledLayer& layer);
        bool ParseObjectLayer(const json& j, TiledLayer& layer);
        bool ParseImageLayer(const json& j, TiledLayer& layer);
        bool ParseGroupLayer(const json& j, TiledLayer& layer);

        // Parse objects
        bool ParseObject(const json& j, TiledObject& object);

        // Parse tilesets
        bool ParseTileset(const json& j, TiledTileset& tileset, const std::string& mapDir);
        bool LoadExternalTileset(const std::string& filepath, TiledTileset& tileset);

        // Parse chunks (for infinite maps)
        bool ParseChunk(const json& j, TiledChunk& chunk,
                        const std::string& layerEncoding,
                        const std::string& layerCompression);

        // Parse properties
        void ParseProperties(const json& j, std::map<std::string, TiledProperty>& properties);
        void ParseProperty(const json& j, TiledProperty& prop);

        // Parse tile data
        bool ParseTileData(const json& j, TiledLayer& layer);

        // Helper: resolve relative path from map directory
        std::string ResolvePath(const std::string& mapDir, const std::string& relativePath);

        // Helper: get directory from file path
        std::string GetDirectory(const std::string& filepath);

        // Helper: read file to string
        bool ReadFile(const std::string& filepath, std::string& outContent);

        std::string lastError_;
    };

} // namespace Tiled
} // namespace Olympe
