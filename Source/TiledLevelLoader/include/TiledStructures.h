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
#include <functional>
#include <ostream> // For std::ostream

namespace Olympe {
namespace Tiled {

    // Flip flags for tile data (used by structures below)
    constexpr uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    constexpr uint32_t FLIPPED_VERTICALLY_FLAG   = 0x40000000;
    constexpr uint32_t FLIPPED_DIAGONALLY_FLAG   = 0x20000000;
    constexpr uint32_t TILE_ID_MASK              = 0x1FFFFFFF;

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
        int lastgid;        // Last global tile ID (calculated from firstgid + tilecount - 1)
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
            : firstgid(0), lastgid(0), tilewidth(0), tileheight(0), tilecount(0),
              columns(0), spacing(0), margin(0), tileoffsetX(0), tileoffsetY(0),
              imagewidth(0), imageheight(0) {}
              
        // Calculate lastgid from tileset parameters
        void CalculateLastGid()
        {
            if (tilecount > 0) {
                lastgid = firstgid + tilecount - 1;
            } else if (imagewidth > 0 && imageheight > 0 && tilewidth > 0 && tileheight > 0) {
                // Calculate from image dimensions
                int cols = (imagewidth - margin * 2 + spacing) / (tilewidth + spacing);
                int rows = (imageheight - margin * 2 + spacing) / (tileheight + spacing);
                int calculatedTilecount = cols * rows;
                lastgid = firstgid + calculatedTilecount - 1;
            } else {
                lastgid = firstgid;
            }
        }
        
        // Check if a GID belongs to this tileset
        bool ContainsGid(uint32_t gid) const
        {
            uint32_t cleanGid = gid & TILE_ID_MASK;
            return cleanGid >= static_cast<uint32_t>(firstgid) && 
                   cleanGid <= static_cast<uint32_t>(lastgid);
        }
        
        // Get local tile ID from global ID
        int GetLocalId(uint32_t gid) const
        {
            uint32_t cleanGid = gid & TILE_ID_MASK;
            return static_cast<int>(cleanGid) - firstgid;
        }
        
        // Get tile coordinates in atlas (for image-based tilesets)
        void GetTileCoords(uint32_t gid, int& tileX, int& tileY) const
        {
            int localId = GetLocalId(gid);
            if (columns > 0) {
                tileX = localId % columns;
                tileY = localId / columns;
            } else {
                tileX = 0;
                tileY = 0;
            }
        }
    };

    // Map orientations
    enum class MapOrientation
    {
        Orthogonal,
        Isometric,
        Staggered,
        Hexagonal
    };

    // Stream operator for MapOrientation (for logging)
    inline std::ostream& operator<<(std::ostream& os, MapOrientation orientation)
    {
        switch (orientation)
        {
            case MapOrientation::Orthogonal: return os << "Orthogonal";
            case MapOrientation::Isometric: return os << "Isometric";
            case MapOrientation::Staggered: return os << "Staggered";
            case MapOrientation::Hexagonal: return os << "Hexagonal";
            default: return os << "Unknown";
        }
    }

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
              
        // GID resolver: Find tileset for a given GID
        // Returns pointer to tileset or nullptr if not found
        const TiledTileset* FindTilesetForGid(uint32_t gid) const
        {
            uint32_t cleanGid = gid & TILE_ID_MASK;
            if (cleanGid == 0) return nullptr; // GID 0 is empty tile
            
            // Tilesets are stored in order, find the one containing this GID
            for (const auto& tileset : tilesets) {
                if (tileset.ContainsGid(cleanGid)) {
                    return &tileset;
                }
            }
            return nullptr;
        }
        
        // Non-const version
        TiledTileset* FindTilesetForGid(uint32_t gid)
        {
            uint32_t cleanGid = gid & TILE_ID_MASK;
            if (cleanGid == 0) return nullptr; // GID 0 is empty tile
            
            // Tilesets are stored in order, find the one containing this GID
            for (auto& tileset : tilesets) {
                if (tileset.ContainsGid(cleanGid)) {
                    return &tileset;
                }
            }
            return nullptr;
        }
        
        // Helper to initialize all tilesets lastgid values
        void CalculateAllLastGids()
        {
            for (auto& tileset : tilesets) {
                tileset.CalculateLastGid();
            }
        }
    };
    
    // Resolved GID information
    struct ResolvedGid
    {
        const TiledTileset* tileset; // Pointer to tileset (nullptr if not found)
        int localId;                 // Local tile ID within tileset
        int tileX;                   // X coordinate in atlas
        int tileY;                   // Y coordinate in atlas
        bool flipH;                  // Horizontal flip flag
        bool flipV;                  // Vertical flip flag
        bool flipD;                  // Diagonal flip flag
        
        ResolvedGid()
            : tileset(nullptr), localId(-1), tileX(0), tileY(0),
              flipH(false), flipV(false), flipD(false) {}
              
        bool IsValid() const { return tileset != nullptr && localId >= 0; }
    };

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
    
    // Comprehensive GID resolver
    // Resolves a GID to its tileset, local ID, and atlas coordinates
    inline ResolvedGid ResolveGid(const TiledMap& map, uint32_t gid)
    {
        ResolvedGid result;
        
        // Extract flip flags
        result.flipH = IsFlippedHorizontally(gid);
        result.flipV = IsFlippedVertically(gid);
        result.flipD = IsFlippedDiagonally(gid);
        
        // Clean GID
        uint32_t cleanGid = gid & TILE_ID_MASK;
        
        // GID 0 is empty tile
        if (cleanGid == 0) {
            return result;
        }
        
        // Find tileset
        result.tileset = map.FindTilesetForGid(cleanGid);
        if (result.tileset == nullptr) {
            return result;
        }
        
        // Calculate local ID and atlas coordinates
        result.localId = result.tileset->GetLocalId(cleanGid);
        result.tileset->GetTileCoords(cleanGid, result.tileX, result.tileY);
        
        return result;
    }
    
    // Helper to get all image paths from a map (for preloading validation)
    // Returns a vector of image file paths from all tilesets and image layers
    // NOTE: The runtime must ensure these images are loaded before rendering
    inline std::vector<std::string> GetAllImagePaths(const TiledMap& map)
    {
        std::vector<std::string> imagePaths;
        
        // Collect tileset images
        for (const auto& tileset : map.tilesets) {
            // Main tileset image
            if (!tileset.image.empty()) {
                imagePaths.push_back(tileset.image);
            }
            
            // Collection tileset individual images
            for (const auto& tile : tileset.tiles) {
                if (!tile.image.empty()) {
                    imagePaths.push_back(tile.image);
                }
            }
        }
        
        // Collect image layer paths (recursive)
        std::function<void(const std::shared_ptr<TiledLayer>&)> processLayer;
        processLayer = [&](const std::shared_ptr<TiledLayer>& layer) {
            if (!layer) return;
            
            if (layer->type == LayerType::ImageLayer && !layer->image.empty()) {
                imagePaths.push_back(layer->image);
            }
            
            // Recursively process group layers
            if (layer->type == LayerType::Group) {
                for (const auto& childLayer : layer->layers) {
                    processLayer(childLayer);
                }
            }
        };
        
        for (const auto& layer : map.layers) {
            processLayer(layer);
        }
        
        return imagePaths;
    }

} // namespace Tiled
} // namespace Olymp
