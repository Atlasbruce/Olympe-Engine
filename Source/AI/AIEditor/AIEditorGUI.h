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
#include "../../BlueprintEditor/BTNodeGraphManager.h"
// TODO: CommandSystem removed - reimplement after rendering works
// #include "../../NodeGraphCore/CommandSystem.h"
// #include "../../NodeGraphCore/Commands/ToggleNodeBreakpointCommand.h"
// #include "../../NodeGraphCore/BlackboardSystem.h"
#include "../AIGraphPlugin_BT/BTNodeRegistry.h"
#include "../AIGraphPlugin_BT/BTNodePalette.h"
#include "BlackboardPanel.h"
#include <string>
#include <memory>
#include <vector>

namespace Olympe {
namespace AI {

// Type aliases for NodeGraphTypes for backward compatibility
using NodeId = Olympe::NodeGraphTypes::NodeId;
using GraphDocument = Olympe::NodeGraphTypes::GraphDocument;
using GraphId = Olympe::NodeGraphTypes::GraphId;
using NodeData = Olympe::NodeGraphTypes::NodeData;
using LinkData = Olympe::NodeGraphTypes::LinkData;
using PinId = Olympe::NodeGraphTypes::PinId;

// TODO: Reimplement command/annotation system
// using CommandStack = Olympe::NodeGraphTypes::CommandStack; // Doesn't exist in modern schema
// using ToggleNodeBreakpointCommand = Olympe::ToggleNodeBreakpointCommand;

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
    // Friend class to allow menu access
    friend class AIEditorMenus;
    
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

    // TODO: CommandStack system needs reimplementation
    // GetCommandStack() removed - CommandStack type doesn't exist in modern schema

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
    void MenuAction_AutoLayout();
    
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
    void RenderNode(NodeId nodeId);  // TODO: RenderNode needs reimplementation with modern schema

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
    // Helper Methods
    // ========================================================================
    
    /**
     * @brief Extract directory from filepath
     * @param filepath Full file path
     * @return Directory path without filename
     */
    static std::string ExtractDirectory(const std::string& filepath);
    
    /**
     * @brief Extract filename from filepath
     * @param filepath Full file path
     * @return Filename without directory
     */
    static std::string ExtractFilename(const std::string& filepath);
    
    /**
     * @brief Check if string ends with suffix
     * @param str String to check
     * @param suffix Suffix to look for
     * @return true if str ends with suffix
     */
    static bool EndsWith(const std::string& str, const std::string& suffix);
    
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
    
    // File dialog state
    std::string m_lastOpenPath;
    std::string m_lastSavePath;
    
    // Node palette
    std::unique_ptr<BTNodePalette> m_nodePalette;
    
    // Blackboard panel (Phase 2.1)
    BlackboardPanel m_blackboardPanel;

    // TODO: Command system for undo/redo - needs reimplementation
    // NodeGraph::CommandStack m_commandStack; // Removed - type doesn't exist

    // ImNodes context
    void* m_imnodesContext;
    
    // Current selection
    std::vector<int> m_selectedNodeIds;
    std::vector<int> m_selectedLinkIds;
};

} // namespace AI
} // namespace Olympe
