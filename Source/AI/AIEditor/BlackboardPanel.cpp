/**
 * @file BlackboardPanel.cpp
 * @brief Implementation of BlackboardPanel ImGui panel (Phase 2.1)
 * @author Olympe Engine
 * @date 2026-02-19
 */

#include "BlackboardPanel.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"
#include <cstring>
#include <cstdio>

namespace Olympe {
namespace AI {

// ============================================================================
// Constructor
// ============================================================================

BlackboardPanel::BlackboardPanel()
    : m_showAddDialog(false)
    , m_showEditPopup(false)
    , m_newEntryTypeIndex(0)
{
    std::memset(m_newEntryName,  0, sizeof(m_newEntryName));
    std::memset(m_editStringBuf, 0, sizeof(m_editStringBuf));
    std::memset(m_renameBuffer,  0, sizeof(m_renameBuffer));
}

// ============================================================================
// Render
// ============================================================================

void BlackboardPanel::Render(NodeGraph::BlackboardSystem* blackboard, bool* pOpen)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("Blackboard", pOpen, flags))
    {
        ImGui::End();
        return;
    }

    if (blackboard == nullptr)
    {
        ImGui::Text("No active graph.");
        ImGui::End();
        return;
    }

    ImGui::Text("Blackboard Variables");
    ImGui::Separator();

    if (ImGui::Button("Add Variable"))
    {
        m_showAddDialog = true;
        std::memset(m_newEntryName, 0, sizeof(m_newEntryName));
        m_newEntryTypeIndex = 0;
        m_editBuffer = NodeGraph::BlackboardValue();
        std::memset(m_editStringBuf, 0, sizeof(m_editStringBuf));
    }

    ImGui::Separator();
    RenderEntryList(blackboard);

    if (m_showAddDialog)
    {
        RenderAddDialog(blackboard);
    }

    if (m_showEditPopup)
    {
        RenderEditPopup(blackboard);
    }

    ImGui::End();
}

// ============================================================================
// Entry list
// ============================================================================

static const char* s_typeNames[] = { "Int", "Float", "Bool", "String", "Vector3" };
static const int   s_typeCount   = 5;

void BlackboardPanel::RenderEntryList(NodeGraph::BlackboardSystem* blackboard)
{
    const auto& entries = blackboard->GetAll();

    if (entries.empty())
    {
        ImGui::TextDisabled("(no variables)");
        return;
    }

    // Column headers
    ImGui::Columns(3, "bb_cols");
    ImGui::SetColumnWidth(0, 140.0f);
    ImGui::SetColumnWidth(1, 70.0f);
    ImGui::Text("Name");   ImGui::NextColumn();
    ImGui::Text("Type");   ImGui::NextColumn();
    ImGui::Text("Value");  ImGui::NextColumn();
    ImGui::Separator();

    std::string toDelete;
    std::string toEdit;

    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        const std::string& name = it->first;
        const NodeGraph::BlackboardValue& val = it->second;

        ImGui::PushID(name.c_str());

        // Name column
        ImGui::Text("%s", name.c_str());
        ImGui::NextColumn();

        // Type column
        int typeIdx = static_cast<int>(val.type);
        if (typeIdx >= 0 && typeIdx < s_typeCount)
        {
            ImGui::Text("%s", s_typeNames[typeIdx]);
        }
        ImGui::NextColumn();

        // Value column with inline editing
        char valueBuf[256];
        std::memset(valueBuf, 0, sizeof(valueBuf));

        switch (val.type)
        {
        case NodeGraph::BlackboardType::Int:
            std::snprintf(valueBuf, sizeof(valueBuf), "%d", val.intValue);
            break;
        case NodeGraph::BlackboardType::Float:
            std::snprintf(valueBuf, sizeof(valueBuf), "%.3f", val.floatValue);
            break;
        case NodeGraph::BlackboardType::Bool:
            std::snprintf(valueBuf, sizeof(valueBuf), "%s", val.boolValue ? "true" : "false");
            break;
        case NodeGraph::BlackboardType::String:
            std::snprintf(valueBuf, sizeof(valueBuf), "%s", val.stringValue.c_str());
            break;
        case NodeGraph::BlackboardType::Vector3:
            std::snprintf(valueBuf, sizeof(valueBuf), "(%.2f, %.2f, %.2f)",
                          val.vec3X, val.vec3Y, val.vec3Z);
            break;
        default:
            break;
        }
        ImGui::Text("%s", valueBuf);

        // Context menu for edit/delete
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
        {
            ImGui::OpenPopup("EntryCtx");
        }
        if (ImGui::BeginPopup("EntryCtx"))
        {
            if (ImGui::MenuItem("Edit"))
            {
                toEdit = name;
            }
            if (ImGui::MenuItem("Delete"))
            {
                toDelete = name;
            }
            ImGui::EndPopup();
        }

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    if (!toDelete.empty())
    {
        blackboard->RemoveEntry(toDelete);
    }

    if (!toEdit.empty())
    {
        m_editTargetName = toEdit;
        const NodeGraph::BlackboardValue* ev = blackboard->GetEntry(toEdit);
        if (ev != nullptr)
        {
            m_editBuffer = *ev;
            std::memset(m_editStringBuf, 0, sizeof(m_editStringBuf));
            std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
            std::snprintf(m_renameBuffer, sizeof(m_renameBuffer), "%s", toEdit.c_str());
            if (ev->type == NodeGraph::BlackboardType::String)
            {
                std::snprintf(m_editStringBuf, sizeof(m_editStringBuf),
                              "%s", ev->stringValue.c_str());
            }
        }
        m_showEditPopup = true;
    }
}

