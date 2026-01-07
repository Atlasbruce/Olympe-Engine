/*
 * Olympe Blueprint Editor - Inspector Panel Implementation
 */

#include "InspectorPanel.h"
#include "BlueprintEditor.h"
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

        case InspectorContext::AssetFile:
            RenderAssetFileInspector();
            break;

        case InspectorContext::None:
        default:
            ImGui::Text("No selection");
            ImGui::TextWrapped("Select an entity or asset file to inspect its properties.");
            break;
        }

        ImGui::End();
    }

    InspectorContext InspectorPanel::DetermineContext()
    {
        // C) Priority 1: Entity selection from BlueprintEditor backend
        if (BlueprintEditor::Get().HasSelectedEntity())
        {
            return InspectorContext::RuntimeEntity;
        }

        // Priority 2: Asset file selection from BlueprintEditor backend
        if (BlueprintEditor::Get().HasSelectedAsset())
        {
            return InspectorContext::AssetFile;
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
        // C) Get selected entity from BlueprintEditor backend
        uint64_t selectedEntity = BlueprintEditor::Get().GetSelectedEntity();

        if (selectedEntity == 0)  // INVALID_ENTITY_ID
        {
            ImGui::Text("No entity selected");
            ImGui::TextWrapped("Select an entity from the Asset Browser or Entities panel to inspect its properties.");
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

    void InspectorPanel::RenderAssetFileInspector()
    {
        // Get selected asset path from BlueprintEditor backend
        std::string selectedAssetPath = BlueprintEditor::Get().GetSelectedAssetPath();
        
        if (selectedAssetPath.empty())
        {
            ImGui::Text("No asset selected");
            return;
        }
        
        // Get asset metadata from backend
        AssetMetadata metadata = BlueprintEditor::Get().GetAssetMetadata(selectedAssetPath);
        
        // Extract filename from path
        size_t lastSlash = selectedAssetPath.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) 
            ? selectedAssetPath.substr(lastSlash + 1) 
            : selectedAssetPath;
        
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Asset: %s", filename.c_str());
        ImGui::Separator();
        
        if (!metadata.isValid)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Invalid or malformed asset");
            if (!metadata.errorMessage.empty())
            {
                ImGui::TextWrapped("Error: %s", metadata.errorMessage.c_str());
            }
            return;
        }
        
        // Display asset type
        ImGui::Text("Type: %s", metadata.type.c_str());
        
        // Display name and description if available
        if (!metadata.name.empty())
        {
            ImGui::Text("Name: %s", metadata.name.c_str());
        }
        
        if (!metadata.description.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("Description: %s", metadata.description.c_str());
        }
        
        ImGui::Separator();
        
        // Type-specific information
        if (metadata.type == "BehaviorTree" || metadata.type == "HFSM")
        {
            ImGui::Text("Nodes: %d", metadata.nodeCount);
            
            if (!metadata.nodes.empty() && ImGui::CollapsingHeader("Node List"))
            {
                ImGui::Indent();
                for (const auto& node : metadata.nodes)
                {
                    ImGui::BulletText("%s", node.c_str());
                }
                ImGui::Unindent();
            }
            
            ImGui::Separator();
            if (ImGui::Button("Open in Node Graph Editor"))
            {
                BlueprintEditor::Get().OpenGraphInEditor(selectedAssetPath);
            }
        }
        else if (metadata.type == "EntityBlueprint")
        {
            ImGui::Text("Components: %d", metadata.componentCount);
            
            if (!metadata.components.empty() && ImGui::CollapsingHeader("Component List"))
            {
                ImGui::Indent();
                for (const auto& comp : metadata.components)
                {
                    ImGui::BulletText("%s", comp.c_str());
                }
                ImGui::Unindent();
            }
        }
        
        // Full file path at the bottom
        ImGui::Separator();
        ImGui::TextDisabled("Path: %s", selectedAssetPath.c_str());
    }
}
