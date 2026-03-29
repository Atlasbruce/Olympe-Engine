// ============================================================================
// VisualScriptEditorPanel_NodeProperties.cpp
// Properties Panel & Condition Editor (LARGEST FILE)
// ============================================================================
//
// This file contains all properties panel rendering and node property editing.
// It includes condition editor, variable selectors, constant inputs, and
// preview generation - the most complex UI logic in the editor.
//
// Methods:
// - RenderProperties()                    : Main properties panel dispatcher
// - RenderNodePropertiesPanel()           : Node-specific properties
// - RenderBranchNodeProperties()          : Branch node condition editor
// - RenderMathOpNodeProperties()          : Math operator editor
// - RenderNodeDataParameters()            : Data pin parameter binding
// - RenderConditionEditor() [see Verification]
// - RenderVariableSelector() [see Verification]
// - RenderConstValueInput() [see Verification]
// - RenderPinSelector()                   : Pin dropdown selector
// - BuildConditionPreview()               : Human-readable condition string
// - GetDefaultValueForType()              : Type-specific defaults
// - GetTypeLabel()                        : Type name formatting
//
// Integration Points:
// - m_selectedNodeID                      : Currently selected node
// - m_template.Nodes                      : Node definitions
// - m_conditionsPanel                     : Condition UI helper
// - m_mathOpPanel                         : Math operator UI helper
// - NodeBranchRenderer                    : Branch-specific rendering
// ============================================================================

#include "VisualScriptEditorPanel.h"
#include <sstream>
#include <iomanip>

namespace Olympe {

// ============================================================================
// Main Properties Panel
// ============================================================================

void VisualScriptEditorPanel::RenderProperties()
{
    ImGui::BeginChild("PropertiesPanel", ImVec2(300, 0), true);

    ImGui::TextDisabled("Node Properties");
    ImGui::Separator();

    if (m_selectedNodeID < 0)
    {
        ImGui::TextDisabled("(no selection)");
        ImGui::EndChild();
        return;
    }

    // Find selected node
    const TaskNodeDefinition* selectedNode = m_template.GetNode(m_selectedNodeID);
    if (!selectedNode)
    {
        ImGui::TextDisabled("(node not found)");
        ImGui::EndChild();
        return;
    }

    RenderNodePropertiesPanel(*selectedNode);

    ImGui::EndChild();
}

// ============================================================================
// Node Properties Panel - Type-Specific Rendering
// ============================================================================

void VisualScriptEditorPanel::RenderNodePropertiesPanel(const TaskNodeDefinition& node)
{
    ImGui::Text("Node: %s (ID %d)", node.NodeName.c_str(), node.NodeID);
    ImGui::Separator();

    ImGui::TextDisabled("Type:");
    ImGui::SameLine();
    ImGui::Text("%s", GetNodeTypeLabel(node.Type).c_str());
    ImGui::Separator();

    // Type-specific properties
    switch (node.Type)
    {
        case TaskNodeType::Branch:
            RenderBranchNodeProperties(node);
            break;
        case TaskNodeType::MathOp:
            RenderMathOpNodeProperties(node);
            break;
        case TaskNodeType::GetBBValue:
        case TaskNodeType::SetBBValue:
            RenderNodeDataParameters(node);
            break;
        case TaskNodeType::SubGraph:
        {
            ImGui::TextDisabled("SubGraph Path:");
            ImGui::SameLine();
            ImGui::Text("%s", node.SubGraphPath.c_str());
            break;
        }
        default:
            ImGui::TextDisabled("(no special properties)");
    }
}

// ============================================================================
// Branch Node Properties - Condition Editing
// ============================================================================

void VisualScriptEditorPanel::RenderBranchNodeProperties(const TaskNodeDefinition& node)
{
    ImGui::TextDisabled("Branch Condition");
    ImGui::Separator();

    // Phase 24: Display dynamic data-in pins (conditions)
    ImGui::Text("Conditions: %zu", node.DynamicDataInputPins.size());

    if (node.DynamicDataInputPins.empty())
    {
        ImGui::TextDisabled("(no conditions)");
        return;
    }

    for (size_t i = 0; i < node.DynamicDataInputPins.size(); ++i)
    {
        const auto& pin = node.DynamicDataInputPins[i];
        ImGui::BulletText("Pin-in #%zu: %s", i + 1, pin.label.c_str());
    }
}

// ============================================================================
// Math Op Node Properties - Operator Editing
// ============================================================================

void VisualScriptEditorPanel::RenderMathOpNodeProperties(const TaskNodeDefinition& node)
{
    ImGui::TextDisabled("Math Operator");
    ImGui::Separator();

    // Display current math operation
    ImGui::Text("Operation: [%s] %s [%s]",
                (node.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const ? "Const" :
                 node.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable ? "Var" : "Pin"),
                node.mathOpRef.mathOperator.c_str(),
                (node.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const ? "Const" :
                 node.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable ? "Var" : "Pin"));

    ImGui::TextDisabled("Left: %s", node.mathOpRef.leftOperand.constValue.c_str());
    ImGui::TextDisabled("Op: %s", node.mathOpRef.mathOperator.c_str());
    ImGui::TextDisabled("Right: %s", node.mathOpRef.rightOperand.constValue.c_str());

    // Would call m_mathOpPanel->Render() for editing interface
}

// ============================================================================
// Data Parameters - Variable Binding
// ============================================================================

void VisualScriptEditorPanel::RenderNodeDataParameters(const TaskNodeDefinition& node)
{
    ImGui::TextDisabled("Data Parameters");
    ImGui::Separator();

    if (node.Parameters.empty())
    {
        ImGui::TextDisabled("(no parameters)");
        return;
    }

    for (const auto& param : node.Parameters)
    {
        ImGui::BulletText("%s: ", param.first.c_str());
        ImGui::SameLine();

        if (param.second.Type == ParameterBindingType::Literal)
        {
            ImGui::TextDisabled("[Literal]");
        }
        else if (param.second.Type == ParameterBindingType::Variable)
        {
            ImGui::TextDisabled("[Var: %s]", param.second.VariableName.c_str());
        }
        else
        {
            ImGui::TextDisabled("[???]");
        }
    }
}

// ============================================================================
// Pin Selector - Dropdown for Available Pins
// ============================================================================

void VisualScriptEditorPanel::RenderPinSelector(
    std::string& selectedPin,
    const std::vector<std::string>& availablePins,
    const char* label)
{
    if (availablePins.empty())
    {
        ImGui::TextDisabled("(no pins)");
        return;
    }

    int selected = 0;
    for (int i = 0; i < static_cast<int>(availablePins.size()); ++i)
    {
        if (availablePins[i] == selectedPin)
        {
            selected = i;
            break;
        }
    }

    std::vector<const char*> pinNames;
    for (const auto& pin : availablePins)
    {
        pinNames.push_back(pin.c_str());
    }

    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo(label, &selected, pinNames.data(), static_cast<int>(pinNames.size())))
    {
        selectedPin = availablePins[selected];
        m_dirty = true;
    }
}

