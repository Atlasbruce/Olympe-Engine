/*
 * LevelParser.cpp - Phase 1: Parsing & Visual Analysis Implementation
 * 
 * Implements comprehensive level parsing with visual resource extraction,
 * object census, and reference analysis.
 */

#include "../include/LevelParser.h"
#include "../include/TiledLevelLoader.h"
#include "../include/TiledStructures.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>

namespace Olympe {
namespace Tiled {

    // Helper function for box-drawing padding
    static std::string GetPadding(int totalWidth, const std::string& content)
    {
        int contentLen = static_cast<int>(content.length());
        int padding = totalWidth - contentLen;
        return std::string(padding > 0 ? padding : 0, ' ');
    }

    LevelParser::LevelParser()
    {
    }

    LevelParser::~LevelParser()
    {
    }

    LevelParseResult LevelParser::ParseAndAnalyze(const std::string& levelPath)
    {
        LevelParseResult result;
        
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║         PHASE 1: PARSING & VISUAL ANALYSIS                           ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ File: " << levelPath << std::string(std::max(0, 65 - static_cast<int>(levelPath.length())), ' ') << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

        // Step 1: Load the Tiled map using existing loader
        TiledMap tiledMap;
        TiledLevelLoader loader;
        
        std::cout << "→ Loading Tiled map file...\n";
        if (!loader.LoadFromFile(levelPath, tiledMap))
        {
            result.success = false;
            result.errors.push_back("Failed to load Tiled map: " + loader.GetLastError());
            std::cout << "✗ Failed to load map: " << loader.GetLastError() << "\n";
            return result;
        }
        std::cout << "✓ Map loaded successfully\n\n";

        // Step 2: Extract map metadata
        result.width = tiledMap.width;
        result.height = tiledMap.height;
        result.tileWidth = tiledMap.tilewidth;
        result.tileHeight = tiledMap.tileheight;
        
        switch (tiledMap.orientation)
        {
            case MapOrientation::Orthogonal: result.orientation = "orthogonal"; break;
            case MapOrientation::Isometric: result.orientation = "isometric"; break;
            case MapOrientation::Staggered: result.orientation = "staggered"; break;
            case MapOrientation::Hexagonal: result.orientation = "hexagonal"; break;
            default: result.orientation = "unknown"; break;
        }

        std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ MAP METADATA                                                         ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Dimensions:   " << result.width << " x " << result.height 
                  << " tiles (" << (result.width * result.tileWidth) << " x " 
                  << (result.height * result.tileHeight) << " pixels)" 
                  << std::string(std::max(0, 19 - static_cast<int>(std::to_string(result.width).length() + std::to_string(result.height).length())), ' ') << "║\n";
        std::cout << "║ Tile Size:    " << result.tileWidth << " x " << result.tileHeight << " pixels"
                  << std::string(std::max(0, 44 - static_cast<int>(std::to_string(result.tileWidth).length() + std::to_string(result.tileHeight).length())), ' ') << "║\n";
        std::cout << "║ Orientation:  " << result.orientation 
                  << std::string(54 - result.orientation.length(), ' ') << "║\n";
        std::cout << "║ Infinite:     " << (tiledMap.infinite ? "Yes" : "No ")
                  << std::string(53, ' ') << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

        // Step 3: Extract visual resources
        std::cout << "→ Extracting visual resource manifest...\n";
        ExtractVisualResources(tiledMap, result.visualManifest);
        
        std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ VISUAL RESOURCES                                                     ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Tilesets:        " << result.visualManifest.GetTilesetCount() 
                  << std::string(51 - std::to_string(result.visualManifest.GetTilesetCount()).length(), ' ') << "║\n";
        std::cout << "║ Parallax Layers: " << result.visualManifest.GetParallaxLayerCount()
                  << std::string(51 - std::to_string(result.visualManifest.GetParallaxLayerCount()).length(), ' ') << "║\n";
        std::cout << "║ Total Images:    " << result.visualManifest.GetTotalImageCount()
                  << std::string(51 - std::to_string(result.visualManifest.GetTotalImageCount()).length(), ' ') << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

        // Step 4: Build object census
        std::cout << "→ Building object census...\n";
        BuildObjectCensus(tiledMap, result.objectCensus);
        
        std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ OBJECT CENSUS                                                        ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Total Objects:   " << result.objectCensus.GetTotalObjectCount()
                  << std::string(51 - std::to_string(result.objectCensus.GetTotalObjectCount()).length(), ' ') << "║\n";
        std::cout << "║ Unique Types:    " << result.objectCensus.GetUniqueTypeCount()
                  << std::string(51 - std::to_string(result.objectCensus.GetUniqueTypeCount()).length(), ' ') << "║\n";
        
        if (!result.objectCensus.typeCounts.empty())
        {
            std::cout << "║                                                                      ║\n";
            std::cout << "║ Type Breakdown:                                                      ║\n";
            for (const auto& kv : result.objectCensus.typeCounts)
            {
                std::string line = "║   " + kv.first + ": " + std::to_string(kv.second);
                std::cout << line << std::string(70 - line.length(), ' ') << "║\n";
            }
        }
        std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

        // Step 5: Extract object references
        std::cout << "→ Extracting object references...\n";
        ExtractObjectReferences(tiledMap, result.objectReferences);
        
        if (!result.objectReferences.empty())
        {
            std::cout << "✓ Found " << result.objectReferences.size() << " object reference(s)\n\n";
        }

        result.success = true;
        
        std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ PHASE 1 COMPLETE                                                     ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Status:   ✓ SUCCESS                                                  ║\n";
        std::cout << "║ Errors:   " << result.GetErrorCount()
                  << std::string(59 - std::to_string(result.GetErrorCount()).length(), ' ') << "║\n";
        std::cout << "║ Warnings: " << result.GetWarningCount()
                  << std::string(59 - std::to_string(result.GetWarningCount()).length(), ' ') << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

        return result;
    }

