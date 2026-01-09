/*
 * Olympe Blueprint Editor - Node Graph Manager Implementation
 */

#include "NodeGraphManager.h"
#include "../json_helper.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include <set>
#include <map>

using json = nlohmann::json;

namespace Olympe
{
    // ========== NodeGraph Implementation ==========

    NodeGraph::NodeGraph()
        : name("Untitled Graph")
        , type("BehaviorTree")
        , rootNodeId(-1)
        , m_NextNodeId(1)
    {
    }

    int NodeGraph::CreateNode(NodeType nodeType, float x, float y, const std::string& nodeName)
    {
        GraphNode node;
        node.id = m_NextNodeId++;
        node.type = nodeType;
        node.name = nodeName.empty() ? NodeTypeToString(nodeType) : nodeName;
        node.posX = x;
        node.posY = y;

        m_Nodes.push_back(node);

        std::cout << "[NodeGraph] Created node " << node.id << " (" << node.name << ")\n";
        return node.id;
    }

    bool NodeGraph::DeleteNode(int nodeId)
    {
        int index = FindNodeIndex(nodeId);
        if (index < 0)
            return false;

        // Remove node
        m_Nodes.erase(m_Nodes.begin() + index);

        // Clean up references to this node
        for (auto& node : m_Nodes)
        {
            // Remove from child lists
            auto it = std::find(node.childIds.begin(), node.childIds.end(), nodeId);
            if (it != node.childIds.end())
                node.childIds.erase(it);

            // Clear decorator child if it matches
            if (node.decoratorChildId == nodeId)
                node.decoratorChildId = -1;
        }

        std::cout << "[NodeGraph] Deleted node " << nodeId << "\n";
        return true;
    }

    GraphNode* NodeGraph::GetNode(int nodeId)
    {
        int index = FindNodeIndex(nodeId);
        if (index < 0)
            return nullptr;
        return &m_Nodes[index];
    }

    const GraphNode* NodeGraph::GetNode(int nodeId) const
    {
        int index = FindNodeIndex(nodeId);
        if (index < 0)
            return nullptr;
        return &m_Nodes[index];
    }

    std::vector<GraphNode*> NodeGraph::GetAllNodes()
    {
        std::vector<GraphNode*> result;
        for (auto& node : m_Nodes)
            result.push_back(&node);
        return result;
    }

    std::vector<const GraphNode*> NodeGraph::GetAllNodes() const
    {
        std::vector<const GraphNode*> result;
        for (const auto& node : m_Nodes)
            result.push_back(&node);
        return result;
    }

    bool NodeGraph::LinkNodes(int parentId, int childId)
    {
        GraphNode* parent = GetNode(parentId);
        if (!parent)
            return false;

        // Check if already linked
        if (std::find(parent->childIds.begin(), parent->childIds.end(), childId) != parent->childIds.end())
            return false;

        // For decorator nodes, use decoratorChildId instead
        if (parent->type == NodeType::BT_Decorator)
        {
            parent->decoratorChildId = childId;
        }
        else
        {
            parent->childIds.push_back(childId);
        }

        std::cout << "[NodeGraph] Linked node " << parentId << " -> " << childId << "\n";
        return true;
    }

    bool NodeGraph::UnlinkNodes(int parentId, int childId)
    {
        GraphNode* parent = GetNode(parentId);
        if (!parent)
            return false;

        // Remove from child list
        auto it = std::find(parent->childIds.begin(), parent->childIds.end(), childId);
        if (it != parent->childIds.end())
        {
            parent->childIds.erase(it);
            std::cout << "[NodeGraph] Unlinked node " << parentId << " -> " << childId << "\n";
            return true;
        }

        // Check decorator child
        if (parent->decoratorChildId == childId)
        {
            parent->decoratorChildId = -1;
            std::cout << "[NodeGraph] Unlinked decorator child " << parentId << " -> " << childId << "\n";
            return true;
        }

        return false;
    }

    std::vector<GraphLink> NodeGraph::GetAllLinks() const
    {
        std::vector<GraphLink> links;

        for (const auto& node : m_Nodes)
        {
            // Add links to children
            for (int childId : node.childIds)
            {
                links.push_back(GraphLink(node.id, childId));
            }

            // Add decorator child link
            if (node.decoratorChildId >= 0)
            {
                links.push_back(GraphLink(node.id, node.decoratorChildId));
            }
        }

        return links;
    }

