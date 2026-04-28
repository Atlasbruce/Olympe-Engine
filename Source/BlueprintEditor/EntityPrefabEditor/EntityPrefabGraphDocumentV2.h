#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../vector.h"
#include "ComponentNodeData.h"
#include "PrefabLoader.h"
#include "../Framework/IGraphDocument.h"

namespace Olympe
{
    // Forward declaration
    class IGraphRenderer;

    /**
     * @class EntityPrefabGraphDocumentV2
     * @brief Framework-compliant document model for Entity Prefab graphs
     * 
     * Implements IGraphDocument interface for seamless integration with CanvasFramework.
     * Manages component nodes, connections, and canvas state for entity prefab editing.
     * 
     * Data Model:
     * - Nodes: vector<ComponentNode> with unique nodeId
     * - Connections: vector<pair<sourceId, targetId>>
     * - Properties: per-node parameter values from schema
     * 
     * Persistence:
     * - Load: JSON file -> m_nodes, m_connections, canvas state
     * - Save: m_nodes, m_connections, canvas state -> JSON file
     * - Dirty tracking: Set on any graph modification, cleared on Load/Save
     */
    class EntityPrefabGraphDocumentV2 : public IGraphDocument
    {
    public:
        EntityPrefabGraphDocumentV2();
        ~EntityPrefabGraphDocumentV2();

        // ========== NODE MANAGEMENT ==========

        /**
         * Create a new component node
         * @param componentType Type of component (e.g., "Transform")
         * @return NodeId of created node (never InvalidNodeId on success)
         */
        PrefabNodeId CreateComponentNode(const std::string& componentType);

        /**
         * Create a new component node with custom name
         * @param componentType Type of component
         * @param componentName Display name for the node
         * @return NodeId of created node
         */
        PrefabNodeId CreateComponentNode(const std::string& componentType, const std::string& componentName);

        /**
         * Remove node and all its connections
         * @param nodeId Node to remove
         */
        void RemoveNode(PrefabNodeId nodeId);

        /**
         * Check if node exists
         * @param nodeId Node to check
         * @return true if node with this ID exists
         */
        bool HasNode(PrefabNodeId nodeId) const;

        /**
         * Get node by ID (mutable)
         * @param nodeId Node to retrieve
         * @return Pointer to node or nullptr if not found
         */
        ComponentNode* GetNode(PrefabNodeId nodeId);

        /**
         * Get node by ID (const)
         * @param nodeId Node to retrieve
         * @return Const pointer to node or nullptr if not found
         */
        const ComponentNode* GetNode(PrefabNodeId nodeId) const;

        /**
         * Get all nodes
         * @return Const reference to nodes vector
         */
        const std::vector<ComponentNode>& GetAllNodes() const;

        // ========== SELECTION MANAGEMENT ==========

        /**
         * Select a node
         * @param nodeId Node to select
         */
        void SelectNode(PrefabNodeId nodeId);

        /**
         * Deselect a node
         * @param nodeId Node to deselect
         */
        void DeselectNode(PrefabNodeId nodeId);

        /**
         * Deselect all nodes
         */
        void DeselectAll();

        /**
         * Get first selected node
         * @return First selected node ID or InvalidNodeId if none selected
         */
        PrefabNodeId GetSelectedNode() const;

        /**
         * Get all selected nodes
         * @return Vector of selected node IDs
         */
        const std::vector<PrefabNodeId>& GetSelectedNodes() const;

        // ========== CONNECTION MANAGEMENT ==========

        /**
         * Create connection between two nodes
         * @param sourceId Source node ID
         * @param targetId Target node ID
         * @return true if connection created, false if validation failed
         */
        bool ConnectNodes(PrefabNodeId sourceId, PrefabNodeId targetId);

        /**
         * Remove connection between two nodes
         * @param sourceId Source node ID
         * @param targetId Target node ID
         * @return true if connection removed, false if not found
         */
        bool DisconnectNodes(PrefabNodeId sourceId, PrefabNodeId targetId);

        /**
         * Get all connections
         * @return Vector of connection pairs
         */
        const std::vector<std::pair<PrefabNodeId, PrefabNodeId>>& GetConnections() const;

        /**
         * Validate if connection is allowed
         * @param sourceId Source node ID
         * @param targetId Target node ID
         * @return true if connection is valid (no self-connections, no duplicates)
         */
        bool ValidateConnection(PrefabNodeId sourceId, PrefabNodeId targetId) const;

        /**
         * Check if connection exists
         * @param sourceId Source node ID
         * @param targetId Target node ID
         * @return true if connection exists
         */
        bool HasConnection(PrefabNodeId sourceId, PrefabNodeId targetId) const;

        // ========== LAYOUT ==========

        /**
         * Auto-arrange nodes in grid
         */
        void AutoLayout();

