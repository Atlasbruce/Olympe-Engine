# UNIFIED FRAMEWORK - TECHNICAL SPECIFICATION
**For**: Development Team  
**Status**: 🛠️ IMPLEMENTATION GUIDE  
**C++ Standard**: C++14 compliant

---

## 1. GRAPHEDITORBASE - CORE CLASS SPECIFICATION

### 1.1 Class Declaration

```cpp
// === File: Source/BlueprintEditor/Framework/GraphEditorBase.h ===

#pragma once

#include "IGraphRenderer.h"
#include "../Utilities/ICanvasEditor.h"
#include "../Utilities/CanvasGridRenderer.h"
#include "../Utilities/CanvasMinimapRenderer.h"
#include <vector>
#include <memory>
#include <map>

struct ImVec2;

namespace Olympe {

// Forward declarations
class IGraphDocument;
class ICanvasEditor;
class GraphEditorBase;

/**
 * @class GraphEditorBase
 * @brief Abstract base class providing ~80% common functionality for all graph editors.
 *
 * This class implements the Template Method pattern:
 * - Render() is final and calls template methods
 * - Subclasses override specific template methods for custom behavior
 * - All common features (toolbar, grid, selection, menus) provided by base
 *
 * KEY DESIGN PRINCIPLES:
 * 1. Non-breaking: Implements IGraphRenderer interface unchanged
 * 2. Composable: Uses ICanvasEditor internally, delegates canvas specifics
 * 3. Extensible: Template methods for override points
 * 4. Consistent: All graph types use same toolbar, menus, shortcuts
 *
 * USAGE:
 * class MyGraphRenderer : public GraphEditorBase {
 *     void RenderGraphContent() override { /* render nodes/links */ }
 *     void RenderTypePanels() override { /* render side panels */ }
 * };
 */
class GraphEditorBase : public IGraphRenderer
{
public:
    virtual ~GraphEditorBase();

    // --- IGraphRenderer Implementation (from IGraphRenderer.h) ---
    
    /**
     * @brief Main render call - FINAL (implements template method pattern)
     * 
     * Render order (critical for ImGui):
     * 1. RenderBegin() - setup ImGui windows
     * 2. RenderToolbar() - common + type-specific toolbar
     * 3. RenderGraphContent() - type override: render nodes/links
     * 4. RenderTypePanels() - type override: side panels
     * 5. RenderContextMenu() - right-click menu
     * 6. RenderModals() - Save/SaveAs/Unsaved dialogs
     * 7. RenderEnd() - cleanup ImGui windows
     */
    virtual void Render() override final;

    /**
     * @brief Load from file (deferred to subclass)
     */
    virtual bool Load(const std::string& path) override = 0;

    /**
     * @brief Save to file (deferred to subclass)
     */
    virtual bool Save(const std::string& path) override = 0;

    /**
     * @brief Query dirty state from document
     */
    virtual bool IsDirty() const override final;

    /**
     * @brief Get graph type name (e.g., "VisualScript")
     */
    virtual std::string GetGraphType() const override = 0;

    /**
     * @brief Get current file path
     */
    virtual std::string GetCurrentPath() const override final;

    /**
     * @brief Save canvas state (pan, zoom, selection)
     * Called when switching tabs or closing document
     */
    virtual void SaveCanvasState() override;

    /**
     * @brief Restore canvas state from previous session
     * Called when reopening document after tab switch
     */
    virtual void RestoreCanvasState() override;

    /**
     * @brief Render framework modals (Save/SaveAs/Unsaved)
     * Called by TabManager/BlueprintEditorGUI during modal phase
     */
    virtual void RenderFrameworkModals() override;

    // --- Common Public Methods ---

    /**
     * @brief Set the document this renderer manages
     */
    void SetDocument(IGraphDocument* document);

    /**
     * @brief Get the document
     */
    IGraphDocument* GetDocument() const { return m_document; }

    /**
     * @brief Get the canvas editor
     */
    ICanvasEditor* GetCanvasEditor() const { return m_canvas; }

    /**
     * @brief Pan/Zoom utilities
     */
    ImVec2 GetCanvasOffset() const;
    void SetCanvasOffset(const ImVec2& offset);
    float GetCanvasZoom() const;
    void SetCanvasZoom(float zoom);
    void ResetPanZoom();

    /**
     * @brief Grid management
     */
    bool IsGridVisible() const { return m_gridVisible; }
    void SetGridVisible(bool visible) { m_gridVisible = visible; }
    void ToggleGrid() { m_gridVisible = !m_gridVisible; }

    /**
     * @brief Selection queries
     */
    std::vector<int> GetSelectedNodeIds() const { return m_selectedNodeIds; }
    bool HasSelection() const { return !m_selectedNodeIds.empty(); }
    void SelectAll();
    void DeselectAll();
    void ClearSelection() { DeselectAll(); }

    /**
     * @brief Mark document as modified
     */
    void MarkDirty();

    /**
     * @brief Trigger Save dialog from code
     */
    void ShowSaveDialog();

    /**
     * @brief Trigger SaveAs dialog from code
     */
    void ShowSaveAsDialog();

    // --- PROTECTED: Template Methods for Subclass Override ---

protected:
    /**
     * @brief Render the main graph content (nodes, links, etc.)
     * 
     * MANDATORY OVERRIDE: Subclass must implement
     * Responsibility: Draw all graph elements
     * 
     * EXAMPLE (EntityPrefab):
     *   void RenderGraphContent() override {
     *       m_prefabCanvas->Render();  // Delegate to canvas
     *   }
     * 
     * EXAMPLE (VisualScript):
     *   void RenderGraphContent() override {
     *       m_canvas->BeginRender();
     *       ImNodes::BeginNodeEditor(m_nodeEditorID);
     *       // Render nodes and links
     *       ImNodes::EndNodeEditor();
     *       m_canvas->EndRender();
     *   }
     */
    virtual void RenderGraphContent() = 0;

    /**
     * @brief Render type-specific toolbar buttons (optional)
     * 
     * OPTIONAL OVERRIDE: Default implementation is empty
     * Responsibility: Add custom buttons after common toolbar
     * Called after RenderCommonToolbar()
     * 
     * EXAMPLE (BehaviorTree):
     *   void RenderTypeSpecificToolbar() override {
     *       if (ImGui::Button("Verify Graph", ImVec2(100, 0))) {
     *           RunVerification();
     *       }
     *       ImGui::SameLine();
     *       if (ImGui::Button("Run", ImVec2(100, 0))) {
     *           ExecuteGraph();
     *       }
     *   }
     */
    virtual void RenderTypeSpecificToolbar() {}

    /**
     * @brief Render type-specific side panels (optional)
     * 
     * OPTIONAL OVERRIDE: Default implementation is empty
     * Responsibility: Render left/right/bottom panels for properties, variables, etc.
     * 
     * EXAMPLE (VisualScript):
     *   void RenderTypePanels() override {
     *       // Right panel with tabs
     *       if (ImGui::BeginChild("RightPanel", ImVec2(250, -1))) {
     *           if (ImGui::BeginTabBar("##tabs")) {
     *               if (ImGui::BeginTabItem("Variables")) {
     *                   RenderVariablesPanel();
     *                   ImGui::EndTabItem();
     *               }
     *               ImGui::EndTabBar();
     *           }
     *           ImGui::EndChild();
     *       }
     *   }
     */
    virtual void RenderTypePanels() {}

    /**
     * @brief Handle type-specific keyboard shortcuts (optional)
     * 
     * OPTIONAL OVERRIDE: Default implementation is empty
     * Common shortcuts handled by base class (Ctrl+S, Del, Ctrl+A)
     * Responsibility: Handle type-specific shortcuts
     * 
     * EXAMPLE (BehaviorTree):
     *   void HandleTypeSpecificShortcuts() override {
     *       if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
     *           FindNodeDialog();
     *       }
     *   }
     */
    virtual void HandleTypeSpecificShortcuts() {}

    /**
     * @brief Get type-specific context menu options (optional)
     * 
     * OPTIONAL OVERRIDE: Default returns empty vector
     * Responsibility: Return list of custom context menu items
     * Base class adds these after common items (Cut, Copy, Paste, Delete)
     * 
     * RETURNS: Vector of option names (strings)
     * 
     * EXAMPLE (BehaviorTree):
     *   std::vector<std::string> GetTypeContextMenuOptions() override {
     *       return {"Collapse Subtree", "Expand Subtree", "Convert to Sequence"};
     *   }
     */
    virtual std::vector<std::string> GetTypeContextMenuOptions() { return {}; }

    /**
     * @brief Handle context menu item selection (optional)
     * 
     * OPTIONAL OVERRIDE: Default implementation is empty
     * Called when user clicks a context menu item
     * 
     * EXAMPLE (BehaviorTree):
     *   void OnContextMenuSelected(const std::string& option) override {
     *       if (option == "Collapse Subtree") {
     *           CollapseSelectedSubtree();
     *       } else if (option == "Expand Subtree") {
     *           ExpandSelectedSubtree();
     *       }
     *   }
     */
    virtual void OnContextMenuSelected(const std::string& option) {}

    // --- PROTECTED: Common Utilities Available to Subclasses ---

    /**
     * @brief Render the common toolbar (grid, pan/zoom, minimap, save)
     * Called at beginning of Render()
     * Calls RenderTypeSpecificToolbar() at end for subclass additions
     */
    void RenderCommonToolbar();

    /**
     * @brief Apply pan/zoom transformation to ImGui
     * Sets up coordinate space for rendering
     */
    void ApplyPanZoomTransform();

    /**
     * @brief Handle pan/zoom input (mouse drag, scroll)
     */
    void HandlePanZoomInput();

    /**
     * @brief Update selection based on input
     * Handles: single click, Ctrl+click, rectangle drag
     */
    void UpdateSelection(ImVec2 mousePos, bool ctrlPressed, bool shiftPressed);

    /**
     * @brief Render selection rectangle while dragging
     */
    void RenderSelectionRectangle();

    /**
     * @brief Render right-click context menu
     * Includes common items + type-specific via GetTypeContextMenuOptions()
     */
    void RenderContextMenu();

    /**
     * @brief Handle common keyboard shortcuts (Ctrl+S, Del, Ctrl+A, Ctrl+D)
     */
    void HandleCommonShortcuts();

    /**
     * @brief Initialize canvas editor based on type
     * Creates ImNodesCanvasEditor or CustomCanvasEditor as needed
     * Called by Initialize()
     */
    virtual void InitializeCanvasEditor() = 0;

    // --- PROTECTED: Member Variables (accessible to subclasses) ---

    IGraphDocument* m_document;            // The data model
    ICanvasEditor* m_canvas;               // Canvas visualization backend

    // Pan/Zoom state
    ImVec2 m_canvasOffset;
    float m_canvasZoom;

    // Grid state
    bool m_gridVisible;
    CanvasGridRenderer::Style m_gridStyle;

    // Minimap state
    bool m_minimapVisible;
    float m_minimapSize;
    CanvasMinimapRenderer::MinimapPosition m_minimapPosition;

    // Selection state
    std::vector<int> m_selectedNodeIds;
    bool m_isDrawingSelectionRect;
    ImVec2 m_selectionRectStart;
    ImVec2 m_selectionRectEnd;

    // Context menu state
    bool m_showContextMenu;
    ImVec2 m_contextMenuPos;
    std::vector<std::string> m_contextMenuOptions;

    // Keyboard state
    bool m_ctrlPressed;
    bool m_shiftPressed;
    bool m_altPressed;

    // Modal state
    bool m_showSaveAsDialog;
    char m_saveAsBuffer[512];

    // Canvas state persistence
    json m_savedCanvasState;

private:
    // Render phases (private, called by Render())
    void RenderBegin();
    void RenderEnd();
    void RenderModals();

    // Helper for context menu
    void HandleContextMenuSelection();
};

} // namespace Olympe

```