    bool NodeGraph::SetNodeParameter(int nodeId, const std::string& paramName, const std::string& value)
    {
        GraphNode* node = GetNode(nodeId);
        if (!node)
            return false;

        node->parameters[paramName] = value;
        return true;
    }

    std::string NodeGraph::GetNodeParameter(int nodeId, const std::string& paramName) const
    {
        const GraphNode* node = GetNode(nodeId);
        if (!node)
            return "";

        auto it = node->parameters.find(paramName);
        if (it != node->parameters.end())
            return it->second;

        return "";
    }

    nlohmann::json NodeGraph::ToJson() const
    {
        json j;
        j["name"] = name;
        j["type"] = type;
        j["rootNodeId"] = rootNodeId;
        j["nodes"] = json::array();

        for (const auto& node : m_Nodes)
        {
            json nj;
            nj["id"] = node.id;
            nj["type"] = NodeTypeToString(node.type);
            nj["name"] = node.name;
            
            // Save position in a structured format
            nlohmann::json posJson = nlohmann::json::object();
            posJson["x"] = node.posX;
            posJson["y"] = node.posY;
            nj["position"] = posJson;

            if (!node.actionType.empty())
                nj["actionType"] = node.actionType;
            if (!node.conditionType.empty())
                nj["conditionType"] = node.conditionType;
            if (!node.decoratorType.empty())
                nj["decoratorType"] = node.decoratorType;

            if (!node.parameters.empty())
            {
                json params = json::object();
                for (const auto& pair : node.parameters)
                    params[pair.first] = pair.second;
                nj["parameters"] = params;
            }

            nlohmann::json childrenJson = nlohmann::json::array();
            for (int childId : node.childIds)
                childrenJson.push_back(childId);
            nj["children"] = childrenJson;

            if (node.decoratorChildId >= 0)
                nj["decoratorChild"] = node.decoratorChildId;

            j["nodes"].push_back(nj);
        }

        // Add editor metadata
        nlohmann::json editorMeta = nlohmann::json::object();
        editorMeta["zoom"] = editorMetadata.zoom;
        nlohmann::json scrollOffset = nlohmann::json::object();
        scrollOffset["x"] = editorMetadata.scrollOffsetX;
        scrollOffset["y"] = editorMetadata.scrollOffsetY;
        editorMeta["scrollOffset"] = scrollOffset;
        editorMeta["lastModified"] = editorMetadata.lastModified;
        j["editorMetadata"] = editorMeta;

        return j;
    }

