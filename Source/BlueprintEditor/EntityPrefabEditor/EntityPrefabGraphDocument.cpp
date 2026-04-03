#include "EntityPrefabGraphDocument.h"

namespace Olympe
{
    EntityPrefabGraphDocument::EntityPrefabGraphDocument() : m_canvasZoom(1.0f), m_nextNodeId(1) { }
    EntityPrefabGraphDocument::~EntityPrefabGraphDocument() { }

    NodeId EntityPrefabGraphDocument::CreateComponentNode(const std::string& componentType)
    { return CreateComponentNode(componentType, ""); }

    NodeId EntityPrefabGraphDocument::CreateComponentNode(const std::string& componentType, const std::string& componentName)
    { 
        ComponentNode node(componentType);
        node.nodeId = m_nextNodeId++;
        node.componentName = componentName;
        m_nodes.push_back(node);
        return node.nodeId;
    }

    void EntityPrefabGraphDocument::RemoveNode(NodeId nodeId)
    { 
        for (size_t i = 0; i < m_nodes.size(); ++i)
        { if (m_nodes[i].nodeId == nodeId) { m_nodes.erase(m_nodes.begin() + i); break; } }
    }

    bool EntityPrefabGraphDocument::HasNode(NodeId nodeId) const
    { for (size_t i = 0; i < m_nodes.size(); ++i) { if (m_nodes[i].nodeId == nodeId) { return true; } } return false; }

    ComponentNode* EntityPrefabGraphDocument::GetNode(NodeId nodeId)
    { for (size_t i = 0; i < m_nodes.size(); ++i) { if (m_nodes[i].nodeId == nodeId) { return &m_nodes[i]; } } return nullptr; }

    const ComponentNode* EntityPrefabGraphDocument::GetNode(NodeId nodeId) const
    { for (size_t i = 0; i < m_nodes.size(); ++i) { if (m_nodes[i].nodeId == nodeId) { return &m_nodes[i]; } } return nullptr; }

    const std::vector<ComponentNode>& EntityPrefabGraphDocument::GetAllNodes() const { return m_nodes; }

    void EntityPrefabGraphDocument::SelectNode(NodeId nodeId)
    { 
        for (auto it = m_selectedNodes.begin(); it != m_selectedNodes.end(); ++it)
        { if (*it == nodeId) { return; } }
        m_selectedNodes.push_back(nodeId);
        ComponentNode* node = GetNode(nodeId);
        if (node != nullptr) { node->selected = true; }
    }

    void EntityPrefabGraphDocument::DeselectNode(NodeId nodeId)
    { 
        for (auto it = m_selectedNodes.begin(); it != m_selectedNodes.end(); ++it)
        { if (*it == nodeId) { m_selectedNodes.erase(it); break; } }
        ComponentNode* node = GetNode(nodeId);
        if (node != nullptr) { node->selected = false; }
    }

    void EntityPrefabGraphDocument::DeselectAll()
    { 
        for (size_t i = 0; i < m_selectedNodes.size(); ++i)
        { ComponentNode* node = GetNode(m_selectedNodes[i]); if (node != nullptr) { node->selected = false; } }
        m_selectedNodes.clear();
    }

    NodeId EntityPrefabGraphDocument::GetSelectedNode() const
    { if (m_selectedNodes.size() > 0) { return m_selectedNodes[0]; } return InvalidNodeId; }

    const std::vector<NodeId>& EntityPrefabGraphDocument::GetSelectedNodes() const { return m_selectedNodes; }

    void EntityPrefabGraphDocument::AutoLayout() { }
    void EntityPrefabGraphDocument::ArrangeNodesInGrid(int gridWidth, float spacing) { (void)gridWidth; (void)spacing; }
    void EntityPrefabGraphDocument::CenterViewport() { }

    bool EntityPrefabGraphDocument::ConnectNodes(NodeId sourceId, NodeId targetId)
    { m_connections.push_back(std::make_pair(sourceId, targetId)); return true; }

    bool EntityPrefabGraphDocument::DisconnectNodes(NodeId sourceId, NodeId targetId)
    { 
        for (auto it = m_connections.begin(); it != m_connections.end(); ++it)
        { if (it->first == sourceId && it->second == targetId) { m_connections.erase(it); return true; } }
        return false;
    }

    const std::vector<std::pair<NodeId, NodeId>>& EntityPrefabGraphDocument::GetConnections() const { return m_connections; }

    json EntityPrefabGraphDocument::ToJson() const { return json::object(); }
    EntityPrefabGraphDocument EntityPrefabGraphDocument::FromJson(const json& data) { (void)data; return EntityPrefabGraphDocument(); }

    bool EntityPrefabGraphDocument::LoadFromFile(const std::string& filePath) { (void)filePath; return false; }
    bool EntityPrefabGraphDocument::SaveToFile(const std::string& filePath) { (void)filePath; return false; }

    void EntityPrefabGraphDocument::SetDocumentName(const std::string& name) { m_documentName = name; }
    std::string EntityPrefabGraphDocument::GetDocumentName() const { return m_documentName; }

    Vector EntityPrefabGraphDocument::GetCanvasOffset() const { return m_canvasOffset; }
    void EntityPrefabGraphDocument::SetCanvasOffset(const Vector& offset) { m_canvasOffset = offset; }

    float EntityPrefabGraphDocument::GetCanvasZoom() const { return m_canvasZoom; }
    void EntityPrefabGraphDocument::SetCanvasZoom(float zoom) { m_canvasZoom = zoom; }

    void EntityPrefabGraphDocument::Clear() { m_nodes.clear(); m_selectedNodes.clear(); m_connections.clear(); m_nextNodeId = 1; }
    size_t EntityPrefabGraphDocument::GetNodeCount() const { return m_nodes.size(); }

    std::vector<LayoutNode> EntityPrefabGraphDocument::CalculateLayout() { return std::vector<LayoutNode>(); }

} // namespace Olympe
