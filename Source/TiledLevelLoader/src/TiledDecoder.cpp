/*
 * TiledDecoder.cpp - Base64 and compression implementation
 */

#include "../include/TiledDecoder.h"
#include "../../system/system_utils.h"
#include "../../TiledLevelLoader/third_party/miniz/miniz.h"
#include <sstream>
#include <cctype>

namespace Olympe {
namespace Tiled {

    const std::string TiledDecoder::base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::vector<uint8_t> TiledDecoder::DecodeBase64(const std::string& encoded)
    {
        std::vector<uint8_t> result;
        
        // Remove whitespace
        std::string cleaned;
        cleaned.reserve(encoded.size());
        for (char c : encoded) {
            if (!isspace(static_cast<unsigned char>(c))) {
                cleaned += c;
            }
        }

        int in_len = static_cast<int>(cleaned.size());
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];

        while (in_len-- && (cleaned[in_] != '=') && IsBase64(cleaned[in_])) {
            char_array_4[i++] = cleaned[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    result.push_back(char_array_3[i]);
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++)
                result.push_back(char_array_3[j]);
        }

        return result;
    }

    std::vector<uint8_t> TiledDecoder::DecompressGzip(const std::vector<uint8_t>& compressed)
    {
        if (compressed.empty()) {
            SYSTEM_LOG << "TiledDecoder: ERROR - Empty input for gzip decompression" << std::endl;
            return std::vector<uint8_t>();
        }

        // Estimate output size (assume 10x compression ratio)
        size_t estimated_size = compressed.size() * 10;
        std::vector<uint8_t> result(estimated_size);

        mz_ulong dest_len = static_cast<mz_ulong>(estimated_size);
        int status = mz_uncompress(result.data(), &dest_len, compressed.data(), 
                                   static_cast<mz_ulong>(compressed.size()));

        if (status == MZ_BUF_ERROR) {
            // Buffer too small, resize to 50 times compressed size to accommodate higher compression ratios
            estimated_size = compressed.size() * 50;
            result.resize(estimated_size);
            dest_len = static_cast<mz_ulong>(estimated_size);
            status = mz_uncompress(result.data(), &dest_len, compressed.data(),
                                  static_cast<mz_ulong>(compressed.size()));
        }

        if (status != MZ_OK) {
            const char* errorStr = "unknown error";
            switch (status) {
                case MZ_STREAM_ERROR: errorStr = "stream error (invalid parameters)"; break;
                case MZ_DATA_ERROR: errorStr = "data error (corrupted or incomplete data)"; break;
                case MZ_MEM_ERROR: errorStr = "memory error (out of memory)"; break;
                case MZ_BUF_ERROR: errorStr = "buffer error (output buffer too small)"; break;
            }
            SYSTEM_LOG << "TiledDecoder: ERROR - Gzip decompression failed"
                      << "\n  Error code: " << status << " (" << errorStr << ")"
                      << "\n  Input size: " << compressed.size() << " bytes"
                      << "\n  This indicates corrupted, truncated, or invalid gzip data"
                      << std::endl;
            return std::vector<uint8_t>();
        }

        result.resize(dest_len);
        return result;
    }

    std::vector<uint8_t> TiledDecoder::DecompressZlib(const std::vector<uint8_t>& compressed)
    {
        // zlib and gzip use the same decompression in miniz
        return DecompressGzip(compressed);
    }

    std::vector<uint32_t> TiledDecoder::BytesToTileIds(const std::vector<uint8_t>& bytes)
    {
        std::vector<uint32_t> result;
        
        // ====================================================================
        // CRITICAL VALIDATION: Verify byte array size is multiple of 4
        // Each tile ID requires exactly 4 bytes (uint32_t little-endian)
        // ====================================================================
        if (bytes.size() % 4 != 0) {
            SYSTEM_LOG << "TiledDecoder: ERROR - Byte array size (" << bytes.size() 
                      << ") is not a multiple of 4" 
                      << "\n  This indicates corrupted or truncated tile data"
                      << "\n  Each tile ID requires exactly 4 bytes"
                      << "\n  Missing bytes: " << (4 - (bytes.size() % 4))
                      << std::endl;
            return result;
        }

        result.reserve(bytes.size() / 4);

        for (size_t i = 0; i < bytes.size(); i += 4) {
            // Little-endian conversion
            uint32_t tileId = 
                static_cast<uint32_t>(bytes[i]) |
                (static_cast<uint32_t>(bytes[i + 1]) << 8) |
                (static_cast<uint32_t>(bytes[i + 2]) << 16) |
                (static_cast<uint32_t>(bytes[i + 3]) << 24);
            result.push_back(tileId);
        }

        return result;
    }

