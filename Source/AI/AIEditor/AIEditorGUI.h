/**
 * @file AIEditorGUI.h
 * @brief Main GUI class for AI Editor (Phase 1.3)
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Provides complete AI Editor interface with:
 * - 3-panel layout (AssetBrowser, NodeGraph, Inspector)
 * - Integration with NodeGraphCore and AIGraphPlugin_BT
 * - AI-specific panels (Blackboard, Senses, Runtime Debug)
 * - Full CRUD workflow for Behavior Trees
 */

#pragma once

#include "../../NodeGraphCore/NodeGraphCore.h"
#include "../../NodeGraphCore/GraphDocument.h"
#include "../../NodeGraphCore/NodeGraphManager.h"
#include "../../NodeGraphCore/CommandSystem.h"
#include "../AIGraphPlugin_BT/BTNodeRegistry.h"
#include "../AIGraphPlugin_BT/BTNodePalette.h"
#include <string>
#include <memory>
#include <vector>

namespace Olympe {
namespace AI {

/**
 * @class AIEditorGUI
 * @brief Main AI Editor GUI class
 *
 * @details
 * Manages the complete AI Editor interface. Integrates NodeGraphCore
 * for graph management and AIGraphPlugin_BT for BT-specific functionality.
 */
class AIEditorGUI {
public:
    AIEditorGUI();
    ~AIEditorGUI();
    
    /**
     * @brief Initialize the editor
     * @return true if successful
     */
    bool Initialize();
    
    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();
    
    /**
     * @brief Render the complete UI
     */
    void Render();
    
    /**
     * @brief Update (called per frame)
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);
    
    /**
     * @brief Check if editor is active
     */
    bool IsActive() const { return m_isActive; }
    
    /**
     * @brief Set editor active state
     */
    void SetActive(bool active) { m_isActive = active; }
    
    /**
     * @brief Get command stack for undo/redo
     */
    NodeGraph::CommandStack& GetCommandStack() { return m_commandStack; }
    
private:
    // ========================================================================
    // Core Rendering
    // ========================================================================
    
    /**
     * @brief Render main menu bar
     */
    void RenderMenuBar();
    
    /**
     * @brief Render asset browser panel (left)
     */
    void RenderAssetBrowser();
    
    /**
     * @brief Render node graph panel (center)
     */
    void RenderNodeGraph();
    
    /**
     * @brief Render inspector panel (right)
     */
    void RenderInspector();
    
    // ========================================================================
    // Menu Actions
    // ========================================================================
    
    void MenuAction_NewBT();
    void MenuAction_NewHFSM();
    void MenuAction_Open();
    void MenuAction_Save();
    void MenuAction_SaveAs();
    void MenuAction_Close();
    void MenuAction_Undo();
    void MenuAction_Redo();
    void MenuAction_Cut();
    void MenuAction_Copy();
    void MenuAction_Paste();
    void MenuAction_Delete();
    void MenuAction_SelectAll();
    void MenuAction_ResetLayout();
    void MenuAction_ShowNodePalette();
    void MenuAction_ShowBlackboard();
    void MenuAction_ShowSensesPanel();
    void MenuAction_ShowRuntimeDebug();
    void MenuAction_About();
    
    // ========================================================================
    // Node Graph Rendering
    // ========================================================================
    
    /**
     * @brief Render node graph with ImNodes
     */
    void RenderNodeGraphCanvas();
    
    /**
     * @brief Render a single node
     * @param nodeId Node to render
     */
    void RenderNode(NodeGraph::NodeId nodeId);
    
    /**
     * @brief Render connections between nodes
     */
    void RenderConnections();
    
    /**
     * @brief Handle node creation from palette
     */
    void HandleNodeCreation();
    
    /**
     * @brief Handle node selection
     */
    void HandleNodeSelection();
    
    /**
     * @brief Handle link creation
     */
    void HandleLinkCreation();
    
    // ========================================================================
    // AI-Specific Panels
    // ========================================================================
    
    /**
     * @brief Render blackboard inspector panel
     */
    void RenderBlackboardPanel();
    
    /**
     * @brief Render AI senses debug panel
     */
    void RenderSensesPanel();
    
    /**
     * @brief Render runtime debug panel (entity list + execution)
     */
    void RenderRuntimeDebugPanel();
    
    // ========================================================================
    // Asset Browser
    // ========================================================================
    
    /**
     * @brief Scan directory for AI graphs
     */
    void ScanAIGraphDirectory(const std::string& directory);
    
    /**
     * @brief Render asset entry
     * @param filename Filename to render
     * @param fullPath Full path to file
     */
    void RenderAssetEntry(const std::string& filename, const std::string& fullPath);
    
    // ========================================================================
    // State
    // ========================================================================
    
    bool m_isActive;
    bool m_showNodePalette;
    bool m_showBlackboardPanel;
    bool m_showSensesPanel;
    bool m_showRuntimeDebugPanel;
    
    // Panel state
    char m_assetSearchFilter[256];
    std::vector<std::string> m_assetFiles;
    
    // Node palette
    std::unique_ptr<BTNodePalette> m_nodePalette;
    
    // Command system for undo/redo
    NodeGraph::CommandStack m_commandStack;
    
    // ImNodes context
    void* m_imnodesContext;
    
    // Current selection
    std::vector<int> m_selectedNodeIds;
    std::vector<int> m_selectedLinkIds;
};

} // namespace AI
} // namespace Olympe
