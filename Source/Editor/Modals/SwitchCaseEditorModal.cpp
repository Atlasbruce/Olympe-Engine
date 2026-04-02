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

void SwitchCaseEditorModal::Open(const std::vector<SwitchCaseDefinition>& currentCases,
                                  const std::string& switchVarName,
                                  const std::string& switchVarType,
                                  const std::string& currentVarValue)
{
    m_editingCases = currentCases;
    m_confirmed = false;
    m_isOpen = true;
    m_hasValidationError = false;

    // Phase 26-A: Store context for rendering
    m_switchVarName = switchVarName;
    m_switchVarType = switchVarType;
    m_currentVarValue = currentVarValue;

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
    // Increased size: 1000px wide to accommodate all columns properly, 500px tall for scrolling
    ImGui::SetNextWindowSize(ImVec2(1000.0f, 500.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(800.0f, 300.0f), ImVec2(1200.0f, 700.0f));

    bool open = true;
    // FIXED: Removed ImGuiWindowFlags_AlwaysAutoResize to prevent animation
    if (ImGui::BeginPopupModal("Switch Case Editor##modal", &open, ImGuiWindowFlags_NoMove))
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
    // Phase 26-A: Display context header
    if (!m_switchVarName.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f)); // Cyan
        ImGui::Text("Switching on: %s", m_switchVarName.c_str());
        ImGui::PopStyleColor();

        if (!m_switchVarType.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", m_switchVarType.c_str());
        }

        if (!m_currentVarValue.empty())
        {
            ImGui::TextDisabled("Current Value: %s", m_currentVarValue.c_str());
        }

        ImGui::TextDisabled("Add cases for each value to match below.");
        ImGui::Separator();
    }

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

    // Phase 26-A Fix: Fixed header + scrollable content
    // Draw the header OUTSIDE the scroll area so it stays fixed
    ImGui::Columns(5, "CaseListColumns", false);
    ImGui::SetColumnWidth(0, 35.0f);    // Index
    ImGui::SetColumnWidth(1, 140.0f);   // Match Value (INCREASED)
    ImGui::SetColumnWidth(2, 180.0f);   // Display Name (INCREASED)
    ImGui::SetColumnWidth(3, 120.0f);   // Pin Name (INCREASED)
    ImGui::SetColumnWidth(4, 110.0f);   // Actions (INCREASED)

    ImGui::TextDisabled("#");
    ImGui::NextColumn();
    ImGui::TextDisabled("⚙️ Match Value");
    ImGui::NextColumn();
    ImGui::TextDisabled("👁️ Display Name");
    ImGui::NextColumn();
    ImGui::TextDisabled("Pin Name");
    ImGui::NextColumn();
    ImGui::TextDisabled("Actions");
    ImGui::NextColumn();
    ImGui::Separator();

    ImGui::Columns(1);  // Reset columns before scrollable area

    // NOW the scrollable area
    ImGui::BeginChild("CaseListScroll##modal", ImVec2(0, 300), true);

    // Reset validation error flag before checking
    m_hasValidationError = false;

    // Draw rows with columns INSIDE scroll area
    ImGui::Columns(5, "CaseListColumns", false);
    ImGui::SetColumnWidth(0, 35.0f);    // Index (same widths as header)
    ImGui::SetColumnWidth(1, 140.0f);   // Match Value
    ImGui::SetColumnWidth(2, 180.0f);   // Display Name
    ImGui::SetColumnWidth(3, 120.0f);   // Pin Name
    ImGui::SetColumnWidth(4, 110.0f);   // Actions

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

    ImGui::Columns(1);
    ImGui::EndChild();
}

