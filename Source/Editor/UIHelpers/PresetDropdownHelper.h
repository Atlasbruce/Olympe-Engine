/**
 * @file PresetDropdownHelper.h
 * @brief Reusable ImGui dropdown component for ConditionPreset selection (Phase 24).
 * @author Olympe Engine
 * @date 2026-03-20
 *
 * @details
 * PresetDropdownHelper provides a reusable dropdown widget that displays all
 * available ConditionPresets from the global registry, with optional filtering
 * and search capabilities.
 *
 * Usage:
 * @code
 *   PresetDropdownHelper dropdown(registry);
 *   dropdown.SetLabel("Select Condition");
 *   if (dropdown.Render(selectedPresetID)) {
 *       // User selected a preset; selectedPresetID was updated
 *       OnPresetSelected(selectedPresetID);
 *   }
 * @endcode
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include "../../Editor/ConditionPreset/ConditionPreset.h"
#include "../../Editor/ConditionPreset/ConditionPresetRegistry.h"

namespace Olympe {

/**
 * @class PresetDropdownHelper
 * @brief ImGui dropdown widget for selecting a ConditionPreset from the registry.
 */
class PresetDropdownHelper {
public:

    /**
     * @brief Constructs the dropdown helper bound to the preset registry.
     * @param registry  Global ConditionPresetRegistry (must outlive this helper).
     */
    explicit PresetDropdownHelper(ConditionPresetRegistry& registry);

    ~PresetDropdownHelper() = default;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the label displayed next to the dropdown.
     * @param label Display label (e.g. "Select Condition").
     */
    void SetLabel(const std::string& label) { m_label = label; }

    /**
     * @brief Sets optional filter — only presets matching this substring
     *        are shown in the dropdown.
     * @param filter Filter string (empty = no filter).
     */
    void SetFilter(const std::string& filter) { m_filter = filter; }

    /**
     * @brief Clears any active filter.
     */
    void ClearFilter() { m_filter.clear(); }

    // -----------------------------------------------------------------------
    // Rendering & Interaction (ImGui — headless-safe)
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the dropdown widget and handles selection.
     *
     * Must be called once per frame inside the ImGui render loop.
     * 
     * @param[in,out] selectedPresetID  Current selection; updated on user change.
     *                                  Pass empty string for "no selection".
     * @return true if the user made a new selection in this frame.
     *
     * @details
     * The dropdown displays presets filtered by m_filter and the registry's
     * internal search state. Clicking a preset updates selectedPresetID and
     * returns true.
     */
    bool Render(std::string& selectedPresetID);

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the number of presets visible after filtering.
     */
    size_t GetVisiblePresetCount() const;

    /**
     * @brief Returns the display name of a preset, or empty if not found.
     */
    std::string GetPresetDisplayName(const std::string& presetID) const;

private:

    ConditionPresetRegistry& m_registry;
    std::string              m_label;       // Dropdown label
    std::string              m_filter;      // Search/filter substring
    bool                     m_isOpen = false; // Dropdown open state
    int                      m_hoveredIndex = -1; // Hovered item (-1 = none)
};

} // namespace Olympe
