/**
 * @file BehaviorTreeRenderer.h
 * @brief IGraphRenderer adapter for BehaviorTree graphs (wraps BTNodeGraphManager).
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details C++14 compliant.
 * Phase 2 (2026-04-08): Added BTNodePalette and BTNodePropertyPanel for modern UI.
 */

#pragma once

#include "IGraphRenderer.h"
#include "BehaviorTreeImNodesAdapter.h"
#include "BTNodePropertyPanel.h"
#include "../third_party/imgui/imgui.h"

// Phase 41 — Framework integration
#include "Framework/CanvasFramework.h"
#include "Framework/BehaviorTreeGraphDocument.h"

#include <string>
#include <memory>

namespace Olympe {

// Forward declarations
class NodeGraphPanel;
class GraphExecutionTracer;
class ExecutionTestPanel;
namespace AI {
    class BTNodePalette;
}

/**
 * @class BehaviorTreeRenderer
 * @brief Adapts the BTNodeGraphManager + NodeGraphPanel to IGraphRenderer.
 *
 * Phase 2 Enhancement: Now includes split-panel layout with:
 * - 75% NodeGraphPanel (canvas rendering)
 * - 25% Right panel with tabs:
 *   * Tab 0: BTNodePalette (node selection + drag-drop)
 *   * Tab 1: BTNodePropertyPanel (node property editing)
 *
 * Each instance manages a single BT graph ID in BTNodeGraphManager.
 * When Render() is called, this renderer sets that graph as the active one and
 * delegates to a shared NodeGraphPanel reference for drawing.
 */
class BehaviorTreeRenderer : public IGraphRenderer
{
public:
    /**
     * @brief Constructor for BehaviorTreeRenderer
     * @param panel Shared NodeGraphPanel reference for rendering canvas
     */
    explicit BehaviorTreeRenderer(NodeGraphPanel& panel);
    ~BehaviorTreeRenderer();

    void        Render()                         override;
    bool        Load(const std::string& path)    override;
    bool        Save(const std::string& path)    override;
    bool        IsDirty()              const     override;
    std::string GetGraphType()         const     override;
    std::string GetCurrentPath()       const     override;

    /**
     * @brief Create a new empty BehaviorTree graph and set it active
     * @param name Name for the new graph
     * @return true if successful
     */
    bool CreateNew(const std::string& name = "Untitled BehaviorTree");

    /**
     * @brief Run graph simulation via BehaviorTreeGraphAdapter + GraphExecutionSimulator
     * Converts BT to graph format, executes, formats trace, and displays in ExecutionTestPanel.
     * Called by "Run Graph" toolbar button.
     */
    void OnRunGraphClicked();

    /**
      * @brief Phase 44.2: Get the document adapter for framework integration.
      *  Returns the underlying IGraphDocument* for reuse in TabManager,
      *  avoiding duplicate document object creation.
      */
    IGraphDocument* GetDocument() const;

    /**
     * @brief Phase 47B: Set the file path for this renderer's graph.
     * Called after legacy load succeeds to synchronize filepath between legacy
     * loader and framework renderer, enabling Save to work with correct path.
     * @param path The file path to set
     */
    void SetFilePath(const std::string& path);

    /**
     * @brief Get the NodeGraphPanel reference for direct access.
     * Used by BehaviorTreeGraphDocument for position sync.
     * @return Reference to the shared NodeGraphPanel
     */
    NodeGraphPanel* GetNodeGraphPanel() { return &m_panel; }

private:
      NodeGraphPanel& m_panel;                       ///< Shared NodeGraphPanel for canvas rendering
      std::unique_ptr<AI::BTNodePalette> m_palette; ///< BTNodePalette for drag-drop
      BTNodePropertyPanel m_propertyPanel;          ///< Property editor for node properties
      int m_graphId;                                ///< ID in BTNodeGraphManager; -1 if not loaded
      std::string m_filePath;                       ///< Path that was loaded
      float m_canvasPanelWidth = 0.75f;            ///< Split ratio: 75% canvas, 25% right panel
      int m_rightPanelTabSelection = 0;             ///< 0 = Palette, 1 = Properties
      ImVec2 m_canvasScreenPos = ImVec2(0, 0);     ///< Screen position of canvas for drag-drop coordinate transformation
      std::unique_ptr<ExecutionTestPanel> m_executionTestPanel;  ///< REUSED: Simulation results panel
      std::unique_ptr<GraphExecutionTracer> m_lastTracer;        ///< Last simulation trace for results display

      // Phase 41 — Framework Integration
      /// Adapter document implementing IGraphDocument for BehaviorTree graphs
      std::unique_ptr<BehaviorTreeGraphDocument> m_document;

      /// Unified framework for toolbar and modal management (Phase 41)
      /// Handles Save/SaveAs/Browse buttons and centralized modals
      std::unique_ptr<CanvasFramework> m_framework;

      /// ImNodes rendering adapter for BehaviorTree (Phase 50.3)
      std::unique_ptr<BehaviorTreeImNodesAdapter> m_imNodesAdapter;

    // Layout rendering helpers
    void RenderLayoutWithTabs();
    void RenderRightPanelTabs();

    /**
     * @brief Handle drop of node type at screen position
     * @param nodeType BT node type name
     * @param screenX Absolute screen X coordinate
     * @param screenY Absolute screen Y coordinate
     */
    void AcceptNodeDrop(const std::string& nodeType, float screenX, float screenY);

    /**
     * @brief Handle keyboard shortcuts for copy/paste/duplicate operations
     * Ctrl+C: Copy selected node
     * Ctrl+V: Paste nodes from clipboard
     * Ctrl+D: Duplicate selected node
     */
    void HandleKeyboardShortcuts();

    // Phase 35.0: Canvas state management
    void SaveCanvasState() override;
    void RestoreCanvasState() override;
    std::string GetCanvasStateJSON() const override;
    void SetCanvasStateJSON(const std::string& json) override;

    // Phase 43: Framework modal rendering (centralized toolbar Save/SaveAs/Browse)
    void RenderFrameworkModals() override;

private:
    // Canvas state snapshot (Phase 35.0)
    struct CanvasState
    {
        ImVec2 canvasOffset;
        // Note: BehaviorTree via imnodes doesn't have explicit zoom
    } m_savedCanvasState;
};

} // namespace Olympe