### 1.2 Implementation Structure

```cpp
// === File: Source/BlueprintEditor/Framework/GraphEditorBase.cpp ===

#include "GraphEditorBase.h"
#include "IGraphDocument.h"
#include "../Utilities/ICanvasEditor.h"
#include "../third_party/imgui/imgui.h"
#include <algorithm>

namespace Olympe {

GraphEditorBase::GraphEditorBase()
    : m_document(nullptr)
    , m_canvas(nullptr)
    , m_canvasOffset(0, 0)
    , m_canvasZoom(1.0f)
    , m_gridVisible(true)
    , m_minimapVisible(true)
    , m_minimapSize(0.2f)
    , m_minimapPosition(CanvasMinimapRenderer::MinimapPosition::TopRight)
    , m_isDrawingSelectionRect(false)
    , m_showContextMenu(false)
    , m_ctrlPressed(false)
    , m_shiftPressed(false)
    , m_altPressed(false)
    , m_showSaveAsDialog(false)
{
    std::memset(m_saveAsBuffer, 0, sizeof(m_saveAsBuffer));
}

GraphEditorBase::~GraphEditorBase()
{
    // Note: m_document and m_canvas are NOT owned by this class
    // They are managed by specific subclasses
    if (m_canvas) {
        delete m_canvas;
        m_canvas = nullptr;
    }
}

// === FINAL RENDER METHOD - Template Method Pattern ===
void GraphEditorBase::Render()
{
    RenderBegin();

    // Update input state
    m_ctrlPressed = ImGui::GetIO().KeyCtrl;
    m_shiftPressed = ImGui::GetIO().KeyShift;
    m_altPressed = ImGui::GetIO().KeyAlt;

    // Render toolbar (common + type-specific)
    RenderCommonToolbar();

    // Handle input
    HandlePanZoomInput();
    HandleCommonShortcuts();
    HandleTypeSpecificShortcuts();

    // Update selection
    ImVec2 mousePos = ImGui::GetMousePos();
    UpdateSelection(mousePos, m_ctrlPressed, m_shiftPressed);

    // Apply transformations
    ApplyPanZoomTransform();

    // Render main graph content (subclass responsibility)
    RenderGraphContent();

    // Render type-specific panels
    RenderTypePanels();

    // Render selection rectangle
    RenderSelectionRectangle();

    // Render context menu
    RenderContextMenu();

    // Render modals (called LAST for proper ImGui frame timing)
    RenderModals();

    RenderEnd();
}

// === TOOLBAR RENDERING ===
void GraphEditorBase::RenderCommonToolbar()
{
    // Start toolbar
    if (ImGui::BeginMenuBar()) {
        // Grid toggle
        bool gridWas = m_gridVisible;
        ImGui::Checkbox("Grid##toolbar", &m_gridVisible);
        if (gridWas != m_gridVisible) {
            SYSTEM_LOG << "[GraphEditorBase] Grid toggled: " << (m_gridVisible ? "ON" : "OFF") << "\n";
        }

        ImGui::SameLine();

        // Pan/Zoom reset
        if (ImGui::Button("Reset View##btn", ImVec2(80, 0))) {
            ResetPanZoom();
            SYSTEM_LOG << "[GraphEditorBase] View reset\n";
        }

        ImGui::SameLine();

        // Minimap toggle
        ImGui::Checkbox("Minimap##toolbar", &m_minimapVisible);

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // Type-specific buttons (delegated to subclass)
        RenderTypeSpecificToolbar();

        ImGui::EndMenuBar();
    }
}

// === COMMON SHORTCUTS ===
void GraphEditorBase::HandleCommonShortcuts()
{
    // Ctrl+S: Save
    if (m_ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        if (m_document && !GetCurrentPath().empty()) {
            Save(GetCurrentPath());
            SYSTEM_LOG << "[GraphEditorBase] Saved (Ctrl+S)\n";
        } else {
            ShowSaveAsDialog();
        }
    }

    // Ctrl+Shift+S: Save As
    if (m_ctrlPressed && m_shiftPressed && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        ShowSaveAsDialog();
        SYSTEM_LOG << "[GraphEditorBase] SaveAs (Ctrl+Shift+S)\n";
    }

    // Ctrl+A: Select All
    if (m_ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_A, false)) {
        SelectAll();
        SYSTEM_LOG << "[GraphEditorBase] SelectAll (Ctrl+A)\n";
    }

    // Delete: Delete selected
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && HasSelection()) {
        SYSTEM_LOG << "[GraphEditorBase] Delete pressed (" << m_selectedNodeIds.size() << " nodes)\n";
        // Subclass handles actual deletion via context menu / override
    }
}

// === SELECTION MANAGEMENT ===
void GraphEditorBase::SelectAll()
{
    // Subclass responsibility to populate m_selectedNodeIds
    // This is a placeholder - override in subclass if needed
}

void GraphEditorBase::DeselectAll()
{
    m_selectedNodeIds.clear();
    m_isDrawingSelectionRect = false;
}

// === CONTEXT MENU ===
void GraphEditorBase::RenderContextMenu()
{
    if (!m_showContextMenu) return;

    ImGui::SetNextWindowPos(m_contextMenuPos);
    if (ImGui::BeginPopupContextWindow("##GraphContextMenu", ImGuiPopupFlags_NoOpenOverItems)) {
        // Common items
        if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            SYSTEM_LOG << "[GraphEditorBase] Cut\n";
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            SYSTEM_LOG << "[GraphEditorBase] Copy\n";
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            SYSTEM_LOG << "[GraphEditorBase] Paste\n";
        }
        ImGui::Separator();

        if (ImGui::MenuItem("Delete", "Del", false, HasSelection())) {
            SYSTEM_LOG << "[GraphEditorBase] Delete from menu\n";
        }

        ImGui::Separator();

        // Type-specific items
        for (const auto& option : m_contextMenuOptions) {
            if (ImGui::MenuItem(option.c_str())) {
                OnContextMenuSelected(option);
                m_showContextMenu = false;
            }
        }

        ImGui::EndPopup();
    } else {
        m_showContextMenu = false;
    }
}

// === MODALS ===
void GraphEditorBase::RenderModals()
{
    // Save As Dialog
    if (m_showSaveAsDialog) {
        if (ImGui::BeginPopupModal("Save As##modal", &m_showSaveAsDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Filename", m_saveAsBuffer, sizeof(m_saveAsBuffer));

            if (ImGui::Button("Save", ImVec2(120, 0))) {
                Save(m_saveAsBuffer);
                m_showSaveAsDialog = false;
                std::memset(m_saveAsBuffer, 0, sizeof(m_saveAsBuffer));
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                m_showSaveAsDialog = false;
                std::memset(m_saveAsBuffer, 0, sizeof(m_saveAsBuffer));
            }

            ImGui::EndPopup();
        }
    }

    // Render framework modals (called by TabManager)
    // This is for coordinating Save/SaveAs from both the framework and the tab manager
}

// === STATE MANAGEMENT ===
void GraphEditorBase::MarkDirty()
{
    if (m_document) {
        m_document->OnDocumentModified();
    }
}

void GraphEditorBase::SaveCanvasState()
{
    if (m_canvas) {
        m_savedCanvasState = {
            {"canvasOffset", {m_canvasOffset.x, m_canvasOffset.y}},
            {"canvasZoom", m_canvasZoom},
            {"gridVisible", m_gridVisible},
            {"minimapVisible", m_minimapVisible},
            {"selectedNodes", m_selectedNodeIds}
        };
    }
}

void GraphEditorBase::RestoreCanvasState()
{
    // Restore saved state (if exists)
    if (!m_savedCanvasState.is_null()) {
        try {
            if (m_savedCanvasState.contains("canvasOffset")) {
                auto& offset = m_savedCanvasState["canvasOffset"];
                m_canvasOffset = ImVec2(offset[0], offset[1]);
            }
            if (m_savedCanvasState.contains("canvasZoom")) {
                m_canvasZoom = m_savedCanvasState["canvasZoom"];
            }
            if (m_savedCanvasState.contains("gridVisible")) {
                m_gridVisible = m_savedCanvasState["gridVisible"];
            }
            if (m_savedCanvasState.contains("minimapVisible")) {
                m_minimapVisible = m_savedCanvasState["minimapVisible"];
            }
        } catch (...) {
            SYSTEM_LOG << "[GraphEditorBase] Failed to restore canvas state\n";
        }
    }
}

// === UTILITY METHODS ===
bool GraphEditorBase::IsDirty() const
{
    return m_document ? m_document->IsDirty() : false;
}

std::string GraphEditorBase::GetCurrentPath() const
{
    return m_document ? m_document->GetFilePath() : "";
}

ImVec2 GraphEditorBase::GetCanvasOffset() const { return m_canvasOffset; }
void GraphEditorBase::SetCanvasOffset(const ImVec2& offset) { m_canvasOffset = offset; }

float GraphEditorBase::GetCanvasZoom() const { return m_canvasZoom; }
void GraphEditorBase::SetCanvasZoom(float zoom) { m_canvasZoom = std::max(0.1f, std::min(3.0f, zoom)); }

void GraphEditorBase::ResetPanZoom()
{
    m_canvasOffset = ImVec2(0, 0);
    m_canvasZoom = 1.0f;
}

void GraphEditorBase::ShowSaveDialog()
{
    m_showSaveAsDialog = false;  // SaveAs uses the modal
    // TabManager will handle this
}

void GraphEditorBase::ShowSaveAsDialog()
{
    m_showSaveAsDialog = true;
}

} // namespace Olympe
```

