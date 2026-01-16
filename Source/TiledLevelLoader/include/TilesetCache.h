/*
 * TilesetCache.h - Cache for loaded external tilesets
 * 
 * Singleton cache that stores loaded tilesets to avoid
 * re-parsing the same tileset file multiple times.
 * Useful when multiple maps reference the same tileset.
 */

#pragma once

#include "TiledStructures.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>

namespace Olympe {
namespace Tiled {

    class TilesetCache
    {
    public:
        // Get singleton instance
        static TilesetCache& GetInstance();

        // Get tileset from cache or load if not cached
        // Returns nullptr on failure
        std::shared_ptr<TiledTileset> GetTileset(const std::string& filepath);

        // Add tileset to cache
        void AddTileset(const std::string& filepath, std::shared_ptr<TiledTileset> tileset);

        // Clear all cached tilesets
        void Clear();

        // Check if tileset is cached
        bool HasTileset(const std::string& filepath) const;

    private:
        TilesetCache() = default;
        ~TilesetCache() = default;
        TilesetCache(const TilesetCache&) = delete;
        TilesetCache& operator=(const TilesetCache&) = delete;

        std::map<std::string, std::shared_ptr<TiledTileset>> cache_;
        mutable std::mutex mutex_;
    };

} // namespace Tiled
} // namespace Olympe
