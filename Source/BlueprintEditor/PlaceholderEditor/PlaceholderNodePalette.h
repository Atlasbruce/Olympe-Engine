#pragma once

#include <string>
#include <vector>
#include "../../third_party/imgui/imgui.h"

namespace Olympe {

enum class PlaceholderNodeType;

/**
 * @class PlaceholderNodePalette
 * @brief Drag-drop palette for placeholder node types
 *
 * Displays 3 node types (Blue, Green, Magenta) as draggable items.
 * Used for Phase 64 node creation via drag-drop.
 *
 * Features:
 * - Searchable component list
 * - ImGui drag-source setup
 * - Category support for future expansion
 * - Tooltip descriptions
 *
 * C++14 compliant
 */
class PlaceholderNodePalette
{
public:
    PlaceholderNodePalette();
    ~PlaceholderNodePalette();

    /// Render palette UI (call from containing panel)
    void Render();

    /// Get currently hovered component type (-1 if none)
    int GetHoveredComponentType() const { return m_hoveredType; }

    /// Get palette size for layout calculations
    float GetPreferredWidth() const { return 200.0f; }

private:
    struct PaletteItem
    {
        std::string name;
        PlaceholderNodeType type;
        std::string category;
        std::string description;
    };

    std::vector<PaletteItem> m_items;
    int m_hoveredType;
    std::string m_searchFilter;
    int m_selectedCategory;

    /// Initialize palette with 3 node types
    void Initialize();

    /// Render search bar
    void RenderSearchBar();

    /// Render component list
    void RenderComponentList();

    /// Setup drag-source for component
    void SetupDragSource(const PaletteItem& item);
};

} // namespace Olympe
