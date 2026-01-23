/*
 * TiledLevelLoader.cpp - Main loader implementation
 */

#include "../include/TiledLevelLoader.h"
#include "../include/TiledDecoder.h"
#include "../include/TilesetCache.h"
#include "../include/TilesetParser.h"
#include "../../system/system_utils.h"
#include <fstream>
#include <sstream>
#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {
namespace Tiled {

    TiledLevelLoader::TiledLevelLoader()
    {
    }

    TiledLevelLoader::~TiledLevelLoader()
    {
    }

    bool TiledLevelLoader::LoadFromFile(const std::string& filepath, TiledMap& outMap)
    {
        lastError_.clear();

        SYSTEM_LOG << "TiledLevelLoader: Loading map from " << filepath << std::endl;

        std::string content;
        if (!ReadFile(filepath, content)) {
            lastError_ = "Failed to read file: " + filepath;
            return false;
        }

        json j;
        try {
            j = json::parse(content);
        } catch (const std::exception& e) {
            lastError_ = std::string("JSON parse error: ") + e.what();
            SYSTEM_LOG << "TiledLevelLoader: " << lastError_ << std::endl;
            return false;
        }

        std::string mapDir = GetDirectory(filepath);
        
        if (!ParseMap(j, outMap)) {
            SYSTEM_LOG << "TiledLevelLoader: Failed to parse map" << std::endl;
            return false;
        }

        // Load external tilesets
        for (auto& tileset : outMap.tilesets) {
            if (!tileset.source.empty()) {
                std::string tilesetPath = ResolvePath(mapDir, tileset.source);
                if (!LoadExternalTileset(tilesetPath, tileset)) {
                    lastError_ = "Failed to load external tileset: " + tilesetPath;
                    SYSTEM_LOG << "TiledLevelLoader: " << lastError_ << std::endl;
                    return false;
                }
            }
        }

        SYSTEM_LOG << "TiledLevelLoader: Successfully loaded map" << std::endl;
        return true;
    }

    bool TiledLevelLoader::ParseMap(const json& j, TiledMap& map)
    {
        map.version = GetInt(j, "version", 1);
        map.tiledversion = GetString(j, "tiledversion");
        map.type = GetString(j, "type", "map");
        
        // Parse orientation
        std::string orientStr = GetString(j, "orientation", "orthogonal");
        if (orientStr == "orthogonal") map.orientation = MapOrientation::Orthogonal;
        else if (orientStr == "isometric") map.orientation = MapOrientation::Isometric;
        else if (orientStr == "staggered") map.orientation = MapOrientation::Staggered;
        else if (orientStr == "hexagonal") map.orientation = MapOrientation::Hexagonal;

        // Parse render order
        std::string renderStr = GetString(j, "renderorder", "right-down");
        if (renderStr == "right-down") map.renderorder = RenderOrder::RightDown;
        else if (renderStr == "right-up") map.renderorder = RenderOrder::RightUp;
        else if (renderStr == "left-down") map.renderorder = RenderOrder::LeftDown;
        else if (renderStr == "left-up") map.renderorder = RenderOrder::LeftUp;

        map.compressionlevel = GetInt(j, "compressionlevel", -1);
        map.width = GetInt(j, "width");
        map.height = GetInt(j, "height");
        map.tilewidth = GetInt(j, "tilewidth");
        map.tileheight = GetInt(j, "tileheight");
        map.infinite = GetBool(j, "infinite", false);
        map.backgroundcolor = GetString(j, "backgroundcolor");
        map.nextlayerid = GetInt(j, "nextlayerid", 1);
        map.nextobjectid = GetInt(j, "nextobjectid", 1);

        // Parse tilesets
        if (HasKey(j, "tilesets") && j["tilesets"].is_array()) {
            for (size_t i = 0; i < j["tilesets"].size(); ++i) {
                const json& tsJson = j["tilesets"][i];
                TiledTileset tileset;
                if (ParseTileset(tsJson, tileset, "")) {
                    map.tilesets.push_back(tileset);
                }
            }
        }

        // Parse layers
        if (HasKey(j, "layers") && j["layers"].is_array()) {
            for (size_t i = 0; i < j["layers"].size(); ++i) {
                const json& layerJson = j["layers"][i];
                std::shared_ptr<TiledLayer> layer;
                if (ParseLayer(layerJson, layer)) {
                    map.layers.push_back(layer);
                }
            }
        }

        // Parse properties
        if (HasKey(j, "properties")) {
            ParseProperties(j["properties"], map.properties);
        }

        return true;
    }

