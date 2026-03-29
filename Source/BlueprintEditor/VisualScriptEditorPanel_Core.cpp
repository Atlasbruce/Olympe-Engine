/**
 * @file VisualScriptEditorPanel_Core.cpp
 * @brief Core lifecycle and dispatcher methods for VisualScriptEditorPanel (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Part of the VisualScriptEditorPanel refactoring (Phase 1).
 * This file contains:
 * - Constructor / Destructor
 * - Initialize / Shutdown (lifecycle)
 * - Main Render dispatcher
 * - Core public accessors
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
// Constructor / Destructor
// ============================================================================

VisualScriptEditorPanel::VisualScriptEditorPanel()
{
    std::memset(m_saveAsFilename, 0, sizeof(m_saveAsFilename));
    strcpy_s(m_saveAsFilename, sizeof(m_saveAsFilename), "untitled_graph");
}

VisualScriptEditorPanel::~VisualScriptEditorPanel()
{
}

// ============================================================================
// Lifecycle: Initialize / Shutdown
// ============================================================================

void VisualScriptEditorPanel::Initialize()
{
    // Create a dedicated ImNodes editor context for this panel instance.
    // This ensures that node positions and canvas panning are tracked
    // independently for each open tab (switching tabs preserves layout).
    m_imnodesContext = ImNodes::EditorContextCreate();

    // Phase 24 — Condition Preset UI: create helpers bound to m_presetRegistry.
    m_pinManager      = std::unique_ptr<DynamicDataPinManager>(
                            new DynamicDataPinManager(m_presetRegistry));
    m_branchRenderer  = std::unique_ptr<NodeBranchRenderer>(
                            new NodeBranchRenderer(m_presetRegistry, *m_pinManager));
    m_conditionsPanel = std::unique_ptr<NodeConditionsPanel>(
                            new NodeConditionsPanel(m_presetRegistry));
    m_mathOpPanel     = std::unique_ptr<MathOpPropertyPanel>(
                            new MathOpPropertyPanel(m_presetRegistry, *m_pinManager));
     m_getBBPanel      = std::unique_ptr<GetBBValuePropertyPanel>(
                             new GetBBValuePropertyPanel());
    m_setBBPanel      = std::unique_ptr<SetBBValuePropertyPanel>(
                             new SetBBValuePropertyPanel());
    m_variablePanel   = std::unique_ptr<VariablePropertyPanel>(
                             new VariablePropertyPanel());
    m_libraryPanel    = std::unique_ptr<ConditionPresetLibraryPanel>(
                             new ConditionPresetLibraryPanel(m_presetRegistry));

    // Phase 24 Global Blackboard Integration: Create EntityBlackboard for managing
    // both local and global variables in the editor context (entity ID 0)
    m_entityBlackboard = std::unique_ptr<EntityBlackboard>(
                            new EntityBlackboard(0));  // 0 = editor context entity

    // Wire the pin-regeneration callback so the Edit-Conditions modal can
    // trigger a canvas update when the user confirms changes.
    m_conditionsPanel->OnDynamicPinsNeedRegeneration = [this]()
    {
        if (m_selectedNodeID < 0)
            return;
        for (size_t ni = 0; ni < m_editorNodes.size(); ++ni)
        {
            VSEditorNode& eNode = m_editorNodes[ni];
            if (eNode.nodeID != m_selectedNodeID)
                continue;

            // Phase 24: Get FRESH condition data from panel (not stale data from eNode)
            // This ensures that edits via RenderConditionList dropdown are picked up
            std::vector<NodeConditionRef> freshConditionRefs = m_conditionsPanel->GetConditionRefs();
            std::vector<ConditionRef> freshOperandRefs = m_conditionsPanel->GetConditionOperandRefs();

            // Sync fresh data to eNode
            eNode.def.conditionRefs = freshConditionRefs;
            eNode.def.conditionOperandRefs = freshOperandRefs;

            // Regenerate pins with FRESH operand data
            m_pinManager->RegeneratePinsFromConditions(freshConditionRefs, freshOperandRefs);
            eNode.def.dynamicPins = m_pinManager->GetAllPins();

            // Keep m_template in sync for serialization.
            for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
            {
                if (m_template.Nodes[ti].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[ti].conditionRefs = eNode.def.conditionRefs;
                    m_template.Nodes[ti].conditionOperandRefs = eNode.def.conditionOperandRefs;
                    m_template.Nodes[ti].dynamicPins   = eNode.def.dynamicPins;
                    break;
                }
            }
            m_conditionsPanel->SetDynamicPins(eNode.def.dynamicPins);
            m_dirty = true;
            break;
        }
    };

    // Phase 24 — Load presets from the graph (now embedded in blueprint JSON)
    // instead of from an external file. This makes each blueprint self-contained.
    // If the graph has presets, populate the registry; otherwise leave empty.
    if (!m_template.Presets.empty())
    {
        m_presetRegistry.LoadFromPresetList(m_template.Presets);
        SYSTEM_LOG << "[VSEditor] Initialize: loaded " << m_template.Presets.size()
                   << " presets from graph '" << m_template.Name << "'\n";
    }
    else
    {
        SYSTEM_LOG << "[VSEditor] Initialize: graph '" << m_template.Name
                   << "' has no embedded presets\n";
    }
}

void VisualScriptEditorPanel::Shutdown()
{
    if (m_imnodesContext)
    {
        ImNodes::EditorContextFree(m_imnodesContext);
        m_imnodesContext = nullptr;
    }
    m_editorNodes.clear();
    m_editorLinks.clear();
    m_positionedNodes.clear();

    // Phase 24 — release helpers before registry is destroyed.
    m_conditionsPanel.reset();
    m_mathOpPanel.reset();
    m_getBBPanel.reset();
    m_setBBPanel.reset();
    m_variablePanel.reset();
    m_libraryPanel.reset();
    m_branchRenderer.reset();
    m_pinManager.reset();
    m_condPanelNodeID = -1;
}

// ============================================================================
// Main Render Dispatcher
// ============================================================================

void VisualScriptEditorPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("VS Graph Editor", &m_visible);
    RenderContent();
    ImGui::End();

    // Render the condition preset library panel (Phase 24 UI integration)
    m_libraryPanel->Render();
}

void VisualScriptEditorPanel::RenderContent()
{
    RenderToolbar();
    RenderSaveAsDialog();
    ImGui::Separator();

    // Two-column layout: canvas (left) | resize handle | properties panel (right, 3 sub-panels)
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // Initialize panel width to default 28% on first use
    if (m_propertiesPanelWidth <= 0.0f)
        m_propertiesPanelWidth = totalWidth * 0.28f;

    // Clamp to a sensible range
    if (m_propertiesPanelWidth < 200.0f) m_propertiesPanelWidth = 200.0f;
    if (m_propertiesPanelWidth > totalWidth * 0.60f) m_propertiesPanelWidth = totalWidth * 0.60f;

    float handleWidth = 6.0f;
    float canvasWidth = totalWidth - m_propertiesPanelWidth - handleWidth;

    ImGui::BeginChild("VSCanvas", ImVec2(canvasWidth, 0), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderCanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    // UX Fix #3: Drag-to-resize handle between canvas and properties panel
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##vsresize", ImVec2(handleWidth, -1.0f));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_propertiesPanelWidth -= ImGui::GetIO().MouseDelta.x;
        if (m_propertiesPanelWidth < 200.0f)          m_propertiesPanelWidth = 200.0f;
        if (m_propertiesPanelWidth > totalWidth * 0.60f) m_propertiesPanelWidth = totalWidth * 0.60f;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Right panel container with 3 vertical sub-panels (A: Node Props | B: Preset Bank | C: Local Vars)
    ImGui::BeginChild("VSRightPanel", ImVec2(m_propertiesPanelWidth, 0), true);

    float rightPanelHeight = ImGui::GetContentRegionAvail().y;
    float splitterHeight = 4.0f;

    // Initialize sub-panel heights on first use (equal thirds for 3 panels)
    if (m_nodePropertiesPanelHeight <= 0.0f)
    {
        m_nodePropertiesPanelHeight = (rightPanelHeight - splitterHeight * 2) / 3.0f;
        m_presetBankPanelHeight = (rightPanelHeight - splitterHeight * 2) / 3.0f;
    }

    // Clamp heights to reasonable ranges
    float minPanelHeight = 50.0f;
    if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;
    if (m_presetBankPanelHeight < minPanelHeight) m_presetBankPanelHeight = minPanelHeight;

    float localVarHeight = rightPanelHeight - m_nodePropertiesPanelHeight - m_presetBankPanelHeight - splitterHeight * 2;
    if (localVarHeight < minPanelHeight) localVarHeight = minPanelHeight;

    // ---- Part A: Node Properties Panel ----
    ImGui::BeginChild("Part_A_NodeProps", ImVec2(0, m_nodePropertiesPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderNodePropertiesPanel();
    ImGui::EndChild();

    // ---- Splitter 1 (between Part A and Part B) ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##splitter1", ImVec2(-1.0f, splitterHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_nodePropertiesPanelHeight += ImGui::GetIO().MouseDelta.y;
        if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;
    }
    ImGui::PopStyleColor(3);

    // ---- Part B: Preset Bank Panel ----
    ImGui::BeginChild("Part_B_PresetBank", ImVec2(0, m_presetBankPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderPresetBankPanel();
    ImGui::EndChild();

    // ---- Splitter 2 (between Part B and Part C) ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##splitter2", ImVec2(-1.0f, splitterHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_presetBankPanelHeight += ImGui::GetIO().MouseDelta.y;
        if (m_presetBankPanelHeight < minPanelHeight) m_presetBankPanelHeight = minPanelHeight;
    }
    ImGui::PopStyleColor(3);

    // ---- Part C: Local/Global Variables Panel (with tab selection) ----
    ImGui::BeginChild("Part_C_Blackboard", ImVec2(0, localVarHeight), false,
                      ImGuiWindowFlags_NoScrollbar);

    // Tab selector for Local vs Global variables
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
    ImGui::RadioButton("Local Variables", &m_blackboardTabSelection, 0);
    ImGui::SameLine(150.0f);
    ImGui::RadioButton("Global Variables", &m_blackboardTabSelection, 1);
    ImGui::PopStyleVar();
    ImGui::Separator();

    // Render appropriate panel based on tab selection
    if (m_blackboardTabSelection == 0)
        RenderLocalVariablesPanel();
    else
        RenderGlobalVariablesPanel();

    ImGui::EndChild();

    ImGui::EndChild();  // End VSRightPanel
}


} // namespace Olympe
