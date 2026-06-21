/**
 * @file GraphDocument.cpp
 * @brief Implementation of GraphDocument class
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "GraphDocument.h"
#include "../system/system_utils.h"
#include <algorithm>
#include <unordered_map>
#include <cstdint>

using json = nlohmann::json;

namespace Olympe {
namespace NodeGraphTypes {

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
        // Erase the node
        m_nodes.erase(it);
        m_isDirty = true;

        // Remove links that reference this node.
        // Historically link.fromPin.value and link.toPin.value could be stored
        // either as a canonical node id or as legacy ImNodes-encoded pin ids
        // (nodeUid*2 + attr). Use the same resolution heuristics as ToJson()
        // to determine which node a stored value refers to.
        auto resolveStoredPinToNodeId = [this](uint32_t stored)->uint32_t {
            // If it's already a valid node id, return it
            for (const auto& n : m_nodes) if (n.id.value == stored) return stored;
            // Try common raw pin encodings used historically by ImNodes/legacy
            if (stored > 1)
            {
                uint32_t cand = stored / 2u;
                for (const auto& n : m_nodes) if (n.id.value == cand) return cand;

                cand = stored / 100u;
                for (const auto& n : m_nodes) if (n.id.value == cand) return cand;

                cand = stored / 10000u;
                for (const auto& n : m_nodes) if (n.id.value == cand) return cand;
            }
            // Not resolvable -> return UINT32_MAX as sentinel
            return UINT32_MAX;
        };

        m_links.erase(
            std::remove_if(m_links.begin(), m_links.end(),
                [id, &resolveStoredPinToNodeId](const LinkData& link) {
                    uint32_t fromNode = resolveStoredPinToNodeId(static_cast<uint32_t>(link.fromPin.value));
                    uint32_t toNode = resolveStoredPinToNodeId(static_cast<uint32_t>(link.toPin.value));
                    return (fromNode == id.value) || (toNode == id.value);
                }),
            m_links.end()
        );

        // Also remove any lingering references to this node in other nodes
        // (children lists and decoratorChild). This keeps NodeData consistent
        // after a deletion.
        for (auto& n : m_nodes)
        {
            // Remove from children vector
            n.children.erase(
                std::remove_if(n.children.begin(), n.children.end(),
                    [id](const NodeId& cid) { return cid == id; }),
                n.children.end()
            );

            // Clear decorator child if it referenced the deleted node
            if (n.decoratorChild == id)
            {
                n.decoratorChild = NodeId{0};
            }
        }

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
    // Ensure blueprintType is always present for downstream tools that rely on it
    if (!graphKind.empty())
    {
        j["blueprintType"] = graphKind;
    }
    else
    {
        // Fallback to the generic type field if graphKind is not set
        j["blueprintType"] = type;
    }
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

        // Optional editor-only ImNodes UID
        if (node.imnodesUid != 0)
        {
            nodeJson["imnodesUid"] = static_cast<int>(node.imnodesUid);
        }
        
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

        // Helper: try to resolve stored pin value to an actual node id present in document.
        auto resolveStoredPinToNodeId = [&](uint32_t stored)->uint32_t {
            // If it's already a valid node id, return it
            for (const auto& n : m_nodes) if (n.id.value == stored) return stored;
            // Try common raw pin encodings used by ImNodes/legacy: division heuristics
            if (stored > 1)
            {
                uint32_t cand = stored / 2u;
                for (const auto& n : m_nodes) if (n.id.value == cand) return cand;

                cand = stored / 100u;
                for (const auto& n : m_nodes) if (n.id.value == cand) return cand;

                cand = stored / 10000u;
                for (const auto& n : m_nodes) if (n.id.value == cand) return cand;
            }
            // Not resolvable -> return stored as-is (fallback)
            return stored;
        };

        json fromPinJson = json::object();
        uint32_t resolvedFrom = resolveStoredPinToNodeId(static_cast<uint32_t>(link.fromPin.value));
        fromPinJson["nodeId"] = static_cast<int>(resolvedFrom);
        if (!link.fromPinName.empty()) fromPinJson["pinId"] = link.fromPinName;
        else fromPinJson["pinId"] = "output";
        // Editor-only: record ImNodes attribute UID if available
        if (link.fromAttrUid != 0) fromPinJson["attrUid"] = static_cast<int>(link.fromAttrUid);
        linkJson["fromPin"] = fromPinJson;

        json toPinJson = json::object();
        uint32_t resolvedTo = resolveStoredPinToNodeId(static_cast<uint32_t>(link.toPin.value));
        toPinJson["nodeId"] = static_cast<int>(resolvedTo);
        if (!link.toPinName.empty()) toPinJson["pinId"] = link.toPinName;
        // Editor-only: record ImNodes attribute UID if available
        if (link.toAttrUid != 0) toPinJson["attrUid"] = static_cast<int>(link.toAttrUid);
        else toPinJson["pinId"] = "input";
        linkJson["toPin"] = toPinJson;

        // If resolution produced ids that do not map to any node, warn for later inspection
        bool fromValid = false, toValid = false;
        for (const auto& n : m_nodes)
        {
            if (n.id.value == resolvedFrom) fromValid = true;
            if (n.id.value == resolvedTo) toValid = true;
        }
        if (!fromValid || !toValid)
        {
            // Invalid endpoints after resolution: skip serializing this link to avoid
            // persisting references to non-existent nodes. Log for diagnostics.
            SYSTEM_LOG << "[GraphDocument::ToJson] REMOVING invalid link id=" << link.id.value
                       << " resolvedFrom=" << resolvedFrom << " resolvedTo=" << resolvedTo
                       << " (not present in nodes)." << std::endl;
            continue; // do not serialize this link
        }

        linksJson.push_back(linkJson);
    }
    dataJson["links"] = linksJson;
    
    j["data"] = dataJson;
    
    // Phase 2.0 - Annotations
    j["annotations"] = m_nodeAnnotations.ToJson();
    
    // Phase 2.1 - Blackboard
    j["blackboard"] = m_blackboard.ToJson();
    
    return j;
}

GraphDocument GraphDocument::FromJson(const json& j)
{
    GraphDocument doc;
    
    // Basic properties
    doc.type = JsonHelper::GetString(j, "type", "AIGraph");
    // Prefer explicit graphKind, but fall back to legacy 'blueprintType' when present
    std::string gk = JsonHelper::GetString(j, "graphKind", "");
    if (gk.empty())
    {
        gk = JsonHelper::GetString(j, "blueprintType", "");
    }
    doc.graphKind = gk.empty() ? std::string("BehaviorTree") : gk;
    
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

        // Determine which format: legacy BehaviorTree (rootGraph) or standard format
        const json* nodesSource = nullptr;
        const json* linksSource = nullptr;

        // Try legacy BehaviorTree format first: data.rootGraph.nodes[]
        if (data.contains("rootGraph") && data["rootGraph"].is_object())
        {
            const json& rootGraph = data["rootGraph"];
            if (rootGraph.contains("nodes") && rootGraph["nodes"].is_array())
            {
                nodesSource = &rootGraph["nodes"];
            }
            if (rootGraph.contains("links") && rootGraph["links"].is_array())
            {
                linksSource = &rootGraph["links"];
            }

            // Legacy format: rootNodeId may be inside rootGraph
            if (rootGraph.contains("rootNodeId"))
            {
                doc.rootNodeId.value = JsonHelper::GetUInt(rootGraph, "rootNodeId", 0);
            }
            else
            {
                doc.rootNodeId.value = JsonHelper::GetUInt(data, "rootNodeId", 0);
            }
        }
        // Fallback to standard format: data.nodes[]
        else if (data.contains("nodes") && data["nodes"].is_array())
        {
            nodesSource = &data["nodes"];
            doc.rootNodeId.value = JsonHelper::GetUInt(data, "rootNodeId", 0);
        }
        // No nodes source found - try for links anyway
        else
        {
            doc.rootNodeId.value = JsonHelper::GetUInt(data, "rootNodeId", 0);
        }

        // If no links source from rootGraph, try standard format
        if (linksSource == nullptr && data.contains("links") && data["links"].is_array())
        {
            linksSource = &data["links"];
        }

        // Parse nodes
        if (nodesSource != nullptr && nodesSource->is_array())
        {
            const json& nodesArray = *nodesSource;
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

                // Optional ImNodes UID (editor-only)
                node.imnodesUid = static_cast<uint32_t>(JsonHelper::GetUInt(nodeJson, "imnodesUid", 0));

                doc.m_nodes.push_back(node);

                // Update next node ID
                if (node.id.value >= doc.m_nextNodeId)
                {
                    doc.m_nextNodeId = node.id.value + 1;
                }
            }
        }

        // Parse links - current .bt.json format uses fromPin.nodeId / toPin.nodeId
        if (linksSource != nullptr && linksSource->is_array())
        {
            const json& linksArray = *linksSource;
            for (size_t i = 0; i < linksArray.size(); ++i)
            {
                const json& linkJson = linksArray[i];

                LinkData link;
                link.id.value = JsonHelper::GetUInt(linkJson, "id", 0);

                auto resolvePinToNode = [&](const json& pinObj)->uint32_t {
                    if (!pinObj.is_object()) return 0u;

                    // Helper to test existence against parsed nodes
                    auto existsNode = [&](uint32_t candidate)->bool {
                        for (const auto& n : doc.m_nodes) if (n.id.value == candidate) return true;
                        return false;
                    };

                    // Preferred: explicit nodeId field provided by the editor export
                    if (pinObj.contains("nodeId") && pinObj["nodeId"].is_number())
                    {
                        uint32_t v = JsonHelper::GetUInt(pinObj, "nodeId", 0);
                        if (existsNode(v)) return v;
                        // If stored value appears to be an ImNodes attribute id (nodeId*2 or *2+1), try to decode
                        if (v > 1)
                        {
                            uint32_t cand = v / 2u;
                            if (existsNode(cand)) return cand;
                        }
                        // Try other legacy encodings
                        if (v > 100)
                        {
                            uint32_t cand = v / 100u;
                            if (existsNode(cand)) return cand;
                        }
                        // Fallback: return raw value (will be handled later)
                        return v;
                    }

                    // Older/alternate encodings: numeric 'pin' field encodes a raw pin id or attribute id
                    if (pinObj.contains("pin") && pinObj["pin"].is_number())
                    {
                        uint32_t raw = JsonHelper::GetUInt(pinObj, "pin", 0);
                        if (raw == 0) return 0u;
                        // If raw directly matches a node id, accept
                        if (existsNode(raw)) return raw;
                        // If raw is an ImNodes attribute id (node*2 / node*2+1)
                        if (raw > 1)
                        {
                            uint32_t cand = raw / 2u;
                            if (existsNode(cand)) return cand;
                        }
                        // Legacy heuristics
                        uint32_t cand = raw / 100u;
                        if (existsNode(cand)) return cand;
                        cand = raw / 10000u;
                        if (existsNode(cand)) return cand;
                        return raw;
                    }

                    // Fallback: numeric pinId
                    if (pinObj.contains("pinId") && pinObj["pinId"].is_number())
                    {
                        uint32_t v = JsonHelper::GetUInt(pinObj, "pinId", 0);
                        if (existsNode(v)) return v;
                        if (v > 1)
                        {
                            uint32_t cand = v / 2u;
                            if (existsNode(cand)) return cand;
                        }
                        return v;
                    }

                    // If pinId is a string ("output"/"input"), the nodeId must accompany it to resolve
                    return 0u;
                };

                if (linkJson.contains("fromPin") && linkJson["fromPin"].is_object())
                {
                    link.fromPin.value = resolvePinToNode(linkJson["fromPin"]);
                    // preserve semantic pin name when available
                    link.fromPinName = JsonHelper::GetString(linkJson["fromPin"], "pinId", "");
                    // Optional editor-only ImNodes attr uid
                    link.fromAttrUid = static_cast<uint32_t>(JsonHelper::GetUInt(linkJson["fromPin"], "attrUid", 0));
                }

                if (linkJson.contains("toPin") && linkJson["toPin"].is_object())
                {
                    link.toPin.value = resolvePinToNode(linkJson["toPin"]);
                    link.toPinName = JsonHelper::GetString(linkJson["toPin"], "pinId", "");
                    // Optional editor-only ImNodes attr uid
                    link.toAttrUid = static_cast<uint32_t>(JsonHelper::GetUInt(linkJson["toPin"], "attrUid", 0));
                }

                doc.m_links.push_back(link);

                // Update next link ID
                if (link.id.value >= doc.m_nextLinkId)
                {
                    doc.m_nextLinkId = link.id.value + 1;
                }
                // Diagnostic: log resolved link mapping
                SYSTEM_LOG << "[GraphDocument::FromJson] Link id=" << link.id.value
                           << " fromNode=" << link.fromPin.value
                           << " toNode=" << link.toPin.value << std::endl;
            }
        }

        // Phase 51: Diagnostic logging - confirm nodes and links loaded
        if (doc.m_nodes.size() > 0 || doc.m_links.size() > 0)
        {
            SYSTEM_LOG << "[GraphDocument::FromJson] Loaded: " << doc.m_nodes.size() 
                       << " nodes, " << doc.m_links.size() << " links\n";
        }
    }

    // Phase 2.0 - Annotations (backward compatible: missing key = no annotations)
    if (j.contains("annotations") && j["annotations"].is_array())
    {
        doc.m_nodeAnnotations.FromJson(j["annotations"]);
    }
    
    // Phase 2.1 - Blackboard (backward compatible: missing key = empty blackboard)
    if (j.contains("blackboard") && j["blackboard"].is_array())
    {
        doc.m_blackboard.FromJson(j["blackboard"]);
    }

    // Reconstruct node children from links to ensure a canonical hierarchy
    // This mirrors editor reconstruction rules so runtime/debugger share the same topology
    if (!doc.m_links.empty() && !doc.m_nodes.empty())
    {
        std::unordered_map<uint32_t, std::vector<uint32_t>> forwardAdj;
        auto isCompositeType = [](const NodeData& n)->bool {
            return (n.type.find("Selector") != std::string::npos) ||
                   (n.type.find("Sequence") != std::string::npos) ||
                   (n.type.find("Root") != std::string::npos);
        };

        // Build adjacency using link pin semantics preserved earlier
        for (const auto& link : doc.m_links)
        {
            uint32_t fromNode = static_cast<uint32_t>(link.fromPin.value);
            uint32_t toNode = static_cast<uint32_t>(link.toPin.value);
            if (fromNode == 0 || toNode == 0) continue;

            NodeId fromN; fromN.value = fromNode;
            NodeId toN; toN.value = toNode;
            const NodeData* fromNd = doc.GetNode(fromN);
            const NodeData* toNd = doc.GetNode(toN);

            // resolve composite vs child relationship
            bool applied = false;
            if (fromNd && toNd)
            {
                if (isCompositeType(*fromNd) && !isCompositeType(*toNd))
                {
                    forwardAdj[fromNode].push_back(toNode);
                    applied = true;
                }
                else if (!isCompositeType(*fromNd) && isCompositeType(*toNd))
                {
                    forwardAdj[toNode].push_back(fromNode);
                    applied = true;
                }
            }

            if (!applied)
            {
                std::string fromPinId = link.fromPinName;
                std::string toPinId = link.toPinName;
                if (!fromPinId.empty() && !toPinId.empty())
                {
                    if ((fromPinId == "output" && toPinId == "input") || (fromPinId == "out" && toPinId == "in"))
                    {
                        forwardAdj[fromNode].push_back(toNode);
                        applied = true;
                    }
                    else if ((toPinId == "output" && fromPinId == "input") || (toPinId == "out" && fromPinId == "in"))
                    {
                        forwardAdj[toNode].push_back(fromNode);
                        applied = true;
                    }
                }
            }

            if (!applied)
            {
                // spatial fallback: left node is parent
                float fx = 0.0f, tx = 0.0f;
                if (fromNd) { fx = fromNd->position.x; }
                if (toNd) { tx = toNd->position.x; }
                if (fx <= tx) forwardAdj[fromNode].push_back(toNode);
                else forwardAdj[toNode].push_back(fromNode);
            }
        }

        // Clear any existing children saved from file and apply reconstructed adjacency
        for (auto& n : doc.m_nodes) n.children.clear();
        for (const auto& kv : forwardAdj)
        {
            // find node index in doc.m_nodes by id
            for (auto& n : doc.m_nodes)
            {
                if (n.id.value == kv.first)
                {
                    for (uint32_t cid : kv.second)
                    {
                        NodeId childId; childId.value = cid;
                        n.children.push_back(childId);
                    }
                    break;
                }
            }
        }
    }
    
    // Diagnostic: dump nodes and their children to help runtime/editor parity checks
    if (!doc.m_nodes.empty())
    {
        SYSTEM_LOG << "[GraphDocument::FromJson] Node children dump:" << std::endl;
        for (const auto& n : doc.m_nodes)
        {
            SYSTEM_LOG << "  Node " << n.id.value << ": '" << n.name << "' children=[";
            for (size_t i = 0; i < n.children.size(); ++i)
            {
                if (i) SYSTEM_LOG << ",";
                SYSTEM_LOG << n.children[i].value;
            }
            SYSTEM_LOG << "]" << std::endl;
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

} // namespace NodeGraphTypes
} // namespace Olympe
