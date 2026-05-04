#pragma once

#include "../IGraphRenderer.h"
#include "../../third_party/imgui/imgui.h"
#include <string>
#include <vector>
#include <memory>

namespace Olympe {

/**
 * @class GraphEditorBase
 * @brief Abstract base class providing ~80% common functionality for all graph editors.
 *
 * Template Method Pattern:
 * - Render() is final and orchestrates the render pipeline
 * - Subclasses override specific template methods for custom behavior
 * - Common features: toolbar, grid, selection, context menu, shortcuts
 *
 * C++14 compliant, no complex dependencies in header
 */
class GraphEditorBase : public IGraphRenderer
{
public:
    explicit GraphEditorBase(const std::string& name = "GraphEditor");
    virtual ~GraphEditorBase();

    // --- IGraphRenderer Implementation ---

    /// Main render - FINAL (template method)
    virtual void Render() override final;

    /// Load from file - deferred to subclass
    virtual bool Load(const std::string& path) override = 0;

    /// Save to file - deferred to subclass
    virtual bool Save(const std::string& path) override = 0;

    /// Query dirty state
    virtual bool IsDirty() const override final;

    /// Get graph type name ("VisualScript", "Placeholder", etc.)
    virtual std::string GetGraphType() const override = 0;

    /// Get current file path
    virtual std::string GetCurrentPath() const override final;

    /// Save canvas pan/zoom/selection state
    virtual void SaveCanvasState() override;

    /// Restore previous canvas state
    virtual void RestoreCanvasState() override;

    /// Render framework modals (Save/SaveAs)
    virtual void RenderFrameworkModals() override;

    // --- Public API ---

    /** Set the document being edited */
    void SetDocument(void* document);  // void* to avoid circular dependencies

    /** Get document pointer */
    void* GetDocument() const { return m_document; }

    /** Pan/Zoom utilities */
    ImVec2 GetCanvasOffset() const;
    void SetCanvasOffset(const ImVec2& offset);
    float GetCanvasZoom() const;
    void SetCanvasZoom(float zoom);
    void ResetPanZoom();

    /** Grid management */
    bool IsGridVisible() const { return m_gridVisible; }
    void SetGridVisible(bool visible) { m_gridVisible = visible; }
    void ToggleGrid() { m_gridVisible = !m_gridVisible; }

    /** Selection queries */
    std::vector<int> GetSelectedNodeIds() const { return m_selectedNodeIds; }
    int GetFirstSelectedNodeId() const { return m_selectedNodeIds.empty() ? -1 : m_selectedNodeIds[0]; }
    int GetSelectedNodeCount() const { return static_cast<int>(m_selectedNodeIds.size()); }
    bool HasSelection() const { return !m_selectedNodeIds.empty(); }
    void SelectAll();
    void DeselectAll();

    /** Multi-selection with Ctrl+Click support */
    void SelectMultipleNodes(int nodeId, bool ctrlPressed = false, bool shiftPressed = false);

    /** Check if node is selected */
    bool IsNodeSelected(int nodeId) const;

    /** Rectangle selection from drag - subclass should populate m_selectionRectStart/End */
    virtual void SelectNodesInRectangle(const ImVec2& rectStart, const ImVec2& rectEnd);

    /** Delete all selected nodes - delegates to subclass */
    virtual void DeleteSelectedNodes();

    /** Batch operations - subclass overrides for actual implementation */
    virtual void MoveSelectedNodes(float deltaX, float deltaY);
    virtual void UpdateSelectedNodesProperty(const std::string& propName, const std::string& propValue);

    /** Mark document as modified */
    void MarkDirty();

    /** Show dialogs programmatically */
    void ShowSaveDialog();
    void ShowSaveAsDialog();

    // --- Protected: Template Methods for Subclass Override ---

protected:

    /// MANDATORY OVERRIDE: Render the main graph content (nodes, links, etc.)
    virtual void RenderGraphContent() = 0;

    /// OPTIONAL OVERRIDE: Render type-specific toolbar buttons
    virtual void RenderTypeSpecificToolbar() {}

    /// OPTIONAL OVERRIDE: Render type-specific side panels
    virtual void RenderTypePanels() {}

    /// OPTIONAL OVERRIDE: Handle type-specific keyboard shortcuts
    virtual void HandleTypeSpecificShortcuts() {}

    /// OPTIONAL OVERRIDE: Get type-specific context menu options
    virtual std::vector<std::string> GetTypeContextMenuOptions() { return {}; }

    /// OPTIONAL OVERRIDE: Handle context menu selection
    virtual void OnContextMenuSelected(const std::string& option) { (void)option; }

    /// MANDATORY OVERRIDE: Initialize canvas editor for this type
    virtual void InitializeCanvasEditor() = 0;

    // --- Protected: Common Utilities ---

    virtual void RenderCommonToolbar();  // Phase 53: Made virtual for subclass override
    void HandlePanZoomInput();
    void UpdateSelection(ImVec2 mousePos, bool ctrlPressed, bool shiftPressed);
    void RenderSelectionRectangle();
    void RenderContextMenu();
    void HandleCommonShortcuts();

    // --- Protected: Member Variables ---

    std::string m_editorName;
    void* m_document;           // Opaque pointer - resolved by subclass
    void* m_canvas;             // Opaque pointer - resolved by subclass

    // Phase 53: Framework integration for unified toolbar (Save, SaveAs, Browse)
    std::unique_ptr<class CanvasFramework> m_framework;

    // Pan/Zoom
    ImVec2 m_canvasOffset;
    float m_canvasZoom;

    // Grid
    bool m_gridVisible;

    // Minimap
    bool m_minimapVisible;
    float m_minimapSize;

    // Selection
    std::vector<int> m_selectedNodeIds;
    bool m_isDrawingSelectionRect;
    ImVec2 m_selectionRectStart;
    ImVec2 m_selectionRectEnd;

    // Context menu
    bool m_showContextMenu;
    ImVec2 m_contextMenuPos;
    std::vector<std::string> m_contextMenuOptions;

    // Keyboard
    bool m_ctrlPressed;
    bool m_shiftPressed;
    bool m_altPressed;

    // Modals
    bool m_showSaveAsDialog;
    char m_saveAsBuffer[512];

    // Canvas state (as string to avoid JSON dependency)
    std::string m_savedCanvasState;

private:
    void RenderBegin();
    void RenderEnd();
    virtual void RenderModals();
    void HandleContextMenuSelection();
};

} // namespace Olympe
