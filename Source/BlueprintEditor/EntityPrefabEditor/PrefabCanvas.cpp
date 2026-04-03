#include "PrefabCanvas.h"
#include <algorithm>
#include <cmath>

namespace OlympeEngine {

// ============================================================================
// Constructor & Destructor
// ============================================================================

PrefabCanvas::PrefabCanvas(std::shared_ptr<EntityPrefabGraphDocument> document)
    : m_document(document) {
    // Initialize ImNodes context (placeholder - actual initialization in Render)
    m_canvasState.panOffset = glm::vec2(0.0f, 0.0f);
    m_canvasState.zoomLevel = 1.0f;
}

PrefabCanvas::~PrefabCanvas() {
    // Cleanup ImNodes context if needed
}

// ============================================================================
// Rendering
// ============================================================================

void PrefabCanvas::Render(float width, float height) {
    if (!m_document) return;

    // Update viewport size
    SetViewportSize(width, height);

    // Render background grid
    RenderGrid(32.0f);

    // Render connections between nodes
    RenderConnections();

    // Render all component nodes
    RenderNodes();

    // Handle input
    HandleMouseInput();
    HandleKeyboardInput();
}

void PrefabCanvas::RenderNodes() {
    if (!m_document) return;

    auto nodes = m_document->GetAllComponentNodes();
    for (const auto* node : nodes) {
        if (node) {
            RenderNodeInternal(*node);
        }
    }
}

void PrefabCanvas::RenderConnections() {
    // Placeholder for connection rendering
    // In Phase 4+, will draw connection lines between nodes
}

void PrefabCanvas::RenderGrid(float gridSize) {
    // Placeholder for grid rendering
    // Will draw grid background using ImGui drawing API
}

// ============================================================================
// Input Handling
// ============================================================================

void PrefabCanvas::HandleMouseInput() {
    // Placeholder for mouse input handling
    // Will handle pan, zoom, selection, dragging
}

void PrefabCanvas::HandleKeyboardInput() {
    // Placeholder for keyboard input handling
    // Will handle delete, copy, paste, etc.
}

// ============================================================================
// View Control
// ============================================================================

void PrefabCanvas::Pan(const glm::vec2& offset) {
    m_canvasState.panOffset += offset;

    // Clamp pan offset
    m_canvasState.panOffset.x = std::max(m_canvasState.minPan.x, 
                                        std::min(m_canvasState.panOffset.x, m_canvasState.maxPan.x));
    m_canvasState.panOffset.y = std::max(m_canvasState.minPan.y, 
                                        std::min(m_canvasState.panOffset.y, m_canvasState.maxPan.y));
}

void PrefabCanvas::Zoom(float delta, const glm::vec2& centerPoint) {
    const float minZoom = 0.1f;
    const float maxZoom = 5.0f;

    float oldZoom = m_canvasState.zoomLevel;
    m_canvasState.zoomLevel += delta * 0.1f;
    m_canvasState.zoomLevel = std::max(minZoom, std::min(maxZoom, m_canvasState.zoomLevel));

    // Adjust pan to zoom around center point
    float zoomRatio = m_canvasState.zoomLevel / oldZoom;
    m_canvasState.panOffset = centerPoint + (m_canvasState.panOffset - centerPoint) * zoomRatio;
}

void PrefabCanvas::FitToView(float padding) {
    if (!m_document) return;

    auto nodes = m_document->GetAllComponentNodes();
    if (nodes.empty()) return;

    // Calculate bounding box
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto* node : nodes) {
        if (node) {
            minX = std::min(minX, node->position.x - 100.0f);
            maxX = std::max(maxX, node->position.x + 100.0f);
            minY = std::min(minY, node->position.y - 50.0f);
            maxY = std::max(maxY, node->position.y + 50.0f);
        }
    }

    // Calculate zoom and pan to fit
    float width = maxX - minX + 2.0f * padding;
    float height = maxY - minY + 2.0f * padding;

    float zoomX = m_canvasState.viewportSize.x / width;
    float zoomY = m_canvasState.viewportSize.y / height;
    m_canvasState.zoomLevel = std::min(zoomX, zoomY);

    glm::vec2 center = glm::vec2((minX + maxX) / 2.0f, (minY + maxY) / 2.0f);
    m_canvasState.panOffset = glm::vec2(m_canvasState.viewportSize.x, m_canvasState.viewportSize.y) / 2.0f - center * m_canvasState.zoomLevel;
}

void PrefabCanvas::ResetView() {
    m_canvasState.panOffset = glm::vec2(0.0f, 0.0f);
    m_canvasState.zoomLevel = 1.0f;
}

// ============================================================================
// Coordinate Conversion
// ============================================================================

glm::vec2 PrefabCanvas::ScreenToWorld(const glm::vec2& screenPos) const {
    return (screenPos - m_canvasState.panOffset) / m_canvasState.zoomLevel;
}

glm::vec2 PrefabCanvas::WorldToScreen(const glm::vec2& worldPos) const {
    return worldPos * m_canvasState.zoomLevel + m_canvasState.panOffset;
}

