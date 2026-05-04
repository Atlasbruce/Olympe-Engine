#pragma once

#include "../Framework/GraphEditorBase.h"
#include "../Framework/CanvasToolbarRenderer.h"
#include "../Framework/CanvasFramework.h"
#include "PlaceholderGraphDocument.h"
#include "PlaceholderPropertyEditorPanel.h"
#include "PlaceholderNodePalette.h"  // Phase 64.1: Node palette for drag-drop creation
#include "../../PanelManager.h"  // Access to framework panel dimensions
#include <memory>
#include <string>

namespace Olympe {

class PlaceholderCanvas;

/**
 * @class PlaceholderGraphRenderer
 * @brief Renderer for Placeholder test graph type
 *
 * Demonstrates GraphEditorBase inheritance pattern.
 * Provides rendering for Blue/Green/Magenta nodes with connections.
 * Implements IGraphRenderer interface for TabManager integration.
 *
 * Architecture:
 * - PlaceholderGraphRenderer (this class) inherits GraphEditorBase
 * - Delegates rendering to PlaceholderCanvas
 * - Manages document and plugin tools
 *
 * C++14 compliant
 */
class PlaceholderGraphRenderer : public GraphEditorBase
{
public:
    PlaceholderGraphRenderer();
    virtual ~PlaceholderGraphRenderer();

    // ========== IGraphRenderer interface ==========

    virtual bool Load(const std::string& filePath) override;

    virtual bool Save(const std::string& filePath) override;

    virtual std::string GetGraphType() const override;
    
    /// Initialize canvas editor for this graph type
    virtual void InitializeCanvasEditor() override;
    
    /// Render type-specific graph content
    virtual void RenderGraphContent() override;

    /// Phase 53 FIX: Override common toolbar to integrate framework toolbar
    virtual void RenderCommonToolbar() override;

    /// Render type-specific toolbar items
    virtual void RenderTypeSpecificToolbar() override;
    
    /// Render type-specific panels (right side)
    virtual void RenderTypePanels() override;
    
    /// Handle type-specific keyboard shortcuts
    virtual void HandleTypeSpecificShortcuts() override;

    /// Phase 3: Rectangle selection with AABB hit detection
    virtual void SelectNodesInRectangle(const ImVec2& rectStart, const ImVec2& rectEnd) override;

    /// Phase 3: Delete selected nodes from document
    virtual void DeleteSelectedNodes() override;

    /// Phase 3: Move selected nodes by delta
    virtual void MoveSelectedNodes(float deltaX, float deltaY) override;

    /// Phase 4 Step 3: Batch property editing - apply property to all selected nodes
    virtual void UpdateSelectedNodesProperty(const std::string& propName, const std::string& propValue) override;

    /// Phase 4 Step 4: Render tabbed right panel (Components | Properties)
    void RenderRightPanelTabs();

    /// Phase 64: Sync canvas selection with base class selection
    void SyncCanvasSelectionWithBase();

    // ========== PlaceholderGraphRenderer specific ==========
    
    /// Get underlying document
    PlaceholderGraphDocument* GetDocument() { return GetDoc(); }

    /// Create new placeholder graph
    void CreateNewGraph();

    /// Phase 64: Check if node is selected in base class m_selectedNodeIds vector
    bool IsNodeSelected(int nodeId) const {
        for (int id : m_selectedNodeIds) {
            if (id == nodeId) return true;
        }
        return false;
    }

    /// Phase 64: Get the selected node IDs vector for debugging
    const std::vector<int>& GetSelectedNodeIds() const {
        return m_selectedNodeIds;
    }

    /// Phase 64: Update base class m_selectedNodeIds vector directly
    void SetSelectedNodeIds(const std::vector<int>& nodeIds) {
        m_selectedNodeIds = nodeIds;
    }

    /// Phase 64: Clear all selections
    void ClearSelectedNodes() {
        m_selectedNodeIds.clear();
    }

    /// Phase 64: Add single node to selection
    void AddSelectedNode(int nodeId) {
        // Avoid duplicates
        for (int id : m_selectedNodeIds) {
            if (id == nodeId) return;
        }
        m_selectedNodeIds.push_back(nodeId);
    }

    /// Phase 69: Execute save operation with given filepath
    bool ExecuteSave(const std::string& filePath);

    /// Phase 69: Execute save-as operation
    bool ExecuteSaveAs(const std::string& filePath);

    /// Phase 72: Render verification output panel (public for BlueprintEditorGUI access)
    void RenderVerificationLogsPanel();

    /// Phase 73: Render framework modals (Save/SaveAs/Browse dialogs)
    /// Overrides IGraphRenderer::RenderFrameworkModals() to delegate to toolbar
    virtual void RenderFrameworkModals() override;

private:
    // ====================================================================
    // Member Variables - Core (Using parent members with casts to avoid shadowing)
    // ====================================================================

    // Helper accessors for type safety
    PlaceholderGraphDocument* GetDoc() const { return static_cast<PlaceholderGraphDocument*>(m_document); }
    PlaceholderCanvas* GetCanvasPtr() const { return static_cast<PlaceholderCanvas*>(m_canvas); }

    // Owned objects (Unique to this subclass)
    std::unique_ptr<PlaceholderGraphDocument> m_ownedDocument; // Actual owner of document
    std::unique_ptr<PlaceholderCanvas> m_ownedCanvas;         // Actual owner of canvas

    std::unique_ptr<PlaceholderPropertyEditorPanel> m_propertyEditor;
    std::unique_ptr<CanvasToolbarRenderer> m_toolbar;  // Phase 4 Step 5 Feature #1: Unified toolbar (Save/SaveAs/Browse)
    std::unique_ptr<PlaceholderNodePalette> m_palette;  // Phase 64.1: Node palette for drag-drop creation

    // Phase 4 Step 5: Layout management - resizable panels
    // Right panel width uses PanelManager::InspectorPanelWidth (framework default: 300px)

    // Load tracking for Phase 51 pattern
    bool m_isLoading;

    // Phase 52+ MINIMAP SUPPORT: Minimap visibility and controls
    bool m_minimapVisible = true;
    float m_minimapSize = 0.15f;  // 0.05 - 0.5 ratio of canvas
    int m_minimapPosition = 1;     // 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight

    // Phase 72: Verification output panel (legacy pattern from VisualScriptEditorPanel)
    std::vector<std::string> m_verificationLogs;  // Formatted log entries for display
    bool m_verificationDone = false;              // Flag to show verification results
};

} // namespace Olympe
