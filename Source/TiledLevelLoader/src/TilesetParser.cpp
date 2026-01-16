/*
 * TilesetParser.cpp - Parse external tileset files
 */

#include "TilesetParser.h"
#include "TiledJsonHelper.h"
#include "system/system_utils.h"
#include "tinyxml2/tinyxml2.h"
#include <fstream>
#include <sstream>

namespace Olympe {
namespace Tiled {

    TilesetParser::TilesetParser()
    {
    }

    TilesetParser::~TilesetParser()
    {
    }

    bool TilesetParser::ParseFile(const std::string& filepath, TiledTileset& tileset)
    {
        // Detect format by extension
        size_t dotPos = filepath.find_last_of('.');
        if (dotPos == std::string::npos) {
            SYSTEM_LOG << "TilesetParser: No file extension in " << filepath << std::endl;
            return false;
        }

        std::string ext = filepath.substr(dotPos);
        
        if (ext == ".tsx") {
            return ParseTSX(filepath, tileset);
        }
        else if (ext == ".tsj" || ext == ".json") {
            return ParseTSJ(filepath, tileset);
        }
        else {
            SYSTEM_LOG << "TilesetParser: Unknown tileset format: " << ext << std::endl;
            return false;
        }
    }

    bool TilesetParser::ParseTSX(const std::string& filepath, TiledTileset& tileset)
    {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(filepath.c_str()) != tinyxml2::XML_SUCCESS) {
            SYSTEM_LOG << "TilesetParser: Failed to load TSX file: " << filepath << std::endl;
            return false;
        }

        tinyxml2::XMLElement* tsElement = doc.FirstChildElement("tileset");
        if (!tsElement) {
            SYSTEM_LOG << "TilesetParser: No <tileset> element in " << filepath << std::endl;
            return false;
        }

        // Parse tileset attributes
        tileset.name = tsElement->Attribute("name") ? tsElement->Attribute("name") : "";
        tileset.tilewidth = tsElement->IntAttribute("tilewidth", 0);
        tileset.tileheight = tsElement->IntAttribute("tileheight", 0);
        tileset.tilecount = tsElement->IntAttribute("tilecount", 0);
        tileset.columns = tsElement->IntAttribute("columns", 0);
        tileset.spacing = tsElement->IntAttribute("spacing", 0);
        tileset.margin = tsElement->IntAttribute("margin", 0);

        // Parse image element (for image-based tilesets)
        tinyxml2::XMLElement* imageElement = tsElement->FirstChildElement("image");
        if (imageElement) {
            tileset.image = imageElement->Attribute("source") ? imageElement->Attribute("source") : "";
            tileset.imagewidth = imageElement->IntAttribute("width", 0);
            tileset.imageheight = imageElement->IntAttribute("height", 0);
            const char* trans = imageElement->Attribute("trans");
            if (trans) {
                tileset.transparentcolor = std::string("#") + trans;
            }
        }

        // Parse tiles (for collection tilesets or tiles with properties)
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

            // Parse tile image (for collection tilesets)
            tinyxml2::XMLElement* tileImageElement = tileElement->FirstChildElement("image");
            if (tileImageElement) {
                tile.image = tileImageElement->Attribute("source") ? 
                            tileImageElement->Attribute("source") : "";
                tile.imagewidth = tileImageElement->IntAttribute("width", 0);
                tile.imageheight = tileImageElement->IntAttribute("height", 0);
            }

            // Parse properties
            tinyxml2::XMLElement* propertiesElement = tileElement->FirstChildElement("properties");
            if (propertiesElement) {
                ParsePropertiesFromXML(propertiesElement, tile.properties);
            }

            tileset.tiles.push_back(tile);
        }

        // Parse tileset properties
        tinyxml2::XMLElement* propertiesElement = tsElement->FirstChildElement("properties");
        if (propertiesElement) {
            ParsePropertiesFromXML(propertiesElement, tileset.properties);
        }

        return true;
    }

    bool TilesetParser::ParseTSJ(const std::string& filepath, TiledTileset& tileset)
    {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            SYSTEM_LOG << "TilesetParser: Failed to open TSJ file: " << filepath << std::endl;
            return false;
        }

        json j;
        try {
            file >> j;
        } catch (const std::exception& e) {
            SYSTEM_LOG << "TilesetParser: JSON parse error in " << filepath << ": " << e.what() << std::endl;
            return false;
        }

        // Parse tileset data
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
            for (const auto& tileJson : j["tiles"]) {
                TiledTile tile;
                ParseTileFromJSON(tileJson, tile);
                tileset.tiles.push_back(tile);
            }
        }

        // Parse properties
        if (HasKey(j, "properties")) {
            ParsePropertiesFromJSON(j["properties"], tileset.properties);
        }

        return true;
    }

    void TilesetParser::ParsePropertiesFromXML(void* element, std::map<std::string, TiledProperty>& properties)
    {
        tinyxml2::XMLElement* propsElement = static_cast<tinyxml2::XMLElement*>(element);
        
        for (tinyxml2::XMLElement* propElement = propsElement->FirstChildElement("property");
             propElement != nullptr;
             propElement = propElement->NextSiblingElement("property"))
        {
            TiledProperty prop;
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

            properties[prop.name] = prop;
        }
    }

    void TilesetParser::ParsePropertiesFromJSON(const json& j, std::map<std::string, TiledProperty>& properties)
    {
        if (!j.is_array()) return;

        for (const auto& propJson : j) {
            TiledProperty prop;
            prop.name = GetString(propJson, "name");
            
            std::string typeStr = GetString(propJson, "type", "string");
            
            if (typeStr == "int") {
                prop.type = PropertyType::Int;
                prop.intValue = GetInt(propJson, "value");
            }
            else if (typeStr == "float") {
                prop.type = PropertyType::Float;
                prop.floatValue = GetFloat(propJson, "value");
            }
            else if (typeStr == "bool") {
                prop.type = PropertyType::Bool;
                prop.boolValue = GetBool(propJson, "value");
            }
            else if (typeStr == "color") {
                prop.type = PropertyType::Color;
                prop.stringValue = GetString(propJson, "value");
            }
            else if (typeStr == "file") {
                prop.type = PropertyType::File;
                prop.stringValue = GetString(propJson, "value");
            }
            else {
                prop.type = PropertyType::String;
                prop.stringValue = GetString(propJson, "value");
            }

            properties[prop.name] = prop;
        }
    }

    void TilesetParser::ParseTileFromXML(void* element, TiledTile& tile)
    {
        // Not implemented - handled inline in ParseTSX
    }

    void TilesetParser::ParseTileFromJSON(const json& j, TiledTile& tile)
    {
        tile.id = GetInt(j, "id");
        tile.type = GetString(j, "type");
        tile.image = GetString(j, "image");
        tile.imagewidth = GetInt(j, "imagewidth");
        tile.imageheight = GetInt(j, "imageheight");

        if (HasKey(j, "properties")) {
            ParsePropertiesFromJSON(j["properties"], tile.properties);
        }
    }

} // namespace Tiled
} // namespace Olympe
