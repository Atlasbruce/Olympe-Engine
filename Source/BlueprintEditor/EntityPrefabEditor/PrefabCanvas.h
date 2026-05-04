#pragma once

// Fix Windows min/max macro conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef min
#undef max

#include <string>
#include <memory>
#include <map>
#include <limits>
#include "./../../vector.h"
#include "EntityPrefabGraphDocument.h"
#include "EntityPrefabGraphDocumentV2.h"  // PHASE 54: Need full definition for inline methods
#include "ComponentNodeRenderer.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../Utilities/CanvasGridRenderer.h"
#include "../Utilities/ICanvasEditor.h"

namespace Olympe
{
    // Forward declarations
    class EntityPrefabEditorV2;  // PHASE E: Forward declaration for renderer reference

    // Enum for canvas interaction modes
    enum class CanvasInteractionMode
    {
        Normal,
        DraggingNode,
        PanningCamera,
        CreatingConnection
    };

    class PrefabCanvas
    {
    public:
        PrefabCanvas()
            : m_document(nullptr), m_documentV2(nullptr), m_renderer_ref(nullptr), m_canvasEditor(nullptr)
        {
        }

        ~PrefabCanvas()
        {
        }

        void Initialize(EntityPrefabGraphDocument* document)
        {
            m_document = document;
            // PHASE 52 FIX: Initialize ComponentNodeRenderer for rendering nodes
            if (!m_renderer)
            {
                m_renderer = std::make_unique<ComponentNodeRenderer>();
            }
        }

        // V2 SURCHARGE: Initialize with V2 document
        void InitializeV2(EntityPrefabGraphDocumentV2* documentV2)
        {
            // Store V2 document
            m_documentV2 = documentV2;
            m_document = nullptr;  // Clear V1 document

            // Initialize ComponentNodeRenderer for rendering nodes
            if (!m_renderer)
            {
                m_renderer = std::make_unique<ComponentNodeRenderer>();
            }
        }

        // PHASE E: Set renderer reference for updating selection in base class
        void SetRenderer(EntityPrefabEditorV2* renderer) { m_renderer_ref = renderer; }

        // Getter for V2 document
        EntityPrefabGraphDocumentV2* GetDocumentV2() const
        {
            return m_documentV2;
        }

        void SetCanvasEditor(ICanvasEditor* canvasEditor)  // NEW: Set the standardized interface
        {
            m_canvasEditor = canvasEditor;
        }

        EntityPrefabGraphDocument* GetDocument() const
        {
            return m_document;
        }

        void Render()
        {
            // Check if we have a document (V1 or V2)
            if (!m_document && !m_documentV2)
            {
                ImGui::TextDisabled("No document loaded");
                return;
            }

            // Store canvas screen position for coordinate transformations
            m_canvasScreenPos = ImGui::GetCursorScreenPos();

            // ================================================================
            // PHASE 54: INPUT HANDLING - Poll ImGui input state
            // ================================================================
            ImGuiIO& io = ImGui::GetIO();
            ImVec2 mousePos = io.MousePos;
            ImVec2 canvasMin = m_canvasScreenPos;
            ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            ImVec2 canvasMax(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);

            bool isMouseOverCanvas = (mousePos.x >= canvasMin.x && mousePos.x < canvasMax.x &&
                                     mousePos.y >= canvasMin.y && mousePos.y < canvasMax.y);

            if (isMouseOverCanvas)
            {
                // Update keyboard modifiers
                m_ctrlPressed = io.KeyCtrl;
                m_shiftPressed = io.KeyShift;

                // Mouse move
                OnMouseMove(mousePos.x, mousePos.y);

                // Mouse down
                if (ImGui::IsMouseClicked(0))  // Left
                    OnMouseDown(0, mousePos.x, mousePos.y);
                if (ImGui::IsMouseClicked(1))  // Right
                    OnMouseDown(1, mousePos.x, mousePos.y);
                if (ImGui::IsMouseClicked(2))  // Middle
                    OnMouseDown(2, mousePos.x, mousePos.y);

                // Mouse up
                if (ImGui::IsMouseReleased(0))
                    OnMouseUp(0, mousePos.x, mousePos.y);
                if (ImGui::IsMouseReleased(1))
                    OnMouseUp(1, mousePos.x, mousePos.y);
                if (ImGui::IsMouseReleased(2))
                    OnMouseUp(2, mousePos.x, mousePos.y);

                // Mouse scroll
                if (io.MouseWheel != 0.0f)
                    OnMouseScroll(io.MouseWheel);

                // Delete key
                if (ImGui::IsKeyPressed(ImGuiKey_Delete))
                    OnKeyDown(46);
            }

            // ================================================================
            // RENDERING PIPELINE
            // ================================================================

            // Render grid background if enabled
            if (m_showGrid && m_canvasEditor)
            {
                // Get grid renderer
                CanvasGridRenderer::GridConfig gridConfig;
                gridConfig.canvasPos = m_canvasScreenPos;
                gridConfig.canvasSize = ImGui::GetContentRegionAvail();
                gridConfig.zoom = m_canvasEditor->GetZoom();

                ImVec2 pan = m_canvasEditor->GetPan();
                gridConfig.offsetX = pan.x;
                gridConfig.offsetY = pan.y;
                gridConfig.majorSpacing = m_gridSpacing;

                // Use VisualScript preset style
                CanvasGridRenderer::RenderGrid(gridConfig);
            }

            // Render nodes
            if (m_renderer && m_canvasEditor)
            {
                // PHASE 52.1 FIX: Sync canvas transform (pan/zoom) to renderer each frame
                ImVec2 pan = m_canvasEditor->GetPan();
                float zoom = m_canvasEditor->GetZoom();
                Vector panOffset(pan.x, pan.y);
                m_renderer->SetCanvasTransform(panOffset, zoom);

                // PHASE 54 FIX: Sync canvas screen position to renderer (CRITICAL for coordinate transform)
                m_renderer->SetCanvasScreenPos(m_canvasScreenPos);

                // Render V1 or V2 document
                if (m_document)
                {
                    m_renderer->RenderNodes(m_document);
                }
                else if (m_documentV2)
                {
                    m_renderer->RenderNodes((EntityPrefabGraphDocumentV2*)m_documentV2);
                }
            }

            // Render connections (Phase 54: Use RenderConnections from renderer)
            if (m_renderer && m_canvasEditor)
            {
                if (m_document)
                {
                    m_renderer->RenderConnections(m_document, m_hoveredConnectionIndex);
                }
                else if (m_documentV2)
                {
                    m_renderer->RenderConnections((EntityPrefabGraphDocumentV2*)m_documentV2, m_hoveredConnectionIndex);
                }
            }

            // PHASE 54: Render interactive overlays
            RenderConnectionPreview();  // Yellow connection line during dragging
            RenderSelectionRectangle(); // Blue selection rectangle
            RenderContextMenu();        // Right-click context menus

            // Draw debug info if enabled
            if (m_showDebugInfo)
            {
                size_t nodeCount = 0;
                size_t connCount = 0;

                if (m_document)
                {
                    nodeCount = m_document->GetAllNodes().size();
                    connCount = m_document->GetConnections().size();
                }
                else if (m_documentV2)
                {
                    // V2 document - can't access methods from void pointer inline
                    // Just show "V2 Document" for now
                    nodeCount = 999;  // Placeholder
                    connCount = 0;
                }

                if (m_documentV2)
                {
                    ImGui::TextDisabled("Debug: V2 Document (nodes via canvas)");
                }
                else
                {
                    ImGui::TextDisabled("Debug: %zu nodes, %zu connections", nodeCount, connCount);
                }
            }
        }

