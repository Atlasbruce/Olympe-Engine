#include "GraphEditorBase.h"
#include "CanvasFramework.h"
#include "../../third_party/imgui/imgui.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace Olympe {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

GraphEditorBase::GraphEditorBase(const std::string& name)
    : m_editorName(name)
    , m_document(nullptr)
    , m_canvas(nullptr)
    , m_canvasOffset(0, 0)
    , m_canvasZoom(1.0f)
    , m_gridVisible(true)
    , m_minimapVisible(true)
    , m_minimapSize(0.2f)
    , m_isDrawingSelectionRect(false)
    , m_showContextMenu(false)
    , m_ctrlPressed(false)
    , m_shiftPressed(false)
    , m_altPressed(false)
    , m_showSaveAsDialog(false)
{
    std::memset(m_saveAsBuffer, 0, sizeof(m_saveAsBuffer));
    std::cout << "[GraphEditorBase] Created: " << m_editorName << "\n";
}

GraphEditorBase::~GraphEditorBase()
{
    if (m_canvas) {
        delete m_canvas;
        m_canvas = nullptr;
    }
    std::cout << "[GraphEditorBase] Destroyed: " << m_editorName << "\n";
}

// ============================================================================
// IGRAPHRENDERER IMPLEMENTATION
// ============================================================================

void GraphEditorBase::SetDocument(void* document)
{
    m_document = document;
    std::cout << "[GraphEditorBase] Document set\n";
}

bool GraphEditorBase::IsDirty() const
{
    // To be overridden or enhanced by subclass
    return false;
}

std::string GraphEditorBase::GetCurrentPath() const
{
    // To be overridden or enhanced by subclass
    return "";
}

void GraphEditorBase::SaveCanvasState()
{
    // Base implementation: save pan/zoom/grid
    std::cout << "[GraphEditorBase] Canvas state saved\n";
}

void GraphEditorBase::RestoreCanvasState()
{
    // Base implementation: restore pan/zoom/grid
    std::cout << "[GraphEditorBase] Canvas state restored\n";
}

void GraphEditorBase::RenderFrameworkModals()
{
    RenderModals();
}

// ============================================================================
// PAN / ZOOM
// ============================================================================

ImVec2 GraphEditorBase::GetCanvasOffset() const
{
    return m_canvasOffset;
}

void GraphEditorBase::SetCanvasOffset(const ImVec2& offset)
{
    m_canvasOffset = offset;
}

float GraphEditorBase::GetCanvasZoom() const
{
    return m_canvasZoom;
}

void GraphEditorBase::SetCanvasZoom(float zoom)
{
    m_canvasZoom = std::max(0.1f, std::min(3.0f, zoom));
}

void GraphEditorBase::ResetPanZoom()
{
    m_canvasOffset = ImVec2(0, 0);
    m_canvasZoom = 1.0f;
    std::cout << "[GraphEditorBase] Pan/Zoom reset\n";
}

// ============================================================================
// SELECTION
// ============================================================================

void GraphEditorBase::SelectAll()
{
    std::cout << "[GraphEditorBase] SelectAll (override in subclass)\n";
}

void GraphEditorBase::DeselectAll()
{
    m_selectedNodeIds.clear();
    m_isDrawingSelectionRect = false;
}

void GraphEditorBase::SelectMultipleNodes(int nodeId, bool ctrlPressed, bool shiftPressed)
{
    // Phase 3: Multi-selection logic with Ctrl+Click support

    if (ctrlPressed) {
        // Ctrl+Click: Toggle node selection
        auto it = std::find(m_selectedNodeIds.begin(), m_selectedNodeIds.end(), nodeId);
        if (it != m_selectedNodeIds.end()) {
            m_selectedNodeIds.erase(it);  // Deselect
        } else {
            m_selectedNodeIds.push_back(nodeId);  // Select
        }
    } else if (shiftPressed) {
        // Shift+Click: Range selection (simple: just add to selection)
        auto it = std::find(m_selectedNodeIds.begin(), m_selectedNodeIds.end(), nodeId);
        if (it == m_selectedNodeIds.end()) {
            m_selectedNodeIds.push_back(nodeId);
        }
    } else {
        // Regular click: Single selection
        if (m_selectedNodeIds.size() == 1 && m_selectedNodeIds[0] == nodeId) {
            // Already selected, keep selection
        } else {
            m_selectedNodeIds.clear();
            m_selectedNodeIds.push_back(nodeId);
        }
    }
}

bool GraphEditorBase::IsNodeSelected(int nodeId) const
{
    return std::find(m_selectedNodeIds.begin(), m_selectedNodeIds.end(), nodeId) != m_selectedNodeIds.end();
}