---

## 2. PLUGIN INTERFACES

### 2.1 IEditorTool (Base)

```cpp
// === File: Source/BlueprintEditor/Framework/IEditorTool.h ===

#pragma once

#include <string>

namespace Olympe {

class IGraphDocument;
class GraphEditorBase;

/**
 * @class IEditorTool
 * @brief Base interface for all editor tools/plugins
 * 
 * A tool is any component that extends graph editor functionality:
 * - Verification/validation (IGraphVerifier)
 * - Output/tracing (IOutputPanel)
 * - Node creation (INodePaletteProvider)
 * - Custom analysis (IGraphAnalyzer)
 */
class IEditorTool
{
public:
    virtual ~IEditorTool() = default;

    /**
     * @brief Unique name for this tool instance
     */
    virtual std::string GetToolName() const = 0;

    /**
     * @brief Initialize tool when document opens
     * Called once per document load
     */
    virtual void Initialize(IGraphDocument* document, GraphEditorBase* editor) = 0;

    /**
     * @brief Shutdown tool when document closes
     * Called when tab closes or document unloads
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Called each frame to update tool state
     * Optional - default implementation does nothing
     */
    virtual void Update() {}

    /**
     * @brief Render tool UI (if applicable)
     * Optional - default implementation does nothing
     */
    virtual void Render() {}
};

} // namespace Olympe
```

