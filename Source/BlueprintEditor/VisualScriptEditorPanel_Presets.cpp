// ============================================================================
// VisualScriptEditorPanel_Presets.cpp
// Phase 24: Condition Presets Rendering and Management
// ============================================================================
//
// This file contains all preset bank rendering and operand editor UI code
// for the Blueprint Editor's Phase 24 Condition Presets feature.
//
// Methods:
// - RenderPresetBankPanel()         : Render the preset library panel
// - RenderPresetItemCompact()       : Render a single preset in compact format
// - RenderOperandEditor()           : Unified operand mode/value dropdown
//
// Integration Points:
// - ConditionPresetRegistry       : Preset storage and access
// - DynamicDataPinManager         : Available pins for operand mode
// - GlobalTemplateBlackboard      : Global variable access
// - m_template.Blackboard         : Local variable access
// - m_dirty flag                  : Mark graph as modified on preset changes
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

void VisualScriptEditorPanel::RenderPresetBankPanel()
{
    ImGui::TextDisabled("Preset Bank (Global)");
    ImGui::Separator();

    if (!m_libraryPanel)
        return;

    size_t presetCount = m_presetRegistry.GetPresetCount();

    // Toolbar: Add preset button
    if (ImGui::Button("+##addpreset", ImVec2(25, 0)))
    {
        m_libraryPanel->OnAddPresetClicked();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("New Preset");

    ImGui::Separator();
    ImGui::TextDisabled("Total: %zu preset(s)", presetCount);
    ImGui::Separator();

    // List all presets in compact horizontal format
    std::vector<ConditionPreset> allPresets = m_presetRegistry.GetFilteredPresets("");

    if (allPresets.empty())
    {
        ImGui::TextDisabled("(no presets - create one to get started)");
    }

    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        const ConditionPreset& preset = allPresets[i];
        ImGui::PushID(preset.id.c_str());
        RenderPresetItemCompact(preset, i + 1);  // 1-indexed for display
        ImGui::PopID();
    }
}


