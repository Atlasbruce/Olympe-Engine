/**
 * @file GetBBValuePropertyPanel.cpp
 * @brief Implementation of GetBBValue properties panel.
 */

#include "GetBBValuePropertyPanel.h"
#include "../../third_party/imgui/imgui.h"
#include "../../BlueprintEditor/BBVariableRegistry.h"

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

GetBBValuePropertyPanel::GetBBValuePropertyPanel()
    : m_template(nullptr)
{
}

// ============================================================================
// State accessors
// ============================================================================

void GetBBValuePropertyPanel::SetNodeName(const std::string& name)
{
    m_nodeName = name;
}

void GetBBValuePropertyPanel::SetTemplate(const TaskGraphTemplate* tmpl)
{
    m_template = tmpl;
}

void GetBBValuePropertyPanel::SetBBKey(const std::string& key)
{
    m_bbKey = key;
}

const std::string& GetBBValuePropertyPanel::GetBBKey() const
{
    return m_bbKey;
}

// ============================================================================
// Rendering
// ============================================================================

void GetBBValuePropertyPanel::Render()
{
    RenderTitleSection();
    ImGui::Spacing();

    RenderVariableSelector();
    ImGui::Spacing();

    RenderVariableInfo();
}

void GetBBValuePropertyPanel::RenderTitleSection()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::Text("GetBBValue: %s", m_nodeName.c_str());
    ImGui::PopStyleColor();
}

void GetBBValuePropertyPanel::RenderVariableSelector()
{
    if (!m_template)
    {
        ImGui::TextDisabled("(no template)");
        return;
    }

    BBVariableRegistry bbReg;
    bbReg.LoadFromTemplate(*m_template);
    const std::vector<VarSpec>& vars = bbReg.GetAllVariables();

    ImGui::Text("Select Variable:");
    ImGui::Indent();

    const char* previewLabel = m_bbKey.empty() ? "(select variable...)" : m_bbKey.c_str();
    ImGui::SetNextItemWidth(200.0f);

    if (ImGui::BeginCombo("##bbvar_combo", previewLabel))
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

void GetBBValuePropertyPanel::RenderVariableInfo()
{
    if (m_bbKey.empty() || !m_template)
        return;

    ImGui::Separator();
    ImGui::Text("Variable Info:");
    ImGui::Indent();

    // Find the variable in template
    for (const auto& entry : m_template->Blackboard)
    {
        if (entry.Key == m_bbKey)
        {
            // Display type
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

            // Display default value
            if (!entry.Default.IsNone())
            {
                ImGui::Text("Default: ");
                ImGui::SameLine();
                switch (entry.Type)
                {
                    case VariableType::Bool:   ImGui::Text("%s", entry.Default.AsBool() ? "true" : "false"); break;
                    case VariableType::Int:    ImGui::Text("%d", entry.Default.AsInt());   break;
                    case VariableType::Float:  ImGui::Text("%.3f", entry.Default.AsFloat()); break;
                    case VariableType::String: ImGui::Text("\"%s\"", entry.Default.AsString().c_str()); break;
                    case VariableType::Vector:
                    {
                        const ::Vector v = entry.Default.AsVector();
                        ImGui::Text("(%.2f, %.2f, %.2f)", v.x, v.y, v.z);
                        break;
                    }
                    default: ImGui::Text("(N/A)"); break;
                }
            }

            break;
        }
    }

    ImGui::Unindent();
}

} // namespace Olympe
