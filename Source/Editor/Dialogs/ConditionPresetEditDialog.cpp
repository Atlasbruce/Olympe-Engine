/**
 * @file ConditionPresetEditDialog.cpp
 * @brief Implementation of ConditionPresetEditDialog (Phase 24.1).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant - no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPresetEditDialog.h"

#include <algorithm>

#ifndef OLYMPE_HEADLESS
#include "../../third_party/imgui/imgui.h"
#endif

namespace Olympe {

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

ConditionPresetEditDialog::ConditionPresetEditDialog()
    : m_mode(Mode::Create)
{
    m_workingCopy.left = Operand::CreateVariable("");
    m_workingCopy.op = ComparisonOp::Equal;
    m_workingCopy.right = Operand::CreateConst(0.0);
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
        m_workingCopy.left = Operand::CreateVariable("");
        m_workingCopy.op = ComparisonOp::Equal;
        m_workingCopy.right = Operand::CreateConst(0.0);
    }
}

ConditionPreset ConditionPresetEditDialog::GetResult() const
{
    return m_workingCopy;
}

void ConditionPresetEditDialog::SetLeftMode(const std::string& mode)
{
    if (mode == "Variable")
        m_workingCopy.left = Operand::CreateVariable("");
    else if (mode == "Const")
        m_workingCopy.left = Operand::CreateConst(0.0);
    else if (mode == "Pin")
        m_workingCopy.left = Operand::CreatePin("");
}

void ConditionPresetEditDialog::SetLeftVariable(const std::string& varName)
{
    m_workingCopy.left = Operand::CreateVariable(varName);
}

void ConditionPresetEditDialog::SetLeftConst(double value)
{
    m_workingCopy.left = Operand::CreateConst(value);
}

void ConditionPresetEditDialog::SetLeftPin(const std::string& pinRef)
{
    m_workingCopy.left = Operand::CreatePin(pinRef);
}

void ConditionPresetEditDialog::SetOperator(const std::string& op)
{
    m_workingCopy.op = ConditionPreset::OpFromString(op);
}

void ConditionPresetEditDialog::SetRightMode(const std::string& mode)
{
    if (mode == "Variable")
        m_workingCopy.right = Operand::CreateVariable("");
    else if (mode == "Const")
        m_workingCopy.right = Operand::CreateConst(0.0);
    else if (mode == "Pin")
        m_workingCopy.right = Operand::CreatePin("");
}

void ConditionPresetEditDialog::SetRightVariable(const std::string& varName)
{
    m_workingCopy.right = Operand::CreateVariable(varName);
}

void ConditionPresetEditDialog::SetRightConst(double value)
{
    m_workingCopy.right = Operand::CreateConst(value);
}

void ConditionPresetEditDialog::SetRightPin(const std::string& pinRef)
{
    m_workingCopy.right = Operand::CreatePin(pinRef);
}

void ConditionPresetEditDialog::SetName(const std::string& name)
{
    m_workingCopy.name = name;
}

std::string ConditionPresetEditDialog::GetPreview() const
{
    return m_workingCopy.GetPreview();
}

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

bool ConditionPresetEditDialog::IsOperandFilled(const Operand& operand)
{
    if (operand.IsVariable())
        return !operand.stringValue.empty();
    if (operand.IsPin())
        return !operand.stringValue.empty();
    if (operand.IsConst())
        return true;
    return false;
}

bool ConditionPresetEditDialog::IsValid() const
{
    if (!IsOperandFilled(m_workingCopy.left))
        return false;
    if (!IsOperandFilled(m_workingCopy.right))
        return false;
    return true;
}

bool ConditionPresetEditDialog::Confirm()
{
    if (!IsValid()) { return false; }

    m_isConfirmed = true;
    m_isOpen      = false;
    return true;
}

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

        RenderOperandSelector("Left", true);
        RenderOperatorSelector();
        RenderOperandSelector("Right", false);

        ImGui::Separator();
        RenderPreview();
        ImGui::Separator();
        RenderConfirmButtons();

        ImGui::EndPopup();
    }
#endif
}

void ConditionPresetEditDialog::RenderOperandSelector(const char* label, bool isLeft)
{
#ifndef OLYMPE_HEADLESS
    Operand& operand = isLeft ? m_workingCopy.left : m_workingCopy.right;

    ImGui::PushID(label);
    ImGui::Text("%s:", label);

    const char* const modeItems[] = { "Variable", "Const", "Pin" };
    int modeIdx = 0;
    if (operand.IsConst()) { modeIdx = 1; }
    else if (operand.IsPin()) { modeIdx = 2; }

    ImGui::SetNextItemWidth(90.f);
    if (ImGui::Combo("##mode", &modeIdx, modeItems, 3))
    {
        if (modeIdx == 0)
            operand = Operand::CreateVariable("");
        else if (modeIdx == 1)
            operand = Operand::CreateConst(0.0);
        else if (modeIdx == 2)
            operand = Operand::CreatePin("");
    }

    ImGui::SameLine();

    if (operand.IsVariable())
    {
        char buf[64] = {};
        if (operand.stringValue.size() < sizeof(buf))
        {
            operand.stringValue.copy(buf, operand.stringValue.size());
        }
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::InputText("##var", buf, sizeof(buf)))
        {
            operand.stringValue = buf;
        }
    }
    else if (operand.IsConst())
    {
        float fval = static_cast<float>(operand.constValue);
        ImGui::SetNextItemWidth(80.f);
        if (ImGui::InputFloat("##const", &fval, 0.f, 0.f, "%.2f"))
        {
            operand.constValue = static_cast<double>(fval);
        }
    }
    else if (operand.IsPin())
    {
        char buf[64] = {};
        if (operand.stringValue.size() < sizeof(buf))
        {
            operand.stringValue.copy(buf, operand.stringValue.size());
        }
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
    std::string current = ConditionPreset::OpToString(m_workingCopy.op);

    int opIdx = 0;
    for (int i = 0; i < static_cast<int>(ops.size()); ++i)
    {
        if (ops[i] == current) { opIdx = i; break; }
    }

    const char* items[] = { "==", "!=", "<", "<=", ">", ">=" };
    ImGui::SetNextItemWidth(70.f);
    if (ImGui::Combo("Operator", &opIdx, items, static_cast<int>(ops.size())))
    {
        m_workingCopy.op = ConditionPreset::OpFromString(ops[opIdx]);
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