### 2.2 IGraphVerifier

```cpp
// === File: Source/BlueprintEditor/Framework/IGraphVerifier.h ===

#pragma once

#include "IEditorTool.h"
#include <vector>
#include <string>

namespace Olympe {

/**
 * @class IGraphVerifier
 * @brief Interface for graph validation/verification tools
 * 
 * Verifiers check graph correctness and report errors/warnings
 * Examples:
 * - BehaviorTreeVerifier: Check for cycles, disconnected nodes
 * - VisualScriptVerifier: Check type mismatches, unresolved variables
 * - EntityPrefabVerifier: Check component compatibility
 */
class IGraphVerifier : public IEditorTool
{
public:
    enum class LogLevel {
        Error,
        Warning,
        Info
    };

    struct VerificationResult {
        bool isValid;
        std::vector<std::pair<std::string, LogLevel>> messages;  // error/warning/info messages
        std::vector<int> highlightedNodeIds;                     // nodes to highlight
    };

    /**
     * @brief Run verification on the document
     * @return VerificationResult with validity and messages
     */
    virtual VerificationResult Verify() = 0;

    /**
     * @brief Highlight specific error node in canvas
     * Called when user clicks on error in output panel
     */
    virtual void HighlightError(int nodeId) = 0;

    /**
     * @brief Clear all highlighting
     */
    virtual void ClearHighlighting() = 0;

    /**
     * @brief Query if verification is currently running
     * Used by UI to disable buttons during long-running verification
     */
    virtual bool IsVerifying() const { return false; }
};

} // namespace Olympe
```

