/**
 * @file EntityPrefabEditorV2.cpp
 * @brief Framework-compliant renderer adapter for Entity Prefab graphs (PHASE E: GraphEditorBase inheritance)
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * PHASE E REFACTORING: Changed inheritance from IGraphRenderer to GraphEditorBase.
 * Now follows template method pattern like PlaceholderGraphRenderer:
 * - InitializeCanvasEditor() - setup canvas/editor
 * - RenderGraphContent() - render nodes/connections
 * - RenderTypePanels() - render right panel tabs
 * - SelectNodesInRectangle() - delegate to canvas
 * - DeleteSelectedNodes() - delete from document
 * - MoveSelectedNodes() - move in document
 *
 * Benefits:
 * - Selection management: Inherited from GraphEditorBase (m_selectedNodeIds)
 * - Pan/Zoom: Inherited utilities (m_canvasOffset, m_canvasZoom, ResetPanZoom)
 * - Context menus: Inherited (RenderContextMenu)
 * - Keyboard shortcuts: Inherited (HandleCommonShortcuts)
 * - Framework toolbar: Inherited (m_framework, RenderCommonToolbar)
 *
 * Fixes:
 * - Bug 1 (coordinate offset): Base class handles coordinate sync
 * - Bug 2 (multi-select drag reset): Base class preserves m_selectedNodeIds
 * - Bug 3 (port selection): Base class HandlePanZoomInput()
 * - Bug 4 (hover feedback): Base class UpdateSelection()
 * - Bug 5 (context menus): Base class RenderContextMenu()
 */

#include "EntityPrefabEditorV2.h"
#include "../../system/system_utils.h"
#include "../../third_party/imgui/imgui.h"
#include "../Framework/CanvasFramework.h"       // PHASE E: Include for m_framework
#include "../Framework/CanvasToolbarRenderer.h" // PHASE 75 FIX: for GetToolbar()->Render()
#include "../../PanelManager.h"                 // Access to framework panel dimensions

namespace Olympe
{
    // ========================================================================
    // Lifecycle
    // ========================================================================

