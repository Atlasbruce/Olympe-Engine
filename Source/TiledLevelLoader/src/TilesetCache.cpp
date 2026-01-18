/*
 * TilesetCache.cpp - Implementation of tileset cache
 */

#include "../include/TilesetCache.h"
#include "../include/TilesetParser.h"
#include "../../system/system_utils.h"

namespace Olympe {
namespace Tiled {

    TilesetCache& TilesetCache::GetInstance()
    {
        static TilesetCache instance;
        return instance;
    }

    std::shared_ptr<TiledTileset> TilesetCache::GetTileset(const std::string& filepath)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already cached
        auto it = cache_.find(filepath);
        if (it != cache_.end()) {
            return it->second;
        }

        // Not cached, load it
        SYSTEM_LOG << "TilesetCache: Loading tileset from " << filepath << "\n";
        
        auto tileset = std::make_shared<TiledTileset>();
        TilesetParser parser;
        
        if (!parser.ParseFile(filepath, *tileset)) {
            SYSTEM_LOG << "TilesetCache: Failed to load tileset from " << filepath << "\n";
            return nullptr;
        }

        // Add to cache
        cache_[filepath] = tileset;
        return tileset;
    }

    void TilesetCache::AddTileset(const std::string& filepath, std::shared_ptr<TiledTileset> tileset)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[filepath] = tileset;
    }

    void TilesetCache::Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        SYSTEM_LOG << "TilesetCache: Cleared all cached tilesets" << std::endl;
    }

    bool TilesetCache::HasTileset(const std::string& filepath) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.find(filepath) != cache_.end();
    }

} // namespace Tiled
} // namespace Olympe
