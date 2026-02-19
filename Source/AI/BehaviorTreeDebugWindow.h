/**
 * @file BehaviorTreeDebugWindow.h
 * @brief Runtime debugger for behavior tree visualization and inspection
 * @author Olympe Engine - Behavior Tree Debugger
 * @date 2025
 *
 * @details
 * Provides real-time visualization of AI decision-making with:
 * - Entity list with filtering and sorting
 * - Interactive node graph with execution token highlighting
 * - Inspector panel with blackboard and execution log
 */

#pragma once

#include "../ECS_Entity.h"
#include "../vector.h"
#include "../json_helper.h"
#include "BTGraphLayoutEngine.h"
#include "BehaviorTree.h"
#include "BTEditorCommand.h"
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <cstdint>

 // Forward declarations for SDL3
struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;
struct ImGuiContext;

namespace Olympe
{
    /**
     * @enum PinType
     * @brief Type of connection pin on a node
     */
    enum class PinType
    {
        Input,      ///< Input pin (left side of node in horizontal layout)
        Output      ///< Output pin (right side of node in horizontal layout)
    };

    /**
     * @struct NodePin
     * @brief Visual connection pin on a behavior tree node
     */
    struct NodePin
    {
        uint32_t nodeId = 0;                    ///< ID of the node this pin belongs to
        PinType type = PinType::Input;          ///< Pin type (input or output)
        Vector worldPos;                         ///< World position of the pin
        std::vector<uint32_t> connectedTo;      ///< IDs of connected nodes
    };

    /**
     * @struct BTConfig
     * @brief Configuration loaded from BT_config.json
     */
    struct BTConfig
    {
        // Layout settings
        bool defaultHorizontal = true;
        float gridSize = 16.0f;
        bool gridSnappingEnabled = true;
        float horizontalSpacing = 280.0f;
        float verticalSpacing = 120.0f;

        // Rendering settings
        float pinRadius = 6.0f;
        float pinOutlineThickness = 2.0f;
        float bezierTangent = 80.0f;
        float connectionThickness = 2.0f;

        // Node colors by type and status (RGBA values 0-255)
        // nodeColors[nodeType][status] = {r, g, b, a}
        struct Color { uint8_t r, g, b, a; };
        std::map<std::string, std::map<std::string, Color>> nodeColors;
    };

    /**
     * @struct BTColor
     * @brief RGBA color value (0-255 range)
     */
    struct BTColor
    {
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;
    };

    /**
     * @struct ExecutionLogEntry
     * @brief Single entry in the execution log
     */
    struct ExecutionLogEntry
    {
        float timeAgo = 0.0f;           ///< Time since entry (seconds)
        EntityID entity = 0;            ///< Entity that executed
        uint32_t nodeId = 0;            ///< Node that was executed
        std::string nodeName;           ///< Node name
        BTStatus status = BTStatus::Running;  ///< Execution result
    };

    /**
     * @struct EntityDebugInfo
     * @brief Cached debug information for a single entity
     */
    struct EntityDebugInfo
    {
        EntityID entityId = 0;
        std::string entityName;
        uint32_t treeId = 0;
        std::string treeName;
        bool isActive = false;
        bool hasTarget = false;
        BTStatus lastStatus = BTStatus::Running;
        uint32_t currentNodeId = 0;
        std::string aiMode;
        float lastUpdateTime = 0.0f;
    };

    /**
     * @class BehaviorTreeDebugWindow
     * @brief Main debug window for behavior tree runtime visualization
     *
     * Provides comprehensive debugging capabilities for AI behavior trees,
     * including entity selection, graph visualization, and blackboard inspection.
     *
     * Renders in a separate SDL3 native window (not embedded in main engine window).
     */
    class BehaviorTreeDebugWindow
    {
    public:
        BehaviorTreeDebugWindow();
        ~BehaviorTreeDebugWindow();

