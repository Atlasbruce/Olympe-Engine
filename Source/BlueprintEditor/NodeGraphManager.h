/*
 * Olympe Blueprint Editor - Node Graph Manager
 * 
 * Backend for managing behavior tree and HFSM node graphs
 * Provides CRUD operations and graph serialization
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe
{
    // Node type enumeration
    enum class NodeType
    {
        // Behavior Tree nodes
        BT_Sequence,
        BT_Selector,
        BT_Action,
        BT_Condition,
        BT_Decorator,
        
        // HFSM nodes
        HFSM_State,
        HFSM_Transition,
        
        // Generic
        Comment
    };

    // Convert NodeType to string
    inline const char* NodeTypeToString(NodeType type)
    {
        switch (type)
        {
            case NodeType::BT_Sequence: return "Sequence";
            case NodeType::BT_Selector: return "Selector";
            case NodeType::BT_Action: return "Action";
            case NodeType::BT_Condition: return "Condition";
            case NodeType::BT_Decorator: return "Decorator";
            case NodeType::HFSM_State: return "State";
            case NodeType::HFSM_Transition: return "Transition";
            case NodeType::Comment: return "Comment";
            default: return "Unknown";
        }
    }

    // Convert string to NodeType
    inline NodeType StringToNodeType(const std::string& str)
    {
        if (str == "Sequence") return NodeType::BT_Sequence;
        if (str == "Selector") return NodeType::BT_Selector;
        if (str == "Action") return NodeType::BT_Action;
        if (str == "Condition") return NodeType::BT_Condition;
        if (str == "Decorator") return NodeType::BT_Decorator;
        if (str == "State") return NodeType::HFSM_State;
        if (str == "Transition") return NodeType::HFSM_Transition;
        if (str == "Comment") return NodeType::Comment;
        return NodeType::BT_Action; // Default
    }

    // Graph node structure
    struct GraphNode
    {
        int id = 0;
        NodeType type = NodeType::BT_Action;
        std::string name;
        float posX = 0.0f;
        float posY = 0.0f;
        
        // For Action nodes
        std::string actionType;
        
        // For Condition nodes
        std::string conditionType;
        
        // For Decorator nodes
        std::string decoratorType;
        
        // Generic parameters (key-value pairs)
        std::map<std::string, std::string> parameters;
        
        // Child nodes (for composite nodes)
        std::vector<int> childIds;
        
        // Decorator child (single child for decorators)
        int decoratorChildId = -1;

        GraphNode() = default;
        GraphNode(int nodeId, NodeType nodeType, const std::string& nodeName = "")
            : id(nodeId), type(nodeType), name(nodeName) {}
    };

    // Link between nodes
    struct GraphLink
    {
        int fromNode = 0;
        int toNode = 0;
        int fromAttr = 0;  // Output attribute ID
        int toAttr = 0;    // Input attribute ID

        GraphLink() = default;
        GraphLink(int from, int to) : fromNode(from), toNode(to) {}
    };

    // Node graph (Behavior Tree or HFSM)
    class NodeGraph
    {
    public:
        NodeGraph();
        ~NodeGraph() = default;

        // Graph metadata
        std::string name;
        std::string type;  // "BehaviorTree" or "HFSM"
        int rootNodeId = -1;

        // Node CRUD
        int CreateNode(NodeType type, float x, float y, const std::string& name = "");
        bool DeleteNode(int nodeId);
        GraphNode* GetNode(int nodeId);
        const GraphNode* GetNode(int nodeId) const;
        std::vector<GraphNode*> GetAllNodes();
        std::vector<const GraphNode*> GetAllNodes() const;

        // Link operations
        bool LinkNodes(int parentId, int childId);
        bool UnlinkNodes(int parentId, int childId);
        std::vector<GraphLink> GetAllLinks() const;

        // Parameter operations
        bool SetNodeParameter(int nodeId, const std::string& paramName, const std::string& value);
        std::string GetNodeParameter(int nodeId, const std::string& paramName) const;

        // Serialization
        nlohmann::json ToJson() const;
        static NodeGraph FromJson(const nlohmann::json& j);

        // Validation
        bool ValidateGraph(std::string& errorMsg) const;

        // Utility
        void Clear();
        int GetNextNodeId() const { return m_NextNodeId; }

    private:
        std::vector<GraphNode> m_Nodes;
        int m_NextNodeId = 1;

        // Helper to find node index
        int FindNodeIndex(int nodeId) const;
    };

    /**
     * NodeGraphManager - Manages multiple node graphs
     * Allows opening multiple behavior trees/FSMs simultaneously
     */
    class NodeGraphManager
    {
    public:
        static NodeGraphManager& Instance();
        static NodeGraphManager& Get() { return Instance(); }

        // Lifecycle
        void Initialize();
        void Shutdown();

        // Graph management
        int CreateGraph(const std::string& name, const std::string& type);
        bool CloseGraph(int graphId);
        NodeGraph* GetGraph(int graphId);
        const NodeGraph* GetGraph(int graphId) const;

        // Active graph
        void SetActiveGraph(int graphId);
        int GetActiveGraphId() const { return m_ActiveGraphId; }
        NodeGraph* GetActiveGraph();
        const NodeGraph* GetActiveGraph() const;

        // Graph list
        std::vector<int> GetAllGraphIds() const;
        std::string GetGraphName(int graphId) const;

        // File operations
        bool SaveGraph(int graphId, const std::string& filepath);
        int LoadGraph(const std::string& filepath);

        // State
        bool IsInitialized() const { return m_Initialized; }

    private:
        NodeGraphManager();
        ~NodeGraphManager();
        
        NodeGraphManager(const NodeGraphManager&) = delete;
        NodeGraphManager& operator=(const NodeGraphManager&) = delete;

    private:
        bool m_Initialized = false;
        int m_ActiveGraphId = -1;
        int m_NextGraphId = 1;
        std::map<int, std::unique_ptr<NodeGraph>> m_Graphs;
    };
}
