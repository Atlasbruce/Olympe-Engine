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
        PrefabNodeId nodeId = InvalidNodeId;
        Vector position;
        Vector size;
        std::vector<PrefabNodeId> inputs;
        std::vector<PrefabNodeId> outputs;
    };

    // Forward declaration
    class EntityPrefabRenderer;

    class EntityPrefabGraphDocument : public IGraphDocument
    {
    public:
        EntityPrefabGraphDocument();
        ~EntityPrefabGraphDocument();

        // Node management
        PrefabNodeId CreateComponentNode(const std::string& componentType);
        PrefabNodeId CreateComponentNode(const std::string& componentType, const std::string& componentName);
        void RemoveNode(PrefabNodeId nodeId);
        bool HasNode(PrefabNodeId nodeId) const;

        // Node access
        ComponentNode* GetNode(PrefabNodeId nodeId);
        const ComponentNode* GetNode(PrefabNodeId nodeId) const;
        const std::vector<ComponentNode>& GetAllNodes() const;

        // Selection management
        void SelectNode(PrefabNodeId nodeId);
        void DeselectNode(PrefabNodeId nodeId);
        void DeselectAll();
        PrefabNodeId GetSelectedNode() const;
        const std::vector<PrefabNodeId>& GetSelectedNodes() const;

        // Layout
        void AutoLayout();
        void ArrangeNodesInGrid(int gridWidth = 4, float spacing = 200.0f);
        void CenterViewport();

        // Connection management
        bool ConnectNodes(PrefabNodeId sourceId, PrefabNodeId targetId);
        bool DisconnectNodes(PrefabNodeId sourceId, PrefabNodeId targetId);
        const std::vector<std::pair<PrefabNodeId, PrefabNodeId>>& GetConnections() const;

        // Connection validation
        bool ValidateConnection(PrefabNodeId sourceId, PrefabNodeId targetId) const;
        bool HasConnection(PrefabNodeId sourceId, PrefabNodeId targetId) const;

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
        std::vector<PrefabNodeId> m_selectedNodes;
        std::vector<std::pair<PrefabNodeId, PrefabNodeId>> m_connections;
        std::string m_documentName;
        std::string m_filePath;  // NEW: Track current file path for IGraphDocument
        Vector m_canvasOffset;
        float m_canvasZoom = 1.0f;
        PrefabNodeId m_nextNodeId = 1;
        bool m_isDirty = false;

        // Parameter schemas: componentType -> { paramName -> defaultValue }
        std::map<std::string, std::map<std::string, std::string>> m_parameterSchemas;

        PrefabNodeId GenerateNodeId();
        std::vector<LayoutNode> CalculateLayout();
    };

} // namespace Olympe
