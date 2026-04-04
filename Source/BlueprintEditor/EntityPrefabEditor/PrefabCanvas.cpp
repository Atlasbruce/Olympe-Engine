#include "PrefabCanvas.h"
#include "ComponentNodeRenderer.h"
#include "../../Source/third_party/imgui/imgui.h"

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

        if (m_showGrid) { RenderGrid(); }
        RenderConnections();
        RenderNodes();
        if (m_isCreatingConnection) { RenderConnectionPreview(); }
        if (m_showDebugInfo) { RenderDebugInfo(); }

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

        m_lastMousePos = m_currentMousePos;
    }

    void PrefabCanvas::OnMouseDown(int button, float x, float y)
    { 
        if (!m_document) { return; }

        m_currentMousePos = Vector(x, y, 0.0f);
        Vector canvasPos = ScreenToCanvas(x, y);

        if (button == 0) // Left mouse button
        {
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
                m_document->DeselectAll();
            }
        }
        else if (button == 2) // Middle mouse button
        {
            HandlePanStart(x, y);
        }

        m_lastMousePos = m_currentMousePos;
    }

    void PrefabCanvas::OnMouseUp(int button, float x, float y)
    { 
        m_currentMousePos = Vector(x, y, 0.0f);

        if (button == 0 && m_interactionMode == CanvasInteractionMode::DraggingNode)
        {
            HandleNodeDragEnd();
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

        float offsetX = (int)(m_canvasOffset.x / m_gridSpacing) * m_gridSpacing;
        float offsetY = (int)(m_canvasOffset.y / m_gridSpacing) * m_gridSpacing;

        float minorSpacing = m_gridSpacing / 5.0f;
        for (float x = offsetX - m_gridSpacing; x < canvasPos.x + canvasSize.x; x += minorSpacing)
        { 
            drawList->AddLine(
                ImVec2(x, canvasPos.y),
                ImVec2(x, canvasEnd.y),
                minorGridColor
            );
        }

        for (float y = offsetY - m_gridSpacing; y < canvasPos.y + canvasSize.y; y += minorSpacing)
        { 
            drawList->AddLine(
                ImVec2(canvasPos.x, y),
                ImVec2(canvasEnd.x, y),
                minorGridColor
            );
        }

        for (float x = offsetX - m_gridSpacing; x < canvasPos.x + canvasSize.x; x += m_gridSpacing)
        { 
            drawList->AddLine(
                ImVec2(x, canvasPos.y),
                ImVec2(x, canvasEnd.y),
                majorGridColor,
                1.5f
            );
        }

        for (float y = offsetY - m_gridSpacing; y < canvasPos.y + canvasSize.y; y += m_gridSpacing)
        { 
            drawList->AddLine(
                ImVec2(canvasPos.x, y),
                ImVec2(canvasEnd.x, y),
                majorGridColor,
                1.5f
            );
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

        m_renderer->RenderConnections(m_document);

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

    void PrefabCanvas::RenderSelectionBox()
    { 
        // TODO: Implement selection box rendering for drag-to-select
    }

    void PrefabCanvas::UpdateNodePositions()
    {
        // Apply pending node position updates
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

        if (m_snapToGrid)
        {
            SnapNodePositionToGrid(newNodePos);
        }

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
                    if (m_snapToGrid)
                    {
                        SnapNodePositionToGrid(updatedPos);
                    }
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

} // namespace Olympe