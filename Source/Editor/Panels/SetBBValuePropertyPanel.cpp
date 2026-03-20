/**
 * @file SetBBValuePropertyPanel.cpp
 * @brief Implementation of SetBBValue properties panel.
 */

#include "SetBBValuePropertyPanel.h"
#include "../../third_party/imgui/imgui.h"
#include "../../BlueprintEditor/BBVariableRegistry.h"

namespace Olympe {

SetBBValuePropertyPanel::SetBBValuePropertyPanel()
    : m_template(nullptr)
{
}

void SetBBValuePropertyPanel::SetNodeName(const std::string& name)
{
    m_nodeName = name;
}

void SetBBValuePropertyPanel::SetTemplate(const TaskGraphTemplate* tmpl)
{
    m_template = tmpl;
}

void SetBBValuePropertyPanel::SetBBKey(const std::string& key)
{
    m_bbKey = key;
}

const std::string& SetBBValuePropertyPanel::GetBBKey() const
{
    return m_bbKey;
}

void SetBBValuePropertyPanel::Render()
{
    RenderTitleSection();
    ImGui::Spacing();

    RenderVariableSelector();
    ImGui::Spacing();

    RenderValueInput();
}

void SetBBValuePropertyPanel::RenderTitleSection()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
    ImGui::Text("SetBBValue: %s", m_nodeName.c_str());
    ImGui::PopStyleColor();
}

void SetBBValuePropertyPanel::RenderVariableSelector()
{
    if (!m_template)
    {
        ImGui::TextDisabled("(no template)");
        return;
    }

    BBVariableRegistry bbReg;
    bbReg.LoadFromTemplate(*m_template);
    const std::vector<VarSpec>& vars = bbReg.GetAllVariables();

    ImGui::Text("Target Variable:");
    ImGui::Indent();

    const char* previewLabel = m_bbKey.empty() ? "(select variable...)" : m_bbKey.c_str();
    ImGui::SetNextItemWidth(200.0f);

    if (ImGui::BeginCombo("##bbvar_target", previewLabel))
    {
        for (size_t i = 0; i < vars.size(); ++i)
        {
            const VarSpec& v = vars[i];
            bool selected = (v.name == m_bbKey);

            // Display with type info (displayLabel already includes type and scope)
            if (ImGui::Selectable(v.displayLabel.c_str(), selected))
            {
                if (m_bbKey != v.name)
                {
                    m_bbKey = v.name;
                    m_dirty = true;
                }
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Unindent();
}

void SetBBValuePropertyPanel::RenderValueInput()
{
    if (m_bbKey.empty() || !m_template)
        return;

    ImGui::Separator();
    ImGui::Text("Value Input:");
    ImGui::Indent();

    // Find the variable to get its type
    for (const auto& entry : m_template->Blackboard)
    {
        if (entry.Key == m_bbKey)
        {
            std::string typeStr;
            switch (entry.Type)
            {
                case VariableType::Bool:   typeStr = "Bool";   break;
                case VariableType::Int:    typeStr = "Int";    break;
                case VariableType::Float:  typeStr = "Float";  break;
                case VariableType::String: typeStr = "String"; break;
                case VariableType::Vector: typeStr = "Vector"; break;
                default:                   typeStr = "Unknown"; break;
            }

            ImGui::Text("Type: %s", typeStr.c_str());
            ImGui::TextDisabled("(Value set at runtime via incoming exec or parameter binding)");

            break;
        }
    }

    ImGui::Unindent();
}

} // namespace Olympe