### 2.3 IOutputPanel

```cpp
// === File: Source/BlueprintEditor/Framework/IOutputPanel.h ===

#pragma once

#include "IEditorTool.h"
#include <string>
#include <vector>

namespace Olympe {

enum class OutputLogLevel {
    Trace,
    Info,
    Warning,
    Error
};

/**
 * @class IOutputPanel
 * @brief Interface for output/logging panels
 * 
 * Output panels display logs, traces, debug info, etc.
 * Examples:
 * - ExecutionTracePanel: Show BT execution trace
 * - ErrorOutputPanel: Show verification errors
 * - DebuggerPanel: Show variable inspection
 */
class IOutputPanel : public IEditorTool
{
public:
    /**
     * @brief Add a log message to the output
     */
    virtual void AddLog(const std::string& message, OutputLogLevel level = OutputLogLevel::Info) = 0;

    /**
     * @brief Clear all logs
     */
    virtual void Clear() = 0;

    /**
     * @brief Check if panel is currently visible
     */
    virtual bool IsVisible() const = 0;

    /**
     * @brief Show/hide the panel
     */
    virtual void SetVisible(bool visible) = 0;

    /**
     * @brief Toggle visibility
     */
    void ToggleVisibility() { SetVisible(!IsVisible()); }

    /**
     * @brief Scroll to bottom of logs
     */
    virtual void ScrollToBottom() {}

    /**
     * @brief Filter logs by level (optional)
     */
    virtual void SetLogLevelFilter(OutputLogLevel minLevel) {}
};

} // namespace Olympe
```