    NodeGraph NodeGraph::FromJson(const nlohmann::json& j)
    {
        NodeGraph graph;

        // Detect schema version - v2 has nested "data" structure, v1 doesn't
        bool isV2 = j.contains("schema_version") || j.contains("data");
        const json* dataSection = &j;
        
        if (isV2 && j.contains("data"))
        {
            dataSection = &j["data"];
            graph.name = JsonHelper::GetString(j, "name", "Untitled Graph");
            graph.type = JsonHelper::GetString(j, "blueprintType", "BehaviorTree");
        }
        else
        {
            graph.name = JsonHelper::GetString(j, "name", "Untitled Graph");
            graph.type = JsonHelper::GetString(j, "type", "BehaviorTree");
        }
        
        graph.rootNodeId = JsonHelper::GetInt(*dataSection, "rootNodeId", -1);

        std::cout << "[NodeGraph::FromJson] Loading graph '" << graph.name << "' (v" << (isV2 ? "2" : "1") << ")\n";

        if (JsonHelper::IsArray(*dataSection, "nodes"))
        {
            int maxId = 0;
            
            // First pass: load nodes
            JsonHelper::ForEachInArray(*dataSection, "nodes", [&](const json& nj, size_t idx)
            {
                GraphNode node;
                node.id = JsonHelper::GetInt(nj, "id", 0);
                node.type = StringToNodeType(JsonHelper::GetString(nj, "type", "Action"));
                node.name = JsonHelper::GetString(nj, "name", "");
                
                // Load position - try v2 format first
                if (nj.contains("position") && nj["position"].is_object())
                {
                    node.posX = JsonHelper::GetFloat(nj["position"], "x", 0.0f);
                    node.posY = JsonHelper::GetFloat(nj["position"], "y", 0.0f);
                }
                else
                {
                    // v1 format has no position - will calculate later
                    node.posX = 0.0f;
                    node.posY = 0.0f;
                }

                node.actionType = JsonHelper::GetString(nj, "actionType", "");
                node.conditionType = JsonHelper::GetString(nj, "conditionType", "");
                node.decoratorType = JsonHelper::GetString(nj, "decoratorType", "");

                // Load parameters - v2 has nested "parameters" object, v1 has flat structure
                if (nj.contains("parameters") && nj["parameters"].is_object())
                {
                    // v2 format
                    const json& params = nj["parameters"];
                    for (auto it = params.begin(); it != params.end(); ++it)
                    {
                        node.parameters[it->first] = it->second.is_string() 
                            ? it->second.get<std::string>() 
                            : it->second.dump();
                    }
                }
                else
                {
                    // v1 format - parameters are flat in node object
                    if (nj.contains("param"))
                        node.parameters["param"] = nj["param"].dump();
                    if (nj.contains("param1"))
                        node.parameters["param1"] = nj["param1"].dump();
                    if (nj.contains("param2"))
                        node.parameters["param2"] = nj["param2"].dump();
                }

                // Load children
                if (JsonHelper::IsArray(nj, "children"))
                {
                    JsonHelper::ForEachInArray(nj, "children", [&](const json& childJson, size_t childIdx)
                    {
                        if (childJson.is_number())
                            node.childIds.push_back(childJson.get<int>());
                    });
                }

                node.decoratorChildId = JsonHelper::GetInt(nj, "decoratorChild", -1);

                graph.m_Nodes.push_back(node);

                if (node.id > maxId)
                    maxId = node.id;
            });

            graph.m_NextNodeId = maxId + 1;
            
            // If v1 format, calculate positions using hierarchical layout
            if (!isV2)
            {
                std::cout << "[NodeGraph::FromJson] v1 format detected, calculating node positions...\n";
                graph.CalculateNodePositionsHierarchical();
            }
        }

        // Load editor metadata if present (v2 only)
        if (isV2)
        {
            if (j.contains("editorState") && j["editorState"].is_object())
            {
                const json& state = j["editorState"];
                graph.editorMetadata.zoom = JsonHelper::GetFloat(state, "zoom", 1.0f);
                
                if (state.contains("scrollOffset") && state["scrollOffset"].is_object())
                {
                    graph.editorMetadata.scrollOffsetX = JsonHelper::GetFloat(state["scrollOffset"], "x", 0.0f);
                    graph.editorMetadata.scrollOffsetY = JsonHelper::GetFloat(state["scrollOffset"], "y", 0.0f);
                }
            }
        }

        std::cout << "[NodeGraph::FromJson] Loaded " << graph.m_Nodes.size() << " nodes\n";
        return graph;
    }

    bool NodeGraph::ValidateGraph(std::string& errorMsg) const
    {
        // Check for cycles (simplified check)
        // Check that all child references are valid
        for (const auto& node : m_Nodes)
        {
            for (int childId : node.childIds)
            {
                if (!GetNode(childId))
                {
                    errorMsg = "Node " + std::to_string(node.id) + " has invalid child " + std::to_string(childId);
                    return false;
                }
            }

            if (node.decoratorChildId >= 0 && !GetNode(node.decoratorChildId))
            {
                errorMsg = "Node " + std::to_string(node.id) + " has invalid decorator child";
                return false;
            }
        }

        return true;
    }

    void NodeGraph::Clear()
    {
        m_Nodes.clear();
        m_NextNodeId = 1;
        rootNodeId = -1;
    }

    int NodeGraph::FindNodeIndex(int nodeId) const
    {
        for (size_t i = 0; i < m_Nodes.size(); ++i)
        {
            if (m_Nodes[i].id == nodeId)
                return static_cast<int>(i);
        }
        return -1;
    }

