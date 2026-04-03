#pragma once

#include "EntityPrefabGraphDocument.h"
#include "ComponentNodeData.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace OlympeEngine {

// ============================================================================
// PrefabCanvas - ImNodes wrapper for prefab visualization
// ============================================================================

struct CanvasState {
    glm::vec2 panOffset = glm::vec2(0.0f);
    float zoomLevel = 1.0f;
    bool isDragging = false;
    NodeGraph::NodeId draggedNodeId = NodeGraph::InvalidNodeId;
    glm::vec2 dragStartPosition = glm::vec2(0.0f);
    bool canvasHovered = false;

    // View bounds
    glm::vec2 viewportSize = glm::vec2(800.0f, 600.0f);
    glm::vec2 minPan = glm::vec2(-10000.0f);
    glm::vec2 maxPan = glm::vec2(10000.0f);
};

class PrefabCanvas {
public:
    PrefabCanvas(std::shared_ptr<EntityPrefabGraphDocument> document);
    ~PrefabCanvas();

    // Rendering
    void Render(float width, float height);
    void RenderNodes();
    void RenderConnections();
    void RenderGrid(float gridSize = 32.0f);

    // Input Handling
    void HandleMouseInput();
    void HandleKeyboardInput();
    bool IsCanvasHovered() const { return m_canvasState.canvasHovered; }

    // View Control
    void Pan(const glm::vec2& offset);
    void Zoom(float delta, const glm::vec2& centerPoint);
    void FitToView(float padding = 50.0f);
    void ResetView();

    // Coordinate Conversion
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;

    // Selection
    NodeGraph::NodeId GetNodeAtPosition(const glm::vec2& screenPos);
    void SelectNode(NodeGraph::NodeId nodeId);
    void DeselectNode(NodeGraph::NodeId nodeId);
    void DeselectAll();
    void SelectMultiple(const std::vector<NodeGraph::NodeId>& nodeIds);

    // Node Creation & Removal
    void CreateComponentNode(const std::string& componentType, const glm::vec2& screenPos);
    void RemoveSelectedNodes();
    void DuplicateSelectedNodes();

    // Properties Panel Integration
    void SetSelectedNodeProperties(const std::map<std::string, std::string>& properties);
    std::map<std::string, std::string> GetSelectedNodeProperties() const;

    // State Access
    const CanvasState& GetState() const { return m_canvasState; }
    CanvasState& GetState() { return m_canvasState; }

    // Connection Management
    bool CanConnect(NodeGraph::NodeId fromNode, NodeGraph::NodeId toNode) const;
    void CreateConnection(NodeGraph::NodeId fromNode, NodeGraph::NodeId toNode);
    void RemoveConnection(NodeGraph::NodeId fromNode, NodeGraph::NodeId toNode);

    // Viewport Management
    void SetViewportSize(float width, float height);
    glm::vec2 GetViewportSize() const { return m_canvasState.viewportSize; }

    // History & Undo/Redo (placeholder for Phase 2)
    void NotifyNodeMoved(NodeGraph::NodeId nodeId, const glm::vec2& oldPos, const glm::vec2& newPos);
    void NotifyNodeDeleted(NodeGraph::NodeId nodeId);
    void NotifyNodeCreated(NodeGraph::NodeId nodeId, const std::string& componentType);

private:
    std::shared_ptr<EntityPrefabGraphDocument> m_document;
    CanvasState m_canvasState;

    // ImNodes context (not directly exposed, managed internally)
    void* m_imNodesContext = nullptr;

    // Helper methods
    void UpdateCanvasInteraction();
    void RenderNodeInternal(const ComponentNode& node);
    void RenderConnectionLine(const glm::vec2& from, const glm::vec2& to);
    ComponentNode* GetNodeFromImNodesContext(NodeGraph::NodeId nodeId);

    // Input state tracking
    struct InputState {
        bool mouseLeftPressed = false;
        bool mouseRightPressed = false;
        bool mouseMidPressed = false;
        glm::vec2 mousePosition = glm::vec2(0.0f);
        glm::vec2 mouseLastPosition = glm::vec2(0.0f);
    } m_inputState;
};

}  // namespace OlympeEngine