    void LevelParser::ExtractVisualResources(const TiledMap& map, VisualResourceManifest& manifest)
    {
        // Extract tilesets
        for (const auto& tileset : map.tilesets)
        {
            VisualResourceManifest::TilesetRef ref;
            ref.sourceFile = tileset.source;
            ref.imageFile = tileset.image;
            ref.firstGid = tileset.firstgid;
            
            // Check if it's a collection tileset
            if (!tileset.tiles.empty() && tileset.image.empty())
            {
                ref.isCollection = true;
                for (const auto& tile : tileset.tiles)
                {
                    if (!tile.image.empty())
                    {
                        ref.individualImages.push_back(tile.image);
                        manifest.allImagePaths.insert(tile.image);
                    }
                }
            }
            else if (!tileset.image.empty())
            {
                ref.isCollection = false;
                manifest.allImagePaths.insert(tileset.image);
            }
            
            manifest.tilesets.push_back(ref);
        }
        
        // Extract image layers (parallax backgrounds)
        std::function<void(const std::shared_ptr<TiledLayer>&)> processLayer;
        processLayer = [&](const std::shared_ptr<TiledLayer>& layer)
        {
            if (!layer) return;
            
            if (layer->type == LayerType::ImageLayer && !layer->image.empty())
            {
                manifest.parallaxLayers.push_back(layer->image);
                manifest.allImagePaths.insert(layer->image);
            }
            
            // Recursively process group layers
            if (layer->type == LayerType::Group)
            {
                for (const auto& childLayer : layer->layers)
                {
                    processLayer(childLayer);
                }
            }
        };
        
        for (const auto& layer : map.layers)
        {
            processLayer(layer);
        }
    }

    void LevelParser::BuildObjectCensus(const TiledMap& map, ObjectTypeCensus& census)
    {
        std::function<void(const std::shared_ptr<TiledLayer>&)> processLayer;
        processLayer = [&](const std::shared_ptr<TiledLayer>& layer)
        {
            if (!layer) return;
            
            if (layer->type == LayerType::ObjectGroup)
            {
                for (const auto& obj : layer->objects)
                {
                    std::string type = obj.type;
                    if (type.empty()) type = "undefined";
                    
                    census.uniqueTypes.insert(type);
                    census.typeCounts[type]++;
                    
                    // Check for template reference
                    auto it = obj.properties.find("template");
                    if (it != obj.properties.end() && it->second.type == PropertyType::String)
                    {
                        census.templates[obj.name] = it->second.stringValue;
                    }
                }
            }
            
            // Recursively process group layers
            if (layer->type == LayerType::Group)
            {
                for (const auto& childLayer : layer->layers)
                {
                    processLayer(childLayer);
                }
            }
        };
        
        for (const auto& layer : map.layers)
        {
            processLayer(layer);
        }
    }

    void LevelParser::ExtractObjectReferences(const TiledMap& map, std::vector<ObjectReference>& references)
    {
        std::function<void(const std::shared_ptr<TiledLayer>&)> processLayer;
        processLayer = [&](const std::shared_ptr<TiledLayer>& layer)
        {
            if (!layer) return;
            
            if (layer->type == LayerType::ObjectGroup)
            {
                for (const auto& obj : layer->objects)
                {
                    // Look for custom properties that reference other objects
                    for (const auto& prop : obj.properties)
                    {
                        if (prop.first == "targetObject" || 
                            prop.first == "patrolPath" || 
                            prop.first == "linkedObject")
                        {
                            ObjectReference ref;
                            ref.sourceObjectId = obj.id;
                            ref.sourceObjectName = obj.name;
                            ref.referenceType = prop.first;
                            
                            if (prop.second.type == PropertyType::Int)
                            {
                                ref.targetObjectId = prop.second.intValue;
                            }
                            else if (prop.second.type == PropertyType::String)
                            {
                                ref.targetObjectName = prop.second.stringValue;
                            }
                            
                            references.push_back(ref);
                        }
                    }
                }
            }
            
            // Recursively process group layers
            if (layer->type == LayerType::Group)
            {
                for (const auto& childLayer : layer->layers)
                {
                    processLayer(childLayer);
                }
            }
        };
        
        for (const auto& layer : map.layers)
        {
            processLayer(layer);
        }
    }

    std::string LevelParser::GetDirectory(const std::string& filepath)
    {
        size_t pos = filepath.find_last_of("/\\");
        if (pos != std::string::npos)
        {
            return filepath.substr(0, pos);
        }
        return "";
    }

    std::string LevelParser::ResolvePath(const std::string& baseDir, const std::string& relativePath)
    {
        if (relativePath.empty()) return "";
        if (baseDir.empty()) return relativePath;
        
        // If relative path starts with / or \, it's already absolute
        if (relativePath[0] == '/' || relativePath[0] == '\\')
            return relativePath;
        
        return baseDir + "/" + relativePath;
    }

} // namespace Tiled
} // namespace Olympe