### 2.4 INodePaletteProvider

```cpp
// === File: Source/BlueprintEditor/Framework/INodePaletteProvider.h ===

#pragma once

#include "IEditorTool.h"
#include <vector>
#include <string>

struct ImVec2;

namespace Olympe {

/**
 * @class INodePaletteProvider
 * @brief Interface for node/component creation palettes
 * 
 * Palettes provide UI for creating new nodes in the graph
 * Examples:
 * - VisualScriptNodePalette: Create task nodes
 * - BehaviorTreeNodePalette: Create BT nodes
 * - ComponentPalettePanel: Create entity components
 */
class INodePaletteProvider : public IEditorTool
{
public:
    struct NodeType {
        std::string name;
        std::string category;
        std::string description;
        std::string iconPath;  // optional
    };

    /**
     * @brief Get list of available node types
     */
    virtual std::vector<NodeType> GetNodeTypes() const = 0;

    /**
     * @brief Render the palette UI
     * Usually a panel with searchable list of node types
     */
    virtual void Render() override = 0;

    /**
     * @brief Accept drag-drop of node type at position
     * @param nodeType The node type to create
     * @param screenPos Screen position where to create
     * @return true if node was created successfully
     */
    virtual bool AcceptDragDrop(const std::string& nodeType, ImVec2 screenPos) = 0;

    /**
     * @brief Check if a specific node type is available
     */
    virtual bool IsNodeTypeAvailable(const std::string& nodeType) const = 0;

    /**
     * @brief Get category list (for filtering)
     */
    virtual std::vector<std::string> GetCategories() const = 0;
};

} // namespace Olympe
```

---

## 3. IMPLEMENTATION EXAMPLE: VisualScriptEditorPanel (Refactored)

### 3.1 Header

