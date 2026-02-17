/*
 * TiledDecoder.h - Base64 and compression utilities for Tiled tile data
 * 
 * Handles decoding of tile layer data encoded as:
 * - Base64 strings (with optional gzip/zlib compression)
 * - CSV arrays
 * 
 * Extracts tile IDs and flip flags from encoded data.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

#define MINIZ_NO_DLLIMPORT

namespace Olympe {
namespace Tiled {

    class TiledDecoder
    {
    public:
        // Decode base64 string to raw bytes
        static std::vector<uint8_t> DecodeBase64(const std::string& encoded);

        // Decompress gzip data
        static std::vector<uint8_t> DecompressGzip(const std::vector<uint8_t>& compressed);

        // Decompress zlib data
        static std::vector<uint8_t> DecompressZlib(const std::vector<uint8_t>& compressed);

        // Convert raw bytes to uint32 tile IDs (little-endian)
        static std::vector<uint32_t> BytesToTileIds(const std::vector<uint8_t>& bytes);

        // Parse CSV string to tile IDs
        static std::vector<uint32_t> ParseCSV(const std::string& csv);

        // Complete decode pipeline: base64 -> decompress -> tile IDs
        static std::vector<uint32_t> DecodeTileData(
            const std::string& data,
            const std::string& encoding,
            const std::string& compression);

    private:
        static const std::string base64_chars;
        static inline bool IsBase64(unsigned char c)
        {
            return (isalnum(c) || (c == '+') || (c == '/'));
        }
    };

} // namespace Tiled
} // namespace Olympe
