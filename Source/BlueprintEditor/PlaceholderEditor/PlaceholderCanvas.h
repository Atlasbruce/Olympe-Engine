#pragma once

#include <memory>
#include <unordered_map>
#include "../../third_party/imgui/imgui.h"
#include "../Utilities/CanvasMinimapRenderer.h"

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

    /// Get selected node ID (-1 if none)
    int GetSelectedNodeId() const { return m_selectedNodeId; }

    /// Set selected node
    void SetSelectedNodeId(int nodeId) { m_selectedNodeId = nodeId; }

    /// Get minimap renderer for rendering minimap overlay
    CanvasMinimapRenderer* GetMinimapRenderer() { return m_minimapRenderer.get(); }

    /// Set minimap visibility
    void SetMinimapVisible(bool visible) { 
        if (m_minimapRenderer) m_minimapRenderer->SetVisible(visible);
    }

    /// Set minimap size (0.05 - 0.5 ratio)
    void SetMinimapSize(float size) {
        if (m_minimapRenderer) m_minimapRenderer->SetSize(size);
    }

         /// Set minimap position (0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight)
        void SetMinimapPosition(int position) {
            if (m_minimapRenderer) {
                MinimapPosition pos = static_cast<MinimapPosition>(position);
                m_minimapRenderer->SetPosition(pos);
            }
        }

        /// Phase 64: Toolbar integration - Grid visibility
        void SetGridVisible(bool visible) { m_gridVisible = visible; }

        /// Phase 64: Toolbar integration - Reset pan/zoom
        void ResetPanZoom() {
            m_canvasOffset = ImVec2(0.0f, 0.0f);
            m_canvasZoom = 1.0f;
        }

        /// Phase 64: Get pan offset for canvas positioning
        ImVec2 GetCanvasOffset() const { return m_canvasOffset; }

        /// Phase 64: Get zoom level
        float GetCanvasZoom() const { return m_canvasZoom; }

        /// Phase 68 NEW: Accept drag-drop of node type at screen position
        /// Called from renderer's drag-drop overlay
        void AcceptNodeDropAtScreenPosition(PlaceholderNodeType nodeType, float screenX, float screenY);

        /// Phase 68 FIX: Accept drag-drop with pre-calculated canvas position
        /// Called from renderer's drag-drop overlay with canvas screen region info
        void AcceptNodeDropAtCanvasPosition(PlaceholderNodeType nodeType, ImVec2 screenPos, ImVec2 canvasScreenMin, float canvasZoom);

    private:
        PlaceholderGraphDocument* m_document;
        PlaceholderGraphRenderer* m_renderer;  // Phase 63.2: Reference to update base class selection
        ImVec2 m_canvasOffset;      // Pan offset
        float m_canvasZoom;         // Zoom level (0.1x - 3.0x)
        int m_selectedNodeId;       // Currently selected node (-1 for none)
         bool m_isDraggingNode;      // Currently dragging a node
             bool m_gridVisible = true;  // Phase 64: Grid visibility toggle

         // Phase 64.4 STEP 6: Multi-node drag tracking
          // Maps nodeID → (startX, startY) position when drag begins
          // Allows all selected nodes to move together with identical delta
          std::unordered_map<int, std::pair<float, float>> m_nodeDragStartPositions;

         // Phase 66: Accumulated drag delta (for proper multi-node drag)
         ImVec2 m_accumulatedDragDelta;

        // Phase 64: Connection drag-drop tracking
    bool m_isDraggingConnection;  // Currently dragging from a port
    int m_dragConnectionFromNodeId;  // Source node for connection drag
    ImVec2 m_dragConnectionPreviewEnd;  // End position for preview line

    // Feature #2: Rectangle selection (Phase 4 Step 5)
    bool m_isSelectingRectangle;    // Currently drawing selection rectangle
    ImVec2 m_selectionRectStart;    // Rectangle start point
    ImVec2 m_selectionRectEnd;      // Rectangle end point

    // Phase 52+: Minimap rendering support
    std::unique_ptr<CanvasMinimapRenderer> m_minimapRenderer;

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
    void HandlePanZoomInput();
    void HandleNodeInteraction();
    void SelectNodesInRectangle();    // Feature #2: AABB intersection test
    void HandleDragDropInput();       // Phase 64.1: Drag-drop node creation

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