    void NodeGraph::CalculateNodePositionsHierarchical()
    {
        const float HORIZONTAL_SPACING = 350.0f;
        const float VERTICAL_SPACING = 200.0f;
        const float START_X = 200.0f;
        const float START_Y = 300.0f;

        std::cout << "[NodeGraph] Calculating hierarchical positions for " << m_Nodes.size() << " nodes\n";

        // Build parent-child map
        std::map<int, std::vector<int>> childrenMap;
        for (const auto& node : m_Nodes)
        {
            if (!node.childIds.empty())
            {
                childrenMap[node.id] = node.childIds;
            }
        }

        // BFS from root to assign positions by depth
        if (rootNodeId < 0)
        {
            std::cerr << "[NodeGraph] No root node ID, cannot calculate positions\n";
            return;
        }

        std::queue<std::pair<int, int>> queue; // nodeId, depth
        queue.push({rootNodeId, 0});
        
        std::map<int, int> depthCounter; // tracks sibling index at each depth
        std::set<int> visited;

        while (!queue.empty())
        {
            std::pair<int, int> front = queue.front();
            int nodeId = front.first;
            int depth = front.second;
            queue.pop();

            if (visited.count(nodeId)) continue;
            visited.insert(nodeId);

            int siblingIndex = depthCounter[depth]++;
            
            // Find node and set position
            int nodeIndex = FindNodeIndex(nodeId);
            if (nodeIndex >= 0)
            {
                m_Nodes[nodeIndex].posX = START_X + depth * HORIZONTAL_SPACING;
                m_Nodes[nodeIndex].posY = START_Y + siblingIndex * VERTICAL_SPACING;
                
                std::cout << "[NodeGraph] Node " << nodeId << " positioned at (" 
                         << m_Nodes[nodeIndex].posX << ", " << m_Nodes[nodeIndex].posY << ")\n";
            }

            // Queue children
            if (childrenMap.count(nodeId))
            {
                for (int childId : childrenMap[nodeId])
                {
                    if (!visited.count(childId))
                    {
                        queue.push({childId, depth + 1});
                    }
                }
            }
        }

        std::cout << "[NodeGraph] Position calculation complete\n";
    }

    // ========== NodeGraphManager Implementation ==========

    NodeGraphManager& NodeGraphManager::Instance()
    {
        static NodeGraphManager instance;
        return instance;
    }

    NodeGraphManager::NodeGraphManager()
    {
    }

    NodeGraphManager::~NodeGraphManager()
    {
        Shutdown();
    }

    void NodeGraphManager::Initialize()
    {
        if (m_Initialized)
            return;

        std::cout << "[NodeGraphManager] Initializing...\n";
        m_Initialized = true;
    }

    void NodeGraphManager::Shutdown()
    {
        if (!m_Initialized)
            return;

        std::cout << "[NodeGraphManager] Shutting down...\n";
        m_Graphs.clear();
        m_ActiveGraphId = -1;
        m_Initialized = false;
    }

    int NodeGraphManager::CreateGraph(const std::string& name, const std::string& type)
    {
        auto graph = std::make_unique<NodeGraph>();
        graph->name = name;
        graph->type = type;

        int graphId = m_NextGraphId++;
        m_Graphs[graphId] = std::move(graph);
        m_ActiveGraphId = graphId;

        std::cout << "[NodeGraphManager] Created graph " << graphId << " (" << name << ")\n";
        return graphId;
    }

    bool NodeGraphManager::CloseGraph(int graphId)
    {
        auto it = m_Graphs.find(graphId);
        if (it == m_Graphs.end())
            return false;

        m_Graphs.erase(it);

        if (m_ActiveGraphId == graphId)
        {
            // Set active to first available graph
            if (!m_Graphs.empty())
                m_ActiveGraphId = m_Graphs.begin()->first;
            else
                m_ActiveGraphId = -1;
        }

        std::cout << "[NodeGraphManager] Closed graph " << graphId << "\n";
        return true;
    }

    NodeGraph* NodeGraphManager::GetGraph(int graphId)
    {
        auto it = m_Graphs.find(graphId);
        if (it == m_Graphs.end())
            return nullptr;
        return it->second.get();
    }

    const NodeGraph* NodeGraphManager::GetGraph(int graphId) const
    {
        auto it = m_Graphs.find(graphId);
        if (it == m_Graphs.end())
            return nullptr;
        return it->second.get();
    }

    void NodeGraphManager::SetActiveGraph(int graphId)
    {
        if (m_Graphs.find(graphId) != m_Graphs.end())
            m_ActiveGraphId = graphId;
    }

    NodeGraph* NodeGraphManager::GetActiveGraph()
    {
        return GetGraph(m_ActiveGraphId);
    }

    const NodeGraph* NodeGraphManager::GetActiveGraph() const
    {
        return GetGraph(m_ActiveGraphId);
    }

    std::vector<int> NodeGraphManager::GetAllGraphIds() const
    {
        std::vector<int> ids;
        for (const auto& pair : m_Graphs)
            ids.push_back(pair.first);
        return ids;
    }

