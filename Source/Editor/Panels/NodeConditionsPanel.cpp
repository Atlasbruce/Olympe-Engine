/**
 * @file NodeConditionsPanel.cpp
 * @brief Implementation of NodeConditionsPanel (Phase 24.2).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "NodeConditionsPanel.h"

#include <algorithm>
#include <sstream>

// ImGui is only compiled in the full editor build.
#ifndef OLYMPE_HEADLESS
#  include "../../third_party/imgui/imgui.h"
#endif

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

NodeConditionsPanel::NodeConditionsPanel(ConditionPresetRegistry& registry)
    : m_registry(registry)
{
}

// ============================================================================
// State accessors
// ============================================================================

void NodeConditionsPanel::SetConditionRefs(const std::vector<NodeConditionRef>& refs)
{
    m_conditionRefs = refs;
    NormalizeLogicalOps();
    m_dirty = false;
}

const std::vector<NodeConditionRef>& NodeConditionsPanel::GetConditionRefs() const
{
    return m_conditionRefs;
}

void NodeConditionsPanel::SetDynamicPins(const std::vector<DynamicDataPin>& pins)
{
    m_dynamicPins = pins;
}

// ============================================================================
// Condition management
// ============================================================================

void NodeConditionsPanel::AddCondition(const std::string& presetID)
{
    if (presetID.empty())
        return;

    LogicalOp op = m_conditionRefs.empty() ? LogicalOp::Start : LogicalOp::And;
    m_conditionRefs.emplace_back(presetID, op);
    NormalizeLogicalOps();
    m_dirty = true;

    if (OnPresetChanged)
        OnPresetChanged(presetID);
}

void NodeConditionsPanel::RemoveCondition(size_t index)
{
    if (index >= m_conditionRefs.size())
        return;

    m_conditionRefs.erase(m_conditionRefs.begin() + static_cast<int>(index));
    NormalizeLogicalOps();
    m_dirty = true;
}

void NodeConditionsPanel::SetLogicalOp(size_t index, LogicalOp op)
{
    if (index >= m_conditionRefs.size())
        return;

    // Index 0 is always Start
    m_conditionRefs[index].logicalOp = (index == 0) ? LogicalOp::Start : op;
    m_dirty = true;
}

size_t NodeConditionsPanel::GetConditionCount() const
{
    return m_conditionRefs.size();
}

// ============================================================================
// Preset deletion handler
// ============================================================================

void NodeConditionsPanel::OnPresetDeleted(const std::string& deletedPresetID)
{
    bool changed = false;
    auto it = m_conditionRefs.begin();
    while (it != m_conditionRefs.end())
    {
        if (it->presetID == deletedPresetID)
        {
            it      = m_conditionRefs.erase(it);
            changed = true;
        }
        else
        {
            ++it;
        }
    }

    if (changed)
    {
        NormalizeLogicalOps();
        m_dirty = true;
    }
}

// ============================================================================
// Validation
// ============================================================================

bool NodeConditionsPanel::IsValid() const
{
    return Validate().empty();
}

std::vector<std::string> NodeConditionsPanel::Validate() const
{
    std::vector<std::string> errors;

    for (size_t i = 0; i < m_conditionRefs.size(); ++i)
    {
        const NodeConditionRef& ref = m_conditionRefs[i];

        if (ref.presetID.empty())
        {
            std::ostringstream oss;
            oss << "Condition " << (i + 1) << ": preset ID is empty.";
            errors.push_back(oss.str());
            continue;
        }

        if (!m_registry.ValidatePresetID(ref.presetID))
        {
            std::ostringstream oss;
            oss << "Condition " << (i + 1) << ": preset \"" << ref.presetID
                << "\" not found in registry.";
            errors.push_back(oss.str());
        }
    }

    return errors;
}

// ============================================================================
// Dropdown filter
// ============================================================================

void NodeConditionsPanel::SetDropdownFilter(const std::string& filter)
{
    m_dropdownFilter = filter;
}

std::vector<ConditionPreset>
NodeConditionsPanel::GetFilteredPresetsForDropdown() const
{
    if (m_dropdownFilter.empty())
    {
        // Return all presets in registry order
        std::vector<ConditionPreset> result;
        for (const auto& id : m_registry.GetAllPresetIDs())
        {
            const ConditionPreset* p = m_registry.GetPreset(id);
            if (p)
                result.push_back(*p);
        }
        return result;
    }

    // Filtered by name substring
    std::vector<ConditionPreset> result;
    for (const auto& id : m_registry.FindPresetsByName(m_dropdownFilter))
    {
        const ConditionPreset* p = m_registry.GetPreset(id);
        if (p)
            result.push_back(*p);
    }
    return result;
}

// ============================================================================
// Rendering
// ============================================================================

void NodeConditionsPanel::Render()
{
#ifndef OLYMPE_HEADLESS
    ImGui::PushID("NodeConditionsPanel");
    ImGui::Text("Conditions");
    ImGui::Separator();

    for (size_t i = 0; i < m_conditionRefs.size(); ++i)
    {
        RenderConditionRow(i, m_conditionRefs[i]);
    }

    RenderAddConditionDropdown();

    ImGui::Separator();
    RenderDynamicPinsSection();

    ImGui::PopID();
#endif
}

void NodeConditionsPanel::RenderConditionRow(size_t index,
                                              const NodeConditionRef& ref)
{
#ifndef OLYMPE_HEADLESS
    ImGui::PushID(static_cast<int>(index));

    // Logical operator selector (radio buttons)
    if (index == 0)
    {
        ImGui::TextDisabled("Start");
    }
    else
    {
        int op = (ref.logicalOp == LogicalOp::And) ? 0 : 1;
        if (ImGui::RadioButton("And", op == 0))
            SetLogicalOp(index, LogicalOp::And);
        ImGui::SameLine();
        if (ImGui::RadioButton("Or", op == 1))
            SetLogicalOp(index, LogicalOp::Or);
    }

    ImGui::SameLine();

    // Condition preview
    const ConditionPreset* preset = m_registry.GetPreset(ref.presetID);
    if (preset)
    {
        ImGui::Text("[%s] %s", preset->name.c_str(),
                    preset->GetPreview().c_str());
    }
    else
    {
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                           "(missing preset: %s)", ref.presetID.c_str());
    }

    ImGui::SameLine();

    // Remove button
    if (ImGui::SmallButton("X"))
        RemoveCondition(index);

    ImGui::PopID();
#endif
}

void NodeConditionsPanel::RenderAddConditionDropdown()
{
#ifndef OLYMPE_HEADLESS
    ImGui::Spacing();

    if (ImGui::Button("+ Add Condition"))
        m_dropdownOpen = !m_dropdownOpen;

    if (m_dropdownOpen)
    {
        // Filter text
        static char filterBuf[256] = {};
        if (ImGui::InputText("##filter", filterBuf, sizeof(filterBuf)))
            SetDropdownFilter(std::string(filterBuf));

        const std::vector<ConditionPreset> presets = GetFilteredPresetsForDropdown();
        for (const auto& preset : presets)
        {
            std::string label = preset.name + "  " + preset.GetPreview();
            if (ImGui::Selectable(label.c_str()))
            {
                AddCondition(preset.id);
                m_dropdownOpen = false;
                // Reset filter buffer
                filterBuf[0] = '\0';
                SetDropdownFilter("");
            }
        }

        if (presets.empty())
            ImGui::TextDisabled("No presets found.");
    }
#endif
}

void NodeConditionsPanel::RenderDynamicPinsSection()
{
#ifndef OLYMPE_HEADLESS
    ImGui::Text("Dynamic Pins");
    ImGui::Separator();

    if (m_dynamicPins.empty())
    {
        ImGui::TextDisabled("(none)");
    }
    else
    {
        for (const auto& pin : m_dynamicPins)
        {
            // Yellow color for dynamic pins
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 0.f, 1.f));
            ImGui::BulletText("%s", pin.GetDisplayLabel().c_str());
            ImGui::PopStyleColor();
        }
    }
#endif
}

// ============================================================================
// Internal helpers
// ============================================================================

void NodeConditionsPanel::NormalizeLogicalOps()
{
    if (!m_conditionRefs.empty())
        m_conditionRefs[0].logicalOp = LogicalOp::Start;
}

} // namespace Olympe
