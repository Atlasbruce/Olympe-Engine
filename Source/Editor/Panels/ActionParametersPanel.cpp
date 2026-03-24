/**
 * @file ActionParametersPanel.cpp
 * @brief Implementation of ActionParametersPanel for editing action node parameters.
 */

#include "ActionParametersPanel.h"
#include "../../third_party/imgui/imgui.h"
#include <algorithm>
#include <cstring>

namespace Olympe {

ActionParametersPanel::ActionParametersPanel()
    : m_dirty(false)
{
}

void ActionParametersPanel::SetActionTaskID(const std::string& taskID)
{
    m_taskID = taskID;
}

void ActionParametersPanel::SetNodeName(const std::string& name)
{
    m_nodeName = name;
}

void ActionParametersPanel::SetParameters(const std::unordered_map<std::string, std::string>& params)
{
    // Rebuild parameter list from provided map
    m_parameters.clear();

    for (const auto& kv : params)
    {
        ActionParameter param;
        param.name = kv.first;
        param.value = kv.second;
        param.defaultValue = kv.second;  // Assume current value is default if not specified
        param.isDirty = false;

        // Guess type from value (simple heuristic)
        if (kv.second == "true" || kv.second == "false")
        {
            param.type = "Bool";
        }
        else if (std::all_of(kv.second.begin(), kv.second.end(),
                            [](unsigned char c) { return std::isdigit(c) || c == '.'; }))
        {
            param.type = (kv.second.find('.') != std::string::npos) ? "Float" : "Int";
        }
        else
        {
            param.type = "String";
        }

        m_parameters.push_back(param);
    }

    m_dirty = false;
}

const std::vector<ActionParameter>& ActionParametersPanel::GetParameters() const
{
    return m_parameters;
}

std::string ActionParametersPanel::GetParameterValue(const std::string& paramName) const
{
    for (const auto& param : m_parameters)
    {
        if (param.name == paramName)
            return param.value;
    }
    return "";
}

bool ActionParametersPanel::IsDirty() const
{
    if (m_dirty)
        return true;

    for (const auto& param : m_parameters)
    {
        if (param.isDirty)
            return true;
    }
    return false;
}

void ActionParametersPanel::ClearDirty()
{
    m_dirty = false;
    for (auto& param : m_parameters)
    {
        param.isDirty = false;
    }
}

void ActionParametersPanel::Render()
{
#ifndef OLYMPE_HEADLESS
    if (m_taskID.empty())
    {
        ImGui::TextDisabled("(no action selected)");
        return;
    }

    RenderTitleSection();
    ImGui::Spacing();

    if (!m_parameters.empty())
    {
        RenderParametersSection();
    }
    else
    {
        ImGui::TextDisabled("(no parameters for this action)");
    }
#endif
}

void ActionParametersPanel::RenderTitleSection()
{
#ifndef OLYMPE_HEADLESS
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    ImGui::Text("Action: %s", m_nodeName.c_str());
    ImGui::PopStyleColor();
    ImGui::TextDisabled("Task ID: %s", m_taskID.c_str());
#endif
}

void ActionParametersPanel::RenderParametersSection()
{
#ifndef OLYMPE_HEADLESS
    ImGui::Separator();
    ImGui::Text("Parameters:");
    ImGui::Spacing();

    for (size_t i = 0; i < m_parameters.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        RenderParameter(m_parameters[i]);
        ImGui::PopID();
    }
#endif
}

void ActionParametersPanel::RenderParameter(ActionParameter& param)
{
#ifndef OLYMPE_HEADLESS
    ImGui::Indent();

    // Parameter label with type hint
    std::string label = param.name + " (" + param.type + ")";

    if (param.type == "Bool")
    {
        bool value = (param.value == "true" || param.value == "1");
        if (ImGui::Checkbox(label.c_str(), &value))
        {
            param.value = value ? "true" : "false";
            param.isDirty = true;
            m_dirty = true;
        }
    }
    else if (param.type == "Int")
    {
        int value = 0;
        try {
            value = std::stoi(param.value);
        } catch (...) {
            // Keep default 0
        }
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::DragInt(label.c_str(), &value))
        {
            param.value = std::to_string(value);
            param.isDirty = true;
            m_dirty = true;
        }
    }
    else if (param.type == "Float")
    {
        float value = 0.0f;
        try {
            value = std::stof(param.value);
        } catch (...) {
            // Keep default 0.0f
        }
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::DragFloat(label.c_str(), &value, 0.1f))
        {
            param.value = std::to_string(value);
            param.isDirty = true;
            m_dirty = true;
        }
    }
    else if (param.type == "String")
    {
        // For path references or generic strings
        static char buffer[512] = {0};
        strncpy_s(buffer, param.value.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        ImGui::SetNextItemWidth(250.0f);
        if (ImGui::InputText(label.c_str(), buffer, sizeof(buffer)))
        {
            param.value = buffer;
            param.isDirty = true;
            m_dirty = true;
        }
    }
    else
    {
        // Unknown type: display as text
        ImGui::TextDisabled("%s: %s", label.c_str(), param.value.c_str());
    }

    ImGui::Unindent();
#endif
}

} // namespace Olympe
