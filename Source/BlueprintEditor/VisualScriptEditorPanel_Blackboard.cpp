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

namespace Olympe {

// ============================================================================
// Main Blackboard Panel
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
        entry.Default  = GetDefaultValueForType(VariableType::Int);
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

        // Type selector — "None" is excluded to prevent invalid entries.
        const char* typeLabels[] = {"Bool","Int","Float","Vector","EntityID","String"};
        int typeIdx = static_cast<int>(entry.Type) - 1; // offset: Bool->0, Int->1, ...
        if (typeIdx < 0 || typeIdx >= 6)
        {
            typeIdx    = 1; // default to "Int"
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

        // Default value display (read-only for now)
        ImGui::TextDisabled("Default: (auto)");

        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::TextDisabled("Total: %zu variable(s)", m_template.Blackboard.size());
}

// ============================================================================
// Local Variables Panel - Sub-panel for blackboard
// ============================================================================

void VisualScriptEditorPanel::RenderLocalVariablesPanel()
{
    ImGui::TextDisabled("Local Variables");
    ImGui::Separator();

    int count = 0;
    for (const auto& entry : m_template.Blackboard)
    {
        if (entry.Key.empty() || entry.Type == VariableType::None)
            continue;
        if (entry.IsGlobal)
            continue;

        ++count;
        ImGui::BulletText("%s (%s)", entry.Key.c_str(), GetTypeLabel(entry.Type));
    }

    if (count == 0)
    {
        ImGui::TextDisabled("(no local variables)");
    }

    ImGui::TextDisabled("Total: %d local variable(s)", count);
}

// ============================================================================
// Global Variables Panel - Sub-panel for global scope
// ============================================================================

void VisualScriptEditorPanel::RenderGlobalVariablesPanel()
{
    ImGui::TextDisabled("Global Variables");
    ImGui::Separator();

    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const auto& globalVars = gtb.GetAllVariables();

    if (globalVars.empty())
    {
        ImGui::TextDisabled("(no global variables)");
        return;
    }

    for (const auto& gvar : globalVars)
    {
        ImGui::BulletText("%s (%s)", gvar.Key.c_str(), GetTypeLabel(gvar.Type));
    }

    ImGui::TextDisabled("Total: %zu global variable(s)", globalVars.size());
}

// ============================================================================
// Pending Edits Commit
// ============================================================================

void VisualScriptEditorPanel::CommitPendingBlackboardEdits()
{
    // Process deferred blackboard edits
    // (Reserved for future batching if needed)
    m_pendingBlackboardEdits.clear();
}

// ============================================================================
// Validation and Cleanup
// ============================================================================

void VisualScriptEditorPanel::ValidateAndCleanBlackboardEntries()
{
    // Remove invalid entries before save
    m_template.Blackboard.erase(
        std::remove_if(m_template.Blackboard.begin(), m_template.Blackboard.end(),
                      [](const BlackboardEntry& e) {
                          return e.Key.empty() || e.Type == VariableType::None;
                      }),
        m_template.Blackboard.end());
}

// ============================================================================
// Helper: Get Variables by Type
// ============================================================================

std::vector<BlackboardEntry> VisualScriptEditorPanel::GetVariablesByType(VariableType type)
{
    std::vector<BlackboardEntry> result;
    for (const auto& entry : m_template.Blackboard)
    {
        if (entry.Type == type && !entry.Key.empty())
        {
            result.push_back(entry);
        }
    }
    return result;
}

}  // namespace Olympe