    std::vector<uint32_t> TiledDecoder::ParseCSV(const std::string& csv)
    {
        std::vector<uint32_t> result;
        std::istringstream stream(csv);
        std::string token;
        int tokenCount = 0;
        int errorCount = 0;

        while (std::getline(stream, token, ',')) {
            ++tokenCount;
            // Remove whitespace
            token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
            token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);
            
            if (!token.empty()) {
                try {
                    uint32_t value = static_cast<uint32_t>(std::stoul(token));
                    result.push_back(value);
                } catch (const std::invalid_argument& e) {
                    ++errorCount;
                    SYSTEM_LOG << "TiledDecoder: ERROR - Invalid CSV token at position " << tokenCount 
                              << ": '" << token << "' (not a valid number). Error : " << e.what() << std::endl;
                } catch (const std::out_of_range& e) {
                    ++errorCount;
                    SYSTEM_LOG << "TiledDecoder: ERROR - CSV token out of range at position " << tokenCount 
                              << ": '" << token << "' (exceeds uint32_t maximum). Error " << e.what() << std::endl;
                } catch (...) {
                    ++errorCount;
                    SYSTEM_LOG << "TiledDecoder: ERROR - Failed to parse CSV token at position " << tokenCount 
                              << ": '" << token << "'" << std::endl;
                }
            }
        }
        
        if (errorCount > 0) {
            SYSTEM_LOG << "TiledDecoder: WARNING - Parsed " << result.size() << " valid tiles from " 
                      << tokenCount << " tokens with " << errorCount << " errors" << std::endl;
        }

        return result;
    }

    std::vector<uint32_t> TiledDecoder::DecodeTileData(
        const std::string& data,
        const std::string& encoding,
        const std::string& compression)
    {
        if (encoding == "csv") {
            return ParseCSV(data);
        }
        else if (encoding == "base64") {
            // Decode base64
            std::vector<uint8_t> decoded = DecodeBase64(data);
            if (decoded.empty()) {
                SYSTEM_LOG << "TiledDecoder: ERROR - Base64 decode failed"
                          << "\n  Input data length: " << data.length() << " characters"
                          << "\n  This may indicate corrupted or invalid base64 data"
                          << std::endl;
                return std::vector<uint32_t>();
            }

            SYSTEM_LOG << "TiledDecoder: Base64 decoded " << decoded.size() << " bytes" << std::endl;

            // Decompress if needed
            if (compression == "gzip") {
                size_t compressedSize = decoded.size();
                decoded = DecompressGzip(decoded);
                if (decoded.empty()) {
                    SYSTEM_LOG << "TiledDecoder: ERROR - Gzip decompression failed"
                              << "\n  Compressed size: " << compressedSize << " bytes"
                              << "\n  This may indicate corrupted compression data"
                              << std::endl;
                    return std::vector<uint32_t>();
                }
                SYSTEM_LOG << "TiledDecoder: Gzip decompressed from " << compressedSize 
                          << " to " << decoded.size() << " bytes" << std::endl;
            }
            else if (compression == "zlib") {
                size_t compressedSize = decoded.size();
                decoded = DecompressZlib(decoded);
                if (decoded.empty()) {
                    SYSTEM_LOG << "TiledDecoder: ERROR - Zlib decompression failed"
                              << "\n  Compressed size: " << compressedSize << " bytes"
                              << "\n  This may indicate corrupted compression data"
                              << std::endl;
                    return std::vector<uint32_t>();
                }
                SYSTEM_LOG << "TiledDecoder: Zlib decompressed from " << compressedSize 
                          << " to " << decoded.size() << " bytes" << std::endl;
            }

            // Convert to tile IDs
            return BytesToTileIds(decoded);
        }
        else {
            SYSTEM_LOG << "TiledDecoder: ERROR - Unsupported encoding: '" << encoding << "'"
                      << "\n  Supported encodings: 'csv', 'base64'"
                      << std::endl;
            return std::vector<uint32_t>();
        }
    }

} // namespace Tiled
} // namespace Olympe