```cpp
// === File: Source/BlueprintEditor/VisualScriptEditorPanel.h (REFACTORED) ===

#pragma once

#include "Framework/GraphEditorBase.h"
#include <memory>

namespace Olympe {

class VisualScriptGraphDocument;
class ImNodesCanvasEditor;

/**
 * @class VisualScriptEditorPanel
 * @brief Renderer for VisualScript graphs (REFACTORED to use GraphEditorBase)
 * 
 * REFACTORING SUMMARY:
 * - Before: ~800 lines (with toolbar, grid, selection, context menu duplication)
 * - After: ~350 lines (just type-specific code)
 * - Inherited from GraphEditorBase: ~400 lines of common functionality
 * 
 * Responsibilities (now ONLY type-specific):
 * - RenderGraphContent(): imnodes setup
 * - RenderTypePanels(): Local/Global variables, Presets
 * - RenderTypeSpecificToolbar(): Verify, Run buttons (if any)
 * - Load/Save implementation (delegates to document)
 */
class VisualScriptEditorPanel : public GraphEditorBase
{
public:
    VisualScriptEditorPanel();
    ~VisualScriptEditorPanel();

    // --- IGraphRenderer Implementation ---
    bool Load(const std::string& path) override;
    bool Save(const std::string& path) override;
    std::string GetGraphType() const override { return "VisualScript"; }

    // --- Initialization ---
    void Initialize();

protected:
    // --- GraphEditorBase Template Methods (Override) ---

    /**
     * @brief Render imnodes node editor
     * (Inherited toolbar, grid, selection from base)
     */
    void RenderGraphContent() override;

    /**
     * @brief Render Local/Global variables and Preset panels
     */
    void RenderTypePanels() override;

    /**
     * @brief Handle VisualScript-specific shortcuts (if any)
     */
    void HandleTypeSpecificShortcuts() override;

    /**
     * @brief VisualScript doesn't have type-specific toolbar buttons
     */
    void RenderTypeSpecificToolbar() override {} // Empty

    /**
     * @brief Initialize ImNodesCanvasEditor
     */
    void InitializeCanvasEditor() override;

private:
    std::unique_ptr<ImNodesCanvasEditor> m_nodesCanvas;
    
    // VisualScript-specific UI state
    bool m_showLocalVariablesPanel;
    bool m_showGlobalVariablesPanel;
    bool m_showPresetsPanel;
    
    // Panel rendering helpers
    void RenderLocalVariablesPanel();
    void RenderGlobalVariablesPanel();
    void RenderPresetsPanel();
};

} // namespace Olympe
```

### 3.2 Implementation (Excerpt)

