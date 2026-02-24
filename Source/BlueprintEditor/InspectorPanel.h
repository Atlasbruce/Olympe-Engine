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

namespace Olympe
{
    // Forward declaration: avoid pulling in LocalBlackboard headers into the GUI layer
    class LocalBlackboard;

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

        /**
         * @brief Register a live LocalBlackboard to display in the inspector.
         * @param bb  Pointer to the blackboard (non-owning).  nullptr removes it.
         */
        static void SetDebugBlackboard(const LocalBlackboard* bb);

    private:
        void RenderNodeInspector();
        void RenderEntityInspector();
        void RenderAssetFileInspector();
        void RenderDebugBlackboard();
        void RenderComponentProperties(uint64_t entityId, const std::string& componentType);
        
        InspectorContext DetermineContext();

        static const LocalBlackboard* s_DebugBlackboard;
    };
}
