#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Olympe
{
    enum class GridProjection : uint8_t
    {
        Orthographic = 0,
        Isometric    = 1,
        HexAxial     = 2
    };

    enum class GridSortMode : uint8_t
    {
        None = 0,
        Y    = 1,
        Iso  = 2,
        Hex  = 3
    };

    struct GridSettings_data
    {
        bool enabled{false};
        GridProjection projection{GridProjection::Orthographic};

        glm::vec2 cellSize{32.0f, 32.0f};
        float hexRadius{16.0f};

        glm::vec4 color{0.2f, 0.8f, 0.2f, 1.0f};
        float lineThickness{1.0f};
        int32_t maxLines{256};

        bool cullToCamera{true};

        bool drawBehindScene{true};
        bool drawOverlay{false};

        GridSortMode sortMode{GridSortMode::None};
    };
}