        void Update(float deltaTime);

        // Input handling
        void OnMouseMove(float x, float y);
        void OnMouseDown(int button, float x, float y);
        void OnMouseUp(int button, float x, float y);
        void OnMouseScroll(float delta);
        void OnKeyDown(int keyCode);
        void OnKeyUp(int keyCode);

        // Canvas manipulation
        void PanCanvas(float deltaX, float deltaY);
        void ZoomCanvas(float zoomDelta, float centerX, float centerY);
        void ResetView();
        void FitToContent();

        // PHASE 75.1: Framework sync helpers
        ComponentNodeRenderer* GetRenderer() const { return m_renderer.get(); }
        PrefabNodeId GetHoveredNodeId() const { return m_hoveredNodeId; }
        int GetHoveredConnectionIndex() const { return m_hoveredConnectionIndex; }

        // Node interaction
        PrefabNodeId GetNodeAtPosition(float x, float y);
        void SelectNodeAt(float x, float y, bool addToSelection = false);
        void SelectNodesInRectangle(const Vector& rectStart, const Vector& rectEnd, bool addToSelection = false);
        void ClearSelection();
        void SelectAll();
        void DeleteSelectedNodes();
        void AddComponentNode(const std::string& componentType, const std::string& componentName, float x, float y);
        void AcceptComponentDropAtScreenPos(const std::string& componentType, const std::string& componentName, float screenX, float screenY)
        {
            // Handle component drop at screen position
            if (m_document)
            {
                // Create new node at dropped position
                // Implementation deferred to Phase 50.4
            }
        }


        // Connection interaction
        void StartConnectionCreation(PrefabNodeId sourceNodeId);
        void CompleteConnection(PrefabNodeId targetNodeId);
        void CancelConnectionCreation();
        bool IsCreatingConnection() const;
        PrefabNodeId GetConnectionSourceNode() const;

        // Display properties
        void SetGridEnabled(bool enabled);
        bool IsGridEnabled() const;

        void SetGridSpacing(float spacing);
        float GetGridSpacing() const;

        void SetShowDebugInfo(bool show);
        bool GetShowDebugInfo() const;

        void SetSnapToGrid(bool snap);
        bool IsSnapToGridEnabled() const;

        // ====================================================================
        // FRAMEWORK INTEGRATION METHODS (CRITICAL - NEW)
        // ====================================================================
        // These methods are called by RenderCommonToolbar() when toolbar buttons are clicked
        // See FRAMEWORK_INTEGRATION_GUIDE.md for integration pattern

        /**
         * @brief Set grid visibility (called by Grid checkbox in toolbar)
         * @param visible True to show grid, false to hide
         * Controls m_showGrid flag which RenderGrid() uses
         */
        void SetGridVisible(bool visible)
        {
            m_showGrid = visible;
        }

