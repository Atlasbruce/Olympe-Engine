/*
 * Olympe Blueprint Editor - Node Graph Manager Implementation
 */

#include "BTNodeGraphManager.h"
#include "Commands/CommandHistory.h"
#include "Commands/BTGraphCommands.h"
#include "SubgraphMigrator.h"
#include "../json_helper.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include <set>
#include <map>
#include <chrono>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace Olympe
{
    // ========== NodeGraph Implementation ==========

    NodeGraph::NodeGraph()
        : name("Untitled Graph")
        , type("BehaviorTree")
        , rootNodeId(-1)
        , m_NextNodeId(1)
        , m_commandHistory(new CommandHistory())
    {
    }

    NodeGraph::NodeGraph(const NodeGraph& other)
        : name(other.name)
        , type(other.type)
        , rootNodeId(other.rootNodeId)
        , editorMetadata(other.editorMetadata)
        , m_Nodes(other.m_Nodes)
        , m_NextNodeId(other.m_NextNodeId)
        , m_IsDirty(other.m_IsDirty)
        , m_Filepath(other.m_Filepath)
        , m_eventRootIds(other.m_eventRootIds)
        , m_commandHistory(new CommandHistory())  // New empty history
    {
    }

    NodeGraph& NodeGraph::operator=(const NodeGraph& other)
    {
        if (this != &other)
        {
            name = other.name;
            type = other.type;
            rootNodeId = other.rootNodeId;
            editorMetadata = other.editorMetadata;
            m_Nodes = other.m_Nodes;
            m_NextNodeId = other.m_NextNodeId;
            m_IsDirty = other.m_IsDirty;
            m_Filepath = other.m_Filepath;
            m_eventRootIds = other.m_eventRootIds;
            // Reset command history
            m_commandHistory = std::unique_ptr<CommandHistory>(new CommandHistory());
        }
        return *this;
    }

    NodeGraph::NodeGraph(NodeGraph&& other) noexcept
        : name(std::move(other.name))
        , type(std::move(other.type))
        , rootNodeId(other.rootNodeId)
        , editorMetadata(std::move(other.editorMetadata))
        , m_Nodes(std::move(other.m_Nodes))
        , m_NextNodeId(other.m_NextNodeId)
        , m_IsDirty(other.m_IsDirty)
        , m_Filepath(std::move(other.m_Filepath))
        , m_eventRootIds(std::move(other.m_eventRootIds))
        , m_commandHistory(std::move(other.m_commandHistory))
    {
    }

    NodeGraph& NodeGraph::operator=(NodeGraph&& other) noexcept
    {
        if (this != &other)
        {
            name = std::move(other.name);
            type = std::move(other.type);
            rootNodeId = other.rootNodeId;
            editorMetadata = std::move(other.editorMetadata);
            m_Nodes = std::move(other.m_Nodes);
            m_NextNodeId = other.m_NextNodeId;
            m_IsDirty = other.m_IsDirty;
            m_Filepath = std::move(other.m_Filepath);
            m_commandHistory = std::move(other.m_commandHistory);
        }
        return *this;
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
        
        MarkDirty();  // Mark graph as modified

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
        
        MarkDirty();  // Mark graph as modified

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
        
        MarkDirty();  // Mark graph as modified

        std::cout << "[NodeGraph] Linked node " << parentId << " -> " << childId << "\n";
        return true;
    }

    bool NodeGraph::UnlinkNodes(int parentId, int childId)
    {
        GraphNode* parent = GetNode(parentId);
        if (!parent)
            return false;

        bool unlinked = false;

        // Remove from child list
        auto it = std::find(parent->childIds.begin(), parent->childIds.end(), childId);
        if (it != parent->childIds.end())
        {
            parent->childIds.erase(it);
            std::cout << "[NodeGraph] Unlinked node " << parentId << " -> " << childId << "\n";
            unlinked = true;
        }

        // Check decorator child
        if (parent->decoratorChildId == childId)
        {
            parent->decoratorChildId = -1;
            std::cout << "[NodeGraph] Unlinked decorator child " << parentId << " -> " << childId << "\n";
            unlinked = true;
        }
        
        if (unlinked)
            MarkDirty();  // Mark graph as modified

        return unlinked;
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
        MarkDirty();  // Mark graph as modified
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
        
        // v2 Schema wrapper
        j["schema_version"] = 2;
        j["blueprintType"] = type.empty() ? "BehaviorTree" : type;
        j["name"] = name;
        j["description"] = "";  // Could be added to NodeGraph if needed
        
        // Metadata section
        j["metadata"]["author"] = "User";  // Could be made configurable
        j["metadata"]["created"] = "";  // Could be tracked if needed
        j["metadata"]["lastModified"] = editorMetadata.lastModified;
        j["metadata"]["tags"] = json::array();
        
        // Editor state
        j["editorState"]["zoom"] = editorMetadata.zoom;
        j["editorState"]["scrollOffset"]["x"] = editorMetadata.scrollOffsetX;
        j["editorState"]["scrollOffset"]["y"] = editorMetadata.scrollOffsetY;
        
        // Data section containing the actual tree
        j["data"]["rootNodeId"] = rootNodeId;
        j["data"]["nodes"] = json::array();
        j["data"]["eventRoots"] = json::array();  // NEW: Array of OnEvent root node IDs

        for (const auto& node : m_Nodes)
        {
            json nj;
            nj["id"] = node.id;
            nj["type"] = NodeTypeToString(node.type);
            nj["name"] = node.name;

            // Save position in a structured format
            nj["position"]["x"] = node.posX;
            nj["position"]["y"] = node.posY;

            if (!node.actionType.empty())
                nj["actionType"] = node.actionType;
            if (!node.conditionType.empty())
                nj["conditionType"] = node.conditionType;
            if (!node.decoratorType.empty())
                nj["decoratorType"] = node.decoratorType;
            if (!node.subgraphUUID.empty())
                nj["subgraphUUID"] = node.subgraphUUID;

            // NEW: Event-driven execution fields (for OnEvent nodes)
            if (!node.eventType.empty())
                nj["eventType"] = node.eventType;
            if (!node.eventMessage.empty())
                nj["eventMessage"] = node.eventMessage;

            // Parameters as nested object (v2 format)
            nj["parameters"] = json::object();
            if (!node.parameters.empty())
            {
                for (const auto& pair : node.parameters)
                    nj["parameters"][pair.first] = pair.second;
            }

            // Children array
            nj["children"] = json::array();
            for (int childId : node.childIds)
                nj["children"].push_back(childId);

            if (node.decoratorChildId >= 0)
                nj["decoratorChild"] = node.decoratorChildId;

            j["data"]["nodes"].push_back(nj);
        }

        // NEW: Save event root IDs separately
        for (uint32_t eventRootId : m_eventRootIds)
        {
            j["data"]["eventRoots"].push_back((int)eventRootId);
        }

        return j;
    }

    NodeGraph NodeGraph::FromJson(const nlohmann::json& j)
    {
        std::cout << "[NodeGraph::FromJson] Starting parsing..." << std::endl;
        
        NodeGraph graph;

        try {
            // Detect schema version - v2 has nested "data" structure, v1 doesn't
            bool isV2 = j.contains("schema_version") || j.contains("data");
            std::cout << "[NodeGraph::FromJson] Format: " << (isV2 ? "v2" : "v1") << std::endl;

            const json* dataSection = &j;
            
            if (isV2 && j.contains("data"))
            {
                const json& dataObj = j["data"];
                graph.name = JsonHelper::GetString(j, "name", "Untitled Graph");
                graph.type = JsonHelper::GetString(j, "blueprintType", "BehaviorTree");
                std::cout << "[NodeGraph::FromJson] Extracted 'data' section from v2" << std::endl;

                // Phase 8: support the flat-dictionary subgraph format where
                // nodes live in data.rootGraph rather than directly in data.
                if (dataObj.contains("rootGraph") && dataObj["rootGraph"].is_object())
                {
                    dataSection = &dataObj["rootGraph"];
                    std::cout << "[NodeGraph::FromJson] Using data.rootGraph (Phase 8 format)" << std::endl;
                }
                else
                {
                    dataSection = &dataObj;
                }
            }
            else
            {
                graph.name = JsonHelper::GetString(j, "name", "Untitled Graph");
                graph.type = JsonHelper::GetString(j, "type", "BehaviorTree");
                std::cout << "[NodeGraph::FromJson] Using root as data section (v1)" << std::endl;
            }
            
            graph.rootNodeId = JsonHelper::GetInt(*dataSection, "rootNodeId", -1);
            std::cout << "[NodeGraph::FromJson] Root node ID: " << graph.rootNodeId << std::endl;

            // Parse nodes
            if (!JsonHelper::IsArray(*dataSection, "nodes"))
            {
                std::cerr << "[NodeGraph::FromJson] ERROR: No 'nodes' array in data section" << std::endl;
                return graph;
            }

            // Get node count
            int nodeCount = 0;
            JsonHelper::ForEachInArray(*dataSection, "nodes", [&](const json& nj, size_t idx) { nodeCount++; });
            std::cout << "[NodeGraph::FromJson] Parsing " << nodeCount << " nodes..." << std::endl;

            int maxId = 0;
            bool hasPositions = false;
            
            // First pass: load nodes
            JsonHelper::ForEachInArray(*dataSection, "nodes", [&](const json& nj, size_t idx)
            {
                GraphNode node;
                node.id = JsonHelper::GetInt(nj, "id", 0);
                node.type = StringToNodeType(JsonHelper::GetString(nj, "type", "Action"));
                node.name = JsonHelper::GetString(nj, "name", "");
                
                std::string typeStr = JsonHelper::GetString(nj, "type", "Action");
                
                // Load position - try v2 format first
                if (nj.contains("position") && nj["position"].is_object())
                {
                    node.posX = JsonHelper::GetFloat(nj["position"], "x", 0.0f);
                    node.posY = JsonHelper::GetFloat(nj["position"], "y", 0.0f);
                    hasPositions = true;
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

                // Phase 8: load subgraph UUID reference for BT_SubGraph / HFSM_SubGraph nodes.
                node.subgraphUUID = JsonHelper::GetString(nj, "subgraphUUID", "");

                // NEW: Load event-driven execution fields (for OnEvent nodes)
                node.eventType = JsonHelper::GetString(nj, "eventType", "");
                node.eventMessage = JsonHelper::GetString(nj, "eventMessage", "");

                // Load parameters - v2 has nested "parameters" object, v1 has flat structure
                if (nj.contains("parameters") && nj["parameters"].is_object())
                {
                    // v2 format
                    const json& params = nj["parameters"];
                    for (auto it = params.begin(); it != params.end(); ++it)
                    {
						node.parameters[it.key()] = it.value().is_string() ? it.value().get<std::string>()
                            : it.value().dump();
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

                std::cout << "[NodeGraph::FromJson]   Node " << node.id << ": " << node.name 
                          << " (" << typeStr << ") at (" << node.posX << "," << node.posY << ")"
                          << " children: " << node.childIds.size() << std::endl;

                if (node.id > maxId)
                    maxId = node.id;
            });

            graph.m_NextNodeId = maxId + 1;

            // NEW: Load event root IDs (OnEvent nodes)
            if (JsonHelper::IsArray(*dataSection, "eventRoots"))
            {
                std::cout << "[NodeGraph::FromJson] Loading event roots..." << std::endl;
                JsonHelper::ForEachInArray(*dataSection, "eventRoots", [&](const json& eventRootJson, size_t idx)
                {
                    if (eventRootJson.is_number())
                    {
                        uint32_t eventRootId = eventRootJson.get<uint32_t>();
                        graph.m_eventRootIds.push_back(eventRootId);
                        std::cout << "[NodeGraph::FromJson]   Event root: " << eventRootId << std::endl;
                    }
                });
            }

            // Calculate positions if v1 (no positions)
            if (!hasPositions)
            {
                std::cout << "[NodeGraph::FromJson] No positions found, calculating hierarchical layout..." << std::endl;
                graph.CalculateNodePositionsHierarchical();
                std::cout << "[NodeGraph::FromJson] Position calculation complete" << std::endl;
            }
            else
            {
                std::cout << "[NodeGraph::FromJson] Using existing node positions from file" << std::endl;
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

            std::cout << "[NodeGraph::FromJson] Parsing complete: " << graph.m_Nodes.size() << " nodes loaded" << std::endl;

            return graph;

        } catch (const std::exception& e) {
            std::cerr << "[NodeGraph::FromJson] EXCEPTION: " << e.what() << std::endl;
            return graph;
        }
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
        if (m_commandHistory)
            m_commandHistory->Clear();
    }

    CommandHistory* NodeGraph::GetCommandHistory()
    {
        return m_commandHistory.get();
    }

    const CommandHistory* NodeGraph::GetCommandHistory() const
    {
        return m_commandHistory.get();
    }

    bool NodeGraph::CanUndo() const
    {
        return m_commandHistory && m_commandHistory->CanUndo();
    }

    bool NodeGraph::CanRedo() const
    {
        return m_commandHistory && m_commandHistory->CanRedo();
    }

    std::string NodeGraph::GetUndoDescription() const
    {
        return m_commandHistory ? m_commandHistory->GetUndoDescription() : "";
    }

    std::string NodeGraph::GetRedoDescription() const
    {
        return m_commandHistory ? m_commandHistory->GetRedoDescription() : "";
    }

    bool NodeGraph::Undo()
    {
        return m_commandHistory && m_commandHistory->Undo();
    }

    bool NodeGraph::Redo()
    {
        return m_commandHistory && m_commandHistory->Redo();
    }

    // ========== Event Root Methods ==========

    bool NodeGraph::IsValidRoot(uint32_t nodeId) const
    {
        // Main Root node
        if (static_cast<int>(nodeId) == rootNodeId)
            return true;

        // OnEvent roots
        return std::find(m_eventRootIds.begin(), m_eventRootIds.end(), nodeId) != m_eventRootIds.end();
    }

    void NodeGraph::AddEventRoot(uint32_t nodeId)
    {
        // Check if already in list
        if (std::find(m_eventRootIds.begin(), m_eventRootIds.end(), nodeId) == m_eventRootIds.end())
        {
            m_eventRootIds.push_back(nodeId);
            MarkDirty();
            std::cout << "[NodeGraph] Added event root: " << nodeId << std::endl;
        }
    }

    void NodeGraph::RemoveEventRoot(uint32_t nodeId)
    {
        auto it = std::find(m_eventRootIds.begin(), m_eventRootIds.end(), nodeId);
        if (it != m_eventRootIds.end())
        {
            m_eventRootIds.erase(it);
            MarkDirty();
            std::cout << "[NodeGraph] Removed event root: " << nodeId << std::endl;
        }
    }

    const std::vector<uint32_t>& NodeGraph::GetEventRootIds() const
    {
        return m_eventRootIds;
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
        m_GraphOrder.push_back(graphId);  // Track insertion order
        m_ActiveGraphId = graphId;
        m_LastActiveGraphId = graphId;  // Update last active

        std::cout << "[NodeGraphManager] Created graph " << graphId << " (" << name << ")\n";
        return graphId;
    }

    bool NodeGraphManager::CloseGraph(int graphId)
    {
        auto it = m_Graphs.find(graphId);
        if (it == m_Graphs.end())
            return false;

        // Remove from graph order
        auto orderIt = std::find(m_GraphOrder.begin(), m_GraphOrder.end(), graphId);
        if (orderIt != m_GraphOrder.end())
            m_GraphOrder.erase(orderIt);

        m_Graphs.erase(it);

        if (m_ActiveGraphId == graphId)
        {
            // Try to select a neighbor from graph order for better UX
            // Find closest neighbor (prefer next, then previous)
            if (!m_GraphOrder.empty())
            {
                // Find where the closed graph was in order
                size_t closedIndex = 0;
                for (size_t i = 0; i < m_GraphOrder.size(); ++i)
                {
                    if (m_GraphOrder[i] > graphId)
                    {
                        closedIndex = i;
                        break;
                    }
                    closedIndex = i + 1;
                }
                
                // Pick the next available tab, or previous if at end
                if (closedIndex < m_GraphOrder.size())
                    m_ActiveGraphId = m_GraphOrder[closedIndex];
                else if (!m_GraphOrder.empty())
                    m_ActiveGraphId = m_GraphOrder[m_GraphOrder.size() - 1];
                else
                    m_ActiveGraphId = -1;
            }
            else
            {
                m_ActiveGraphId = -1;
            }
            
            if (m_ActiveGraphId != -1)
                m_LastActiveGraphId = m_ActiveGraphId;
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
        {
            m_ActiveGraphId = graphId;
            m_LastActiveGraphId = graphId;  // Update last active for persistence
        }
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
        // Return graphs in insertion order for consistent tab rendering
        return m_GraphOrder;
    }

    std::string NodeGraphManager::GetGraphName(int graphId) const
    {
        const NodeGraph* graph = GetGraph(graphId);
        return graph ? graph->name : "";
    }

    void NodeGraphManager::SetGraphOrder(const std::vector<int>& newOrder)
    {
        // Update the graph order (e.g., after tab reordering in UI)
        // Only update if the order contains valid graph IDs
        if (newOrder.size() != m_GraphOrder.size())
            return;
        
        // Verify all IDs in newOrder exist in m_Graphs
        for (int graphId : newOrder)
        {
            if (m_Graphs.find(graphId) == m_Graphs.end())
                return;  // Invalid ID, don't update
        }
        
        m_GraphOrder = newOrder;
    }

    bool NodeGraphManager::SaveGraph(int graphId, const std::string& filepath)
    {
        NodeGraph* graph = GetGraph(graphId);
        if (!graph)
            return false;
        
        // Update lastModified timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        
        #ifdef _MSC_VER
            std::tm timeinfo;
            localtime_s(&timeinfo, &time);
            ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
        #else
            // Use localtime_r for thread safety on POSIX systems
            std::tm timeinfo;
            localtime_r(&time, &timeinfo);
            ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
        #endif
        
        graph->editorMetadata.lastModified = ss.str();

        json j = graph->ToJson();

        std::ofstream file(filepath);
        if (!file.is_open())
            return false;

        file << j.dump(2);
        file.close();
        
        // Update filepath and clear dirty flag on successful save
        graph->SetFilepath(filepath);
        graph->ClearDirty();

        std::cout << "[NodeGraphManager] Saved graph " << graphId << " to " << filepath << "\n";
        return true;
    }

    int NodeGraphManager::LoadGraph(const std::string& filepath)
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << "[NodeGraphManager::LoadGraph] CALLED" << std::endl;
        std::cout << "[NodeGraphManager::LoadGraph] Path: " << filepath << std::endl;
        std::cout << "========================================+n" << std::endl;
        
        try {
            // 1. Check file exists
            std::cout << "[NodeGraphManager] Step 1: Checking file exists..." << std::endl;
            std::ifstream testFile(filepath);
            if (!testFile.is_open())
            {
                std::cerr << "[NodeGraphManager] ERROR: File not found: " << filepath << std::endl;
                std::cout << "========================================+n" << std::endl;
                return -1;
            }
            testFile.close();
            std::cout << "[NodeGraphManager] File exists: OK" << std::endl;

            // 2. Load file content
            std::cout << "[NodeGraphManager] Step 2: Loading file content..." << std::endl;
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "[NodeGraphManager] ERROR: Cannot open file: " << filepath << std::endl;
                std::cout << "========================================+n" << std::endl;
                return -1;
            }

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            std::cout << "[NodeGraphManager] File loaded: " << content.size() << " bytes" << std::endl;

            if (content.empty())
            {
                std::cerr << "[NodeGraphManager] ERROR: File is empty" << std::endl;
                std::cout << "========================================+n" << std::endl;
                return -1;
            }

            // 3. Parse JSON
            std::cout << "[NodeGraphManager] Step 3: Parsing JSON..." << std::endl;
            json j;
            try {
                j = json::parse(content);
                std::cout << "[NodeGraphManager] JSON parsed: OK" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[NodeGraphManager] ERROR parsing JSON: " << e.what() << std::endl;
                std::cout << "========================================+n" << std::endl;
                return -1;
            }

            // 3b. Phase 8: auto-migrate legacy format to flat-dictionary subgraph format.
            {
                SubgraphMigrator migrator;
                if (migrator.NeedsMigration(j))
                {
                    std::cout << "[NodeGraphManager] Applying Phase 8 subgraph migration..." << std::endl;
                    j = migrator.Migrate(j);

                    // Persist the migrated file immediately so it is not re-migrated next load.
                    try {
                        std::ofstream migOut(filepath);
                        if (migOut.is_open())
                        {
                            migOut << j.dump(2);
                            migOut.close();
                            std::cout << "[NodeGraphManager] Migrated file saved: " << filepath << std::endl;
                        }
                    } catch (const std::exception& saveEx) {
                        std::cerr << "[NodeGraphManager] WARNING: Could not save migrated file: "
                                  << saveEx.what() << std::endl;
                    }
                }
            }

            // 4. Detect version
            std::cout << "[NodeGraphManager] Step 4: Detecting version..." << std::endl;
            bool isV2 = j.contains("schema_version") &&
                        (j["schema_version"].get<int>() >= 2);
            bool isV1 = !isV2 && (j.contains("nodes") || j.contains("rootNodeId"));
            
            std::cout << "[NodeGraphManager] Version: " << (isV2 ? "v2+" : (isV1 ? "v1" : "Unknown")) << std::endl;

            if (!isV1 && !isV2)
            {
                std::cerr << "[NodeGraphManager] ERROR: Invalid blueprint format (neither v1 nor v2)" << std::endl;
                std::cout << "========================================+n" << std::endl;
                return -1;
            }

            // 5. Parse graph from JSON
            std::cout << "[NodeGraphManager] Step 5: Parsing graph with FromJson..." << std::endl;
            NodeGraph graph;
            try {
                graph = NodeGraph::FromJson(j);
                std::cout << "[NodeGraphManager] FromJson returned: " << graph.GetAllNodes().size() << " nodes" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[NodeGraphManager] ERROR in FromJson: " << e.what() << std::endl;
                std::cout << "========================================+n" << std::endl;
                return -1;
            }

            // 6. Handle v1 migration if needed
            if (isV1)
            {
                std::cout << "[NodeGraphManager] Step 6: Detected v1 format, migrating to v2..." << std::endl;
                
                // Create v2 structure
                json v2Json = json::object();
                v2Json["schema_version"] = 2;
                v2Json["blueprintType"] = graph.type.empty() ? "BehaviorTree" : graph.type;
                v2Json["name"] = graph.name;
                v2Json["description"] = "";
                
                // Metadata
                json metadata = json::object();
                metadata["author"] = "Atlasbruce";
                metadata["created"] = "2026-01-09T18:26:00Z";
                metadata["lastModified"] = "2026-01-09T18:26:00Z";
                metadata["tags"] = json::array();
                v2Json["metadata"] = metadata;
                
                // Editor state
                json editorState = json::object();
                editorState["zoom"] = 1.0;
                json scrollOffset = json::object();
                scrollOffset["x"] = 0;
                scrollOffset["y"] = 0;
                editorState["scrollOffset"] = scrollOffset;
                v2Json["editorState"] = editorState;
                
                // Data (re-serialize current graph)
                v2Json["data"] = graph.ToJson();
                
                // Save migrated version
                std::cout << "[NodeGraphManager] Saving migrated v2 file..." << std::endl;
                try {
                    // Backup original
                    std::string backupPath = filepath + ".v1.backup";
                    std::ifstream src(filepath, std::ios::binary);
                    std::ofstream dst(backupPath, std::ios::binary);
                    dst << src.rdbuf();
                    src.close();
                    dst.close();
                    std::cout << "[NodeGraphManager] Original backed up to: " << backupPath << std::endl;
                    
                    // Save new version
                    std::ofstream outFile(filepath);
                    if (outFile.is_open())
                    {
                        outFile << v2Json.dump(2);
                        outFile.close();
                        std::cout << "[NodeGraphManager] Migrated file saved: " << filepath << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[NodeGraphManager] WARNING: Could not save migrated file: " << e.what() << std::endl;
                }
            }

            // 7. Create graph in manager
            std::cout << "[NodeGraphManager] Step 7: Creating graph in manager..." << std::endl;
            int graphId = m_NextGraphId++;
            auto graphPtr = std::make_unique<NodeGraph>(std::move(graph));
            
            // Set filepath and clear dirty flag for freshly loaded graph
            graphPtr->SetFilepath(filepath);
            graphPtr->ClearDirty();
            
            m_Graphs[graphId] = std::move(graphPtr);
            m_GraphOrder.push_back(graphId);  // Track insertion order
            m_ActiveGraphId = graphId;
            m_LastActiveGraphId = graphId;  // Update last active

            std::cout << "[NodeGraphManager] Graph registered with ID: " << graphId << std::endl;
            std::cout << "[NodeGraphManager] Graph name: " << m_Graphs[graphId]->name << std::endl;
            std::cout << "[NodeGraphManager] Graph type: " << m_Graphs[graphId]->type << std::endl;
            std::cout << "[NodeGraphManager] Total graphs loaded: " << m_Graphs.size() << std::endl;
            std::cout << "[NodeGraphManager] Active graph ID: " << m_ActiveGraphId << std::endl;

            std::cout << "\n========================================" << std::endl;
            std::cout << "[NodeGraphManager::LoadGraph] SUCCESS ->" << std::endl;
            std::cout << "========================================+n" << std::endl;

            return graphId;

        } catch (const std::exception& e) {
            std::cerr << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            std::cerr << "[NodeGraphManager] EXCEPTION: " << e.what() << std::endl;
            std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
            std::cout << "========================================+n" << std::endl;
            return -1;
        } catch (...) {
            std::cerr << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            std::cerr << "[NodeGraphManager] UNKNOWN EXCEPTION" << std::endl;
            std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
            std::cout << "========================================+n" << std::endl;
            return -1;
        }
    }
    
    bool NodeGraphManager::IsGraphDirty(int graphId) const
    {
        const NodeGraph* graph = GetGraph(graphId);
        return graph ? graph->IsDirty() : false;
    }

    bool NodeGraphManager::HasUnsavedChanges() const
    {
        for (const auto& pair : m_Graphs)
        {
            if (pair.second && pair.second->IsDirty())
                return true;
        }
        return false;
    }

    // ========== Copy/Paste Methods in NodeGraph ==========
    void NodeGraph::CopyNodesToClipboard(const std::vector<int>& nodeIds)
    {
        m_clipboardData.clear();

        for (int nodeId : nodeIds)
        {
            auto nodeIt = std::find_if(m_Nodes.begin(), m_Nodes.end(),
                [nodeId](const GraphNode& n) { return n.id == nodeId; });

            if (nodeIt != m_Nodes.end())
            {
                ClipboardNode clipNode;
                clipNode.nodeId = nodeIt->id;
                clipNode.nodeType = static_cast<int>(nodeIt->type);
                clipNode.name = nodeIt->name;
                clipNode.posX = nodeIt->posX;
                clipNode.posY = nodeIt->posY;
                clipNode.actionType = nodeIt->actionType;
                clipNode.conditionType = nodeIt->conditionType;
                clipNode.decoratorType = nodeIt->decoratorType;
                clipNode.subgraphUUID = nodeIt->subgraphUUID;
                clipNode.parameters = nodeIt->parameters;
                clipNode.childIds = nodeIt->childIds;
                clipNode.decoratorChildId = nodeIt->decoratorChildId;

                m_clipboardData.push_back(clipNode);
            }
        }

        m_IsDirty = true;
    }

    std::vector<int> NodeGraph::PasteNodesFromClipboard(float offsetX, float offsetY)
    {
        std::vector<int> pastedIds;

        if (m_clipboardData.empty())
            return pastedIds;

        std::map<int, int> idMapping;  // Old ID -> New ID
        int maxId = m_NextNodeId;

        // Create new nodes from clipboard
        for (const ClipboardNode& clipNode : m_clipboardData)
        {
            GraphNode newNode;
            newNode.id = maxId;
            newNode.type = static_cast<NodeType>(clipNode.nodeType);
            newNode.name = clipNode.name;
            newNode.posX = clipNode.posX + offsetX;
            newNode.posY = clipNode.posY + offsetY;
            newNode.actionType = clipNode.actionType;
            newNode.conditionType = clipNode.conditionType;
            newNode.decoratorType = clipNode.decoratorType;
            newNode.subgraphUUID = clipNode.subgraphUUID;
            newNode.parameters = clipNode.parameters;
            // Don't copy connections initially
            newNode.childIds.clear();
            newNode.decoratorChildId = -1;

            idMapping[clipNode.nodeId] = maxId;
            pastedIds.push_back(maxId);
            m_Nodes.push_back(newNode);
            maxId++;
        }

        m_NextNodeId = maxId;
        m_IsDirty = true;

        return pastedIds;
    }

    std::vector<int> NodeGraph::DuplicateNodes(const std::vector<int>& nodeIds, float offsetX, float offsetY)
    {
        std::vector<int> duplicatedIds;

        for (int origId : nodeIds)
        {
            auto nodeIt = std::find_if(m_Nodes.begin(), m_Nodes.end(),
                [origId](const GraphNode& n) { return n.id == origId; });

            if (nodeIt != m_Nodes.end())
            {
                GraphNode newNode = *nodeIt;
                newNode.id = m_NextNodeId++;
                newNode.posX += offsetX;
                newNode.posY += offsetY;
                newNode.name = nodeIt->name + " (copy)";
                newNode.childIds.clear();
                newNode.decoratorChildId = -1;

                duplicatedIds.push_back(newNode.id);
                m_Nodes.push_back(newNode);
            }
        }

        m_IsDirty = true;

        return duplicatedIds;
    }
}
