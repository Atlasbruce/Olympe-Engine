// ============================================================================
// VisualScriptEditorPanel_Blackboard.cpp
// Variable Management (Local/Global Blackboard)
// ============================================================================
//
// This file contains all blackboard rendering and variable management code
// for the Blueprint Editor.
//
// Methods:
// - RenderBlackboard()                : Main blackboard variable panel
// - RenderLocalVariablesPanel()       : Local variables sub-panel
// - RenderGlobalVariablesPanel()      : Global variables sub-panel
// - CommitPendingBlackboardEdits()    : Deferred edit commit
// - ValidateAndCleanBlackboardEntries(): Validate before save
// - GetVariablesByType()              : Filter variables by type
//
// Integration Points:
// - m_template.Blackboard            : Local variable storage
// - GlobalTemplateBlackboard          : Global variable registry
// - EntityBlackboard                  : Entity-specific variables
// ============================================================================

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"
#include "BBVariableRegistry.h"
#include "MathOpOperand.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../json_helper.h"
#include "../TaskSystem/TaskGraphLoader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <unordered_set>

namespace Olympe {

void VisualScriptEditorPanel::ValidateAndCleanBlackboardEntries()
{
    std::vector<BlackboardEntry>& entries = m_template.Blackboard;
    size_t before = entries.size();

    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [](const BlackboardEntry& e) {
                if (e.Key.empty()) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry with empty key\n";
                    return true;
                }
                if (e.Type == VariableType::None) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry '"
                               << e.Key << "' with VariableType::None\n";
                    return true;
                }
                return false;
            }),
        entries.end());

    size_t removed = before - entries.size();
    if (removed > 0)
    {
        SYSTEM_LOG << "[VSEditor] ValidateAndClean: removed " << removed
                   << " invalid blackboard entries\n";
        m_dirty = true;
    }
}


void VisualScriptEditorPanel::CommitPendingBlackboardEdits()
{
    for (std::unordered_map<int, std::string>::iterator it = m_pendingBlackboardEdits.begin();
         it != m_pendingBlackboardEdits.end(); ++it)
    {
        int idx = it->first;
        if (idx >= 0 && idx < static_cast<int>(m_template.Blackboard.size()))
        {
            m_template.Blackboard[static_cast<size_t>(idx)].Key = it->second;
        }
    }
    m_pendingBlackboardEdits.clear();
}

// ============================================================================
// BUG-003 Viewport helpers
// ============================================================================


std::vector<BlackboardEntry> VisualScriptEditorPanel::GetVariablesByType(
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType)
{
    std::vector<BlackboardEntry> filtered;
    for (size_t i = 0; i < allVars.size(); ++i)
    {
        if (allVars[i].Type == expectedType)
            filtered.push_back(allVars[i]);
    }
    return filtered;
}

// ============================================================================
// Rendering
// ============================================================================

// ============================================================================
// Undo/Redo wrappers
// ============================================================================