        /**
         * @brief Set minimap visibility (called by Minimap checkbox in toolbar)
         * @param visible True to show minimap, false to hide
         * TODO: Connect to minimap renderer when implemented
         */
        void SetMinimapVisible(bool visible)
        {
            m_minimapVisible = visible;
            // Future: Call m_minimapRenderer->SetVisible(visible) when minimap implemented
        }

        /**
         * @brief Reset pan and zoom to default (called by Reset View button in toolbar)
         * Resets canvas offset to (0,0) and zoom to 1.0x
         */
        void ResetPanZoom()
        {
            if (m_canvasEditor)
            {
                m_canvasEditor->SetPan(ImVec2(0.0f, 0.0f));
                m_canvasEditor->SetZoom(1.0f);
            }
        }

        // ====================================================================
        // End Framework Integration Methods
        // ====================================================================

        // Canvas state
        Vector GetCanvasOffset() const;
        void SetCanvasOffset(const Vector& offset);

        float GetCanvasZoom() const;
        void SetCanvasZoom(float zoom);

        Vector ScreenToCanvas(float screenX, float screenY) const;
        Vector CanvasToScreen(float canvasX, float canvasY) const;

        // Get canvas screen position for coordinate transformations
        ImVec2 GetCanvasScreenPos() const;

        // Interaction state
        CanvasInteractionMode GetInteractionMode() const;
        bool IsNodeDragging() const;
        bool IsPanning() const;

    private:
        EntityPrefabGraphDocument* m_document = nullptr;
        EntityPrefabGraphDocumentV2* m_documentV2 = nullptr;  // PHASE E: Strong typed pointer (not void*)
        EntityPrefabEditorV2* m_renderer_ref = nullptr;  // PHASE E: Renderer reference for selection sync
        std::unique_ptr<ComponentNodeRenderer> m_renderer;
        ICanvasEditor* m_canvasEditor = nullptr;  // NEW: Reference to standardized canvas interface

        // Canvas state - REMOVED: m_canvasZoom, m_canvasOffset (now managed by ICanvasEditor)
        // Access via m_canvasEditor->GetZoom(), GetPan() instead
        float m_gridSpacing = 50.0f;
        bool m_showGrid = true;
        bool m_showDebugInfo = false;
        bool m_snapToGrid = true;

        // Framework toolbar state (synced with RenderCommonToolbar() checkboxes)
        bool m_minimapVisible = true;      // Minimap visibility toggle (from toolbar)

        // Canvas screen position (stored for drag-drop context)
        ImVec2 m_canvasScreenPos = ImVec2(0, 0);

        // Interaction state
        Vector m_lastMousePos;
        Vector m_currentMousePos;
        PrefabNodeId m_draggedNodeId = InvalidNodeId;
        PrefabNodeId m_hoveredNodeId = InvalidNodeId;  // PHASE 75.1: Added hover member
        Vector m_dragStartPos;
        Vector m_nodeDragOffset;
        bool m_ctrlPressed = false;
        bool m_shiftPressed = false;
        CanvasInteractionMode m_interactionMode = CanvasInteractionMode::Normal;

        // Connection creation state
        PrefabNodeId m_connectionSourceNodeId = InvalidNodeId;
        Vector m_connectionPreviewEnd;
        bool m_isCreatingConnection = false;

        // Camera panning state
        bool m_isPanning = false;
        Vector m_panStartOffset;

        // Context menu state
        PrefabNodeId m_contextMenuNodeId = InvalidNodeId;
        Vector m_contextMenuMousePos;

        // Connection context menu state
        int m_hoveredConnectionIndex = -1;  // Index in connections array, -1 if none
        int m_contextMenuConnectionIndex = -1;  // Index for connection context menu
        Vector m_contextMenuConnectionMousePos;
        bool m_showConnectionContextMenu = false;

        // Rectangle selection state
        bool m_isSelectingRectangle = false;
        Vector m_selectionRectStart;
        Vector m_selectionRectEnd;

        // Utility methods
        void UpdateNodePositions();
        void HandleNodeDragStart(PrefabNodeId nodeId, float x, float y);
        void HandleNodeDrag(float x, float y);
        void HandleNodeDragEnd();
        void HandleConnectionCreation(float x, float y);
        void HandleConnectionEnd(float x, float y);
        void HandlePanStart(float x, float y);
        void HandlePan(float x, float y);
        void SnapNodePositionToGrid(Vector& position);

        // Rendering helpers
        void RenderGrid();
        void RenderNodes();
        void RenderConnections();
        void RenderConnectionPreview();
        void RenderConnectionContextMenu();
        void RenderDebugInfo();
        void RenderSelectionBox();
        void RenderSelectionRectangle();
        void RenderContextMenu();

        // Helper methods for interaction
        PrefabNodeId GetNodeAtScreenPos(const ImVec2& screenPos) const;
        bool IsPointInNodeBounds(PrefabNodeId nodeId, const ImVec2& screenPos) const;
        int GetConnectionAtScreenPos(const ImVec2& screenPos) const;
        float GetDistanceToConnectionLine(int connectionIndex, const ImVec2& screenPos) const;
    };

    // ========================================================================
    // PHASE 54: INLINE IMPLEMENTATIONS - Interactive Editing Features
    // ========================================================================
    // All interactive handlers implemented inline for complete feature parity with Phase 30 legacy

