/**
 * @file VisualScriptEditorPanel_Properties.cpp
 * @brief Properties panel rendering methods for VS graph node editors (Phase 10).
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * This file contains 5 methods extracted from VisualScriptEditorPanel.cpp
 * for better code organization and maintainability:
 *   - RenderProperties() — Main properties panel dispatcher
 *   - RenderNodePropertiesPanel() — Node properties panel (alternative renderer)
 *   - RenderBranchNodeProperties() — Branch/While node properties
 *   - RenderMathOpNodeProperties() — MathOp node properties
 *   - RenderNodeDataParameters() — Generic parameter editor for data nodes
 *
 * These methods handle all UI rendering for the Properties panel on the right side
 * of the editor, supporting type-specific field editing with undo/redo integration.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

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

// ============================================================================
// Branch node — dedicated Properties panel renderer
// ============================================================================

void VisualScriptEditorPanel::RenderBranchNodeProperties(VSEditorNode& eNode,
                                                         TaskNodeDefinition& def)
{
    // ── Blue header: node name (matches canvas Section 1 title bar) ──────────
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable(def.NodeName.c_str(), true,
                      ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor(4);

    ImGui::Separator();
    ImGui::Spacing();

    // ── Structured Conditions (Phase 24 — NodeConditionsPanel) ───────────────
    if (m_conditionsPanel)
    {
        // Reload the panel when the selected node changes.
        if (m_condPanelNodeID != eNode.nodeID)
        {
            m_condPanelNodeID = eNode.nodeID;
            m_conditionsPanel->SetNodeName(def.NodeName);
            m_conditionsPanel->SetConditionRefs(def.conditionRefs);
            m_conditionsPanel->SetDynamicPins(def.dynamicPins);
            m_conditionsPanel->ClearDirty();
        }
        else
        {
            // Keep node name in sync with any in-frame name edits.
            m_conditionsPanel->SetNodeName(def.NodeName);
        }

        m_conditionsPanel->Render();

        if (m_conditionsPanel->IsDirty())
        {
            def.conditionRefs = m_conditionsPanel->GetConditionRefs();

            // Keep m_template in sync for serialization.
            for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
            {
                if (m_template.Nodes[ti].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[ti].conditionRefs = def.conditionRefs;
                    break;
                }
            }
            m_conditionsPanel->ClearDirty();
            m_dirty = true;
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ── Breakpoint checkbox (F9) ─────────────────────────────────────────────
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##vsbp_branch", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                def.NodeName);
    }

    RenderVerificationPanel();

    (void)eNode; // suppress unused-warning when branches have no eNode-specific fields
}

// ============================================================================
// MathOp node — dedicated Properties panel renderer
// ============================================================================

void VisualScriptEditorPanel::RenderMathOpNodeProperties(VSEditorNode& eNode,
                                                        TaskNodeDefinition& def)
{
    // ── Blue header: node name (matches canvas Section 1 title bar) ──────────
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable(def.NodeName.c_str(), true,
                      ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor(4);

    ImGui::Separator();
    ImGui::Spacing();

    // ── Operand Editor (Phase 24 Milestone 2 — MathOpPropertyPanel) ───────────
    if (m_mathOpPanel)
    {
        // Lazy-initialize the panel when node changes
        if (!m_mathOpPanel)
        {
            m_mathOpPanel = std::unique_ptr<MathOpPropertyPanel>(
                new MathOpPropertyPanel(m_presetRegistry, *m_pinManager));
        }

        m_mathOpPanel->SetNodeName(def.NodeName);
        m_mathOpPanel->SetMathOpRef(def.mathOpRef);
        m_mathOpPanel->SetDynamicPins(def.dynamicPins);

        m_mathOpPanel->SetOnOperandChange([this]() {
            // Callback when operands change: regenerate dynamic pins
            if (m_pinManager && m_selectedNodeID >= 0)
            {
                for (size_t i = 0; i < m_editorNodes.size(); ++i)
                {
                    if (m_editorNodes[i].nodeID == m_selectedNodeID)
                    {
                        m_editorNodes[i].def.mathOpRef = m_mathOpPanel->GetMathOpRef();
                        break;
                    }
                }
                m_dirty = true;
            }
        });

        m_mathOpPanel->Render();

        if (m_mathOpPanel->IsDirty())
        {
            def.mathOpRef = m_mathOpPanel->GetMathOpRef();

            // Keep m_template in sync for serialization
            for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
            {
                if (m_template.Nodes[ti].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[ti].mathOpRef = def.mathOpRef;
                    break;
                }
            }
            m_mathOpPanel->ClearDirty();
            m_dirty = true;
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    RenderVerificationPanel();

    (void)eNode; // suppress unused-warning
}

// ============================================================================
// Generic parameter editor for data nodes
// ============================================================================

void VisualScriptEditorPanel::RenderNodeDataParameters(TaskNodeDefinition& def)
{
    // Phase 24 — Generic parameter editor for data nodes (GetBBValue, SetBBValue, MathOp)
    // Allows storing and serializing additional parameters on data nodes

    // Filter out system parameters (those starting with __)
    std::vector<std::string> userParams;
    for (const auto& paramPair : def.Parameters)
    {
        const std::string& paramName = paramPair.first;
        // Skip system parameters
        if (paramName.length() >= 2 && paramName[0] == '_' && paramName[1] == '_')
            continue;
        userParams.push_back(paramName);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Node Parameters:");

    if (userParams.empty())
    {
        ImGui::TextDisabled("(no user parameters - add one below)");
    }

    // Display user parameters
    for (const auto& paramName : userParams)
    {
        auto paramIt = def.Parameters.find(paramName);
        if (paramIt == def.Parameters.end())
            continue;

        ParameterBinding& binding = paramIt->second;

        ImGui::PushID(paramName.c_str());

        // Display parameter name
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "%s", paramName.c_str());

        // Build a label showing the binding type
        const char* typeLabel = "?";
        switch (binding.Type)
        {
            case ParameterBindingType::Literal:       typeLabel = "Literal"; break;
            case ParameterBindingType::LocalVariable:  typeLabel = "Variable"; break;
            case ParameterBindingType::AtomicTaskID:   typeLabel = "AtomicTaskID"; break;
            case ParameterBindingType::ConditionID:    typeLabel = "ConditionID"; break;
            case ParameterBindingType::MathOperator:   typeLabel = "MathOp"; break;
            case ParameterBindingType::ComparisonOp:   typeLabel = "CompOp"; break;
            case ParameterBindingType::SubGraphPath:   typeLabel = "SubGraph"; break;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", typeLabel);

        // Input field for editing parameter value
        if (binding.Type == ParameterBindingType::Literal)
        {
            // For literal values, show an input field
            std::string currentValue;
            if (!binding.LiteralValue.IsNone())
            {
                currentValue = binding.LiteralValue.AsString();
            }

            char buf[256];
            strncpy_s(buf, sizeof(buf), currentValue.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText(("##" + paramName + "_val").c_str(), buf, sizeof(buf)))
            {
                // Parse and store the value
                std::string strVal(buf);
                binding.LiteralValue = TaskValue(strVal);

                // Keep template in sync
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].Parameters[paramName] = binding;
                        break;
                    }
                }
                m_dirty = true;
            }
        }
        else if (binding.Type == ParameterBindingType::LocalVariable)
        {
            // For local variables, show a dropdown of available variables
            BBVariableRegistry bbReg;
            bbReg.LoadFromTemplate(m_template);
            const std::vector<VarSpec> vars = bbReg.GetAllVariables();

            const char* preview = binding.VariableName.empty() ? "(select...)" : binding.VariableName.c_str();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo(("##" + paramName + "_var").c_str(), preview))
            {
                for (const auto& var : vars)
                {
                    bool selected = (var.name == binding.VariableName);
                    if (ImGui::Selectable(var.displayLabel.c_str(), selected))
                    {
                        binding.VariableName = var.name;

                        // Keep template in sync
                        for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                        {
                            if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                            {
                                m_template.Nodes[i].Parameters[paramName] = binding;
                                break;
                            }
                        }
                        m_dirty = true;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            // For other types, show a text field for the identifier
            char buf[256];
            strncpy_s(buf, sizeof(buf), binding.VariableName.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText(("##" + paramName + "_id").c_str(), buf, sizeof(buf)))
            {
                binding.VariableName = buf;

                // Keep template in sync
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].Parameters[paramName] = binding;
                        break;
                    }
                }
                m_dirty = true;
            }
        }

        ImGui::PopID();
    }

    // Add parameter section
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Add Parameter:");

    static char paramNameBuf[256] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80.0f);
    ImGui::InputText("##new_param_name", paramNameBuf, sizeof(paramNameBuf), ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    if (ImGui::Button("Add", ImVec2(70.0f, 0.0f)))
    {
        std::string newParamName(paramNameBuf);
        if (!newParamName.empty() && def.Parameters.find(newParamName) == def.Parameters.end())
        {
            // Create new parameter with default Literal binding
            ParameterBinding newBinding;
            newBinding.Type = ParameterBindingType::Literal;
            newBinding.LiteralValue = TaskValue("");

            def.Parameters[newParamName] = newBinding;

            // Keep template in sync
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].Parameters[newParamName] = newBinding;
                    break;
                }
            }

            m_dirty = true;
            paramNameBuf[0] = '\0';  // Clear the input field
        }
    }
}

// ============================================================================
// While Node Properties
// ============================================================================

void VisualScriptEditorPanel::RenderWhileNodeProperties()
{
    if (m_selectedNodeID < 0)
        return;

    // Find the selected node in the template
    TaskNodeDefinition* nodePtr = nullptr;
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
        {
            nodePtr = &m_template.Nodes[i];
            break;
        }
    }

    if (!nodePtr || nodePtr->Type != TaskNodeType::While)
        return;

    ImGui::TextDisabled("While Loop Configuration");
    ImGui::Separator();

    // Display node name
    static char nodeName[256] = "";
    strncpy_s(nodeName, sizeof(nodeName), nodePtr->NodeName.c_str(), 
              sizeof(nodeName) - 1);

    if (ImGui::InputText("##while_name", nodeName, sizeof(nodeName)))
    {
        nodePtr->NodeName = nodeName;
        m_dirty = true;
    }

    ImGui::Separator();
    ImGui::TextDisabled("Loop Conditions");
    ImGui::Separator();

    // Display existing conditions
    if (!nodePtr->conditions.empty())
    {
        for (size_t ci = 0; ci < nodePtr->conditions.size(); ++ci)
        {
            ImGui::PushID(static_cast<int>(ci));

            Condition& cond = nodePtr->conditions[ci];

            ImGui::Text("Condition #%zu:", ci + 1);

            // Left operand
            const char* leftModes[] = { "Variable", "Const", "Pin" };
            int leftModeIdx = 0;
            if (cond.leftMode == "Const") leftModeIdx = 1;
            else if (cond.leftMode == "Pin") leftModeIdx = 2;

            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::Combo("##left_mode", &leftModeIdx, leftModes, 3))
            {
                cond.leftMode = leftModes[leftModeIdx];
                m_dirty = true;
            }
            ImGui::SameLine();

            // Left value input
            if (leftModeIdx == 0)  // Variable
            {
                static char leftVar[256] = "";
                strncpy_s(leftVar, sizeof(leftVar), cond.leftVariable.c_str(), sizeof(leftVar) - 1);
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputText("##left_var", leftVar, sizeof(leftVar)))
                {
                    cond.leftVariable = leftVar;
                    m_dirty = true;
                }
            }
            else if (leftModeIdx == 1)  // Const
            {
                static float leftConst = 0.0f;
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputFloat("##left_const", &leftConst))
                {
                    cond.leftConstValue = TaskValue(leftConst);
                    m_dirty = true;
                }
            }

            // Operator
            const char* operators[] = { "==", "!=", "<", ">", "<=", ">=" };
            int opIdx = 0;
            for (int i = 0; i < 6; ++i)
            {
                if (cond.operatorStr == operators[i])
                {
                    opIdx = i;
                    break;
                }
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::Combo("##op", &opIdx, operators, 6))
            {
                cond.operatorStr = operators[opIdx];
                m_dirty = true;
            }

            // Right operand
            const char* rightModes[] = { "Variable", "Const", "Pin" };
            int rightModeIdx = 0;
            if (cond.rightMode == "Const") rightModeIdx = 1;
            else if (cond.rightMode == "Pin") rightModeIdx = 2;

            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::Combo("##right_mode", &rightModeIdx, rightModes, 3))
            {
                cond.rightMode = rightModes[rightModeIdx];
                m_dirty = true;
            }
            ImGui::SameLine();

            // Right value input
            if (rightModeIdx == 0)  // Variable
            {
                static char rightVar[256] = "";
                strncpy_s(rightVar, sizeof(rightVar), cond.rightVariable.c_str(), sizeof(rightVar) - 1);
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputText("##right_var", rightVar, sizeof(rightVar)))
                {
                    cond.rightVariable = rightVar;
                    m_dirty = true;
                }
            }
            else if (rightModeIdx == 1)  // Const
            {
                static float rightConst = 0.0f;
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputFloat("##right_const", &rightConst))
                {
                    cond.rightConstValue = TaskValue(rightConst);
                    m_dirty = true;
                }
            }

            // Delete button
            ImGui::SameLine();
            if (ImGui::Button("X##del_cond", ImVec2(25, 0)))
            {
                nodePtr->conditions.erase(
                    nodePtr->conditions.begin() + ci);
                m_dirty = true;
                ImGui::PopID();
                break;  // Exit loop to avoid iterator issues
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }
    else
    {
        ImGui::TextDisabled("(no conditions defined)");
    }

    // Add condition button
    if (ImGui::Button("+ Add Condition", ImVec2(-1.0f, 0.0f)))
    {
        Condition newCond;
        newCond.leftMode = "Variable";
        newCond.operatorStr = "==";
        newCond.rightMode = "Variable";
        nodePtr->conditions.push_back(newCond);
        m_dirty = true;
    }
}

// ============================================================================
// ForEach Node Properties
// ============================================================================

void VisualScriptEditorPanel::RenderForEachNodeProperties()
{
    if (m_selectedNodeID < 0)
        return;

    // Find the selected node in the template
    TaskNodeDefinition* nodePtr = nullptr;
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
        {
            nodePtr = &m_template.Nodes[i];
            break;
        }
    }

    if (!nodePtr || nodePtr->Type != TaskNodeType::ForEach)
        return;

    ImGui::TextDisabled("ForEach Loop Configuration");
    ImGui::Separator();

    // Display node name
    static char nodeName[256] = "";
    strncpy_s(nodeName, sizeof(nodeName), nodePtr->NodeName.c_str(), 
              sizeof(nodeName) - 1);

    if (ImGui::InputText("##foreach_name", nodeName, sizeof(nodeName)))
    {
        nodePtr->NodeName = nodeName;
        m_dirty = true;
    }

    ImGui::Separator();
    ImGui::TextDisabled("Loop Configuration");
    ImGui::Separator();

    // Display available blackboard variables as suggestions
    ImGui::TextDisabled("Suggested List Variables:");
    ImGui::BeginChild("##foreach_bb_list", ImVec2(0, 100), true);
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template.Blackboard[i];

        // Only show List-type variables
        if (entry.Type == VariableType::List)
        {
            ImGui::TextDisabled("%s (List)", entry.Key.c_str());
        }
    }
    ImGui::EndChild();

    ImGui::TextDisabled("(ForEach node specific parameters pending implementation)");
}

// ============================================================================
// SubGraph Node Properties
// ============================================================================

void VisualScriptEditorPanel::RenderSubGraphNodeProperties()
{
    if (m_selectedNodeID < 0)
        return;

    // Find the selected node in the template
    TaskNodeDefinition* nodePtr = nullptr;
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
        {
            nodePtr = &m_template.Nodes[i];
            break;
        }
    }

    if (!nodePtr || nodePtr->Type != TaskNodeType::SubGraph)
        return;

    // ========================================================================
    // Blue header: node name (matches canvas title bar)
    // ========================================================================
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable(nodePtr->NodeName.c_str(), true,
                      ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor(4);

    ImGui::Separator();
    ImGui::Spacing();

    // ========================================================================
    // SubGraph File Path - stored as editable parameter (Phase 24)
    // ========================================================================
    ImGui::TextDisabled("SubGraph File");
    ImGui::Separator();

    // Store path as a parameter binding for true serialization
    // This ensures the path is saved with the graph in JSON format
    ParameterBinding* pathBinding = nullptr;
    auto pathIt = nodePtr->Parameters.find("subgraph_path");
    if (pathIt == nodePtr->Parameters.end())
    {
        // Create the binding if it doesn't exist
        ParameterBinding newBinding;
        newBinding.Type = ParameterBindingType::Literal;
        newBinding.LiteralValue = TaskValue(nodePtr->SubGraphPath);
        nodePtr->Parameters["subgraph_path"] = newBinding;
        pathIt = nodePtr->Parameters.find("subgraph_path");
    }
    pathBinding = &pathIt->second;

    // Display path as editable text field
    std::string currentPath = pathBinding->LiteralValue.to_string();
    static char pathBuffer[512] = "";
    strncpy_s(pathBuffer, sizeof(pathBuffer), currentPath.c_str(), 
              sizeof(pathBuffer) - 1);

    ImGui::SetNextItemWidth(-50.0f);
    if (ImGui::InputText("##subgraph_path_input", pathBuffer, sizeof(pathBuffer),
                         ImGuiInputTextFlags_EnterReturnsTrue))
    {
        std::string newPath(pathBuffer);
        pathBinding->LiteralValue = TaskValue(newPath);
        nodePtr->SubGraphPath = newPath;  // Keep both in sync
        m_dirty = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Browse##sg_browse", ImVec2(45, 0)))
    {
        // TODO: Implement file browser for SubGraph files
        ImGui::OpenPopup("##subgraph_file_browser");
    }

    ImGui::Spacing();
    if (nodePtr->SubGraphPath.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), 
                          "⚠ SubGraph path is empty - set a valid .ats file");
    }
    else
    {
        ImGui::TextDisabled("Path: %s", nodePtr->SubGraphPath.c_str());
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ========================================================================
    // Input Parameters Section
    // ========================================================================
    ImGui::TextDisabled("Input Parameters");
    ImGui::Separator();

    // Display input parameters with full editor
    if (!nodePtr->InputParams.empty())
    {
        ImGui::BeginChild("##subgraph_params", ImVec2(0, 200), true);

        size_t paramIdx = 0;
        std::vector<std::string> paramsToDelete;

        for (auto it = nodePtr->InputParams.begin(); 
             it != nodePtr->InputParams.end(); ++it, ++paramIdx)
        {
            ImGui::PushID(static_cast<int>(paramIdx));

            const std::string& paramName = it->first;
            ParameterBinding& binding = it->second;

            // Parameter name (yellow, read-only)
            ImGui::TextColored(ImVec4(1.0f, 0.843f, 0.0f, 1.0f), "%s", paramName.c_str());
            ImGui::SameLine(150.0f);

            // Binding type selection (Literal vs LocalVariable)
            const char* bindingTypes[] = { "Literal", "LocalVariable" };
            int typeIdx = static_cast<int>(binding.Type);

            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::Combo("##binding_type", &typeIdx, bindingTypes, 2))
            {
                binding.Type = static_cast<ParameterBindingType>(typeIdx);
                m_dirty = true;
            }
            ImGui::SameLine();

            // Value editor based on binding type
            if (binding.Type == ParameterBindingType::Literal)
            {
                // Literal value editor - display as text
                std::string literalValue = binding.LiteralValue.to_string();
                static char valueBuffer[256] = "";
                strncpy_s(valueBuffer, sizeof(valueBuffer), literalValue.c_str(), 
                         sizeof(valueBuffer) - 1);

                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputText("##param_value", valueBuffer, sizeof(valueBuffer)))
                {
                    // Update the literal value from text
                    binding.LiteralValue = TaskValue(std::string(valueBuffer));
                    m_dirty = true;
                }
            }
            else if (binding.Type == ParameterBindingType::LocalVariable)
            {
                // LocalVariable selector - show available BB keys
                static char varBuffer[256] = "";
                strncpy_s(varBuffer, sizeof(varBuffer), 
                         binding.VariableName.c_str(), 
                         sizeof(varBuffer) - 1);

                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputText("##param_var", varBuffer, sizeof(varBuffer)))
                {
                    binding.VariableName = varBuffer;
                    m_dirty = true;
                }

                // Show hint with available blackboard keys
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::BeginCombo("##var_hint", "(suggestions)"))
                {
                    for (size_t bbIdx = 0; bbIdx < m_template.Blackboard.size(); ++bbIdx)
                    {
                        const BlackboardEntry& bbEntry = m_template.Blackboard[bbIdx];
                        if (ImGui::Selectable(bbEntry.Key.c_str()))
                        {
                            binding.VariableName = bbEntry.Key;
                            m_dirty = true;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::SameLine();

            // Delete button for this parameter
            if (ImGui::Button("X##del_param", ImVec2(25, 0)))
            {
                paramsToDelete.push_back(paramName);
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::EndChild();

        // Process deletions (can't delete while iterating)
        for (const std::string& paramName : paramsToDelete)
        {
            nodePtr->InputParams.erase(paramName);
            m_dirty = true;
        }
    }
    else
    {
        ImGui::TextDisabled("(no input parameters - check SubGraph file)");
    }

    ImGui::Separator();

    // Add new parameter button
    if (ImGui::Button("+##add_param", ImVec2(25, 0)))
    {
        // Generate unique parameter name
        int idx = 1;
        std::string newParamName = "param_1";
        while (nodePtr->InputParams.find(newParamName) != nodePtr->InputParams.end())
        {
            ++idx;
            newParamName = "param_" + std::to_string(idx);
        }

        // Create new parameter with default Literal binding
        ParameterBinding newBinding;
        newBinding.Type = ParameterBindingType::Literal;
        newBinding.LiteralValue = TaskValue(std::string(""));
        nodePtr->InputParams[newParamName] = newBinding;
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add Parameter");

    // ========================================================================
    // Output Parameters Section
    // ========================================================================

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Output Parameters (Return Values)");
    ImGui::Separator();

    // Display output parameter mappings (SubGraphOutput -> Blackboard Key)
    if (!nodePtr->OutputParams.empty())
    {
        ImGui::BeginChild("##subgraph_outputs", ImVec2(0, 150), true);

        size_t outIdx = 0;
        std::vector<std::string> outputsToDelete;

        for (auto it = nodePtr->OutputParams.begin(); 
             it != nodePtr->OutputParams.end(); ++it, ++outIdx)
        {
            ImGui::PushID(static_cast<int>(1000 + outIdx));  // Offset to avoid ID collisions

            const std::string& outputName = it->first;
            std::string& bbKey = it->second;

            // Output name (cyan, read-only)
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", outputName.c_str());
            ImGui::SameLine(150.0f);

            // Blackboard key assignment
            std::string bbKeyValue = bbKey;
            static char bbKeyBuffer[256] = "";
            strncpy_s(bbKeyBuffer, sizeof(bbKeyBuffer), bbKeyValue.c_str(), 
                     sizeof(bbKeyBuffer) - 1);

            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputText("##output_bb", bbKeyBuffer, sizeof(bbKeyBuffer)))
            {
                bbKey = bbKeyBuffer;
                m_dirty = true;
            }
            ImGui::SameLine();

            // Suggestions dropdown
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::BeginCombo("##out_hint", "(suggestions)"))
            {
                for (size_t bbIdx = 0; bbIdx < m_template.Blackboard.size(); ++bbIdx)
                {
                    const BlackboardEntry& bbEntry = m_template.Blackboard[bbIdx];
                    if (ImGui::Selectable(bbEntry.Key.c_str()))
                    {
                        bbKey = bbEntry.Key;
                        m_dirty = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            // Delete button for this output mapping
            if (ImGui::Button("X##del_output", ImVec2(25, 0)))
            {
                outputsToDelete.push_back(outputName);
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::EndChild();

        // Process deletions
        for (const std::string& outputName : outputsToDelete)
        {
            nodePtr->OutputParams.erase(outputName);
            m_dirty = true;
        }
    }
    else
    {
        ImGui::TextDisabled("(no output parameters - check SubGraph file)");
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Breakpoint checkbox
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##subgraph_bp", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                nodePtr->NodeName);
    }
}

// ============================================================================
// Main Properties panel dispatcher
// ============================================================================

void VisualScriptEditorPanel::RenderProperties()
{
    ImGui::TextDisabled("Properties");

    if (m_selectedNodeID < 0)
    {
        ImGui::TextDisabled("(select a node)");
        return;
    }

    // Find the editor node
    VSEditorNode* eNode = nullptr;
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        if (m_editorNodes[i].nodeID == m_selectedNodeID)
        {
            eNode = &m_editorNodes[i];
            break;
        }
    }
    if (eNode == nullptr)
        return;

    TaskNodeDefinition& def = eNode->def;

    // Reset focus-node tracking when the selected node changes.
    // Old-value snapshots do NOT need explicit resetting here — they are
    // naturally overwritten by the next IsItemActivated() event.
    m_propEditNodeIDOnFocus = m_selectedNodeID;

    // ---- NodeName (present for all node types) ----
    {
        char nameBuf[128];
        strncpy_s(nameBuf, sizeof(nameBuf), def.NodeName.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name##vsname", nameBuf, sizeof(nameBuf)))
        {
            def.NodeName = nameBuf;
            // Sync live to template for immediate canvas display and serialization
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].NodeName = def.NodeName;
                    break;
                }
            }
            m_dirty = true;
        }
        if (ImGui::IsItemActivated())
        {
            m_propEditOldName       = def.NodeName;
            m_propEditNodeIDOnFocus = m_selectedNodeID;
        }
        if (ImGui::IsItemDeactivatedAfterEdit() &&
            m_propEditNodeIDOnFocus == m_selectedNodeID &&
            def.NodeName != m_propEditOldName)
        {
            m_undoStack.PushCommand(
                std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                    m_selectedNodeID, "NodeName",
                    PropertyValue::FromString(m_propEditOldName),
                    PropertyValue::FromString(def.NodeName))),
                m_template);
        }
    }

    // ---- Type-specific fields — all buffers are local (non-static) to avoid
    //      stale data when switching between selected nodes. ----
    switch (def.Type)
    {
        case TaskNodeType::AtomicTask:
        {
            // --- AtomicTaskID dropdown ---
            const std::vector<TaskSpec> tasks = AtomicTaskUIRegistry::Get().GetSortedForUI();
            const std::string& currentTask = def.AtomicTaskID;
            const char* previewLabel = currentTask.empty() ? "(select task...)" : currentTask.c_str();

            if (ImGui::IsItemActivated())
            {
                m_propEditOldTaskID     = def.AtomicTaskID;
                m_propEditNodeIDOnFocus = m_selectedNodeID;
            }

            if (ImGui::BeginCombo("TaskType##vstask", previewLabel))
            {
                if (m_propEditOldTaskID != def.AtomicTaskID)
                {
                    m_propEditOldTaskID     = def.AtomicTaskID;
                    m_propEditNodeIDOnFocus = m_selectedNodeID;
                }
                std::string lastCat;
                for (size_t ti = 0; ti < tasks.size(); ++ti)
                {
                    const TaskSpec& spec = tasks[ti];
                    // Show category header separator when category changes
                    if (spec.category != lastCat)
                    {
                        if (!lastCat.empty())
                            ImGui::Separator();
                        ImGui::TextDisabled("%s", spec.category.c_str());
                        lastCat = spec.category;
                    }
                    bool selected = (spec.id == currentTask);
                    std::string label = "  " + spec.displayName + "##" + spec.id;
                    if (ImGui::Selectable(label.c_str(), selected))
                    {
                        const std::string oldTaskID = def.AtomicTaskID;
                        def.AtomicTaskID = spec.id;
                        // Auto-fill node name with the action's display name
                        def.NodeName = spec.displayName;
                        for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                        {
                            if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                            {
                                m_template.Nodes[i].AtomicTaskID = def.AtomicTaskID;
                                m_template.Nodes[i].NodeName = def.NodeName;
                                break;
                            }
                        }
                        if (def.AtomicTaskID != oldTaskID)
                        {
                            m_undoStack.PushCommand(
                                std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                    m_selectedNodeID, "AtomicTaskID",
                                    PropertyValue::FromString(oldTaskID),
                                    PropertyValue::FromString(def.AtomicTaskID))),
                                m_template);
                        }
                        m_dirty = true;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                    // Tooltip with description
                    if (ImGui::IsItemHovered() && !spec.description.empty())
                    {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(spec.description.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }
        case TaskNodeType::Delay:
        {
            float delay = def.DelaySeconds;
            if (ImGui::InputFloat("Delay (s)##vsdelay", &delay, 0.1f, 1.0f))
            {
                def.DelaySeconds = delay;
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].DelaySeconds = def.DelaySeconds;
                        break;
                    }
                }
                m_dirty = true;
            }
            if (ImGui::IsItemActivated())
            {
                m_propEditOldDelay      = def.DelaySeconds;
                m_propEditNodeIDOnFocus = m_selectedNodeID;
            }
            if (ImGui::IsItemDeactivatedAfterEdit() &&
                m_propEditNodeIDOnFocus == m_selectedNodeID &&
                def.DelaySeconds != m_propEditOldDelay)
            {
                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                        m_selectedNodeID, "DelaySeconds",
                        PropertyValue::FromFloat(m_propEditOldDelay),
                        PropertyValue::FromFloat(def.DelaySeconds))),
                    m_template);
            }
            break;
        }
        case TaskNodeType::GetBBValue:
        {
            // Phase 24.2: Variable node (data pure read node) - use dedicated renderer
            // Variables are rendered with m_variablePanel instead of m_getBBPanel
            if (m_variablePanel)
            {
                m_variablePanel->SetNodeName(def.NodeName);
                m_variablePanel->SetTemplate(&m_template);
                m_variablePanel->SetBBKey(def.BBKey);

                m_variablePanel->Render();

                if (m_variablePanel->IsDirty())
                {
                    const std::string oldKey = def.BBKey;
                    def.BBKey = m_variablePanel->GetBBKey();

                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].BBKey = def.BBKey;
                            break;
                        }
                    }

                    if (def.BBKey != oldKey)
                    {
                        m_undoStack.PushCommand(
                            std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                m_selectedNodeID, "BBKey",
                                PropertyValue::FromString(oldKey),
                                PropertyValue::FromString(def.BBKey))),
                            m_template);
                    }
                    m_variablePanel->ClearDirty();
                    m_dirty = true;
                }
            }

            // Render node parameters (Phase 24 — node data serialization)
            RenderNodeDataParameters(def);
            break;
        }
        case TaskNodeType::SetBBValue:
        {
            // Phase 24 Milestone 3: Delegate to dedicated SetBBValue properties renderer
            if (m_setBBPanel)
            {
                m_setBBPanel->SetNodeName(def.NodeName);
                m_setBBPanel->SetTemplate(&m_template);
                m_setBBPanel->SetBBKey(def.BBKey);

                m_setBBPanel->Render();

                if (m_setBBPanel->IsDirty())
                {
                    const std::string oldKey = def.BBKey;
                    def.BBKey = m_setBBPanel->GetBBKey();

                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].BBKey = def.BBKey;
                            break;
                        }
                    }

                    if (def.BBKey != oldKey)
                    {
                        m_undoStack.PushCommand(
                            std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                m_selectedNodeID, "BBKey",
                                PropertyValue::FromString(oldKey),
                                PropertyValue::FromString(def.BBKey))),
                            m_template);
                    }
                    m_setBBPanel->ClearDirty();
                    m_dirty = true;
                }
            }

            // Render node parameters (Phase 24 — node data serialization)
            RenderNodeDataParameters(def);
            break;
        }
        case TaskNodeType::Branch:
        case TaskNodeType::While:
        {
            // Delegate to the dedicated Phase 24-Rendering branch properties renderer.
            // This shows: blue header -> NodeConditionsPanel -> Breakpoint checkbox.
            // The return prevents any legacy condition UI from also rendering.
            RenderBranchNodeProperties(*eNode, def);
            return;
        }
        case TaskNodeType::SubGraph:
        {
            // Phase 24: Delegate to the dedicated SubGraph properties renderer.
            // This shows: blue header -> editable path -> input/output parameter editors.
            // The return prevents any legacy UI from also rendering.
            RenderSubGraphNodeProperties();
            return;
        }
        case TaskNodeType::MathOp:
        {
            // Phase 24 Milestone 2: Delegate to the dedicated MathOp properties renderer.
            // This shows: blue header -> MathOpPropertyPanel -> operand editors.
            RenderMathOpNodeProperties(*eNode, def);

            // Render node parameters (Phase 24 — node data serialization)
            RenderNodeDataParameters(def);
            return;
        }
        case TaskNodeType::Switch:
        {
            // Sync m_propEditSwitchCases with the node's switchCases when node changes
            if (m_propEditNodeIDOnFocus != m_selectedNodeID)
                m_propEditSwitchCases = def.switchCases;

            // Find the corresponding template node once for all edits below
            TaskNodeDefinition* tmplNode = nullptr;
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    tmplNode = &m_template.Nodes[i];
                    break;
                }
            }

            // ---- Switch Variable ----
            {
                // Dropdown populated from Int-typed blackboard variables only
                // (Switch node evaluates an Int variable against integer case values)
                BBVariableRegistry bbReg;
                bbReg.LoadFromTemplate(m_template);
                const std::vector<VarSpec> vars = bbReg.GetVariablesByType(VariableType::Int);
                const std::string& curVar   = def.switchVariable;
                const char* previewVar      = curVar.empty() ? "(select variable...)" : curVar.c_str();

                if (ImGui::BeginCombo("Switch Var##vsswitchvar", previewVar))
                {
                    for (size_t vi = 0; vi < vars.size(); ++vi)
                    {
                        const VarSpec& v = vars[vi];
                        bool selected    = (v.name == curVar);
                        if (ImGui::Selectable(v.displayLabel.c_str(), selected))
                        {
                            def.switchVariable = v.name;
                            if (tmplNode)
                                tmplNode->switchVariable = def.switchVariable;
                            m_dirty = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            // ---- Case Labels ----
            if (!def.switchCases.empty())
            {
                ImGui::Separator();
                ImGui::TextDisabled("Case Labels");
            }

            // Ensure our edit buffer stays in sync
            if (m_propEditSwitchCases.size() != def.switchCases.size())
                m_propEditSwitchCases = def.switchCases;

            for (size_t ci = 0; ci < def.switchCases.size(); ++ci)
            {
                // Show pin name as read-only, allow editing the custom label
                const std::string pinLabel = def.switchCases[ci].pinName
                    + " (val=" + def.switchCases[ci].value + ")";
                ImGui::TextUnformatted(pinLabel.c_str());
                ImGui::SameLine();

                char labelBuf[64];
                const std::string& curLabel = m_propEditSwitchCases[ci].customLabel;
                strncpy_s(labelBuf, sizeof(labelBuf), curLabel.c_str(), _TRUNCATE);

                // Unique widget ID per case index
                std::string widgetID = "##vscaselabel" + std::to_string(ci);
                if (ImGui::InputText(widgetID.c_str(), labelBuf, sizeof(labelBuf)))
                {
                    m_propEditSwitchCases[ci].customLabel = labelBuf;
                    // Apply to the live def and template immediately
                    def.switchCases[ci].customLabel = labelBuf;
                    if (tmplNode && ci < tmplNode->switchCases.size())
                        tmplNode->switchCases[ci].customLabel = labelBuf;
                    m_dirty = true;
                }
            }
            break;
        }
        case TaskNodeType::ForEach:
        {
            // Phase 24: Delegate to the dedicated ForEach properties renderer.
            // This shows: blue header -> list variable selector -> loop control options.
            RenderForEachNodeProperties();
            return;
        }
        default:
            break;
    }

    // Breakpoint toggle button
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##vsbp", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                def.NodeName);
    }

    RenderVerificationPanel();
}

// ============================================================================
// Alternative Node Properties Panel renderer
// ============================================================================

void VisualScriptEditorPanel::RenderNodePropertiesPanel()
{
    ImGui::TextDisabled("Node Properties");

    if (m_selectedNodeID < 0)
    {
        ImGui::TextDisabled("(select a node)");
        return;
    }

    // Find the editor node
    VSEditorNode* eNode = nullptr;
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        if (m_editorNodes[i].nodeID == m_selectedNodeID)
        {
            eNode = &m_editorNodes[i];
            break;
        }
    }
    if (eNode == nullptr)
        return;

    TaskNodeDefinition& def = eNode->def;

    // ---- ALL node types: standard fields ----
    {
        // Node Name
        char nameBuf[128];
        strncpy_s(nameBuf, sizeof(nameBuf), def.NodeName.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name##nodeprops_name", nameBuf, sizeof(nameBuf)))
        {
            def.NodeName = nameBuf;
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].NodeName = def.NodeName;
                    break;
                }
            }
            m_dirty = true;
        }

        ImGui::Separator();
    }

    // ---- Type-specific fields (for non-Branch nodes) ----
    // For Branch nodes, the specialized renderer is handled separately
    if (def.Type != TaskNodeType::Branch)
    {
        // Call RenderProperties() which already handles all type-specific fields
        // BUT we need to inline it here to avoid infinite recursion / double-rendering
        // So instead, render just the critical type-specific parts:

        switch (def.Type)
        {
            case TaskNodeType::AtomicTask:
            {
                const std::vector<TaskSpec> tasks = AtomicTaskUIRegistry::Get().GetSortedForUI();
                const std::string& currentTask = def.AtomicTaskID;
                const char* previewLabel = currentTask.empty() ? "(select task...)" : currentTask.c_str();

                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::BeginCombo("Task##nodeprops_task", previewLabel))
                {
                    for (const auto& spec : tasks)
                    {
                        bool selected = (spec.id == currentTask);
                        if (ImGui::Selectable(spec.displayName.c_str(), selected))
                        {
                            def.AtomicTaskID = spec.id;
                            // Auto-fill node name with the action's display name
                            def.NodeName = spec.displayName;
                            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                            {
                                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                {
                                    m_template.Nodes[i].AtomicTaskID = def.AtomicTaskID;
                                    m_template.Nodes[i].NodeName = def.NodeName;
                                    break;
                                }
                            }
                            m_dirty = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Display task parameters
                if (!currentTask.empty())
                {
                    const TaskSpec* taskSpec = AtomicTaskUIRegistry::Get().GetTaskSpec(currentTask);
                    if (taskSpec && !taskSpec->parameters.empty())
                    {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Parameters:");

                        for (const auto& param : taskSpec->parameters)
                        {
                            ImGui::PushID(param.name.c_str());

                            // Build label: parameter name + type hint
                            std::string label = param.name + " (" + param.type + ")";

                            // Get current value from def.Parameters if it exists
                            std::string currentValue = param.defaultValue;
                            auto paramIt = def.Parameters.find(param.name);
                            if (paramIt != def.Parameters.end() && paramIt->second.Type == ParameterBindingType::Literal)
                            {
                                currentValue = paramIt->second.LiteralValue.AsString();
                            }

                            // Display parameter name with description as label
                            ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "%s", param.name.c_str());
                            ImGui::SameLine();

                            // Add help icon (?) next to parameter name for discoverability
                            ImGui::TextDisabled("(?)");

                            // Add tooltip with description if available (on parameter name or help icon)
                            if (ImGui::IsItemHovered() && !param.description.empty())
                            {
                                ImGui::BeginTooltip();
                                ImGui::TextWrapped("%s", param.description.c_str());
                                ImGui::Separator();
                                ImGui::TextDisabled("Type: %s", param.type.c_str());
                                ImGui::TextDisabled("Default: %s", param.defaultValue.c_str());
                                ImGui::EndTooltip();
                            }

                            // Add description text below the parameter name (smaller, grayed out) for immediate clarity
                            if (!param.description.empty())
                            {
                                ImGui::TextDisabled("%s", param.description.c_str());
                            }

                            if (param.type == "Bool")
                            {
                                bool value = (currentValue == "true" || currentValue == "1");
                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::Checkbox(("##" + param.name + "_input").c_str(), &value))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(value);
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }
                            else if (param.type == "Int")
                            {
                                int value = 0;
                                try { value = std::stoi(currentValue); } catch (...) {}
                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::InputInt(("##" + param.name + "_input").c_str(), &value))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(value);
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }
                            else if (param.type == "Float")
                            {
                                float value = 0.0f;
                                try { value = std::stof(currentValue); } catch (...) {}
                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::InputFloat(("##" + param.name + "_input").c_str(), &value, 0.1f))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(value);
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }
                            else if (param.type == "String")
                            {
                                static char buffer[512] = {0};
                                strncpy_s(buffer, currentValue.c_str(), sizeof(buffer) - 1);
                                buffer[sizeof(buffer) - 1] = '\0';

                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::InputText(("##" + param.name + "_input").c_str(), buffer, sizeof(buffer)))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(std::string(buffer));
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }

                            ImGui::Spacing();
                            ImGui::PopID();
                        }
                    }
                }
                break;
            }

            case TaskNodeType::Delay:
            {
                float delay = def.DelaySeconds;
                if (ImGui::InputFloat("Delay (s)##nodeprops_delay", &delay, 0.1f, 1.0f))
                {
                    def.DelaySeconds = delay;
                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].DelaySeconds = def.DelaySeconds;
                            break;
                        }
                    }
                    m_dirty = true;
                }
                break;
            }

            case TaskNodeType::Switch:
            {
                ImGui::TextDisabled("Switch node - edit via modal");
                if (ImGui::Button("Edit Switch Cases"))
                {
                    // Open switch case editor if available
                }
                break;
            }

            case TaskNodeType::GetBBValue:
            case TaskNodeType::SetBBValue:
            {
                const char* nodeType = (def.Type == TaskNodeType::GetBBValue) ? "Get" : "Set";
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%s Blackboard Value", nodeType);
                ImGui::Separator();

                // BBKey dropdown selector from local blackboard variables
                BBVariableRegistry bbReg;
                bbReg.LoadFromTemplate(m_template);
                const std::vector<VarSpec> allVars = bbReg.GetAllVariables();

                const char* previewLabel = def.BBKey.empty() ? "(select variable...)" : def.BBKey.c_str();

                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::BeginCombo("Blackboard Variable##bbkey_combo", previewLabel))
                {
                    for (const auto& var : allVars)
                    {
                        bool selected = (var.name == def.BBKey);
                        if (ImGui::Selectable(var.displayLabel.c_str(), selected))
                        {
                            const std::string oldBBKey = def.BBKey;
                            def.BBKey = var.name;

                            // Sync to template
                            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                            {
                                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                {
                                    m_template.Nodes[i].BBKey = def.BBKey;
                                    break;
                                }
                            }

                            // Push undo command if changed
                            if (def.BBKey != oldBBKey)
                            {
                                m_undoStack.PushCommand(
                                    std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                        m_selectedNodeID, "BBKey",
                                        PropertyValue::FromString(oldBBKey),
                                        PropertyValue::FromString(def.BBKey))),
                                    m_template);
                            }
                            m_dirty = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Display node parameters (common for data nodes)
                if (!def.Parameters.empty())
                {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Parameters:");
                    RenderNodeDataParameters(def);
                }

                break;
            }

            case TaskNodeType::MathOp:
            {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Math Operation");
                ImGui::Separator();

                // MathOpRef operator editor
                static const char* operators[] = { "+", "-", "*", "/", "%", "^" };
                static int operatorIdx = 0;

                if (!def.mathOpRef.mathOperator.empty())
                {
                    for (int i = 0; i < 6; ++i)
                    {
                        if (def.mathOpRef.mathOperator == operators[i])
                        {
                            operatorIdx = i;
                            break;
                        }
                    }
                }

                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::Combo("Operator##mathop", &operatorIdx, operators, 6))
                {
                    def.mathOpRef.mathOperator = operators[operatorIdx];
                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].mathOpRef.mathOperator = operators[operatorIdx];
                            break;
                        }
                    }
                    m_dirty = true;
                }

                // Display operation preview with actual operand values
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.843f, 0.0f, 1.0f), "Operation:");
                ImGui::SameLine();

                // Build left operand display string
                std::string leftStr = "A";
                if (def.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const)
                    leftStr = def.mathOpRef.leftOperand.constValue;
                else if (def.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable)
                    leftStr = "[" + def.mathOpRef.leftOperand.variableName + "]";
                else if (def.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Pin)
                    leftStr = "[Pin]";

                // Build right operand display string
                std::string rightStr = "B";
                if (def.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const)
                    rightStr = def.mathOpRef.rightOperand.constValue;
                else if (def.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable)
                    rightStr = "[" + def.mathOpRef.rightOperand.variableName + "]";
                else if (def.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Pin)
                    rightStr = "[Pin]";

                // Display final operation string in bright green
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), 
                                   "%s %s %s", 
                                   leftStr.c_str(),
                                   def.mathOpRef.mathOperator.empty() ? "?" : def.mathOpRef.mathOperator.c_str(),
                                   rightStr.c_str());

                // Display node parameters
                if (!def.Parameters.empty())
                {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Custom Parameters:");
                    RenderNodeDataParameters(def);
                }

                break;
            }

            case TaskNodeType::SubGraph:
            {
                ImGui::TextDisabled("SubGraph");
                ImGui::TextDisabled("Path: %s", def.SubGraphPath.c_str());
                break;
            }

            case TaskNodeType::Sequence:
            case TaskNodeType::Selector:
            case TaskNodeType::Parallel:
            {
                ImGui::TextDisabled("Control flow node");
                break;
            }

            default:
                ImGui::TextDisabled("(type-specific properties)");
                break;
        }

        ImGui::Separator();
    }

    // ---- Branch-specific: Conditions panel ----
    if (def.Type == TaskNodeType::Branch)
    {
        // Update condition panel with current node's data
        if (m_condPanelNodeID != m_selectedNodeID)
        {
            m_conditionsPanel->SetConditionRefs(def.conditionRefs);
            m_conditionsPanel->SetConditionOperandRefs(def.conditionOperandRefs);
            m_conditionsPanel->SetDynamicPins(def.dynamicPins);
            m_conditionsPanel->SetNodeName(def.NodeName);
            m_condPanelNodeID = m_selectedNodeID;
        }

        // Render the conditions panel
        m_conditionsPanel->Render();

        // Check if dirty and sync back to node
        if (m_conditionsPanel->IsDirty())
        {
            def.conditionRefs = m_conditionsPanel->GetConditionRefs();
            def.conditionOperandRefs = m_conditionsPanel->GetConditionOperandRefs();
            m_conditionsPanel->ClearDirty();
            m_dirty = true;

            // Sync to template
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].conditionRefs = def.conditionRefs;
                    m_template.Nodes[i].conditionOperandRefs = def.conditionOperandRefs;
                    break;
                }
            }
        }

        ImGui::Separator();
    }

    // ---- ALL nodes: Breakpoint ----
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##nodeprops_bp", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                def.NodeName);
    }
}

} // namespace Olympe
