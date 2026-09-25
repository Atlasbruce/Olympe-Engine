#include "PlaceholderCanvas.h"
#include "../Utilities/CustomCanvasEditor.h"
#include "../Utilities/CanvasBezier.h"
#include "../Utilities/CanvasHitTesting.h"
#include "PlaceholderGraphDocument.h"
#include "PlaceholderGraphRenderer.h"  // Phase 63.2: For updating selection in base class
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"  // For SYSTEM_LOG
#include <iostream>
#include <cmath>

// Fix Windows min/max macro conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef min
#undef max

#include <algorithm>  // For std::find
#include <limits>     // For std::numeric_limits

namespace Olympe {

PlaceholderCanvas::PlaceholderCanvas()
    : m_document(nullptr),
      m_renderer(nullptr),  // Phase 63.2: Initialize renderer reference
      m_canvasEditor(std::make_unique<CustomCanvasEditor>("PlaceholderCanvasView", ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f), 1.0f, 0.1f, 3.0f)),
      m_isDraggingNode(false),
      m_isSelectingRectangle(false),
      m_addToRectangleSelection(false),
      m_selectionRectStart(ImVec2(0.0f, 0.0f)),
      m_selectionRectEnd(ImVec2(0.0f, 0.0f)),
      m_hoveredNodeId(-1),            // Phase 76: No hovered node initially
      m_hoveredConnectionId(-1)       // Phase 76: No hovered connection initially
{
}

PlaceholderCanvas::~PlaceholderCanvas()
{
}

void PlaceholderCanvas::Initialize(PlaceholderGraphDocument* document)
{
    m_document = document;
    std::cout << "[PlaceholderCanvas] Initialized with document. "
              << "Node count: " << m_document->GetAllNodes().size() << std::endl;
}

void PlaceholderCanvas::Render()
{
    if (!m_document) {
        ImGui::Text("No document loaded");
        return;
    }

    const ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    m_canvasEditor->SetCanvasScreenPos(canvasScreenPos);
    m_canvasEditor->SetCanvasSize(canvasSize);

    // ICanvasEditor is the single authority for pan, zoom and transforms.
    m_canvasEditor->BeginRender();
    HandleNodeInteraction();

        // Rendering in order: grid → connections → nodes → selection rectangle → preview line → context menu → minimap
        RenderGrid();
        RenderConnections();
        RenderNodes();
        RenderSelectionRectangle();  // Feature #2: Selection rectangle on top
        RenderConnectionPreviewLine(); // Phase 64.2: Connection drag preview
        RenderContextMenu();
        RenderMinimap();             // Phase 52+: Minimap overlay on top
        m_canvasEditor->EndRender();
    }

void PlaceholderCanvas::RenderGrid()
{
    if (!m_canvasEditor)
        return;

    CanvasGridRenderer::GridConfig config = m_canvasEditor->GetGridConfig();

    // CanvasGridRenderer computes its phase from canvasPos + offset. Preserve
    // Placeholder's canvas-relative grid origin without changing other users.
    config.offsetX -= config.canvasPos.x;
    config.offsetY -= config.canvasPos.y;

    // The canvas background remains visible when the grid is disabled.
    // CanvasGridRenderer stays the sole owner of both background and line drawing.
    if (!m_canvasEditor->IsGridVisible())
    {
        config.majorLineColor.w = 0.0f;
        config.minorLineColor.w = 0.0f;
    }

    CanvasGridRenderer::RenderGrid(config);
}

void PlaceholderCanvas::RenderNodes()
{
    if (!m_document || !m_renderer) return;

    const auto& nodes = m_document->GetAllNodes();

    for (const auto& node : nodes) {
        const bool isSelected = m_renderer->IsNodeSelected(node.nodeId);
        RenderNodeBox(node, isSelected);
    }
}

