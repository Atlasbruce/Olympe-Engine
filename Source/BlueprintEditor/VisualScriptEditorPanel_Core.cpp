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
}

// ============================================================================
// Main Render Dispatcher
// ============================================================================

void VisualScriptEditorPanel::Render()
{
    if (!m_visible)
        return;

    // Main panel window
    if (ImGui::Begin("Visual Script Editor", &m_visible))
    {
        RenderContent();
        ImGui::End();
    }
}

void VisualScriptEditorPanel::RenderContent()
{
    // Save viewport pan before any ImGui operations (BUG-003)
    // This is done to preserve canvas panning across frames

    // Render main editor layout:
    // ┌─────────────────────────────────────────────────────────────┐
    // │                      Toolbar                                 │
    // ├──────────────────────┬──────────────────────────────────────┤
    // │   Blueprint Files    │                                       │
    // │   (left panel)       │      Canvas Area                      │
    // │                      │      (center - ImNodes)               │
    // │ Verification Logs    │                                       │
    // │                      ├──────────────────────────────────────┤
    // │ (Phase 24.3)         │ Node Properties (right panel, Part A)  │
    // │                      │ Preset Bank (right panel, Part B)     │
    // │                      │ Local Variables (right panel, Part C)  │
    // └──────────────────────┴──────────────────────────────────────┘

    RenderToolbar();

    // Main horizontal split: left panel (files/logs) + canvas + right panel (properties)
    ImGui::SetNextItemWidth(-1.0f);

    // Render main canvas
    RenderCanvas();

    // Render properties panel on the right
    RenderProperties();

    // Render blackboard panel
    RenderBlackboard();

    // Render validation overlay
    RenderValidationOverlay();

    // Render breakpoints
    RenderBreakpoints();

    // Render SaveAs dialog if open
    RenderSaveAsDialog();

    // Render context menus (must be after EndNodeEditor)
    RenderContextMenus();
}

} // namespace Olympe
