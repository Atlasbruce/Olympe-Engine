#pragma once

#include <string>
#include <memory>
#include <map>
#include "./../../vector.h"
#include "EntityPrefabGraphDocument.h"
#include "../../Source/third_party/imgui/imgui.h"

namespace Olympe
{
    // Forward declarations
    class ComponentNodeRenderer;

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
        PrefabCanvas();
        ~PrefabCanvas();

        void Initialize(EntityPrefabGraphDocument* document);
        EntityPrefabGraphDocument* GetDocument() const;
        void Render();
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
        NodeId GetNodeAtPosition(float x, float y);
        void SelectNodeAt(float x, float y, bool addToSelection = false);
        void ClearSelection();
        void SelectAll();
        void DeleteSelectedNodes();
        void AddComponentNode(const std::string& componentType, const std::string& componentName, float x, float y);
        void AcceptComponentDropAtScreenPos(const std::string& componentType, const std::string& componentName, float screenX, float screenY);


        // Connection interaction
        void StartConnectionCreation(NodeId sourceNodeId);
        void CompleteConnection(NodeId targetNodeId);
        void CancelConnectionCreation();
        bool IsCreatingConnection() const;
        NodeId GetConnectionSourceNode() const;

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

        // Canvas state
        Vector m_canvasOffset;
        float m_canvasZoom = 1.0f;
        float m_gridSpacing = 50.0f;
        bool m_showGrid = true;
        bool m_showDebugInfo = false;
        bool m_snapToGrid = true;

        // Canvas screen position (stored for drag-drop context)
        ImVec2 m_canvasScreenPos = ImVec2(0, 0);

        // Interaction state
        Vector m_lastMousePos;
        Vector m_currentMousePos;
        NodeId m_draggedNodeId = InvalidNodeId;
        Vector m_dragStartPos;
        Vector m_nodeDragOffset;
        bool m_ctrlPressed = false;
        bool m_shiftPressed = false;
        CanvasInteractionMode m_interactionMode = CanvasInteractionMode::Normal;

        // Connection creation state
        NodeId m_connectionSourceNodeId = InvalidNodeId;
        Vector m_connectionPreviewEnd;
        bool m_isCreatingConnection = false;

        // Camera panning state
        bool m_isPanning = false;
        Vector m_panStartOffset;

        // Context menu state
        NodeId m_contextMenuNodeId = InvalidNodeId;
        Vector m_contextMenuMousePos;

        // Utility methods
        void UpdateNodePositions();
        void HandleNodeDragStart(NodeId nodeId, float x, float y);
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
        void RenderDebugInfo();
        void RenderSelectionBox();
        void RenderContextMenu();
    };
}

