/**
 * @file SwitchCaseEditorModal.cpp
 * @brief Implementation of SwitchCaseEditorModal for editing Switch node cases.
 * @author Olympe Engine
 * @date 2026-03-20
 */

#include "SwitchCaseEditorModal.h"
#include "../../third_party/imgui/imgui.h"
#include <algorithm>

namespace Olympe {

SwitchCaseEditorModal::SwitchCaseEditorModal()
    : m_isOpen(false)
    , m_confirmed(false)
{
}

void SwitchCaseEditorModal::Open(const std::vector<SwitchCaseDefinition>& currentCases)
{
    m_editingCases = currentCases;
    m_confirmed = false;
    m_isOpen = true;

    // Sync buffers: one value and label per case
    m_caseValueBuffers.clear();
    m_caseLabelBuffers.clear();
    for (const auto& casedef : m_editingCases)
    {
        m_caseValueBuffers.push_back(casedef.value);
        m_caseLabelBuffers.push_back(casedef.customLabel);
    }

    ImGui::OpenPopup("Switch Case Editor##modal");
}

void SwitchCaseEditorModal::Close()
{
    m_isOpen = false;
    m_confirmed = false;
    m_editingCases.clear();
    m_caseValueBuffers.clear();
    m_caseLabelBuffers.clear();
}

void SwitchCaseEditorModal::Render()
{
    if (!m_isOpen)
        return;

    // Center the modal on screen with reasonable size
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600.0f, 350.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 200.0f), ImVec2(800.0f, 600.0f));

    bool open = true;
    if (ImGui::BeginPopupModal("Switch Case Editor##modal", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Edit Switch Cases");
        ImGui::Separator();

        // Scrollable case list
        RenderCaseList();

        ImGui::Separator();

        // Action buttons
        RenderActionButtons();

        ImGui::EndPopup();
    }

    if (!open)
    {
        Close();
    }
}

void SwitchCaseEditorModal::RenderCaseList()
{
    // Ensure buffers match the case count
    while (m_caseValueBuffers.size() < m_editingCases.size())
    {
        m_caseValueBuffers.push_back("");
        m_caseLabelBuffers.push_back("");
    }
    while (m_caseValueBuffers.size() > m_editingCases.size())
    {
        m_caseValueBuffers.pop_back();
        m_caseLabelBuffers.pop_back();
    }

    ImGui::BeginChild("CaseListScroll##modal", ImVec2(0, 250), true);

    for (size_t i = 0; i < m_editingCases.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));

        bool modified = RenderCaseRow(i);
        if (modified)
        {
            // Sync back to the case
            m_editingCases[i].value = m_caseValueBuffers[i];
            m_editingCases[i].customLabel = m_caseLabelBuffers[i];
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

bool SwitchCaseEditorModal::RenderCaseRow(size_t caseIndex)
{
    bool modified = false;

    ImGui::Columns(4, nullptr, false);

    // Column 1: Case index (read-only)
    ImGui::Text("%zu", caseIndex);
    ImGui::NextColumn();

    // Column 2: Case value
    float columnWidth = ImGui::GetColumnWidth() - 20.0f;
    ImGui::SetNextItemWidth(columnWidth);

    // Use a buffer for InputText (std::string needs to be converted)
    char valueBuf[256];
    strncpy_s(valueBuf, sizeof(valueBuf), m_caseValueBuffers[caseIndex].c_str(), _TRUNCATE);
    if (ImGui::InputText("##value", valueBuf, sizeof(valueBuf)))
    {
        m_caseValueBuffers[caseIndex] = valueBuf;
        modified = true;
    }
    ImGui::NextColumn();

    // Column 3: Custom label
    ImGui::SetNextItemWidth(columnWidth);

    char labelBuf[256];
    strncpy_s(labelBuf, sizeof(labelBuf), m_caseLabelBuffers[caseIndex].c_str(), _TRUNCATE);
    if (ImGui::InputText("##label", labelBuf, sizeof(labelBuf)))
    {
        m_caseLabelBuffers[caseIndex] = labelBuf;
        modified = true;
    }
    ImGui::NextColumn();

    // Column 4: Action buttons (Move Up, Move Down, Delete)
    if (caseIndex > 0 && ImGui::Button("^##up"))
    {
        // Move case up
        std::swap(m_editingCases[caseIndex], m_editingCases[caseIndex - 1]);
        std::swap(m_caseValueBuffers[caseIndex], m_caseValueBuffers[caseIndex - 1]);
        std::swap(m_caseLabelBuffers[caseIndex], m_caseLabelBuffers[caseIndex - 1]);
        modified = true;
    }
    ImGui::SameLine();
    if (caseIndex < m_editingCases.size() - 1 && ImGui::Button("v##down"))
    {
        // Move case down
        std::swap(m_editingCases[caseIndex], m_editingCases[caseIndex + 1]);
        std::swap(m_caseValueBuffers[caseIndex], m_caseValueBuffers[caseIndex + 1]);
        std::swap(m_caseLabelBuffers[caseIndex], m_caseLabelBuffers[caseIndex + 1]);
        modified = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("X##delete", ImVec2(20, 0)))
    {
        // Mark for deletion
        m_editingCases.erase(m_editingCases.begin() + caseIndex);
        m_caseValueBuffers.erase(m_caseValueBuffers.begin() + caseIndex);
        m_caseLabelBuffers.erase(m_caseLabelBuffers.begin() + caseIndex);
        modified = true;
    }
    ImGui::NextColumn();

    ImGui::Columns(1);

    return modified;
}

void SwitchCaseEditorModal::RenderActionButtons()
{
    // "Add Case" button
    if (ImGui::Button("Add Case", ImVec2(100, 0)))
    {
        SwitchCaseDefinition newCase;
        newCase.value = "NewCase";
        newCase.pinName = "Case_" + std::to_string(m_editingCases.size());
        newCase.customLabel = "";
        m_editingCases.push_back(newCase);
        m_caseValueBuffers.push_back(newCase.value);
        m_caseLabelBuffers.push_back(newCase.customLabel);
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine(ImGui::GetWindowWidth() - 210);

    // "Apply" button
    if (ImGui::Button("Apply", ImVec2(100, 0)))
    {
        m_confirmed = true;
        ImGui::CloseCurrentPopup();
        m_isOpen = false;
    }

    ImGui::SameLine();

    // "Cancel" button
    if (ImGui::Button("Cancel", ImVec2(100, 0)))
    {
        m_confirmed = false;
        ImGui::CloseCurrentPopup();
        m_isOpen = false;
    }
}

} // namespace Olympe
