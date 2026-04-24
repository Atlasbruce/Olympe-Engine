#include "PlaceholderCanvas.h"
#include "PlaceholderGraphDocument.h"
#include "PlaceholderGraphRenderer.h"  // Phase 63.2: For updating selection in base class
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"  // For SYSTEM_LOG
#include <iostream>
#include <cmath>
#include <algorithm>  // For std::find

namespace Olympe {

PlaceholderCanvas::PlaceholderCanvas()
    : m_document(nullptr),
      m_renderer(nullptr),  // Phase 63.2: Initialize renderer reference
      m_canvasOffset(ImVec2(0.0f, 0.0f)),
      m_canvasZoom(1.0f),
      m_selectedNodeId(-1),
      m_isDraggingNode(false),
      m_isDraggingConnection(false),  // Phase 64: Connection drag tracking
      m_dragConnectionFromNodeId(-1),
      m_dragConnectionPreviewEnd(ImVec2(0.0f, 0.0f)),
      m_isSelectingRectangle(false),
      m_selectionRectStart(ImVec2(0.0f, 0.0f)),
      m_selectionRectEnd(ImVec2(0.0f, 0.0f)),
      m_minimapRenderer(std::make_unique<CanvasMinimapRenderer>())
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

    // Input handling (pan, zoom, selection)
    HandlePanZoomInput();
    HandleNodeInteraction();
    HandleDragDropInput();  // Phase 64.1: Drag-drop node creation and connections

        // Rendering in order: grid → connections → nodes → selection rectangle → preview line → context menu → minimap
        RenderGrid();
        RenderConnections();
        RenderNodes();
        RenderSelectionRectangle();  // Feature #2: Selection rectangle on top
        RenderConnectionPreviewLine(); // Phase 64.2: Connection drag preview
        RenderContextMenu();
        RenderMinimap();             // Phase 52+: Minimap overlay on top
    }

void PlaceholderCanvas::RenderGrid()
{
    // Phase 64: Check grid visibility before rendering
    if (!m_gridVisible) {
        // Draw background only, no grid lines
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        const ImU32 bgColor = IM_COL32(38, 38, 47, 255);        // #26262FFF
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bgColor);
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    // Grid parameters (Phase 5 standardization)
    const float gridSpacing = 24.0f * m_canvasZoom;
    const ImU32 gridColor = IM_COL32(63, 63, 71, 255);      // #3F3F47FF
    const ImU32 bgColor = IM_COL32(38, 38, 47, 255);        // #26262FFF

    // Draw background
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bgColor);

    // Draw grid
    float startX = fmodf(m_canvasOffset.x * m_canvasZoom, gridSpacing);
    float startY = fmodf(m_canvasOffset.y * m_canvasZoom, gridSpacing);

    for (float x = startX; x < canvasSize.x; x += gridSpacing) {
        drawList->AddLine(
            ImVec2(canvasPos.x + x, canvasPos.y),
            ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
            gridColor
        );
    }

    for (float y = startY; y < canvasSize.y; y += gridSpacing) {
        drawList->AddLine(
            ImVec2(canvasPos.x, canvasPos.y + y),
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
            gridColor
        );
    }
}

void PlaceholderCanvas::RenderNodes()
{
    if (!m_document || !m_renderer) return;

    const auto& nodes = m_document->GetAllNodes();

    for (const auto& node : nodes) {
        // Phase 64: Check both single selection AND base class multi-select vector
        bool isSelected = (node.nodeId == m_selectedNodeId) || m_renderer->IsNodeSelected(node.nodeId);
        RenderNodeBox(node, isSelected);
    }
}

void PlaceholderCanvas::RenderConnections()
{
    if (!m_document) return;

    const auto& connections = m_document->GetAllConnections();
    for (const auto& conn : connections) {
        RenderConnectionLine(conn);
    }
}

