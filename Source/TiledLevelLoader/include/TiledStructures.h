/*
 * TiledStructures.h - Data structures mirroring Tiled MapEditor JSON format
 * 
 * Complete representation of Tiled .tmj/.tsj file formats including:
 * - Maps (orthogonal, isometric, infinite, chunked)
 * - Layers (tilelayer, objectgroup, imagelayer, group)
 * - Objects (rectangle, ellipse, point, polygon, polyline)
 * - Tilesets (embedded, external, image-based, collection)
 * - Properties (custom properties of all types)
 * - Parallax scrolling support
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace Olympe {
namespace Tiled {

    // Forward declarations
    struct TiledProperty;
    struct TiledChunk;
    struct TiledLayer;
    struct TiledObject;
    struct TiledTile;
    struct TiledTileset;
    struct TiledMap;

    // Property value types
    enum class PropertyType
    {
        String,
        Int,
        Float,
        Bool,
        Color,
        File,
        Object
    };

    // Property structure
    struct TiledProperty
    {
        std::string name;
        PropertyType type;
        std::string stringValue;
        int intValue;
        float floatValue;
        bool boolValue;

        TiledProperty()
            : type(PropertyType::String), intValue(0), floatValue(0.0f), boolValue(false) {}
    };

    // Chunk for infinite maps
    struct TiledChunk
    {
        int x;              // Chunk X position (in tiles)
        int y;              // Chunk Y position (in tiles)
        int width;          // Chunk width (in tiles)
        int height;         // Chunk height (in tiles)
        std::vector<uint32_t> data;  // Tile IDs (with flip flags)

        TiledChunk()
            : x(0), y(0), width(0), height(0) {}
    };

    // Layer types
    enum class LayerType
    {
        TileLayer,
        ObjectGroup,
        ImageLayer,
        Group
    };

    // Object types for object layers
    enum class ObjectType
    {
        Rectangle,
        Ellipse,
        Point,
        Polygon,
        Polyline,
        Text
    };

    // Point for polygons/polylines
    struct Point
    {
        float x;
        float y;

        Point() : x(0.0f), y(0.0f) {}
        Point(float x_, float y_) : x(x_), y(y_) {}
    };

    // Object in object layer
    struct TiledObject
    {
        int id;
        std::string name;
        std::string type;
        ObjectType objectType;
        float x;
        float y;
        float width;
        float height;
        float rotation;
        int gid;                    // Tile ID for tile objects
        bool visible;
        std::vector<Point> polygon;
        std::vector<Point> polyline;
        std::string text;
        std::map<std::string, TiledProperty> properties;

        TiledObject()
            : id(0), objectType(ObjectType::Rectangle),
              x(0.0f), y(0.0f), width(0.0f), height(0.0f),
              rotation(0.0f), gid(0), visible(true) {}
    };

    // Layer structure
    struct TiledLayer
    {
        int id;
        std::string name;
        LayerType type;
        bool visible;
        float opacity;
        float offsetx;
        float offsety;
        float parallaxx;    // Parallax scroll factor X (1.0 = normal)
        float parallaxy;    // Parallax scroll factor Y (1.0 = normal)
        int tintcolor;      // ARGB color
        
        // Tile layer data
        int width;
        int height;
        int startx;         // Starting X tile position offset (for infinite maps)
        int starty;         // Starting Y tile position offset (for infinite maps)
        std::vector<uint32_t> data;  // Tile IDs (for finite maps)
        std::vector<TiledChunk> chunks;  // Chunks (for infinite maps)
        std::string encoding;        // "csv", "base64"
        std::string compression;     // "gzip", "zlib", "" (none)
        
        // Object layer data
        std::vector<TiledObject> objects;
        
        // Image layer data
        std::string image;
        bool repeatx;
        bool repeaty;
        
        // Group layer data
        std::vector<std::shared_ptr<TiledLayer>> layers;
        
        // Properties
        std::map<std::string, TiledProperty> properties;

        TiledLayer()
            : id(0), type(LayerType::TileLayer), visible(true),
              opacity(1.0f), offsetx(0.0f), offsety(0.0f),
              parallaxx(1.0f), parallaxy(1.0f), tintcolor(0xFFFFFFFF),
              width(0), height(0), startx(0), starty(0),
              repeatx(false), repeaty(false) {}
    };

    // Individual tile in tileset (for tile-specific properties)
    struct TiledTile
    {
        int id;             // Local tile ID (0-based)
        std::string image;  // For collection of images tilesets
        int imagewidth;
        int imageheight;
        std::string type;   // Tile type/class
        std::map<std::string, TiledProperty> properties;

        TiledTile()
            : id(0), imagewidth(0), imageheight(0) {}
    };

    // Tileset structure
    struct TiledTileset
    {
        int firstgid;       // First global tile ID
        std::string name;
        std::string source; // External tileset file path (.tsx or .tsj)
        
        // For embedded or loaded external tilesets
        int tilewidth;
        int tileheight;
        int tilecount;
        int columns;
        int spacing;
        int margin;
        
        // ====================================================================
        // CRITICAL: Global tile offset from tileset definition
        // Parsed from .tsx <tileoffset> or .tsj "tileoffset" property
        // ALL tiles in this tileset inherit these values
        // ====================================================================
        int tileoffsetX;
        int tileoffsetY;
        
        std::string image;  // Single image file (for image-based tilesets)
        int imagewidth;
        int imageheight;
        std::string transparentcolor;
        
        // Collection of images tileset
        std::vector<TiledTile> tiles;
        
        // Properties
        std::map<std::string, TiledProperty> properties;

        // Constructor with explicit default values
        TiledTileset()
            : firstgid(0), tilewidth(0), tileheight(0), tilecount(0),
              columns(0), spacing(0), margin(0), tileoffsetX(0), tileoffsetY(0),
              imagewidth(0), imageheight(0) {}
    };

    // Map orientations
    enum class MapOrientation
    {
        Orthogonal,
        Isometric,
        Staggered,
        Hexagonal
    };

    // Render order
    enum class RenderOrder
    {
        RightDown,
        RightUp,
        LeftDown,
        LeftUp
    };

    // Main map structure
    struct TiledMap
    {
        int version;        // Tiled version
        std::string tiledversion;
        std::string type;   // "map"
        MapOrientation orientation;
        RenderOrder renderorder;
        int compressionlevel;
        
        int width;          // Map width in tiles
        int height;         // Map height in tiles
        int tilewidth;      // Tile width in pixels
        int tileheight;     // Tile height in pixels
        
        bool infinite;      // Whether map uses chunks
        
        std::string backgroundcolor;
        int nextlayerid;
        int nextobjectid;
        
        std::vector<std::shared_ptr<TiledLayer>> layers;
        std::vector<TiledTileset> tilesets;
        std::map<std::string, TiledProperty> properties;

        TiledMap()
            : version(1), type("map"), orientation(MapOrientation::Orthogonal),
              renderorder(RenderOrder::RightDown), compressionlevel(-1),
              width(0), height(0), tilewidth(0), tileheight(0),
              infinite(false), nextlayerid(1), nextobjectid(1) {}
    };

    // Flip flags for tile data
    constexpr uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    constexpr uint32_t FLIPPED_VERTICALLY_FLAG   = 0x40000000;
    constexpr uint32_t FLIPPED_DIAGONALLY_FLAG   = 0x20000000;
    constexpr uint32_t TILE_ID_MASK              = 0x1FFFFFFF;

    // Helper to extract tile ID and flip flags
    inline uint32_t GetTileId(uint32_t gid)
    {
        return gid & TILE_ID_MASK;
    }

    inline bool IsFlippedHorizontally(uint32_t gid)
    {
        return (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
    }

    inline bool IsFlippedVertically(uint32_t gid)
    {
        return (gid & FLIPPED_VERTICALLY_FLAG) != 0;
    }

    inline bool IsFlippedDiagonally(uint32_t gid)
    {
        return (gid & FLIPPED_DIAGONALLY_FLAG) != 0;
    }

} // namespace Tiled
} // namespace Olympe