        /**
         * @brief Initialize the debug window
         */
        void Initialize();

        /**
         * @brief Shutdown and cleanup
         */
        void Shutdown();

        /**
         * @brief Render the debug window (in separate SDL3 window)
         */
        void Render();

        /**
         * @brief Toggle window visibility (creates/destroys separate window)
         */
        void ToggleVisibility();

        /**
         * @brief Check if window is visible
         */
        bool IsVisible() const { return m_isVisible; }

        /**
         * @brief Process SDL events for separate window
         * @param event SDL event to process
         */
        void ProcessEvent(SDL_Event* event);

        /**
         * @brief Add an execution log entry
         * @param entity Entity that executed
         * @param nodeId Node that was executed
         * @param nodeName Node name
         * @param status Execution result
         */
        void AddExecutionEntry(EntityID entity, uint32_t nodeId, const std::string& nodeName, BTStatus status);

    private:
        // Main panel rendering
        void RenderEntityListPanel();
        void RenderNodeGraphPanel();
        void RenderInspectorPanel();

        // Separate window management
        void CreateSeparateWindow();
        void DestroySeparateWindow();
        void RenderInSeparateWindow();

        // Entity list helpers
        void RefreshEntityList();
        void UpdateEntityFiltering();
        void UpdateEntitySorting();
        void RenderEntityEntry(const EntityDebugInfo& info);

        // Node graph helpers
        void RenderBehaviorTreeGraph();
        void RenderNode(const BTNode* node, const BTNodeLayout* layout, bool isCurrentNode);
        void RenderNodeConnections(const BTNode* node, const BTNodeLayout* layout, const BehaviorTreeAsset* tree);
        void RenderBezierConnection(const Vector& start, const Vector& end, uint32_t color, float thickness, float tangent);
        void RenderNodePins(const BTNode* node, const BTNodeLayout* layout);
        uint32_t GetNodeColor(BTNodeType type) const;
        uint32_t GetNodeColorByStatus(BTNodeType type, BTStatus status) const;
        const char* GetNodeIcon(BTNodeType type) const;

        // Editor mode helpers
        void RenderNodePalette();
        void RenderEditorToolbar();
        void HandleNodeCreation(BTNodeType nodeType);
        void HandleNodeDeletion();
        void HandleNodeDuplication();
        bool ValidateConnection(uint32_t parentId, uint32_t childId) const;
        void SaveEditedTree();
        void UndoLastAction();
        void RedoLastAction();
        
        // Validation helpers
        void RenderValidationPanel();
        uint32_t GetPinColor(uint32_t nodeId, PinType pinType) const;
        bool IsConnectionValid(uint32_t parentId, uint32_t childId) const;
        
        // Node properties editor
        void RenderNodeProperties();
        void RenderNodePropertiesSection();
        void ApplyModifiedParameters();
        uint32_t GenerateUniqueNodeId();
        BTNode* GetMutableNodeById(uint32_t nodeId);
        
        // Save system
        void Save();
        void SaveAs();
        void RenderFileMenu();
        json SerializeTreeToJson(const BehaviorTreeAsset& tree) const;
        std::string GetCurrentTimestamp() const;
        
        // New BT creation
        void RenderNewBTDialog();
        BehaviorTreeAsset CreateFromTemplate(int templateIndex, const std::string& name);
        
        // Edit menu
        void RenderEditMenu();

        // Configuration helpers
        void LoadBTConfig();
        void ApplyConfigToLayout();
        Vector SnapToGrid(const Vector& pos) const;

        // Inspector helpers
        void RenderRuntimeInfo();
        void RenderBlackboardSection();
        void RenderExecutionLog();

        // Camera control helpers
        void FitGraphToView();
        void CenterViewOnGraph();
        void ResetZoom();
        void RenderMinimap();