void PlaceholderCanvas::RenderNodeBox(const PlaceholderNode& node, bool isSelected)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    // Transform node canvas coordinates to screen coordinates
    ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node.posX, node.posY));
    ImVec2 nodeSize(node.width * m_canvasZoom, node.height * m_canvasZoom);
    ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + nodeSize.x, nodeScreenPos.y + nodeSize.y);

    // Node color based on type
    ImU32 nodeColor = GetNodeColorForType(node.type);
    ImU32 borderColor = isSelected ? IM_COL32(0, 255, 255, 255) : IM_COL32(200, 200, 200, 255);
    float borderWidth = isSelected ? 3.0f : 1.5f;

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
    const float portRadius = 5.0f * m_canvasZoom;
    const ImU32 portColor = IM_COL32(255, 255, 0, 255);  // Yellow

    // Input port (left side, middle)
    ImVec2 inputPortPos = ImVec2(nodeScreenPos.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
    drawList->AddCircleFilled(inputPortPos, portRadius, portColor);

    // Output port (right side, middle)
    ImVec2 outputPortPos = ImVec2(nodeScreenEnd.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
    drawList->AddCircleFilled(outputPortPos, portRadius, portColor);
}

void PlaceholderCanvas::RenderConnectionLine(const PlaceholderConnection& conn)
{
    if (!m_document) return;

    PlaceholderNode* fromNode = m_document->GetNode(conn.fromNodeId);
    PlaceholderNode* toNode = m_document->GetNode(conn.toNodeId);

    if (!fromNode || !toNode) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Calculate connection points (right side of from-node, left side of to-node)
    ImVec2 fromPos = CanvasToScreen(ImVec2(fromNode->posX + fromNode->width, fromNode->posY + fromNode->height / 2.0f));
    ImVec2 toPos = CanvasToScreen(ImVec2(toNode->posX, toNode->posY + toNode->height / 2.0f));

    // Bezier control points (40% of horizontal distance)
    float controlPointOffset = (toPos.x - fromPos.x) * 0.4f;
    ImVec2 cp1(fromPos.x + controlPointOffset, fromPos.y);
    ImVec2 cp2(toPos.x - controlPointOffset, toPos.y);

    // Draw Bezier curve using path rendering
    drawList->PathLineTo(fromPos);
    drawList->PathBezierCubicCurveTo(cp1, cp2, toPos, 32);
    drawList->PathStroke(IM_COL32(255, 255, 0, 255), false, 2.0f);
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

void PlaceholderCanvas::HandlePanZoomInput()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 mousePos = io.MousePos;

    // Check if mouse is over canvas
    bool isMouseOverCanvas = (mousePos.x >= canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
                              mousePos.y >= canvasPos.y && mousePos.y < canvasPos.y + canvasSize.y);

    if (!isMouseOverCanvas) return;

    // Pan with middle mouse button
    if (ImGui::IsMouseDown(2)) {  // Middle mouse button
        ImVec2 delta = io.MouseDelta;
        m_canvasOffset.x += delta.x / m_canvasZoom;
        m_canvasOffset.y += delta.y / m_canvasZoom;
    }

    // Zoom with scroll wheel
    if (io.MouseWheel != 0.0f) {
        float zoomFactor = io.MouseWheel > 0.0f ? 1.1f : 0.909f;
        float oldZoom = m_canvasZoom;
        m_canvasZoom *= zoomFactor;
        // Clamp zoom to 0.1x - 3.0x range
        if (m_canvasZoom < 0.1f) m_canvasZoom = 0.1f;
        if (m_canvasZoom > 3.0f) m_canvasZoom = 3.0f;

        // Zoom centered on mouse cursor
        ImVec2 mouseCanvasBefore = ScreenToCanvas(mousePos);
        ImVec2 mouseCanvasAfter = ScreenToCanvas(mousePos);
        m_canvasOffset.x += (mouseCanvasBefore.x - mouseCanvasAfter.x);
        m_canvasOffset.y += (mouseCanvasBefore.y - mouseCanvasAfter.y);
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

    if (!isMouseOverCanvas) return;

    // Phase 63.1 FIX: Get keyboard modifiers for multi-select
    bool ctrlPressed = io.KeyCtrl;
    bool shiftPressed = io.KeyShift;

    // Feature #2: Rectangle selection detection
    // Start rectangle selection on left-click in empty space
    if (ImGui::IsMouseClicked(0)) {  // Left mouse button pressed
        int nodeAtPos = GetNodeAtScreenPos(mousePos);

        // Phase 64.2: Check if clicking on a port for connection drag
        if (nodeAtPos >= 0) {
            PlaceholderNode* node = m_document->GetNode(nodeAtPos);
            if (node) {
                ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node->posX, node->posY));
                ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + node->width * m_canvasZoom,
                                              nodeScreenPos.y + node->height * m_canvasZoom);

                const float portRadius = 5.0f * m_canvasZoom;

                // Input port (left side)
                ImVec2 inputPortPos = ImVec2(nodeScreenPos.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
                float dx = mousePos.x - inputPortPos.x;
                float dy = mousePos.y - inputPortPos.y;
                float distToInputPort = sqrt(dx * dx + dy * dy);

                // Output port (right side)
                ImVec2 outputPortPos = ImVec2(nodeScreenEnd.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
                dx = mousePos.x - outputPortPos.x;
                dy = mousePos.y - outputPortPos.y;
                float distToOutputPort = sqrt(dx * dx + dy * dy);

                // If close to output port, start connection drag
                if (distToOutputPort <= portRadius) {
                    m_isDraggingConnection = true;
                    m_dragConnectionFromNodeId = nodeAtPos;
                    m_dragConnectionPreviewEnd = mousePos;
                    std::cout << "[PlaceholderCanvas] Started connection drag from node " << nodeAtPos << "\n";
                    return;
                }
            }
        }

        if (nodeAtPos < 0) {  // Empty space, not on a node
            // Phase 63.1: Clear selection on empty space click (unless Ctrl held for multi-select)
            if (!ctrlPressed) {
                m_selectedNodeId = -1;
                m_renderer->ClearSelectedNodes();  // Sync to base class
            }
            m_isSelectingRectangle = true;
            m_selectionRectStart = mousePos;
            m_selectionRectEnd = mousePos;
            } else {
               // Phase 63.1: Handle multi-select with Ctrl+Click
               m_isSelectingRectangle = false;
               if (ctrlPressed) {
                   // Toggle selection on Ctrl+Click - Phase 65 FIX: properly sync with base class
                   std::vector<int> currentSelection = m_renderer->GetSelectedNodeIds();

                   // Check if node already in selection
                   auto it = std::find(currentSelection.begin(), currentSelection.end(), nodeAtPos);

                   if (it != currentSelection.end()) {
                       // Remove from selection
                       currentSelection.erase(it);
                   } else {
                       // Add to selection
                       currentSelection.push_back(nodeAtPos);
                   }

                   // Sync entire updated list to base class
                   m_renderer->SetSelectedNodeIds(currentSelection);
                   m_selectedNodeId = nodeAtPos;  // Keep last clicked node
                   } else {
                       // Single select on regular click - Phase 67 FIX: Preserve multi-selection on drag
                       // Only reset selection if clicking on a NON-selected node
                       std::vector<int> currentSelection = m_renderer->GetSelectedNodeIds();
                       auto it = std::find(currentSelection.begin(), currentSelection.end(), nodeAtPos);

                       if (it == currentSelection.end()) {
                           // Node not in selection - replace selection with this node
                           m_selectedNodeId = nodeAtPos;
                           m_renderer->SetSelectedNodeIds({nodeAtPos});
                           std::cout << "[PlaceholderCanvas] Selection changed to nodeId: " << nodeAtPos << std::endl;
                       } else {
                           // Node already selected - keep multi-selection, just update "last clicked" reference
                           m_selectedNodeId = nodeAtPos;
                           std::cout << "[PlaceholderCanvas] Node " << nodeAtPos << " already selected, preserving multi-selection for drag" << std::endl;
                       }
                   }
                   m_isDraggingNode = true;

                   // Phase 66 FIX: Initialize accumulated delta at start of drag
                   m_accumulatedDragDelta = ImVec2(0.0f, 0.0f);

                   // Phase 64.4 STEP 7: Record ALL selected node start positions for group drag
                   m_nodeDragStartPositions.clear();
                   std::vector<int> selectedNodeIds = m_renderer->GetSelectedNodeIds();
                   for (size_t i = 0; i < selectedNodeIds.size(); ++i) {
                       int nodeId = selectedNodeIds[i];
                       PlaceholderNode* node = m_document->GetNode(nodeId);
                       if (node) {
                           m_nodeDragStartPositions[nodeId] = std::make_pair(node->posX, node->posY);
                    }
               }
        }
    }

    // Continue rectangle selection or node dragging
    if (ImGui::IsMouseDown(0)) {  // Left mouse button held
        if (m_isSelectingRectangle) {
            // Update rectangle end point
            m_selectionRectEnd = mousePos;
        } else if (m_isDraggingConnection) {
            // Update connection preview end point
            m_dragConnectionPreviewEnd = mousePos;
        } else if (m_isDraggingNode && !m_nodeDragStartPositions.empty()) {
            // Phase 66 FIX: Properly accumulate drag delta from start position
            // Use pure mouse delta converted to canvas space
            if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
                // Convert screen mouse delta to canvas delta (account for zoom)
                ImVec2 canvasDelta = ImVec2(io.MouseDelta.x / m_canvasZoom, io.MouseDelta.y / m_canvasZoom);

                // Accumulate delta
                m_accumulatedDragDelta.x += canvasDelta.x;
                m_accumulatedDragDelta.y += canvasDelta.y;

                // Apply accumulated delta to ALL selected nodes from their START positions
                for (auto it = m_nodeDragStartPositions.begin(); it != m_nodeDragStartPositions.end(); ++it) {
                    int nodeId = it->first;
                    std::pair<float, float> startPos = it->second;

                    PlaceholderNode* node = m_document->GetNode(nodeId);
                    if (node) {
                        // Move from START position by accumulated delta
                        m_document->SetNodePosition(
                            nodeId,
                            startPos.first + m_accumulatedDragDelta.x,
                            startPos.second + m_accumulatedDragDelta.y
                        );
                    }
                }
                m_document->OnDocumentModified();
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
        if (m_isDraggingConnection) {
            // Check if releasing on an input port of another node
            int nodeAtMouse = GetNodeAtScreenPos(mousePos);
            if (nodeAtMouse >= 0 && nodeAtMouse != m_dragConnectionFromNodeId) {
                PlaceholderNode* targetNode = m_document->GetNode(nodeAtMouse);
                if (targetNode) {
                    ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(targetNode->posX, targetNode->posY));
                    ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + targetNode->width * m_canvasZoom,
                                                  nodeScreenPos.y + targetNode->height * m_canvasZoom);

                    const float portRadius = 5.0f * m_canvasZoom;
                    ImVec2 inputPortPos = ImVec2(nodeScreenPos.x, (nodeScreenPos.y + nodeScreenEnd.y) * 0.5f);
                    float dx = mousePos.x - inputPortPos.x;
                    float dy = mousePos.y - inputPortPos.y;
                    float distToInputPort = sqrt(dx * dx + dy * dy);

                    // If close to input port, create connection
                    if (distToInputPort <= portRadius) {
                        m_document->CreateConnection(m_dragConnectionFromNodeId, nodeAtMouse);
                        m_document->OnDocumentModified();
                        std::cout << "[PlaceholderCanvas] Created connection from node " << m_dragConnectionFromNodeId 
                                  << " to node " << nodeAtMouse << "\n";
                    }
                }
            }
            m_isDraggingConnection = false;
        }
        m_isDraggingNode = false;
    }

    // Right mouse button: context menu (handled by RenderContextMenu)
}

