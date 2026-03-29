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
// - RenderBranchNodeProperties()          : Branch node condition editor
// - RenderMathOpNodeProperties()          : Math operator editor
// - RenderNodeDataParameters()            : Data pin parameter binding
// - RenderProperties()                    : Main properties panel dispatcher
// - RenderConditionEditor()               : Condition expression editor
// - RenderVariableSelector()              : Variable dropdown selector
// - RenderConstValueInput()              : Const value input field
// - RenderPinSelector()                   : Pin dropdown selector
// - BuildConditionPreview()               : Human-readable condition string
// - RenderNodePropertiesPanel()           : Node-specific properties
//
// Integration Points:
// - m_selectedNodeID                      : Currently selected node
// - m_template.Nodes                      : Node definitions
// - m_conditionsPanel                     : Condition UI helper
// - m_mathOpPanel                         : Math operator UI helper
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
            // This shows: blue header → NodeConditionsPanel → Breakpoint checkbox.
            // The return prevents any legacy condition UI from also rendering.
            RenderBranchNodeProperties(*eNode, def);
            return;
        }
        case TaskNodeType::SubGraph:
        {
            char sgPathBuf[256];
            strncpy_s(sgPathBuf, sizeof(sgPathBuf), def.SubGraphPath.c_str(), _TRUNCATE);
            if (ImGui::InputText("SubGraph Path##vssg", sgPathBuf, sizeof(sgPathBuf)))
            {
                def.SubGraphPath = sgPathBuf;
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].SubGraphPath = def.SubGraphPath;
                        break;
                    }
                }
                m_dirty = true;
            }
            if (ImGui::IsItemActivated())
            {
                m_propEditOldSubGraphPath = def.SubGraphPath;
                m_propEditNodeIDOnFocus   = m_selectedNodeID;
            }
            if (ImGui::IsItemDeactivatedAfterEdit() &&
                m_propEditNodeIDOnFocus == m_selectedNodeID &&
                def.SubGraphPath != m_propEditOldSubGraphPath)
            {
                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                        m_selectedNodeID, "SubGraphPath",
                        PropertyValue::FromString(m_propEditOldSubGraphPath),
                        PropertyValue::FromString(def.SubGraphPath))),
                    m_template);
            }
            break;
        }
        case TaskNodeType::MathOp:
        {
            // Phase 24 Milestone 2: Delegate to the dedicated MathOp properties renderer.
            // This shows: blue header → MathOpPropertyPanel → operand editors.
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

// ----------------------------------------------------------------------------


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

// ----------------------------------------------------------------------------


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

// ----------------------------------------------------------------------------


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

// ----------------------------------------------------------------------------

/*static*/

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
// PHASE 24 Panel Integration — Part A: Node Properties
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
                                char buffer[512] = {0};
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

// ============================================================================
// PHASE 24 Panel Integration — Part B: Preset Bank
// ============================================================================


} // namespace Olympe
