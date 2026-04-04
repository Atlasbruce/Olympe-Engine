#include "PrefabCanvas.h"
#include "ComponentNodeRenderer.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../../system/system_utils.h"
#include <cmath>

namespace Olympe
{
    PrefabCanvas::PrefabCanvas() : m_canvasZoom(1.0f), m_isPanning(false), m_gridSpacing(50.0f), m_showGrid(true), m_showDebugInfo(false), m_snapToGrid(true)
    { m_renderer = std::make_unique<ComponentNodeRenderer>(); }

    PrefabCanvas::~PrefabCanvas() { }

    void PrefabCanvas::Initialize(EntityPrefabGraphDocument* document) { m_document = document; if (m_renderer) { m_renderer->Initialize(); } }

    EntityPrefabGraphDocument* PrefabCanvas::GetDocument() const { return m_document; }

    void PrefabCanvas::Render()
    { 
        if (!m_document || !m_renderer) { return; }
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::BeginChild("##PrefabCanvas", ImVec2(0, 0), true);

        // Handle ImGui input events
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        m_canvasScreenPos = canvasPos;  // Store for use in drag-drop context
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImVec2 canvasEnd(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);

        ImVec2 mousePos = ImGui::GetMousePos();
        bool isMouseInCanvas = (mousePos.x >= canvasPos.x && mousePos.x <= canvasEnd.x &&
                                mousePos.y >= canvasPos.y && mousePos.y <= canvasEnd.y &&
                                ImGui::IsWindowHovered());

        // Poll and process ImGui input
        if (isMouseInCanvas)
        {
            // Mouse movement
            OnMouseMove(mousePos.x, mousePos.y);

            // Mouse button events
            if (ImGui::IsMouseClicked(0)) { OnMouseDown(0, mousePos.x, mousePos.y); }
            if (ImGui::IsMouseReleased(0)) { OnMouseUp(0, mousePos.x, mousePos.y); }
            if (ImGui::IsMouseClicked(2)) { OnMouseDown(2, mousePos.x, mousePos.y); }
            if (ImGui::IsMouseReleased(2)) { OnMouseUp(2, mousePos.x, mousePos.y); }

            // Mouse scroll
            if (ImGui::GetIO().MouseWheel != 0.0f) { OnMouseScroll(ImGui::GetIO().MouseWheel); }
        }

        // Keyboard events
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) { m_ctrlPressed = true; OnKeyDown(17); }
        else { m_ctrlPressed = false; }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_LeftShift)) { m_shiftPressed = true; OnKeyDown(16); }
        else { m_shiftPressed = false; }

        if (ImGui::IsKeyPressed(ImGuiKey_A) && m_ctrlPressed) { OnKeyDown(65); }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) { OnKeyDown(46); }

        // Render canvas content
        if (m_showGrid) { RenderGrid(); }
        RenderConnections();
        RenderNodes();
        if (m_isCreatingConnection) { RenderConnectionPreview(); }
        if (m_isSelectingRectangle) { RenderSelectionRectangle(); }
        if (m_showDebugInfo) { RenderDebugInfo(); }

        // Context menu handling (right-click)
        RenderContextMenu();
        RenderConnectionContextMenu();

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void PrefabCanvas::Update(float deltaTime) { (void)deltaTime; }

    void PrefabCanvas::OnMouseMove(float x, float y)
    { 
        m_currentMousePos = Vector(x, y, 0.0f);

        if (m_interactionMode == CanvasInteractionMode::DraggingNode)
        {
            HandleNodeDrag(x, y);
        }
        else if (m_interactionMode == CanvasInteractionMode::PanningCamera)
        {
            HandlePan(x, y);
        }
        else if (m_interactionMode == CanvasInteractionMode::CreatingConnection)
        {
            m_connectionPreviewEnd = ScreenToCanvas(x, y);
        }

        // Update rectangle selection during drag
        if (m_isSelectingRectangle)
        {
            m_selectionRectEnd = ScreenToCanvas(x, y);
        }

        // Update hovered connection for visual feedback
        if (!m_isCreatingConnection && m_interactionMode == CanvasInteractionMode::Normal && !m_isSelectingRectangle)
        {
            m_hoveredConnectionIndex = -1;

            const std::vector<std::pair<NodeId, NodeId>>& connections = m_document->GetConnections();
            const float hoverTolerance = 10.0f;  // Tolerance for hover detection (in screen pixels)

            for (size_t connIdx = 0; connIdx < connections.size(); ++connIdx)
            {
                const ComponentNode* sourceNode = m_document->GetNode(connections[connIdx].first);
                const ComponentNode* targetNode = m_document->GetNode(connections[connIdx].second);

                if (sourceNode != nullptr && targetNode != nullptr)
                {
                    // Get connection endpoints in canvas space
                    Vector from = sourceNode->position;
                    from.x += sourceNode->size.x * 0.5f;
                    Vector to = targetNode->position;
                    to.x -= targetNode->size.x * 0.5f;

                    // Get distance from mouse position to connection curve
                    float distance = m_renderer->GetDistanceToConnection(
                        Vector(x, y, 0.0f),  // Screen space position
                        from, to
                    );

                    if (distance <= hoverTolerance)
                    {
                        m_hoveredConnectionIndex = static_cast<int>(connIdx);
                        break;  // Only highlight one connection
                    }
                }
            }
        }

        m_lastMousePos = m_currentMousePos;
    }

    void PrefabCanvas::OnMouseDown(int button, float x, float y)
    { 
        if (!m_document) { return; }

        m_currentMousePos = Vector(x, y, 0.0f);
        Vector canvasPos = ScreenToCanvas(x, y);

        if (button == 0) // Left mouse button
        {
            // First check if clicking on a port
            const std::vector<ComponentNode>& nodes = m_document->GetAllNodes();
            for (size_t i = 0; i < nodes.size(); ++i)
            {
                PortId portId = InvalidPortId;
                if (m_renderer->IsPointInPort(canvasPos, nodes[i], portId))
                {
                    // Starting connection from this port
                    StartConnectionCreation(nodes[i].nodeId);
                    return;
                }
            }

            // If not on a port, check if on a node
            NodeId nodeAtPos = GetNodeAtPosition(x, y);
            if (nodeAtPos != InvalidNodeId)
            {
                if (!m_ctrlPressed)
                {
                    m_document->DeselectAll();
                }
                HandleNodeDragStart(nodeAtPos, x, y);
            }
            else if (!m_ctrlPressed)
            {
                // Empty space clicked - start rectangle selection
                m_document->DeselectAll();
                m_isSelectingRectangle = true;
                m_selectionRectStart = canvasPos;
                m_selectionRectEnd = canvasPos;
            }
            else
            {
                // Ctrl+click on empty space - start rectangle selection with additive mode
                m_isSelectingRectangle = true;
                m_selectionRectStart = canvasPos;
                m_selectionRectEnd = canvasPos;
            }
        }
        else if (button == 2) // Middle mouse button
        {
            HandlePanStart(x, y);
        }
        // Right mouse button (button==1) is handled by RenderContextMenu() for priority

        m_lastMousePos = m_currentMousePos;
    }

    void PrefabCanvas::OnMouseUp(int button, float x, float y)
    { 
        m_currentMousePos = Vector(x, y, 0.0f);

        if (button == 0)
        {
            if (m_isSelectingRectangle)
            {
                // Complete rectangle selection
                Vector canvasPos = ScreenToCanvas(x, y);
                m_selectionRectEnd = canvasPos;
                SelectNodesInRectangle(m_selectionRectStart, m_selectionRectEnd, m_ctrlPressed);
                m_isSelectingRectangle = false;
            }
            else if (m_interactionMode == CanvasInteractionMode::DraggingNode)
            {
                HandleNodeDragEnd();
            }
            else if (m_isCreatingConnection)
            {
                // Check if releasing on a port
                Vector canvasPos = ScreenToCanvas(x, y);
                const std::vector<ComponentNode>& nodes = m_document->GetAllNodes();
                bool connectionCompleted = false;

                for (size_t i = 0; i < nodes.size(); ++i)
                {
                    PortId portId = InvalidPortId;
                    if (m_renderer->IsPointInPort(canvasPos, nodes[i], portId))
                    {
                        // Check if different node
                        if (nodes[i].nodeId != m_connectionSourceNodeId)
                        {
                            CompleteConnection(nodes[i].nodeId);
                            connectionCompleted = true;
                        }
                        break;
                    }
                }

                if (!connectionCompleted)
                {
                    CancelConnectionCreation();
                }
            }
        }
        else if (button == 2 && m_interactionMode == CanvasInteractionMode::PanningCamera)
        {
            m_interactionMode = CanvasInteractionMode::Normal;
            m_isPanning = false;
        }
    }

    void PrefabCanvas::OnMouseScroll(float delta) 
    { 
        ZoomCanvas(delta * 0.1f, m_currentMousePos.x, m_currentMousePos.y); 
    }

    void PrefabCanvas::OnKeyDown(int keyCode)
    { 
        if (keyCode == 17) { m_ctrlPressed = true; } // Ctrl key
        if (keyCode == 16) { m_shiftPressed = true; } // Shift key

        if (keyCode == 65 && m_ctrlPressed) { SelectAll(); } // Ctrl+A
        if (keyCode == 46) { DeleteSelectedNodes(); } // Delete key
    }

    void PrefabCanvas::OnKeyUp(int keyCode)
    { 
        if (keyCode == 17) { m_ctrlPressed = false; } // Ctrl key
        if (keyCode == 16) { m_shiftPressed = false; } // Shift key
    }

    void PrefabCanvas::PanCanvas(float deltaX, float deltaY) { m_canvasOffset.x += deltaX; m_canvasOffset.y += deltaY; }

    void PrefabCanvas::ZoomCanvas(float zoomDelta, float centerX, float centerY)
    { 
        float oldZoom = m_canvasZoom;
        m_canvasZoom += zoomDelta;
        if (m_canvasZoom < 0.1f) { m_canvasZoom = 0.1f; }
        if (m_canvasZoom > 3.0f) { m_canvasZoom = 3.0f; }
        float zoomRatio = m_canvasZoom / oldZoom;
        m_canvasOffset.x = centerX + (m_canvasOffset.x - centerX) * zoomRatio;
        m_canvasOffset.y = centerY + (m_canvasOffset.y - centerY) * zoomRatio;
    }

    void PrefabCanvas::ResetView() { m_canvasOffset = Vector(0.0f, 0.0f, 0.0f); m_canvasZoom = 1.0f; }
    void PrefabCanvas::FitToContent() { ResetView(); }

    NodeId PrefabCanvas::GetNodeAtPosition(float x, float y)
    { 
        if (!m_document || !m_renderer) { return InvalidNodeId; }
        Vector pos = ScreenToCanvas(x, y);
        const std::vector<ComponentNode>& nodes = m_document->GetAllNodes();
        for (size_t i = 0; i < nodes.size(); ++i)
        { 
            if (m_renderer->IsPointInNode(pos, nodes[i])) 
            { 
                return nodes[i].nodeId; 
            } 
        }
        return InvalidNodeId;
    }

    void PrefabCanvas::SelectNodeAt(float x, float y, bool addToSelection)
    { 
        if (!m_document) { return; }
        NodeId nodeId = GetNodeAtPosition(x, y);
        if (nodeId != InvalidNodeId)
        {
            if (!addToSelection)
            {
                m_document->DeselectAll();
            }
            m_document->SelectNode(nodeId);
        }
    }

    void PrefabCanvas::SelectNodesInRectangle(const Vector& rectStart, const Vector& rectEnd, bool addToSelection)
    {
        if (!m_document) { return; }

        // Normalize rectangle (handle both drag directions)
        float minX = (rectStart.x < rectEnd.x) ? rectStart.x : rectEnd.x;
        float maxX = (rectStart.x > rectEnd.x) ? rectStart.x : rectEnd.x;
        float minY = (rectStart.y < rectEnd.y) ? rectStart.y : rectEnd.y;
        float maxY = (rectStart.y > rectEnd.y) ? rectStart.y : rectEnd.y;

        if (!addToSelection)
        {
            m_document->DeselectAll();
        }

        // Check all nodes for intersection with rectangle
        const std::vector<ComponentNode>& nodes = m_document->GetAllNodes();
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const ComponentNode& node = nodes[i];
            // Check if node bounding box intersects with selection rectangle
            float nodeMinX = node.position.x;
            float nodeMaxX = node.position.x + node.size.x;
            float nodeMinY = node.position.y;
            float nodeMaxY = node.position.y + node.size.y;

            if (!(maxX < nodeMinX || minX > nodeMaxX || maxY < nodeMinY || minY > nodeMaxY))
            {
                // Rectangle intersects node bounds
                m_document->SelectNode(node.nodeId);
            }
        }
    }

    void PrefabCanvas::ClearSelection() { if (m_document) { m_document->DeselectAll(); } }

    void PrefabCanvas::SelectAll()
    {
        if (!m_document) { return; }
        m_document->DeselectAll();
        const std::vector<ComponentNode>& nodes = m_document->GetAllNodes();
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            m_document->SelectNode(nodes[i].nodeId);
        }
    }

    void PrefabCanvas::DeleteSelectedNodes()
    {
        if (!m_document) { return; }
        const std::vector<NodeId>& selectedNodes = m_document->GetSelectedNodes();
        std::vector<NodeId> nodesToDelete(selectedNodes.begin(), selectedNodes.end());
        for (size_t i = 0; i < nodesToDelete.size(); ++i)
        {
            m_document->RemoveNode(nodesToDelete[i]);
        }
        m_document->DeselectAll();
    }

    void PrefabCanvas::AddComponentNode(const std::string& componentType, const std::string& componentName, float screenX, float screenY)
    {
        if (!m_document) { return; }

        // Convert screen position to canvas position
        Vector canvasPos = ScreenToCanvas(screenX, screenY);

        // Create new node
        NodeId newNodeId = m_document->CreateComponentNode(componentType, componentName);
        ComponentNode* newNode = m_document->GetNode(newNodeId);
        if (newNode)
        {
            newNode->position = canvasPos;
            newNode->size = Vector(150.0f, 80.0f, 0.0f);
            newNode->enabled = true;

            SYSTEM_LOG << "[PrefabCanvas::AddComponentNode] Created node: " << componentName 
                       << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
        }
    }

    void PrefabCanvas::AcceptComponentDropAtScreenPos(const std::string& componentType, const std::string& componentName, float screenX, float screenY)
    {
        if (!m_document) { return; }

        // Transform from absolute screen position to logical canvas position
        // Using stored canvas screen position for correct context
        // Matches formula: screen = canvas * zoom + offset + screenBase
        // Therefore: canvas = (screen - screenBase - offset) / zoom
        ImVec2 storedCanvasPos = m_canvasScreenPos;

        Vector canvas;
        canvas.x = (screenX - storedCanvasPos.x - m_canvasOffset.x) / m_canvasZoom;
        canvas.y = (screenY - storedCanvasPos.y - m_canvasOffset.y) / m_canvasZoom;
        canvas.z = 0.0f;

        // Create new node at the computed canvas position
        NodeId newNodeId = m_document->CreateComponentNode(componentType, componentName);
        ComponentNode* newNode = m_document->GetNode(newNodeId);
        if (newNode)
        {
            newNode->position = canvas;
            newNode->size = Vector(150.0f, 80.0f, 0.0f);
            newNode->enabled = true;
        }
    }

    void PrefabCanvas::StartConnectionCreation(NodeId sourceNodeId)
    {
        if (sourceNodeId == InvalidNodeId || !m_document) { return; }

        m_connectionSourceNodeId = sourceNodeId;
        m_isCreatingConnection = true;
        m_interactionMode = CanvasInteractionMode::CreatingConnection;
        m_connectionPreviewEnd = Vector(0.0f, 0.0f, 0.0f);
    }

    void PrefabCanvas::CompleteConnection(NodeId targetNodeId)
    {
        if (targetNodeId == InvalidNodeId || !m_document) { return; }
        if (m_connectionSourceNodeId == InvalidNodeId || m_connectionSourceNodeId == targetNodeId) { return; }

        m_document->ConnectNodes(m_connectionSourceNodeId, targetNodeId);
        CancelConnectionCreation();
    }

    void PrefabCanvas::CancelConnectionCreation()
    {
        m_isCreatingConnection = false;
        m_connectionSourceNodeId = InvalidNodeId;
        m_interactionMode = CanvasInteractionMode::Normal;
    }

    bool PrefabCanvas::IsCreatingConnection() const { return m_isCreatingConnection; }
    NodeId PrefabCanvas::GetConnectionSourceNode() const { return m_connectionSourceNodeId; }

    void PrefabCanvas::SetGridEnabled(bool enabled) { m_showGrid = enabled; }
    bool PrefabCanvas::IsGridEnabled() const { return m_showGrid; }
    void PrefabCanvas::SetGridSpacing(float spacing) { m_gridSpacing = (spacing > 5.0f) ? spacing : 5.0f; }
    float PrefabCanvas::GetGridSpacing() const { return m_gridSpacing; }
    void PrefabCanvas::SetShowDebugInfo(bool show) { m_showDebugInfo = show; }
    bool PrefabCanvas::GetShowDebugInfo() const { return m_showDebugInfo; }

    void PrefabCanvas::SetSnapToGrid(bool snap) { m_snapToGrid = snap; }
    bool PrefabCanvas::IsSnapToGridEnabled() const { return m_snapToGrid; }

    Vector PrefabCanvas::GetCanvasOffset() const { return m_canvasOffset; }
    void PrefabCanvas::SetCanvasOffset(const Vector& offset) { m_canvasOffset = offset; }
    float PrefabCanvas::GetCanvasZoom() const { return m_canvasZoom; }
    void PrefabCanvas::SetCanvasZoom(float zoom) { m_canvasZoom = (zoom > 0.1f && zoom < 3.0f) ? zoom : m_canvasZoom; }

    Vector PrefabCanvas::ScreenToCanvas(float screenX, float screenY) const
    { 
        Vector screen(screenX, screenY, 0.0f);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        // Correct transformation: undo screen position, undo offset, undo zoom
        // screen = canvas * zoom + offset + canvasPos
        // canvas = (screen - canvasPos - offset) / zoom
        screen.x = (screen.x - canvasPos.x - m_canvasOffset.x) / m_canvasZoom;
        screen.y = (screen.y - canvasPos.y - m_canvasOffset.y) / m_canvasZoom;
        return screen;
    }

    Vector PrefabCanvas::CanvasToScreen(float canvasX, float canvasY) const
    { 
        Vector canvas(canvasX, canvasY, 0.0f);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        canvas.x = canvas.x * m_canvasZoom + canvasPos.x + m_canvasOffset.x;
        canvas.y = canvas.y * m_canvasZoom + canvasPos.y + m_canvasOffset.y;
        return canvas;
    }

    ImVec2 PrefabCanvas::GetCanvasScreenPos() const
    {
        return m_canvasScreenPos;
    }

    CanvasInteractionMode PrefabCanvas::GetInteractionMode() const { return m_interactionMode; }
    bool PrefabCanvas::IsNodeDragging() const { return m_interactionMode == CanvasInteractionMode::DraggingNode; }
    bool PrefabCanvas::IsPanning() const { return m_interactionMode == CanvasInteractionMode::PanningCamera; }

    void PrefabCanvas::RenderGrid()
    { 
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImVec2 canvasEnd(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);

        ImU32 majorGridColor = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 0.4f));
        ImU32 minorGridColor = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 0.2f));

        // Scale grid spacing by zoom level to maintain consistent grid density
        float scaledGridSpacing = m_gridSpacing * m_canvasZoom;
        float scaledMinorSpacing = scaledGridSpacing / 5.0f;

        // Calculate grid offset in screen space
        // Grid origin = canvasPos + offset * zoom
        // We need to find where the grid should start on screen
        float gridStartX = canvasPos.x + m_canvasOffset.x * m_canvasZoom;
        float gridStartY = canvasPos.y + m_canvasOffset.y * m_canvasZoom;

        // Calculate which grid lines should be visible and their screen positions
        float gridOffsetX = fmod(gridStartX, scaledGridSpacing);
        float gridOffsetY = fmod(gridStartY, scaledGridSpacing);

        // Handle negative modulos
        if (gridOffsetX < 0) gridOffsetX += scaledGridSpacing;
        if (gridOffsetY < 0) gridOffsetY += scaledGridSpacing;

        // Draw minor grid lines
        for (float x = canvasPos.x + gridOffsetX - scaledGridSpacing; x < canvasEnd.x + scaledMinorSpacing; x += scaledMinorSpacing)
        { 
            drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y), minorGridColor);
        }

        for (float y = canvasPos.y + gridOffsetY - scaledGridSpacing; y < canvasEnd.y + scaledMinorSpacing; y += scaledMinorSpacing)
        { 
            drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y), minorGridColor);
        }

        // Draw major grid lines
        for (float x = canvasPos.x + gridOffsetX - scaledGridSpacing; x < canvasEnd.x + scaledGridSpacing; x += scaledGridSpacing)
        { 
            drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y), majorGridColor, 1.5f);
        }

        for (float y = canvasPos.y + gridOffsetY - scaledGridSpacing; y < canvasEnd.y + scaledGridSpacing; y += scaledGridSpacing)
        { 
            drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y), majorGridColor, 1.5f);
        }
    }

    void PrefabCanvas::RenderNodes()
    { 
        if (!m_document || !m_renderer) { return; }

        // Pass canvas transformation context to renderer
        m_renderer->SetCanvasTransform(m_canvasOffset, m_canvasZoom);
        m_renderer->SetCanvasScreenPos(ImGui::GetCursorScreenPos());

        ImGui::PushClipRect(
            ImGui::GetCursorScreenPos(),
            ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                   ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y),
            true
        );

        m_renderer->RenderNodes(m_document);

        ImGui::PopClipRect();
    }

    void PrefabCanvas::RenderConnections()
    { 
        if (!m_document || !m_renderer) { return; }

        // Pass canvas transformation context to renderer
        m_renderer->SetCanvasTransform(m_canvasOffset, m_canvasZoom);
        m_renderer->SetCanvasScreenPos(ImGui::GetCursorScreenPos());

        ImGui::PushClipRect(
            ImGui::GetCursorScreenPos(),
            ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                   ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y),
            true
        );

        m_renderer->RenderConnections(m_document, m_hoveredConnectionIndex);

        ImGui::PopClipRect();
    }

    void PrefabCanvas::RenderDebugInfo()
    { 
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        ImVec2 debugPos(ImGui::GetCursorScreenPos().x + 10.0f, ImGui::GetCursorScreenPos().y + 10.0f);
        ImU32 debugTextColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        char debugBuffer[512];
        snprintf(debugBuffer, sizeof(debugBuffer), 
                 "Zoom: %.2f | Offset: %.0f, %.0f | Nodes: %zu | Mode: %d | Dragging: %s",
                 m_canvasZoom, m_canvasOffset.x, m_canvasOffset.y,
                 m_document ? m_document->GetNodeCount() : 0,
                 (int)m_interactionMode,
                 IsNodeDragging() ? "YES" : "NO");

        drawList->AddText(debugPos, debugTextColor, debugBuffer);
    }

    void PrefabCanvas::RenderConnectionPreview()
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr || !m_document) { return; }

        const ComponentNode* sourceNode = m_document->GetNode(m_connectionSourceNodeId);
        if (sourceNode == nullptr) { return; }

        // Find output port of source node (use first output port)
        const NodePort* outputPort = nullptr;
        for (const auto& port : sourceNode->GetPorts())
        {
            if (port.isOutput)
            {
                outputPort = &port;
                break;
            }
        }

        if (!outputPort)
        {
            // Fallback to node center if no output port
            Vector screenSourcePos = CanvasToScreen(sourceNode->position.x, sourceNode->position.y);
            Vector screenEndPos = CanvasToScreen(m_connectionPreviewEnd.x, m_connectionPreviewEnd.y);

            ImU32 previewLineColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 0.0f, 0.8f));
            drawList->AddLine(
                ImVec2(screenSourcePos.x, screenSourcePos.y),
                ImVec2(screenEndPos.x, screenEndPos.y),
                previewLineColor,
                2.5f
            );
        }
        else
        {
            // Calculate port position on node edge (same logic as RenderPort)
            Vector screenCenter = CanvasToScreen(sourceNode->position.x, sourceNode->position.y);
            float scaledWidth = sourceNode->size.x * 0.5f * m_renderer->GetNodeScale() * m_canvasZoom;
            float scaledHeight = sourceNode->size.y * 0.5f * m_renderer->GetNodeScale() * m_canvasZoom;

            std::vector<const NodePort*> outputPorts;
            for (const auto& port : sourceNode->GetPorts())
            {
                if (port.isOutput)
                {
                    outputPorts.push_back(&port);
                }
            }

            uint32_t portIndexInType = 0;
            for (size_t i = 0; i < outputPorts.size(); ++i)
            {
                if (outputPorts[i]->portId == outputPort->portId)
                {
                    portIndexInType = i;
                    break;
                }
            }

            Vector portPos = sourceNode->position;
            if (outputPorts.size() > 0)
            {
                float spacing = (2.0f * scaledHeight) / (outputPorts.size() + 1);
                float yOffset = -scaledHeight + spacing * (portIndexInType + 1);
                portPos.x += scaledWidth / m_canvasZoom;
                portPos.y += yOffset / m_canvasZoom;
            }

            Vector screenSourcePos = CanvasToScreen(portPos.x, portPos.y);
            Vector screenEndPos = CanvasToScreen(m_connectionPreviewEnd.x, m_connectionPreviewEnd.y);

            ImU32 previewLineColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 0.0f, 0.8f));
            drawList->AddLine(
                ImVec2(screenSourcePos.x, screenSourcePos.y),
                ImVec2(screenEndPos.x, screenEndPos.y),
                previewLineColor,
                2.5f
            );

            // Draw circle at source to indicate starting port
            drawList->AddCircle(ImVec2(screenSourcePos.x, screenSourcePos.y), 6.0f, previewLineColor, 0, 1.5f);
        }
    }

    void PrefabCanvas::RenderSelectionBox()
    { 
        // TODO: Implement selection box rendering for drag-to-select
    }

    void PrefabCanvas::RenderSelectionRectangle()
    {
        if (!m_isSelectingRectangle) { return; }

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Convert canvas positions to screen positions
        Vector screenStart = CanvasToScreen(m_selectionRectStart.x, m_selectionRectStart.y);
        Vector screenEnd = CanvasToScreen(m_selectionRectEnd.x, m_selectionRectEnd.y);

        // Draw rectangle outline
        ImU32 rectOutlineColor = ImGui::GetColorU32(ImVec4(0.5f, 0.7f, 1.0f, 1.0f));  // Blue
        ImU32 rectFillColor = ImGui::GetColorU32(ImVec4(0.5f, 0.7f, 1.0f, 0.15f));     // Semi-transparent blue

        drawList->AddRect(
            ImVec2(screenStart.x, screenStart.y),
            ImVec2(screenEnd.x, screenEnd.y),
            rectOutlineColor,
            0.0f,
            0,
            2.0f
        );

        drawList->AddRectFilled(
            ImVec2(screenStart.x, screenStart.y),
            ImVec2(screenEnd.x, screenEnd.y),
            rectFillColor
        );
    }

    void PrefabCanvas::UpdateNodePositions()
    {
        // Apply pending node position updates
    }

    void PrefabCanvas::RenderConnectionContextMenu()
    {
        // Render context menu for connection deletion
        if (ImGui::BeginPopup("ConnectionContextMenu"))
        {
            if (ImGui::MenuItem("Delete Connection"))
            {
                if (m_contextMenuConnectionIndex >= 0 && m_document)
                {
                    const std::vector<std::pair<NodeId, NodeId>>& connections = m_document->GetConnections();
                    if (m_contextMenuConnectionIndex < static_cast<int>(connections.size()))
                    {
                        const auto& conn = connections[m_contextMenuConnectionIndex];
                        m_document->DisconnectNodes(conn.first, conn.second);
                        m_document->SetDirty(true);
                    }
                }
                ImGui::CloseCurrentPopup();
                m_showConnectionContextMenu = false;
            }

            ImGui::EndPopup();
        }
    }

    void PrefabCanvas::HandleNodeDragStart(NodeId nodeId, float x, float y)
    {
        if (nodeId == InvalidNodeId || !m_document) { return; }

        ComponentNode* node = m_document->GetNode(nodeId);
        if (node == nullptr) { return; }

        m_interactionMode = CanvasInteractionMode::DraggingNode;
        m_draggedNodeId = nodeId;
        m_dragStartPos = ScreenToCanvas(x, y);
        m_nodeDragOffset = Vector(node->position.x - m_dragStartPos.x, node->position.y - m_dragStartPos.y, 0.0f);

        m_document->SelectNode(nodeId);
    }

    void PrefabCanvas::HandleNodeDrag(float x, float y)
    {
        if (m_draggedNodeId == InvalidNodeId || !m_document) { return; }

        ComponentNode* node = m_document->GetNode(m_draggedNodeId);
        if (node == nullptr) { return; }

        Vector currentCanvasPos = ScreenToCanvas(x, y);
        Vector newNodePos = Vector(
            currentCanvasPos.x + m_nodeDragOffset.x,
            currentCanvasPos.y + m_nodeDragOffset.y,
            0.0f
        );

        node->position = newNodePos;

        // Also update all other selected nodes with the same delta
        const std::vector<NodeId>& selectedNodes = m_document->GetSelectedNodes();
        Vector delta = newNodePos - m_dragStartPos;

        for (size_t i = 0; i < selectedNodes.size(); ++i)
        {
            if (selectedNodes[i] != m_draggedNodeId)
            {
                ComponentNode* selectedNode = m_document->GetNode(selectedNodes[i]);
                if (selectedNode != nullptr)
                {
                    Vector updatedPos = selectedNode->position + delta;
                    selectedNode->position = updatedPos;
                }
            }
        }
    }

    void PrefabCanvas::HandleNodeDragEnd()
    {
        m_draggedNodeId = InvalidNodeId;
        m_interactionMode = CanvasInteractionMode::Normal;
    }

    void PrefabCanvas::HandleConnectionCreation(float x, float y)
    {
        // TODO: Implement connection creation logic
    }

    void PrefabCanvas::HandleConnectionEnd(float x, float y)
    {
        // TODO: Implement connection finalization
    }

    void PrefabCanvas::HandlePanStart(float x, float y)
    {
        if (m_interactionMode != CanvasInteractionMode::Normal) { return; }

        m_interactionMode = CanvasInteractionMode::PanningCamera;
        m_isPanning = true;
        m_panStartOffset = m_canvasOffset;
        m_dragStartPos = Vector(x, y, 0.0f);
    }

    void PrefabCanvas::HandlePan(float x, float y)
    {
        if (!m_isPanning) { return; }

        Vector currentPos(x, y, 0.0f);
        Vector delta = currentPos - m_dragStartPos;

        m_canvasOffset = m_panStartOffset + Vector(delta.x, delta.y, 0.0f);
    }

    void PrefabCanvas::SnapNodePositionToGrid(Vector& position)
    {
        float gridSize = m_gridSpacing;
        position.x = (float)((int)((position.x + gridSize * 0.5f) / gridSize) * gridSize);
        position.y = (float)((int)((position.y + gridSize * 0.5f) / gridSize) * gridSize);
    }

    void PrefabCanvas::RenderContextMenu()
    {
        if (!m_document || !m_renderer) { return; }

        ImVec2 mousePos = ImGui::GetMousePos();

        // Detect right-click on canvas
        if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered())
        {
            // First check if right-clicking on a connection
            Vector canvasPos = ScreenToCanvas(mousePos.x, mousePos.y);
            const std::vector<std::pair<NodeId, NodeId>>& connections = m_document->GetConnections();
            const std::vector<ComponentNode>& nodes = m_document->GetAllNodes();

            const float hitTolerance = 10.0f;
            bool connectionFound = false;

            for (size_t connIdx = 0; connIdx < connections.size(); ++connIdx)
            {
                const ComponentNode* sourceNode = m_document->GetNode(connections[connIdx].first);
                const ComponentNode* targetNode = m_document->GetNode(connections[connIdx].second);

                if (sourceNode != nullptr && targetNode != nullptr)
                {
                    // Get connection endpoints in canvas space
                    Vector from = sourceNode->position;
                    from.x += sourceNode->size.x * 0.5f;
                    Vector to = targetNode->position;
                    to.x -= targetNode->size.x * 0.5f;

                    // Get distance from click point to connection curve
                    float distance = m_renderer->GetDistanceToConnection(
                        Vector(mousePos.x, mousePos.y, 0.0f),
                        from, to
                    );

                    if (distance <= hitTolerance)
                    {
                        m_contextMenuConnectionIndex = static_cast<int>(connIdx);
                        m_contextMenuConnectionMousePos = Vector(mousePos.x, mousePos.y, 0.0f);
                        m_showConnectionContextMenu = true;
                        ImGui::OpenPopup("ConnectionContextMenu");
                        connectionFound = true;
                        return;  // Don't open canvas context menu
                    }
                }
            }

            // If no connection found, check for node or canvas context menu
            // Store the node at click position for context menu
            m_contextMenuMousePos = Vector(mousePos.x, mousePos.y, 0.0f);
            m_contextMenuNodeId = GetNodeAtPosition(mousePos.x, mousePos.y);
            ImGui::OpenPopup("CanvasContextMenu");
        }

        // Render context menu
        if (ImGui::BeginPopup("CanvasContextMenu"))
        {
            if (m_contextMenuNodeId != InvalidNodeId)
            {
                // Context menu on a node
                if (ImGui::MenuItem("Delete Node"))
                {
                    m_document->RemoveNode(m_contextMenuNodeId);
                    m_document->DeselectAll();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Select Node"))
                {
                    if (!m_ctrlPressed)
                    {
                        m_document->DeselectAll();
                    }
                    m_document->SelectNode(m_contextMenuNodeId);
                    ImGui::CloseCurrentPopup();
                }
            }
            else
            {
                // Context menu on empty canvas
                if (ImGui::MenuItem("Clear Selection"))
                {
                    m_document->DeselectAll();
                    ImGui::CloseCurrentPopup();
                }

                if (m_document->GetNodeCount() > 0)
                {
                    if (ImGui::MenuItem("Select All"))
                    {
                        SelectAll();
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Reset View"))
                {
                    ResetView();
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }

} // namespace Olympe