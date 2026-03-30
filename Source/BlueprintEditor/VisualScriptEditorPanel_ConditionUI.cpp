/**
 * @file VisualScriptEditorPanel_ConditionUI.cpp
 * @brief Condition editor UI helper methods (Phase 11).
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * This file contains 6 methods extracted from VisualScriptEditorPanel.cpp
 * for better code organization and maintainability:
 *   - RenderConditionEditor() — Main condition editor with left/op/right configuration
 *   - RenderVariableSelector() — Dropdown for selecting blackboard variables
 *   - RenderConstValueInput() — Type-specific constant value input UI
 *   - RenderPinSelector() — Dropdown for selecting data output pins
 *   - BuildConditionPreview() — Build human-readable preview string for conditions
 *   - RenderOperandEditor() — Unified operand selector (pin/const/variable)
 *
 * These helpers support the condition editing UI for Branch nodes,
 * providing type-safe input fields and visual feedback.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"
#include "../third_party/imgui/imgui.h"
#include "../json_helper.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

namespace Olympe {

// ============================================================================
// Condition Editor — Main UI for editing left/op/right conditions
// ============================================================================

void VisualScriptEditorPanel::RenderConditionEditor(
    Condition& condition,
    int conditionIndex,
    const std::vector<BlackboardEntry>& allVars,
    const std::vector<std::string>& availablePins)
{
    ImGui::PushID(conditionIndex);
    ImGui::Separator();
    ImGui::Text("Condition #%d", conditionIndex + 1);

    // -- LEFT SIDE --
    ImGui::Text("Left:");
    ImGui::SameLine();

    const bool isLeftPin   = (condition.leftMode == "Pin");
    const bool isLeftVar   = (condition.leftMode == "Variable");
    const bool isLeftConst = (condition.leftMode == "Const");

    if (ImGui::Button(isLeftPin ? "[PIN]" : "Pin", ImVec2(55, 0)))
    {
        condition.leftMode = "Pin";
        condition.leftPin  = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isLeftVar ? "[VAR]" : "Var", ImVec2(55, 0)))
    {
        condition.leftMode     = "Variable";
        condition.leftVariable = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isLeftConst ? "[CST]" : "Const", ImVec2(55, 0)))
    {
        condition.leftMode = "Const";
        m_dirty = true;
    }

    ImGui::Indent();
    if (condition.leftMode == "Pin")
        RenderPinSelector(condition.leftPin, availablePins, "##leftpin");
    else if (condition.leftMode == "Variable")
        RenderVariableSelector(condition.leftVariable, allVars,
                               condition.compareType, "##leftvar");
    else
        RenderConstValueInput(condition.leftConstValue,
                              condition.compareType, "##leftconst");
    ImGui::Unindent();

    // -- OPERATOR --
    ImGui::Text("Op:");
    ImGui::SameLine();
    const char* operators[] = { "==", "!=", "<", ">", "<=", ">=" };
    int opIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (condition.operatorStr == operators[i])
        {
            opIdx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(70.0f);
    if (ImGui::Combo("##op", &opIdx, operators, 6))
    {
        condition.operatorStr = operators[opIdx];
        m_dirty = true;
    }

    // -- RIGHT SIDE --
    ImGui::Text("Right:");
    ImGui::SameLine();

    const bool isRightPin   = (condition.rightMode == "Pin");
    const bool isRightVar   = (condition.rightMode == "Variable");
    const bool isRightConst = (condition.rightMode == "Const");

    if (ImGui::Button(isRightPin ? "[PIN]##r" : "Pin##r", ImVec2(55, 0)))
    {
        condition.rightMode = "Pin";
        condition.rightPin  = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isRightVar ? "[VAR]##r" : "Var##r", ImVec2(55, 0)))
    {
        condition.rightMode     = "Variable";
        condition.rightVariable = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isRightConst ? "[CST]##r" : "Const##r", ImVec2(55, 0)))
    {
        condition.rightMode = "Const";
        m_dirty = true;
    }

    ImGui::Indent();
    if (condition.rightMode == "Pin")
        RenderPinSelector(condition.rightPin, availablePins, "##rightpin");
    else if (condition.rightMode == "Variable")
        RenderVariableSelector(condition.rightVariable, allVars,
                               condition.compareType, "##rightvar");
    else
        RenderConstValueInput(condition.rightConstValue,
                              condition.compareType, "##rightconst");
    ImGui::Unindent();

    // -- TYPE HINT --
    ImGui::Text("Type:");
    ImGui::SameLine();
    const char* types[] = { "None", "Bool", "Int", "Float", "String", "Vector" };
    const VariableType typeValues[] = {
        VariableType::None, VariableType::Bool, VariableType::Int,
        VariableType::Float, VariableType::String, VariableType::Vector
    };
    int typeIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (condition.compareType == typeValues[i])
        {
            typeIdx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::Combo("##cmptype", &typeIdx, types, 6))
    {
        condition.compareType = typeValues[typeIdx];
        m_dirty = true;
    }

    // -- PREVIEW --
    const std::string preview = BuildConditionPreview(condition);
    ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f),
                       "Preview: %s", preview.c_str());

    ImGui::PopID();
}

// ============================================================================
// Variable Selector — Dropdown UI for selecting blackboard variables
// ============================================================================

void VisualScriptEditorPanel::RenderVariableSelector(
    std::string& selectedVar,
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType,
    const char* label)
{
    // Filter by type (if a type is specified)
    std::vector<std::string> names;
    for (size_t i = 0; i < allVars.size(); ++i)
    {
        if (expectedType == VariableType::None || allVars[i].Type == expectedType)
        {
            if (!allVars[i].Key.empty())
                names.push_back(allVars[i].Key);
        }
    }

    if (names.empty())
    {
        ImGui::TextDisabled("(no variables)");
        return;
    }

    // BUG-029 Fix: auto-initialise to the first available variable when the
    // selection is empty (e.g. right after switching to Variable mode).
    // Without this the combo visually shows the first item but selectedVar
    // remains "" so BuildConditionPreview displays "[Var: ?]".
    if (selectedVar.empty())
    {
        selectedVar = names[0];
        m_dirty = true;
    }

    int selected = 0;
    for (int i = 0; i < static_cast<int>(names.size()); ++i)
    {
        if (names[static_cast<size_t>(i)] == selectedVar)
        {
            selected = i;
            break;
        }
    }

    std::vector<const char*> cstrs;
    cstrs.reserve(names.size());
    for (size_t i = 0; i < names.size(); ++i)
        cstrs.push_back(names[i].c_str());

    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo(label, &selected, cstrs.data(), static_cast<int>(cstrs.size())))
    {
        selectedVar = names[static_cast<size_t>(selected)];
        m_dirty = true;
    }
}

// ============================================================================
// Const Value Input — Type-specific constant value editor
// ============================================================================

void VisualScriptEditorPanel::RenderConstValueInput(
    TaskValue& value,
    VariableType varType,
    const char* label)
{
    // BUG-029 Fix: auto-initialise to a typed default when value is None and
    // a type is known.  Without this the preview always shows "[Const: ?]"
    // until the user explicitly edits the field, because BuildConditionPreview
    // only formats the value when !IsNone().
    if (value.IsNone() && varType != VariableType::None)
    {
        switch (varType)
        {
            case VariableType::Bool:   value = TaskValue(false);                      break;
            case VariableType::Int:    value = TaskValue(0);                          break;
            case VariableType::Float:  value = TaskValue(0.0f);                       break;
            case VariableType::String: value = TaskValue(std::string(""));            break;
            case VariableType::Vector: value = TaskValue(::Vector{0.f, 0.f, 0.f});   break;
            default: break;
        }
        if (!value.IsNone())
            m_dirty = true;
    }

    switch (varType)
    {
        case VariableType::Bool:
        {
            bool bVal = value.IsNone() ? false : value.AsBool();
            if (ImGui::Checkbox(label, &bVal))
            {
                value = TaskValue(bVal);
                m_dirty = true;
            }
            break;
        }
        case VariableType::Int:
        {
            int iVal = value.IsNone() ? 0 : value.AsInt();
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::InputInt(label, &iVal))
            {
                value = TaskValue(iVal);
                m_dirty = true;
            }
            break;
        }
        case VariableType::Float:
        {
            float fVal = value.IsNone() ? 0.0f : value.AsFloat();
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::InputFloat(label, &fVal, 0.0f, 0.0f, "%.3f"))
            {
                value = TaskValue(fVal);
                m_dirty = true;
            }
            break;
        }
        case VariableType::String:
        {
            const std::string sVal = value.IsNone() ? "" : value.AsString();
            char sBuf[256];
            strncpy_s(sBuf, sizeof(sBuf), sVal.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputText(label, sBuf, sizeof(sBuf)))
            {
                value = TaskValue(std::string(sBuf));
                m_dirty = true;
            }
            break;
        }
        case VariableType::Vector:
        {
            ::Vector vVal = value.IsNone() ? ::Vector{0.f, 0.f, 0.f} : value.AsVector();
            float v[3] = { vVal.x, vVal.y, vVal.z };
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::InputFloat3(label, v))
            {
                value = TaskValue(::Vector{ v[0], v[1], v[2] });
                m_dirty = true;
            }
            break;
        }
        default:
        {
            // No type set yet — show a hint
            ImGui::TextDisabled("(set Type first)");
            break;
        }
    }
}

// ============================================================================
// Pin Selector — Dropdown UI for selecting data output pins
// ============================================================================

void VisualScriptEditorPanel::RenderPinSelector(
    std::string& selectedPin,
    const std::vector<std::string>& availablePins,
    const char* label)
{
    if (availablePins.empty())
    {
        ImGui::TextDisabled("(no data-output pins in graph)");
        return;
    }

    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::BeginCombo(label, selectedPin.empty() ? "(select pin)" : selectedPin.c_str()))
    {
        for (size_t i = 0; i < availablePins.size(); ++i)
        {
            const bool isSelected = (selectedPin == availablePins[i]);
            if (ImGui::Selectable(availablePins[i].c_str(), isSelected))
                selectedPin = availablePins[i];
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// ============================================================================
// Condition Preview Builder — Generates human-readable condition string
// ============================================================================

std::string VisualScriptEditorPanel::BuildConditionPreview(const Condition& cond)
{
    auto descSide = [](const std::string& mode,
                       const std::string& pin,
                       const std::string& var,
                       const TaskValue& constValue) -> std::string
    {
        if (mode == "Pin")
            return "[Pin: " + (pin.empty() ? "?" : pin) + "]";
        if (mode == "Variable")
            return "[Var: " + (var.empty() ? "?" : var) + "]";

        // Const — try to format value
        if (!constValue.IsNone())
        {
            std::ostringstream oss;
            switch (constValue.GetType())
            {
                case VariableType::Bool:   oss << (constValue.AsBool() ? "true" : "false"); break;
                case VariableType::Int:    oss << constValue.AsInt();   break;
                case VariableType::Float:  oss << constValue.AsFloat(); break;
                case VariableType::String: oss << '"' << constValue.AsString() << '"'; break;
                case VariableType::Vector:
                {
                    const ::Vector v = constValue.AsVector();
                    oss << "(" << v.x << "," << v.y << "," << v.z << ")";
                    break;
                }
                default: oss << "?"; break;
            }
            return "[Const: " + oss.str() + "]";
        }
        return "[Const: ?]";
    };

    const std::string left  = descSide(cond.leftMode,  cond.leftPin,  cond.leftVariable,  cond.leftConstValue);
    const std::string right = descSide(cond.rightMode, cond.rightPin, cond.rightVariable, cond.rightConstValue);
    const std::string op    = cond.operatorStr.empty() ? "?" : cond.operatorStr;

    return left + " " + op + " " + right;
}

// ============================================================================
// Operand Editor — Unified selector for operands (pin/const/variable)
// ============================================================================

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

} // namespace Olympe
