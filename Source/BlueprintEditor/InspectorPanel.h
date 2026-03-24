/*
 * Olympe Blueprint Editor - Inspector Panel
 * 
 * Dynamic inspector that adapts to context:
 * - Shows node properties when a graph node is selected
 * - Shows entity components when a runtime entity is selected
 * - Shows live LocalBlackboard values when a debug blackboard is registered
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>

namespace Olympe
{
    // Forward declarations
    class LocalBlackboard;
    class ActionParametersPanel;

    enum class InspectorContext
    {
        None,
        GraphNode,
        RuntimeEntity,
        AssetFile  // New context for asset file metadata
    };

    /**
     * InspectorPanel - Adaptive inspector panel
     * Displays properties based on current selection context.
     *
     * Runtime debug overlay:
     *   Call SetDebugBlackboard() with a pointer to the live LocalBlackboard of
     *   the currently executing task graph to make InspectorPanel show its values
     *   as an overlay section.  Pass nullptr to hide the section.
     *   The pointer is not owned and must remain valid for the frame duration.
     */
    class InspectorPanel
    {
    public:
        InspectorPanel();
        ~InspectorPanel();

        void Initialize();
        void Shutdown();
        void Render();
        void RenderContent();  // Render without window wrapper - for fixed layout

        /**
         * @brief Register a live LocalBlackboard to display in the inspector.
         * @param bb  Pointer to the blackboard (non-owning).  nullptr removes it.
         */
        static void SetDebugBlackboard(const LocalBlackboard* bb);

        /**
         * @brief Get the active InspectorPanel instance (singleton access).
         *        Returns nullptr if not initialized.
         */
        static InspectorPanel* GetInstance()  { return s_Instance; }

        /**
         * @brief Set the active InspectorPanel instance.
         *        Called internally by Initialize().
         */
        static void SetInstance(InspectorPanel* instance) { s_Instance = instance; }

        /**
         * @brief Set the currently selected action node for parameter editing.
         * @param taskID     The task ID (e.g., "log_message", "patrol_path")
         * @param nodeName   The display name of the node
         * @param parameters Map of parameter name -> value
         */
        void SetSelectedActionNode(const std::string& taskID,
                                   const std::string& nodeName,
                                   const std::unordered_map<std::string, std::string>& parameters);

        /**
         * @brief Clear the current action node selection.
         */
        void ClearSelectedActionNode();

        /**
         * @brief Get the action parameters panel (for direct access if needed).
         */
        ActionParametersPanel* GetActionPanel() { return m_actionPanel.get(); }

    private:
        void RenderNodeInspector();
        void RenderEntityInspector();
        void RenderAssetFileInspector();
        void RenderActionNodeInspector();
        void RenderDebugBlackboard();
        void RenderComponentProperties(uint64_t entityId, const std::string& componentType);

        InspectorContext DetermineContext();

        static const LocalBlackboard* s_DebugBlackboard;
        std::unique_ptr<ActionParametersPanel> m_actionPanel;
        bool m_hasSelectedAction = false;

        // Singleton instance pointer
        static InspectorPanel* s_Instance;
    };
}
