/*
Olympe Engine V2 - 2026
Nicolas Chereau
nchereau@gmail.com

purpose:

*/



#pragma once
#include <cstdint>

// TMX/TMJ tile flip flags (bits 29-31)
// Reference: https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#tile-flipping
constexpr uint32_t TILE_FLIP_HORIZONTAL = 0x80000000;
constexpr uint32_t TILE_FLIP_VERTICAL = 0x40000000;
constexpr uint32_t TILE_FLIP_DIAGONAL = 0x20000000;
constexpr uint32_t TILE_FLIP_FLAGS_MASK = 0xE0000000;

// Helper function
inline uint32_t GetTileGID(uint32_t gid) {
    return gid & ~TILE_FLIP_FLAGS_MASK;
}