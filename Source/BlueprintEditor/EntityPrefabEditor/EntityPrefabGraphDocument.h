#pragma once

#include "PrefabLoader.h"
#include "../NodeGraphCore/GraphDocument.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>

namespace OlympeEngine {

// ============================================================================
// ComponentNode - Visual representation of a component
// ============================================================================

struct ComponentNode {
    NodeGraph::NodeId nodeId;
    std::string componentType;
    std::string componentName;
    glm::vec2 position;
    bool selected = false;
    bool isEntityCenter = false;  // Special node for entity center
    int inputPins;
    int outputPins;
};

// ============================================================================
// EntityPrefabGraphDocument - Extends GraphDocument for prefab editing
// ============================================================================

class EntityPrefabGraphDocument : public GraphDocument {
public:
    EntityPrefabGraphDocument();
    ~EntityPrefabGraphDocument();

    // Prefab Loading & Conversion
    bool LoadFromPrefab(const EntityPrefab& prefab);
    bool LoadFromFile(const std::string& filePath);
    EntityPrefab SerializeToPrefab() const;

    // Component Node Operations
    NodeGraph::NodeId CreateComponentNode(
        const std::string& componentType,
        const std::string& componentName,
        const glm::vec2& position
    );

    bool RemoveComponentNode(NodeGraph::NodeId nodeId);
    bool UpdateComponentProperties(NodeGraph::NodeId nodeId, const ComponentData& data);
    ComponentData GetComponentData(NodeGraph::NodeId nodeId) const;

    // Node Queries
    ComponentNode* GetComponentNode(NodeGraph::NodeId nodeId);
    const ComponentNode* GetComponentNode(NodeGraph::NodeId nodeId) const;
    std::vector<ComponentNode*> GetAllComponentNodes();
    std::vector<const ComponentNode*> GetAllComponentNodes() const;

    // Layout & Visualization
    void AutoLayoutComponents(float radius = 300.0f);
    void UpdateNodePosition(NodeGraph::NodeId nodeId, const glm::vec2& newPosition);
    glm::vec2 GetNodePosition(NodeGraph::NodeId nodeId) const;

    // Selection Management
    void SelectNode(NodeGraph::NodeId nodeId);
    void DeselectNode(NodeGraph::NodeId nodeId);
    void DeselectAll();
    std::vector<NodeGraph::NodeId> GetSelectedNodes() const;

    // Validation
    bool ValidateDocument() const;
    std::vector<std::string> GetValidationErrors() const;

    // Entity Center Node
    NodeGraph::NodeId GetEntityCenterNode() const { return m_entityCenterNodeId; }
    void CreateEntityCenterNode();

    // Undo/Redo Support
    void BeginAction(const std::string& actionName);
    void EndAction();

    // Export & Statistics
    int GetComponentCount() const;
    int GetNodeCount() const;
    const EntityPrefab& GetPrefab() const { return m_prefab; }

private:
    // Core data
    EntityPrefab m_prefab;
    std::map<NodeGraph::NodeId, ComponentNode> m_componentNodes;
    NodeGraph::NodeId m_entityCenterNodeId;

    // State tracking
    std::vector<NodeGraph::NodeId> m_selectedNodes;
    std::vector<std::string> m_validationErrors;
    bool m_isInAction = false;

    // Helper methods
    void RefreshNodePinCounts(ComponentNode& node);
    glm::vec2 CalculateLayoutPosition(int componentIndex, int totalComponents, float radius);
    void PropagateNodeChanges(NodeGraph::NodeId nodeId);
};

}  // namespace OlympeEngine