void PlaceholderCanvas::RenderContextMenu()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 mousePos = io.MousePos;

    // Check if mouse is over canvas
    bool isMouseOverCanvas = (mousePos.x >= canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
                              mousePos.y >= canvasPos.y && mousePos.y < canvasSize.y);

    if (!isMouseOverCanvas) return;

    // Check for right mouse button click
    if (ImGui::IsMouseClicked(1)) {  // Right mouse button
        // Determine if click is on a node or empty canvas
        int nodeAtClick = GetNodeAtScreenPos(mousePos);

        if (nodeAtClick >= 0) {
            // Right-click on node
            m_selectedNodeId = nodeAtClick;
            ImGui::OpenPopup("##node_context_menu");
        } else {
            // Right-click on empty canvas
            ImGui::OpenPopup("##canvas_context_menu");
        }
    }

    // Render node context menu
    if (ImGui::BeginPopup("##node_context_menu")) {
        if (ImGui::MenuItem("Delete")) {
            if (m_document && m_selectedNodeId >= 0) {
                m_document->DeleteNode(m_selectedNodeId);
                m_selectedNodeId = -1;
                std::cout << "[PlaceholderCanvas] Node deleted via context menu\n";
            }
        }
        if (ImGui::MenuItem("Properties")) {
            // Select the node for property panel to display
            if (m_selectedNodeId >= 0) {
                std::cout << "[PlaceholderCanvas] Properties selected for node " << m_selectedNodeId << "\n";
            }
        }
        ImGui::EndPopup();
    }

    // Render canvas context menu
    if (ImGui::BeginPopup("##canvas_context_menu")) {
        if (ImGui::MenuItem("Select All")) {
            // Future: select all nodes
            std::cout << "[PlaceholderCanvas] Select All clicked\n";
        }
        if (ImGui::MenuItem("Reset View")) {
            // Reset pan and zoom
            m_canvasOffset = ImVec2(0.0f, 0.0f);
            m_canvasZoom = 1.0f;
            std::cout << "[PlaceholderCanvas] View reset\n";
        }
        ImGui::EndPopup();
    }
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
    ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + node->width * m_canvasZoom,
                                  nodeScreenPos.y + node->height * m_canvasZoom);

    return (screen.x >= nodeScreenPos.x && screen.x <= nodeScreenEnd.x &&
            screen.y >= nodeScreenPos.y && screen.y <= nodeScreenEnd.y);
}