    std::string NodeGraphManager::GetGraphName(int graphId) const
    {
        const NodeGraph* graph = GetGraph(graphId);
        return graph ? graph->name : "";
    }

    bool NodeGraphManager::SaveGraph(int graphId, const std::string& filepath)
    {
        const NodeGraph* graph = GetGraph(graphId);
        if (!graph)
            return false;

        json j = graph->ToJson();

        std::ofstream file(filepath);
        if (!file.is_open())
            return false;

        file << j.dump(2);
        file.close();

        std::cout << "[NodeGraphManager] Saved graph " << graphId << " to " << filepath << "\n";
        return true;
    }

    int NodeGraphManager::LoadGraph(const std::string& filepath)
    {
        std::cout << "[NodeGraphManager] ========================================" << std::endl;
        std::cout << "[NodeGraphManager] Loading graph: " << filepath << std::endl;
        
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "[NodeGraphManager] ERROR: Failed to open file: " << filepath << "\n";
            std::cout << "[NodeGraphManager] ========================================" << std::endl;
            return -1;
        }

        json j;
        std::string jsonText((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        std::cout << "[NodeGraphManager] File loaded: " << jsonText.size() << " bytes" << std::endl;
        
        try
        {
            j = json::parse(jsonText);
            std::cout << "[NodeGraphManager] JSON parsed successfully" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[NodeGraphManager] ERROR: JSON parse error: " << e.what() << "\n";
            std::cout << "[NodeGraphManager] ========================================" << std::endl;
            return -1;
        }

        // Detect if v1 format (no schema_version or data section)
        bool isV1 = !j.contains("schema_version") && !j.contains("data");
        bool isV2 = j.contains("schema_version") && j["schema_version"].get<int>() == 2;
        
        std::cout << "[NodeGraphManager] Version: " << (isV2 ? "v2" : (isV1 ? "v1" : "unknown")) << std::endl;
        
        std::cout << "[NodeGraphManager] Converting JSON to NodeGraph..." << std::endl;
        NodeGraph graph = NodeGraph::FromJson(j);
        std::cout << "[NodeGraphManager] NodeGraph created: " << graph.name << std::endl;

        int graphId = m_NextGraphId++;
        m_Graphs[graphId] = std::make_unique<NodeGraph>(std::move(graph));
        m_ActiveGraphId = graphId;

        std::cout << "[NodeGraphManager] Graph loaded with ID: " << graphId << std::endl;
        
        // If v1 format was loaded, auto-migrate to v2 and save
        if (isV1)
        {
            std::cout << "[NodeGraphManager] Detected v1 format, auto-migrating to v2..." << std::endl;
            
            // Convert to v2 JSON format
            json v2Json = m_Graphs[graphId]->ToJson();
            
            // Wrap in v2 structure
            json wrappedV2 = json::object();
            wrappedV2["schema_version"] = 2;
            wrappedV2["blueprintType"] = v2Json["type"];
            wrappedV2["name"] = v2Json["name"];
            wrappedV2["description"] = "";
            
            json metadata = json::object();
            metadata["author"] = "Atlasbruce";
            metadata["created"] = "2026-01-09T15:33:00Z";
            metadata["lastModified"] = "2026-01-09T15:33:00Z";
            metadata["tags"] = json::array();
            wrappedV2["metadata"] = metadata;
            
            json editorState = json::object();
            editorState["zoom"] = 1.0;
            json scrollOffset = json::object();
            scrollOffset["x"] = 0;
            scrollOffset["y"] = 0;
            editorState["scrollOffset"] = scrollOffset;
            wrappedV2["editorState"] = editorState;
            
            wrappedV2["data"] = v2Json;
            
            // Save migrated version
            std::cout << "[NodeGraphManager] Saving migrated v2 format..." << std::endl;
            std::ofstream outFile(filepath);
            if (outFile.is_open())
            {
                outFile << wrappedV2.dump(2);
                outFile.close();
                std::cout << "[NodeGraphManager] Saved migrated v2 format to " << filepath << std::endl;
            }
            else
            {
                std::cerr << "[NodeGraphManager] WARNING: Failed to save migrated v2 format" << std::endl;
            }
        }
        
        std::cout << "[NodeGraphManager] SUCCESS: Graph loaded and active" << std::endl;
        std::cout << "[NodeGraphManager] ========================================" << std::endl;
        
        return graphId;
    }
}
