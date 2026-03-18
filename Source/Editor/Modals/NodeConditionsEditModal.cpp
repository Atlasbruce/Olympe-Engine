/**
 * @file NodeConditionsEditModal.cpp
 * @brief Implementation of NodeConditionsEditModal (Phase 24-REFONTE).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "NodeConditionsEditModal.h"

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

NodeConditionsEditModal::NodeConditionsEditModal(ConditionPresetRegistry& registry)
    : m_registry(registry)
{
}

// ============================================================================
// Visibility / lifecycle
// ============================================================================

void NodeConditionsEditModal::Open(const std::vector<NodeConditionRef>& currentRefs)
{
    m_workingCopy = currentRefs;
    NormalizeLogicalOps();
    m_isOpen      = true;
    m_isConfirmed = false;
    m_pickerOpen  = false;
    m_dropdownFilter.clear();
}

void NodeConditionsEditModal::Close()
{
    m_isOpen     = false;
    m_pickerOpen = false;
}

// ============================================================================
// Result
// ============================================================================

const std::vector<NodeConditionRef>& NodeConditionsEditModal::GetConditionRefs() const
{
    return m_workingCopy;
}

// ============================================================================
// Condition management (testable without ImGui)
// ============================================================================

size_t NodeConditionsEditModal::GetConditionCount() const
{
    return m_workingCopy.size();
}

void NodeConditionsEditModal::AddCondition(const std::string& presetID)
{
    if (presetID.empty())
        return;

    LogicalOp op = m_workingCopy.empty() ? LogicalOp::Start : LogicalOp::And;
    m_workingCopy.emplace_back(presetID, op);
    NormalizeLogicalOps();
}

void NodeConditionsEditModal::RemoveCondition(size_t index)
{
    if (index >= m_workingCopy.size())
        return;

    m_workingCopy.erase(m_workingCopy.begin() + static_cast<int>(index));
    NormalizeLogicalOps();
}

void NodeConditionsEditModal::SetLogicalOp(size_t index, LogicalOp op)
{
    if (index >= m_workingCopy.size())
        return;

    m_workingCopy[index].logicalOp = (index == 0) ? LogicalOp::Start : op;
}

void NodeConditionsEditModal::MoveConditionUp(size_t index)
{
    if (index == 0 || index >= m_workingCopy.size())
        return;

    std::swap(m_workingCopy[index - 1], m_workingCopy[index]);
    NormalizeLogicalOps();
}

void NodeConditionsEditModal::MoveConditionDown(size_t index)
{
    if (index + 1 >= m_workingCopy.size())
        return;

    std::swap(m_workingCopy[index], m_workingCopy[index + 1]);
    NormalizeLogicalOps();
}

void NodeConditionsEditModal::Confirm()
{
    m_isConfirmed = true;
    if (OnApply)
        OnApply();
    Close();
}

// ============================================================================
// Dropdown helper
// ============================================================================

void NodeConditionsEditModal::SetDropdownFilter(const std::string& filter)
{
    m_dropdownFilter = filter;
}

std::vector<ConditionPreset>
NodeConditionsEditModal::GetFilteredPresetsForDropdown() const
{
    std::vector<ConditionPreset> result;

    if (m_dropdownFilter.empty())
    {
        for (const auto& id : m_registry.GetAllPresetIDs())
        {
            const ConditionPreset* p = m_registry.GetPreset(id);
            if (p)
                result.push_back(*p);
        }
    }
    else
    {
        for (const auto& id : m_registry.FindPresetsByName(m_dropdownFilter))
        {
            const ConditionPreset* p = m_registry.GetPreset(id);
            if (p)
                result.push_back(*p);
        }
    }

    return result;
}

// ============================================================================
// Rendering
// ============================================================================

void NodeConditionsEditModal::Render()
{
#ifndef OLYMPE_HEADLESS
    if (!m_isOpen)
        return;

    ImGui::OpenPopup("Edit Conditions##NodeConditionsEditModal");

    ImGui::SetNextWindowSize(ImVec2(480.f, 400.f), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Edit Conditions##NodeConditionsEditModal",
                               nullptr, ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Conditions");
        ImGui::Separator();

        for (size_t i = 0; i < m_workingCopy.size(); ++i)
        {
            RenderConditionRow(i, m_workingCopy[i]);
        }

        ImGui::Spacing();
        RenderAddConditionPicker();

        ImGui::Separator();
        RenderFooterButtons();

        ImGui::EndPopup();
    }
#endif
}

void NodeConditionsEditModal::RenderConditionRow(size_t index,
                                                  const NodeConditionRef& ref)
{
#ifndef OLYMPE_HEADLESS
    ImGui::PushID(static_cast<int>(index));

    // Logical operator dropdown (disabled for first entry)
    if (index == 0)
    {
        ImGui::TextDisabled("     ");
    }
    else
    {
        const char* ops[] = { "And", "Or" };
        int current = (ref.logicalOp == LogicalOp::And) ? 0 : 1;
        ImGui::SetNextItemWidth(60.f);
        if (ImGui::Combo("##op", &current, ops, 2))
            SetLogicalOp(index, (current == 0) ? LogicalOp::And : LogicalOp::Or);
    }

    ImGui::SameLine();

    // Condition preset preview
    const ConditionPreset* preset = m_registry.GetPreset(ref.presetID);
    if (preset)
        ImGui::Text("[%s] %s", preset->name.c_str(), preset->GetPreview().c_str());
    else
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                           "(missing: %s)", ref.presetID.c_str());

    ImGui::SameLine();

    // Move up
    if (ImGui::SmallButton("^"))
        MoveConditionUp(index);

    ImGui::SameLine();

    // Move down
    if (ImGui::SmallButton("v"))
        MoveConditionDown(index);

    ImGui::SameLine();

    // Delete
    if (ImGui::SmallButton("X"))
        RemoveCondition(index);

    ImGui::PopID();
#endif
}

void NodeConditionsEditModal::RenderAddConditionPicker()
{
#ifndef OLYMPE_HEADLESS
    if (ImGui::Button("+ Add Condition"))
        m_pickerOpen = !m_pickerOpen;

    if (m_pickerOpen)
    {
        static char filterBuf[256] = {};
        if (ImGui::InputText("##filter", filterBuf, sizeof(filterBuf)))
            SetDropdownFilter(std::string(filterBuf));

        const std::vector<ConditionPreset> presets = GetFilteredPresetsForDropdown();
        for (const auto& preset : presets)
        {
            const std::string label = preset.name + "  " + preset.GetPreview();
            if (ImGui::Selectable(label.c_str()))
            {
                AddCondition(preset.id);
                m_pickerOpen = false;
                filterBuf[0] = '\0';
                SetDropdownFilter("");
            }
        }

        if (presets.empty())
            ImGui::TextDisabled("No presets found.");
    }
#endif
}

void NodeConditionsEditModal::RenderFooterButtons()
{
#ifndef OLYMPE_HEADLESS
    if (ImGui::Button("Apply"))
        Confirm();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
        Close();
#endif
}

// ============================================================================
// Internal helpers
// ============================================================================

void NodeConditionsEditModal::NormalizeLogicalOps()
{
    if (!m_workingCopy.empty())
        m_workingCopy[0].logicalOp = LogicalOp::Start;
}

} // namespace Olympe
