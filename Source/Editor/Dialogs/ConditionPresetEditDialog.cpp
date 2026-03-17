/**
 * @file ConditionPresetEditDialog.cpp
 * @brief Implementation of ConditionPresetEditDialog (Phase 24.1).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPresetEditDialog.h"

#include <algorithm>

#ifndef OLYMPE_HEADLESS
#include "../../third_party/imgui/imgui.h"
#endif

namespace Olympe {

// ============================================================================
// Static helpers
// ============================================================================

namespace {

const std::vector<std::string>& GetValidOperators()
{
    static const std::vector<std::string> ops = { "==", "!=", "<", "<=", ">", ">=" };
    return ops;
}

const std::vector<std::string>& GetValidModes()
{
    static const std::vector<std::string> modes = { "Variable", "Const", "Pin" };
    return modes;
}

} // anonymous namespace

// ============================================================================
// Construction
// ============================================================================

ConditionPresetEditDialog::ConditionPresetEditDialog()
    : m_mode(Mode::Create)
{
    // Default: Variable == Const
    m_workingCopy.condition.leftMode    = "Variable";
    m_workingCopy.condition.operatorStr = "==";
    m_workingCopy.condition.rightMode   = "Const";
}

ConditionPresetEditDialog::ConditionPresetEditDialog(Mode mode,
                                                      const ConditionPreset* existingPreset)
    : m_mode(mode)
{
    if (mode == Mode::Edit && existingPreset != nullptr)
    {
        m_workingCopy = *existingPreset;
    }
    else
    {
        // Default blank form
        m_workingCopy.condition.leftMode    = "Variable";
        m_workingCopy.condition.operatorStr = "==";
        m_workingCopy.condition.rightMode   = "Const";
    }
}

// ============================================================================
// Result
// ============================================================================

ConditionPreset ConditionPresetEditDialog::GetResult() const
{
    return m_workingCopy;
}

// ============================================================================
// Setters
// ============================================================================

void ConditionPresetEditDialog::SetLeftMode(const std::string& mode)
{
    m_workingCopy.condition.leftMode = mode;
}

void ConditionPresetEditDialog::SetLeftVariable(const std::string& varName)
{
    m_workingCopy.condition.leftVariable = varName;
}

void ConditionPresetEditDialog::SetLeftConst(const TaskValue& value)
{
    m_workingCopy.condition.leftConstValue = value;
}

void ConditionPresetEditDialog::SetLeftPin(const std::string& pinRef)
{
    m_workingCopy.condition.leftPin = pinRef;
}

void ConditionPresetEditDialog::SetOperator(const std::string& op)
{
    m_workingCopy.condition.operatorStr = op;
}

void ConditionPresetEditDialog::SetRightMode(const std::string& mode)
{
    m_workingCopy.condition.rightMode = mode;
}

void ConditionPresetEditDialog::SetRightVariable(const std::string& varName)
{
    m_workingCopy.condition.rightVariable = varName;
}

void ConditionPresetEditDialog::SetRightConst(const TaskValue& value)
{
    m_workingCopy.condition.rightConstValue = value;
}

void ConditionPresetEditDialog::SetRightPin(const std::string& pinRef)
{
    m_workingCopy.condition.rightPin = pinRef;
}

void ConditionPresetEditDialog::SetName(const std::string& name)
{
    m_workingCopy.name = name;
}

// ============================================================================
// Preview
// ============================================================================

std::string ConditionPresetEditDialog::GetPreview() const
{
    return m_workingCopy.GetPreview();
}

// ============================================================================
// Validation
// ============================================================================

bool ConditionPresetEditDialog::IsValidOperator(const std::string& op)
{
    const auto& ops = GetValidOperators();
    return std::find(ops.begin(), ops.end(), op) != ops.end();
}

bool ConditionPresetEditDialog::IsValidMode(const std::string& mode)
{
    const auto& modes = GetValidModes();
    return std::find(modes.begin(), modes.end(), mode) != modes.end();
}

bool ConditionPresetEditDialog::IsOperandFilled(const std::string& mode,
                                                 const std::string& variable,
                                                 const std::string& pin,
                                                 const TaskValue&   constVal)
{
    if (mode == "Variable") { return !variable.empty(); }
    if (mode == "Pin")      { return !pin.empty();      }
    if (mode == "Const")    { return !constVal.IsNone(); }
    return false;
}

bool ConditionPresetEditDialog::IsValid() const
{
    const Condition& c = m_workingCopy.condition;

    if (!IsValidMode(c.leftMode))  { return false; }
    if (!IsValidMode(c.rightMode)) { return false; }
    if (!IsValidOperator(c.operatorStr)) { return false; }

    if (!IsOperandFilled(c.leftMode, c.leftVariable, c.leftPin, c.leftConstValue))
    {
        return false;
    }
    if (!IsOperandFilled(c.rightMode, c.rightVariable, c.rightPin, c.rightConstValue))
    {
        return false;
    }

    return true;
}

// ============================================================================
// Programmatic confirmation
// ============================================================================

bool ConditionPresetEditDialog::Confirm()
{
    if (!IsValid()) { return false; }

    m_isConfirmed = true;
    m_isOpen      = false;
    return true;
}

// ============================================================================
// Render (ImGui — skipped in headless builds / tests)
// ============================================================================

void ConditionPresetEditDialog::Render()
{
#ifndef OLYMPE_HEADLESS
    if (!m_isOpen) { return; }

    const char* title = (m_mode == Mode::Create)
                        ? "Create Condition Preset"
                        : "Edit Condition Preset";

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::OpenPopup(title);

    if (ImGui::BeginPopupModal(title, &m_isOpen,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Preset name
        char nameBuf[128] = {};
        if (m_workingCopy.name.size() < sizeof(nameBuf))
        {
            m_workingCopy.name.copy(nameBuf, m_workingCopy.name.size());
        }
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
        {
            m_workingCopy.name = nameBuf;
        }

        ImGui::Separator();

        RenderOperandSelector("Left", /*isLeft=*/true);
        RenderOperatorSelector();
        RenderOperandSelector("Right", /*isLeft=*/false);

        ImGui::Separator();
        RenderPreview();
        ImGui::Separator();
        RenderConfirmButtons();

        ImGui::EndPopup();
    }
