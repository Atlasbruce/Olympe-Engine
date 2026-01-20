/*
 * LevelParser.h - Phase 1: Parsing & Visual Analysis for Level Loading
 * 
 * Responsible for parsing .tmj files and extracting:
 * - Visual resource manifests (tilesets, image layers, sprites)
 * - Object census (object types, counts, templates)
 * - Object references (patrol paths, trigger zones, etc.)
 * 
 * Part of the 3-Phase Level Loading System:
 * Phase 1: Parsing & Visual Analysis (this file)
 * Phase 2: Prefab Discovery & Preloading
 * Phase 3: Instantiation Pipeline
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

namespace Olympe {
namespace Tiled {

    // Forward declarations
    struct TiledMap;

    // Visual resource manifest extracted from level
    struct VisualResourceManifest
    {
        struct TilesetRef
        {
            std::string sourceFile;      // .tsj file path
            std::string imageFile;       // Main tileset image
            std::vector<std::string> individualImages;  // For collection tilesets
            bool isCollection;
            int firstGid;
            
            TilesetRef() : isCollection(false), firstGid(0) {}
        };
        
        std::vector<TilesetRef> tilesets;
        std::vector<std::string> parallaxLayers;  // Image layer paths
        std::set<std::string> allImagePaths;      // Deduplicated set of all images
        
        int GetTotalImageCount() const { return static_cast<int>(allImagePaths.size()); }
        int GetTilesetCount() const { return static_cast<int>(tilesets.size()); }
        int GetParallaxLayerCount() const { return static_cast<int>(parallaxLayers.size()); }
    };

    // Census of objects in the level (for prefab discovery)
    struct ObjectTypeCensus
    {
        std::map<std::string, int> typeCounts;     // type -> count
        std::set<std::string> uniqueTypes;          // All unique types
        std::map<std::string, std::string> templates; // template name -> template path
        
        int GetTotalObjectCount() const
        {
            int total = 0;
            for (const auto& kv : typeCounts) total += kv.second;
            return total;
        }
        
        int GetUniqueTypeCount() const { return static_cast<int>(uniqueTypes.size()); }
        
        bool HasType(const std::string& type) const
        {
            return uniqueTypes.find(type) != uniqueTypes.end();
        }
    };

    // Object reference (for patrol paths, triggers, etc.)
    struct ObjectReference
    {
        int sourceObjectId;          // ID of object referencing another
        std::string sourceObjectName;
        int targetObjectId;          // ID of referenced object
        std::string targetObjectName;
        std::string referenceType;   // "patrol", "trigger", "target", etc.
        
        ObjectReference()
            : sourceObjectId(0), targetObjectId(0) {}
    };

    // Result of parsing and analysis
    struct LevelParseResult
    {
        bool success;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        // Map metadata
        std::string orientation;     // "orthogonal", "isometric", etc.
        int width;
        int height;
        int tileWidth;
        int tileHeight;
        
        // Extracted manifests
        VisualResourceManifest visualManifest;
        ObjectTypeCensus objectCensus;
        std::vector<ObjectReference> objectReferences;
        
        LevelParseResult() 
            : success(false), width(0), height(0), tileWidth(0), tileHeight(0) {}
        
        bool IsSuccess() const { return success && errors.empty(); }
        bool HasWarnings() const { return !warnings.empty(); }
        int GetErrorCount() const { return static_cast<int>(errors.size()); }
        int GetWarningCount() const { return static_cast<int>(warnings.size()); }
    };

    // LevelParser: Phase 1 of level loading
    class LevelParser
    {
    public:
        LevelParser();
        ~LevelParser();
        
        // Main entry point: Parse and analyze a Tiled map file
        LevelParseResult ParseAndAnalyze(const std::string& levelPath);
        
    private:
        // Extract visual resources from map
        void ExtractVisualResources(const TiledMap& map, VisualResourceManifest& manifest);
        
        // Build object census from all object layers
        void BuildObjectCensus(const TiledMap& map, ObjectTypeCensus& census);
        
        // Extract object references (patrol paths, etc.)
        void ExtractObjectReferences(const TiledMap& map, std::vector<ObjectReference>& references);
        
        // Helper: Get directory from file path
        std::string GetDirectory(const std::string& filepath);
        
        // Helper: Resolve relative path
        std::string ResolvePath(const std::string& baseDir, const std::string& relativePath);
    };

} // namespace Tiled
} // namespace Olympe