        /**
         * Arrange nodes in grid pattern
         * @param gridWidth Number of columns
         * @param spacing Distance between nodes
         */
        void ArrangeNodesInGrid(int gridWidth = 4, float spacing = 200.0f);

        /**
         * Center viewport on graph
         */
        void CenterViewport();

        // ========== SERIALIZATION (IGraphDocument Implementation) ==========

        /**
         * Load document from file
         * @param filePath Path to .json file
         * @return true on success (nodes loaded, dirty flag cleared)
         */
        bool Load(const std::string& filePath) override;

        /**
         * Save document to file
         * @param filePath Path to save location
         * @return true on success (file written, dirty flag cleared)
         */
        bool Save(const std::string& filePath) override;

        /**
         * Get display name
         * @return Document name for UI display
         */
        std::string GetName() const override;

        /**
         * Get document type
         * @return DocumentType::ENTITY_PREFAB
         */
        DocumentType GetType() const override;

        /**
         * Get current file path
         * @return Path to current document file (empty if unsaved)
         */
        std::string GetFilePath() const override;

        /**
         * Update file path (called after SaveAs)
         * @param path New file path
         */
        void SetFilePath(const std::string& path) override;

        /**
         * Get renderer for canvas display
         * @return Non-owning pointer to renderer (never nullptr)
         */
        IGraphRenderer* GetRenderer() override;

        /**
         * Get renderer (const)
         * @return Const non-owning pointer to renderer
         */
        const IGraphRenderer* GetRenderer() const override;

        /**
         * Notify framework of modifications
         */
        void OnDocumentModified() override;

        // ========== PROPERTIES ==========

        /**
         * Set document name
         * @param name New name
         */
        void SetDocumentName(const std::string& name);

        /**
         * Get document name
         * @return Current name
         */
        std::string GetDocumentName() const;

        /**
         * Get canvas offset (pan position)
         * @return Current pan offset
         */
        Vector GetCanvasOffset() const;

        /**
         * Set canvas offset
         * @param offset New pan offset
         */
        void SetCanvasOffset(const Vector& offset);

        /**
         * Get canvas zoom level
         * @return Current zoom (e.g., 1.0 = 100%, 2.0 = 200%)
         */
        float GetCanvasZoom() const;

        /**
         * Set canvas zoom level
         * @param zoom New zoom level
         */
        void SetCanvasZoom(float zoom);

        /**
         * Check if document has unsaved changes
         * @return true if modified since last Load/Save
         */
        bool IsDirty() const override;

        /**
         * Set dirty flag
         * @param dirty true to mark as modified, false to mark as saved
         */
        void SetDirty(bool dirty);

        /**
         * Get node count
         * @return Number of nodes in document
         */
        size_t GetNodeCount() const;

        /**
         * Clear all data
         */
        void Clear();

        // ========== PARAMETER SCHEMA MANAGEMENT ==========

        /**
         * Load parameter schemas from JSON file
         * @param schemasFilePath Path to ComponentsParameters.json
         */
        void LoadParameterSchemas(const std::string& schemasFilePath);

        /**
         * Initialize node properties from schema
         * @param node Node to initialize
         */
        void InitializeNodeProperties(ComponentNode& node);

    private:
        // ========== DATA MEMBERS ==========

        std::vector<ComponentNode> m_nodes;
        std::vector<PrefabNodeId> m_selectedNodes;
        std::vector<std::pair<PrefabNodeId, PrefabNodeId>> m_connections;
        std::string m_documentName;
        std::string m_filePath;
        Vector m_canvasOffset;
        float m_canvasZoom = 1.0f;
        PrefabNodeId m_nextNodeId = 1;
        bool m_isDirty = false;

        // Parameter schemas: componentType -> { paramName -> defaultValue }
        std::map<std::string, std::map<std::string, std::string>> m_parameterSchemas;

        // Renderer (non-owned, managed by framework)
        IGraphRenderer* m_renderer = nullptr;

        // ========== PRIVATE HELPERS ==========

        /**
         * Generate unique node ID
         * @return New unique ID
         */
        PrefabNodeId GenerateNodeId();

        /**
         * Calculate layout for nodes
         * @return Layout information for auto-layout
         */
        struct LayoutNode
        {
            PrefabNodeId nodeId = InvalidNodeId;
            Vector position;
            Vector size;
            std::vector<PrefabNodeId> inputs;
            std::vector<PrefabNodeId> outputs;
        };

        std::vector<LayoutNode> CalculateLayout();

        /**
         * Internal file loading (called by Load)
         * @param filePath Path to load from
         * @return true on success
         */
        bool LoadFromFile(const std::string& filePath);

        /**
         * Internal file saving (called by Save)
         * @param filePath Path to save to
         * @return true on success
         */
        bool SaveToFile(const std::string& filePath);
    };

} // namespace Olympe