    EntityPrefabEditorV2::EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document)
        : GraphEditorBase("EntityPrefabEditorV2")  // PHASE E: Call base class constructor
    {
        m_document = document; // Set base class member (void*)
        m_canvas = nullptr;    // Set base class member (void*)

        if (!document)
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Constructor received nullptr document" << std::endl;
            return;
        }

        SYSTEM_LOG << "[EntityPrefabEditorV2::Constructor] Initializing Entity Prefab renderer" << std::endl;

        // ====================================================================
        // Phase E: Initialize UI Panels
        // ====================================================================

        // Initialize Component Palette Panel
        m_componentPalette.Initialize();
        SYSTEM_LOG << "[EntityPrefabEditorV2::Constructor] Component palette initialized" << std::endl;

        // Initialize Property Editor Panel
        m_propertyEditor.Initialize();
        SYSTEM_LOG << "[EntityPrefabEditorV2::Constructor] Property editor initialized" << std::endl;

        // NOTE: Canvas and framework are initialized lazily in InitializeCanvasEditor()
        // (called by base class Render() on first frame)

        SYSTEM_LOG << "[EntityPrefabEditorV2::Constructor] Panels initialized - canvas will be initialized on first render" << std::endl;

        LogAction("Constructor completed");
    }

    EntityPrefabEditorV2::~EntityPrefabEditorV2()
    {
        // PHASE 72 FIX: Avoid calling LogAction in destructor to prevent access violation 
        // if m_document is already destroyed or invalid during application shutdown.

        // Clean up canvas (casted to correct type for proper destructor call)
        PrefabCanvas* canvas = GetCanvasPtr();
        if (canvas)
        {
            delete canvas;
            m_canvas = nullptr; // Clear base class member
        }

        // Canvas editor is std::unique_ptr - auto cleanup
        // Component palette and property editor are std::objects - auto cleanup
        // Framework is now in base class (GraphEditorBase) - auto cleanup

        // Document is not owned - do not delete
        m_document = nullptr; // Clear base class member
    }

    // ========================================================================
    // GraphEditorBase Template Method Overrides (PHASE E)
    // ========================================================================

    void EntityPrefabEditorV2::InitializeCanvasEditor()
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2::InitializeCanvasEditor] Creating canvas and editor" << std::endl;

        EntityPrefabGraphDocumentV2* doc = GetDoc();
        if (!doc)
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: InitializeCanvasEditor() called with no document" << std::endl;
            return;
        }

        // ====================================================================
        // Create Canvas UI Container
        // ====================================================================

        if (!m_canvas)
        {
            PrefabCanvas* newCanvas = new PrefabCanvas();
            newCanvas->InitializeV2(doc);
            newCanvas->SetRenderer(this);  // PHASE E: Link canvas to renderer for selection sync
            m_canvas = (void*)newCanvas;    // Set base class member
            SYSTEM_LOG << "[EntityPrefabEditorV2] Canvas created and initialized" << std::endl;
        }

        PrefabCanvas* canvas = GetCanvasPtr();

        // ====================================================================
        // Create Canvas Adapter (zoom/pan)
        // ====================================================================

        if (!m_canvasEditor)
        {
            float initialZoom = doc->GetCanvasZoom();
            Vector canvasOffset = doc->GetCanvasOffset();

            m_canvasEditor = std::make_unique<CustomCanvasEditor>(
                "##EntityPrefabCanvas",
                ImVec2(0, 0),  // Will be updated on first render
                ImVec2(500, 400),  // Will be updated on first render
                initialZoom,
                0.1f, 3.0f
            );

            m_canvasEditor->SetPan(ImVec2(canvasOffset.x, canvasOffset.y));
            if (canvas) canvas->SetCanvasEditor(m_canvasEditor.get());

            SYSTEM_LOG << "[EntityPrefabEditorV2] Canvas editor created (zoom: " << initialZoom << ")" << std::endl;
        }

        // ====================================================================
        // Create Framework Integration (inherited from base class)
        // ====================================================================

        if (!m_framework)
        {
            m_framework = std::make_unique<CanvasFramework>(doc);
            SYSTEM_LOG << "[EntityPrefabEditorV2] Framework created (toolbar/modals)" << std::endl;
        }
    }

    void EntityPrefabEditorV2::RenderCommonToolbar()
    {
        // PHASE 75 FIX: Render framework toolbar (Save, SaveAs) first
        if (m_framework && m_framework->GetToolbar())
        {
            m_framework->GetToolbar()->Render();
            ImGui::SameLine(0.0f, 20.0f);
            ImGui::Separator();
            ImGui::SameLine(0.0f, 20.0f);
        }

        // Call base class common items (Grid, Reset, Minimap)
        GraphEditorBase::RenderCommonToolbar();
    }

    void EntityPrefabEditorV2::RenderGraphContent()
    {
        // PHASE 75.1: Unified layout following Placeholder pattern with framework-to-canvas sync
        if (!m_canvas)
        {
            InitializeCanvasEditor();
        }

        PrefabCanvas* canvas = GetCanvasPtr();
        EntityPrefabGraphDocumentV2* doc = GetDoc();

        if (!canvas || !doc)
        {
            ImGui::TextDisabled("Canvas or Document not initialized");
            return;
        }

        // ====================================================================
        // SYNC FRAMEWORK STATE -> CANVAS
        // ====================================================================
        if (m_canvasEditor)
        {
            // Sync Minimap toggle from toolbar
            m_canvasEditor->SetMinimapVisible(m_minimapVisible);

            // Sync Reset View from toolbar (if base class vars were reset)
            if (m_canvasOffset.x == 0 && m_canvasOffset.y == 0 && m_canvasZoom == 1.0f)
            {
               if (m_canvasEditor->GetPan().x != 0 || m_canvasEditor->GetPan().y != 0 || m_canvasEditor->GetZoom() != 1.0f)
               {
                   m_canvasEditor->SetPan(ImVec2(0, 0));
                   m_canvasEditor->SetZoom(1.0f);
               }
            }
        }

        // Sync Grid visibility from toolbar (Phase 75.1 FIX: Use SetGridVisible)
        canvas->SetGridVisible(m_gridVisible);

        // ====================================================================
        // SELECTION SYNC: Document -> Property Editor
        // ====================================================================
        const auto& selectedNodes = doc->GetSelectedNodes();
        if (!selectedNodes.empty())
        {
            m_propertyEditor.SetSelectedNode(selectedNodes[0]);

            // Sync base class selection vector for framework features (e.g. status bar)
            m_selectedNodeIds.clear();
            for (auto id : selectedNodes) m_selectedNodeIds.push_back((int)id);
        }
        else
        {
            m_propertyEditor.ClearSelection();
            m_selectedNodeIds.clear();
        }

        // ====================================================================
        // LAYOUT CALCULATION: THREE-COLUMN (Canvas | Handle | Right Panel)
        // ====================================================================
        float totalWidth = ImGui::GetContentRegionAvail().x;
        float totalHeight = ImGui::GetContentRegionAvail().y;
        float handleWidth = 4.0f;

        float rightPanelWidth = static_cast<float>(PanelManager::InspectorPanelWidth);
        if (rightPanelWidth < 200.0f) rightPanelWidth = 200.0f;
        if (rightPanelWidth > totalWidth * 0.60f) rightPanelWidth = totalWidth * 0.60f;

        float canvasWidth = totalWidth - rightPanelWidth - handleWidth;

        // ====================================================================
        // LEFT PANEL: Canvas Area
        // ====================================================================
        ImGui::BeginChild("EntityPrefabCanvas", ImVec2(canvasWidth, 0), false, ImGuiWindowFlags_NoScrollbar);

        // Get canvas dimensions
        ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();

        // Update canvas editor context
        if (m_canvasEditor)
        {
            ImVec2 currentSize = m_canvasEditor->GetCanvasSize();
            if (currentSize.x != canvasSize.x || currentSize.y != canvasSize.y)
            {
                float oldZoom = m_canvasEditor->GetZoom();
                ImVec2 oldPan = m_canvasEditor->GetPan();
                m_canvasEditor = std::make_unique<CustomCanvasEditor>("##EntityPrefabCanvas", canvasScreenPos, canvasSize, oldZoom, 0.1f, 3.0f);
                m_canvasEditor->SetPan(oldPan);
                m_canvasEditor->SetMinimapVisible(m_minimapVisible);
                canvas->SetCanvasEditor(m_canvasEditor.get());
            }
        }

        // Render Canvas content
        if (m_canvasEditor)
        {
            m_canvasEditor->BeginRender();

            // ================================================================
            // MINIMAP DATA SYNC: Update nodes and viewport for the minimap
            // ================================================================
            if (m_canvasEditor->IsMinimapVisible())
            {
                const auto& nodes = doc->GetAllNodes();
                if (!nodes.empty())
                {
                    std::vector<std::tuple<int, float, float, float, float>> minimapNodes;
                    float minX = 1e10f, maxX = -1e10f, minY = 1e10f, maxY = -1e10f;

                    for (const auto& node : nodes)
                    {
                        minimapNodes.emplace_back((int)node.nodeId, node.position.x, node.position.y, node.size.x, node.size.y);

                        // Expand bounds
                        float nLeft = node.position.x - node.size.x * 0.5f;
                        float nRight = node.position.x + node.size.x * 0.5f;
                        float nTop = node.position.y - node.size.y * 0.5f;
                        float nBottom = node.position.y + node.size.y * 0.5f;

                        if (nLeft < minX) minX = nLeft;
                        if (nRight > maxX) maxX = nRight;
                        if (nTop < minY) minY = nTop;
                        if (nBottom > maxY) maxY = nBottom;
                    }

                    // Add some padding to graph bounds
                    minX -= 100.0f; maxX += 100.0f;
                    minY -= 100.0f; maxY += 100.0f;

                    // Update minimap nodes
                    m_canvasEditor->UpdateMinimapNodes(minimapNodes, minX, maxX, minY, maxY);

                    // Update minimap viewport
                    ImVec2 viewMin, viewMax;
                    m_canvasEditor->GetCanvasVisibleBounds(viewMin, viewMax);
                    m_canvasEditor->UpdateMinimapViewport(viewMin.x, viewMax.x, viewMin.y, viewMax.y, minX, maxX, minY, maxY);
                }
            }

            // PASS HOVER INFO TO RENDERER (Phase 75.1)
            ComponentNodeRenderer* renderer = canvas->GetRenderer();
            if (renderer)
            {
                renderer->SetHoveredNode(canvas->GetHoveredNodeId());
                renderer->SetHoveredConnection(canvas->GetHoveredConnectionIndex());
            }

            canvas->Render();
            m_canvasEditor->RenderMinimap();
            m_canvasEditor->EndRender();

            // SYNC BACK: Canvas -> Framework state (pan/zoom)
            m_canvasOffset = m_canvasEditor->GetPan();
            m_canvasZoom = m_canvasEditor->GetZoom();
        }

        ImGui::EndChild();

        // Handle Drag-Drop from palette overlay
        ImVec2 canvasRegionMin = ImGui::GetItemRectMin();
        ImVec2 canvasRegionMax = ImGui::GetItemRectMax();
        ImGui::SetCursorScreenPos(canvasRegionMin);
        ImGui::PushClipRect(canvasRegionMin, canvasRegionMax, false);
        ImGui::Dummy(ImVec2(canvasWidth, canvasRegionMax.y - canvasRegionMin.y));
        if (ImGui::BeginDragDropTarget())
        {
             if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT_TYPE"))
             {
                 const char* componentName = (const char*)payload->Data;
                 if (m_canvasEditor)
                 {
                    ImVec2 canvasPos = m_canvasEditor->ScreenToCanvas(ImGui::GetMousePos());
                    PrefabNodeId newNodeId = doc->CreateComponentNode(componentName, componentName);
                    ComponentNode* node = doc->GetNode(newNodeId);
                    if (node) { node->position = Vector(canvasPos.x, canvasPos.y, 0.0f); node->size = Vector(150.0f, 80.0f, 0.0f); node->enabled = true; }
                 }
             }
             ImGui::EndDragDropTarget();
        }
        ImGui::PopClipRect();

        ImGui::SameLine();

        // ====================================================================
        // RESIZE HANDLE
        // ====================================================================
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
        ImGui::Button("##resizeHandle", ImVec2(handleWidth, -1.0f));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            PanelManager::InspectorPanelWidth -= static_cast<int>(ImGui::GetIO().MouseDelta.x);
        }
        ImGui::PopStyleColor(1);

        ImGui::SameLine();

        // ====================================================================
        // RIGHT PANEL: Tabbed components + properties
        // ====================================================================
        ImGui::BeginChild("EntityPrefabRightPanel", ImVec2(rightPanelWidth, 0), true);
        if (ImGui::BeginTabBar("##EntityPrefabTabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Components")) { m_componentPalette.Render(doc); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Properties")) { m_propertyEditor.Render(doc); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
    }

    void EntityPrefabEditorV2::RenderTypePanels()
    {
        // PHASE 75 FIX: Layout matches Placeholder pattern. 
        // RenderTypePanels must be a NO-OP to prevent duplicated panels below the canvas.
    }

    void EntityPrefabEditorV2::RenderModals()
    {
        // PHASE 75.1 FIX: Delegate modal rendering to framework toolbar
        if (m_framework && m_framework->GetToolbar())
        {
            m_framework->GetToolbar()->RenderModals();
        }
    }

    void EntityPrefabEditorV2::SelectNodesInRectangle(const ImVec2& rectStart, const ImVec2& rectEnd)
    {
        // PHASE E: Delegate to canvas which performs AABB test
        // Canvas will update m_selectedNodeIds in base class via SetRenderer()

        PrefabCanvas* canvas = GetCanvasPtr();
        EntityPrefabGraphDocumentV2* doc = GetDoc();

        if (!canvas || !doc)
            return;

        Vector vStart(rectStart.x, rectStart.y, 0.0f);
        Vector vEnd(rectEnd.x, rectEnd.y, 0.0f);

        canvas->SelectNodesInRectangle(vStart, vEnd, m_ctrlPressed);

        SYSTEM_LOG << "[EntityPrefabEditorV2] SelectNodesInRectangle: " << m_selectedNodeIds.size() << " nodes selected" << std::endl;
    }

    void EntityPrefabEditorV2::DeleteSelectedNodes()
    {
        // PHASE E: Delete all selected nodes from document

        EntityPrefabGraphDocumentV2* doc = GetDoc();
        if (!doc)
            return;

        if (m_selectedNodeIds.empty())
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] DeleteSelectedNodes: No nodes selected" << std::endl;
            return;
        }

        SYSTEM_LOG << "[EntityPrefabEditorV2] DeleteSelectedNodes: Deleting " << m_selectedNodeIds.size() << " node(s)" << std::endl;

        for (int nodeId : m_selectedNodeIds)
        {
            doc->RemoveNode((PrefabNodeId)nodeId);
        }

        m_selectedNodeIds.clear();
        doc->SetDirty(true);  // PHASE E FIX: Use SetDirty() instead of MarkDirty()
    }

    void EntityPrefabEditorV2::MoveSelectedNodes(float deltaX, float deltaY)
    {
        // PHASE E: Move all selected nodes by delta

        EntityPrefabGraphDocumentV2* doc = GetDoc();
        if (!doc)
            return;

        if (m_selectedNodeIds.empty())
            return;

        SYSTEM_LOG << "[EntityPrefabEditorV2] MoveSelectedNodes: Moving " << m_selectedNodeIds.size() << " node(s) by (" << deltaX << ", " << deltaY << ")" << std::endl;

        for (int nodeId : m_selectedNodeIds)
        {
            ComponentNode* node = doc->GetNode((PrefabNodeId)nodeId);
            if (node)
            {
                node->position.x += deltaX;
                node->position.y += deltaY;
            }
        }

        doc->SetDirty(true);  // PHASE E FIX: Use SetDirty() instead of MarkDirty()
    }

    // ========================================================================
    // IGraphRenderer Implementation - File I/O
    // ========================================================================

    bool EntityPrefabEditorV2::Load(const std::string& path)
    {
        EntityPrefabGraphDocumentV2* doc = GetDoc();
        if (!doc)
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Load() called with no document" << std::endl;
            return false;
        }

        SYSTEM_LOG << "[EntityPrefabEditorV2] Loading from: " << path << std::endl;

        bool success = doc->Load(path);

        if (success)
        {
            // Initialize canvas editor if not already done
            if (!m_canvas)
            {
                InitializeCanvasEditor();
            }

            SYSTEM_LOG << "[EntityPrefabEditorV2] Load successful: " << path << std::endl;
            LogAction("Load completed");
        }
        else
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Load failed for: " << path << std::endl;
        }

        return success;
    }

    bool EntityPrefabEditorV2::Save(const std::string& path)
    {
        EntityPrefabGraphDocumentV2* doc = GetDoc();
        if (!doc)
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Save() called with no document" << std::endl;
            return false;
        }

        std::string savePath = path;
        if (savePath.empty())
        {
            savePath = doc->GetFilePath();
            if (savePath.empty())
            {
                SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Save() with empty path and no current file path" << std::endl;
                return false;
            }
        }

        SYSTEM_LOG << "[EntityPrefabEditorV2] Saving to: " << savePath << std::endl;

        bool success = doc->Save(savePath);

        if (success)
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] Save successful: " << savePath << std::endl;
            LogAction("Save completed");
        }
        else
        {
            SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Save failed for: " << savePath << std::endl;
        }

        return success;
    }

    std::string EntityPrefabEditorV2::GetGraphType() const
    {
        return "EntityPrefab";
    }

    // ========================================================================
    // EntityPrefabEditorV2 Specific Methods
    // ========================================================================

    PrefabCanvas* EntityPrefabEditorV2::GetCanvas() const
    {
        return GetCanvasPtr();
    }

    EntityPrefabGraphDocumentV2* EntityPrefabEditorV2::GetDocument() const
    {
        return GetDoc();
    }

    // ========================================================================
    // Private Helper Methods
    // ========================================================================

    void EntityPrefabEditorV2::RenderRightPanelTabs()
    {
        // LEGACY METHOD: Kept for compatibility
        // Now called by RenderTypePanels() override

        RenderTypePanels();
    }

    void EntityPrefabEditorV2::LogAction(const std::string& action) const
    {
        EntityPrefabGraphDocumentV2* doc = GetDoc();
        if (doc)
        {
            std::string filePath = doc->GetFilePath();
            if (!filePath.empty())
            {
                SYSTEM_LOG << "[EntityPrefabEditorV2] " << action << " (" << filePath << ")" << std::endl;
            }
            else
            {
                SYSTEM_LOG << "[EntityPrefabEditorV2] " << action << " (new document)" << std::endl;
            }
        }
    }

} // namespace Olympe