void PlaceholderCanvas::RenderConnections()
{
    if (!m_document) return;

    const auto& connections = m_document->GetAllConnections();
    for (size_t index = 0; index < connections.size(); ++index) {
        RenderConnectionLine(connections[index], static_cast<int>(index));
    }
}

void PlaceholderCanvas::RenderNodeBox(const PlaceholderNode& node, bool isSelected)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    // Transform node canvas coordinates to screen coordinates
    ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node.posX, node.posY));
    const float canvasZoom = GetCanvasZoom();
    ImVec2 nodeSize(node.width * canvasZoom, node.height * canvasZoom);
    ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + nodeSize.x, nodeScreenPos.y + nodeSize.y);

    // Node color based on type
    ImU32 nodeColor = GetNodeColorForType(node.type);

    // Phase 76: Hover detection affects border
    bool isHovered = (m_hoveredNodeId == node.nodeId);
    ImU32 borderColor = isSelected ? IM_COL32(0, 255, 255, 255) : (isHovered ? IM_COL32(255, 200, 0, 255) : IM_COL32(200, 200, 200, 255));
    float borderWidth = isSelected ? 3.0f : (isHovered ? 2.5f : 1.5f);

    // Phase 63.1: Draw blue glow on selection (subtle shadow effect)
    if (isSelected) {
        drawList->AddRect(
            ImVec2(nodeScreenPos.x - 3.0f, nodeScreenPos.y - 3.0f),
            ImVec2(nodeScreenEnd.x + 3.0f, nodeScreenEnd.y + 3.0f),
            IM_COL32(0, 200, 255, 100),  // Light cyan glow
            4.0f,
            ImDrawFlags_RoundCornersAll,
            1.0f
        );
    }

    // Phase 76: Draw yellow glow on hover
    if (isHovered && !isSelected) {
        drawList->AddRect(
            ImVec2(nodeScreenPos.x - 2.0f, nodeScreenPos.y - 2.0f),
            ImVec2(nodeScreenEnd.x + 2.0f, nodeScreenEnd.y + 2.0f),
            IM_COL32(255, 200, 0, 150),  // Yellow hover glow
            4.0f,
            ImDrawFlags_RoundCornersAll,
            1.0f
        );
    }

    // Draw node box
    drawList->AddRectFilled(nodeScreenPos, nodeScreenEnd, nodeColor, 4.0f);
    drawList->AddRect(nodeScreenPos, nodeScreenEnd, borderColor, 4.0f, ImDrawFlags_RoundCornersAll, borderWidth);

    // Draw title text
    drawList->AddText(
        ImVec2(nodeScreenPos.x + 8.0f, nodeScreenPos.y + 8.0f),
        IM_COL32(255, 255, 255, 255),
        node.title.c_str()
    );

    // Draw node ID as small label
    std::string idLabel = "ID:" + std::to_string(node.nodeId);
    drawList->AddText(
        ImVec2(nodeScreenPos.x + 8.0f, nodeScreenEnd.y - 18.0f),
        IM_COL32(200, 200, 200, 255),
        idLabel.c_str()
    );

    // Phase 64.2: Draw input/output ports (for connection UI)
    const float portRadius = 5.0f * canvasZoom;
    const ImU32 portColor = IM_COL32(255, 255, 0, 255);  // Yellow

    // Input port (left side, middle)
    ImVec2 inputPortPos = ImVec2(nodeScreenPos.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
    drawList->AddCircleFilled(inputPortPos, portRadius, portColor);

    // Output port (right side, middle)
    ImVec2 outputPortPos = ImVec2(nodeScreenEnd.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
    drawList->AddCircleFilled(outputPortPos, portRadius, portColor);
}

void PlaceholderCanvas::RenderConnectionLine(const PlaceholderConnection& conn, int connectionIndex)
{
    if (!m_document) return;

    PlaceholderNode* fromNode = m_document->GetNode(conn.fromNodeId);
    PlaceholderNode* toNode = m_document->GetNode(conn.toNodeId);

    if (!fromNode || !toNode) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Calculate connection points (right side of from-node, left side of to-node)
    ImVec2 fromPos = CanvasToScreen(ImVec2(fromNode->posX + fromNode->width, fromNode->posY + fromNode->height / 2.0f));
    ImVec2 toPos = CanvasToScreen(ImVec2(toNode->posX, toNode->posY + toNode->height / 2.0f));

    const CanvasBezier::CubicCurve curve = CanvasBezier::MakeHorizontalCurve(fromPos, toPos);

    // Phase 76: Check if this connection is hovered (using index in connections array)
    const bool isHovered = (m_hoveredConnectionId == connectionIndex);
    ImU32 lineColor = isHovered ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 0, 255);  // Yellow always
    float lineWidth = isHovered ? 3.0f : 2.0f;

    CanvasBezier::Render(drawList, curve, lineColor, lineWidth);
}

