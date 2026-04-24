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
#include "../NodeGraphCore/NodeGraphCore.h"
#include "../NodeGraphCore/GraphDocument.h"

namespace Olympe
{
    // Type aliases for NodeGraphTypes to maintain NodeGraph:: namespace compatibility
    using GraphDocument = Olympe::NodeGraphTypes::GraphDocument;
    using GraphId = Olympe::NodeGraphTypes::GraphId;
    using NodeId = Olympe::NodeGraphTypes::NodeId;
    using PinId = Olympe::NodeGraphTypes::PinId;
    using NodeData = Olympe::NodeGraphTypes::NodeData;
    using LinkData = Olympe::NodeGraphTypes::LinkData;
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
        // Behavior Tree - Flow Control (Composite)
        BT_Sequence,
        BT_Selector,
        BT_Parallel,
        BT_RandomSelector,
        BT_ParallelThreshold,

        // Behavior Tree - Conditions
        BT_Condition,

        // Behavior Tree - Actions
        BT_Action,

        // Behavior Tree - Decorators
        BT_Decorator,
        BT_Inverter,
        BT_Monitor,
        BT_Repeater,
        BT_UntilSuccess,
        BT_UntilFailure,
        BT_Cooldown,

        // Behavior Tree - Entry Points
        BT_Root,            ///< Phase 38b: Root entry point (green, fixed position)
        BT_OnEvent,         ///< Phase 38b: Event-driven root (green, event-triggered)

        // Behavior Tree - Utilities
        BT_SendMessage,
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
            // Composites
            case NodeType::BT_Sequence: return "Sequence";
            case NodeType::BT_Selector: return "Selector";
            case NodeType::BT_Parallel: return "Parallel";
            case NodeType::BT_RandomSelector: return "RandomSelector";
            case NodeType::BT_ParallelThreshold: return "ParallelThreshold";

            // Conditions & Actions
            case NodeType::BT_Condition: return "Condition";
            case NodeType::BT_Action: return "Action";

            // Decorators
            case NodeType::BT_Decorator: return "Decorator";
            case NodeType::BT_Inverter: return "Inverter";
            case NodeType::BT_Monitor: return "Monitor";
            case NodeType::BT_Repeater: return "Repeater";
            case NodeType::BT_UntilSuccess: return "UntilSuccess";
            case NodeType::BT_UntilFailure: return "UntilFailure";
            case NodeType::BT_Cooldown: return "Cooldown";

            // Entry Points
            case NodeType::BT_Root: return "Root";
            case NodeType::BT_OnEvent: return "OnEvent";

            // Utilities
            case NodeType::BT_SendMessage: return "SendMessage";
            case NodeType::BT_SubGraph: return "SubGraph";

            // HFSM
            case NodeType::HFSM_State: return "State";
            case NodeType::HFSM_Transition: return "Transition";
            case NodeType::HFSM_SubGraph: return "HFSMSubGraph";

