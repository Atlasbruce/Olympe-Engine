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

    // ── Section 1: Title bar ─────────────────────────────────────────────────
    RenderTitleSection();

    ImGui::Separator();

    // ── Section 2: Static exec pins ─────────────────────────────────────────
    RenderExecPinsSection();

    ImGui::Separator();

    // ── Section 3: Conditions preview + Edit button ──────────────────────────
    RenderConditionsPreview();

    // ── Section 4: Dynamic data pins (only if any) ──────────────────────────
    if (!m_dynamicPins.empty())
    {
        ImGui::Separator();
        RenderDynamicPinsSection();
    }

    ImGui::PopID();
#endif
}

void NodeConditionsPanel::RenderTitleSection()
{
#ifndef OLYMPE_HEADLESS
    // Blue background for the title bar (matches ImGuiCol_Header)
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_Header));
    const std::string& title = m_nodeName.empty() ? "Node" : m_nodeName;
    ImGui::Selectable(title.c_str(), true, ImGuiSelectableFlags_None, ImVec2(0.f, 24.f));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("Name");
#endif
}

void NodeConditionsPanel::RenderExecPinsSection()
{
#ifndef OLYMPE_HEADLESS
    // Static exec pins — never editable from this panel
    const float columnWidth = 120.f;

    ImGui::TextDisabled("In");
    ImGui::SameLine(columnWidth);
    ImGui::TextDisabled("Then");

    ImGui::TextDisabled("  ");
    ImGui::SameLine(columnWidth);
    ImGui::TextDisabled("Else");
#endif
}

void NodeConditionsPanel::RenderConditionsPreview()
{
#ifndef OLYMPE_HEADLESS
    // Green text for condition previews (READ-ONLY)
    const ImVec4 condColor(0.f, 1.f, 0.f, 1.f);

    if (m_conditionRefs.empty())
    {
        ImGui::TextDisabled("(no conditions)");
    }
    else
    {
        for (size_t i = 0; i < m_conditionRefs.size(); ++i)
        {
            const NodeConditionRef& ref = m_conditionRefs[i];

            ImGui::PushID(static_cast<int>(i));

            // Logical operator column (fixed width)
            const char* opLabel = "   ";
            if (i > 0)
            {
                if (ref.logicalOp == LogicalOp::And)
                    opLabel = "And";
                else if (ref.logicalOp == LogicalOp::Or)
                    opLabel = "Or ";
            }

            const ConditionPreset* preset = m_registry.GetPreset(ref.presetID);
            ImGui::PushStyleColor(ImGuiCol_Text, condColor);
            if (preset)
                ImGui::Text("%s %s", opLabel, preset->GetPreview().c_str());
            else
                ImGui::Text("%s (missing: %s)", opLabel, ref.presetID.c_str());
            ImGui::PopStyleColor();

            ImGui::PopID();
        }
    }

    ImGui::Spacing();

    // "Edit Conditions" button opens the dedicated modal
    if (ImGui::Button("Edit Conditions"))
        m_editModalRequested = true;
#endif
}

void NodeConditionsPanel::RenderDynamicPinsSection()
{
#ifndef OLYMPE_HEADLESS
    // Yellow color for dynamic pins
    const ImVec4 pinColor(1.f, 1.f, 0.f, 1.f);
    for (const auto& pin : m_dynamicPins)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, pinColor);
        ImGui::BulletText("%s", pin.GetDisplayLabel().c_str());
        ImGui::PopStyleColor();
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