ImU32 PlaceholderCanvas::GetNodeColorForType(PlaceholderNodeType type)
{
    switch (type) {
        case PlaceholderNodeType::Blue:    return IM_COL32(100, 150, 255, 255);  // Light blue
        case PlaceholderNodeType::Green:   return IM_COL32(100, 255, 150, 255);  // Light green
        case PlaceholderNodeType::Magenta: return IM_COL32(255, 100, 200, 255);  // Light magenta
        default:                           return IM_COL32(128, 128, 128, 255);  // Gray
    }
}

void PlaceholderCanvas::HandleNodeInteraction()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 mousePos = io.MousePos;

    // Check if mouse is over canvas
    bool isMouseOverCanvas = (mousePos.x >= canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
                              mousePos.y >= canvasPos.y && mousePos.y < canvasPos.y + canvasSize.y);

    if (!isMouseOverCanvas) {
        const bool activeInteraction = ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            (m_isDraggingNode || m_isSelectingRectangle || m_linkDrag.IsActive());
        if (m_canvasEditor)
            m_canvasEditor->UpdateAutoPanning(activeInteraction);

        // Reset hover state when mouse leaves canvas
        m_hoveredNodeId = -1;
        m_hoveredConnectionId = -1;
        if (m_linkDrag.IsActive() && !ImGui::IsMouseDown(0)) {
            m_linkDrag.Cancel();
        }
        return;
    }

    // Phase 76: Update hover state (detect which node/connection is under mouse)
    m_hoveredNodeId = GetNodeAtScreenPos(mousePos);
    m_hoveredConnectionId = GetConnectionAtScreenPos(mousePos);

    // Route the right-click to the single highest-priority canvas target.
    if (ImGui::IsMouseClicked(1)) {  // Right mouse button
        m_contextMenu.OpenForHitTest(m_hoveredNodeId, m_hoveredConnectionId);
    }

    // Phase 63.1 FIX: Get keyboard modifiers for multi-select
    bool ctrlPressed = io.KeyCtrl;

    // Feature #2: Rectangle selection detection
    // Start rectangle selection on left-click in empty space
    if (ImGui::IsMouseClicked(0)) {  // Left mouse button pressed
        int nodeAtPos = GetNodeAtScreenPos(mousePos);

        // Phase 64.2: Check if clicking on a port for connection drag
        if (nodeAtPos >= 0) {
            PlaceholderNode* node = m_document->GetNode(nodeAtPos);
            if (node) {
                ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node->posX, node->posY));
                const float canvasZoom = GetCanvasZoom();
                ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + node->width * canvasZoom,
                                              nodeScreenPos.y + node->height * canvasZoom);

                const float portRadius = 5.0f * canvasZoom;

                // Output port (right side)
                ImVec2 outputPortPos = ImVec2(nodeScreenEnd.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);

                // If close to output port, start connection drag
                if (CanvasHitTesting::ContainsPointInCircle(mousePos, outputPortPos, portRadius)) {
                    m_linkDrag.Begin(nodeAtPos, mousePos);
                    std::cout << "[PlaceholderCanvas] Started connection drag from node " << nodeAtPos << "\n";
                    return;
                }
            }
        }

        if (nodeAtPos < 0) {  // Empty space, not on a node
            // Phase 63.1: Clear selection on empty space click (unless Ctrl held for multi-select)
            if (!ctrlPressed) {
                m_renderer->DeselectAll();
            }
            m_isSelectingRectangle = true;
            m_addToRectangleSelection = ctrlPressed;
            m_selectionRectStart = mousePos;
            m_selectionRectEnd = mousePos;
            } else {
               // Phase 63.1: Handle multi-select with Ctrl+Click
               m_isSelectingRectangle = false;
               if (ctrlPressed) {
                   m_renderer->SelectMultipleNodes(nodeAtPos, true, false);
                   } else {
                       // Single select on regular click - Phase 67 FIX: Preserve multi-selection on drag
                       // Only reset selection if clicking on a NON-selected node
                       std::vector<int> currentSelection = m_renderer->GetSelectedNodeIds();
                       auto it = std::find(currentSelection.begin(), currentSelection.end(), nodeAtPos);

                       if (it == currentSelection.end()) {
                           // Node not in selection - replace selection with this node
                           m_renderer->SelectMultipleNodes(nodeAtPos, false, false);
                           std::cout << "[PlaceholderCanvas] Selection changed to nodeId: " << nodeAtPos << std::endl;
                       } else {
                           // Node already selected - keep multi-selection for group drag.
                           std::cout << "[PlaceholderCanvas] Node " << nodeAtPos << " already selected, preserving multi-selection for drag" << std::endl;
                       }
                   }
                   m_isDraggingNode = true;
        }
    }

    // Continue rectangle selection or node dragging
    if (ImGui::IsMouseDown(0)) {  // Left mouse button held
        if (m_isSelectingRectangle) {
            // Update rectangle end point
            m_selectionRectEnd = mousePos;
        } else if (m_linkDrag.IsActive()) {
            m_linkDrag.UpdatePreviewEnd(mousePos);
        } else if (m_isDraggingNode) {
            if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
                m_renderer->ApplyNodeDragDelta(io.MouseDelta, GetCanvasZoom());
            }
        }
    } else {
        // Mouse released
        if (m_isSelectingRectangle) {
            // Perform AABB intersection test to select nodes
            SelectNodesInRectangle();
            m_isSelectingRectangle = false;
            return;  // PHASE 64: CRITICAL FIX - Prevent single-click logic from overwriting rectangle selection
        }

        // Phase 64.2: Handle connection drag release
        if (m_linkDrag.IsActive()) {
            const int sourceNodeId = m_linkDrag.Complete();
            // Check if releasing on an input port of another node
            int nodeAtMouse = GetNodeAtScreenPos(mousePos);
            if (nodeAtMouse >= 0 && nodeAtMouse != sourceNodeId) {
                PlaceholderNode* targetNode = m_document->GetNode(nodeAtMouse);
                if (targetNode) {
                    ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(targetNode->posX, targetNode->posY));
                    const float canvasZoom = GetCanvasZoom();
                    ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + targetNode->width * canvasZoom,
                                                  nodeScreenPos.y + targetNode->height * canvasZoom);

                    const float portRadius = 5.0f * canvasZoom;
                    ImVec2 inputPortPos = ImVec2(nodeScreenPos.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
                    // If close to input port, create connection
                    if (CanvasHitTesting::ContainsPointInCircle(mousePos, inputPortPos, portRadius)) {
                        HandleConnectionCreated(sourceNodeId, nodeAtMouse);
                    }
                }
            }
        }
        m_isDraggingNode = false;
    }

    // Right mouse button: context menu (handled by RenderContextMenu)
}