    bool TiledLevelLoader::ParseLayer(const json& j, std::shared_ptr<TiledLayer>& layer)
    {
        layer = std::make_shared<TiledLayer>();

        layer->id = GetInt(j, "id");
        layer->name = GetString(j, "name");
        layer->visible = GetBool(j, "visible", true);
        layer->opacity = GetFloat(j, "opacity", 1.0f);
        layer->offsetx = GetFloat(j, "offsetx", 0.0f);
        layer->offsety = GetFloat(j, "offsety", 0.0f);
        layer->parallaxx = GetFloat(j, "parallaxx", 1.0f);
        layer->parallaxy = GetFloat(j, "parallaxy", 1.0f);
        
        if (HasKey(j, "tintcolor")) {
            layer->tintcolor = ParseColor(GetString(j, "tintcolor"));
        }

        std::string typeStr = GetString(j, "type", "tilelayer");
        
        if (typeStr == "tilelayer") {
            layer->type = LayerType::TileLayer;
            if (!ParseTileLayer(j, *layer)) return false;
        }
        else if (typeStr == "objectgroup") {
            layer->type = LayerType::ObjectGroup;
            if (!ParseObjectLayer(j, *layer)) return false;
        }
        else if (typeStr == "imagelayer") {
            layer->type = LayerType::ImageLayer;
            if (!ParseImageLayer(j, *layer)) return false;
        }
        else if (typeStr == "group") {
            layer->type = LayerType::Group;
            if (!ParseGroupLayer(j, *layer)) return false;
        }

        // Parse properties
        if (HasKey(j, "properties")) {
            ParseProperties(j["properties"], layer->properties);
        }

        return true;
    }

    bool TiledLevelLoader::ParseTileLayer(const json& j, TiledLayer& layer)
    {
        layer.width = GetInt(j, "width");
        layer.height = GetInt(j, "height");
        layer.encoding = GetString(j, "encoding", "csv");
        layer.compression = GetString(j, "compression", "");

        // Parse chunks (for infinite maps)
        if (HasKey(j, "chunks") && j["chunks"].is_array()) {
            for (size_t i = 0; i < j["chunks"].size(); ++i) {
                const json& chunkJson = j["chunks"][i];
                TiledChunk chunk;
                // PASS LAYER ENCODING/COMPRESSION TO CHUNK
                if (ParseChunk(chunkJson, chunk, layer.encoding, layer.compression)) {
                    layer.chunks.push_back(chunk);
                }
            }
        }
        // Parse data (for finite maps)
        else if (HasKey(j, "data")) {
            if (!ParseTileData(j, layer)) {
                return false;
            }
        }

        return true;
    }

    bool TiledLevelLoader::ParseObjectLayer(const json& j, TiledLayer& layer)
    {
        if (HasKey(j, "objects") && j["objects"].is_array()) {
            for (size_t i = 0; i < j["objects"].size(); ++i) {
                const json& objJson = j["objects"][i];
                TiledObject object;
                if (ParseObject(objJson, object)) {
                    layer.objects.push_back(object);
                }
            }
        }

        return true;
    }

    bool TiledLevelLoader::ParseImageLayer(const json& j, TiledLayer& layer)
    {
        layer.image = GetString(j, "image");
        layer.repeatx = GetBool(j, "repeatx", false);
        layer.repeaty = GetBool(j, "repeaty", false);
        return true;
    }

    bool TiledLevelLoader::ParseGroupLayer(const json& j, TiledLayer& layer)
    {
        if (HasKey(j, "layers") && j["layers"].is_array()) {
            for (size_t i = 0; i < j["layers"].size(); ++i) {
                const json& childJson = j["layers"][i];
                std::shared_ptr<TiledLayer> childLayer;
                if (ParseLayer(childJson, childLayer)) {
                    layer.layers.push_back(childLayer);
                }
            }
        }

        return true;
    }