#endif
}

// ============================================================================
// ImGui rendering helpers
// ============================================================================

void ConditionPresetEditDialog::RenderOperandSelector(const char* label, bool isLeft)
{
#ifndef OLYMPE_HEADLESS
    std::string& mode     = isLeft ? m_workingCopy.condition.leftMode
                                   : m_workingCopy.condition.rightMode;
    std::string& variable = isLeft ? m_workingCopy.condition.leftVariable
                                   : m_workingCopy.condition.rightVariable;
    std::string& pin      = isLeft ? m_workingCopy.condition.leftPin
                                   : m_workingCopy.condition.rightPin;
    TaskValue&   constVal = isLeft ? m_workingCopy.condition.leftConstValue
                                   : m_workingCopy.condition.rightConstValue;

    ImGui::PushID(label);
    ImGui::Text("%s:", label);

    // Mode selector (combo)
    const char* const modeItems[] = { "Variable", "Const", "Pin" };
    int modeIdx = 0;
    if (mode == "Const") { modeIdx = 1; }
    else if (mode == "Pin") { modeIdx = 2; }

    ImGui::SetNextItemWidth(90.f);
    if (ImGui::Combo("##mode", &modeIdx, modeItems, 3))
    {
        mode = modeItems[modeIdx];
    }

    ImGui::SameLine();

    if (mode == "Variable")
    {
        char buf[64] = {};
        if (variable.size() < sizeof(buf)) { variable.copy(buf, variable.size()); }
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::InputText("##var", buf, sizeof(buf)))
        {
            variable = buf;
        }
    }
    else if (mode == "Const")
    {
        // Float input (simplified — full implementation would pick type)
        float fval = 0.f;
        if (constVal.GetType() == VariableType::Float) { fval = constVal.AsFloat(); }
        else if (constVal.GetType() == VariableType::Int) { fval = static_cast<float>(constVal.AsInt()); }
        ImGui::SetNextItemWidth(80.f);
        if (ImGui::InputFloat("##const", &fval, 0.f, 0.f, "%.2f"))
        {
            constVal = TaskValue(fval);
        }
    }
    else if (mode == "Pin")
    {
        char buf[64] = {};
        if (pin.size() < sizeof(buf)) { pin.copy(buf, pin.size()); }
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::InputText("##pin", buf, sizeof(buf)))
        {
            pin = buf;
        }
    }

    ImGui::PopID();
#endif
}

void ConditionPresetEditDialog::RenderOperatorSelector()
{
#ifndef OLYMPE_HEADLESS
    const auto& ops = GetValidOperators();
    const std::string& current = m_workingCopy.condition.operatorStr;

    int opIdx = 0;
    for (int i = 0; i < static_cast<int>(ops.size()); ++i)
    {
        if (ops[i] == current) { opIdx = i; break; }
    }

    // Build C-string array for ImGui::Combo
    const char* items[] = { "==", "!=", "<", "<=", ">", ">=" };
    ImGui::SetNextItemWidth(70.f);
    if (ImGui::Combo("Operator", &opIdx, items, static_cast<int>(ops.size())))
    {
        m_workingCopy.condition.operatorStr = ops[opIdx];
    }
#endif
}

void ConditionPresetEditDialog::RenderPreview()
{
#ifndef OLYMPE_HEADLESS
    const std::string preview = GetPreview();
    ImGui::Text("Preview: %s", preview.c_str());
#endif
}

void ConditionPresetEditDialog::RenderConfirmButtons()
{
#ifndef OLYMPE_HEADLESS
    const bool canSave = IsValid();
    if (!canSave) { ImGui::BeginDisabled(); }

    if (ImGui::Button("Save"))
    {
        m_isConfirmed = true;
        m_isOpen      = false;
        ImGui::CloseCurrentPopup();
    }

    if (!canSave) { ImGui::EndDisabled(); }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        m_isOpen = false;
        ImGui::CloseCurrentPopup();
    }
#endif
}

} // namespace Olympe
