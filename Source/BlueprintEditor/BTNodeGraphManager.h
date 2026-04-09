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
#include "Commands/CommandHistory.h"

namespace Olympe
{
    /**
     * @struct ClipboardNode
     * @brief Serializable node data for copy/paste operations
     */
    struct ClipboardNode
    {
        int nodeId;
        int nodeType;  // Stored as int for serialization
        std::string name;
        float posX, posY;
        std::string actionType;
        std::string conditionType;
        std::string decoratorType;
        std::string subgraphUUID;
        std::map<std::string, std::string> parameters;
        std::vector<int> childIds;
        int decoratorChildId;

        ClipboardNode() : nodeId(0), nodeType(0), posX(0.0f), posY(0.0f), decoratorChildId(-1) {}
    };
    // Node type enumeration
    enum class NodeType
    {
        // Behavior Tree nodes
        BT_Sequence,
        BT_Selector,
        BT_Action,
        BT_Condition,
        BT_Decorator,
        BT_SubGraph,        ///< Phase 8: references a subgraph by UUID (BehaviorTree)

        // HFSM nodes
        HFSM_State,
        HFSM_Transition,
        HFSM_SubGraph,      ///< Phase 8: references a subgraph by UUID (HFSM)

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
            case NodeType::BT_SubGraph: return "SubGraph";
            case NodeType::HFSM_State: return "State";
            case NodeType::HFSM_Transition: return "Transition";
            case NodeType::HFSM_SubGraph: return "HFSMSubGraph";
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
        if (str == "SubGraph") return NodeType::BT_SubGraph;
        if (str == "State") return NodeType::HFSM_State;
        if (str == "Transition") return NodeType::HFSM_Transition;
        if (str == "HFSMSubGraph") return NodeType::HFSM_SubGraph;
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
        
        // Phase 8: UUID reference for BT_SubGraph / HFSM_SubGraph nodes.
        // Empty string means this node is not a subgraph reference.
        std::string subgraphUUID;

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

    // Editor metadata for graph
    struct EditorMetadata
    {
        float zoom = 1.0f;
        float scrollOffsetX = 0.0f;
        float scrollOffsetY = 0.0f;
        std::string lastModified;
        
        EditorMetadata() = default;
    };

    // Node graph (Behavior Tree or HFSM)
    class NodeGraph
    {
    public:
        NodeGraph();
        ~NodeGraph() = default;

        // Copy constructor and assignment - explicitly defaulted
        NodeGraph(const NodeGraph& other);
        NodeGraph& operator=(const NodeGraph& other);

        // Move constructor and assignment
        NodeGraph(NodeGraph&& other) noexcept;
        NodeGraph& operator=(NodeGraph&& other) noexcept;

        // Graph metadata
        std::string name;
        std::string type;  // "BehaviorTree" or "HFSM"
        int rootNodeId = -1;
        EditorMetadata editorMetadata;

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

        // Calculate node positions for v1 blueprints (hierarchical layout)
        void CalculateNodePositionsHierarchical();

        // Dirty flag tracking for unsaved changes
        bool IsDirty() const { return m_IsDirty; }
        void MarkDirty() { m_IsDirty = true; }
        void ClearDirty() { m_IsDirty = false; }

        // Filepath tracking
        const std::string& GetFilepath() const { return m_Filepath; }
        void SetFilepath(const std::string& filepath) { m_Filepath = filepath; }
        bool HasFilepath() const { return !m_Filepath.empty(); }

        // Undo/Redo support
        CommandHistory* GetCommandHistory();
        const CommandHistory* GetCommandHistory() const;
        bool CanUndo() const;
        bool CanRedo() const;
        std::string GetUndoDescription() const;
        std::string GetRedoDescription() const;
        bool Undo();
        bool Redo();

        // Copy/Paste support
        std::vector<ClipboardNode> m_clipboardData;
        void CopyNodesToClipboard(const std::vector<int>& nodeIds);
        std::vector<int> PasteNodesFromClipboard(float offsetX = 30.0f, float offsetY = 30.0f);
        std::vector<int> DuplicateNodes(const std::vector<int>& nodeIds, float offsetX = 30.0f, float offsetY = 30.0f);

    private:
        std::vector<GraphNode> m_Nodes;
        int m_NextNodeId = 1;
        bool m_IsDirty = false;
        std::string m_Filepath;
        std::unique_ptr<CommandHistory> m_commandHistory;

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
        void SetGraphOrder(const std::vector<int>& newOrder);

        // File operations
        bool SaveGraph(int graphId, const std::string& filepath);
        int LoadGraph(const std::string& filepath);

        // State
        bool IsInitialized() const { return m_Initialized; }
        
        // Dirty flag queries for graphs
        bool IsGraphDirty(int graphId) const;
        bool HasUnsavedChanges() const;  // Returns true if any graph has unsaved changes

    private:
        NodeGraphManager();
        ~NodeGraphManager();
        
        NodeGraphManager(const NodeGraphManager&) = delete;
        NodeGraphManager& operator=(const NodeGraphManager&) = delete;

    private:
        bool m_Initialized = false;
        int m_ActiveGraphId = -1;
        int m_LastActiveGraphId = -1;  // Track last active for persistence
        int m_NextGraphId = 1;
        std::map<int, std::unique_ptr<NodeGraph>> m_Graphs;
        std::vector<int> m_GraphOrder;  // Track insertion order for consistent tab rendering
    };
}
