/**
 * @file MathOpPropertyPanel.cpp
 * @brief Implementation of MathOp properties panel with operand editors.
 */

#include "MathOpPropertyPanel.h"
#include "../../third_party/imgui/imgui.h"
#include "../../BlueprintEditor/ConditionRef.h"
#include "../../BlueprintEditor/MathOpOperand.h"

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

MathOpPropertyPanel::MathOpPropertyPanel(
    ConditionPresetRegistry& registry,
    DynamicDataPinManager& dynamicPinMgr)
    : m_registry(registry), m_dynamicPinMgr(dynamicPinMgr)
{
}

// ============================================================================
// State accessors
// ============================================================================

void MathOpPropertyPanel::SetMathOpRef(const MathOpRef& ref)
{
    m_mathOpRef = ref;
}

const MathOpRef& MathOpPropertyPanel::GetMathOpRef() const
{
    return m_mathOpRef;
}

void MathOpPropertyPanel::SetDynamicPins(const std::vector<DynamicDataPin>& pins)
{
    m_dynamicPins = pins;
}

void MathOpPropertyPanel::SetNodeName(const std::string& name)
{
    m_nodeName = name;
}

// ============================================================================
// Rendering
// ============================================================================

void MathOpPropertyPanel::Render()
{
    RenderTitleSection();
    ImGui::Spacing();

    RenderInlineOperandEditor();
    ImGui::Spacing();

    if (!m_dynamicPins.empty()) {
        RenderDynamicPinsSection();
    }
}

void MathOpPropertyPanel::RenderTitleSection()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
    ImGui::Text("MathOp: %s", m_nodeName.c_str());
    ImGui::PopStyleColor();
}

void MathOpPropertyPanel::RenderInlineOperandEditor()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.7f, 0.4f, 1.0f));
    const bool editorOpen = ImGui::CollapsingHeader(
        "Operand Editor (Inline)", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();

    if (!editorOpen) return;

    ImGui::Indent();

    // Left operand row
    ImGui::Text("Left Operand (A):");
    RenderOperandRow(0, m_mathOpRef.leftOperand);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Operator selector (centered)
    ImGui::Text("Operator:");
    RenderOperatorSelector();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Right operand row
    ImGui::Text("Right Operand (B):");
    RenderOperandRow(1, m_mathOpRef.rightOperand);

    ImGui::Unindent();
}

void MathOpPropertyPanel::RenderOperandRow(int rowIndex, MathOpOperand& operand)
{
    // Mode selector (Variable/Const/Pin)
    const char* modes[] = { "Variable", "Const", "Pin" };
    int modeIdx = static_cast<int>(operand.mode);

    std::string modeLabel = "Mode##operand" + std::to_string(rowIndex);
    if (ImGui::Combo(modeLabel.c_str(), &modeIdx, modes, 3)) {
        operand.mode = static_cast<MathOpOperand::Mode>(modeIdx);
        m_dirty = true;
        if (m_onOperandChange) {
            m_onOperandChange();
        }
    }

    ImGui::Indent();

    // Render mode-specific input
    switch (operand.mode) {
        case MathOpOperand::Mode::Variable:
            RenderVariableModeInput(operand);
            break;
        case MathOpOperand::Mode::Const:
            RenderConstModeInput(operand);
            break;
        case MathOpOperand::Mode::Pin:
            RenderPinModeSelector(operand);
            break;
    }

    ImGui::Unindent();
}

void MathOpPropertyPanel::RenderVariableModeInput(MathOpOperand& operand)
{
    static char varNameBuf[256] = {};
    strcpy_s(varNameBuf, sizeof(varNameBuf), operand.variableName.c_str());

    std::string label = "Variable Name##var" + operand.variableName;
    if (ImGui::InputText(label.c_str(), varNameBuf, sizeof(varNameBuf))) {
        operand.variableName = varNameBuf;
        m_dirty = true;
        if (m_onOperandChange) {
            m_onOperandChange();
        }
    }

    ImGui::Text("(e.g., \"mSpeed\", \"mHealth\")");
}

void MathOpPropertyPanel::RenderConstModeInput(MathOpOperand& operand)
{
    static char constBuf[64] = {};
    strcpy_s(constBuf, sizeof(constBuf), operand.constValue.c_str());

    std::string label = "Constant Value##const" + operand.constValue;
    if (ImGui::InputText(label.c_str(), constBuf, sizeof(constBuf))) {
        operand.constValue = constBuf;
        m_dirty = true;
    }

    ImGui::Text("(e.g., \"5.0\", \"100\")");
}

void MathOpPropertyPanel::RenderPinModeSelector(MathOpOperand& operand)
{
    ImGui::Text("Pin Selection:");
    ImGui::Indent();

    if (m_dynamicPins.empty()) {
        ImGui::Text("(No dynamic pins yet)");
    } else {
        for (size_t i = 0; i < m_dynamicPins.size(); ++i) {
            std::string pinLabel = "Pin #" + std::to_string(i) + ": " +
                                   m_dynamicPins[i].label;

            bool selected = (operand.dynamicPinID == m_dynamicPins[i].id);
            if (ImGui::Selectable(pinLabel.c_str(), selected)) {
                operand.dynamicPinID = m_dynamicPins[i].id;
                m_dirty = true;
                if (m_onOperandChange) {
                    m_onOperandChange();
                }
            }
        }
    }

    ImGui::Unindent();
}

void MathOpPropertyPanel::RenderOperatorSelector()
{
    const char* operators[] = { "+", "-", "*", "/", "%", "^" };
    int opIdx = 0;

    // Find current operator in list
    for (int i = 0; i < 6; ++i) {
        if (m_mathOpRef.mathOperator == operators[i]) {
            opIdx = i;
            break;
        }
    }

    if (ImGui::Combo("Math Operator##op", &opIdx, operators, 6)) {
        m_mathOpRef.mathOperator = operators[opIdx];
        m_dirty = true;
    }

    ImGui::Text("Result = [Left] %s [Right]", m_mathOpRef.mathOperator.c_str());
}

void MathOpPropertyPanel::RenderDynamicPinsSection()
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    const bool pinsOpen = ImGui::CollapsingHeader("Dynamic Pins (Read-Only)");
    ImGui::PopStyleColor();

    if (!pinsOpen) return;

    ImGui::Indent();

    for (size_t i = 0; i < m_dynamicPins.size(); ++i) {
        ImGui::BulletText("%s (Pin ID: %s)", 
                          m_dynamicPins[i].label.c_str(),
                          m_dynamicPins[i].id.c_str());
    }

    ImGui::Unindent();
}

} // namespace Olympe
