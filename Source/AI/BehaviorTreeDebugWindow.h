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
#include "BTGraphLayoutEngine.h"
#include "BehaviorTree.h"
#include <string>
#include <vector>
#include <deque>
#include <cstdint>

namespace Olympe
{
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
         * @brief Render the debug window
         */
        void Render();

        /**
         * @brief Toggle window visibility
         */
        void ToggleVisibility();

        /**
         * @brief Check if window is visible
         */
        bool IsVisible() const { return m_isVisible; }

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

        // Entity list helpers
        void RefreshEntityList();
        void UpdateEntityFiltering();
        void UpdateEntitySorting();
        void RenderEntityEntry(const EntityDebugInfo& info);

        // Node graph helpers
        void RenderBehaviorTreeGraph();
        void RenderNode(const BTNode* node, const BTNodeLayout* layout, bool isCurrentNode);
        void RenderNodeConnections(const BTNode* node, const BTNodeLayout* layout, const BehaviorTreeAsset* tree);
        uint32_t GetNodeColor(BTNodeType type) const;
        const char* GetNodeIcon(BTNodeType type) const;

        // Inspector helpers
        void RenderRuntimeInfo();
        void RenderBlackboardSection();
        void RenderExecutionLog();

        // Data management
        std::vector<EntityDebugInfo> m_entities;
        std::vector<EntityDebugInfo> m_filteredEntities;
        EntityID m_selectedEntity = 0;
        
        // Layout engine
        BTGraphLayoutEngine m_layoutEngine;
        std::vector<BTNodeLayout> m_currentLayout;

        // Execution log (circular buffer with max 100 entries)
        std::deque<ExecutionLogEntry> m_executionLog;
        const size_t MAX_LOG_ENTRIES = 100;

        // UI state
        bool m_isVisible = false;
        bool m_isInitialized = false;
        float m_lastRefreshTime = 0.0f;
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
        float m_nodeSpacingX = 250.0f;
        float m_nodeSpacingY = 150.0f;

        // Graph view state
        bool m_imnodesInitialized = false;
        int m_imnodesEditorContext = -1;

        // Animation
        float m_pulseTimer = 0.0f;
    };
}