        // Camera helper utilities
        void ApplyZoomToStyle();
        void GetGraphBounds(Vector& outMin, Vector& outMax) const;
        float GetSafeZoom() const;
        Vector CalculatePanOffset(const Vector& graphCenter, const Vector& viewportSize) const;

        // Data management
        std::vector<EntityDebugInfo> m_entities;
        std::vector<EntityDebugInfo> m_filteredEntities;
        EntityID m_selectedEntity = 0;

        // Layout engine
        BTGraphLayoutEngine m_layoutEngine;
        std::vector<BTNodeLayout> m_currentLayout;

        // Camera state tracking
        EntityID m_lastCenteredEntity = 0;  // Track which entity was last centered
        float m_currentZoom = 1.0f;         // Current zoom level (0.3 to 3.0)
        bool m_showMinimap = true;          // Show minimap overlay

        // Execution log (circular buffer with max 100 entries)
        std::deque<ExecutionLogEntry> m_executionLog;
        const size_t MAX_LOG_ENTRIES = 100;

        // UI state
        bool m_isVisible = false;
        bool m_isInitialized = false;
        float m_autoRefreshInterval = 0.5f;

        // Filtering
        char m_filterText[256] = "";
        bool m_filterActiveOnly = false;
        bool m_filterHasTarget = false;

        // Sorting
        enum class SortMode
        {
            Name,
            TreeName,
            LastUpdate,
            AIMode
        };
        SortMode m_sortMode = SortMode::Name;
        bool m_sortAscending = true;

        // Panel layout
        float m_entityListWidth = 250.0f;
        float m_inspectorWidth = 350.0f;
        float m_nodeSpacingX = 320.0f;
        float m_nodeSpacingY = 180.0f;

        // Graph view state
        bool m_imnodesInitialized = false;
        int m_imnodesEditorContext = -1;
        BTLayoutDirection m_layoutDirection = BTLayoutDirection::TopToBottom;

        // Animation
        float m_pulseTimer = 0.0f;

        // Layout update flags
        bool m_needsLayoutUpdate = false;
        bool m_autoFitOnLoad = true;

        // Configuration
        BTConfig m_config;
        bool m_configLoaded = false;

        // Node colors by type and status (loaded from BT_config.json)
        std::map<BTNodeType, std::map<BTStatus, BTColor>> m_nodeColors;

        // Separate SDL3 window for debugger
        SDL_Window* m_separateWindow;
        SDL_Renderer* m_separateRenderer;
        bool m_windowCreated;

        // Separate ImGui context for this window
        ImGuiContext* m_separateImGuiContext;

        // Editor mode state
        bool m_editorMode = false;
        bool m_treeModified = false;
        bool m_isDirty = false;
        BehaviorTreeAsset m_editingTree;
        uint32_t m_nextNodeId = 1000;
        std::string m_currentFilePath;

        // Editor interaction state
        std::vector<uint32_t> m_selectedNodes;
        bool m_showNodePalette = false;
        Vector m_nodeCreationPos;
        
        // Pin dragging for connections
        bool m_isDraggingPin = false;
        uint32_t m_dragSourceNodeId = 0;
        PinType m_dragSourcePinType = PinType::Output;
        Vector m_dragEndPos;

        // Undo/Redo system (command pattern)
        BTCommandStack m_commandStack;

        // Link ID tracking for connection deletion
        struct LinkInfo {
            int linkId;
            uint32_t parentId;
            uint32_t childId;
        };
        std::vector<LinkInfo> m_linkMap;
        int m_nextLinkId = 100000;
        
        // Validation
        std::vector<BTValidationMessage> m_validationMessages;
        bool m_showValidationPanel = true;
        
        // Node properties
        uint32_t m_inspectedNodeId = 0;
        bool m_showNodeProperties = false;
        std::unordered_map<uint32_t, json> m_modifiedParameters;
        
        // New BT dialog
        bool m_showNewBTDialog = false;
        char m_newBTName[256] = "";
        int m_selectedTemplate = 0;
    };
}