```cpp
// === File: Source/BlueprintEditor/VisualScriptEditorPanel.cpp (REFACTORED EXCERPT) ===

#include "VisualScriptEditorPanel.h"
#include "Framework/VisualScriptGraphDocument.h"
#include "Utilities/ImNodesCanvasEditor.h"
#include "../third_party/imgui/imgui.h"

namespace Olympe {

VisualScriptEditorPanel::VisualScriptEditorPanel()
    : m_nodesCanvas(nullptr)
    , m_showLocalVariablesPanel(true)
    , m_showGlobalVariablesPanel(false)
    , m_showPresetsPanel(true)
{
}

void VisualScriptEditorPanel::Initialize()
{
    // Initialize canvas
    InitializeCanvasEditor();

    // Set grid style for VisualScript
    if (m_canvas) {
        m_canvas->SetGridStyle(CanvasGridRenderer::Style_VisualScript);
    }

    // Initialize UI state
    m_gridVisible = true;
    m_minimapVisible = true;
}

void VisualScriptEditorPanel::InitializeCanvasEditor()
{
    // Create ImNodes canvas editor
    m_nodesCanvas = std::make_unique<ImNodesCanvasEditor>(
        "VS_NodeEditor",
        ImVec2(0, 0),
        ImVec2(800, 600),
        nullptr  // EditorContext (optional)
    );
    m_canvas = m_nodesCanvas.get();
}

// === RENDER: Called every frame from base class ===
void VisualScriptEditorPanel::RenderGraphContent()
{
    // Base class handles: toolbar, grid, selection, context menus
    // We just need to render imnodes content

    if (!m_canvas) return;

    m_canvas->BeginRender();
    ImNodes::BeginNodeEditor(123);  // Node editor ID

    // Get document
    auto vsDoc = dynamic_cast<VisualScriptGraphDocument*>(m_document);
    if (!vsDoc) {
        ImNodes::EndNodeEditor();
        m_canvas->EndRender();
        return;
    }

    // Render all nodes (simplified - actual would iterate document's nodes)
    for (const auto& node : vsDoc->GetAllNodes()) {
        // Render node...
    }

    // Render all links
    for (const auto& link : vsDoc->GetAllLinks()) {
        // Render link...
    }

    ImNodes::EndNodeEditor();
    m_canvas->EndRender();
}

// === TYPE-SPECIFIC PANELS ===
void VisualScriptEditorPanel::RenderTypePanels()
{
    // Render right-side panels (variables, presets)
    // In reality, this would be tabbed layout - simplified here

    if (ImGui::BeginChild("RightPanel", ImVec2(250, -1))) {
        if (m_showLocalVariablesPanel) {
            RenderLocalVariablesPanel();
        }
        if (m_showGlobalVariablesPanel) {
            RenderGlobalVariablesPanel();
        }
        if (m_showPresetsPanel) {
            RenderPresetsPanel();
        }
        ImGui::EndChild();
    }
}

void VisualScriptEditorPanel::RenderLocalVariablesPanel()
{
    // Render local variables from document
    // (Existing logic extracted to here)
    if (ImGui::CollapsingHeader("Local Variables", ImGuiTreeNodeFlags_DefaultOpen)) {
        // ... existing variable rendering code
    }
}

void VisualScriptEditorPanel::RenderGlobalVariablesPanel()
{
    // Render global variables
    if (ImGui::CollapsingHeader("Global Variables", ImGuiTreeNodeFlags_DefaultOpen)) {
        // ... existing variable rendering code
    }
}

void VisualScriptEditorPanel::RenderPresetsPanel()
{
    // Render condition presets
    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        // ... existing preset rendering code
    }
}

// === FILE OPERATIONS ===
bool VisualScriptEditorPanel::Load(const std::string& path)
{
    auto vsDoc = dynamic_cast<VisualScriptGraphDocument*>(m_document);
    if (!vsDoc) {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Invalid document type\n";
        return false;
    }

    if (!vsDoc->Load(path)) {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Failed to load " << path << "\n";
        return false;
    }

    // Restore canvas state if available
    RestoreCanvasState();

    SYSTEM_LOG << "[VisualScriptEditorPanel] Loaded " << path << "\n";
    return true;
}

bool VisualScriptEditorPanel::Save(const std::string& path)
{
    auto vsDoc = dynamic_cast<VisualScriptGraphDocument*>(m_document);
    if (!vsDoc) {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Invalid document type\n";
        return false;
    }

    if (!vsDoc->Save(path)) {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Failed to save " << path << "\n";
        return false;
    }

    // Save canvas state before unloading
    SaveCanvasState();

    SYSTEM_LOG << "[VisualScriptEditorPanel] Saved " << path << "\n";
    return true;
}

} // namespace Olympe
```

---

## 4. IMPLEMENTATION PHASES (DETAILED CODING GUIDE)

### Phase 1: Foundation (Week 1)

**Deliverables**:
1. GraphEditorBase.h/cpp (400 lines total)
2. IEditorTool.h and derivatives (400 lines total)
3. IEditorToolManager.h/cpp (250 lines total)
4. Unit tests (100 lines)

**Verification**:
```cpp
// Test: Verify base class compiles with empty subclass
class TestGraphEditor : public GraphEditorBase {
    void RenderGraphContent() override {}
    void InitializeCanvasEditor() override {}
    bool Load(const std::string&) override { return true; }
    bool Save(const std::string&) override { return true; }
    std::string GetGraphType() const override { return "Test"; }
};

// Test: Verify plugin registration works
IEditorToolManager& mgr = IEditorToolManager::Get();
auto verifier = std::make_shared<TestVerifier>();
mgr.RegisterTool(verifier);
auto result = mgr.RunVerification(nullptr);
```

**Build Verification**:
```bash
# After Phase 1, should compile with:
# - 0 errors
# - 0 warnings (in our code)
# - All existing tests still pass
```

### Phase 2-4: Incremental Refactoring

[Detailed refactoring for each renderer type...]

### Phase 5: Animation Implementation

[New graph type leveraging framework...]

---

## 5. MIGRATION CHECKLIST

- [ ] Phase 1: GraphEditorBase + plugins created, compiles
- [ ] Phase 1: Unit tests pass for base class
- [ ] Phase 2: VisualScript refactored, 100% tests pass
- [ ] Phase 2: Code reduction verified (should be ~50%)
- [ ] Phase 3: BehaviorTree refactored, tests pass
- [ ] Phase 4: EntityPrefab refactored, tests pass
- [ ] Phase 5: Animation implemented, tests pass
- [ ] Integration: All 4 types load/save correctly
- [ ] Performance: No regression vs. original
- [ ] Documentation: Architecture guide updated

---

**Document Status**: ✅ COMPLETE TECHNICAL SPEC  
**For**: Development Team Implementation  
**Reference**: TABMANAGER_CANVASMANAGER_FRAMEWORK_AUDIT.md  
**Next**: Phase 1 code review + approval