    bool TiledLevelLoader::ParseObject(const json& j, TiledObject& object)
    {
        object.id = GetInt(j, "id");
        object.name = GetString(j, "name");
        object.type = GetString(j, "type");
        object.x = GetFloat(j, "x");
        object.y = GetFloat(j, "y");
        object.width = GetFloat(j, "width");
        object.height = GetFloat(j, "height");
        object.rotation = GetFloat(j, "rotation");
        object.gid = GetInt(j, "gid");
        object.visible = GetBool(j, "visible", true);

        // Determine object type
        if (HasKey(j, "point") && GetBool(j, "point", false)) {
            object.objectType = ObjectType::Point;
        }
        else if (HasKey(j, "ellipse") && GetBool(j, "ellipse", false)) {
            object.objectType = ObjectType::Ellipse;
        }
        else if (HasKey(j, "polygon")) {
            object.objectType = ObjectType::Polygon;
            if (j["polygon"].is_array()) {
                for (size_t i = 0; i < j["polygon"].size(); ++i) {
                    const json& ptJson = j["polygon"][i];
                    Point pt;
                    pt.x = GetFloat(ptJson, "x");
                    pt.y = GetFloat(ptJson, "y");
                    object.polygon.push_back(pt);
                }
            }
        }
        else if (HasKey(j, "polyline")) {
            object.objectType = ObjectType::Polyline;
            if (j["polyline"].is_array()) {
                for (size_t i = 0; i < j["polyline"].size(); ++i) {
                    const json& ptJson = j["polyline"][i];
                    Point pt;
                    pt.x = GetFloat(ptJson, "x");
                    pt.y = GetFloat(ptJson, "y");
                    object.polyline.push_back(pt);
                }
            }
        }
        else if (HasKey(j, "text")) {
            object.objectType = ObjectType::Text;
            object.text = GetString(j["text"], "text", "");
        }
        else {
            object.objectType = ObjectType::Rectangle;
        }

        // Parse properties
        if (HasKey(j, "properties")) {
            ParseProperties(j["properties"], object.properties);
        }

        return true;
    }

    bool TiledLevelLoader::ParseTileset(const json& j, TiledTileset& tileset, const std::string& mapDir)
    {
        tileset.firstgid = GetInt(j, "firstgid");
        tileset.source = GetString(j, "source");
        
        // If external tileset, just store the source path
        if (!tileset.source.empty()) {
            return true;
        }

        // Parse embedded tileset
        tileset.name = GetString(j, "name");
        tileset.tilewidth = GetInt(j, "tilewidth");
        tileset.tileheight = GetInt(j, "tileheight");
        tileset.tilecount = GetInt(j, "tilecount");
        tileset.columns = GetInt(j, "columns");
        tileset.spacing = GetInt(j, "spacing");
        tileset.margin = GetInt(j, "margin");
        tileset.image = GetString(j, "image");
        tileset.imagewidth = GetInt(j, "imagewidth");
        tileset.imageheight = GetInt(j, "imageheight");
        tileset.transparentcolor = GetString(j, "transparentcolor");

        // Parse tiles
        if (HasKey(j, "tiles") && j["tiles"].is_array()) {
            for (size_t i = 0; i < j["tiles"].size(); ++i) {
                const json& tileJson = j["tiles"][i];
                TiledTile tile;
                tile.id = GetInt(tileJson, "id");
                tile.type = GetString(tileJson, "type");
                tile.image = GetString(tileJson, "image");
                tile.imagewidth = GetInt(tileJson, "imagewidth");
                tile.imageheight = GetInt(tileJson, "imageheight");

                if (HasKey(tileJson, "properties")) {
                    ParseProperties(tileJson["properties"], tile.properties);
                }

                tileset.tiles.push_back(tile);
            }
        }

        // Parse properties
        if (HasKey(j, "properties")) {
            ParseProperties(j["properties"], tileset.properties);
        }

        return true;
    }

    bool TiledLevelLoader::LoadExternalTileset(const std::string& filepath, TiledTileset& tileset)
    {
        // Try to get from cache
        auto cachedTileset = TilesetCache::GetInstance().GetTileset(filepath);
        if (cachedTileset) {
            // Copy data from cached tileset (preserve firstgid)
            int firstgid = tileset.firstgid;
            std::string source = tileset.source;
            tileset = *cachedTileset;
            tileset.firstgid = firstgid;
            tileset.source = source;
            return true;
        }

        return false;
    }