void GraphEditorBase::DeleteSelectedNodes()
{
    // Phase 3: Base implementation - logs selection count
    // Subclasses should override to actually delete nodes from document
    if (!m_selectedNodeIds.empty()) {
        std::cout << "[GraphEditorBase] DeleteSelectedNodes: " << m_selectedNodeIds.size() << " node(s)\n";
    }
}

void GraphEditorBase::SelectNodesInRectangle(const ImVec2& rectStart, const ImVec2& rectEnd)
{
    // Phase 3: Base implementation for rectangle selection
    // Subclasses override to perform hit detection on actual nodes
    // This method is called when rectangle drag ends
    (void)rectStart;
    (void)rectEnd;
    // Default: no action (subclass responsible for actual selection)
}

void GraphEditorBase::MoveSelectedNodes(float deltaX, float deltaY)
{
    // Phase 3: Batch move operation - base implementation
    // Subclasses override to actually move nodes in their document
    if (!m_selectedNodeIds.empty()) {
        std::cout << "[GraphEditorBase] MoveSelectedNodes: delta=(" << deltaX << ", " << deltaY << ")\n";
    }
}

void GraphEditorBase::UpdateSelectedNodesProperty(const std::string& propName, const std::string& propValue)
{
    // Phase 3: Batch property update - base implementation
    // Subclasses override to apply property updates to selected nodes
    if (!m_selectedNodeIds.empty()) {
        std::cout << "[GraphEditorBase] UpdateSelectedNodesProperty: " << propName << "=" << propValue << "\n";
    }
}

void GraphEditorBase::MarkDirty()
{
    std::cout << "[GraphEditorBase] Document marked dirty\n";
}

// ============================================================================
// DIALOGS
// ============================================================================

void GraphEditorBase::ShowSaveDialog()
{
    std::cout << "[GraphEditorBase] Show save dialog\n";
}

void GraphEditorBase::ShowSaveAsDialog()
{
    m_showSaveAsDialog = true;
    std::cout << "[GraphEditorBase] Show save-as dialog\n";
}

// ============================================================================
// MAIN RENDER - TEMPLATE METHOD PATTERN
// ============================================================================

void GraphEditorBase::Render()
{
    RenderBegin();

    // Update input state
    m_ctrlPressed = ImGui::GetIO().KeyCtrl;
    m_shiftPressed = ImGui::GetIO().KeyShift;
    m_altPressed = ImGui::GetIO().KeyAlt;

    // Phase 61: Render toolbar with proper spacing
    {
        ImGui::BeginGroup();
        RenderCommonToolbar();
        ImGui::EndGroup();
        ImGui::Separator();  // Visual separation
        ImGui::Spacing();
    }

    // Handle input
    HandlePanZoomInput();
    HandleCommonShortcuts();
    HandleTypeSpecificShortcuts();

    // Update selection
    ImVec2 mousePos = ImGui::GetMousePos();
    UpdateSelection(mousePos, m_ctrlPressed, m_shiftPressed);

    // Render main graph content (subclass responsibility)
    RenderGraphContent();

    // Render type-specific panels
    RenderTypePanels();

    // Render selection rectangle
    RenderSelectionRectangle();

    // Render context menu
    RenderContextMenu();

    RenderEnd();
}

// ============================================================================
// TOOLBAR RENDERING
// ============================================================================

