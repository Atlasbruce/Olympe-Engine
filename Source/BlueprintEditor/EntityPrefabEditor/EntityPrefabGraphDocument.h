#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "./../../vector.h"
#include "ComponentNodeData.h"
#include "PrefabLoader.h"
#include "../Framework/IGraphDocument.h"

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

    // Forward declaration
    class EntityPrefabRenderer;

    class EntityPrefabGraphDocument : public IGraphDocument
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

        // Connection validation
        bool ValidateConnection(NodeId sourceId, NodeId targetId) const;
        bool HasConnection(NodeId sourceId, NodeId targetId) const;

        // Serialization
        json ToJson() const;
        // NOTE: Returns by value but uses move semantics for unique_ptr member
        static GraphDocumentPtr FromJson(const json& data);

        // File I/O
        bool LoadFromFile(const std::string& filePath);
        bool SaveToFile(const std::string& filePath);

        // IGraphDocument interface implementation
        bool Load(const std::string& filePath) override;
        bool Save(const std::string& filePath) override;
        std::string GetName() const override;
        DocumentType GetType() const override;
        std::string GetFilePath() const override;
        void SetFilePath(const std::string& path) override;
        IGraphRenderer* GetRenderer() override;
        const IGraphRenderer* GetRenderer() const override;
        void OnDocumentModified() override;

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

        // Parameter schema management
        void LoadParameterSchemas(const std::string& schemasFilePath);
        void InitializeNodeProperties(ComponentNode& node);

    private:
        std::vector<ComponentNode> m_nodes;
        std::vector<NodeId> m_selectedNodes;
        std::vector<std::pair<NodeId, NodeId>> m_connections;
        std::string m_documentName;
        std::string m_filePath;  // NEW: Track current file path for IGraphDocument
        Vector m_canvasOffset;
        float m_canvasZoom = 1.0f;
        NodeId m_nextNodeId = 1;
        bool m_isDirty = false;

        // Parameter schemas: componentType -> { paramName -> defaultValue }
        std::map<std::string, std::map<std::string, std::string>> m_parameterSchemas;

        NodeId GenerateNodeId();
        std::vector<LayoutNode> CalculateLayout();
    };

} // namespace Olympe
