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

} // anonymous namespace

// ============================================================================
// Construction
// ============================================================================

ConditionPresetEditDialog::ConditionPresetEditDialog()
    : m_mode(Mode::Create)
    , m_operatorStr("==")
{
    // Default: Variable == Const
    m_workingCopy.left  = Operand::CreateVariable("");
    m_workingCopy.op    = ComparisonOp::Equal;
    m_workingCopy.right = Operand::CreateConst(0.0);
}

ConditionPresetEditDialog::ConditionPresetEditDialog(Mode mode,
                                                      const ConditionPreset* existingPreset)
    : m_mode(mode)
{
    if (mode == Mode::Edit && existingPreset != nullptr)
    {
        m_workingCopy  = *existingPreset;
        m_operatorStr  = ConditionPreset::OpToString(m_workingCopy.op);
    }
    else
    {
        // Default blank form
        m_workingCopy.left  = Operand::CreateVariable("");
        m_workingCopy.op    = ComparisonOp::Equal;
        m_workingCopy.right = Operand::CreateConst(0.0);
        m_operatorStr       = "==";
    }
}

// ============================================================================
// Accessors
// ============================================================================

std::string ConditionPresetEditDialog::GetLeftMode() const
{
    return ModeToString(m_workingCopy.left.mode);
}

std::string ConditionPresetEditDialog::GetRightMode() const
{
    return ModeToString(m_workingCopy.right.mode);
}

std::string ConditionPresetEditDialog::GetOperator() const
{
    return m_operatorStr;
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
    m_workingCopy.left.mode = ModeFromString(mode);
}

void ConditionPresetEditDialog::SetLeftVariable(const std::string& varName)
{
    m_workingCopy.left.stringValue = varName;
}

void ConditionPresetEditDialog::SetLeftConst(const TaskValue& value)
{
    if (value.GetType() == VariableType::Int)
        m_workingCopy.left.constValue = static_cast<double>(value.AsInt());
    else if (value.GetType() == VariableType::Float)
        m_workingCopy.left.constValue = static_cast<double>(value.AsFloat());
    else if (value.GetType() == VariableType::Bool)
        m_workingCopy.left.constValue = value.AsBool() ? 1.0 : 0.0;
    else
        m_workingCopy.left.constValue = 0.0;
}

void ConditionPresetEditDialog::SetLeftPin(const std::string& pinRef)
{
    m_workingCopy.left.stringValue = pinRef;
}

void ConditionPresetEditDialog::SetOperator(const std::string& op)
{
    m_operatorStr = op;
    if (IsValidOperator(op))
    {
        m_workingCopy.op = ConditionPreset::OpFromString(op);
    }
}

void ConditionPresetEditDialog::SetRightMode(const std::string& mode)
{
    m_workingCopy.right.mode = ModeFromString(mode);
}

void ConditionPresetEditDialog::SetRightVariable(const std::string& varName)
{
    m_workingCopy.right.stringValue = varName;
}

void ConditionPresetEditDialog::SetRightConst(const TaskValue& value)
{
    if (value.GetType() == VariableType::Int)
        m_workingCopy.right.constValue = static_cast<double>(value.AsInt());
    else if (value.GetType() == VariableType::Float)
        m_workingCopy.right.constValue = static_cast<double>(value.AsFloat());
    else if (value.GetType() == VariableType::Bool)
        m_workingCopy.right.constValue = value.AsBool() ? 1.0 : 0.0;
    else
        m_workingCopy.right.constValue = 0.0;
}

void ConditionPresetEditDialog::SetRightPin(const std::string& pinRef)
{
    m_workingCopy.right.stringValue = pinRef;
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

/*static*/
bool ConditionPresetEditDialog::IsValidOperator(const std::string& op)
{
    const auto& ops = GetValidOperators();
    return std::find(ops.begin(), ops.end(), op) != ops.end();
}

/*static*/
bool ConditionPresetEditDialog::IsOperandFilled(const Operand& operand)
{
    if (operand.mode == OperandMode::Variable) { return !operand.stringValue.empty(); }
    if (operand.mode == OperandMode::Pin)      { return !operand.stringValue.empty(); }
    if (operand.mode == OperandMode::Const)    { return true; } // any double is a valid const
    return false;
}

/*static*/
std::string ConditionPresetEditDialog::ModeToString(OperandMode mode)
{
    switch (mode)
    {
    case OperandMode::Variable: return "Variable";
    case OperandMode::Const:    return "Const";
    case OperandMode::Pin:      return "Pin";
    }
    return "Variable";
}

/*static*/
OperandMode ConditionPresetEditDialog::ModeFromString(const std::string& mode)
{
    if (mode == "Const") { return OperandMode::Const; }
    if (mode == "Pin")   { return OperandMode::Pin;   }
    return OperandMode::Variable; // default / unknown
}

bool ConditionPresetEditDialog::IsValid() const
{
    if (!IsValidOperator(m_operatorStr))         { return false; }
    if (!IsOperandFilled(m_workingCopy.left))    { return false; }
    if (!IsOperandFilled(m_workingCopy.right))   { return false; }
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
    Operand& operand = isLeft ? m_workingCopy.left : m_workingCopy.right;

    ImGui::PushID(label);
    ImGui::Text("%s:", label);

    // Mode selector (combo)
    const char* const modeItems[] = { "Variable", "Const", "Pin" };
    int modeIdx = (operand.mode == OperandMode::Const) ? 1
                : (operand.mode == OperandMode::Pin)   ? 2
                : 0;

    ImGui::SetNextItemWidth(90.f);
    if (ImGui::Combo("##mode", &modeIdx, modeItems, 3))
    {
        operand.mode = ModeFromString(modeItems[modeIdx]);
    }

    ImGui::SameLine();

    if (operand.mode == OperandMode::Variable)
    {
        char buf[64] = {};
        if (operand.stringValue.size() < sizeof(buf))
            operand.stringValue.copy(buf, operand.stringValue.size());
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::InputText("##var", buf, sizeof(buf)))
        {
            operand.stringValue = buf;
        }
    }
    else if (operand.mode == OperandMode::Const)
    {
        float fval = static_cast<float>(operand.constValue);
        ImGui::SetNextItemWidth(80.f);
        if (ImGui::InputFloat("##const", &fval, 0.f, 0.f, "%.2f"))
        {
            operand.constValue = static_cast<double>(fval);
        }
    }
    else if (operand.mode == OperandMode::Pin)
    {
        char buf[64] = {};
        if (operand.stringValue.size() < sizeof(buf))
            operand.stringValue.copy(buf, operand.stringValue.size());
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::InputText("##pin", buf, sizeof(buf)))
        {
            operand.stringValue = buf;
        }
    }

    ImGui::PopID();
#endif
}

void ConditionPresetEditDialog::RenderOperatorSelector()
{
#ifndef OLYMPE_HEADLESS
    const auto& ops = GetValidOperators();

    int opIdx = 0;
    for (int i = 0; i < static_cast<int>(ops.size()); ++i)
    {
        if (ops[i] == m_operatorStr) { opIdx = i; break; }
    }

    const char* items[] = { "==", "!=", "<", "<=", ">", ">=" };
    ImGui::SetNextItemWidth(70.f);
    if (ImGui::Combo("Operator", &opIdx, items, static_cast<int>(ops.size())))
    {
        SetOperator(ops[opIdx]);
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

