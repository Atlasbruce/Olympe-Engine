/**
 * @file PresetDropdownHelper.cpp
 * @brief Implementation of PresetDropdownHelper (Phase 24).
 * @author Olympe Engine
 * @date 2026-03-20
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "PresetDropdownHelper.h"

#include <algorithm>

// ImGui is only available in the full editor build.
#ifndef OLYMPE_HEADLESS
#  include "../../third_party/imgui/imgui.h"
#endif

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

PresetDropdownHelper::PresetDropdownHelper(ConditionPresetRegistry& registry)
    : m_registry(registry)
    , m_label("Select Preset")
{
}

// ============================================================================
// Rendering & Interaction
// ============================================================================

bool PresetDropdownHelper::Render(std::string& selectedPresetID)
{
    bool changed = false;

#ifndef OLYMPE_HEADLESS
    // Get filtered presets based on m_filter
    std::vector<ConditionPreset> presets = m_registry.GetFilteredPresets(m_filter);

    // Combo box label: display selected preset name or placeholder
    std::string comboLabel = "None";
    if (!selectedPresetID.empty())
    {
        const ConditionPreset* selected = m_registry.GetPreset(selectedPresetID);
        if (selected)
            comboLabel = selected->name;
    }

    // Render combo box header
    if (ImGui::BeginCombo(m_label.c_str(), comboLabel.c_str()))
    {
        // Optional: Add search field at top of dropdown
        static char searchBuf[128] = {0};
        ImGui::InputTextWithHint("##search", "Filter...", searchBuf, sizeof(searchBuf));
        std::string newFilter(searchBuf);
        if (newFilter != m_filter)
        {
            m_filter = newFilter;
            presets = m_registry.GetFilteredPresets(m_filter);
        }

        ImGui::Separator();

        // Special "None" option
        if (ImGui::Selectable("(None)", selectedPresetID.empty()))
        {
            selectedPresetID.clear();
            changed = true;
        }

        // List all presets
        for (size_t i = 0; i < presets.size(); ++i)
        {
            const ConditionPreset& preset = presets[i];
            bool isSelected = (selectedPresetID == preset.id);

            // Format: "Condition #1  [mHealth] <= [2]"
            std::string itemLabel = preset.name + "  " + preset.GetPreview();

            if (ImGui::Selectable(itemLabel.c_str(), isSelected))
            {
                selectedPresetID = preset.id;
                changed = true;
            }

            // Highlight selected item
            if (isSelected)
                ImGui::SetItemDefaultFocus();

            // Tooltip on hover
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("ID:   %s", preset.id.c_str());
                ImGui::Text("Name: %s", preset.name.c_str());
                ImGui::Text("Expr: %s", preset.GetPreview().c_str());
                ImGui::EndTooltip();
            }
        }

        ImGui::EndCombo();
    }

#endif

    return changed;
}

// ============================================================================
// Query
// ============================================================================

size_t PresetDropdownHelper::GetVisiblePresetCount() const
{
    std::vector<ConditionPreset> filtered = m_registry.GetFilteredPresets(m_filter);
    return filtered.size();
}

std::string PresetDropdownHelper::GetPresetDisplayName(const std::string& presetID) const
{
    const ConditionPreset* preset = m_registry.GetPreset(presetID);
    if (preset)
        return preset->name;
    return "";
}

} // namespace Olympe