    inline void PrefabCanvas::OnMouseMove(float x, float y)
    {
        m_currentMousePos = Vector(x, y, 0.0f);
        ImVec2 screenPos(x, y);

        // Handle ongoing interactions
        if (m_isSelectingRectangle)
        {
            // Update rectangle selection end point
            m_selectionRectEnd = Vector(x, y, 0.0f);
        }
        else if (m_draggedNodeId != InvalidNodeId)
        {
            // Handle node dragging
            HandleNodeDrag(x, y);
        }
        else if (m_isCreatingConnection)
        {
            // Update connection preview end point
            m_connectionPreviewEnd = Vector(x, y, 0.0f);
        }
        else
        {
            // PHASE 75.1: Passive hover detection
            m_hoveredNodeId = GetNodeAtScreenPos(screenPos);
            m_hoveredConnectionIndex = GetConnectionAtScreenPos(screenPos);
        }
    }

    inline void PrefabCanvas::OnMouseDown(int button, float x, float y)
    {
        ImVec2 screenPos(x, y);

        // Get document (V1 or V2)
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2 && !m_document) return;

        // Left mouse button
        if (button == 0)
        {
            // Check for node at position
            PrefabNodeId nodeAtPos = GetNodeAtScreenPos(screenPos);

            if (nodeAtPos != InvalidNodeId)
            {
                // Check if clicking on a port for connection creation
                if (m_renderer && docV2)
                {
                    ComponentNode* node = docV2->GetNode(nodeAtPos);
                    if (node && node->ports.size() > 0)
                    {
                        PortId portId;
                        Vector clickPoint(x, y, 0.0f);

                        if (m_renderer->IsPointInPort(clickPoint, *node, portId))
                        {
                            // Start connection from this port
                            m_isCreatingConnection = true;
                            m_connectionSourceNodeId = nodeAtPos;
                            m_connectionPreviewEnd = Vector(x, y, 0.0f);
                            return;
                        }
                    }
                }

                // Node click (not on port) - handle selection
                if (m_ctrlPressed)
                {
                    // Multi-select with Ctrl
                    if (docV2)
                    {
                        ComponentNode* node = docV2->GetNode(nodeAtPos);
                        if (node && node->selected)
                        {
                            docV2->DeselectNode(nodeAtPos);
                        }
                        else
                        {
                            docV2->SelectNode(nodeAtPos);
                        }
                    }
                }
                else
                {
                    // Single select - PHASE 75 FIX: Keep selection if already selected to allow group dragging
                    ComponentNode* nodeAtPosPtr = docV2 ? docV2->GetNode(nodeAtPos) : nullptr;
                    if (docV2 && nodeAtPosPtr && !nodeAtPosPtr->selected)
                    {
                        docV2->DeselectAll();
                        docV2->SelectNode(nodeAtPos);
                    }
                }

                // Start node dragging
                HandleNodeDragStart(nodeAtPos, x, y);
            }
            else
            {
                // Empty space - start rectangle selection
                if (!m_ctrlPressed && docV2)
                {
                    docV2->DeselectAll();
                }

                m_isSelectingRectangle = true;
                m_selectionRectStart = Vector(x, y, 0.0f);
                m_selectionRectEnd = Vector(x, y, 0.0f);
            }
        }
        // Right mouse button - context menu
        else if (button == 1)
        {
            // PHASE 30 PRIORITY: Check for connection first, then node
            int connectionIndex = GetConnectionAtScreenPos(screenPos);

            if (connectionIndex >= 0)
            {
                m_contextMenuConnectionIndex = connectionIndex;
                ImGui::OpenPopup("##connection_context_menu");
            }
            else
            {
                PrefabNodeId nodeAtPos = GetNodeAtScreenPos(screenPos);
                if (nodeAtPos != InvalidNodeId)
                {
                    // Right-click on node
                    m_contextMenuNodeId = nodeAtPos;
                    ImGui::OpenPopup("##node_context_menu");
                }
                else
                {
                    // Right-click on empty canvas
                    ImGui::OpenPopup("##canvas_context_menu");
                }
            }
        }
        // Middle mouse button - panning
        else if (button == 2)
        {
            HandlePanStart(x, y);
        }
    }

    inline void PrefabCanvas::OnMouseUp(int button, float x, float y)
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();

        // Left mouse button
        if (button == 0)
        {
            // Finalize rectangle selection
            if (m_isSelectingRectangle)
            {
                SelectNodesInRectangle(m_selectionRectStart, m_selectionRectEnd, m_ctrlPressed);
                m_isSelectingRectangle = false;
            }

            // Finalize node dragging
            if (m_draggedNodeId != InvalidNodeId)
            {
                HandleNodeDragEnd();
            }

            // Finalize connection creation
            if (m_isCreatingConnection)
            {
                ImVec2 screenPos(x, y);
                PrefabNodeId targetNodeId = GetNodeAtScreenPos(screenPos);

                if (targetNodeId != InvalidNodeId && targetNodeId != m_connectionSourceNodeId)
                {
                    if (docV2)
                    {
                        docV2->ConnectNodes(m_connectionSourceNodeId, targetNodeId);
                    }
                }

                m_isCreatingConnection = false;
                m_connectionSourceNodeId = InvalidNodeId;
            }
        }
        // Middle mouse button - stop panning
        else if (button == 2)
        {
            m_isPanning = false;
        }
    }

    inline void PrefabCanvas::OnMouseScroll(float delta)
    {
        // Zoom handled by ICanvasEditor (already working via CustomCanvasEditor)
        // No additional implementation needed here
    }

    inline void PrefabCanvas::OnKeyDown(int keyCode)
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();

        // Delete key - delete selected nodes
        if (keyCode == 46 || keyCode == 127)  // Delete or Backspace
        {
            if (docV2)
            {
                const auto& selectedNodes = docV2->GetSelectedNodes();
                for (PrefabNodeId nodeId : selectedNodes)
                {
                    docV2->RemoveNode(nodeId);
                }
                docV2->DeselectAll();
            }
        }
        // Ctrl key
        else if (keyCode == 17 || keyCode == 306)  // Ctrl
        {
            m_ctrlPressed = true;
        }
        // Shift key
        else if (keyCode == 16 || keyCode == 304)  // Shift
        {
            m_shiftPressed = true;
        }
    }

    inline void PrefabCanvas::OnKeyUp(int keyCode)
    {
        // Ctrl key
        if (keyCode == 17 || keyCode == 306)  // Ctrl
        {
            m_ctrlPressed = false;
        }
        // Shift key
        else if (keyCode == 16 || keyCode == 304)  // Shift
        {
            m_shiftPressed = false;
        }
    }

    inline void PrefabCanvas::SelectNodesInRectangle(const Vector& rectStart, const Vector& rectEnd, bool addToSelection)
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2) return;

        // Calculate AABB bounds
        float minX = (rectStart.x < rectEnd.x) ? rectStart.x : rectEnd.x;
        float maxX = (rectStart.x > rectEnd.x) ? rectStart.x : rectEnd.x;
        float minY = (rectStart.y < rectEnd.y) ? rectStart.y : rectEnd.y;
        float maxY = (rectStart.y > rectEnd.y) ? rectStart.y : rectEnd.y;

        if (!addToSelection)
        {
            docV2->DeselectAll();
        }

        // Test each node for intersection
        const auto& nodes = docV2->GetAllNodes();
        for (const auto& node : nodes)
        {
            if (!m_canvasEditor) continue;

            // PHASE 75 FIX: Sync with centered origin logic
            ImVec2 nodeCenterCanvas(node.position.x, node.position.y);
            ImVec2 screenCenter = m_canvasEditor->CanvasToScreen(nodeCenterCanvas);

            float halfWidth = (node.size.x * 0.5f) * m_canvasEditor->GetZoom();
            float halfHeight = (node.size.y * 0.5f) * m_canvasEditor->GetZoom();

            float nodeMinX = screenCenter.x - halfWidth;
            float nodeMaxX = screenCenter.x + halfWidth;
            float nodeMinY = screenCenter.y - halfHeight;
            float nodeMaxY = screenCenter.y + halfHeight;

            // AABB intersection test
            bool intersects = !(nodeMaxX < minX || nodeMinX > maxX ||
                               nodeMaxY < minY || nodeMinY > maxY);

            if (intersects)
            {
                docV2->SelectNode(node.nodeId);
            }
        }
    }

    inline void PrefabCanvas::RenderSelectionRectangle()
    {
        if (!m_isSelectingRectangle) return;

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        if (!drawList) return;

        // Convert to screen coordinates
        ImVec2 start(m_selectionRectStart.x, m_selectionRectStart.y);
        ImVec2 end(m_selectionRectEnd.x, m_selectionRectEnd.y);

        // Calculate rectangle bounds
        ImVec2 minPos((start.x < end.x) ? start.x : end.x, (start.y < end.y) ? start.y : end.y);
        ImVec2 maxPos((start.x > end.x) ? start.x : end.x, (start.y > end.y) ? start.y : end.y);

        // Draw filled rectangle with semi-transparent blue
        drawList->AddRectFilled(minPos, maxPos, IM_COL32(100, 150, 255, 64));

        // Draw outline rectangle in bright blue
        drawList->AddRect(minPos, maxPos, IM_COL32(100, 150, 255, 255), 0.0f, 0, 2.0f);
    }

    inline void PrefabCanvas::RenderConnectionPreview()
    {
        if (!m_isCreatingConnection) return;
        if (m_connectionSourceNodeId == InvalidNodeId) return;

        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2) return;

        ComponentNode* sourceNode = docV2->GetNode(m_connectionSourceNodeId);
        if (!sourceNode || !m_canvasEditor) return;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (!drawList) return;

        // Calculate source position (right side of node)
        // PHASE 75 FIX: node->position is center. Right edge is position.x + (size.x * 0.5)
        ImVec2 sourceCanvasPos(sourceNode->position.x + sourceNode->size.x * 0.5f, 
                              sourceNode->position.y);
        ImVec2 sourceScreenPos = m_canvasEditor->CanvasToScreen(sourceCanvasPos);

        // End position (current mouse position)
        ImVec2 endScreenPos(m_connectionPreviewEnd.x, m_connectionPreviewEnd.y);

        // Bezier control points (40% of horizontal distance)
        float controlPointOffset = (endScreenPos.x - sourceScreenPos.x) * 0.4f;
        ImVec2 cp1(sourceScreenPos.x + controlPointOffset, sourceScreenPos.y);
        ImVec2 cp2(endScreenPos.x - controlPointOffset, endScreenPos.y);

        // Draw yellow preview line
        drawList->PathLineTo(sourceScreenPos);
        drawList->PathBezierCubicCurveTo(cp1, cp2, endScreenPos, 32);
        drawList->PathStroke(IM_COL32(255, 255, 0, 255), false, 2.0f);
    }

    inline void PrefabCanvas::RenderContextMenu()
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();

        // Node context menu
        if (ImGui::BeginPopup("##node_context_menu"))
        {
            if (m_contextMenuNodeId != InvalidNodeId && docV2)
            {
                if (ImGui::MenuItem("Delete Node"))
                {
                    docV2->RemoveNode(m_contextMenuNodeId);
                    docV2->DeselectAll();
                    m_contextMenuNodeId = InvalidNodeId;
                }
                if (ImGui::MenuItem("Properties"))
                {
                    // Select node for property panel
                    docV2->DeselectAll();
                    docV2->SelectNode(m_contextMenuNodeId);
                }
            }
            ImGui::EndPopup();
        }

        // Connection context menu
        if (ImGui::BeginPopup("##connection_context_menu"))
        {
            if (m_contextMenuConnectionIndex >= 0 && docV2)
            {
                if (ImGui::MenuItem("Delete Connection"))
                {
                    const auto& connections = docV2->GetConnections();
                    if (m_contextMenuConnectionIndex < static_cast<int>(connections.size()))
                    {
                        const auto& conn = connections[m_contextMenuConnectionIndex];
                        docV2->DisconnectNodes(conn.first, conn.second);
                        m_contextMenuConnectionIndex = -1;
                    }
                }
            }
            ImGui::EndPopup();
        }

        // Canvas context menu
        if (ImGui::BeginPopup("##canvas_context_menu"))
        {
            if (ImGui::MenuItem("Select All"))
            {
                if (docV2)
                {
                    const auto& nodes = docV2->GetAllNodes();
                    for (const auto& node : nodes)
                    {
                        docV2->SelectNode(node.nodeId);
                    }
                }
            }
            if (ImGui::MenuItem("Reset View"))
            {
                if (m_canvasEditor)
                {
                    m_canvasEditor->SetPan(ImVec2(0.0f, 0.0f));
                    m_canvasEditor->SetZoom(1.0f);
                }
            }
            ImGui::EndPopup();
        }
    }

    inline void PrefabCanvas::HandleNodeDragStart(PrefabNodeId nodeId, float x, float y)
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2) return;

        ComponentNode* node = docV2->GetNode(nodeId);
        if (!node) return;

        m_draggedNodeId = nodeId;
        m_dragStartPos = Vector(x, y, 0.0f);

        // Calculate offset from node position to click point
        if (m_canvasEditor)
        {
            ImVec2 clickCanvas = m_canvasEditor->ScreenToCanvas(ImVec2(x, y));
            m_nodeDragOffset = Vector(node->position.x - clickCanvas.x, 
                                     node->position.y - clickCanvas.y, 0.0f);
        }
    }

    inline void PrefabCanvas::HandleNodeDrag(float x, float y)
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2 || m_draggedNodeId == InvalidNodeId || !m_canvasEditor) return;

        // Convert screen to canvas coordinates
        ImVec2 canvasPos = m_canvasEditor->ScreenToCanvas(ImVec2(x, y));

        // Get all selected nodes
        const auto& selectedNodes = docV2->GetSelectedNodes();
        bool draggedNodeIsSelected = false;
        for (PrefabNodeId id : selectedNodes)
        {
            if (id == m_draggedNodeId)
            {
                draggedNodeIsSelected = true;
                break;
            }
        }

        if (draggedNodeIsSelected && selectedNodes.size() > 1)
        {
            // Multi-node drag: calculate delta and move all selected nodes
            ComponentNode* draggedNode = docV2->GetNode(m_draggedNodeId);
            if (draggedNode)
            {
                Vector newPos(canvasPos.x + m_nodeDragOffset.x, 
                            canvasPos.y + m_nodeDragOffset.y, 0.0f);

                Vector delta = newPos - draggedNode->position;

                // Move all selected nodes by delta
                for (PrefabNodeId id : selectedNodes)
                {
                    ComponentNode* node = docV2->GetNode(id);
                    if (node)
                    {
                        node->position = node->position + delta;
                    }
                }
            }
        }
        else
        {
            // Single node drag
            ComponentNode* node = docV2->GetNode(m_draggedNodeId);
            if (node)
            {
                node->position = Vector(canvasPos.x + m_nodeDragOffset.x, 
                                       canvasPos.y + m_nodeDragOffset.y, 0.0f);
            }
        }
    }

    inline void PrefabCanvas::HandleNodeDragEnd()
    {
        m_draggedNodeId = InvalidNodeId;

        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (docV2)
        {
            docV2->SetDirty(true);
        }
    }

    inline void PrefabCanvas::HandlePanStart(float x, float y)
    {
        m_isPanning = true;
        if (m_canvasEditor)
        {
            ImVec2 currentPan = m_canvasEditor->GetPan();
            m_panStartOffset = Vector(currentPan.x, currentPan.y, 0.0f);
        }
    }

    inline PrefabNodeId PrefabCanvas::GetNodeAtScreenPos(const ImVec2& screenPos) const
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2 || !m_canvasEditor) return InvalidNodeId;

        const auto& nodes = docV2->GetAllNodes();

        // Iterate in reverse order to check top nodes first
        for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
        {
            if (IsPointInNodeBounds(it->nodeId, screenPos))
            {
                return it->nodeId;
            }
        }

        return InvalidNodeId;
    }

    inline bool PrefabCanvas::IsPointInNodeBounds(PrefabNodeId nodeId, const ImVec2& screenPos) const
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2 || !m_canvasEditor) return false;

        const ComponentNode* node = docV2->GetNode(nodeId);
        if (!node) return false;

        // PHASE 75 FIX: ComponentNodeRenderer uses centered origin for nodes.
        // Sync interaction logic to match: node->position is the CENTER of the node.
        ImVec2 nodeCenterPos(node->position.x, node->position.y);
        ImVec2 screenCenter = m_canvasEditor->CanvasToScreen(nodeCenterPos);

        float halfWidth = (node->size.x * 0.5f) * m_canvasEditor->GetZoom();
        float halfHeight = (node->size.y * 0.5f) * m_canvasEditor->GetZoom();

        return (screenPos.x >= screenCenter.x - halfWidth && screenPos.x <= screenCenter.x + halfWidth &&
                screenPos.y >= screenCenter.y - halfHeight && screenPos.y <= screenCenter.y + halfHeight);
    }

    inline int PrefabCanvas::GetConnectionAtScreenPos(const ImVec2& screenPos) const
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2) return -1;

        const auto& connections = docV2->GetConnections();
        const float tolerance = 10.0f;  // 10 pixel tolerance

        for (size_t i = 0; i < connections.size(); ++i)
        {
            float distance = GetDistanceToConnectionLine(static_cast<int>(i), screenPos);
            if (distance <= tolerance)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    inline float PrefabCanvas::GetDistanceToConnectionLine(int connectionIndex, const ImVec2& screenPos) const
    {
        EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2();
        if (!docV2 || !m_canvasEditor) return std::numeric_limits<float>::max();

        const auto& connections = docV2->GetConnections();
        if (connectionIndex < 0 || connectionIndex >= static_cast<int>(connections.size()))
        {
            return std::numeric_limits<float>::max();
        }

        const auto& conn = connections[connectionIndex];
        const ComponentNode* fromNode = docV2->GetNode(conn.first);
        const ComponentNode* toNode = docV2->GetNode(conn.second);

        if (!fromNode || !toNode) return std::numeric_limits<float>::max();

        // PHASE 75 FIX: Calculate connection start and end points using centered coordinates
        // fromNode->position is center. Right edge is center.x + halfWidth.
        ImVec2 fromCanvasPos(fromNode->position.x + fromNode->size.x * 0.5f, 
                            fromNode->position.y);
        // toNode->position is center. Left edge is center.x - halfWidth.
        ImVec2 toCanvasPos(toNode->position.x - toNode->size.x * 0.5f, 
                          toNode->position.y);

        ImVec2 fromScreenPos = m_canvasEditor->CanvasToScreen(fromCanvasPos);
        ImVec2 toScreenPos = m_canvasEditor->CanvasToScreen(toCanvasPos);

        // Bezier control points
        float controlPointOffset = (toScreenPos.x - fromScreenPos.x) * 0.4f;
        ImVec2 cp1(fromScreenPos.x + controlPointOffset, fromScreenPos.y);
        ImVec2 cp2(toScreenPos.x - controlPointOffset, toScreenPos.y);

        // Sample Bezier curve to find closest distance
        float minDistance = std::numeric_limits<float>::max();
        const int samples = 32;

        for (int i = 0; i <= samples; ++i)
        {
            float t = static_cast<float>(i) / samples;
            float mt = 1.0f - t;
            float mt2 = mt * mt;
            float mt3 = mt2 * mt;
            float t2 = t * t;
            float t3 = t2 * t;

            ImVec2 curvePoint(
                mt3 * fromScreenPos.x + 3.0f * mt2 * t * cp1.x + 3.0f * mt * t2 * cp2.x + t3 * toScreenPos.x,
                mt3 * fromScreenPos.y + 3.0f * mt2 * t * cp1.y + 3.0f * mt * t2 * cp2.y + t3 * toScreenPos.y
            );

            float dx = screenPos.x - curvePoint.x;
            float dy = screenPos.y - curvePoint.y;
            float distance = sqrt(dx * dx + dy * dy);

            minDistance = (distance < minDistance) ? distance : minDistance;
        }

        return minDistance;
    }

    // Stub implementations for remaining methods
    inline void PrefabCanvas::Update(float deltaTime) { (void)deltaTime; }
    inline void PrefabCanvas::PanCanvas(float deltaX, float deltaY) { (void)deltaX; (void)deltaY; }
    inline void PrefabCanvas::ZoomCanvas(float zoomDelta, float centerX, float centerY) { (void)zoomDelta; (void)centerX; (void)centerY; }
    inline void PrefabCanvas::ResetView() { if (m_canvasEditor) { m_canvasEditor->SetPan(ImVec2(0, 0)); m_canvasEditor->SetZoom(1.0f); } }
    inline void PrefabCanvas::FitToContent() {}
    inline PrefabNodeId PrefabCanvas::GetNodeAtPosition(float x, float y) { return GetNodeAtScreenPos(ImVec2(x, y)); }
    inline void PrefabCanvas::SelectNodeAt(float x, float y, bool addToSelection) { (void)x; (void)y; (void)addToSelection; }
    inline void PrefabCanvas::ClearSelection() { EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2(); if (docV2) docV2->DeselectAll(); }
    inline void PrefabCanvas::SelectAll() { EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2(); if (docV2) { const auto& nodes = docV2->GetAllNodes(); for (const auto& n : nodes) docV2->SelectNode(n.nodeId); } }
    inline void PrefabCanvas::DeleteSelectedNodes() { EntityPrefabGraphDocumentV2* docV2 = GetDocumentV2(); if (docV2) { const auto& sel = docV2->GetSelectedNodes(); for (auto id : sel) docV2->RemoveNode(id); docV2->DeselectAll(); } }
    inline void PrefabCanvas::AddComponentNode(const std::string& componentType, const std::string& componentName, float x, float y) { (void)componentType; (void)componentName; (void)x; (void)y; }
    inline void PrefabCanvas::StartConnectionCreation(PrefabNodeId sourceNodeId) { m_isCreatingConnection = true; m_connectionSourceNodeId = sourceNodeId; }
    inline void PrefabCanvas::CompleteConnection(PrefabNodeId targetNodeId) { (void)targetNodeId; m_isCreatingConnection = false; }
    inline void PrefabCanvas::CancelConnectionCreation() { m_isCreatingConnection = false; m_connectionSourceNodeId = InvalidNodeId; }
    inline bool PrefabCanvas::IsCreatingConnection() const { return m_isCreatingConnection; }
    inline PrefabNodeId PrefabCanvas::GetConnectionSourceNode() const { return m_connectionSourceNodeId; }
    inline void PrefabCanvas::SetGridEnabled(bool enabled) { m_showGrid = enabled; }
    inline bool PrefabCanvas::IsGridEnabled() const { return m_showGrid; }
    inline void PrefabCanvas::SetGridSpacing(float spacing) { m_gridSpacing = spacing; }
    inline float PrefabCanvas::GetGridSpacing() const { return m_gridSpacing; }
    inline void PrefabCanvas::SetShowDebugInfo(bool show) { m_showDebugInfo = show; }
    inline bool PrefabCanvas::GetShowDebugInfo() const { return m_showDebugInfo; }
    inline void PrefabCanvas::SetSnapToGrid(bool snap) { m_snapToGrid = snap; }
    inline bool PrefabCanvas::IsSnapToGridEnabled() const { return m_snapToGrid; }
    inline Vector PrefabCanvas::GetCanvasOffset() const { if (m_canvasEditor) { ImVec2 p = m_canvasEditor->GetPan(); return Vector(p.x, p.y, 0); } return Vector(0,0,0); }
    inline void PrefabCanvas::SetCanvasOffset(const Vector& offset) { if (m_canvasEditor) m_canvasEditor->SetPan(ImVec2(offset.x, offset.y)); }
    inline float PrefabCanvas::GetCanvasZoom() const { return m_canvasEditor ? m_canvasEditor->GetZoom() : 1.0f; }
    inline void PrefabCanvas::SetCanvasZoom(float zoom) { if (m_canvasEditor) m_canvasEditor->SetZoom(zoom); }
    inline Vector PrefabCanvas::ScreenToCanvas(float screenX, float screenY) const { if (m_canvasEditor) { ImVec2 c = m_canvasEditor->ScreenToCanvas(ImVec2(screenX, screenY)); return Vector(c.x, c.y, 0); } return Vector(screenX, screenY, 0); }
    inline Vector PrefabCanvas::CanvasToScreen(float canvasX, float canvasY) const { if (m_canvasEditor) { ImVec2 s = m_canvasEditor->CanvasToScreen(ImVec2(canvasX, canvasY)); return Vector(s.x, s.y, 0); } return Vector(canvasX, canvasY, 0); }
    inline ImVec2 PrefabCanvas::GetCanvasScreenPos() const { return m_canvasScreenPos; }
    inline CanvasInteractionMode PrefabCanvas::GetInteractionMode() const { return m_interactionMode; }
    inline bool PrefabCanvas::IsNodeDragging() const { return m_draggedNodeId != InvalidNodeId; }
    inline bool PrefabCanvas::IsPanning() const { return m_isPanning; }

    inline void PrefabCanvas::UpdateNodePositions() {}
    inline void PrefabCanvas::HandleConnectionCreation(float x, float y) { (void)x; (void)y; }
    inline void PrefabCanvas::HandleConnectionEnd(float x, float y) { (void)x; (void)y; }
    inline void PrefabCanvas::HandlePan(float x, float y) { (void)x; (void)y; }
    inline void PrefabCanvas::SnapNodePositionToGrid(Vector& position) { (void)position; }
    inline void PrefabCanvas::RenderGrid() {}
    inline void PrefabCanvas::RenderNodes() {}
    inline void PrefabCanvas::RenderConnections() {}
    inline void PrefabCanvas::RenderDebugInfo() {}
    inline void PrefabCanvas::RenderSelectionBox() {}

} // namespace Olympe