void GraphEditorBase::RenderCommonToolbar()
{
    // Phase 61 FIX: Replace BeginMenuBar (doesn't work in BeginChild) with custom toolbar
    // We execute inside CenterColumn BeginChild, so BeginMenuBar() is NOT available
    // Solution: Render toolbar elements directly with separators

    // Grid toggle
    ImGui::Checkbox("Grid##toolbar", &m_gridVisible);
    ImGui::SameLine(0.0f, 10.0f);

    // Pan/Zoom reset
    if (ImGui::Button("Reset View##btn", ImVec2(80, 0))) {
        ResetPanZoom();
    }
    ImGui::SameLine(0.0f, 10.0f);

    // Minimap toggle
    ImGui::Checkbox("Minimap##toolbar", &m_minimapVisible);
    ImGui::SameLine(0.0f, 10.0f);

    // Separator
    ImGui::Separator();
    ImGui::SameLine(0.0f, 10.0f);

    // Type-specific buttons (from subclass)
    RenderTypeSpecificToolbar();
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

void GraphEditorBase::HandlePanZoomInput()
{
    ImGuiIO& io = ImGui::GetIO();

    // Mouse scroll zoom
    if (io.MouseWheel != 0.0f) {
        float oldZoom = m_canvasZoom;
        m_canvasZoom += io.MouseWheel * 0.1f;
        m_canvasZoom = std::max(0.1f, std::min(3.0f, m_canvasZoom));

        // Zoom toward mouse
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();

        float zoomRatio = m_canvasZoom / oldZoom;
        ImVec2 mouseRelative = ImVec2(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
        m_canvasOffset.x += mouseRelative.x * (1.0f - zoomRatio);
        m_canvasOffset.y += mouseRelative.y * (1.0f - zoomRatio);
    }

    // Middle mouse drag pan
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
        m_canvasOffset.x += delta.x;
        m_canvasOffset.y += delta.y;
    }
}

void GraphEditorBase::UpdateSelection(ImVec2 mousePos, bool ctrlPressed, bool shiftPressed)
{
    // Placeholder: subclass should override
    (void)mousePos;
    (void)ctrlPressed;
    (void)shiftPressed;
}

// ============================================================================
// RENDERING
// ============================================================================

void GraphEditorBase::RenderSelectionRectangle()
{
    if (!m_isDrawingSelectionRect) return;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // Phase 3: Enhanced visual feedback for rectangle selection
    ImU32 fillColor = IM_COL32(100, 150, 255, 50);      // Semi-transparent blue
    ImU32 borderColor = IM_COL32(100, 150, 255, 255);   // Opaque blue

    drawList->AddRectFilled(m_selectionRectStart, m_selectionRectEnd, fillColor);
    drawList->AddRect(m_selectionRectStart, m_selectionRectEnd, borderColor, 0.0f, 0, 2.0f);
}

void GraphEditorBase::RenderContextMenu()
{
    if (!m_showContextMenu) return;

    ImGui::SetNextWindowPos(m_contextMenuPos);
    if (ImGui::BeginPopupContextVoid("##GraphContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            std::cout << "[GraphEditorBase] Cut\n";
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            std::cout << "[GraphEditorBase] Copy\n";
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            std::cout << "[GraphEditorBase] Paste\n";
        }
        ImGui::Separator();

        if (ImGui::MenuItem("Delete", "Del", false, HasSelection())) {
            DeleteSelectedNodes();
        }

        ImGui::Separator();

        // Type-specific items
        m_contextMenuOptions = GetTypeContextMenuOptions();
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

// ============================================================================
// KEYBOARD SHORTCUTS
// ============================================================================

void GraphEditorBase::HandleCommonShortcuts()
{
    // Ctrl+S: Save
    if (m_ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        if (!GetCurrentPath().empty()) {
            Save(GetCurrentPath());
            std::cout << "[GraphEditorBase] Saved (Ctrl+S)\n";
        } else {
            ShowSaveAsDialog();
        }
    }

    // Ctrl+Shift+S: Save As
    if (m_ctrlPressed && m_shiftPressed && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        ShowSaveAsDialog();
        std::cout << "[GraphEditorBase] SaveAs (Ctrl+Shift+S)\n";
    }

    // Ctrl+A: Select All
    if (m_ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_A, false)) {
        SelectAll();
        std::cout << "[GraphEditorBase] SelectAll (Ctrl+A)\n";
    }

    // Delete: Delete selected
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && HasSelection()) {
        DeleteSelectedNodes();
    }
}

// ============================================================================
// PRIVATE: RENDER PHASES
// ============================================================================

void GraphEditorBase::RenderBegin()
{
    // Phase 60 FIX: REMOVED ImGui::Begin - was creating floating window
    // We execute inside CenterColumn BeginChild from BlueprintEditorGUI
    // Must NOT add additional Begin/End (breaks hierarchy)
}

void GraphEditorBase::RenderEnd()
{
    // Phase 60 FIX: REMOVED ImGui::End - was closing floating window
    // Preserve CenterColumn context by doing nothing here
}

void GraphEditorBase::RenderModals()
{
    // Save As Dialog
    if (m_showSaveAsDialog) {
        ImGui::SetNextWindowSize(ImVec2(500, 150), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Save As##dialog", &m_showSaveAsDialog, ImGuiWindowFlags_Modal)) {
            ImGui::InputText("Filename##saveAs", m_saveAsBuffer, sizeof(m_saveAsBuffer));

            ImGui::SameLine();
            if (ImGui::Button("Save##btn", ImVec2(80, 0))) {
                if (m_saveAsBuffer[0] != '\0') {
                    std::string filepath = m_saveAsBuffer;
                    if (Save(filepath)) {
                        m_showSaveAsDialog = false;
                        std::memset(m_saveAsBuffer, 0, sizeof(m_saveAsBuffer));
                        std::cout << "[GraphEditorBase] Saved to: " << filepath << "\n";
                    }
                }
            }

            ImGui::End();
        }
    }
}

void GraphEditorBase::HandleContextMenuSelection()
{
    // Called when context menu item selected
}

} // namespace Olympe