            // Generic
            case NodeType::Comment: return "Comment";
            default: return "Unknown";
        }
    }

    // Convert string to NodeType
    // Maps ALL BehaviorTree type names (generic and specific implementations) to generic NodeType enum
    // This ensures consistent styling: all Conditions → BT_Condition, all Actions → BT_Action, etc.
    inline NodeType StringToNodeType(const std::string& str)
    {
        // ====== ENTRY POINTS (GREEN) ======
        if (str == "Root" || str == "BT_Root") return NodeType::BT_Root;
        if (str == "OnEvent" || str == "BT_OnEvent") return NodeType::BT_OnEvent;

        // ====== COMPOSITES (DARK BLUE) ======
        if (str == "Sequence" || str == "BT_Sequence") return NodeType::BT_Sequence;
        if (str == "Selector" || str == "BT_Selector") return NodeType::BT_Selector;
        if (str == "Parallel" || str == "BT_Parallel") return NodeType::BT_Parallel;
        if (str == "RandomSelector" || str == "BT_RandomSelector") return NodeType::BT_RandomSelector;
        if (str == "ParallelThreshold" || str == "BT_ParallelThreshold") return NodeType::BT_ParallelThreshold;

        // ====== CONDITIONS (PURPLE) ======
        // Generic condition types
        if (str == "Condition" || str == "BT_Condition") return NodeType::BT_Condition;

        // Specific condition implementations - all map to BT_Condition for consistent PURPLE color
        if (str == "CheckBlackboardValue" || str == "BT_CheckBlackboardValue") return NodeType::BT_Condition;
        if (str == "HasTarget" || str == "BT_HasTarget") return NodeType::BT_Condition;
        if (str == "IsTargetInRange" || str == "BT_IsTargetInRange") return NodeType::BT_Condition;
        if (str == "CanSeeTarget" || str == "BT_CanSeeTarget") return NodeType::BT_Condition;
        if (str == "CanSetTarget" || str == "BT_CanSetTarget") return NodeType::BT_Condition;
        if (str == "Monitor" || str == "BT_Monitor") return NodeType::BT_Condition;  // Monitor is a condition variant

        // ====== DECORATORS (PLUM) ======
        // Generic decorator
        if (str == "Decorator" || str == "BT_Decorator") return NodeType::BT_Decorator;

        // Specific decorator implementations
        if (str == "Inverter" || str == "BT_Inverter") return NodeType::BT_Decorator;
        if (str == "Cooldown" || str == "BT_Cooldown") return NodeType::BT_Decorator;

        // ====== REPEATERS (YELLOW) ======
        if (str == "Repeater" || str == "BT_Repeater") return NodeType::BT_Repeater;
        if (str == "UntilSuccess" || str == "BT_UntilSuccess") return NodeType::BT_Repeater;
        if (str == "UntilFailure" || str == "BT_UntilFailure") return NodeType::BT_Repeater;

        // ====== ACTIONS (ORANGE) ======
        // Generic action type
        if (str == "Action" || str == "BT_Action") return NodeType::BT_Action;

        // Specific action implementations - all map to BT_Action for consistent ORANGE color
        if (str == "Wait" || str == "BT_Wait") return NodeType::BT_Action;
        if (str == "WaitRandomTime" || str == "BT_WaitRandomTime") return NodeType::BT_Action;
        if (str == "SetBlackboardValue" || str == "BT_SetBlackboardValue") return NodeType::BT_Action;
        if (str == "MoveToTarget" || str == "BT_MoveToTarget") return NodeType::BT_Action;
        if (str == "MoveToPosition" || str == "BT_MoveToPosition") return NodeType::BT_Action;
        if (str == "AttackTarget" || str == "BT_AttackTarget") return NodeType::BT_Action;
        if (str == "PlayAnimation" || str == "BT_PlayAnimation") return NodeType::BT_Action;
        if (str == "EmitSound" || str == "BT_EmitSound") return NodeType::BT_Action;

        // ====== UTILITIES ======
        if (str == "SendMessage" || str == "BT_SendMessage") return NodeType::BT_SendMessage;
        if (str == "SubGraph" || str == "BT_SubGraph") return NodeType::BT_SubGraph;

        // ====== HFSM (Hierarchical Finite State Machine) ======
        if (str == "State" || str == "HFSM_State") return NodeType::HFSM_State;
        if (str == "Transition" || str == "HFSM_Transition") return NodeType::HFSM_Transition;
        if (str == "HFSMSubGraph" || str == "HFSM_SubGraph") return NodeType::HFSM_SubGraph;

        // ====== MISC ======
        if (str == "Comment") return NodeType::Comment;

        // Default fallback: treat as Action (orange)
        return NodeType::BT_Action;
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

        // ====================================================================
        // Event-Driven Execution Fields (for OnEvent nodes)
        // ====================================================================

        /**
         * @brief Event type that triggers this OnEvent node
         * Only used for BT_OnEvent nodes. Value is event type ID string
         * (e.g., "Olympe_EventType_AI_Explosion")
         */
        std::string eventType;

        /**
         * @brief Optional event message filter for OnEvent nodes
         * If set, only events with matching message field trigger this node
         */
        std::string eventMessage;

        /**
         * @brief Index of this node in its graph's event roots array (m_eventRootIds)
         * Only valid if this is an OnEvent node. Used for fast lookup.
         */
        uint32_t onEventRootIndex = ~0u;  // Invalid index by default

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

    // ========================================
    // DEPRECATED: NodeGraph class (Phase 50.3)
    // ========================================
    // Removed: Namespace collision with NodeGraphCore::namespace NodeGraph
    // Replacement: Use modern GraphDocument from NodeGraphTypes
    // This legacy class was unused by rendering pipeline
    // ========================================

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
        GraphDocument* GetGraph(int graphId);
        const GraphDocument* GetGraph(int graphId) const;

        // Active graph
        void SetActiveGraph(int graphId);
        int GetActiveGraphId() const { return m_ActiveGraphId; }
        GraphDocument* GetActiveGraph();
        const GraphDocument* GetActiveGraph() const;

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
        std::map<int, std::unique_ptr<GraphDocument>> m_Graphs;
        std::vector<int> m_GraphOrder;  // Track insertion order for consistent tab rendering
    };
}