bool SwitchCaseEditorModal::RenderCaseRow(size_t caseIndex)
{
    bool modified = false;

    // Index column
    ImGui::TextDisabled("%zu", caseIndex);
    ImGui::NextColumn();

    // Phase 26-A: Match Value column (blue background, critical field)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.3f, 0.5f, 1.0f));
    float columnWidth = ImGui::GetColumnWidth() - 35.0f;  // Leave space for validation indicator
    ImGui::SetNextItemWidth(columnWidth);

    char valueBuf[256];
    strncpy_s(valueBuf, sizeof(valueBuf), m_caseValueBuffers[caseIndex].c_str(), _TRUNCATE);
    if (ImGui::InputText("##value", valueBuf, sizeof(valueBuf)))
    {
        m_caseValueBuffers[caseIndex] = valueBuf;
        modified = true;
    }
    ImGui::PopStyleColor();

    // Phase 26-A: Validation check for duplicate or empty value
    bool hasError = false;
    ImGui::SameLine();
    if (m_caseValueBuffers[caseIndex].empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠️");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Match value cannot be empty");
        hasError = true;
    }
    else
    {
        // Check for duplicate
        bool isDuplicate = false;
        for (size_t j = 0; j < m_editingCases.size(); ++j)
        {
            if (j != caseIndex && m_caseValueBuffers[j] == m_caseValueBuffers[caseIndex])
            {
                isDuplicate = true;
                break;
            }
        }
        if (isDuplicate)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "❌");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Duplicate value - another case already uses this");
            hasError = true;
            m_hasValidationError = true;
        }
        else
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓");
        }
    }

    if (hasError)
        m_hasValidationError = true;

    ImGui::NextColumn();

    // Phase 26-A: Display Label column (green background, optional field)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.4f, 0.2f, 1.0f));
    columnWidth = ImGui::GetColumnWidth() - 20.0f;  // Leave space for info indicator
    ImGui::SetNextItemWidth(columnWidth);

    char labelBuf[256];
    strncpy_s(labelBuf, sizeof(labelBuf), m_caseLabelBuffers[caseIndex].c_str(), _TRUNCATE);
    if (ImGui::InputText("##label", labelBuf, sizeof(labelBuf)))
    {
        m_caseLabelBuffers[caseIndex] = labelBuf;
        modified = true;
    }
    ImGui::PopStyleColor();

    // Info indicator for empty label
    ImGui::SameLine();
    if (m_caseLabelBuffers[caseIndex].empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ℹ️");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Display name is optional - will show pin name on canvas");
    }

    ImGui::NextColumn();

    // Phase 26-A: Pin Name column (read-only, with copy button)
    std::string pinName = m_editingCases[caseIndex].pinName;
    ImGui::TextDisabled("%s", pinName.c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy##pin"))
    {
        ImGui::SetClipboardText(pinName.c_str());
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Copy pin name to clipboard");

    ImGui::NextColumn();

    // Action buttons (Move Up, Move Down, Delete) with tooltips
    if (caseIndex > 0 && ImGui::Button("^##up", ImVec2(25, 0)))
    {
        std::swap(m_editingCases[caseIndex], m_editingCases[caseIndex - 1]);
        std::swap(m_caseValueBuffers[caseIndex], m_caseValueBuffers[caseIndex - 1]);
        std::swap(m_caseLabelBuffers[caseIndex], m_caseLabelBuffers[caseIndex - 1]);
        modified = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Move this case up");

    ImGui::SameLine();
    if (caseIndex < m_editingCases.size() - 1 && ImGui::Button("v##down", ImVec2(25, 0)))
    {
        std::swap(m_editingCases[caseIndex], m_editingCases[caseIndex + 1]);
        std::swap(m_caseValueBuffers[caseIndex], m_caseValueBuffers[caseIndex + 1]);
        std::swap(m_caseLabelBuffers[caseIndex], m_caseLabelBuffers[caseIndex + 1]);
        modified = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Move this case down");

    ImGui::SameLine();
    if (ImGui::Button("X##delete", ImVec2(25, 0)))
    {
        m_editingCases.erase(m_editingCases.begin() + caseIndex);
        m_caseValueBuffers.erase(m_caseValueBuffers.begin() + caseIndex);
        m_caseLabelBuffers.erase(m_caseLabelBuffers.begin() + caseIndex);
        modified = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Delete this case");

    ImGui::NextColumn();

    return modified;
}

void SwitchCaseEditorModal::RenderActionButtons()
{
    // Phase 26-A: "Add Case" button with intelligent defaults
    if (ImGui::Button("Add Case", ImVec2(100, 0)))
    {
        SwitchCaseDefinition newCase;

        // Intelligent suggestion: if all cases are numeric, suggest next number
        bool allNumeric = true;
        int maxNum = -1;
        for (const auto& caseData : m_editingCases)
        {
            try
            {
                int num = std::stoi(caseData.value);
                if (num > maxNum) maxNum = num;
            }
            catch (...)
            {
                allNumeric = false;
                break;
            }
        }

        if (allNumeric && maxNum >= 0)
        {
            // Suggest next sequential number
            newCase.value = std::to_string(maxNum + 1);
            newCase.customLabel = "State_" + std::to_string(maxNum + 1);
        }
        else
        {
            // Empty for user to fill
            newCase.value = "";
            newCase.customLabel = "";
        }

        newCase.pinName = "Case_" + std::to_string(m_editingCases.size());
        m_editingCases.push_back(newCase);
        m_caseValueBuffers.push_back(newCase.value);
        m_caseLabelBuffers.push_back(newCase.customLabel);

        // Auto-focus the new value field
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::SameLine();
    ImGui::Spacing();

    // Phase 26-A: Show validation error message if any
    if (m_hasValidationError)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚠️ Fix errors before applying");
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 210);

    // Phase 26-A: "Apply" button disabled if validation errors exist
    if (m_hasValidationError)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Apply", ImVec2(100, 0)))
    {
        m_confirmed = true;
        ImGui::CloseCurrentPopup();
        m_isOpen = false;
    }
    if (m_hasValidationError)
    {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Cannot apply - please fix validation errors (marked with ❌)");
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
