/*
 * Olympe Blueprint Editor - Entities Panel Implementation
 */

#include "EntitiesPanel.h"
#include "BlueprintEditor.h"
#include "EntityInspectorManager.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>

namespace Olympe
{
    EntitiesPanel::EntitiesPanel()
    {
        m_FilterBuffer[0] = '\0';
    }

    EntitiesPanel::~EntitiesPanel()
    {
    }

    void EntitiesPanel::Initialize()
    {
        std::cout << "[EntitiesPanel] Initialized\n";
    }

    void EntitiesPanel::Shutdown()
    {
        std::cout << "[EntitiesPanel] Shutdown\n";
    }

    void EntitiesPanel::Render()
    {
        ImGui::Begin("Runtime Entities");

        // Header with entity count
        size_t entityCount = EntityInspectorManager::Get().GetEntityCount();
        ImGui::Text("Entities: %zu", entityCount);

        ImGui::Separator();

        // Filter input
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::InputText("##EntityFilter", m_FilterBuffer, sizeof(m_FilterBuffer));

        ImGui::Separator();

        // Entity list
        RenderEntityList();

        ImGui::End();
    }

    void EntitiesPanel::RenderEntityList()
    {
        if (!EntityInspectorManager::Get().IsInitialized())
        {
            ImGui::Text("Entity inspector not initialized.");
            return;
        }

        // Get all entities
        std::vector<EntityInfo> entities = EntityInspectorManager::Get().GetAllEntityInfo();

        // Apply filter if any
        std::string filter(m_FilterBuffer);

        ImGui::BeginChild("EntityListScroll", ImVec2(0, 0), true);

        for (const auto& entityInfo : entities)
        {
            // Apply name filter
            if (!filter.empty() && entityInfo.name.find(filter) == std::string::npos)
                continue;

            RenderEntityItem(entityInfo.id, entityInfo.name);
        }

        ImGui::EndChild();
    }

    void EntitiesPanel::RenderEntityItem(uint64_t entityId, const std::string& entityName)
    {
        // C) Use BlueprintEditor backend for selection state
        bool isSelected = (BlueprintEditor::Get().GetSelectedEntity() == entityId);

        // Selectable entity item
        if (ImGui::Selectable(entityName.c_str(), isSelected))
        {
            // C) Set selection in BlueprintEditor backend - this will synchronize all panels
            BlueprintEditor::Get().SetSelectedEntity(entityId);
        }

        // Context menu
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::Text("Entity: %s", entityName.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Select"))
            {
                BlueprintEditor::Get().SetSelectedEntity(entityId);
            }

            // Note: Destroy would require access to World
            // if (ImGui::MenuItem("Destroy"))
            // {
            //     World::Get().DestroyEntity(entityId);
            // }

            ImGui::EndPopup();
        }

        // Show component count on hover
        if (ImGui::IsItemHovered())
        {
            auto components = EntityInspectorManager::Get().GetEntityComponents(entityId);
            ImGui::BeginTooltip();
            ImGui::Text("Entity ID: %llu", entityId);
            ImGui::Text("Components: %zu", components.size());
            if (!components.empty())
            {
                ImGui::Separator();
                for (const auto& comp : components)
                {
                    ImGui::BulletText("%s", comp.c_str());
                }
            }
            ImGui::EndTooltip();
        }
    }
}
