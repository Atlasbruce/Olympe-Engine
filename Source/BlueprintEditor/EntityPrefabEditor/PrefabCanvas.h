#pragma once

#include <string>
#include <memory>
#include "./../../vector.h"
#include "EntityPrefabGraphDocument.h"

namespace Olympe
{
    // Forward declarations
    class ComponentNodeRenderer;

    class PrefabCanvas
    {
    public:
        PrefabCanvas();
        ~PrefabCanvas();

        void Initialize(EntityPrefabGraphDocument* document);
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

        // Display properties
        void SetGridEnabled(bool enabled);
        bool IsGridEnabled() const;

        void SetGridSpacing(float spacing);
        float GetGridSpacing() const;

        void SetShowDebugInfo(bool show);
        bool GetShowDebugInfo() const;

        // Canvas state
        Vector GetCanvasOffset() const;
        void SetCanvasOffset(const Vector& offset);

        float GetCanvasZoom() const;
        void SetCanvasZoom(float zoom);

        Vector ScreenToCanvas(float screenX, float screenY) const;
        Vector CanvasToScreen(float canvasX, float canvasY) const;

    private:
        EntityPrefabGraphDocument* m_document = nullptr;
        std::unique_ptr<ComponentNodeRenderer> m_renderer;

        Vector m_canvasOffset;
        float m_canvasZoom = 1.0f;
        Vector m_lastMousePos;
        bool m_isPanning = false;
        float m_gridSpacing = 50.0f;
        bool m_showGrid = true;
        bool m_showDebugInfo = false;

        void RenderGrid();
        void RenderNodes();
        void RenderConnections();
        void RenderDebugInfo();
        void RenderSelectionBox();
    };
}