void VisualScriptEditorPanel::RenderPresetItemCompact(const ConditionPreset& preset, size_t index)
{
#ifndef OLYMPE_HEADLESS
    // Single-line horizontal layout matching mockup:
    // [Index: Name (yellow)] [Left▼ mode] [value] [Op▼] [Right▼ mode] [value] [Edit] [Dup] [X]

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

    // Get a mutable copy of the preset for editing
    ConditionPreset editablePreset = preset;
    bool presetModified = false;

    // Condition name display with index (yellow)
    // Use PushID for unique identification, don't add UUID to visible text
    ImGui::PushID(editablePreset.id.c_str());
    ImGui::TextColored(ImVec4(1.0f, 0.843f, 0.0f, 1.0f), "Condition #%zu", index);
    ImGui::PopID();
    ImGui::SameLine(0.0f, 12.0f);

    // Left operand with unified dropdown (mode + value combined)
    if (RenderOperandEditor(editablePreset.left, "##left_op"))
    {
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 6.0f);

    // Operator dropdown
    std::string opStr;
    switch (editablePreset.op)
    {
        case ComparisonOp::Equal:       opStr = "=="; break;
        case ComparisonOp::NotEqual:    opStr = "!="; break;
        case ComparisonOp::Less:        opStr = "<"; break;
        case ComparisonOp::LessEqual:   opStr = "<="; break;
        case ComparisonOp::Greater:     opStr = ">"; break;
        case ComparisonOp::GreaterEqual: opStr = ">="; break;
        default: opStr = "?"; break;
    }

    const char* opNames[] = { "==", "!=", "<", "<=", ">", ">=" };
    const ComparisonOp opValues[] = {
        ComparisonOp::Equal, ComparisonOp::NotEqual,
        ComparisonOp::Less, ComparisonOp::LessEqual,
        ComparisonOp::Greater, ComparisonOp::GreaterEqual
    };
    int curOpIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (editablePreset.op == opValues[i])
        {
            curOpIdx = i;
            break;
        }
    }

    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::Combo("##op_type", &curOpIdx, opNames, 6))
    {
        editablePreset.op = opValues[curOpIdx];
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 6.0f);

    // Right operand with unified dropdown (mode + value combined)
    if (RenderOperandEditor(editablePreset.right, "##right_op"))
    {
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 12.0f);

    // Save modified preset if changed
    if (presetModified)
    {
        m_presetRegistry.UpdatePreset(editablePreset.id, editablePreset);

        // Phase 24 — Sync to template presets for graph serialization
        // Update the preset in m_template.Presets so it gets saved with the graph
        for (size_t pi = 0; pi < m_template.Presets.size(); ++pi)
        {
            if (m_template.Presets[pi].id == editablePreset.id)
            {
                m_template.Presets[pi] = editablePreset;
                break;
            }
        }

        m_dirty = true;
    }

    // Duplicate button
    if (ImGui::Button("Dup##dup_preset", ImVec2(40, 0)))
    {
        std::string newPresetID = m_presetRegistry.DuplicatePreset(editablePreset.id);

        // Phase 24 — Add the duplicate to template presets as well
        if (!newPresetID.empty())
        {
            const ConditionPreset* newPreset = m_presetRegistry.GetPreset(newPresetID);
            if (newPreset)
            {
                m_template.Presets.push_back(*newPreset);
            }
        }

        m_dirty = true;
    }
    ImGui::SameLine(0.0f, 4.0f);

    // Delete button
    if (ImGui::Button("X##del_preset", ImVec2(25, 0)))
    {
        m_presetRegistry.DeletePreset(editablePreset.id);
        m_pinManager->InvalidatePreset(editablePreset.id);

        // Phase 24 — Remove from template presets as well
        for (size_t pi = 0; pi < m_template.Presets.size(); ++pi)
        {
            if (m_template.Presets[pi].id == editablePreset.id)
            {
                m_template.Presets.erase(m_template.Presets.begin() + pi);
                break;
            }
        }
        // Phase 24: Presets are now embedded in graph JSON (via SyncPresetsFromRegistryToTemplate
        // called on Save). This legacy external file save is kept for backwards compatibility
        // with tools that may still read the old flat-file preset bank.
        m_presetRegistry.Save("Blueprints/Presets/condition_presets.json");
    }

    ImGui::PopStyleColor(3);

    // Add visual separator between presets
    ImGui::Separator();
#endif
}