// ============================================================================
// Selection
// ============================================================================

NodeGraph::NodeId PrefabCanvas::GetNodeAtPosition(const glm::vec2& screenPos) {
    if (!m_document) return NodeGraph::InvalidNodeId;

    glm::vec2 worldPos = ScreenToWorld(screenPos);
    auto nodes = m_document->GetAllComponentNodes();

    for (const auto* node : nodes) {
        if (node) {
            glm::vec2 nodeScreenPos = WorldToScreen(node->position);
            float distance = glm::distance(nodeScreenPos, screenPos);

            if (distance < 50.0f) {  // Simplified hit test
                return node->nodeId;
            }
        }
    }

    return NodeGraph::InvalidNodeId;
}

void PrefabCanvas::SelectNode(NodeGraph::NodeId nodeId) {
    if (m_document) {
        m_document->SelectNode(nodeId);
    }
}

void PrefabCanvas::DeselectNode(NodeGraph::NodeId nodeId) {
    if (m_document) {
        m_document->DeselectNode(nodeId);
    }
}

void PrefabCanvas::DeselectAll() {
    if (m_document) {
        m_document->DeselectAll();
    }
}

void PrefabCanvas::SelectMultiple(const std::vector<NodeGraph::NodeId>& nodeIds) {
    if (!m_document) return;

    m_document->DeselectAll();
    for (NodeGraph::NodeId nodeId : nodeIds) {
        m_document->SelectNode(nodeId);
    }
}

// ============================================================================
// Node Creation & Removal
// ============================================================================

void PrefabCanvas::CreateComponentNode(const std::string& componentType, const glm::vec2& screenPos) {
    if (!m_document) return;

    glm::vec2 worldPos = ScreenToWorld(screenPos);
    m_document->CreateComponentNode(componentType, componentType + "_node", worldPos);
}

void PrefabCanvas::RemoveSelectedNodes() {
    if (!m_document) return;

    auto selected = m_document->GetSelectedNodes();
    for (NodeGraph::NodeId nodeId : selected) {
        m_document->RemoveComponentNode(nodeId);
    }
}

void PrefabCanvas::DuplicateSelectedNodes() {
    // Placeholder for Phase 3+
}

// ============================================================================
// Properties Panel Integration
// ============================================================================

void PrefabCanvas::SetSelectedNodeProperties(const std::map<std::string, std::string>& properties) {
    // Placeholder for Phase 3+
}

std::map<std::string, std::string> PrefabCanvas::GetSelectedNodeProperties() const {
    return {};  // Placeholder
}

// ============================================================================
// Connection Management
// ============================================================================

bool PrefabCanvas::CanConnect(NodeGraph::NodeId fromNode, NodeGraph::NodeId toNode) const {
    if (!m_document) return false;

    auto from = m_document->GetComponentNode(fromNode);
    auto to = m_document->GetComponentNode(toNode);

    if (!from || !to) return false;
    if (from->isEntityCenter && to->isEntityCenter) return false;

    return true;
}

void PrefabCanvas::CreateConnection(NodeGraph::NodeId fromNode, NodeGraph::NodeId toNode) {
    if (CanConnect(fromNode, toNode)) {
        // Placeholder for Phase 4+
    }
}

void PrefabCanvas::RemoveConnection(NodeGraph::NodeId fromNode, NodeGraph::NodeId toNode) {
    // Placeholder for Phase 4+
}

// ============================================================================
// Viewport Management
// ============================================================================

void PrefabCanvas::SetViewportSize(float width, float height) {
    m_canvasState.viewportSize = glm::vec2(width, height);
}

// ============================================================================
// History & Notifications
// ============================================================================

void PrefabCanvas::NotifyNodeMoved(NodeGraph::NodeId nodeId, const glm::vec2& oldPos, const glm::vec2& newPos) {
    // Placeholder for Phase 5+ undo/redo
}

void PrefabCanvas::NotifyNodeDeleted(NodeGraph::NodeId nodeId) {
    // Placeholder for Phase 5+ undo/redo
}

void PrefabCanvas::NotifyNodeCreated(NodeGraph::NodeId nodeId, const std::string& componentType) {
    // Placeholder for Phase 5+ undo/redo
}

// ============================================================================
// Private Helpers
// ============================================================================

void PrefabCanvas::UpdateCanvasInteraction() {
    // Placeholder for interaction updates
}

void PrefabCanvas::RenderNodeInternal(const ComponentNode& node) {
    // Placeholder for actual node rendering
    // Will use ImGui drawing or ImNodes API
}

void PrefabCanvas::RenderConnectionLine(const glm::vec2& from, const glm::vec2& to) {
    // Placeholder for connection line rendering
}

ComponentNode* PrefabCanvas::GetNodeFromImNodesContext(NodeGraph::NodeId nodeId) {
    return m_document ? m_document->GetComponentNode(nodeId) : nullptr;
}

}  // namespace OlympeEngine