void PlaceholderCanvas::RenderContextMenu()
{
    if (!m_contextMenu.Begin()) {
        return;
    }

    const CanvasContextTargetType targetType = m_contextMenu.GetTargetType();
    const int targetId = m_contextMenu.GetTargetId();

    if (targetType == CanvasContextTargetType::Node) {
        if (targetId >= 0 && m_document) {
            if (ImGui::MenuItem("Delete Node")) {
                const int deletedNodeId = targetId;
                if (m_renderer->DeleteNode(deletedNodeId)) {
                    if (m_renderer->IsNodeSelected(deletedNodeId))
                        m_renderer->SelectMultipleNodes(deletedNodeId, true, false);
                    m_contextMenu.Clear();
                    std::cout << "[PlaceholderCanvas] Node " << deletedNodeId << " deleted via context menu\n";
                }
            }
            if (ImGui::MenuItem("Properties")) {
                std::cout << "[PlaceholderCanvas] Properties selected for node " << targetId << "\n";
            }
        }
    } else if (targetType == CanvasContextTargetType::Link) {
        if (targetId >= 0 && m_document) {
            if (ImGui::MenuItem("Delete Connection")) {
                const auto& connections = m_document->GetAllConnections();
                if (targetId < static_cast<int>(connections.size())) {
                    const PlaceholderConnection& conn = connections[targetId];
                    if (m_renderer->DeleteConnection(
                            conn.fromNodeId, conn.toNodeId, conn.fromPortIndex, conn.toPortIndex)) {
                        m_contextMenu.Clear();
                        std::cout << "[PlaceholderCanvas] Connection deleted via context menu\n";
                    }
                }
            }
        }
    } else if (targetType == CanvasContextTargetType::Canvas) {
        if (ImGui::MenuItem("Select All")) {
            m_renderer->SelectAll();
            std::cout << "[PlaceholderCanvas] Selected all nodes\n";
        }
        if (ImGui::MenuItem("Reset View")) {
            // Reset pan and zoom
            ResetPanZoom();
            std::cout << "[PlaceholderCanvas] View reset\n";
        }
    }

    ImGui::EndPopup();
}