// ============================================================================
// Add dialog
// ============================================================================

void BlackboardPanel::RenderAddDialog(NodeGraph::BlackboardSystem* blackboard)
{
    ImGui::OpenPopup("Add Variable");

    if (ImGui::BeginPopupModal("Add Variable", &m_showAddDialog,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Name", m_newEntryName, sizeof(m_newEntryName));
        ImGui::Combo("Type", &m_newEntryTypeIndex, s_typeNames, s_typeCount);

        NodeGraph::BlackboardType selectedType =
            static_cast<NodeGraph::BlackboardType>(m_newEntryTypeIndex);

        ImGui::Separator();
        ImGui::Text("Initial value:");

        switch (selectedType)
        {
        case NodeGraph::BlackboardType::Int:
            ImGui::InputInt("##int", &m_editBuffer.intValue);
            break;
        case NodeGraph::BlackboardType::Float:
            ImGui::InputFloat("##float", &m_editBuffer.floatValue);
            break;
        case NodeGraph::BlackboardType::Bool:
            ImGui::Checkbox("##bool", &m_editBuffer.boolValue);
            break;
        case NodeGraph::BlackboardType::String:
            ImGui::InputText("##string", m_editStringBuf, sizeof(m_editStringBuf));
            break;
        case NodeGraph::BlackboardType::Vector3:
        {
            float v[3] = { m_editBuffer.vec3X, m_editBuffer.vec3Y, m_editBuffer.vec3Z };
            if (ImGui::InputFloat3("##vec3", v))
            {
                m_editBuffer.vec3X = v[0];
                m_editBuffer.vec3Y = v[1];
                m_editBuffer.vec3Z = v[2];
            }
            break;
        }
        default:
            break;
        }

        ImGui::Separator();

        if (ImGui::Button("Create"))
        {
            if (m_newEntryName[0] != '\0')
            {
                NodeGraph::BlackboardValue initVal = m_editBuffer;
                initVal.type = selectedType;
                if (selectedType == NodeGraph::BlackboardType::String)
                {
                    initVal.stringValue = std::string(m_editStringBuf);
                }
                blackboard->CreateEntry(std::string(m_newEntryName), selectedType, initVal);
            }
            m_showAddDialog = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_showAddDialog = false;
        }

        ImGui::EndPopup();
    }
}

// ============================================================================
// Edit popup
// ============================================================================

void BlackboardPanel::RenderEditPopup(NodeGraph::BlackboardSystem* blackboard)
{
    ImGui::OpenPopup("Edit Variable");

    if (ImGui::BeginPopupModal("Edit Variable", &m_showEditPopup,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Rename", m_renameBuffer, sizeof(m_renameBuffer));

        ImGui::Separator();
        ImGui::Text("Value:");

        switch (m_editBuffer.type)
        {
        case NodeGraph::BlackboardType::Int:
            ImGui::InputInt("##int", &m_editBuffer.intValue);
            break;
        case NodeGraph::BlackboardType::Float:
            ImGui::InputFloat("##float", &m_editBuffer.floatValue);
            break;
        case NodeGraph::BlackboardType::Bool:
            ImGui::Checkbox("##bool", &m_editBuffer.boolValue);
            break;
        case NodeGraph::BlackboardType::String:
            ImGui::InputText("##string", m_editStringBuf, sizeof(m_editStringBuf));
            break;
        case NodeGraph::BlackboardType::Vector3:
        {
            float v[3] = { m_editBuffer.vec3X, m_editBuffer.vec3Y, m_editBuffer.vec3Z };
            if (ImGui::InputFloat3("##vec3", v))
            {
                m_editBuffer.vec3X = v[0];
                m_editBuffer.vec3Y = v[1];
                m_editBuffer.vec3Z = v[2];
            }
            break;
        }
        default:
            break;
        }

        ImGui::Separator();

        if (ImGui::Button("Apply"))
        {
            // Apply value change
            if (m_editBuffer.type == NodeGraph::BlackboardType::String)
            {
                m_editBuffer.stringValue = std::string(m_editStringBuf);
            }
            blackboard->SetValue(m_editTargetName, m_editBuffer);

            // Handle rename
            std::string newName(m_renameBuffer);
            if (!newName.empty() && newName != m_editTargetName)
            {
                blackboard->RenameEntry(m_editTargetName, newName);
            }

            m_showEditPopup = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_showEditPopup = false;
        }

        ImGui::EndPopup();
    }
}

} // namespace AI
} // namespace Olympe