void VisualScriptEditorPanel::RenderBlackboard()
{
    ImGui::TextDisabled("Local Blackboard");
    ImGui::Separator();

    // BUG-001 Hotfix: warn user if invalid entries exist (key empty or type None)
    // to prevent save crash caused by unhandled None type during serialization.
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

    // Add entry button — BUG-001 Hotfix: init with safe defaults (non-empty key, Int type)
    if (ImGui::Button("+##vsbbAdd"))
    {
        BlackboardEntry entry;
        entry.Key      = "NewVariable";
        entry.Type     = VariableType::Int;
        entry.Default  = GetDefaultValueForType(VariableType::Int);  // UX Fix #1
        entry.IsGlobal = false;
        m_template.Blackboard.push_back(entry);
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add key");

    // List existing entries
    for (int idx = static_cast<int>(m_template.Blackboard.size()) - 1; idx >= 0; --idx)
    {
        BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(idx)];

        ImGui::PushID(idx);

        // Use a local (non-static) buffer per iteration to avoid sharing across entries
        char keyBuf[64];
        strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
        {
            entry.Key = keyBuf;
            m_dirty   = true;
        }
        ImGui::SameLine();

        // Fix #2: Type selector — "None" is excluded to prevent invalid entries.
        // Enum layout: None=0, Bool=1, Int=2, Float=3, Vector=4, EntityID=5, String=6.
        // typeIdx maps to enum value minus 1 (offset by 1 to skip None).
        const char* typeLabels[] = {"Bool","Int","Float","Vector","EntityID","String"};
        int typeIdx = static_cast<int>(entry.Type) - 1; // offset: Bool->0, Int->1, ...
        if (typeIdx < 0 || typeIdx >= 6)
        {
            typeIdx    = 1; // default to "Int" (array index 1; maps to VariableType::Int via typeIdx+1)
            entry.Type = VariableType::Int;
        }
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &typeIdx, typeLabels, 6))
        {
            VariableType newType = static_cast<VariableType>(typeIdx + 1); // +1 to skip None
            entry.Type    = newType;
            entry.Default = GetDefaultValueForType(newType);  // UX Fix #1: sync default
            m_dirty       = true;
        }
        ImGui::SameLine();

        // IsGlobal checkbox
        ImGui::Checkbox("G##bbglob", &entry.IsGlobal);
        ImGui::SameLine();

        // Remove button
        if (ImGui::SmallButton("x##bbdel"))
        {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
            m_pendingBlackboardEdits.erase(idx);
            m_dirty = true;
            ImGui::PopID();
            continue;
        }

        // UX Fix #2: Default value editor (type-specific input field)
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
                // UX Enhancement #1: Vector is auto-sourced from entity position at runtime.
                // Display as read-only to prevent user from entering a value that will be
                // overwritten anyway.
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
                // UX Enhancement #2: EntityID is assigned at runtime; read-only display.
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

    // BUG-001 Hotfix: warn user if invalid entries exist (key empty or type None)
    // to prevent save crash caused by unhandled None type during serialization.
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

    // Add entry button — BUG-001 Hotfix: init with safe defaults (non-empty key, Int type)
    if (ImGui::Button("+##vsbbAdd"))
    {
        BlackboardEntry entry;
        entry.Key      = "NewVariable";
        entry.Type     = VariableType::Int;
        entry.Default  = GetDefaultValueForType(VariableType::Int);  // UX Fix #1
        entry.IsGlobal = false;
        m_template.Blackboard.push_back(entry);
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add key");

    // List existing entries
    for (int idx = static_cast<int>(m_template.Blackboard.size()) - 1; idx >= 0; --idx)
    {
        BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(idx)];

        ImGui::PushID(idx);

        // Use a local (non-static) buffer per iteration to avoid sharing across entries
        char keyBuf[64];
        strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);

        // ── Name (editable text field) ──
        ImGui::SetNextItemWidth(140.0f);
        if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
        {
            entry.Key = keyBuf;
            m_pendingBlackboardEdits[idx] = keyBuf;
            m_dirty = true;
        }

        ImGui::SameLine();

        // ── Type dropdown ──
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

        // ── Default value (type-aware editor) ──
        if (entry.Type != VariableType::None)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("Default:");
            ImGui::SameLine();
            RenderConstValueInput(entry.Default, entry.Type, "##bbdefault");
        }

        // ── Global toggle ──
        ImGui::SameLine();
        bool isGlobal = entry.IsGlobal;
        if (ImGui::Checkbox("G##bbglobal", &isGlobal))
        {
            entry.IsGlobal = isGlobal;
            m_dirty = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Mark as global variable");

        // ── Delete button ──
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

// ============================================================================
// Phase 24 Global Blackboard Integration — RenderGlobalVariablesPanel (Enhanced)
// ============================================================================


void VisualScriptEditorPanel::RenderGlobalVariablesPanel()
{
    ImGui::TextDisabled("Global Variables (Editor Instance)");
    ImGui::Separator();

    // Get reference to the global template registry (non-const for Add)
    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

    // Add Global Variable button
    if (ImGui::Button("+##globalVarAdd", ImVec2(30, 0)))
    {
        ImGui::OpenPopup("AddGlobalVariablePopup");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add global variable");

    // Add Global Variable Modal Dialog
    if (ImGui::BeginPopupModal("AddGlobalVariablePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char newVarName[128] = "newGlobal";
        static int newVarTypeIdx = 2;  // Default to Int
        static char newVarDescription[256] = "Enter description...";

        ImGui::InputText("Variable Name##new", newVarName, sizeof(newVarName));

        const char* typeOptions[] = { "Bool", "Int", "Float", "String", "Vector", "EntityID" };
        const VariableType typeValues[] = {
            VariableType::Bool, VariableType::Int, VariableType::Float,
            VariableType::String, VariableType::Vector, VariableType::EntityID
        };
        ImGui::Combo("Type##new", &newVarTypeIdx, typeOptions, 6);

        ImGui::InputTextMultiline("Description##new", newVarDescription, sizeof(newVarDescription), ImVec2(0, 60));

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (strlen(newVarName) > 0 && !gtb.HasVariable(newVarName))
            {
                TaskValue defaultVal = GetDefaultValueForType(typeValues[newVarTypeIdx]);
                if (gtb.AddVariable(newVarName, typeValues[newVarTypeIdx], defaultVal, newVarDescription, false))
                {
                    SYSTEM_LOG << "[VSEditor] Created new global variable: " << newVarName << "\n";
                    gtb.SaveToFile();  // Use last loaded path automatically

                    // Phase 24: Hot reload to refresh registry and propagate to all panels
                    GlobalTemplateBlackboard::Reload();

                    m_dirty = true;

                    // Reset form
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

    // Check if EntityBlackboard is initialized
    if (!m_entityBlackboard)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "[ERROR] EntityBlackboard not initialized");
        return;
    }

    // Display each global variable with editable entity-specific value
    for (size_t gi = 0; gi < globalVars.size(); ++gi)
    {
        const GlobalEntryDefinition& globalDef = globalVars[gi];

        ImGui::PushID(static_cast<int>(gi));

        // ---- Variable name (read-only) with type label + Delete button ----
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "(%s) %s",
                          VariableTypeToString(globalDef.Type).c_str(),
                          globalDef.Key.c_str());

        ImGui::SameLine();
        ImGui::TextDisabled("(%.1f KB)", 0.1f);  // Placeholder space
        ImGui::SameLine();

        // Delete button for global variable
        if (ImGui::SmallButton("Delete##globalvar"))
        {
            // Mark for deletion (we'll process after the loop to avoid iterator invalidation)
            std::string varToDelete = globalDef.Key;
            if (gtb.RemoveVariable(varToDelete))
            {
                SYSTEM_LOG << "[VSEditor] Deleted global variable: " << varToDelete << "\n";
                gtb.SaveToFile();  // Use last loaded path automatically

                // Phase 24: Hot reload to refresh registry
                GlobalTemplateBlackboard::Reload();

                m_dirty = true;
            }
            ImGui::PopID();
            continue;  // Skip rendering the rest of this variable's UI
        }

        // ---- Description (if available) ----
        if (!globalDef.Description.empty())
        {
            ImGui::TextDisabled("  %s", globalDef.Description.c_str());
        }

        // Create unique table ID per global variable to avoid ImGui::BeginTable() failures
        std::string tableId = "##GlobalVarTable_" + std::to_string(gi);
        if (ImGui::BeginTable(tableId.c_str(), 2, ImGuiTableFlags_SizingStretchSame, ImVec2(0, 0)))
        {
            ImGui::TableSetupColumn("Label", 0);
            ImGui::TableSetupColumn("Value", 0);

            // ---- Default Value (read-only) ----
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

            // ---- Current Value (editable with scope resolution) ----
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Current:");
            ImGui::TableSetColumnIndex(1);

            // Use scoped variable access to get/set entity-specific value
            std::string scopedVarName = "(G)" + globalDef.Key;
            TaskValue currentValue = m_entityBlackboard->GetValueScoped(scopedVarName);

            // Create type-specific input widget
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
                    size_t bufKey = gi; // Use index as unique key for buffer storage
                    if (stringBuffers.find(bufKey) == stringBuffers.end())
                    {
                        std::string initialStr = currentValue.IsNone() ? "" : currentValue.AsString();
                        stringBuffers[bufKey] = std::vector<char>(initialStr.begin(), initialStr.end());
                        stringBuffers[bufKey].push_back('\0');
                        stringBuffers[bufKey].resize(256);  // Allocate buffer
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

            // ---- Persistent flag ----
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


} // namespace Olympe
