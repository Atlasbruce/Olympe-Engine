/*
 * Olympe Blueprint Editor - Inspector Panel
 * 
 * Dynamic inspector that adapts to context:
 * - Shows node properties when a graph node is selected
 * - Shows entity components when a runtime entity is selected
 */

#pragma once

#include <string>

namespace Olympe
{
    enum class InspectorContext
    {
        None,
        GraphNode,
        RuntimeEntity,
        AssetFile  // New context for asset file metadata
    };

    /**
     * InspectorPanel - Adaptive inspector panel
     * Displays properties based on current selection context
     */
    class InspectorPanel
    {
    public:
        InspectorPanel();
        ~InspectorPanel();

        void Initialize();
        void Shutdown();
        void Render();

    private:
        void RenderNodeInspector();
        void RenderEntityInspector();
        void RenderAssetFileInspector();  // New method for asset metadata
        void RenderComponentProperties(uint64_t entityId, const std::string& componentType);
        
        InspectorContext DetermineContext();
    };
}
