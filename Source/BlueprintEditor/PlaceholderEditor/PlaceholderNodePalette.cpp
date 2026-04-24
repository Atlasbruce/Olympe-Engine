#include "PlaceholderNodePalette.h"
#include "PlaceholderGraphDocument.h"
#include <algorithm>
#include <iostream>

namespace Olympe {

PlaceholderNodePalette::PlaceholderNodePalette()
    : m_hoveredType(-1), m_searchFilter(""), m_selectedCategory(0)
{
    Initialize();
}

PlaceholderNodePalette::~PlaceholderNodePalette()
{
}

void PlaceholderNodePalette::Initialize()
{
    // Define the 3 node types available for creation
    m_items.push_back({"Blue Node", PlaceholderNodeType::Blue, "Core", "Basic blue placeholder node"});
    m_items.push_back({"Green Node", PlaceholderNodeType::Green, "Core", "Basic green placeholder node"});
    m_items.push_back({"Magenta Node", PlaceholderNodeType::Magenta, "Core", "Basic magenta placeholder node"});
}

void PlaceholderNodePalette::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

    // Title
    ImGui::TextUnformatted("Component Palette");
    ImGui::Separator();

    // Search filter
    RenderSearchBar();

    // Component list
    ImGui::BeginChild("##palette_list", ImVec2(-1.0f, -40.0f), true);
    {
        RenderComponentList();
    }
    ImGui::EndChild();

    // Status bar
    ImGui::TextDisabled("Drag components onto canvas");

    ImGui::PopStyleVar(2);
}

void PlaceholderNodePalette::RenderSearchBar()
{
    static char searchBuffer[128] = {};
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##palette_search", searchBuffer, sizeof(searchBuffer))) {
        m_searchFilter = searchBuffer;
    }
}

void PlaceholderNodePalette::RenderComponentList()
{
    for (const auto& item : m_items)
    {
        // Apply search filter
        if (!m_searchFilter.empty())
        {
            std::string lowerName = item.name;
            std::string lowerFilter = m_searchFilter;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
            if (lowerName.find(lowerFilter) == std::string::npos)
            {
                continue;
            }
        }

        // Render component item with category label
        std::string itemLabel = item.name + " (" + item.category + ")";
        
        ImGui::PushID(static_cast<int>(item.type));
        {
            // Selectable with visual feedback
            if (ImGui::Selectable(itemLabel.c_str(), false, ImGuiSelectableFlags_DontClosePopups))
            {
                // Item clicked (can implement direct creation if needed)
            }

            // Tooltip
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            {
                ImGui::SetTooltip("%s", item.description.c_str());
            }

            // Phase 64.1: Drag-source setup (must come after Selectable)
            SetupDragSource(item);
        }
        ImGui::PopID();
    }
}

void PlaceholderNodePalette::SetupDragSource(const PaletteItem& item)
{
    // Phase 64.1: Setup drag-source for drag-drop node creation
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        // Store node type as payload
        int nodeTypeValue = static_cast<int>(item.type);
        ImGui::SetDragDropPayload("PLACEHOLDER_NODE_TYPE", &nodeTypeValue, sizeof(int));

        // Phase 64: Diagnostic - show what's being dragged
        ImGui::TextUnformatted(item.name.c_str());
        ImGui::TextDisabled("Drag to canvas to create node");

        ImGui::EndDragDropSource();
    }
}

} // namespace Olympe