ImVec2 PlaceholderCanvas::ScreenToCanvas(const ImVec2& screen)
{
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 screenRelative(screen.x - canvasPos.x, screen.y - canvasPos.y);
    return ImVec2((screenRelative.x - m_canvasOffset.x * m_canvasZoom) / m_canvasZoom,
                   (screenRelative.y - m_canvasOffset.y * m_canvasZoom) / m_canvasZoom);
}

ImVec2 PlaceholderCanvas::CanvasToScreen(const ImVec2& canvas)
{
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    return ImVec2(canvasPos.x + (canvas.x + m_canvasOffset.x) * m_canvasZoom,
                   canvasPos.y + (canvas.y + m_canvasOffset.y) * m_canvasZoom);
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

// Phase 64.1: Handle drag-drop for node creation
void PlaceholderCanvas::HandleDragDropInput()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 mousePos = io.MousePos;

    // Check if mouse is over canvas
    bool isMouseOverCanvas = (mousePos.x >= canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
                              mousePos.y >= canvasPos.y && mousePos.y < canvasPos.y + canvasSize.y);

    if (!isMouseOverCanvas) return;

    // Phase 64.2: Handle connection drag (port-to-port)
    if (m_isDraggingConnection) {
        m_dragConnectionPreviewEnd = ImVec2(mousePos.x, mousePos.y);
    }

    // Setup drag-drop target for node creation (Phase 64: Diagnostic logging)
    if (ImGui::BeginDragDropTarget())
    {
        std::cout << "[PlaceholderCanvas::HandleDragDropInput] BeginDragDropTarget entered\n";

        // Check if payload is being delivered
        if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
        {
            std::cout << "[PlaceholderCanvas::HandleDragDropInput] Payload detected, type: " 
                      << (payload ? payload->DataType : "nullptr") << "\n";

            if (payload->IsDataType("PLACEHOLDER_NODE_TYPE"))
            {
                std::cout << "[PlaceholderCanvas] ✓ Payload hover detected, type: PLACEHOLDER_NODE_TYPE\n";

                if (ImGui::AcceptDragDropPayload("PLACEHOLDER_NODE_TYPE"))
                {
                    // Get node type from payload
                    int nodeTypeValue = *static_cast<const int*>(payload->Data);
                    PlaceholderNodeType nodeType = static_cast<PlaceholderNodeType>(nodeTypeValue);

                    // Convert screen position to canvas position
                    ImVec2 canvasDropPos = ScreenToCanvas(mousePos);

                    // Create node at drop position
                    HandleNodeCreatedFromPalette(nodeType, canvasDropPos);

                    std::cout << "[PlaceholderCanvas] ✓✓✓ DROP ACCEPTED! Node type " << nodeTypeValue 
                              << " created at canvas pos (" << canvasDropPos.x << ", " << canvasDropPos.y << ")\n";
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
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

    int nodeId = m_document->CreateNode(type, title, dropPos.x, dropPos.y);
    if (nodeId >= 0) {
        // Select the newly created node
        m_selectedNodeId = nodeId;
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
    int connId = m_document->CreateConnection(fromNodeId, toNodeId, 0, 0);
    if (connId >= 0) {
        m_document->OnDocumentModified();
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
    if (!m_isDraggingConnection || m_dragConnectionFromNodeId < 0) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Get source node
    PlaceholderNode* fromNode = m_document->GetNode(m_dragConnectionFromNodeId);
    if (!fromNode) return;

    // Calculate connection points
    ImVec2 fromPos = CanvasToScreen(ImVec2(fromNode->posX + fromNode->width, fromNode->posY + fromNode->height / 2.0f));
    ImVec2 toPos = m_dragConnectionPreviewEnd;

    // Bezier control points (40% of horizontal distance)
    float controlPointOffset = (toPos.x - fromPos.x) * 0.4f;
    ImVec2 cp1(fromPos.x + controlPointOffset, fromPos.y);
    ImVec2 cp2(toPos.x - controlPointOffset, toPos.y);

    // Draw yellow preview line (Bezier curve)
    drawList->PathLineTo(fromPos);
    drawList->PathBezierCubicCurveTo(cp1, cp2, toPos, 32);
    drawList->PathStroke(IM_COL32(255, 255, 0, 255), false, 2.0f);
}

void PlaceholderCanvas::SelectNodesInRectangle()
{
    // Feature #2: AABB intersection test to select nodes inside rectangle
    if (!m_document || !m_renderer) return;

    ImVec2 minPos(std::min(m_selectionRectStart.x, m_selectionRectEnd.x),
                  std::min(m_selectionRectStart.y, m_selectionRectEnd.y));
    ImVec2 maxPos(std::max(m_selectionRectStart.x, m_selectionRectEnd.x),
                  std::max(m_selectionRectStart.y, m_selectionRectEnd.y));

    // Phase 64: Clear base class selection and collect rectangle-selected nodes
    m_renderer->ClearSelectedNodes();  // Clear base class m_selectedNodeIds
    std::vector<int> selectedNodeIds;  // Collect nodes for this rectangle

    // Find all nodes inside rectangle (AABB intersection test)
    const auto& nodes = m_document->GetAllNodes();
    int selectedCount = 0;

    for (const auto& node : nodes) {
        ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node.posX, node.posY));
        ImVec2 nodeScreenEnd = ImVec2(nodeScreenPos.x + node.width * m_canvasZoom,
                                      nodeScreenPos.y + node.height * m_canvasZoom);

        // AABB intersection test
        if (!(nodeScreenEnd.x < minPos.x || nodeScreenPos.x > maxPos.x ||
              nodeScreenEnd.y < minPos.y || nodeScreenPos.y > maxPos.y)) {
            // Node is inside rectangle - add to both canvas and base class
            selectedNodeIds.push_back(node.nodeId);  // Collect for base class
            selectedCount++;
            std::cout << "[PlaceholderCanvas] Node " << node.nodeId << " selected via rectangle\n";
        }
    }

    // Phase 64: Sync all selected nodes to base class
    // Phase 64.3 FIX: DON'T sync to m_selectedNodeId (it's for single-node dragging only)
    // Only update the base class renderer's multi-select vector
    m_selectedNodeId = -1;  // Clear single-node selection for multi-select mode
    m_renderer->SetSelectedNodeIds(selectedNodeIds);
    std::cout << "[PlaceholderCanvas] Rectangle selection: " << selectedCount << " nodes selected\n";
}

void PlaceholderCanvas::RenderMinimap()
{
    // Phase 52+: Render minimap overlay
    if (!m_minimapRenderer) return;

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
        minimapNodes.push_back(std::make_tuple(node.nodeId, node.posX, node.posY, node.width, node.height));
    }

    // Update minimap with node data (pass bounds as separate parameters)
    m_minimapRenderer->UpdateNodes(minimapNodes, minBounds.x, maxBounds.x, minBounds.y, maxBounds.y);

    // Update minimap viewport based on current pan/zoom
    ImVec2 viewportMin = ScreenToCanvas(ImVec2(0.0f, 0.0f));
    ImVec2 viewportMax = ScreenToCanvas(ImGui::GetContentRegionAvail());

    // UpdateViewport needs 8 parameters: viewMinX, viewMaxX, viewMinY, viewMaxY, graphMinX, graphMaxX, graphMinY, graphMaxY
    m_minimapRenderer->UpdateViewport(
        viewportMin.x, viewportMax.x, viewportMin.y, viewportMax.y,
        minBounds.x, maxBounds.x, minBounds.y, maxBounds.y
    );

    // Render minimap overlay at screen coordinates
    ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();
    ImVec2 regionAvail = ImGui::GetContentRegionAvail();
    m_minimapRenderer->RenderCustom(canvasScreenPos, regionAvail);
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

// Phase 68 FIX: Accept drag-drop with canvas region info passed from overlay
void PlaceholderCanvas::AcceptNodeDropAtCanvasPosition(PlaceholderNodeType nodeType, ImVec2 screenPos, ImVec2 canvasScreenMin, float canvasZoom)
{
    if (!m_document) return;

    // Direct coordinate transformation: screen → canvas
    // Using provided canvasScreenMin and canvasZoom instead of relying on ImGui context
    ImVec2 screenRelative(screenPos.x - canvasScreenMin.x, screenPos.y - canvasScreenMin.y);
    ImVec2 canvasPos(
        (screenRelative.x - m_canvasOffset.x * canvasZoom) / canvasZoom,
        (screenRelative.y - m_canvasOffset.y * canvasZoom) / canvasZoom
    );

    // Create node at drop position
    HandleNodeCreatedFromPalette(nodeType, canvasPos);
}

} // namespace Olympe