bool VisualScriptEditorPanel::RenderOperandEditor(Operand& operand, const char* labelSuffix)
{
#ifndef OLYMPE_HEADLESS
    bool modified = false;

    // Build a unified dropdown list with this ORDER:
    // 1. [Pin-in #1], [Pin-in #2], ...
    // 2. [Const] <value>
    // 3. Variables (sorted alphabetically)

    std::vector<std::string> allOptions;
    std::vector<int> optionTypes;  // 0=Variable, 1=Const, 2=Pin
    std::vector<std::string> optionValues;  // Store the actual value for each option

    int currentSelectionIdx = -1;

    // ── Add all available pins FIRST ─────────────────────────────────────
    {
        std::vector<DynamicDataPin> allPins = m_pinManager->GetAllPins();
        for (const auto& pin : allPins)
        {
            allOptions.push_back("[Pin-in] " + pin.label);
            optionTypes.push_back(2);  // Pin
            optionValues.push_back(pin.label);

            if (operand.mode == OperandMode::Pin && 
                operand.stringValue == pin.label)
            {
                currentSelectionIdx = static_cast<int>(allOptions.size() - 1);
            }
        }

        // If no pins are available, still show the [Pin-in] option as a category
        if (allPins.empty())
        {
            allOptions.push_back("[Pin-in] (none available)");
            optionTypes.push_back(2);  // Pin
            optionValues.push_back("");  // Empty value for unavailable pin
        }
    }

    // ── Add [Const] option SECOND ────────────────────────────────────────
    {
        std::string constLabel = "[Const] ";
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << operand.constValue;
        std::string constVal = oss.str();
        // Trim trailing zeros
        size_t dot = constVal.find('.');
        if (dot != std::string::npos)
        {
            size_t last = constVal.find_last_not_of('0');
            if (last != std::string::npos && last > dot)
                constVal = constVal.substr(0, last + 1);
            else if (last == dot)
                constVal = constVal.substr(0, dot);
        }
        constLabel += constVal;

        allOptions.push_back(constLabel);
        optionTypes.push_back(1);  // Const
        optionValues.push_back(constVal);

        if (operand.mode == OperandMode::Const)
        {
            currentSelectionIdx = static_cast<int>(allOptions.size() - 1);
        }
    }

    // ── Add all local variables LAST (sorted alphabetically) ──────────────
    {
        std::vector<std::string> sortedVarNames;
        for (const auto& entry : m_template.Blackboard)
        {
            if (entry.Type != VariableType::None && !entry.Key.empty())
            {
                sortedVarNames.push_back(entry.Key);
            }
        }
        // Sort alphabetically
        std::sort(sortedVarNames.begin(), sortedVarNames.end());

        for (const auto& varName : sortedVarNames)
        {
            allOptions.push_back(varName);
            optionTypes.push_back(0);  // Variable
            optionValues.push_back(varName);

            // Check if this is the currently selected variable
            if (operand.mode == OperandMode::Variable && 
                operand.stringValue == varName)
            {
                currentSelectionIdx = static_cast<int>(allOptions.size() - 1);
            }
        }
    }

    // ── Add all global variables (Phase 24) ───────────────────────────────
    {
        GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
        const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

        // Add separator if we have both local and global
        if (!m_template.Blackboard.empty() && !globalVars.empty())
        {
            allOptions.push_back("--- Global Variables ---");
            optionTypes.push_back(-1);  // Separator (no type)
            optionValues.push_back("");
        }

        // Add global variables
        for (const auto& globalVar : globalVars)
        {
            allOptions.push_back(globalVar.Key);
            optionTypes.push_back(0);  // Variable
            optionValues.push_back(globalVar.Key);

            // Check if this is the currently selected global variable
            if (operand.mode == OperandMode::Variable && 
                operand.stringValue == globalVar.Key)
            {
                currentSelectionIdx = static_cast<int>(allOptions.size() - 1);
            }
        }
    }

    // ── Render unified dropdown ──────────────────────────────────────────
    ImGui::SetNextItemWidth(120.0f);

    const char* displayText = (currentSelectionIdx >= 0) ? allOptions[currentSelectionIdx].c_str() : "(none)";

    if (ImGui::BeginCombo(labelSuffix, displayText))
    {
        // Create mutable array of C strings for ImGui
        std::vector<const char*> optionsCStr;
        for (const auto& opt : allOptions)
            optionsCStr.push_back(opt.c_str());

        for (int i = 0; i < static_cast<int>(allOptions.size()); ++i)
        {
            bool selected = (i == currentSelectionIdx);

            // Skip rendering separator as selectable
            if (optionTypes[i] == -1)
            {
                ImGui::Separator();
                continue;
            }

            if (ImGui::Selectable(optionsCStr[i], selected))
            {
                // Update operand based on selected type
                switch (optionTypes[i])
                {
                    case 0:  // Variable
                        operand.mode = OperandMode::Variable;
                        operand.stringValue = optionValues[i];
                        break;
                    case 1:  // Const
                        operand.mode = OperandMode::Const;
                        try {
                            operand.constValue = std::stod(optionValues[i]);
                        } catch (...) {
                            operand.constValue = 0.0;
                        }
                        break;
                    case 2:  // Pin
                        operand.mode = OperandMode::Pin;
                        // For pins, store the pin label. If no specific pin selected (empty),
                        // use a placeholder value that indicates "any available pin"
                        operand.stringValue = optionValues[i].empty() ? "[Pin-in]" : optionValues[i];
                        break;
                }
                modified = true;
            }
        }
        ImGui::EndCombo();
    }

    // ── Add numeric input field for Const mode ──────────────────────────────
    if (operand.mode == OperandMode::Const)
    {
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::SetNextItemWidth(60.0f);
        if (ImGui::InputDouble("##const_value", &operand.constValue, 0.0, 0.0, "%.3f"))
        {
            modified = true;
        }
    }

    return modified;
#else
    return false;
#endif
}

// ============================================================================
// PHASE 24 Panel Integration — Part C: Local Variables Reference
// ============================================================================


} // namespace Olympe
