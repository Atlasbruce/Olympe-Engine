#pragma once

#include <string>
#include <memory>
#include <map>
#include "./../../vector.h"
#include "EntityPrefabGraphDocument.h"
#include "ComponentNodeRenderer.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../Utilities/CanvasGridRenderer.h"
#include "../Utilities/ICanvasEditor.h"

namespace Olympe
{
    // Forward declarations already included via ICanvasEditor.h

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
            : m_document(nullptr), m_canvasEditor(nullptr)
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
            if (!m_document)
            {
                ImGui::TextDisabled("No document loaded");
                return;
            }

            // Store canvas screen position for coordinate transformations
            m_canvasScreenPos = ImGui::GetCursorScreenPos();

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

                m_renderer->RenderNodes(m_document);
            }

            // TODO: Render connections when graph connections are ready
            // RenderConnections();

            // Draw debug info if enabled
            if (m_showDebugInfo)
            {
                ImGui::TextDisabled("Debug: %zu nodes, %zu connections", 
                    m_document->GetAllNodes().size(),
                    m_document->GetConnections().size());
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
        std::unique_ptr<ComponentNodeRenderer> m_renderer;
        ICanvasEditor* m_canvasEditor = nullptr;  // NEW: Reference to standardized canvas interface

        // Canvas state - REMOVED: m_canvasZoom, m_canvasOffset (now managed by ICanvasEditor)
        // Access via m_canvasEditor->GetZoom(), GetPan() instead
        float m_gridSpacing = 50.0f;
        bool m_showGrid = true;
        bool m_showDebugInfo = false;
        bool m_snapToGrid = true;

        // Canvas screen position (stored for drag-drop context)
        ImVec2 m_canvasScreenPos = ImVec2(0, 0);

        // Interaction state
        Vector m_lastMousePos;
        Vector m_currentMousePos;
        PrefabNodeId m_draggedNodeId = InvalidNodeId;
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
    };

} // namespace Olympe

