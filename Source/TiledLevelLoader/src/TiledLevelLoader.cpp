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
#include "../third_party/tinyxml2/tinyxml2.h"

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

        // Detect format by extension
        size_t dotPos = filepath.find_last_of('.');
        if (dotPos == std::string::npos) {
            lastError_ = "No file extension in " + filepath;
            SYSTEM_LOG << "TiledLevelLoader: " << lastError_ << std::endl;
            return false;
        }

        std::string ext = filepath.substr(dotPos);
        std::string mapDir = GetDirectory(filepath);
        
        bool success = false;
        
        if (ext == ".tmx") {
            // Parse TMX (XML format)
            tinyxml2::XMLDocument doc;
            if (doc.LoadFile(filepath.c_str()) != tinyxml2::XML_SUCCESS) {
                lastError_ = "Failed to load TMX file: " + filepath;
                SYSTEM_LOG << "TiledLevelLoader: " << lastError_ << std::endl;
                return false;
            }
            
            success = ParseMapXML(&doc, outMap);
        }
        else if (ext == ".tmj" || ext == ".json") {
            // Parse TMJ (JSON format)
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
            
            success = ParseMap(j, outMap);
        }
        else {
            lastError_ = "Unknown map format: " + ext + " (expected .tmx, .tmj, or .json)";
            SYSTEM_LOG << "TiledLevelLoader: " << lastError_ << std::endl;
            return false;
        }
        
        if (!success) {
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
        
        // Calculate lastgid for all tilesets
        outMap.CalculateAllLastGids();
        
        // Log tileset information
        for (const auto& tileset : outMap.tilesets) {
            SYSTEM_LOG << "TiledLevelLoader: Tileset '" << tileset.name 
                      << "' - firstgid=" << tileset.firstgid 
                      << ", lastgid=" << tileset.lastgid 
                      << ", tilecount=" << tileset.tilecount << std::endl;
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
        layer.startx = GetInt(j, "startx", 0);  // Parse startx offset
        layer.starty = GetInt(j, "starty", 0);  // Parse starty offset
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
        
        // ====================================================================
        // CRITICAL: Parse tileoffset for embedded tilesets
        // This was previously only parsed for external tilesets in TilesetParser
        // Now properly handled for embedded tilesets as well
        // ====================================================================
        if (HasKey(j, "tileoffset"))
        {
            const auto& offset = j["tileoffset"];
            tileset.tileoffsetX = GetInt(offset, "x");
            tileset.tileoffsetY = GetInt(offset, "y");
            SYSTEM_LOG << "TiledLevelLoader: Parsed embedded tileset tileoffset (" 
                      << tileset.tileoffsetX << ", " << tileset.tileoffsetY 
                      << ") for tileset '" << tileset.name << "'" << std::endl;
        }
        else
        {
            // Explicit defaults when no tileoffset property present
            tileset.tileoffsetX = 0;
            tileset.tileoffsetY = 0;
        }
        
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
        SYSTEM_LOG << "TiledLevelLoader: Loading external tileset from " << filepath << std::endl;
        
        // Try to get from cache (cache will load from file if not already cached)
        auto cachedTileset = TilesetCache::GetInstance().GetTileset(filepath);
        if (cachedTileset) {
            // Copy data from cached tileset (preserve firstgid and source)
            int firstgid = tileset.firstgid;
            std::string source = tileset.source;
            tileset = *cachedTileset;
            tileset.firstgid = firstgid;
            tileset.source = source;
            
            // Log successful loading with tileoffset info
            SYSTEM_LOG << "TiledLevelLoader: External tileset loaded successfully"
                      << " - firstgid=" << firstgid
                      << ", tileoffset=(" << tileset.tileoffsetX << ", " << tileset.tileoffsetY << ")"
                      << std::endl;
            return true;
        }

        // If we get here, cache returned nullptr meaning parsing failed
        SYSTEM_LOG << "TiledLevelLoader: Failed to load external tileset from " << filepath 
                  << " - File may not exist, be corrupted, or have invalid format" << std::endl;
        lastError_ = "Failed to load or parse external tileset: " + filepath;
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
                if (j["data"].is_string()) {
                    std::string dataStr = j["data"].get<std::string>();
                    chunk.data = TiledDecoder::DecodeTileData(dataStr, layerEncoding, layerCompression);
                    
                    // Validate decoded data
                    if (chunk.data.empty() && !dataStr.empty()) {
                        SYSTEM_LOG << "TiledLevelLoader: ERROR - Failed to decode chunk data at (" 
                                  << chunk.x << ", " << chunk.y << ")"
                                  << " (encoding=" << layerEncoding 
                                  << ", compression=" << layerCompression << ")" << std::endl;
                        return false;
                    }
                }
                else if (j["data"].is_array()) {
                    // CSV array
                    for (size_t i = 0; i < j["data"].size(); ++i) {
                        const json& val = j["data"][i];
                        if (val.is_number()) {
                            chunk.data.push_back(static_cast<uint32_t>(val.get<int>()));
                        }
                    }
                }
                
                // ====================================================================
                // CRITICAL VALIDATION: Check chunk data size matches dimensions
                // ====================================================================
                int expectedSize = chunk.width * chunk.height;
                int actualSize = static_cast<int>(chunk.data.size());
                
                if (actualSize != expectedSize) {
                    SYSTEM_LOG << "TiledLevelLoader: ERROR - Chunk data size mismatch at (" 
                              << chunk.x << ", " << chunk.y << ")"
                              << "\n  Expected: " << expectedSize << " tiles (" << chunk.width << " x " << chunk.height << ")"
                              << "\n  Actual: " << actualSize << " tiles"
                              << "\n  Encoding: " << layerEncoding
                              << "\n  Compression: " << (layerCompression.empty() ? "none" : layerCompression)
                              << std::endl;
                    return false;
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            SYSTEM_LOG << "TiledLevelLoader: Failed to parse chunk at (" << chunk.x << ", " << chunk.y << "): " << e.what() << std::endl;
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
            SYSTEM_LOG << "TiledLevelLoader: Layer '" << layer.name << "' has no 'data' field" << std::endl;
            return false;
        }

        if (j["data"].is_string()) {
            // Encoded data (base64)
            std::string dataStr = j["data"].get<std::string>();
            layer.data = TiledDecoder::DecodeTileData(dataStr, layer.encoding, layer.compression);
            
            // Validate decoded data size
            if (layer.data.empty() && !dataStr.empty()) {
                SYSTEM_LOG << "TiledLevelLoader: ERROR - Failed to decode tile data for layer '" 
                          << layer.name << "' (encoding=" << layer.encoding 
                          << ", compression=" << layer.compression << ")" << std::endl;
                lastError_ = "Failed to decode tile data for layer: " + layer.name;
                return false;
            }
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

        // ====================================================================
        // CRITICAL VALIDATION: Check data size matches layer dimensions
        // ====================================================================
        int expectedSize = layer.width * layer.height;
        int actualSize = static_cast<int>(layer.data.size());
        
        if (actualSize != expectedSize) {
            SYSTEM_LOG << "TiledLevelLoader: ERROR - Data size mismatch for layer '" << layer.name << "'"
                      << "\n  Expected: " << expectedSize << " tiles (" << layer.width << " x " << layer.height << ")"
                      << "\n  Actual: " << actualSize << " tiles"
                      << "\n  Encoding: " << layer.encoding
                      << "\n  Compression: " << (layer.compression.empty() ? "none" : layer.compression)
                      << std::endl;
            lastError_ = "Data size mismatch for layer '" + layer.name + "': expected " 
                       + std::to_string(expectedSize) + " but got " + std::to_string(actualSize);
            return false;
        }
        
        SYSTEM_LOG << "TiledLevelLoader: Successfully parsed layer '" << layer.name 
                  << "' with " << actualSize << " tiles" << std::endl;

        return true;
    }

    // ============================================================================
    // TMX XML Parsing Functions
    // ============================================================================

    bool TiledLevelLoader::ParseMapXML(void* docPtr, TiledMap& map)
    {
        tinyxml2::XMLDocument* doc = static_cast<tinyxml2::XMLDocument*>(docPtr);
        tinyxml2::XMLElement* mapElement = doc->FirstChildElement("map");
        
        if (!mapElement) {
            lastError_ = "No <map> element in TMX file";
            SYSTEM_LOG << "TiledLevelLoader: " << lastError_ << std::endl;
            return false;
        }

        // Parse map attributes
        map.version = mapElement->IntAttribute("version", 1);
        map.tiledversion = mapElement->Attribute("tiledversion") ? mapElement->Attribute("tiledversion") : "";
        map.type = "map";
        
        // Parse orientation
        const char* orientAttr = mapElement->Attribute("orientation");
        if (orientAttr) {
            std::string orientStr = orientAttr;
            if (orientStr == "orthogonal") map.orientation = MapOrientation::Orthogonal;
            else if (orientStr == "isometric") map.orientation = MapOrientation::Isometric;
            else if (orientStr == "staggered") map.orientation = MapOrientation::Staggered;
            else if (orientStr == "hexagonal") map.orientation = MapOrientation::Hexagonal;
        }
        
        // Parse render order
        const char* renderAttr = mapElement->Attribute("renderorder");
        if (renderAttr) {
            std::string renderStr = renderAttr;
            if (renderStr == "right-down") map.renderorder = RenderOrder::RightDown;
            else if (renderStr == "right-up") map.renderorder = RenderOrder::RightUp;
            else if (renderStr == "left-down") map.renderorder = RenderOrder::LeftDown;
            else if (renderStr == "left-up") map.renderorder = RenderOrder::LeftUp;
        }
        
        map.compressionlevel = mapElement->IntAttribute("compressionlevel", -1);
        map.width = mapElement->IntAttribute("width", 0);
        map.height = mapElement->IntAttribute("height", 0);
        map.tilewidth = mapElement->IntAttribute("tilewidth", 0);
        map.tileheight = mapElement->IntAttribute("tileheight", 0);
        map.infinite = mapElement->BoolAttribute("infinite", false);
        
        const char* bgColorAttr = mapElement->Attribute("backgroundcolor");
        if (bgColorAttr) {
            map.backgroundcolor = bgColorAttr;
        }
        
        map.nextlayerid = mapElement->IntAttribute("nextlayerid", 1);
        map.nextobjectid = mapElement->IntAttribute("nextobjectid", 1);
        
        // Validation
        if (map.width == 0 || map.height == 0 || map.tilewidth == 0 || map.tileheight == 0) {
            lastError_ = "Invalid map dimensions in TMX";
            SYSTEM_LOG << "TiledLevelLoader: " << lastError_ << std::endl;
            return false;
        }

        // Parse tilesets
        for (tinyxml2::XMLElement* tsElement = mapElement->FirstChildElement("tileset");
             tsElement != nullptr;
             tsElement = tsElement->NextSiblingElement("tileset"))
        {
            TiledTileset tileset;
            if (ParseTilesetXML(tsElement, tileset, "")) {
                map.tilesets.push_back(tileset);
            }
        }

        // Parse layers
        for (tinyxml2::XMLElement* layerElement = mapElement->FirstChildElement();
             layerElement != nullptr;
             layerElement = layerElement->NextSiblingElement())
        {
            std::string elementName = layerElement->Name();
            if (elementName == "layer" || elementName == "objectgroup" || 
                elementName == "imagelayer" || elementName == "group")
            {
                std::shared_ptr<TiledLayer> layer;
                if (ParseLayerXML(layerElement, layer)) {
                    map.layers.push_back(layer);
                }
            }
        }

        // Parse properties
        tinyxml2::XMLElement* propertiesElement = mapElement->FirstChildElement("properties");
        if (propertiesElement) {
            ParsePropertiesXML(propertiesElement, map.properties);
        }

        return true;
    }

    bool TiledLevelLoader::ParseTilesetXML(void* element, TiledTileset& tileset, const std::string& mapDir)
    {
        tinyxml2::XMLElement* tsElement = static_cast<tinyxml2::XMLElement*>(element);
        
        tileset.firstgid = tsElement->IntAttribute("firstgid", 0);
        
        // Check for external tileset
        const char* sourceAttr = tsElement->Attribute("source");
        if (sourceAttr) {
            tileset.source = sourceAttr;
            return true; // Will be loaded later
        }
        
        // Parse embedded tileset
        tileset.name = tsElement->Attribute("name") ? tsElement->Attribute("name") : "";
        tileset.tilewidth = tsElement->IntAttribute("tilewidth", 0);
        tileset.tileheight = tsElement->IntAttribute("tileheight", 0);
        tileset.tilecount = tsElement->IntAttribute("tilecount", 0);
        tileset.columns = tsElement->IntAttribute("columns", 0);
        tileset.spacing = tsElement->IntAttribute("spacing", 0);
        tileset.margin = tsElement->IntAttribute("margin", 0);
        
        // Parse tileoffset
        tinyxml2::XMLElement* offsetElement = tsElement->FirstChildElement("tileoffset");
        if (offsetElement) {
            tileset.tileoffsetX = offsetElement->IntAttribute("x", 0);
            tileset.tileoffsetY = offsetElement->IntAttribute("y", 0);
            SYSTEM_LOG << "TiledLevelLoader (TMX): Parsed tileoffset (" 
                      << tileset.tileoffsetX << ", " << tileset.tileoffsetY 
                      << ") for tileset '" << tileset.name << "'" << std::endl;
        }
        
        // Parse image
        tinyxml2::XMLElement* imageElement = tsElement->FirstChildElement("image");
        if (imageElement) {
            tileset.image = imageElement->Attribute("source") ? imageElement->Attribute("source") : "";
            tileset.imagewidth = imageElement->IntAttribute("width", 0);
            tileset.imageheight = imageElement->IntAttribute("height", 0);
            const char* trans = imageElement->Attribute("trans");
            if (trans) {
                std::string transStr = trans;
                tileset.transparentcolor = (transStr[0] == '#') ? transStr : ("#" + transStr);
            }
        }
        
        // Parse tiles (for collection tilesets)
        for (tinyxml2::XMLElement* tileElement = tsElement->FirstChildElement("tile");
             tileElement != nullptr;
             tileElement = tileElement->NextSiblingElement("tile"))
        {
            TiledTile tile;
            tile.id = tileElement->IntAttribute("id", 0);
            
            const char* typeAttr = tileElement->Attribute("type");
            if (typeAttr) {
                tile.type = typeAttr;
            }
            
            // Parse tile image
            tinyxml2::XMLElement* tileImageElement = tileElement->FirstChildElement("image");
            if (tileImageElement) {
                tile.image = tileImageElement->Attribute("source") ? 
                            tileImageElement->Attribute("source") : "";
                tile.imagewidth = tileImageElement->IntAttribute("width", 0);
                tile.imageheight = tileImageElement->IntAttribute("height", 0);
            }
            
            // Parse tile properties
            tinyxml2::XMLElement* tilePropsElement = tileElement->FirstChildElement("properties");
            if (tilePropsElement) {
                ParsePropertiesXML(tilePropsElement, tile.properties);
            }
            
            tileset.tiles.push_back(tile);
        }
        
        // Parse tileset properties
        tinyxml2::XMLElement* propertiesElement = tsElement->FirstChildElement("properties");
        if (propertiesElement) {
            ParsePropertiesXML(propertiesElement, tileset.properties);
        }
        
        return true;
    }

    bool TiledLevelLoader::ParseLayerXML(void* element, std::shared_ptr<TiledLayer>& layer)
    {
        tinyxml2::XMLElement* layerElement = static_cast<tinyxml2::XMLElement*>(element);
        layer = std::make_shared<TiledLayer>();
        
        std::string elementName = layerElement->Name();
        
        // Common attributes
        layer->id = layerElement->IntAttribute("id", 0);
        layer->name = layerElement->Attribute("name") ? layerElement->Attribute("name") : "";
        layer->visible = layerElement->IntAttribute("visible", 1) != 0;
        layer->opacity = layerElement->FloatAttribute("opacity", 1.0f);
        layer->offsetx = layerElement->FloatAttribute("offsetx", 0.0f);
        layer->offsety = layerElement->FloatAttribute("offsety", 0.0f);
        layer->parallaxx = layerElement->FloatAttribute("parallaxx", 1.0f);
        layer->parallaxy = layerElement->FloatAttribute("parallaxy", 1.0f);
        
        const char* tintAttr = layerElement->Attribute("tintcolor");
        if (tintAttr) {
            layer->tintcolor = ParseColor(tintAttr);
        }
        
        // Parse by layer type
        if (elementName == "layer") {
            layer->type = LayerType::TileLayer;
            if (!ParseTileLayerXML(layerElement, *layer)) return false;
        }
        else if (elementName == "objectgroup") {
            layer->type = LayerType::ObjectGroup;
            if (!ParseObjectLayerXML(layerElement, *layer)) return false;
        }
        else if (elementName == "imagelayer") {
            layer->type = LayerType::ImageLayer;
            if (!ParseImageLayerXML(layerElement, *layer)) return false;
        }
        else if (elementName == "group") {
            layer->type = LayerType::Group;
            if (!ParseGroupLayerXML(layerElement, *layer)) return false;
        }
        
        // Parse properties
        tinyxml2::XMLElement* propertiesElement = layerElement->FirstChildElement("properties");
        if (propertiesElement) {
            ParsePropertiesXML(propertiesElement, layer->properties);
        }
        
        return true;
    }

    bool TiledLevelLoader::ParseTileLayerXML(void* element, TiledLayer& layer)
    {
        tinyxml2::XMLElement* layerElement = static_cast<tinyxml2::XMLElement*>(element);
        
        layer.width = layerElement->IntAttribute("width", 0);
        layer.height = layerElement->IntAttribute("height", 0);
        layer.startx = layerElement->IntAttribute("startx", 0);
        layer.starty = layerElement->IntAttribute("starty", 0);
        
        // Parse data element
        tinyxml2::XMLElement* dataElement = layerElement->FirstChildElement("data");
        if (dataElement) {
            if (!ParseTileDataXML(dataElement, layer)) {
                return false;
            }
        }
        
        // Parse chunks (for infinite maps)
        if (dataElement) {
            for (tinyxml2::XMLElement* chunkElement = dataElement->FirstChildElement("chunk");
                 chunkElement != nullptr;
                 chunkElement = chunkElement->NextSiblingElement("chunk"))
            {
                TiledChunk chunk;
                if (ParseChunkXML(chunkElement, chunk, layer.encoding, layer.compression)) {
                    layer.chunks.push_back(chunk);
                }
            }
        }
        
        return true;
    }

    bool TiledLevelLoader::ParseObjectLayerXML(void* element, TiledLayer& layer)
    {
        tinyxml2::XMLElement* layerElement = static_cast<tinyxml2::XMLElement*>(element);
        
        // Parse objects
        for (tinyxml2::XMLElement* objElement = layerElement->FirstChildElement("object");
             objElement != nullptr;
             objElement = objElement->NextSiblingElement("object"))
        {
            TiledObject object;
            if (ParseObjectXML(objElement, object)) {
                layer.objects.push_back(object);
            }
        }
        
        return true;
    }

    bool TiledLevelLoader::ParseImageLayerXML(void* element, TiledLayer& layer)
    {
        tinyxml2::XMLElement* layerElement = static_cast<tinyxml2::XMLElement*>(element);
        
        // Parse image element
        tinyxml2::XMLElement* imageElement = layerElement->FirstChildElement("image");
        if (imageElement) {
            layer.image = imageElement->Attribute("source") ? imageElement->Attribute("source") : "";
        }
        
        layer.repeatx = layerElement->BoolAttribute("repeatx", false);
        layer.repeaty = layerElement->BoolAttribute("repeaty", false);
        
        return true;
    }

    bool TiledLevelLoader::ParseGroupLayerXML(void* element, TiledLayer& layer)
    {
        tinyxml2::XMLElement* layerElement = static_cast<tinyxml2::XMLElement*>(element);
        
        // Parse child layers
        for (tinyxml2::XMLElement* childElement = layerElement->FirstChildElement();
             childElement != nullptr;
             childElement = childElement->NextSiblingElement())
        {
            std::string elementName = childElement->Name();
            if (elementName == "layer" || elementName == "objectgroup" || 
                elementName == "imagelayer" || elementName == "group")
            {
                std::shared_ptr<TiledLayer> childLayer;
                if (ParseLayerXML(childElement, childLayer)) {
                    layer.layers.push_back(childLayer);
                }
            }
        }
        
        return true;
    }

    bool TiledLevelLoader::ParseObjectXML(void* element, TiledObject& object)
    {
        tinyxml2::XMLElement* objElement = static_cast<tinyxml2::XMLElement*>(element);
        
        object.id = objElement->IntAttribute("id", 0);
        object.name = objElement->Attribute("name") ? objElement->Attribute("name") : "";
        object.type = objElement->Attribute("type") ? objElement->Attribute("type") : "";
        object.x = objElement->FloatAttribute("x", 0.0f);
        object.y = objElement->FloatAttribute("y", 0.0f);
        object.width = objElement->FloatAttribute("width", 0.0f);
        object.height = objElement->FloatAttribute("height", 0.0f);
        object.rotation = objElement->FloatAttribute("rotation", 0.0f);
        object.gid = objElement->IntAttribute("gid", 0);
        object.visible = objElement->IntAttribute("visible", 1) != 0;
        
        // Determine object type
        tinyxml2::XMLElement* pointElement = objElement->FirstChildElement("point");
        tinyxml2::XMLElement* ellipseElement = objElement->FirstChildElement("ellipse");
        tinyxml2::XMLElement* polygonElement = objElement->FirstChildElement("polygon");
        tinyxml2::XMLElement* polylineElement = objElement->FirstChildElement("polyline");
        tinyxml2::XMLElement* textElement = objElement->FirstChildElement("text");
        
        if (pointElement) {
            object.objectType = ObjectType::Point;
        }
        else if (ellipseElement) {
            object.objectType = ObjectType::Ellipse;
        }
        else if (polygonElement) {
            object.objectType = ObjectType::Polygon;
            const char* points = polygonElement->Attribute("points");
            if (points) {
                // Parse points string "x1,y1 x2,y2 x3,y3"
                std::string pointsStr = points;
                size_t pos = 0;
                while (pos < pointsStr.length()) {
                    size_t commaPos = pointsStr.find(',', pos);
                    size_t spacePos = pointsStr.find(' ', commaPos);
                    if (commaPos == std::string::npos) break;
                    
                    float x = std::stof(pointsStr.substr(pos, commaPos - pos));
                    float y = std::stof(pointsStr.substr(commaPos + 1, 
                                       (spacePos == std::string::npos ? pointsStr.length() : spacePos) - commaPos - 1));
                    object.polygon.push_back(Point(x, y));
                    
                    if (spacePos == std::string::npos) break;
                    pos = spacePos + 1;
                }
            }
        }
        else if (polylineElement) {
            object.objectType = ObjectType::Polyline;
            const char* points = polylineElement->Attribute("points");
            if (points) {
                // Parse points string "x1,y1 x2,y2 x3,y3"
                std::string pointsStr = points;
                size_t pos = 0;
                while (pos < pointsStr.length()) {
                    size_t commaPos = pointsStr.find(',', pos);
                    size_t spacePos = pointsStr.find(' ', commaPos);
                    if (commaPos == std::string::npos) break;
                    
                    float x = std::stof(pointsStr.substr(pos, commaPos - pos));
                    float y = std::stof(pointsStr.substr(commaPos + 1, 
                                       (spacePos == std::string::npos ? pointsStr.length() : spacePos) - commaPos - 1));
                    object.polyline.push_back(Point(x, y));
                    
                    if (spacePos == std::string::npos) break;
                    pos = spacePos + 1;
                }
            }
        }
        else if (textElement) {
            object.objectType = ObjectType::Text;
            const char* textContent = textElement->GetText();
            if (textContent) {
                object.text = textContent;
            }
        }
        else {
            object.objectType = ObjectType::Rectangle;
        }
        
        // Parse properties
        tinyxml2::XMLElement* propertiesElement = objElement->FirstChildElement("properties");
        if (propertiesElement) {
            ParsePropertiesXML(propertiesElement, object.properties);
        }
        
        return true;
    }

    bool TiledLevelLoader::ParseTileDataXML(void* element, TiledLayer& layer)
    {
        tinyxml2::XMLElement* dataElement = static_cast<tinyxml2::XMLElement*>(element);
        
        // Get encoding and compression
        const char* encodingAttr = dataElement->Attribute("encoding");
        const char* compressionAttr = dataElement->Attribute("compression");
        
        layer.encoding = encodingAttr ? encodingAttr : "csv";
        layer.compression = compressionAttr ? compressionAttr : "";
        
        const char* dataText = dataElement->GetText();
        if (!dataText) {
            // Check for tile elements (uncompressed XML format)
            for (tinyxml2::XMLElement* tileElement = dataElement->FirstChildElement("tile");
                 tileElement != nullptr;
                 tileElement = tileElement->NextSiblingElement("tile"))
            {
                uint32_t gid = tileElement->IntAttribute("gid", 0);
                layer.data.push_back(gid);
            }
        }
        else {
            std::string dataStr = dataText;
            
            // Trim whitespace
            size_t start = dataStr.find_first_not_of(" \t\n\r");
            size_t end = dataStr.find_last_not_of(" \t\n\r");
            if (start != std::string::npos && end != std::string::npos) {
                dataStr = dataStr.substr(start, end - start + 1);
            }
            
            if (layer.encoding == "csv") {
                // Parse CSV
                size_t pos = 0;
                while (pos < dataStr.length()) {
                    size_t commaPos = dataStr.find(',', pos);
                    std::string token = dataStr.substr(pos, 
                                       (commaPos == std::string::npos ? dataStr.length() : commaPos) - pos);
                    
                    // Trim token
                    size_t tokenStart = token.find_first_not_of(" \t\n\r");
                    size_t tokenEnd = token.find_last_not_of(" \t\n\r");
                    if (tokenStart != std::string::npos && tokenEnd != std::string::npos) {
                        token = token.substr(tokenStart, tokenEnd - tokenStart + 1);
                    }
                    
                    if (!token.empty()) {
                        try {
                            uint32_t gid = static_cast<uint32_t>(std::stoul(token));
                            layer.data.push_back(gid);
                        } catch (const std::exception& e) {
                            SYSTEM_LOG << "TiledLevelLoader: Failed to parse CSV token: " << token << " Error : " << e.what() << std::endl;
                        }
                    }
                    
                    if (commaPos == std::string::npos) break;
                    pos = commaPos + 1;
                }
            }
            else if (layer.encoding == "base64") {
                // Decode base64
                layer.data = TiledDecoder::DecodeTileData(dataStr, layer.encoding, layer.compression);
            }
        }
        
        // Validate data size
        int expectedSize = layer.width * layer.height;
        int actualSize = static_cast<int>(layer.data.size());
        
        if (actualSize != expectedSize && !dataText) {
            SYSTEM_LOG << "TiledLevelLoader: ERROR - Data size mismatch for layer '" << layer.name << "'"
                      << "\n  Expected: " << expectedSize << " tiles (" << layer.width << " x " << layer.height << ")"
                      << "\n  Actual: " << actualSize << " tiles" << std::endl;
            lastError_ = "Data size mismatch for layer '" + layer.name + "'";
            return false;
        }
        
        SYSTEM_LOG << "TiledLevelLoader: Successfully parsed TMX layer '" << layer.name 
                  << "' with " << actualSize << " tiles" << std::endl;
        
        return true;
    }

    bool TiledLevelLoader::ParseChunkXML(void* element, TiledChunk& chunk,
                                         const std::string& layerEncoding,
                                         const std::string& layerCompression)
    {
        tinyxml2::XMLElement* chunkElement = static_cast<tinyxml2::XMLElement*>(element);
        
        chunk.x = chunkElement->IntAttribute("x", 0);
        chunk.y = chunkElement->IntAttribute("y", 0);
        chunk.width = chunkElement->IntAttribute("width", 0);
        chunk.height = chunkElement->IntAttribute("height", 0);
        
        const char* dataText = chunkElement->GetText();
        if (dataText) {
            std::string dataStr = dataText;
            
            // Trim whitespace
            size_t start = dataStr.find_first_not_of(" \t\n\r");
            size_t end = dataStr.find_last_not_of(" \t\n\r");
            if (start != std::string::npos && end != std::string::npos) {
                dataStr = dataStr.substr(start, end - start + 1);
            }
            
            chunk.data = TiledDecoder::DecodeTileData(dataStr, layerEncoding, layerCompression);
            
            // Validate chunk data size
            int expectedSize = chunk.width * chunk.height;
            int actualSize = static_cast<int>(chunk.data.size());
            
            if (actualSize != expectedSize) {
                SYSTEM_LOG << "TiledLevelLoader: ERROR - Chunk data size mismatch at (" 
                          << chunk.x << ", " << chunk.y << ")"
                          << "\n  Expected: " << expectedSize << " tiles"
                          << "\n  Actual: " << actualSize << " tiles" << std::endl;
                return false;
            }
        }
        
        return true;
    }

    void TiledLevelLoader::ParsePropertiesXML(void* element, std::map<std::string, TiledProperty>& properties)
    {
        tinyxml2::XMLElement* propsElement = static_cast<tinyxml2::XMLElement*>(element);
        
        for (tinyxml2::XMLElement* propElement = propsElement->FirstChildElement("property");
             propElement != nullptr;
             propElement = propElement->NextSiblingElement("property"))
        {
            TiledProperty prop;
            ParsePropertyXML(propElement, prop);
            properties[prop.name] = prop;
        }
    }

    void TiledLevelLoader::ParsePropertyXML(void* element, TiledProperty& prop)
    {
        tinyxml2::XMLElement* propElement = static_cast<tinyxml2::XMLElement*>(element);
        
        prop.name = propElement->Attribute("name") ? propElement->Attribute("name") : "";
        
        const char* typeAttr = propElement->Attribute("type");
        std::string typeStr = typeAttr ? typeAttr : "string";
        
        if (typeStr == "int") {
            prop.type = PropertyType::Int;
            prop.intValue = propElement->IntAttribute("value", 0);
        }
        else if (typeStr == "float") {
            prop.type = PropertyType::Float;
            prop.floatValue = propElement->FloatAttribute("value", 0.0f);
        }
        else if (typeStr == "bool") {
            prop.type = PropertyType::Bool;
            prop.boolValue = propElement->BoolAttribute("value", false);
        }
        else if (typeStr == "color") {
            prop.type = PropertyType::Color;
            prop.stringValue = propElement->Attribute("value") ? propElement->Attribute("value") : "";
        }
        else if (typeStr == "file") {
            prop.type = PropertyType::File;
            prop.stringValue = propElement->Attribute("value") ? propElement->Attribute("value") : "";
        }
        else {
            prop.type = PropertyType::String;
            prop.stringValue = propElement->Attribute("value") ? propElement->Attribute("value") : "";
        }
    }

    // ============================================================================
    // End of TMX XML Parsing Functions
    // ============================================================================

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
