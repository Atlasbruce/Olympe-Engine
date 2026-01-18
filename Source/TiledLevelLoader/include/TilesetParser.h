/*
 * TilesetParser.h - Parser for external Tiled tileset files
 * 
 * Supports parsing:
 * - .tsx files (XML format) using tinyxml2
 * - .tsj files (JSON format) using nlohmann::json
 * 
 * Handles both image-based tilesets (single image grid) and
 * collection tilesets (individual tile images).
 */

#pragma once

#include "TiledStructures.h"
#include "../../third_party/nlohmann/json.hpp"
#include <string>
#include <memory>

namespace Olympe {
namespace Tiled {
      
    class TilesetParser
    {
    public:
        TilesetParser();
        ~TilesetParser();

        // Parse external tileset file (auto-detects format by extension)
        bool ParseFile(const std::string& filepath, TiledTileset& tileset);

        // Parse TSX (XML) format
        bool ParseTSX(const std::string& filepath, TiledTileset& tileset);

        // Parse TSJ (JSON) format
        bool ParseTSJ(const std::string& filepath, TiledTileset& tileset);

    private:
        void ParsePropertiesFromXML(void* element, std::map<std::string, TiledProperty>& properties);
        void ParsePropertiesFromJSON(const nlohmann::json& j, std::map<std::string, TiledProperty>& properties);
        void ParseTileFromXML(void* element, TiledTile& tile);
        void ParseTileFromJSON(const nlohmann::json& j, TiledTile& tile);
    };

} // namespace Tiled
} // namespace Olympe
