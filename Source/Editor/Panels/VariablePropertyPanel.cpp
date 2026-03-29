/**
 * @file VariablePropertyPanel.cpp
 * @brief Implementation of VariablePropertyPanel.
 * @author Olympe Engine
 * @date 2026-03-20
 *
 * Phase 24.1: Data Pure Nodes
 * 
 * C++14 compliant.
 */

#include "VariablePropertyPanel.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"

namespace Olympe {

VariablePropertyPanel::VariablePropertyPanel()
    : m_template(nullptr)
    , m_dirty(false)
{
}

void VariablePropertyPanel::SetNodeName(const std::string& name)
{
    m_nodeName = name;
}

void VariablePropertyPanel::SetTemplate(const TaskGraphTemplate* tmpl)
{
    m_template = tmpl;
    RebuildVariableList();
}

void VariablePropertyPanel::SetBBKey(const std::string& key)
{
    if (m_selectedBBKey != key)
    {
        m_selectedBBKey = key;
        m_dirty = true;
    }
}

const std::string& VariablePropertyPanel::GetBBKey() const
{
    return m_selectedBBKey;
}

void VariablePropertyPanel::RebuildVariableList()
{
    m_availableVariables.clear();

    if (m_template == nullptr)
        return;

    for (size_t i = 0; i < m_template->Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template->Blackboard[i];
        m_availableVariables.push_back(entry.Key);
    }
}

void VariablePropertyPanel::Render()
{
    if (m_template == nullptr)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No template loaded");
        return;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Variable: %s", m_nodeName.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Select Variable:");

    // Dropdown for variable selection
    if (ImGui::BeginCombo("##variable_selector", m_selectedBBKey.c_str()))
    {
        for (size_t i = 0; i < m_availableVariables.size(); ++i)
        {
            const std::string& varName = m_availableVariables[i];
            bool isSelected = (m_selectedBBKey == varName);

            if (ImGui::Selectable(varName.c_str(), isSelected))
            {
                SetBBKey(varName);
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();

    // Show variable info if selected
    if (!m_selectedBBKey.empty() && m_template != nullptr)
    {
        for (size_t i = 0; i < m_template->Blackboard.size(); ++i)
        {
            const BlackboardEntry& entry = m_template->Blackboard[i];
            if (entry.Key == m_selectedBBKey)
            {
                ImGui::TextDisabled("Type: %s", 
                    (entry.Type == VariableType::Bool ? "Bool" :
                     entry.Type == VariableType::Int ? "Int" :
                     entry.Type == VariableType::Float ? "Float" :
                     entry.Type == VariableType::Vector ? "Vector" :
                     entry.Type == VariableType::EntityID ? "EntityID" :
                     entry.Type == VariableType::String ? "String" : "Unknown"));
                break;
            }
        }
    }
}

} // namespace Olympe

