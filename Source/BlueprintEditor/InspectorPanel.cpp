/*
 * Olympe Blueprint Editor - Inspector Panel Implementation
 */

#include "InspectorPanel.h"
#include "BlueprintEditor.h"
#include "EntityInspectorManager.h"
#include "BTNodeGraphManager.h"
#include "EnumCatalogManager.h"
#include "../TaskSystem/LocalBlackboard.h"
#include "../TaskSystem/TaskGraphTypes.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>
#include <string>
#include <cstring>

namespace Olympe
{
    // Static member definition
    const LocalBlackboard* InspectorPanel::s_DebugBlackboard = nullptr;

    void InspectorPanel::SetDebugBlackboard(const LocalBlackboard* bb)
    {
        s_DebugBlackboard = bb;
    }

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

        // Runtime debug overlay: always shown when a debug blackboard is registered
        RenderDebugBlackboard();

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

    // =========================================================================
    // RenderDebugBlackboard
    // =========================================================================

    void InspectorPanel::RenderDebugBlackboard()
    {
        if (s_DebugBlackboard == nullptr)
            return;

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Runtime Blackboard");
        ImGui::Separator();

        std::vector<std::string> varNames = s_DebugBlackboard->GetVariableNames();
        if (varNames.empty())
        {
            ImGui::TextDisabled("(no variables)");
            return;
        }

        for (const auto& varName : varNames)
        {
            TaskValue val = s_DebugBlackboard->GetValue(varName);
            switch (val.GetType())
            {
            case VariableType::Bool:
                ImGui::Text("  %s: %s", varName.c_str(), val.AsBool() ? "true" : "false");
                break;
            case VariableType::Int:
                ImGui::Text("  %s: %d", varName.c_str(), val.AsInt());
                break;
            case VariableType::Float:
                ImGui::Text("  %s: %.4f", varName.c_str(), val.AsFloat());
                break;
            case VariableType::String:
                ImGui::Text("  %s: \"%s\"", varName.c_str(), val.AsString().c_str());
                break;
            case VariableType::EntityID:
                ImGui::Text("  %s: entity(%llu)", varName.c_str(),
                    static_cast<unsigned long long>(val.AsEntityID()));
                break;
            case VariableType::Vector:
            {
                ::Vector v = val.AsVector();
                ImGui::Text("  %s: (%.2f, %.2f, %.2f)", varName.c_str(), v.x, v.y, v.z);
                break;
            }
            default:
                ImGui::Text("  %s: (none)", varName.c_str());
                break;
            }
        }
    }
}
