/*
 * Olympe Blueprint Editor - Inspector Panel Implementation
 */

#include "InspectorPanel.h"
#include "EntityInspectorManager.h"
#include "NodeGraphManager.h"
#include "EnumCatalogManager.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>
#include <string>
#include <cstring>

namespace Olympe
{
    InspectorPanel::InspectorPanel()
    {
    }

    InspectorPanel::~InspectorPanel()
    {
    }

    void InspectorPanel::Initialize()
    {
        std::cout << "[InspectorPanel] Initialized\n";
    }

    void InspectorPanel::Shutdown()
    {
        std::cout << "[InspectorPanel] Shutdown\n";
    }

    void InspectorPanel::Render()
    {
        ImGui::Begin("Inspector");

        InspectorContext context = DetermineContext();

        switch (context)
        {
        case InspectorContext::GraphNode:
            RenderNodeInspector();
            break;

        case InspectorContext::RuntimeEntity:
            RenderEntityInspector();
            break;

        case InspectorContext::None:
        default:
            ImGui::Text("No selection");
            ImGui::TextWrapped("Select a node in the graph editor or an entity in the entities panel to inspect its properties.");
            break;
        }

        ImGui::End();
    }

    InspectorContext InspectorPanel::DetermineContext()
    {
        // Priority: Entity selection over node selection
        if (EntityInspectorManager::Get().HasSelection())
        {
            return InspectorContext::RuntimeEntity;
        }

        // Check if a graph node is selected
        // For now, we'll assume no node selection (would need to track in NodeGraphPanel)
        
        return InspectorContext::None;
    }

    void InspectorPanel::RenderNodeInspector()
    {
        ImGui::Text("Node Properties");
        ImGui::Separator();

        // This would show properties of the selected graph node
        // Would require selected node tracking in NodeGraphPanel
        
        ImGui::Text("Node inspector coming soon...");
    }

    void InspectorPanel::RenderEntityInspector()
    {
        uint64_t selectedEntity = EntityInspectorManager::Get().GetSelectedEntity();

        if (selectedEntity == 0)  // INVALID_ENTITY_ID
        {
            ImGui::Text("No entity selected");
            return;
        }

        EntityInfo info = EntityInspectorManager::Get().GetEntityInfo(selectedEntity);

        ImGui::Text("Entity: %s", info.name.c_str());
        ImGui::Text("ID: %llu", selectedEntity);
        ImGui::Separator();

        // Show components
        ImGui::Text("Components:");

        auto components = EntityInspectorManager::Get().GetEntityComponents(selectedEntity);

        if (components.empty())
        {
            ImGui::Text("  (no components)");
        }
        else
        {
            for (const auto& componentType : components)
            {
                if (ImGui::CollapsingHeader(componentType.c_str()))
                {
                    RenderComponentProperties(selectedEntity, componentType);
                }
            }
        }
    }

    void InspectorPanel::RenderComponentProperties(uint64_t entityId, const std::string& componentType)
    {
        auto properties = EntityInspectorManager::Get().GetComponentProperties(entityId, componentType);

        if (properties.empty())
        {
            ImGui::Text("  (no editable properties)");
            return;
        }

        for (auto& prop : properties)
        {
            ImGui::PushID(prop.name.c_str());

            if (prop.type == "float")
            {
                float value = std::stof(prop.value);
                if (ImGui::DragFloat(prop.name.c_str(), &value, 1.0f))
                {
                    // Update property
                    EntityInspectorManager::Get().SetComponentProperty(
                        entityId, componentType, prop.name, std::to_string(value)
                    );
                }
            }
            else if (prop.type == "int")
            {
                int value = std::stoi(prop.value);
                if (ImGui::DragInt(prop.name.c_str(), &value))
                {
                    EntityInspectorManager::Get().SetComponentProperty(
                        entityId, componentType, prop.name, std::to_string(value)
                    );
                }
            }
            else if (prop.type == "bool")
            {
                bool value = (prop.value == "true" || prop.value == "1");
                if (ImGui::Checkbox(prop.name.c_str(), &value))
                {
                    EntityInspectorManager::Get().SetComponentProperty(
                        entityId, componentType, prop.name, value ? "true" : "false"
                    );
                }
            }
            else if (prop.type == "string")
            {
                char buffer[256];
                strncpy_s(buffer, prop.value.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText(prop.name.c_str(), buffer, sizeof(buffer)))
                {
                    EntityInspectorManager::Get().SetComponentProperty(
                        entityId, componentType, prop.name, buffer
                    );
                }
            }
            else
            {
                // Default: display as text
                ImGui::Text("%s: %s", prop.name.c_str(), prop.value.c_str());
            }

            ImGui::PopID();
        }
    }
}