// ============================================================================
// Condition Preview Builder
// ============================================================================

std::string VisualScriptEditorPanel::BuildConditionPreview(const Condition& condition)
{
    std::ostringstream oss;

    // Left side
    if (condition.leftMode == "Pin")
    {
        oss << "[Pin: " << (condition.leftPin.empty() ? "?" : condition.leftPin) << "]";
    }
    else if (condition.leftMode == "Variable")
    {
        oss << "[Var: " << (condition.leftVariable.empty() ? "?" : condition.leftVariable) << "]";
    }
    else if (condition.leftMode == "Const")
    {
        if (condition.leftConstValue.IsNone())
            oss << "[Const: ?]";
        else
            oss << "[Const: " << condition.leftConstValue.AsFloat() << "]";
    }
    else
    {
        oss << "[?]";
    }

    // Operator
    oss << " " << condition.operatorStr << " ";

    // Right side
    if (condition.rightMode == "Pin")
    {
        oss << "[Pin: " << (condition.rightPin.empty() ? "?" : condition.rightPin) << "]";
    }
    else if (condition.rightMode == "Variable")
    {
        oss << "[Var: " << (condition.rightVariable.empty() ? "?" : condition.rightVariable) << "]";
    }
    else if (condition.rightMode == "Const")
    {
        if (condition.rightConstValue.IsNone())
            oss << "[Const: ?]";
        else
            oss << "[Const: " << condition.rightConstValue.AsFloat() << "]";
    }
    else
    {
        oss << "[?]";
    }

    return oss.str();
}

// ============================================================================
// Type Helpers
// ============================================================================

TaskValue VisualScriptEditorPanel::GetDefaultValueForType(VariableType type)
{
    switch (type)
    {
        case VariableType::Bool:   return TaskValue(false);
        case VariableType::Int:    return TaskValue(0);
        case VariableType::Float:  return TaskValue(0.0f);
        case VariableType::String: return TaskValue(std::string(""));
        case VariableType::Vector: return TaskValue(::Vector{0.f, 0.f, 0.f});
        default:                   return TaskValue();
    }
}

std::string VisualScriptEditorPanel::GetTypeLabel(VariableType type)
{
    switch (type)
    {
        case VariableType::None:     return "None";
        case VariableType::Bool:     return "Bool";
        case VariableType::Int:      return "Int";
        case VariableType::Float:    return "Float";
        case VariableType::String:   return "String";
        case VariableType::Vector:   return "Vector";
        case VariableType::EntityID: return "EntityID";
        default:                     return "Unknown";
    }
}

}  // namespace Olympe
