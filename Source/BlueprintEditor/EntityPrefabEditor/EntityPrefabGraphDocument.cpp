#include "EntityPrefabGraphDocument.h"
#include <glm/geometric.hpp>
#include <algorithm>
#include <cmath>

namespace OlympeEngine {

// ============================================================================
// Constructor & Destructor
// ============================================================================

EntityPrefabGraphDocument::EntityPrefabGraphDocument()
    : GraphDocument(), m_entityCenterNodeId(NodeGraph::InvalidNodeId) {
}

EntityPrefabGraphDocument::~EntityPrefabGraphDocument() {
    m_componentNodes.clear();
    m_selectedNodes.clear();
}

// ============================================================================
// Prefab Loading & Conversion
// ============================================================================

bool EntityPrefabGraphDocument::LoadFromPrefab(const EntityPrefab& prefab) {
    // Store the prefab
    m_prefab = prefab;
    m_componentNodes.clear();
    m_selectedNodes.clear();
    m_validationErrors.clear();

    // Create entity center node
    CreateEntityCenterNode();

    // Create component nodes
    int componentIndex = 0;
    for (const auto& component : prefab.components) {
        glm::vec2 position = CalculateLayoutPosition(
            componentIndex,
            prefab.components.size(),
            300.0f
        );

        CreateComponentNode(component.type, component.name, position);
        componentIndex++;
    }

    return true;
}

bool EntityPrefabGraphDocument::LoadFromFile(const std::string& filePath) {
    EntityPrefab prefab = PrefabLoader::LoadFromFile(filePath);
    if (prefab.name.empty() && PrefabLoader::GetValidationErrors().size() > 0) {
        m_validationErrors = PrefabLoader::GetValidationErrors();
        return false;
    }
    return LoadFromPrefab(prefab);
}

EntityPrefab EntityPrefabGraphDocument::SerializeToPrefab() const {
    // Update component data from nodes
    EntityPrefab prefab = m_prefab;
    prefab.components.clear();

    for (const auto& [nodeId, node] : m_componentNodes) {
        if (node.isEntityCenter) continue;  // Skip entity center

        ComponentData data;
        data.type = node.componentType;
        data.name = node.componentName;
        data.enabled = true;

        // Get properties from node if they exist
        // This is a placeholder - actual properties stored elsewhere

        prefab.components.push_back(data);
    }

    return prefab;
}

// ============================================================================
// Component Node Operations
// ============================================================================

NodeGraph::NodeId EntityPrefabGraphDocument::CreateComponentNode(
    const std::string& componentType,
    const std::string& componentName,
    const glm::vec2& position
) {
    // Create node in base GraphDocument (generates nodeId)
    NodeGraph::NodeId nodeId = GraphDocument::CreateNode();

    // Create visual representation
    ComponentNode node;
    node.nodeId = nodeId;
    node.componentType = componentType;
    node.componentName = componentName;
    node.position = position;
    node.isEntityCenter = false;

    // Get schema and setup pins
    auto schema = PrefabLoader::GetComponentSchema(componentType);
    if (schema) {
        // For now, use a simple pattern: input and output pins based on parameter count
        node.inputPins = std::max(1, static_cast<int>(schema->parameters.size()) / 3);
        node.outputPins = 1;
    } else {
        node.inputPins = 1;
        node.outputPins = 1;
    }

    m_componentNodes[nodeId] = node;
    return nodeId;
}

bool EntityPrefabGraphDocument::RemoveComponentNode(NodeGraph::NodeId nodeId) {
    auto it = m_componentNodes.find(nodeId);
    if (it == m_componentNodes.end()) {
        return false;
    }

    m_componentNodes.erase(it);

    // Remove from selection
    auto selIt = std::find(m_selectedNodes.begin(), m_selectedNodes.end(), nodeId);
    if (selIt != m_selectedNodes.end()) {
        m_selectedNodes.erase(selIt);
    }

    return GraphDocument::DeleteNode(nodeId);
}

bool EntityPrefabGraphDocument::UpdateComponentProperties(
    NodeGraph::NodeId nodeId,
    const ComponentData& data
) {
    auto it = m_componentNodes.find(nodeId);
    if (it == m_componentNodes.end()) {
        return false;
    }

    it->second.componentType = data.type;
    it->second.componentName = data.name;

    RefreshNodePinCounts(it->second);
    PropagateNodeChanges(nodeId);

    return true;
}

ComponentData EntityPrefabGraphDocument::GetComponentData(NodeGraph::NodeId nodeId) const {
    auto it = m_componentNodes.find(nodeId);
    if (it == m_componentNodes.end()) {
        return ComponentData();
    }

    ComponentData data;
    data.type = it->second.componentType;
    data.name = it->second.componentName;
    data.enabled = true;

    return data;
}

// ============================================================================
// Node Queries
// ============================================================================

ComponentNode* EntityPrefabGraphDocument::GetComponentNode(NodeGraph::NodeId nodeId) {
    auto it = m_componentNodes.find(nodeId);
    if (it == m_componentNodes.end()) {
        return nullptr;
    }
    return &it->second;
}

const ComponentNode* EntityPrefabGraphDocument::GetComponentNode(NodeGraph::NodeId nodeId) const {
    auto it = m_componentNodes.find(nodeId);
    if (it == m_componentNodes.end()) {
        return nullptr;
    }
    return &it->second;
}

std::vector<ComponentNode*> EntityPrefabGraphDocument::GetAllComponentNodes() {
    std::vector<ComponentNode*> nodes;
    for (auto& [nodeId, node] : m_componentNodes) {
        nodes.push_back(&node);
    }
    return nodes;
}

std::vector<const ComponentNode*> EntityPrefabGraphDocument::GetAllComponentNodes() const {
    std::vector<const ComponentNode*> nodes;
    for (const auto& [nodeId, node] : m_componentNodes) {
        nodes.push_back(&node);
    }
    return nodes;
}

// ============================================================================
// Layout & Visualization
// ============================================================================

void EntityPrefabGraphDocument::AutoLayoutComponents(float radius) {
    auto nodes = GetAllComponentNodes();
    int index = 0;

    for (auto* node : nodes) {
        if (node->isEntityCenter) {
            node->position = glm::vec2(0.0f, 0.0f);
        } else {
            node->position = CalculateLayoutPosition(index, nodes.size() - 1, radius);
            index++;
        }
    }
}

void EntityPrefabGraphDocument::UpdateNodePosition(NodeGraph::NodeId nodeId, const glm::vec2& newPosition) {
    auto it = m_componentNodes.find(nodeId);
    if (it != m_componentNodes.end()) {
        it->second.position = newPosition;
        PropagateNodeChanges(nodeId);
    }
}

glm::vec2 EntityPrefabGraphDocument::GetNodePosition(NodeGraph::NodeId nodeId) const {
    auto it = m_componentNodes.find(nodeId);
    if (it != m_componentNodes.end()) {
        return it->second.position;
    }
    return glm::vec2(0.0f, 0.0f);
}

// ============================================================================
// Selection Management
// ============================================================================

void EntityPrefabGraphDocument::SelectNode(NodeGraph::NodeId nodeId) {
    auto it = m_componentNodes.find(nodeId);
    if (it != m_componentNodes.end()) {
        it->second.selected = true;
        m_selectedNodes.push_back(nodeId);
    }
}

void EntityPrefabGraphDocument::DeselectNode(NodeGraph::NodeId nodeId) {
    auto it = m_componentNodes.find(nodeId);
    if (it != m_componentNodes.end()) {
        it->second.selected = false;
    }

    auto selIt = std::find(m_selectedNodes.begin(), m_selectedNodes.end(), nodeId);
    if (selIt != m_selectedNodes.end()) {
        m_selectedNodes.erase(selIt);
    }
}

void EntityPrefabGraphDocument::DeselectAll() {
    for (auto nodeId : m_selectedNodes) {
        auto it = m_componentNodes.find(nodeId);
        if (it != m_componentNodes.end()) {
            it->second.selected = false;
        }
    }
    m_selectedNodes.clear();
}

std::vector<NodeGraph::NodeId> EntityPrefabGraphDocument::GetSelectedNodes() const {
    return m_selectedNodes;
}

// ============================================================================
// Validation
// ============================================================================

bool EntityPrefabGraphDocument::ValidateDocument() const {
    m_validationErrors.clear();

    // Validate the stored prefab
    if (!PrefabLoader::ValidateAgainstSchemas(m_prefab)) {
        m_validationErrors = PrefabLoader::GetValidationErrors();
        return false;
    }

    return true;
}

std::vector<std::string> EntityPrefabGraphDocument::GetValidationErrors() const {
    return m_validationErrors;
}

// ============================================================================
// Entity Center Node
// ============================================================================

void EntityPrefabGraphDocument::CreateEntityCenterNode() {
    NodeGraph::NodeId nodeId = GraphDocument::CreateNode();

    ComponentNode node;
    node.nodeId = nodeId;
    node.componentType = "Entity";
    node.componentName = m_prefab.name;
    node.position = glm::vec2(0.0f, 0.0f);
    node.isEntityCenter = true;
    node.inputPins = 0;
    node.outputPins = static_cast<int>(m_prefab.components.size());

    m_entityCenterNodeId = nodeId;
    m_componentNodes[nodeId] = node;
}

// ============================================================================
// Undo/Redo Support
// ============================================================================

void EntityPrefabGraphDocument::BeginAction(const std::string& actionName) {
    m_isInAction = true;
    // In Phase 2+, this will create a CommandStack transaction
}

void EntityPrefabGraphDocument::EndAction() {
    m_isInAction = false;
}

// ============================================================================
// Export & Statistics
// ============================================================================

int EntityPrefabGraphDocument::GetComponentCount() const {
    return static_cast<int>(m_componentNodes.size()) - 1;  // -1 for entity center
}

int EntityPrefabGraphDocument::GetNodeCount() const {
    return static_cast<int>(m_componentNodes.size());
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void EntityPrefabGraphDocument::RefreshNodePinCounts(ComponentNode& node) {
    auto schema = PrefabLoader::GetComponentSchema(node.componentType);
    if (schema) {
        node.inputPins = std::max(1, static_cast<int>(schema->parameters.size()) / 3);
        node.outputPins = 1;
    }
}

glm::vec2 EntityPrefabGraphDocument::CalculateLayoutPosition(
    int componentIndex,
    int totalComponents,
    float radius
) {
    if (totalComponents <= 0) return glm::vec2(0.0f, 0.0f);

    // Circle layout around entity center
    float angle = (2.0f * 3.14159265359f * componentIndex) / totalComponents;
    glm::vec2 position;
    position.x = radius * std::cos(angle);
    position.y = radius * std::sin(angle);

    return position;
}

void EntityPrefabGraphDocument::PropagateNodeChanges(NodeGraph::NodeId nodeId) {
    // Placeholder for change propagation
    // In Phase 2+, this will trigger validation and UI updates
}

}  // namespace OlympeEngine
