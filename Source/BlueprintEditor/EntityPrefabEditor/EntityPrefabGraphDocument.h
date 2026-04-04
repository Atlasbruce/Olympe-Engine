#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "./../../vector.h"
#include "ComponentNodeData.h"
#include "PrefabLoader.h"

namespace Olympe
{
    struct LayoutNode
    {
        NodeId nodeId = InvalidNodeId;
        Vector position;
        Vector size;
        std::vector<NodeId> inputs;
        std::vector<NodeId> outputs;
    };

    class EntityPrefabGraphDocument
    {
    public:
        EntityPrefabGraphDocument();
        ~EntityPrefabGraphDocument();

        // Node management
        NodeId CreateComponentNode(const std::string& componentType);
        NodeId CreateComponentNode(const std::string& componentType, const std::string& componentName);
        void RemoveNode(NodeId nodeId);
        bool HasNode(NodeId nodeId) const;

        // Node access
        ComponentNode* GetNode(NodeId nodeId);
        const ComponentNode* GetNode(NodeId nodeId) const;
        const std::vector<ComponentNode>& GetAllNodes() const;

        // Selection management
        void SelectNode(NodeId nodeId);
        void DeselectNode(NodeId nodeId);
        void DeselectAll();
        NodeId GetSelectedNode() const;
        const std::vector<NodeId>& GetSelectedNodes() const;

        // Layout
        void AutoLayout();
        void ArrangeNodesInGrid(int gridWidth = 4, float spacing = 200.0f);
        void CenterViewport();

        // Connection management
        bool ConnectNodes(NodeId sourceId, NodeId targetId);
        bool DisconnectNodes(NodeId sourceId, NodeId targetId);
        const std::vector<std::pair<NodeId, NodeId>>& GetConnections() const;

        // Serialization
        json ToJson() const;
        static EntityPrefabGraphDocument FromJson(const json& data);

        // File I/O
        bool LoadFromFile(const std::string& filePath);
        bool SaveToFile(const std::string& filePath);

        // Properties
        void SetDocumentName(const std::string& name);
        std::string GetDocumentName() const;

        Vector GetCanvasOffset() const;
        void SetCanvasOffset(const Vector& offset);

        float GetCanvasZoom() const;
        void SetCanvasZoom(float zoom);

        void Clear();
        size_t GetNodeCount() const;

        // Dirty flag tracking
        bool IsDirty() const;
        void SetDirty(bool dirty);

    private:
        std::vector<ComponentNode> m_nodes;
        std::vector<NodeId> m_selectedNodes;
        std::vector<std::pair<NodeId, NodeId>> m_connections;
        std::string m_documentName;
        Vector m_canvasOffset;
        float m_canvasZoom = 1.0f;
        NodeId m_nextNodeId = 1;
        bool m_isDirty = false;

        NodeId GenerateNodeId();
        std::vector<LayoutNode> CalculateLayout();
    };
}
