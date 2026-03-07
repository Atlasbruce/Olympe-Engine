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
#include "BehaviorTree.h"
#include "../BlueprintEditor/NodeGraphPanel.h"
#include "../BlueprintEditor/BTNodeGraphManager.h"
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include "../EditorCommon/EditorAutosaveManager.h"

 // Forward declarations for SDL3
struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;
struct ImGuiContext;

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

        // Inspector helpers
        void RenderRuntimeInfo();
        void RenderBlackboardSection();
        void RenderExecutionLog();

        // Data management
        std::vector<EntityDebugInfo> m_entities;
        std::vector<EntityDebugInfo> m_filteredEntities;
        EntityID m_selectedEntity = 0;

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

        // ImNodes context flag (shared with NodeGraphPanel rendering pipeline)
        bool m_imnodesInitialized = false;

        // Separate SDL3 window for debugger
        SDL_Window* m_separateWindow;
        SDL_Renderer* m_separateRenderer;
        bool m_windowCreated;

        // Separate ImGui context for this window
        ImGuiContext* m_separateImGuiContext;

        // Async autosave – persists node positions without blocking the UI.
        EditorAutosaveManager m_autosave;

        // --- Unified NodeGraph Debug Panel (Blueprint Editor pipeline) ---
        void InitNodeGraphDebugMode();
        void ShutdownNodeGraphDebugMode();
        void RenderNodeGraphDebugPanel();

        NodeGraphPanel  m_nodeGraphPanel;
        int             m_debugGraphId    = -1;
        uint32_t        m_lastDebugTreeId = 0;
    };
}
