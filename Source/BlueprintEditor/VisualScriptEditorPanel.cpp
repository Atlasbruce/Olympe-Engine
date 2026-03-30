/**
 * @file VisualScriptEditorPanel.cpp
 * @brief ImNodes graph editor implementation for ATS VS graphs (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant — no std::optional, structured bindings, std::filesystem.
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
// Lifecycle Methods
// ============================================================================
// NOTE: Core lifecycle methods (Constructor, Destructor, Initialize, Shutdown)
// have been extracted to VisualScriptEditorPanel_Core.cpp for better organization
// and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_Core.cpp:
//   - VisualScriptEditorPanel() Constructor
//   - ~VisualScriptEditorPanel() Destructor
//   - Initialize() — ImNodes context, UI helpers, preset loading
//   - Shutdown() — Resource cleanup and registry management
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_Core.cpp

// ============================================================================
// Helper Methods
// ============================================================================
// NOTE: 6 helper methods have been extracted to VisualScriptEditorPanel_Helpers.cpp
// for better code organization and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_Helpers.cpp:
//   - AllocNodeID() — Allocate unique node IDs
//   - AllocLinkID() — Allocate unique link IDs
//   - ExecInAttrUID(int nodeID) — UID for execution input pins
//   - ExecOutAttrUID(int nodeID, int pinIndex) — UID for execution output pins
//   - DataInAttrUID(int nodeID, int pinIndex) — UID for data input pins
//   - DataOutAttrUID(int nodeID, int pinIndex) — UID for data output pins
//
// UID Scheme (nodeID * 10000 + offset):
//   0–99:   Reserved for exec-in
//   100–199: Exec-out pins
//   200–299: Data-in pins
//   300–399: Data-out pins
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_Helpers.cpp

// ============================================================================
// Pin Name Helpers
// ============================================================================
// NOTE: 5 pin helper methods have been extracted to VisualScriptEditorPanel_PinHelpers.cpp
// for better code organization and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_PinHelpers.cpp:
//   - GetExecInputPins(TaskNodeType type) — Static exec input pins by node type
//   - GetExecOutputPins(TaskNodeType type) — Static exec output pins by node type
//   - GetExecOutputPinsForNode(const TaskNodeDefinition& def) — Inc. dynamic pins
//   - GetDataInputPins(TaskNodeType type) — Static data input pins by node type
//   - GetDataOutputPins(TaskNodeType type) — Static data output pins by node type
//
// Pin Categories (by node type):
//   - EntryPoint: exec-out only {"Out"}
//   - Branch: exec-out {"Then", "Else"} + dynamic data-in pins
//   - While: exec-out {"Loop", "Completed"}
//   - ForEach: exec-out {"Loop Body", "Completed"}
//   - AtomicTask: exec-in/out {"In"} / {"Completed"}
//   - MathOp: data-in {"A", "B"}, data-out {"Result"} (data-pure)
//   - GetBBValue: data-out {"Value"} (data-pure)
//   - SetBBValue: exec-in/out, data-in {"Value"}
//   - SubGraph: exec-in/out
//   - VSSequence: exec-out + dynamic pins
//   - Switch: exec-out + dynamic pins
//   - Delay: exec-in/out
//   - DoOnce: exec-in/out
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_PinHelpers.cpp

// ============================================================================
// Node management
// ============================================================================

// ============================================================================
// Node Management Methods
// ============================================================================
// NOTE: 4 node management methods (AddNode, RemoveNode, ConnectExec, ConnectData)
// have been extracted to VisualScriptEditorPanel_NodeManagement.cpp for better
// code organization and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_NodeManagement.cpp:
//   - AddNode() — Create a new node with type-specific data pin initialization
//   - RemoveNode() — Delete a node and all associated connections
//   - ConnectExec() — Create an execution pin connection between nodes
//   - ConnectData() — Create a data pin connection between nodes
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_NodeManagement.cpp
//
// These methods integrate with the undo/redo command system (ICommand/UndoStack)
// to ensure all graph modifications (node creation, deletion, link creation) can
// be reversed via Ctrl+Z. All operations update both m_editorNodes (canvas state)
// and m_template (model), with link graph rebuilding after each change.

// ============================================================================
// Template / Canvas Sync
// ============================================================================
// NOTE: 5 template synchronization methods have been extracted to 
// VisualScriptEditorPanel_TemplateSync.cpp (Phase 6).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_TemplateSync.cpp
//
// Methods included:
//   - SyncCanvasFromTemplate() — Load nodes from template into editor canvas
//   - SyncTemplateFromCanvas() — Update template with current editor node state
//   - RebuildLinks() — Rebuild all visual links from template connections
//   - SyncEditorNodesFromTemplate() — Restore editor nodes during undo/redo
//   - RemoveLink(int linkID) — Delete a link and push undo command

// ============================================================================
// Load / Save
// ============================================================================
// NOTE: 6 file operation methods have been extracted to 
// VisualScriptEditorPanel_FileOperations.cpp (Phase 7).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_FileOperations.cpp
//
// Methods included:
//   - LoadTemplate() — Load blueprint from file/memory with preset loading (Phase 24)
//   - Save() — Save current graph to m_currentPath
//   - SaveAs() — Save graph to new path
//   - SyncNodePositionsFromImNodes() — Sync grid-space positions (BUG-003 Fix)
//   - SyncPresetsFromRegistryToTemplate() — Phase 24 preset synchronization
//   - SerializeAndWrite() — Complete JSON v4 serialization with all Phase 24 features
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_FileOperations.cpp

// ============================================================================
// Blackboard validation helpers (BUG-002 Fix #1)
// ============================================================================
// NOTE: ValidateAndCleanBlackboardEntries() and CommitPendingBlackboardEdits()
// have been extracted to VisualScriptEditorPanel_Utilities.cpp (Phase 5).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Utilities.cpp

// ============================================================================
// BUG-003 Viewport helpers
// ============================================================================
// NOTE: ResetViewportBeforeSave(), AfterSave(), and ScreenToCanvasPos()
// have been extracted to VisualScriptEditorPanel_Utilities.cpp (Phase 5).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Utilities.cpp

// ============================================================================
// UX Enhancement #3 — Type-filtered variable utility
// ============================================================================
// NOTE: GetVariablesByType() has been extracted to 
// VisualScriptEditorPanel_Utilities.cpp (Phase 5).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Utilities.cpp

// ============================================================================
// Rendering
// ============================================================================

// ============================================================================
// Undo/Redo wrappers, Rendering (extracted to VisualScriptEditorPanel_RenderingCore.cpp)
// ============================================================================
// NOTE: Core rendering methods have been extracted to VisualScriptEditorPanel_RenderingCore.cpp
// (Phase 8 refactoring).
//
// Methods implemented in VisualScriptEditorPanel_RenderingCore.cpp:
//   - PerformUndo() — Undo operation with position restoration
//   - PerformRedo() — Redo operation with position restoration
//   - Render() — Main render function (delegates to RenderContent)
//   - RenderContent() — Main content layout (toolbar, canvas, properties)
//   - RenderToolbar() — Toolbar with save/load/verify buttons
//   - RenderSaveAsDialog() — Save-As modal dialog
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_RenderingCore.cpp



// ============================================================================
// Canvas Rendering, Node Palette, Context Menus (Phase 9 Extraction)
// ============================================================================
// Implementation extracted to VisualScriptEditorPanel_Canvas.cpp
//
// Extracted methods (~1053 LOC total):
//   - void RenderCanvas() — ImNodes graph rendering, node palette detection, link management
//   - void RenderNodePalette() — Right-click context menu for adding nodes
//   - void RenderContextMenus() — Node/link right-click context menus
//
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Canvas.cpp
//
// Forward declarations (implemented in Canvas.cpp):
//   void RenderCanvas();
//   void RenderNodePalette();
//   void RenderContextMenus();


// ============================================================================
// Branch / While node — dedicated Properties panel renderer
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
        entry.Default  = GetDefaultValueForType(VariableType::Int);  // UX Fix #1
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

        // Fix #2: Type selector — "None" is excluded to prevent invalid entries.
        // Enum layout: None=0, Bool=1, Int=2, Float=3, Vector=4, EntityID=5, String=6.
        // typeIdx maps to enum value minus 1 (offset by 1 to skip None).
        const char* typeLabels[] = {"Bool","Int","Float","Vector","EntityID","String"};
        int typeIdx = static_cast<int>(entry.Type) - 1; // offset: Bool->0, Int->1, ...
        if (typeIdx < 0 || typeIdx >= 6)
        {
            typeIdx    = 1; // default to "Int" (array index 1; maps to VariableType::Int via typeIdx+1)
            entry.Type = VariableType::Int;
        }
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &typeIdx, typeLabels, 6))
        {
            VariableType newType = static_cast<VariableType>(typeIdx + 1); // +1 to skip None
            entry.Type    = newType;
            entry.Default = GetDefaultValueForType(newType);  // UX Fix #1: sync default
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

        // UX Fix #2: Default value editor (type-specific input field)
        ImGui::TextDisabled("Default:");
        ImGui::SameLine();
        switch (entry.Type)
        {
            case VariableType::Bool:
            {
                bool bVal = entry.Default.IsNone() ? false : entry.Default.AsBool();
                if (ImGui::Checkbox("##bbval", &bVal))
                {
                    entry.Default = TaskValue(bVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Int:
            {
                int iVal = entry.Default.IsNone() ? 0 : entry.Default.AsInt();
                ImGui::SetNextItemWidth(70.0f);
                if (ImGui::InputInt("##bbval", &iVal))
                {
                    entry.Default = TaskValue(iVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Float:
            {
                float fVal = entry.Default.IsNone() ? 0.0f : entry.Default.AsFloat();
                ImGui::SetNextItemWidth(70.0f);
                if (ImGui::InputFloat("##bbval", &fVal, 0.0f, 0.0f, "%.3f"))
                {
                    entry.Default = TaskValue(fVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::String:
            {
                std::string sVal = entry.Default.IsNone() ? "" : entry.Default.AsString();
                char sBuf[128];
                strncpy_s(sBuf, sizeof(sBuf), sVal.c_str(), _TRUNCATE);
                ImGui::SetNextItemWidth(100.0f);
                if (ImGui::InputText("##bbval", sBuf, sizeof(sBuf)))
                {
                    entry.Default = TaskValue(std::string(sBuf));
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Vector:
            {
                // UX Enhancement #1: Vector is auto-sourced from entity position at runtime.
                // Display as read-only to prevent user from entering a value that will be
                // overwritten anyway.
                ImGui::BeginDisabled(true);
                float vecVal[3] = { 0.0f, 0.0f, 0.0f };
                ImGui::SetNextItemWidth(140.0f);
                ImGui::DragFloat3("##bbval", vecVal, 0.1f);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("(auto from entity position)");
                break;
            }
            case VariableType::EntityID:
            {
                // UX Enhancement #2: EntityID is assigned at runtime; read-only display.
                ImGui::BeginDisabled(true);
                int entityId = 0;
                ImGui::SetNextItemWidth(70.0f);
                ImGui::InputInt("##bbval", &entityId);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("(assigned at runtime)");
                break;
            }
            default:
                ImGui::TextDisabled("(n/a)");
                break;
        }

        ImGui::PopID();
    }
}

void VisualScriptEditorPanel::RenderValidationOverlay()
{
    m_validationWarnings.clear();
    m_validationErrors.clear();

    // Check: every non-EntryPoint node should have at least one exec-in connection
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        const VSEditorNode& eNode = m_editorNodes[i];
        if (eNode.def.Type == TaskNodeType::EntryPoint)
            continue;

        bool hasExecIn = false;
        for (size_t c = 0; c < m_template.ExecConnections.size(); ++c)
        {
            if (m_template.ExecConnections[c].TargetNodeID == eNode.nodeID)
            {
                hasExecIn = true;
                break;
            }
        }
        if (!hasExecIn)
        {
            m_validationErrors.push_back(
                "Node " + std::to_string(eNode.nodeID) + " (" +
                eNode.def.NodeName + "): no exec-in connection");
        }

        // SubGraph path validation
        if (eNode.def.Type == TaskNodeType::SubGraph &&
            eNode.def.SubGraphPath.empty())
        {
            m_validationWarnings.push_back(
                "Node " + std::to_string(eNode.nodeID) +
                " (SubGraph): SubGraphPath is empty");
        }
    }
}

// ============================================================================
// Phase 21-B — Graph Verification
// ============================================================================

void VisualScriptEditorPanel::RunVerification()
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() called for graph '"
               << m_template.Name << "'\n";
    m_verificationResult = VSGraphVerifier::Verify(m_template);
    m_verificationDone   = true;

    // Phase 24.3 — Populate verification logs for display in the output panel
    m_verificationLogs.clear();
    for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
    {
        const VSVerificationIssue& issue = m_verificationResult.issues[i];
        std::string logEntry;

        // Format: "[SEVERITY] message (Node: nodeID)"
        if (issue.severity == VSVerificationSeverity::Error)
            logEntry = "[ERROR] ";
        else if (issue.severity == VSVerificationSeverity::Warning)
            logEntry = "[WARN] ";
        else
            logEntry = "[INFO] ";

        logEntry += issue.message;
        if (issue.nodeID >= 0)
            logEntry += " (Node: " + std::to_string(issue.nodeID) + ")";

        m_verificationLogs.push_back(logEntry);
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() done: "
               << m_verificationResult.issues.size() << " issue(s), "
               << "errors=" << (m_verificationResult.HasErrors()   ? "yes" : "no") << ", "
               << "warnings=" << (m_verificationResult.HasWarnings() ? "yes" : "no") << "\n";
}

void VisualScriptEditorPanel::RenderVerificationPanel()
{
    ImGui::Separator();
    ImGui::TextDisabled("Graph Verification");

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("Click 'Verify' in toolbar to run verification.");
        return;
    }

    // Global status line
    if (m_verificationResult.HasErrors())
    {
        int errorCount = 0;
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            if (m_verificationResult.issues[i].severity == VSVerificationSeverity::Error)
                ++errorCount;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                           "Errors found: %d", errorCount);
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "OK — warnings present");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "OK — no issues");
    }

    if (m_verificationResult.issues.empty())
        return;

    // List issues grouped: Errors first, then Warnings, then Info
    const VSVerificationSeverity orderedSev[3] = {
        VSVerificationSeverity::Error,
        VSVerificationSeverity::Warning,
        VSVerificationSeverity::Info
    };

    for (int s = 0; s < 3; ++s)
    {
        VSVerificationSeverity sev = orderedSev[s];
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            const VSVerificationIssue& issue = m_verificationResult.issues[i];
            if (issue.severity != sev)
                continue;

            ImGui::PushID(static_cast<int>(i));

            if (sev == VSVerificationSeverity::Error)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[E]");
            else if (sev == VSVerificationSeverity::Warning)
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[W]");
            else
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[I]");

            ImGui::SameLine();
            ImGui::Text("%s: %s", issue.ruleID.c_str(), issue.message.c_str());

            if (issue.nodeID >= 0)
            {
                ImGui::SameLine();
                std::string btnLabel = "Go##go" + std::to_string(i);
                if (ImGui::SmallButton(btnLabel.c_str()))
                {
                    m_focusNodeID    = issue.nodeID;
                    m_selectedNodeID = issue.nodeID;
                }
            }

            ImGui::PopID();
        }
    }
}

// ============================================================================
// Phase 24.3 — Verification Logs Panel
// ============================================================================

void VisualScriptEditorPanel::RenderVerificationLogsPanel()
{
    // Note: The header "Verification Output" is rendered by the container (BlueprintEditorGUI),
    // so we only render the content here (status + logs).

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("(Click 'Verify' button to run verification)");
        return;
    }

    // Display verification result summary
    ImGui::Spacing();

    // Debug: Show issue count
    ImGui::TextDisabled("Issues found: %zu", m_verificationResult.issues.size());

    // Status line with color coding
    if (m_verificationResult.HasErrors())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                          "[ERROR] Graph has %d error(s)", 
                          (int)m_verificationResult.issues.size());
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), 
                          "[WARNING] Graph is valid but has warnings");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), 
                          "[OK] Graph is valid - no issues found");
    }

    ImGui::Separator();

    // Display issues grouped by severity
    ImGui::BeginChild("VerificationLogsChild", ImVec2(0, 0), true);

    const VSVerificationSeverity orderedSev[3] = {
        VSVerificationSeverity::Error,
        VSVerificationSeverity::Warning,
        VSVerificationSeverity::Info
    };

    const char* sevLabels[3] = { "[ERROR]", "[WARN]", "[INFO]" };
    ImVec4      sevColors[3] = {
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),  // Error: red
        ImVec4(1.0f, 0.85f, 0.0f, 1.0f), // Warning: yellow
        ImVec4(0.5f, 0.8f, 1.0f, 1.0f)   // Info: light blue
    };

    for (int s = 0; s < 3; ++s)
    {
        VSVerificationSeverity sev = orderedSev[s];
        bool hasThisSeverity = false;

        // Count issues of this severity
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            if (m_verificationResult.issues[i].severity == sev)
            {
                hasThisSeverity = true;
                break;
            }
        }

        if (!hasThisSeverity)
            continue;

        // Display header for this severity level
        ImGui::TextColored(sevColors[s], "%s", sevLabels[s]);

        // Display all issues with this severity
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            const VSVerificationIssue& issue = m_verificationResult.issues[i];
            if (issue.severity != sev)
                continue;

            // Format message: "[SEVERITY] message (NodeID: xxx)"
            std::string message = issue.message;
            if (issue.nodeID >= 0)
            {
                message += " (Node: " + std::to_string(issue.nodeID) + ")";
            }

            ImGui::BulletText("%s", message.c_str());
        }

        ImGui::Spacing();
    }

    ImGui::EndChild();
}

// ============================================================================
// Phase 23-B.4 — Condition Editor UI helpers
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

// ============================================================================
// PHASE 24 Panel Integration — Part B: Preset Bank
// ============================================================================

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
        // Persist the deletion to disk
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

void VisualScriptEditorPanel::RenderLocalVariablesPanel()
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
        entry.Default  = GetDefaultValueForType(VariableType::Int);  // UX Fix #1
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

        // ── Name (editable text field) ──
        ImGui::SetNextItemWidth(140.0f);
        if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
        {
            entry.Key = keyBuf;
            m_pendingBlackboardEdits[idx] = keyBuf;
            m_dirty = true;
        }

        ImGui::SameLine();

        // ── Type dropdown ──
        const char* typeNames[] = { "None", "Bool", "Int", "Float", "String", "Vector" };
        const VariableType typeValues[] = {
            VariableType::None, VariableType::Bool, VariableType::Int,
            VariableType::Float, VariableType::String, VariableType::Vector
        };
        int curTypeIdx = 0;
        for (int ti = 0; ti < 6; ++ti)
            if (entry.Type == typeValues[ti])
            { curTypeIdx = ti; break; }

        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &curTypeIdx, typeNames, 6))
        {
            entry.Type = typeValues[curTypeIdx];
            entry.Default = GetDefaultValueForType(entry.Type);
            m_dirty = true;
        }

        // ── Default value (type-aware editor) ──
        if (entry.Type != VariableType::None)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("Default:");
            ImGui::SameLine();
            RenderConstValueInput(entry.Default, entry.Type, "##bbdefault");
        }

        // ── Global toggle ──
        ImGui::SameLine();
        bool isGlobal = entry.IsGlobal;
        if (ImGui::Checkbox("G##bbglobal", &isGlobal))
        {
            entry.IsGlobal = isGlobal;
            m_dirty = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Mark as global variable");

        // ── Delete button ──
        ImGui::SameLine();
        if (ImGui::Button("X##bbdel"))
        {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
            m_pendingBlackboardEdits.erase(idx);
            m_dirty = true;
        }

        ImGui::PopID();
    }
}

// ============================================================================
// Phase 24 Global Blackboard Integration — RenderGlobalVariablesPanel (Enhanced)
// ============================================================================

void VisualScriptEditorPanel::RenderGlobalVariablesPanel()
{
    ImGui::TextDisabled("Global Variables (Editor Instance)");
    ImGui::Separator();

    // Get reference to the global template registry (non-const for Add)
    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

    // Add Global Variable button
    if (ImGui::Button("+##globalVarAdd", ImVec2(30, 0)))
    {
        ImGui::OpenPopup("AddGlobalVariablePopup");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add global variable");

    // Add Global Variable Modal Dialog
    if (ImGui::BeginPopupModal("AddGlobalVariablePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char newVarName[128] = "newGlobal";
        static int newVarTypeIdx = 2;  // Default to Int
        static char newVarDescription[256] = "Enter description...";

        ImGui::InputText("Variable Name##new", newVarName, sizeof(newVarName));

        const char* typeOptions[] = { "Bool", "Int", "Float", "String", "Vector", "EntityID" };
        const VariableType typeValues[] = {
            VariableType::Bool, VariableType::Int, VariableType::Float,
            VariableType::String, VariableType::Vector, VariableType::EntityID
        };
        ImGui::Combo("Type##new", &newVarTypeIdx, typeOptions, 6);

        ImGui::InputTextMultiline("Description##new", newVarDescription, sizeof(newVarDescription), ImVec2(0, 60));

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (strlen(newVarName) > 0 && !gtb.HasVariable(newVarName))
            {
                TaskValue defaultVal = GetDefaultValueForType(typeValues[newVarTypeIdx]);
                if (gtb.AddVariable(newVarName, typeValues[newVarTypeIdx], defaultVal, newVarDescription, false))
                {
                    SYSTEM_LOG << "[VSEditor] Created new global variable: " << newVarName << "\n";
                    gtb.SaveToFile();  // Use last loaded path automatically

                    // Phase 24: Hot reload to refresh registry and propagate to all panels
                    GlobalTemplateBlackboard::Reload();

                    m_dirty = true;

                    // Reset form
                    memset(newVarName, 0, sizeof(newVarName));
                    strcpy_s(newVarName, sizeof(newVarName), "newGlobal");
                    newVarTypeIdx = 2;
                    memset(newVarDescription, 0, sizeof(newVarDescription));
                    strcpy_s(newVarDescription, sizeof(newVarDescription), "Enter description...");

                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();

    if (globalVars.empty())
    {
        ImGui::TextDisabled("(no global variables defined)");
        ImGui::TextDisabled("Click [+] above to create new global variables");
        return;
    }

    ImGui::TextDisabled("Global variables from project registry");
    ImGui::TextDisabled("Values shown are editor-specific (persisted with graph)");
    ImGui::Separator();

    // Check if EntityBlackboard is initialized
    if (!m_entityBlackboard)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "[ERROR] EntityBlackboard not initialized");
        return;
    }

    // Display each global variable with editable entity-specific value
    for (size_t gi = 0; gi < globalVars.size(); ++gi)
    {
        const GlobalEntryDefinition& globalDef = globalVars[gi];

        ImGui::PushID(static_cast<int>(gi));

        // ---- Variable name (read-only) with type label + Delete button ----
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "(%s) %s",
                          VariableTypeToString(globalDef.Type).c_str(),
                          globalDef.Key.c_str());

        ImGui::SameLine();
        ImGui::TextDisabled("(%.1f KB)", 0.1f);  // Placeholder space
        ImGui::SameLine();

        // Delete button for global variable
        if (ImGui::SmallButton("Delete##globalvar"))
        {
            // Mark for deletion (we'll process after the loop to avoid iterator invalidation)
            std::string varToDelete = globalDef.Key;
            if (gtb.RemoveVariable(varToDelete))
            {
                SYSTEM_LOG << "[VSEditor] Deleted global variable: " << varToDelete << "\n";
                gtb.SaveToFile();  // Use last loaded path automatically

                // Phase 24: Hot reload to refresh registry
                GlobalTemplateBlackboard::Reload();

                m_dirty = true;
            }
            ImGui::PopID();
            continue;  // Skip rendering the rest of this variable's UI
        }

        // ---- Description (if available) ----
        if (!globalDef.Description.empty())
        {
            ImGui::TextDisabled("  %s", globalDef.Description.c_str());
        }

        // Create unique table ID per global variable to avoid ImGui::BeginTable() failures
        std::string tableId = "##GlobalVarTable_" + std::to_string(gi);
        if (ImGui::BeginTable(tableId.c_str(), 2, ImGuiTableFlags_SizingStretchSame, ImVec2(0, 0)))
        {
            ImGui::TableSetupColumn("Label", 0);
            ImGui::TableSetupColumn("Value", 0);

            // ---- Default Value (read-only) ----
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Default:");
            ImGui::TableSetColumnIndex(1);

            const TaskValue& defaultValue = globalDef.DefaultValue;
            std::string defaultStr;
            switch (globalDef.Type)
            {
                case VariableType::Bool:
                    defaultStr = defaultValue.IsNone() ? "false" : (defaultValue.AsBool() ? "true" : "false");
                    break;
                case VariableType::Int:
                    defaultStr = defaultValue.IsNone() ? "0" : std::to_string(defaultValue.AsInt());
                    break;
                case VariableType::Float:
                {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2);
                    oss << (defaultValue.IsNone() ? 0.0f : defaultValue.AsFloat());
                    defaultStr = oss.str();
                    break;
                }
                case VariableType::String:
                    defaultStr = defaultValue.IsNone() ? "" : defaultValue.AsString();
                    break;
                case VariableType::Vector:
                    defaultStr = "(vector)";
                    break;
                case VariableType::EntityID:
                    defaultStr = defaultValue.IsNone() ? "0" : std::to_string(static_cast<int>(defaultValue.AsEntityID()));
                    break;
                default:
                    defaultStr = "(unknown)";
                    break;
            }
            ImGui::TextDisabled("%s", defaultStr.c_str());

            // ---- Current Value (editable with scope resolution) ----
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Current:");
            ImGui::TableSetColumnIndex(1);

            // Use scoped variable access to get/set entity-specific value
            std::string scopedVarName = "(G)" + globalDef.Key;
            TaskValue currentValue = m_entityBlackboard->GetValueScoped(scopedVarName);

            // Create type-specific input widget
            bool valueChanged = false;
            switch (globalDef.Type)
            {
                case VariableType::Bool:
                {
                    bool bVal = currentValue.IsNone() ? false : currentValue.AsBool();
                    if (ImGui::Checkbox("##bool_val", &bVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(bVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Int:
                {
                    int iVal = currentValue.IsNone() ? 0 : currentValue.AsInt();
                    if (ImGui::InputInt("##int_val", &iVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(iVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Float:
                {
                    float fVal = currentValue.IsNone() ? 0.0f : currentValue.AsFloat();
                    if (ImGui::InputFloat("##float_val", &fVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(fVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::String:
                {
                    static std::unordered_map<size_t, std::vector<char>> stringBuffers;
                    size_t bufKey = gi; // Use index as unique key for buffer storage
                    if (stringBuffers.find(bufKey) == stringBuffers.end())
                    {
                        std::string initialStr = currentValue.IsNone() ? "" : currentValue.AsString();
                        stringBuffers[bufKey] = std::vector<char>(initialStr.begin(), initialStr.end());
                        stringBuffers[bufKey].push_back('\0');
                        stringBuffers[bufKey].resize(256);  // Allocate buffer
                    }

                    ImGui::SetNextItemWidth(-1.0f);
                    if (ImGui::InputText("##string_val", stringBuffers[bufKey].data(), 256, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        std::string newStr(stringBuffers[bufKey].data());
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(newStr));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Vector:
                {
                    Vector vVal = currentValue.IsNone() ? Vector{0.0f, 0.0f, 0.0f} : currentValue.AsVector();
                    float vArray[3] = {vVal.x, vVal.y, vVal.z};
                    if (ImGui::InputFloat3("##vector_val", vArray))
                    {
                        Vector newVec{vArray[0], vArray[1], vArray[2]};
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(newVec));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::EntityID:
                {
                    int eID = currentValue.IsNone() ? 0 : static_cast<int>(currentValue.AsEntityID());
                    if (ImGui::InputInt("##entityid_val", &eID))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(eID >= 0 ? eID : 0));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                default:
                    ImGui::TextDisabled("(unsupported type)");
                    break;
            }

            // ---- Persistent flag ----
            if (globalDef.IsPersistent)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("Flags:");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.5f, 1.0f), "[Persistent]");
            }

            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::PopID();
    }
}

} // namespace Olympe