int PlaceholderCanvas::GetNodeAtScreenPos(const ImVec2& screen)
{
    if (!m_document) return -1;

    const auto& nodes = m_document->GetAllNodes();
    for (const auto& node : nodes) {
        if (IsPointInNodeBounds(node.nodeId, screen)) {
            return node.nodeId;
        }
    }
    return -1;
}

bool PlaceholderCanvas::IsPointInNodeBounds(int nodeId, const ImVec2& screen)
{
    PlaceholderNode* node = m_document->GetNode(nodeId);
    if (!node) return false;

    ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node->posX, node->posY));
    const float canvasZoom = GetCanvasZoom();
    ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + node->width * canvasZoom,
                                  nodeScreenPos.y + node->height * canvasZoom);

    return CanvasHitTesting::ContainsPoint(screen, nodeScreenPos, nodeScreenEnd);
}

// Phase 76: Connection hit detection
int PlaceholderCanvas::GetConnectionAtScreenPos(const ImVec2& screen)
{
    if (!m_document) return -1;

    const auto& connections = m_document->GetAllConnections();
    const float tolerance = 10.0f;  // 10 pixel tolerance for click detection

    for (size_t i = 0; i < connections.size(); ++i) {
        float distance = GetDistanceToConnection(connections[i], screen);
        if (distance <= tolerance) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

float PlaceholderCanvas::GetDistanceToConnection(const PlaceholderConnection& conn, const ImVec2& screenPos)
{
    PlaceholderNode* fromNode = m_document->GetNode(conn.fromNodeId);
    PlaceholderNode* toNode = m_document->GetNode(conn.toNodeId);

    if (!fromNode || !toNode) return std::numeric_limits<float>::max();

    // Calculate connection Bezier curve start and end
    ImVec2 fromPos = CanvasToScreen(ImVec2(fromNode->posX + fromNode->width, fromNode->posY + fromNode->height / 2.0f));
    ImVec2 toPos = CanvasToScreen(ImVec2(toNode->posX, toNode->posY + toNode->height / 2.0f));

    return CanvasBezier::DistanceToCurve(
        CanvasBezier::MakeHorizontalCurve(fromPos, toPos), screenPos);
}

ImVec2 PlaceholderCanvas::ScreenToCanvas(const ImVec2& screen)
{
    return m_canvasEditor->ScreenToCanvas(screen);
}

ImVec2 PlaceholderCanvas::CanvasToScreen(const ImVec2& canvas)
{
    return m_canvasEditor->CanvasToScreen(canvas);
}

void PlaceholderCanvas::RenderSelectionRectangle()
{
    // Feature #2: Render blue selection rectangle overlay
    if (!m_isSelectingRectangle) return;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;

    // Calculate rectangle bounds
    ImVec2 minPos(std::min(m_selectionRectStart.x, m_selectionRectEnd.x),
                  std::min(m_selectionRectStart.y, m_selectionRectEnd.y));
    ImVec2 maxPos(std::max(m_selectionRectStart.x, m_selectionRectEnd.x),
                  std::max(m_selectionRectStart.y, m_selectionRectEnd.y));

    // Draw filled rectangle with semi-transparent blue
    drawList->AddRectFilled(minPos, maxPos, IM_COL32(100, 150, 255, 64));

    // Draw outline rectangle in bright blue
    drawList->AddRect(minPos, maxPos, IM_COL32(100, 150, 255, 255), 0.0f, 15, 2.0f);
}

// Phase 64.1: Create node when dropped from palette
void PlaceholderCanvas::HandleNodeCreatedFromPalette(PlaceholderNodeType type, const ImVec2& dropPos)
{
    if (!m_document) return;

    // Create node at drop position with default size
    std::string title = "";
    switch (type) {
        case PlaceholderNodeType::Blue:    title = "Blue Node"; break;
        case PlaceholderNodeType::Green:   title = "Green Node"; break;
        case PlaceholderNodeType::Magenta: title = "Magenta Node"; break;
    }

    const int nodeId = m_renderer
        ? m_renderer->CreateNodeFromPalette(type, title, dropPos.x, dropPos.y)
        : -1;
    if (nodeId >= 0) {
        // Select the newly created node
        if (m_renderer)
            m_renderer->SelectMultipleNodes(nodeId, false, false);
        m_document->OnDocumentModified();
        std::cout << "[Phase 64.1] Created node " << nodeId << " at position (" 
                  << dropPos.x << ", " << dropPos.y << ")\n";
    }
}

// Phase 64.2: Create connection between two nodes
void PlaceholderCanvas::HandleConnectionCreated(int fromNodeId, int toNodeId)
{
    if (!m_document) return;

    // Prevent self-connections
    if (fromNodeId == toNodeId) {
        std::cout << "[Phase 64.2] Blocked self-connection on node " << fromNodeId << "\n";
        return;
    }

    // Create connection
    if (m_renderer && m_renderer->CreateConnection(fromNodeId, toNodeId, 0, 0)) {
        std::cout << "[Phase 64.2] Created connection from node " << fromNodeId 
                  << " to node " << toNodeId << "\n";
    } else {
        std::cout << "[Phase 64.2] Failed to create connection (duplicate?)\n";
    }
}

// Phase 64.2: Render connection preview line during drag
void PlaceholderCanvas::RenderConnectionPreviewLine()
{
    // Only show preview if dragging from a port
    if (!m_linkDrag.IsActive()) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Get source node
    PlaceholderNode* fromNode = m_document->GetNode(m_linkDrag.GetSourceNodeId());
    if (!fromNode) return;

    // Calculate connection points
    ImVec2 fromPos = CanvasToScreen(ImVec2(fromNode->posX + fromNode->width, fromNode->posY + fromNode->height / 2.0f));
    const ImVec2 toPos = m_linkDrag.GetPreviewEnd();

    CanvasBezier::Render(
        drawList,
        CanvasBezier::MakeHorizontalCurve(fromPos, toPos),
        IM_COL32(255, 255, 0, 255),
        2.0f);
}

void PlaceholderCanvas::SelectNodesInRectangle()
{
    if (!m_document || !m_renderer) return;

    std::vector<GraphNodeScreenBounds> nodeBounds;
    const auto& nodes = m_document->GetAllNodes();
    nodeBounds.reserve(nodes.size());

    for (const auto& node : nodes) {
        const ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node.posX, node.posY));
        const float canvasZoom = GetCanvasZoom();
        const ImVec2 nodeScreenEnd(
            nodeScreenPos.x + node.width * canvasZoom,
            nodeScreenPos.y + node.height * canvasZoom);
        nodeBounds.push_back({ node.nodeId, nodeScreenPos, nodeScreenEnd });
    }

    m_renderer->ApplyRectangleSelection(
        m_selectionRectStart,
        m_selectionRectEnd,
        nodeBounds,
        m_addToRectangleSelection);
    std::cout << "[PlaceholderCanvas] Rectangle selection: "
              << m_renderer->GetSelectedNodeCount() << " nodes selected\n";
}

