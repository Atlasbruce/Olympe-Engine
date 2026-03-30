/**
 * @file VisualScriptEditorPanel_Core.cpp
 * @brief Core lifecycle methods for VisualScriptEditorPanel (Phase 24).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details This file contains the lifecycle methods extracted from VisualScriptEditorPanel.cpp:
 *          - Constructor initialization
 *          - Destructor cleanup
 *          - Initialize() with ImNodes context, UI helpers, and preset loading
 *          - Shutdown() with resource cleanup and registry management
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
// Constructor
// ============================================================================
/**
 * @brief VisualScriptEditorPanel Constructor
 * 
 * Initializes the save-as filename buffer with a default "untitled_graph" name.
 * This is called when a new editor panel tab is created for editing a blueprint graph.
 * 
 * @note The ImNodes editor context and UI helper panels are NOT created here.
 *       They are initialized in Initialize() to ensure proper resource management
 *       and context isolation per panel instance.
 */
VisualScriptEditorPanel::VisualScriptEditorPanel()
{
    std::memset(m_saveAsFilename, 0, sizeof(m_saveAsFilename));
    strcpy_s(m_saveAsFilename, sizeof(m_saveAsFilename), "untitled_graph");
}

// ============================================================================
// Destructor
// ============================================================================
/**
 * @brief VisualScriptEditorPanel Destructor
 * 
 * Cleanup is deferred to Shutdown() to ensure proper sequencing of resource
 * deallocation in the context of a tabbed editor interface.
 * 
 * @note Do NOT manually delete m_imnodesContext or UI helper panels here.
 *       Always call Shutdown() first to free resources in correct order.
 * @see Shutdown()
 */
VisualScriptEditorPanel::~VisualScriptEditorPanel()
{
}

// ============================================================================
// Initialize
// ============================================================================
/**
 * @brief Initialize the editor panel with ImNodes context and UI helpers
 * 
 * Called when a blueprint is loaded into the editor (e.g., when a tab is opened).
 * Sets up:
 * - A dedicated ImNodes editor context for independent canvas state per panel
 * - Dynamic data pin manager for condition/operand-driven pin generation
 * - Node branch renderer for visual flow display
 * - Node conditions panel for condition editing UI
 * - Math operation panel for arithmetic node properties
 * - Blackboard access panels (Get/Set operations)
 * - Variable property panel for variable node editing
 * - Condition preset library UI
 * - Entity blackboard for local/global variable management (entity ID 0 = editor context)
 * 
 * Phase 24 Enhancement (Condition Preset Embedding):
 * - Presets are now stored IN the blueprint JSON (v4 schema) instead of external files
 * - Each graph is self-contained; no need for separate preset files
 * - Presets are loaded from m_template.Presets during Initialize()
 * - If graph has presets, they populate m_presetRegistry; otherwise registry remains empty
 * - Logging reports: count of loaded presets per graph
 * 
 * Callback Setup:
 * - OnDynamicPinsNeedRegeneration: Triggers when user confirms condition edits in modal
 * - Fetches FRESH condition data from m_conditionsPanel (not stale eNode data)
 * - Regenerates dynamic pins with updated operand information
 * - Syncs m_template for serialization
 * 
 * @see Shutdown()
 * @see ConditionPresetRegistry::LoadFromPresetList()
 * @see DynamicDataPinManager::RegeneratePinsFromConditions()
 */
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

// ============================================================================
// Shutdown
// ============================================================================
/**
 * @brief Shutdown the editor panel and release all resources
 * 
 * Called when a blueprint editor tab is closed or the application exits.
 * Performs cleanup in the correct order:
 * - Frees the ImNodes editor context (if allocated)
 * - Clears editor node list (m_editorNodes)
 * - Clears editor link list (m_editorLinks)
 * - Clears positioned node cache (m_positionedNodes)
 * - Releases all UI helper panels in Phase 24 order:
 *   * NodeConditionsPanel (used by OnDynamicPinsNeedRegeneration callback)
 *   * MathOpPropertyPanel (may reference m_presetRegistry)
 *   * GetBBValuePropertyPanel
 *   * SetBBValuePropertyPanel
 *   * VariablePropertyPanel
 *   * ConditionPresetLibraryPanel (may reference m_presetRegistry)
 *   * NodeBranchRenderer (may reference m_pinManager)
 *   * DynamicDataPinManager (lowest-level utility)
 * - Resets condition panel node ID to sentinel value (-1)
 * 
 * Phase 24 Notes:
 * - DO NOT manually clear m_presetRegistry here; it is cleared by the destructor
 * - UI helper reset() calls ensure no dangling callbacks or resource leaks
 * - After Shutdown(), the panel can be re-initialized for a new graph
 * 
 * @note Always call Shutdown() before destroying the panel instance.
 * @see Initialize()
 * @see ~VisualScriptEditorPanel()
 */
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

}  // namespace Olympe
