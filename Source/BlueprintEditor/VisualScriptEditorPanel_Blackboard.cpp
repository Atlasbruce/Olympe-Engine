/**
 * @file VisualScriptEditorPanel_Blackboard.cpp
 * @brief Blackboard and variables panel rendering (Phase 12).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * Extracted methods:
 *   - RenderBlackboard() ~150 LOC
 *   - RenderLocalVariablesPanel() ~120 LOC
 *   - RenderGlobalVariablesPanel() ~80 LOC
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"
#include "../third_party/imgui/imgui.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <unordered_map>
#include <algorithm>

namespace Olympe {

void VisualScriptEditorPanel::RenderBlackboard()
{
    ImGui::TextDisabled("Local Blackboard");
    ImGui::Separator();

    bool hasInvalid = false;
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(i)];
        if (entry.Key.empty() || entry.Type == VariableType::None)
        {
            hasInvalid = true;
            break;
        }
    }
    if (hasInvalid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextUnformatted("[!] Invalid entries will be skipped on save");
        ImGui::PopStyleColor();
    }

    if (ImGui::Button("+##vsbbAdd"))
    {
        BlackboardEntry entry;
        entry.Key      = "NewVariable";
        entry.Type     = VariableType::Int;
        entry.Default  = GetDefaultValueForType(VariableType::Int);
        entry.IsGlobal = false;
        m_template.Blackboard.push_back(entry);
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add key");

    for (int idx = static_cast<int>(m_template.Blackboard.size()) - 1; idx >= 0; --idx)
    {
        BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(idx)];

        ImGui::PushID(idx);

        char keyBuf[64];
        strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
        {
            entry.Key = keyBuf;
            m_dirty   = true;
        }
        ImGui::SameLine();

        const char* typeLabels[] = {"Bool","Int","Float","Vector","EntityID","String"};
        int typeIdx = static_cast<int>(entry.Type) - 1;
        if (typeIdx < 0 || typeIdx >= 6)
        {
            typeIdx    = 1;
            entry.Type = VariableType::Int;
        }
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &typeIdx, typeLabels, 6))
        {
            VariableType newType = static_cast<VariableType>(typeIdx + 1);
            entry.Type    = newType;
            entry.Default = GetDefaultValueForType(newType);
            m_dirty       = true;
        }
        ImGui::SameLine();

        ImGui::Checkbox("G##bbglob", &entry.IsGlobal);
        ImGui::SameLine();

        if (ImGui::SmallButton("x##bbdel"))
        {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
            m_pendingBlackboardEdits.erase(idx);
            m_dirty = true;
            ImGui::PopID();
            continue;
        }

        ImGui::TextDisabled("Default:");
        ImGui::SameLine();
        switch (entry.Type)
        {
            case VariableType::Bool:
            {
                bool bVal = entry.Default.IsNone() ? false : entry.Default.AsBool();
                if (ImGui::Checkbox("##bbval", &bVal))
                {
                    entry.Default = TaskValue(bVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Int:
            {
                int iVal = entry.Default.IsNone() ? 0 : entry.Default.AsInt();
                ImGui::SetNextItemWidth(70.0f);
                if (ImGui::InputInt("##bbval", &iVal))
                {
                    entry.Default = TaskValue(iVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Float:
            {
                float fVal = entry.Default.IsNone() ? 0.0f : entry.Default.AsFloat();
                ImGui::SetNextItemWidth(70.0f);
                if (ImGui::InputFloat("##bbval", &fVal, 0.0f, 0.0f, "%.3f"))
                {
                    entry.Default = TaskValue(fVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::String:
            {
                std::string sVal = entry.Default.IsNone() ? "" : entry.Default.AsString();
                char sBuf[128];
                strncpy_s(sBuf, sizeof(sBuf), sVal.c_str(), _TRUNCATE);
                ImGui::SetNextItemWidth(100.0f);
                if (ImGui::InputText("##bbval", sBuf, sizeof(sBuf)))
                {
                    entry.Default = TaskValue(std::string(sBuf));
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Vector:
            {
                ImGui::BeginDisabled(true);
                float vecVal[3] = { 0.0f, 0.0f, 0.0f };
                ImGui::SetNextItemWidth(140.0f);
                ImGui::DragFloat3("##bbval", vecVal, 0.1f);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("(auto from entity position)");
                break;
            }
            case VariableType::EntityID:
            {
                ImGui::BeginDisabled(true);
                int entityId = 0;
                ImGui::SetNextItemWidth(70.0f);
                ImGui::InputInt("##bbval", &entityId);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("(assigned at runtime)");
                break;
            }
            default:
                ImGui::TextDisabled("(n/a)");
                break;
        }

        ImGui::PopID();
    }
}

void VisualScriptEditorPanel::RenderLocalVariablesPanel()
{
    ImGui::TextDisabled("Local Blackboard");
    ImGui::Separator();

    bool hasInvalid = false;
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(i)];
        if (entry.Key.empty() || entry.Type == VariableType::None)
        {
            hasInvalid = true;
            break;
        }
    }
    if (hasInvalid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextUnformatted("[!] Invalid entries will be skipped on save");
        ImGui::PopStyleColor();
    }

    if (ImGui::Button("+##vsbbAdd"))
    {
        BlackboardEntry entry;
        entry.Key      = "NewVariable";
        entry.Type     = VariableType::Int;
        entry.Default  = GetDefaultValueForType(VariableType::Int);
        entry.IsGlobal = false;
        m_template.Blackboard.push_back(entry);
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add key");

    for (int idx = static_cast<int>(m_template.Blackboard.size()) - 1; idx >= 0; --idx)
    {
        BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(idx)];

        ImGui::PushID(idx);

        char keyBuf[64];
        strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);

        // ====== VALIDATION: Key input with error feedback ======
        ImGui::SetNextItemWidth(140.0f);
        if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
        {
            std::string newKey(keyBuf);
            BlackboardValidationResult validation = ValidateBlackboardKey(newKey, entry.IsGlobal, idx);

            if (validation.IsValid)
            {
                entry.Key = newKey;
                m_pendingBlackboardEdits[idx] = newKey;
                m_dirty = true;
            }
            // If invalid, we display the error below but don't update entry.Key
        }

        // Display validation feedback
        if (!entry.Key.empty() && entry.Type != VariableType::None)
        {
            BlackboardValidationResult validation = ValidateBlackboardKey(entry.Key, entry.IsGlobal, idx);

            if (!validation.IsValid)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));  // Red for error
                ImGui::SameLine();
                ImGui::TextDisabled("/!\\ %s", validation.ErrorMessage.c_str());
                ImGui::PopStyleColor();
            }
            else if (!validation.WarningMessage.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));  // Yellow for warning
                ImGui::SameLine();
                ImGui::TextDisabled("[i] %s", validation.WarningMessage.c_str());
                ImGui::PopStyleColor();
            }
        }

        ImGui::SameLine();

        const char* typeNames[] = { "None", "Bool", "Int", "Float", "String", "Vector" };
        const VariableType typeValues[] = {
            VariableType::None, VariableType::Bool, VariableType::Int,
            VariableType::Float, VariableType::String, VariableType::Vector
        };
        int curTypeIdx = 0;
        for (int ti = 0; ti < 6; ++ti)
            if (entry.Type == typeValues[ti])
            { curTypeIdx = ti; break; }

        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &curTypeIdx, typeNames, 6))
        {
            entry.Type = typeValues[curTypeIdx];
            entry.Default = GetDefaultValueForType(entry.Type);
            m_dirty = true;
        }

        if (entry.Type != VariableType::None)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("Default:");
            ImGui::SameLine();
            RenderConstValueInput(entry.Default, entry.Type, "##bbdefault");
        }

        ImGui::SameLine();
        bool isGlobal = entry.IsGlobal;
        if (ImGui::Checkbox("G##bbglobal", &isGlobal))
        {
            entry.IsGlobal = isGlobal;
            m_dirty = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Mark as global variable");

        ImGui::SameLine();
        if (ImGui::Button("X##bbdel"))
        {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
            m_pendingBlackboardEdits.erase(idx);
            m_dirty = true;
        }

        ImGui::PopID();
    }
}

void VisualScriptEditorPanel::RenderGlobalVariablesPanel()
{
    ImGui::TextDisabled("Global Variables (Editor Instance)");
    ImGui::Separator();

    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

    if (ImGui::Button("+##globalVarAdd", ImVec2(30, 0)))
    {
        ImGui::OpenPopup("AddGlobalVariablePopup");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add global variable");

    if (ImGui::BeginPopupModal("AddGlobalVariablePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char newVarName[128] = "newGlobal";
        static int newVarTypeIdx = 2;
        static char newVarDescription[256] = "Enter description...";

        ImGui::InputText("Variable Name##new", newVarName, sizeof(newVarName));

        // Validate global variable name
        std::string globalName(newVarName);
        BlackboardValidationResult validation = ValidateBlackboardKey(globalName, true);

        if (strlen(newVarName) > 0)
        {
            if (!validation.IsValid)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));  // Red
                ImGui::TextUnformatted(validation.ErrorMessage.c_str());
                ImGui::PopStyleColor();
            }
            else if (!validation.WarningMessage.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));  // Yellow
                ImGui::TextUnformatted(validation.WarningMessage.c_str());
                ImGui::PopStyleColor();
            }

            // Also check if already exists in global blackboard
            if (gtb.HasVariable(globalName))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::TextUnformatted("/!\\ Variable already exists in global registry");
                ImGui::PopStyleColor();
            }
        }

        const char* typeOptions[] = { "Bool", "Int", "Float", "String", "Vector", "EntityID" };
        const VariableType typeValues[] = {
            VariableType::Bool, VariableType::Int, VariableType::Float,
            VariableType::String, VariableType::Vector, VariableType::EntityID
        };
        ImGui::Combo("Type##new", &newVarTypeIdx, typeOptions, 6);

        ImGui::InputTextMultiline("Description##new", newVarDescription, sizeof(newVarDescription), ImVec2(0, 60));

        // Disable Create button if validation fails
        bool canCreate = strlen(newVarName) > 0 && validation.IsValid && !gtb.HasVariable(newVarName);

        if (!canCreate)
            ImGui::BeginDisabled(true);

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (strlen(newVarName) > 0 && !gtb.HasVariable(newVarName))
            {
                TaskValue defaultVal = GetDefaultValueForType(typeValues[newVarTypeIdx]);
                if (gtb.AddVariable(newVarName, typeValues[newVarTypeIdx], defaultVal, newVarDescription, false))
                {
                    SYSTEM_LOG << "[VSEditor] Created new global variable: " << newVarName << "\n";
                    gtb.SaveToFile();

                    GlobalTemplateBlackboard::Reload();

                    m_dirty = true;

                    memset(newVarName, 0, sizeof(newVarName));
                    strcpy_s(newVarName, sizeof(newVarName), "newGlobal");
                    newVarTypeIdx = 2;
                    memset(newVarDescription, 0, sizeof(newVarDescription));
                    strcpy_s(newVarDescription, sizeof(newVarDescription), "Enter description...");

                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        if (!canCreate)
            ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    ImGui::Separator();

    if (globalVars.empty())
    {
        ImGui::TextDisabled("(no global variables defined)");
        ImGui::TextDisabled("Click [+] above to create new global variables");
        return;
    }

    ImGui::TextDisabled("Global variables from project registry");
    ImGui::TextDisabled("Values shown are editor-specific (persisted with graph)");
    ImGui::Separator();

    if (!m_entityBlackboard)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "[ERROR] EntityBlackboard not initialized");
        return;
    }

    for (size_t gi = 0; gi < globalVars.size(); ++gi)
    {
        const GlobalEntryDefinition& globalDef = globalVars[gi];

        ImGui::PushID(static_cast<int>(gi));

        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "(%s) %s",
                          VariableTypeToString(globalDef.Type).c_str(),
                          globalDef.Key.c_str());

        ImGui::SameLine();
        ImGui::TextDisabled("(%.1f KB)", 0.1f);
        ImGui::SameLine();

        if (ImGui::SmallButton("Delete##globalvar"))
        {
            std::string varToDelete = globalDef.Key;
            if (gtb.RemoveVariable(varToDelete))
            {
                SYSTEM_LOG << "[VSEditor] Deleted global variable: " << varToDelete << "\n";
                gtb.SaveToFile();

                GlobalTemplateBlackboard::Reload();

                m_dirty = true;
            }
            ImGui::PopID();
            continue;
        }

        if (!globalDef.Description.empty())
        {
            ImGui::TextDisabled("  %s", globalDef.Description.c_str());
        }

        std::string tableId = "##GlobalVarTable_" + std::to_string(gi);
        if (ImGui::BeginTable(tableId.c_str(), 2, ImGuiTableFlags_SizingStretchSame, ImVec2(0, 0)))
        {
            ImGui::TableSetupColumn("Label", 0);
            ImGui::TableSetupColumn("Value", 0);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Default:");
            ImGui::TableSetColumnIndex(1);

            const TaskValue& defaultValue = globalDef.DefaultValue;
            std::string defaultStr;
            switch (globalDef.Type)
            {
                case VariableType::Bool:
                    defaultStr = defaultValue.IsNone() ? "false" : (defaultValue.AsBool() ? "true" : "false");
                    break;
                case VariableType::Int:
                    defaultStr = defaultValue.IsNone() ? "0" : std::to_string(defaultValue.AsInt());
                    break;
                case VariableType::Float:
                {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2);
                    oss << (defaultValue.IsNone() ? 0.0f : defaultValue.AsFloat());
                    defaultStr = oss.str();
                    break;
                }
                case VariableType::String:
                    defaultStr = defaultValue.IsNone() ? "" : defaultValue.AsString();
                    break;
                case VariableType::Vector:
                    defaultStr = "(vector)";
                    break;
                case VariableType::EntityID:
                    defaultStr = defaultValue.IsNone() ? "0" : std::to_string(static_cast<int>(defaultValue.AsEntityID()));
                    break;
                default:
                    defaultStr = "(unknown)";
                    break;
            }
            ImGui::TextDisabled("%s", defaultStr.c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Current:");
            ImGui::TableSetColumnIndex(1);

            std::string scopedVarName = "(G)" + globalDef.Key;
            TaskValue currentValue = m_entityBlackboard->GetValueScoped(scopedVarName);

            bool valueChanged = false;
            switch (globalDef.Type)
            {
                case VariableType::Bool:
                {
                    bool bVal = currentValue.IsNone() ? false : currentValue.AsBool();
                    if (ImGui::Checkbox("##bool_val", &bVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(bVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Int:
                {
                    int iVal = currentValue.IsNone() ? 0 : currentValue.AsInt();
                    if (ImGui::InputInt("##int_val", &iVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(iVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Float:
                {
                    float fVal = currentValue.IsNone() ? 0.0f : currentValue.AsFloat();
                    if (ImGui::InputFloat("##float_val", &fVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(fVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::String:
                {
                    static std::unordered_map<size_t, std::vector<char>> stringBuffers;
                    size_t bufKey = gi;
                    if (stringBuffers.find(bufKey) == stringBuffers.end())
                    {
                        std::string initialStr = currentValue.IsNone() ? "" : currentValue.AsString();
                        stringBuffers[bufKey] = std::vector<char>(initialStr.begin(), initialStr.end());
                        stringBuffers[bufKey].push_back('\0');
                        stringBuffers[bufKey].resize(256);
                    }

                    ImGui::SetNextItemWidth(-1.0f);
                    if (ImGui::InputText("##string_val", stringBuffers[bufKey].data(), 256, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        std::string newStr(stringBuffers[bufKey].data());
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(newStr));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Vector:
                {
                    Vector vVal = currentValue.IsNone() ? Vector{0.0f, 0.0f, 0.0f} : currentValue.AsVector();
                    float vArray[3] = {vVal.x, vVal.y, vVal.z};
                    if (ImGui::InputFloat3("##vector_val", vArray))
                    {
                        Vector newVec{vArray[0], vArray[1], vArray[2]};
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(newVec));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::EntityID:
                {
                    int eID = currentValue.IsNone() ? 0 : static_cast<int>(currentValue.AsEntityID());
                    if (ImGui::InputInt("##entityid_val", &eID))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(eID >= 0 ? eID : 0));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                default:
                    ImGui::TextDisabled("(unsupported type)");
                    break;
            }

            if (globalDef.IsPersistent)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("Flags:");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.5f, 1.0f), "[Persistent]");
            }

            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::PopID();
    }
}

// ============================================================================
// Phase 26 — Tab-based panel system for right side
// ============================================================================

void VisualScriptEditorPanel::RenderRightPanelTabs()
{
    /// Renders the tab bar for selecting between Presets, Local Variables, and Global Variables
    /// Uses a horizontal button group with visual feedback for the active tab

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));

    // Tab 0: Presets
    {
        bool isActive = (m_rightPanelTabSelection == 0);
        if (isActive)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.7f));

        if (ImGui::Button("Presets##tab0", ImVec2(0, 0)))
            m_rightPanelTabSelection = 0;

        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // Tab 1: Local Variables
    {
        bool isActive = (m_rightPanelTabSelection == 1);
        if (isActive)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.7f));

        if (ImGui::Button("Local Variables##tab1", ImVec2(0, 0)))
            m_rightPanelTabSelection = 1;

        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // Tab 2: Global Variables
    {
        bool isActive = (m_rightPanelTabSelection == 2);
        if (isActive)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.7f));

        if (ImGui::Button("Global Variables##tab2", ImVec2(0, 0)))
            m_rightPanelTabSelection = 2;

        ImGui::PopStyleColor();
    }

    ImGui::PopStyleVar(2);
    ImGui::Separator();
}

void VisualScriptEditorPanel::RenderRightPanelTabContent()
{
    /// Renders the content of the currently selected tab
    /// Dispatches to the appropriate render function based on m_rightPanelTabSelection

    switch (m_rightPanelTabSelection)
    {
        case 0:
            // Tab 0: Presets
            RenderPresetBankPanel();
            break;

        case 1:
            // Tab 1: Local Variables
            RenderLocalVariablesPanel();
            break;

        case 2:
            // Tab 2: Global Variables
            RenderGlobalVariablesPanel();
            break;

        default:
            // Fallback: show presets
            RenderPresetBankPanel();
            break;
    }
}

} // namespace Olympe