void PlaceholderCanvas::RenderMinimap()
{
    if (!m_canvasEditor || !m_canvasEditor->IsMinimapVisible())
        return;

    // Update minimap data from current canvas state
    const auto& nodes = m_document->GetAllNodes();

    // Calculate bounds of all nodes
    ImVec2 minBounds(1e9f, 1e9f), maxBounds(-1e9f, -1e9f);
    for (const auto& node : nodes) {
        minBounds.x = std::min(minBounds.x, node.posX);
        minBounds.y = std::min(minBounds.y, node.posY);
        maxBounds.x = std::max(maxBounds.x, node.posX + node.width);
        maxBounds.y = std::max(maxBounds.y, node.posY + node.height);
    }

    // If no nodes, set reasonable default
    if (minBounds.x >= maxBounds.x) {
        minBounds = ImVec2(-500.0f, -500.0f);
        maxBounds = ImVec2(500.0f, 500.0f);
    }

    // Convert PlaceholderNode vector to minimap's tuple format
    std::vector<std::tuple<int, float, float, float, float>> minimapNodes;
    for (const auto& node : nodes) {
        minimapNodes.push_back(std::make_tuple(node.nodeId, node.posX, node.posY, static_cast<float>(node.width), static_cast<float>(node.height)));
    }

    m_canvasEditor->UpdateMinimapNodes(
        minimapNodes,
        minBounds.x, maxBounds.x, minBounds.y, maxBounds.y);

    // ICanvasEditor owns the canvas origin, size and view transform, so it is
    // also the single authority for the visible canvas-space bounds.
    ImVec2 viewportMin;
    ImVec2 viewportMax;
    m_canvasEditor->GetCanvasVisibleBounds(viewportMin, viewportMax);

    m_canvasEditor->UpdateMinimapViewport(
        viewportMin.x, viewportMax.x, viewportMin.y, viewportMax.y,
        minBounds.x, maxBounds.x, minBounds.y, maxBounds.y);

    m_canvasEditor->RenderMinimap();
}

// Phase 68 NEW: Accept drag-drop node creation from palette overlay
void PlaceholderCanvas::AcceptNodeDropAtScreenPosition(PlaceholderNodeType nodeType, float screenX, float screenY)
{
    if (!m_document) return;

    // Convert screen position to canvas position
    ImVec2 screenPos(screenX, screenY);
    ImVec2 canvasPos = ScreenToCanvas(screenPos);

    // Create node at drop position
    HandleNodeCreatedFromPalette(nodeType, canvasPos);
}

} // namespace Olympe
