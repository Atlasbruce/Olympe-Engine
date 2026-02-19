/**
 * @file GraphDocument.cpp
 * @brief Implementation of GraphDocument class
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "GraphDocument.h"
#include "../system/system_utils.h"
#include <algorithm>

using json = nlohmann::json;

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Constructor / Destructor
// ============================================================================

GraphDocument::GraphDocument()
    : type("AIGraph")
    , graphKind("BehaviorTree")
    , m_nextNodeId(1)
    , m_nextLinkId(1)
    , m_isDirty(false)
{
    metadata = json::object();
}

GraphDocument::~GraphDocument()
{
}

// ============================================================================
// CRUD Operations - Nodes
// ============================================================================

NodeId GraphDocument::CreateNode(const std::string& nodeType, Vector2 pos)
{
    NodeData node;
    node.id.value = m_nextNodeId++;
    node.type = nodeType;
    node.name = nodeType;
    node.position = pos;
    
    m_nodes.push_back(node);
    m_isDirty = true;
    
    return node.id;
}

bool GraphDocument::DeleteNode(NodeId id)
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](const NodeData& n) { return n.id == id; });
    
    if (it != m_nodes.end())
    {
        m_nodes.erase(it);
        m_isDirty = true;
        
        // Remove links connected to this node
        m_links.erase(
            std::remove_if(m_links.begin(), m_links.end(),
                [id](const LinkData& link) {
                    // We would need to check pin ownership here
                    // For now, just keep links
                    return false;
                }),
            m_links.end()
        );
        
        return true;
    }
    
    return false;
}

bool GraphDocument::UpdateNodePosition(NodeId id, Vector2 newPos)
{
    NodeData* node = GetNode(id);
    if (node != nullptr)
    {
        node->position = newPos;
        m_isDirty = true;
        return true;
    }
    return false;
}

bool GraphDocument::UpdateNodeParameters(NodeId id, const std::map<std::string, std::string>& params)
{
    NodeData* node = GetNode(id);
    if (node != nullptr)
    {
        node->parameters = params;
        m_isDirty = true;
        return true;
    }
    return false;
}

NodeData* GraphDocument::GetNode(NodeId id)
{
    for (auto& node : m_nodes)
    {
        if (node.id == id)
            return &node;
    }
    return nullptr;
}

const NodeData* GraphDocument::GetNode(NodeId id) const
{
    for (const auto& node : m_nodes)
    {
        if (node.id == id)
            return &node;
    }
    return nullptr;
}

bool GraphDocument::UpdateNode(NodeId nodeId, const NodeData& newData)
{
    // Find node in document
    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        if (m_nodes[i].id.value == nodeId.value)
        {
            // Update node data while preserving ID
            NodeId originalId = m_nodes[i].id;
            m_nodes[i] = newData;
            m_nodes[i].id = originalId; // Preserve original ID

            SYSTEM_LOG << "[GraphDocument] Updated node ID="
                << nodeId.value
                << " type=" << newData.type
                << " pos=(" << newData.position.x
                << "," << newData.position.y << ")"
                << std::endl;

            return true;
        }
    }

    SYSTEM_LOG << "[GraphDocument] WARNING: UpdateNode failed - node ID="
        << nodeId.value << " not found" << std::endl;
    return false;
}

// ============================================================================
// CRUD Operations - Links
// ============================================================================

LinkId GraphDocument::ConnectPins(PinId fromPin, PinId toPin)
{
    LinkData link;
    link.id.value = m_nextLinkId++;
    link.fromPin = fromPin;
    link.toPin = toPin;
    
    m_links.push_back(link);
    m_isDirty = true;
    
    return link.id;
}

bool GraphDocument::DisconnectLink(LinkId id)
{
    auto it = std::find_if(m_links.begin(), m_links.end(),
        [id](const LinkData& l) { return l.id == id; });
    
    if (it != m_links.end())
    {
        m_links.erase(it);
        m_isDirty = true;
        return true;
    }
    
    return false;
}

LinkData* GraphDocument::GetLink(LinkId id)
{
    for (auto& link : m_links)
    {
        if (link.id == id)
            return &link;
    }
    return nullptr;
}

const LinkData* GraphDocument::GetLink(LinkId id) const
{
    for (const auto& link : m_links)
    {
        if (link.id == id)
            return &link;
    }
    return nullptr;
}

// ============================================================================
// Validation
// ============================================================================

bool GraphDocument::ValidateGraph(std::string& errorMessage) const
{
    // Check if we have at least one node
    if (m_nodes.empty())
    {
        errorMessage = "Graph has no nodes";
        return false;
    }
    
    // Check if root node exists
    if (rootNodeId.value != 0)
    {
        const NodeData* rootNode = GetNode(rootNodeId);
        if (rootNode == nullptr)
        {
            errorMessage = "Root node not found";
            return false;
        }
    }
    
    // Check for cycles
    if (HasCycles())
    {
        errorMessage = "Graph contains cycles";
        return false;
    }
    
    // Validate composite nodes have children
    for (const auto& node : m_nodes)
    {
        if (node.type == "BT_Selector" || node.type == "BT_Sequence")
        {
            if (node.children.empty())
            {
                errorMessage = "Composite node '" + node.name + "' has 0 children";
                return false;
            }
        }
    }
    
    return true;
}

bool GraphDocument::HasCycles() const
{
    if (m_nodes.empty())
        return false;
    
    std::vector<NodeId> visited;
    std::vector<NodeId> recursionStack;
    
    for (const auto& node : m_nodes)
    {
        if (std::find(visited.begin(), visited.end(), node.id) == visited.end())
        {
            if (HasCyclesHelper(node.id, visited, recursionStack))
                return true;
        }
    }
    
    return false;
}

bool GraphDocument::HasCyclesHelper(NodeId nodeId, std::vector<NodeId>& visited, std::vector<NodeId>& recursionStack) const
{
    visited.push_back(nodeId);
    recursionStack.push_back(nodeId);
    
    const NodeData* node = GetNode(nodeId);
    if (node == nullptr)
        return false;
    
    // Check children
    for (const auto& childId : node->children)
    {
        if (std::find(visited.begin(), visited.end(), childId) == visited.end())
        {
            if (HasCyclesHelper(childId, visited, recursionStack))
                return true;
        }
        else if (std::find(recursionStack.begin(), recursionStack.end(), childId) != recursionStack.end())
        {
            return true;
        }
    }
    
    // Check decorator child
    if (node->decoratorChild.value != 0)
    {
        if (std::find(visited.begin(), visited.end(), node->decoratorChild) == visited.end())
        {
            if (HasCyclesHelper(node->decoratorChild, visited, recursionStack))
                return true;
        }
        else if (std::find(recursionStack.begin(), recursionStack.end(), node->decoratorChild) != recursionStack.end())
        {
            return true;
        }
    }
    
    recursionStack.erase(std::remove(recursionStack.begin(), recursionStack.end(), nodeId), recursionStack.end());
    return false;
}

// ============================================================================
// Serialization
// ============================================================================

json GraphDocument::ToJson() const
{
    json j = json::object();
    
    j["schemaVersion"] = 2;
    j["type"] = type;
    j["graphKind"] = graphKind;
    j["metadata"] = metadata;
    
    // Editor state
    json editorStateJson = json::object();
    editorStateJson["zoom"] = editorState.zoom;
    
    json scrollJson = json::object();
    scrollJson["x"] = editorState.scrollOffset.x;
    scrollJson["y"] = editorState.scrollOffset.y;
    editorStateJson["scrollOffset"] = scrollJson;
    
    json selectedJson = json::array();
    for (const auto& nodeId : editorState.selectedNodes)
    {
        selectedJson.push_back(static_cast<int>(nodeId.value));
    }
    editorStateJson["selectedNodes"] = selectedJson;
    editorStateJson["layoutDirection"] = editorState.layoutDirection;
    
    j["editorState"] = editorStateJson;
    
    // Data section
    json dataJson = json::object();
    dataJson["rootNodeId"] = static_cast<int>(rootNodeId.value);
    
    // Nodes
    json nodesJson = json::array();
    for (const auto& node : m_nodes)
    {
        json nodeJson = json::object();
        nodeJson["id"] = static_cast<int>(node.id.value);
        nodeJson["type"] = node.type;
        nodeJson["name"] = node.name;
        
        json posJson = json::object();
        posJson["x"] = node.position.x;
        posJson["y"] = node.position.y;
        nodeJson["position"] = posJson;
        
        // Children
        json childrenJson = json::array();
        for (const auto& childId : node.children)
        {
            childrenJson.push_back(static_cast<int>(childId.value));
        }
        nodeJson["children"] = childrenJson;
        
        // Parameters
        json paramsJson = json::object();
        for (auto it = node.parameters.begin(); it != node.parameters.end(); ++it)
        {
            paramsJson[it->first] = it->second;
        }
        nodeJson["parameters"] = paramsJson;
        
        // Decorator child
        if (node.decoratorChild.value != 0)
        {
            nodeJson["decoratorChildId"] = static_cast<int>(node.decoratorChild.value);
        }
        
        nodesJson.push_back(nodeJson);
    }
    dataJson["nodes"] = nodesJson;
    
    // Links
    json linksJson = json::array();
    for (const auto& link : m_links)
    {
        json linkJson = json::object();
        linkJson["id"] = static_cast<int>(link.id.value);
        
        json fromPinJson = json::object();
        fromPinJson["nodeId"] = static_cast<int>(link.fromPin.value);
        fromPinJson["pinId"] = "output";
        linkJson["fromPin"] = fromPinJson;
        
        json toPinJson = json::object();
        toPinJson["nodeId"] = static_cast<int>(link.toPin.value);
        toPinJson["pinId"] = "input";
        linkJson["toPin"] = toPinJson;
        
        linksJson.push_back(linkJson);
    }
    dataJson["links"] = linksJson;
    
    j["data"] = dataJson;
    
    return j;
}

GraphDocument GraphDocument::FromJson(const json& j)
{
    GraphDocument doc;
    
    // Basic properties
    doc.type = JsonHelper::GetString(j, "type", "AIGraph");
    doc.graphKind = JsonHelper::GetString(j, "graphKind", "BehaviorTree");
    
    // Metadata
    if (j.contains("metadata") && j["metadata"].is_object())
    {
        doc.metadata = j["metadata"];
    }
    
    // Editor state
    if (j.contains("editorState") && j["editorState"].is_object())
    {
        const json& es = j["editorState"];
        doc.editorState.zoom = JsonHelper::GetFloat(es, "zoom", 1.0f);
        doc.editorState.layoutDirection = JsonHelper::GetString(es, "layoutDirection", "TopToBottom");
        
        if (es.contains("scrollOffset") && es["scrollOffset"].is_object())
        {
            const json& scroll = es["scrollOffset"];
            doc.editorState.scrollOffset.x = JsonHelper::GetFloat(scroll, "x", 0.0f);
            doc.editorState.scrollOffset.y = JsonHelper::GetFloat(scroll, "y", 0.0f);
        }
        
        if (es.contains("selectedNodes") && es["selectedNodes"].is_array())
        {
            const json& selArray = es["selectedNodes"];
            for (size_t i = 0; i < selArray.size(); ++i)
            {
                if (selArray[i].is_number())
                {
                    NodeId nodeId;
                    nodeId.value = selArray[i].get<uint32_t>();
                    doc.editorState.selectedNodes.push_back(nodeId);
                }
            }
        }
    }
    
    // Data section
    if (j.contains("data") && j["data"].is_object())
    {
        const json& data = j["data"];
        
        doc.rootNodeId.value = JsonHelper::GetUInt(data, "rootNodeId", 0);
        
        // Nodes
        if (data.contains("nodes") && data["nodes"].is_array())
        {
            const json& nodesArray = data["nodes"];
            for (size_t i = 0; i < nodesArray.size(); ++i)
            {
                const json& nodeJson = nodesArray[i];
                
                NodeData node;
                node.id.value = JsonHelper::GetUInt(nodeJson, "id", 0);
                node.type = JsonHelper::GetString(nodeJson, "type", "");
                node.name = JsonHelper::GetString(nodeJson, "name", "");
                
                if (nodeJson.contains("position") && nodeJson["position"].is_object())
                {
                    const json& pos = nodeJson["position"];
                    node.position.x = JsonHelper::GetFloat(pos, "x", 0.0f);
                    node.position.y = JsonHelper::GetFloat(pos, "y", 0.0f);
                }
                
                // Children
                if (nodeJson.contains("children") && nodeJson["children"].is_array())
                {
                    const json& childrenArray = nodeJson["children"];
                    for (size_t c = 0; c < childrenArray.size(); ++c)
                    {
                        if (childrenArray[c].is_number())
                        {
                            NodeId childId;
                            childId.value = childrenArray[c].get<uint32_t>();
                            node.children.push_back(childId);
                        }
                    }
                }
                
                // Parameters
                if (nodeJson.contains("parameters") && nodeJson["parameters"].is_object())
                {
                    const json& params = nodeJson["parameters"];
                    for (auto it = params.begin(); it != params.end(); ++it)
                    {
                        std::string key = it.key();
                        std::string value;
                        if (it.value().is_string())
                            value = it.value().get<std::string>();
                        else if (it.value().is_number())
                            value = std::to_string(it.value().get<double>());
                        else if (it.value().is_boolean())
                            value = it.value().get<bool>() ? "true" : "false";
                        
                        node.parameters[key] = value;
                    }
                }
                
                // Decorator child
                node.decoratorChild.value = JsonHelper::GetUInt(nodeJson, "decoratorChildId", 0);
                
                doc.m_nodes.push_back(node);
                
                // Update next node ID
                if (node.id.value >= doc.m_nextNodeId)
                {
                    doc.m_nextNodeId = node.id.value + 1;
                }
            }
        }
        
        // Links
        if (data.contains("links") && data["links"].is_array())
        {
            const json& linksArray = data["links"];
            for (size_t i = 0; i < linksArray.size(); ++i)
            {
                const json& linkJson = linksArray[i];
                
                LinkData link;
                link.id.value = JsonHelper::GetUInt(linkJson, "id", 0);
                
                if (linkJson.contains("fromPin") && linkJson["fromPin"].is_object())
                {
                    const json& fromPin = linkJson["fromPin"];
                    link.fromPin.value = JsonHelper::GetUInt(fromPin, "nodeId", 0);
                }
                
                if (linkJson.contains("toPin") && linkJson["toPin"].is_object())
                {
                    const json& toPin = linkJson["toPin"];
                    link.toPin.value = JsonHelper::GetUInt(toPin, "nodeId", 0);
                }
                
                doc.m_links.push_back(link);
                
                // Update next link ID
                if (link.id.value >= doc.m_nextLinkId)
                {
                    doc.m_nextLinkId = link.id.value + 1;
                }
            }
        }
    }
    
    doc.m_isDirty = false;
    return doc;
}

// ============================================================================
// Auto-Layout
// ============================================================================

bool GraphDocument::AutoLayout(const AutoLayoutConfig& config)
{
    // Validate layout direction
    if (config.direction == LayoutDirection::LeftToRight || 
        config.direction == LayoutDirection::RightToLeft)
    {
        SYSTEM_LOG << "[GraphDocument] AutoLayout failed: LeftToRight and RightToLeft not yet implemented" << std::endl;
        return false;
    }
    
    // Validate graph has root node
    if (rootNodeId.value == 0)
    {
        SYSTEM_LOG << "[GraphDocument] AutoLayout failed: No root node defined" << std::endl;
        return false;
    }
    
    if (m_nodes.empty())
    {
        SYSTEM_LOG << "[GraphDocument] AutoLayout failed: Graph is empty" << std::endl;
        return false;
    }
    
    // Check root node exists
    const NodeData* rootNode = GetNode(rootNodeId);
    if (rootNode == nullptr)
    {
        SYSTEM_LOG << "[GraphDocument] AutoLayout failed: Root node not found" << std::endl;
        return false;
    }
    
    SYSTEM_LOG << "[GraphDocument] Starting auto-layout from root node ID=" << rootNodeId.value << std::endl;
    
    // Track visited nodes for cycle detection
    std::map<NodeId, bool> visited;
    
    // Start layout from root
    AutoLayoutNode(rootNodeId, config, config.paddingX, config.paddingY, 0, visited);
    
    // Mark document as modified
    m_isDirty = true;
    
    SYSTEM_LOG << "[GraphDocument] Auto-layout completed successfully" << std::endl;
    return true;
}

float GraphDocument::AutoLayoutNode(
    NodeId nodeId,
    const AutoLayoutConfig& config,
    float startX,
    float startY,
    int depth,
    std::map<NodeId, bool>& visited)
{
    // Cycle detection
    auto it = visited.find(nodeId);
    if (it != visited.end() && it->second)
    {
        SYSTEM_LOG << "[GraphDocument] AutoLayout: Cycle detected at node ID=" << nodeId.value << std::endl;
        return config.nodeWidth + config.horizontalSpacing;
    }
    
    visited[nodeId] = true;
    
    // Get node data
    NodeData* node = GetNode(nodeId);
    if (node == nullptr)
    {
        return config.nodeWidth + config.horizontalSpacing;
    }
    
    // Calculate Y position based on depth
    float nodeY = 0.0f;
    if (config.direction == LayoutDirection::TopToBottom)
    {
        nodeY = startY + static_cast<float>(depth) * config.verticalSpacing;
    }
    else // BottomToTop
    {
        nodeY = startY - static_cast<float>(depth) * config.verticalSpacing;
    }
    
    // Calculate total width of children
    float totalChildrenWidth = 0.0f;
    float childX = startX;
    
    for (size_t i = 0; i < node->children.size(); ++i)
    {
        float childWidth = AutoLayoutNode(
            node->children[i],
            config,
            childX,
            startY,
            depth + 1,
            visited
        );
        
        totalChildrenWidth += childWidth;
        childX += childWidth;
    }
    
    // Calculate X position for this node
    float nodeX = 0.0f;
    if (node->children.empty())
    {
        // Leaf node - use startX
        nodeX = startX;
    }
    else
    {
        // Center above children
        // Each child returns (width + spacing), so last child has trailing spacing
        // Subtract one spacing to get the actual span occupied by children
        float childrenSpan = totalChildrenWidth - config.horizontalSpacing;
        nodeX = startX + childrenSpan * 0.5f - config.nodeWidth * 0.5f;
    }
    
    // Apply position to node
    Vector2 newPos;
    newPos.x = nodeX;
    newPos.y = nodeY;
    UpdateNodePosition(nodeId, newPos);
    
    // Handle decorator child (place to the right of parent)
    if (node->decoratorChild.value != 0)
    {
        float decoratorX = nodeX + config.nodeWidth + config.horizontalSpacing;
        AutoLayoutNode(
            node->decoratorChild,
            config,
            decoratorX,
            nodeY,
            depth,
            visited
        );
    }
    
    // Return width consumed by this subtree
    if (totalChildrenWidth > 0.0f)
    {
        return totalChildrenWidth;
    }
    else
    {
        return config.nodeWidth + config.horizontalSpacing;
    }
}

} // namespace NodeGraph
} // namespace Olympe
