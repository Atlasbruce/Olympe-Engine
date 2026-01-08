/*
 * Olympe Blueprint Editor - Entity Prefab Plugin Implementation
 */

#include "EntityPrefabEditorPlugin.h"
#include "../../Source/third_party/imgui/imgui.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>

using json = nlohmann::json;

namespace Olympe
{
    EntityPrefabEditorPlugin::EntityPrefabEditorPlugin()
    {
    }
    
    EntityPrefabEditorPlugin::~EntityPrefabEditorPlugin()
    {
    }
    
    std::string EntityPrefabEditorPlugin::GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }
    
    json EntityPrefabEditorPlugin::CreateNew(const std::string& name)
    {
        json prefab;
        prefab["schema_version"] = 2;
        prefab["blueprintType"] = "EntityPrefab";
        prefab["name"] = name;
        prefab["description"] = "";
        prefab["metadata"]["author"] = "Atlasbruce";
        prefab["metadata"]["created"] = GetCurrentTimestamp();
        prefab["metadata"]["lastModified"] = GetCurrentTimestamp();
        prefab["metadata"]["tags"] = json::array();
        
        // Components by default
        prefab["data"]["prefabName"] = name;
        prefab["data"]["components"] = json::array();
        
        // Identity + Position by default
        json identity;
        identity["type"] = "Identity_data";
        identity["properties"]["name"] = name + "_{id}";
        identity["properties"]["tag"] = name;
        identity["properties"]["entityType"] = "Generic";
        
        json position;
        position["type"] = "Position_data";
        position["properties"]["position"] = {{"x", 0}, {"y", 0}, {"z", 0}};
        
        prefab["data"]["components"].push_back(identity);
        prefab["data"]["components"].push_back(position);
        
        return prefab;
    }
    
    bool EntityPrefabEditorPlugin::CanHandle(const json& blueprint) const
    {
        // V2 format check
        if (blueprint.contains("blueprintType") && 
            blueprint["blueprintType"] == "EntityPrefab")
        {
            return true;
        }
        
        // V1 format heuristic
        return blueprint.contains("components");
    }
    
    std::vector<ValidationError> EntityPrefabEditorPlugin::Validate(const json& blueprint)
    {
        std::vector<ValidationError> errors;
        
        if (!blueprint.contains("data"))
        {
            errors.push_back(ValidationError("", "Missing 'data' section", ErrorSeverity::Error));
            return errors;
        }
        
        const json& data = blueprint["data"];
        
        if (!data.contains("components"))
        {
            errors.push_back(ValidationError("", "Missing 'components' array", ErrorSeverity::Error));
        }
        
        return errors;
    }
    
    void EntityPrefabEditorPlugin::RenderEditor(json& blueprintData, EditorContext& ctx)
    {
        if (!blueprintData.contains("data"))
        {
            ImGui::Text("Invalid blueprint: missing 'data' section");
            return;
        }
        
        auto& data = blueprintData["data"];
        
        if (!data.contains("components"))
        {
            data["components"] = json::array();
        }
        
        auto& components = data["components"];
        
        ImGui::Text("Components: %d", (int)components.size());
        ImGui::Separator();
        
        // List components
        size_t toRemove = (size_t)-1;
        for (size_t i = 0; i < components.size(); ++i)
        {
            auto& comp = components[i];
            std::string compType = comp.value("type", "Unknown");
            
            ImGui::PushID((int)i);
            
            bool headerOpen = ImGui::CollapsingHeader(compType.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            
            // Remove button
            ImGui::SameLine(ImGui::GetWindowWidth() - 80);
            if (ImGui::SmallButton("Remove"))
            {
                toRemove = i;
            }
            
            if (headerOpen)
            {
                ImGui::Indent();
                if (comp.contains("properties"))
                {
                    RenderComponentPropertiesEditor(comp["properties"], ctx);
                }
                ImGui::Unindent();
            }
            
            ImGui::PopID();
        }
        
        // Remove component if requested
        if (toRemove != (size_t)-1)
        {
            components.erase(toRemove);
            ctx.MarkDirty();
        }
        
        ImGui::Separator();
        
        // Add Component button
        if (ImGui::Button("Add Component", ImVec2(-1, 0)))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }
        
        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            auto availableTypes = GetAvailableComponentTypes();
            for (const auto& type : availableTypes)
            {
                if (ImGui::MenuItem(type.c_str()))
                {
                    AddComponentToBlueprint(blueprintData, type);
                    ctx.MarkDirty();
                }
            }
            ImGui::EndPopup();
        }
    }
    
    void EntityPrefabEditorPlugin::RenderProperties(const json& blueprintData)
    {
        ImGui::Text("Type: Entity Prefab");
        
        if (blueprintData.contains("data") && blueprintData["data"].contains("components"))
        {
            ImGui::Text("Components: %d", (int)blueprintData["data"]["components"].size());
        }
        else
        {
            ImGui::Text("Components: 0");
        }
    }
    
    void EntityPrefabEditorPlugin::RenderToolbar(json& blueprintData)
    {
        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponentToolbar");
        }
    }
    
    void EntityPrefabEditorPlugin::RenderComponentPropertiesEditor(json& properties, EditorContext& ctx)
    {
        for (auto& [key, value] : properties.items())
        {
            ImGui::PushID(key.c_str());
            
            if (value.is_number_float())
            {
                float f = value.get<float>();
                if (ImGui::DragFloat(key.c_str(), &f, 1.0f))
                {
                    properties[key] = f;
                    ctx.MarkDirty();
                }
            }
            else if (value.is_number_integer())
            {
                int i = value.get<int>();
                if (ImGui::InputInt(key.c_str(), &i))
                {
                    properties[key] = i;
                    ctx.MarkDirty();
                }
            }
            else if (value.is_string())
            {
                std::string str = value.get<std::string>();
                char buffer[256];
                strncpy(buffer, str.c_str(), 255);
                buffer[255] = '\0';
                if (ImGui::InputText(key.c_str(), buffer, 256))
                {
                    properties[key] = std::string(buffer);
                    ctx.MarkDirty();
                }
            }
            else if (value.is_object())
            {
                // Nested object (e.g., position with x,y,z)
                if (ImGui::TreeNode(key.c_str()))
                {
                    RenderComponentPropertiesEditor(value, ctx);
                    ImGui::TreePop();
                }
            }
            
            ImGui::PopID();
        }
    }
    
    void EntityPrefabEditorPlugin::AddComponentToBlueprint(json& blueprintData, const std::string& type)
    {
        json newComp;
        newComp["type"] = type;
        newComp["properties"] = json::object();
        
        blueprintData["data"]["components"].push_back(newComp);
    }
    
    std::vector<std::string> EntityPrefabEditorPlugin::GetAvailableComponentTypes()
    {
        return {
            "Identity_data", "Position_data", "VisualSprite_data",
            "BoundingBox_data", "Movement_data", "PhysicsBody_data",
            "Health_data", "PlayerBinding_data", "Controller_data",
            "AIBlackboard_data", "AISenses_data", "AIState_data",
            "BehaviorTreeRuntime_data", "MoveIntent_data", "AttackIntent_data"
        };
    }
}
