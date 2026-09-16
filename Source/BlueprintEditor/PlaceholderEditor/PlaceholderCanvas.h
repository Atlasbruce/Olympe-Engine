#pragma once

#include <memory>
#include "../../third_party/imgui/imgui.h"
#include "../Utilities/ICanvasEditor.h"

namespace Olympe {

class PlaceholderGraphDocument;
class PlaceholderGraphRenderer;  // Phase 63.2: Forward declaration for renderer reference
struct PlaceholderNode;
struct PlaceholderConnection;
enum class PlaceholderNodeType;

/**
 * @class PlaceholderCanvas
 * @brief Custom ImGui canvas for rendering Placeholder graphs
 *
 * Provides:
 * - Grid rendering (standardized from Phase 5)
 * - Blue/Green/Magenta node rendering with visual distinction
 * - Bezier curve connection rendering
 * - Pan/zoom/selection input handling (Phase 29 coordinate patterns)
 * - Context menu support
 *
 * C++14 compliant
 */
class PlaceholderCanvas
{
public:
    PlaceholderCanvas();
    ~PlaceholderCanvas();

    /// Initialize with document
    void Initialize(PlaceholderGraphDocument* document);

    /// Phase 63.2: Set renderer reference for updating selection in base class
    void SetRenderer(PlaceholderGraphRenderer* renderer) { m_renderer = renderer; }

    /// Main render call (called each frame)
    void Render();

    /// Set minimap visibility
    void SetMinimapVisible(bool visible) {
        if (m_canvasEditor) m_canvasEditor->SetMinimapVisible(visible);
    }

    /// Set minimap size (0.05 - 0.5 ratio)
    void SetMinimapSize(float size) {
        if (m_canvasEditor) m_canvasEditor->SetMinimapSize(size);
    }

        /// Set minimap position (0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight)
        void SetMinimapPosition(int position) {
            if (m_canvasEditor) m_canvasEditor->SetMinimapPosition(position);
        }

        /// Toolbar integration - grid visibility is owned by the canvas editor.
        void SetGridVisible(bool visible) {
            if (m_canvasEditor) m_canvasEditor->SetGridVisible(visible);
        }

        /// Phase 64: Toolbar integration - Reset pan/zoom
        void ResetPanZoom() {
            if (m_canvasEditor) {
                m_canvasEditor->SetPan(ImVec2(0.0f, 0.0f));
                m_canvasEditor->SetZoom(1.0f);
            }
        }

        /// Phase 64: Get pan offset for canvas positioning
        ImVec2 GetCanvasOffset() const { return m_canvasEditor ? m_canvasEditor->GetPan() : ImVec2(0.0f, 0.0f); }

        /// Phase 64: Get zoom level
        float GetCanvasZoom() const { return m_canvasEditor ? m_canvasEditor->GetZoom() : 1.0f; }

        /// Canonical view-transform provider for framework interaction helpers.
        const ICanvasEditor& GetCanvasEditor() const { return *m_canvasEditor; }

        /// Phase 68 NEW: Accept drag-drop of node type at screen position
        /// Called from renderer's drag-drop overlay
        void AcceptNodeDropAtScreenPosition(PlaceholderNodeType nodeType, float screenX, float screenY);

    private:
        PlaceholderGraphDocument* m_document;
        PlaceholderGraphRenderer* m_renderer;  // Phase 63.2: Reference to update base class selection
        std::unique_ptr<ICanvasEditor> m_canvasEditor; // Canonical pan/zoom/transform authority
         bool m_isDraggingNode;      // Currently dragging a node

        // Phase 64: Connection drag-drop tracking
    bool m_isDraggingConnection;  // Currently dragging from a port
    int m_dragConnectionFromNodeId;  // Source node for connection drag
    ImVec2 m_dragConnectionPreviewEnd;  // End position for preview line

    // Feature #2: Rectangle selection (Phase 4 Step 5)
    bool m_isSelectingRectangle;    // Currently drawing selection rectangle
    bool m_addToRectangleSelection; // Preserve existing selection when Ctrl-dragging
    ImVec2 m_selectionRectStart;    // Rectangle start point
    ImVec2 m_selectionRectEnd;      // Rectangle end point

    // Phase 76: Hover and context menu state tracking
    int m_hoveredNodeId;            // Currently hovered node (-1 for none)
    int m_hoveredConnectionId;      // Currently hovered connection index (-1 for none)
    int m_contextNodeId;            // Node ID for context menu (-1 for none)
    int m_contextConnectionId;      // Connection ID for context menu (-1 for none)

    // Rendering methods
    void RenderGrid();
    void RenderNodes();
    void RenderConnections();
    void RenderContextMenu();
    void RenderSelectionRectangle();  // Feature #2: Selection rectangle rendering
    void RenderConnectionPreviewLine(); // Phase 64.2: Connection drag preview
    void RenderMinimap();             // Phase 52+: Minimap overlay rendering

    // Input handling
    void HandleNodeInteraction();
    void SelectNodesInRectangle();    // Feature #2: AABB intersection test

    // Coordinate transformation (Phase 29 patterns)
    ImVec2 ScreenToCanvas(const ImVec2& screen);
    ImVec2 CanvasToScreen(const ImVec2& canvas);

    // Hit detection
    int GetNodeAtScreenPos(const ImVec2& screen);
    bool IsPointInNodeBounds(int nodeId, const ImVec2& screen);

    // Phase 76: Connection hover detection
    int GetConnectionAtScreenPos(const ImVec2& screen);
    float GetDistanceToConnection(const PlaceholderConnection& conn, const ImVec2& screenPos);

    // Drag-drop handling (Phase 64)
    void HandleNodeCreatedFromPalette(PlaceholderNodeType type, const ImVec2& dropPos);
    void HandleConnectionCreated(int fromNodeId, int toNodeId);

    // Node rendering helpers
    ImU32 GetNodeColorForType(PlaceholderNodeType type);
    void RenderNodeBox(const PlaceholderNode& node, bool isSelected);
    void RenderConnectionLine(const PlaceholderConnection& conn);
};

} // namespace Olympe
