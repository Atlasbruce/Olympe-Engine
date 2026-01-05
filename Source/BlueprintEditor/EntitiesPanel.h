/*
 * Olympe Blueprint Editor - Entities Panel
 * 
 * GUI panel displaying all runtime entities
 * Allows filtering, selection, and basic entity operations
 */

#pragma once

#include <string>

namespace Olympe
{
    /**
     * EntitiesPanel - ImGui panel for entity list
     * Shows all runtime entities from the World ECS
     */
    class EntitiesPanel
    {
    public:
        EntitiesPanel();
        ~EntitiesPanel();

        void Initialize();
        void Shutdown();
        void Render();

    private:
        void RenderEntityList();
        void RenderEntityItem(uint64_t entityId, const std::string& entityName);

        // Filtering
        char m_FilterBuffer[256];
        std::string m_ComponentFilter;
    };
}
