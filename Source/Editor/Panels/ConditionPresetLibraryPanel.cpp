/**
 * @file ConditionPresetLibraryPanel.cpp
 * @brief Implementation of ConditionPresetLibraryPanel (Phase 24.1).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/Panels/ConditionPresetLibraryPanel.h"

#include <algorithm>

// ImGui is only included when building the full editor (not in tests).
// Tests exercise the logic methods directly without rendering.
#ifndef OLYMPE_HEADLESS
#  include "third_party/imgui/imgui.h"
#endif

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

ConditionPresetLibraryPanel::ConditionPresetLibraryPanel(
        ConditionPresetRegistry& registry)
    : m_registry(registry)
{
}

// ============================================================================
// Search filter
// ============================================================================

void ConditionPresetLibraryPanel::SetSearchFilter(const std::string& filter)
{
    m_searchFilter = filter;
}

// ============================================================================
// Selection
// ============================================================================

void ConditionPresetLibraryPanel::SetSelectedPresetID(const std::string& id)
{
    m_selectedPresetID = id;
}

void ConditionPresetLibraryPanel::OnPresetSelected(const std::string& presetID)
{
    m_selectedPresetID = presetID;
}

// ============================================================================
// Filtered presets
// ============================================================================

std::vector<ConditionPreset> ConditionPresetLibraryPanel::GetFilteredPresets() const
{
    return m_registry.GetFilteredPresets(m_searchFilter);
}

// ============================================================================
// Reference analysis
// ============================================================================

void ConditionPresetLibraryPanel::SetReferenceMap(
        const std::map<std::string, std::vector<std::string>>& refMap)
{
    m_refMap = refMap;
}

std::vector<std::string>
ConditionPresetLibraryPanel::GetReferencingNodes(const std::string& presetID) const
{
    // Build reverse map: presetID → list of nodeIDs that reference it
    std::vector<std::string> nodes;
    for (const auto& kv : m_refMap)
    {
        const std::string& nodeID   = kv.first;
        const std::vector<std::string>& presets = kv.second;

        for (const auto& pid : presets)
        {
            if (pid == presetID)
            {
                nodes.push_back(nodeID);
                break;
            }
        }
    }
    return nodes;
}

// ============================================================================
// Action handlers
// ============================================================================

std::string ConditionPresetLibraryPanel::OnAddPresetClicked()
{
    ConditionPreset newPreset;
    // Default: empty condition with Variable/==Const structure
    newPreset.condition.leftMode  = "Variable";
    newPreset.condition.operatorStr = "==";
    newPreset.condition.rightMode = "Const";

    const std::string id = m_registry.AddPreset(newPreset);

    if (OnPresetCreated)
    {
        OnPresetCreated(id);
    }

    m_selectedPresetID = id;
    return id;
}

std::string ConditionPresetLibraryPanel::OnDuplicatePresetClicked(
        const std::string& presetID)
{
    const std::string newID = m_registry.DuplicatePreset(presetID);

    if (!newID.empty() && OnPresetCreated)
    {
        OnPresetCreated(newID);
    }

    return newID;
}

void ConditionPresetLibraryPanel::OnDeletePresetClicked(const std::string& presetID)
{
    m_presetToDelete         = presetID;
    m_showDeleteConfirmation = true;
}

void ConditionPresetLibraryPanel::OnDeleteConfirmed(const std::string& presetID)
{
    if (presetID != m_presetToDelete)
    {
        return;
    }

    m_registry.RemovePreset(presetID);

    if (m_selectedPresetID == presetID)
    {
        m_selectedPresetID.clear();
    }

    m_showDeleteConfirmation = false;
    m_presetToDelete.clear();

    if (OnPresetDeleted)
    {
        OnPresetDeleted(presetID);
    }
}

void ConditionPresetLibraryPanel::OnDeleteCancelled()
{
    m_showDeleteConfirmation = false;
    m_presetToDelete.clear();
}

// ============================================================================
// Render (ImGui — skipped in headless builds / tests)
// ============================================================================

void ConditionPresetLibraryPanel::Render()
{
#ifndef OLYMPE_HEADLESS
    if (!m_isOpen) { return; }

    ImGui::SetNextWindowSize(ImVec2(440, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Condition Preset Library", &m_isOpen))
    {
        ImGui::End();
        return;
    }

    RenderToolbar();
    ImGui::Separator();
    RenderPresetList();

    if (m_showDeleteConfirmation)
    {
        RenderDeleteConfirmationDialog();
    }

    ImGui::End();
#endif
}

// ============================================================================
// Private rendering helpers
// ============================================================================

void ConditionPresetLibraryPanel::RenderToolbar()
{
#ifndef OLYMPE_HEADLESS
    if (ImGui::Button("+ Add Condition Preset"))
    {
        OnAddPresetClicked();
    }

    ImGui::SameLine();

    // Search box (right-aligned)
    char searchBuf[256] = {};
    if (m_searchFilter.size() < sizeof(searchBuf))
    {
        m_searchFilter.copy(searchBuf, m_searchFilter.size());
    }
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::InputText("##Search", searchBuf, sizeof(searchBuf)))
    {
        m_searchFilter = searchBuf;
    }
#endif
}

void ConditionPresetLibraryPanel::RenderPresetList()
{
#ifndef OLYMPE_HEADLESS
    const std::vector<ConditionPreset> presets = GetFilteredPresets();

    for (const auto& preset : presets)
    {
        RenderPresetItem(preset.id, preset);
    }
#endif
}

void ConditionPresetLibraryPanel::RenderPresetItem(const std::string& presetID,
                                                    const ConditionPreset& preset)
{
#ifndef OLYMPE_HEADLESS
    const bool isSelected = (presetID == m_selectedPresetID);

    ImGui::PushID(presetID.c_str());

    // Collapsible header
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (isSelected) { flags |= ImGuiTreeNodeFlags_Selected; }

    const std::string label = preset.name + "  " + preset.GetPreview();
    bool open = ImGui::CollapsingHeader(label.c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        OnPresetSelected(presetID);
    }

    // Action buttons on the same line
    ImGui::SameLine();
    if (ImGui::SmallButton("Dup"))
    {
        OnDuplicatePresetClicked(presetID);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("X"))
    {
        OnDeletePresetClicked(presetID);
    }

    if (open)
    {
        ImGui::Indent();
        ImGui::TextUnformatted(preset.GetPreview().c_str());
        RenderReferenceAnalysis(presetID);
        ImGui::Unindent();
    }

    ImGui::PopID();
#endif
}

void ConditionPresetLibraryPanel::RenderDeleteConfirmationDialog()
{
#ifndef OLYMPE_HEADLESS
    ImGui::OpenPopup("Delete Preset?");
    if (ImGui::BeginPopupModal("Delete Preset?", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        const ConditionPreset* preset = m_registry.GetPreset(m_presetToDelete);
        if (preset)
        {
            ImGui::Text("Delete \"%s\"?", preset->name.c_str());
            const std::vector<std::string> users = GetReferencingNodes(m_presetToDelete);
            if (!users.empty())
            {
                ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                                   "Warning: used by %d node(s)",
                                   static_cast<int>(users.size()));
            }
        }

        if (ImGui::Button("Delete"))
        {
            OnDeleteConfirmed(m_presetToDelete);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            OnDeleteCancelled();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
#endif
}

void ConditionPresetLibraryPanel::RenderReferenceAnalysis(const std::string& presetID)
{
#ifndef OLYMPE_HEADLESS
    const std::vector<std::string> users = GetReferencingNodes(presetID);
    if (!users.empty())
    {
        std::string usedBy = "Used by: ";
        for (size_t i = 0; i < users.size(); ++i)
        {
            if (i > 0) { usedBy += ", "; }
            usedBy += users[i];
        }
        ImGui::TextDisabled("%s", usedBy.c_str());
    }
#endif
}

} // namespace Olympe
