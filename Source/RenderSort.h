#pragma once

#include <cstdint>
#include <glm/glm.hpp>

#include "ECS_Grid.h"

namespace Olympe
{
    // 64-bit key for stable sorting. Higher bits: layer/camera, lower bits: position.
    inline uint64_t MakeSortKey_OrthoY(const glm::vec2& worldPos)
    {
        // Map Y to uint32; invert so larger Y renders earlier/later depending on convention.
        // Here: smaller Y renders first (back), larger Y last (front)
        const float y = worldPos.y;
        uint32_t uy = (uint32_t)(int32_t)std::clamp((int)(y * 100.0f), -0x7fffffff, 0x7fffffff);
        return uint64_t(uint32_t(uy)) << 0;
    }

    inline uint64_t MakeSortKey_Iso(const glm::vec2& worldPos)
    {
        // Common iso draw: sort by (x + y), then y
        float primary = worldPos.x + worldPos.y;
        float secondary = worldPos.y;
        uint32_t a = (uint32_t)(int32_t)std::clamp((int)(primary * 100.0f), -0x7fffffff, 0x7fffffff);
        uint32_t b = (uint32_t)(int32_t)std::clamp((int)(secondary * 100.0f), -0x7fffffff, 0x7fffffff);
        return (uint64_t(a) << 32) | uint64_t(b);
    }

    inline uint64_t MakeSortKey_Hex(const glm::vec2& worldPos)
    {
        // Approx: sort by y then x.
        float primary = worldPos.y;
        float secondary = worldPos.x;
        uint32_t a = (uint32_t)(int32_t)std::clamp((int)(primary * 100.0f), -0x7fffffff, 0x7fffffff);
        uint32_t b = (uint32_t)(int32_t)std::clamp((int)(secondary * 100.0f), -0x7fffffff, 0x7fffffff);
        return (uint64_t(a) << 32) | uint64_t(b);
    }

    inline uint64_t MakeGridSortKey(GridSortMode mode, const glm::vec2& worldPos)
    {
        switch (mode)
        {
        case GridSortMode::Y:   return MakeSortKey_OrthoY(worldPos);
        case GridSortMode::Iso: return MakeSortKey_Iso(worldPos);
        case GridSortMode::Hex: return MakeSortKey_Hex(worldPos);
        default: return 0;
        }
    }
}