    bool TiledLevelLoader::ParseChunk(const json& j, TiledChunk& chunk,
                                      const std::string& layerEncoding,
                                      const std::string& layerCompression)
    {
        try {
            chunk.x = GetInt(j, "x");
            chunk.y = GetInt(j, "y");
            chunk.width = GetInt(j, "width");
            chunk.height = GetInt(j, "height");

            if (HasKey(j, "data")) {
                // Use layer encoding/compression instead of chunk-level
                std::string encoding = layerEncoding;
                std::string compression = layerCompression;
                
                SYSTEM_LOG << "[ParseChunk] Decoding chunk at (" << chunk.x << ", " << chunk.y 
                           << ") with encoding: '" << encoding << "', compression: '" << compression << "'\n";
                
                if (j["data"].is_string()) {
                    std::string dataStr = j["data"].get<std::string>();
                    
                    SYSTEM_LOG << "[ParseChunk] Data length: " << dataStr.length() << " chars\n";
                    
                    chunk.data = TiledDecoder::DecodeTileData(dataStr, encoding, compression);
                    
                    if (chunk.data.empty()) {
                        SYSTEM_LOG << "[ParseChunk] ERROR: Failed to decode chunk data!\n";
                        return false;
                    }
                    
                    SYSTEM_LOG << "[ParseChunk] Successfully decoded " << chunk.data.size() << " tiles\n";
                }
                else if (j["data"].is_array()) {
                    // CSV array
                    for (size_t i = 0; i < j["data"].size(); ++i) {
                        const json& val = j["data"][i];
                        if (val.is_number()) {
                            chunk.data.push_back(static_cast<uint32_t>(val.get<int>()));
                        }
                    }
                    SYSTEM_LOG << "[ParseChunk] Loaded " << chunk.data.size() << " tiles from CSV array\n";
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            SYSTEM_LOG << "[ParseChunk] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    void TiledLevelLoader::ParseProperties(const json& j, std::map<std::string, TiledProperty>& properties)
    {
        if (!j.is_array()) return;

        for (size_t i = 0; i < j.size(); ++i) {
            const json& propJson = j[i];
            TiledProperty prop;
            ParseProperty(propJson, prop);
            properties[prop.name] = prop;
        }
    }

    void TiledLevelLoader::ParseProperty(const json& j, TiledProperty& prop)
    {
        prop.name = GetString(j, "name");
        std::string typeStr = GetString(j, "type", "string");

        if (typeStr == "int") {
            prop.type = PropertyType::Int;
            prop.intValue = GetInt(j, "value");
        }
        else if (typeStr == "float") {
            prop.type = PropertyType::Float;
            prop.floatValue = GetFloat(j, "value");
        }
        else if (typeStr == "bool") {
            prop.type = PropertyType::Bool;
            prop.boolValue = GetBool(j, "value");
        }
        else if (typeStr == "color") {
            prop.type = PropertyType::Color;
            prop.stringValue = GetString(j, "value");
        }
        else if (typeStr == "file") {
            prop.type = PropertyType::File;
            prop.stringValue = GetString(j, "value");
        }
        else {
            prop.type = PropertyType::String;
            prop.stringValue = GetString(j, "value");
        }
    }

    bool TiledLevelLoader::ParseTileData(const json& j, TiledLayer& layer)
    {
        if (!HasKey(j, "data")) {
            return false;
        }

        if (j["data"].is_string()) {
            // Encoded data (base64)
            std::string dataStr = j["data"].get<std::string>();
            layer.data = TiledDecoder::DecodeTileData(dataStr, layer.encoding, layer.compression);
        }
        else if (j["data"].is_array()) {
            // CSV array
            for (size_t i = 0; i < j["data"].size(); ++i) {
                const json& val = j["data"][i];
                if (val.is_number()) {
                    layer.data.push_back(static_cast<uint32_t>(val.get<int>()));
                }
            }
        }

        return true;
    }

    std::string TiledLevelLoader::ResolvePath(const std::string& mapDir, const std::string& relativePath)
    {
        if (relativePath.empty()) return relativePath;
        
        // If absolute path, return as-is
        if (relativePath[0] == '/' || (relativePath.length() > 1 && relativePath[1] == ':')) {
            return relativePath;
        }

        // Combine with map directory
        if (mapDir.empty()) {
            return relativePath;
        }

        return mapDir + "\\" + relativePath;
    }

    std::string TiledLevelLoader::GetDirectory(const std::string& filepath)
    {
        size_t pos = filepath.find_last_of("/\\");
        if (pos == std::string::npos) {
            return "";
        }
        return filepath.substr(0, pos);
    }

    bool TiledLevelLoader::ReadFile(const std::string& filepath, std::string& outContent)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            SYSTEM_LOG << "TiledLevelLoader: Failed to open file: " << filepath << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        outContent = buffer.str();
        return true;
    }

} // namespace Tiled
} // namespace Olympe
