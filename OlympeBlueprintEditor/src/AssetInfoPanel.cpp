/*
 * Olympe Blueprint Editor - Asset Info Panel Implementation
 */

#include "../include/AssetInfoPanel.h"
#include "../third_party/imgui/imgui.h"
#include "../../Source/json_helper.h"
#include <iostream>

namespace Olympe {

AssetInfoPanel::AssetInfoPanel()
    : m_HasValidData(false)
{
}

AssetInfoPanel::~AssetInfoPanel()
{
}

void AssetInfoPanel::Clear()
{
    m_LastAssetPath.clear();
    m_CachedJson.clear();
    m_HasValidData = false;
}

void AssetInfoPanel::Render(const std::string& assetPath, const std::string& assetType)
{
    ImGui::Begin("Asset Info");

    if (assetPath.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No asset selected");
        ImGui::Text("");
        ImGui::Text("Select an asset from the Asset Browser");
        ImGui::Text("to view detailed information");
        ImGui::End();
        return;
    }

    // Load asset JSON if it's different from last time
    if (assetPath != m_LastAssetPath)
    {
        m_LastAssetPath = assetPath;
        m_HasValidData = JsonHelper::LoadJsonFromFile(assetPath, m_CachedJson);
        
        if (!m_HasValidData)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error loading asset!");
            ImGui::Text("");
            ImGui::Text("Failed to parse JSON file:");
            ImGui::TextWrapped("%s", assetPath.c_str());
            ImGui::Text("");
            ImGui::Text("The file may be corrupted or contain invalid JSON.");
            ImGui::End();
            return;
        }
    }

    if (!m_HasValidData)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Invalid asset data");
        ImGui::End();
        return;
    }

    // Display based on asset type
    if (assetType == "Entity" || assetType == "EntityBlueprint")
    {
        DisplayEntityInfo(m_CachedJson);
    }
    else if (assetType == "BehaviorTree")
    {
        DisplayBTInfo(m_CachedJson);
    }
    else if (assetType == "HFSM")
    {
        DisplayHFSMInfo(m_CachedJson);
    }
    else if (assetType == "Prefab")
    {
        DisplayPrefabInfo(m_CachedJson);
    }
    else
    {
        DisplayGenericInfo(m_CachedJson);
    }

    // Always show metadata section at the bottom
    ImGui::Separator();
    DisplayMetadata(m_CachedJson);

    // Show linked resources if any
    ImGui::Separator();
    DisplayLinkedResources(m_CachedJson);

    ImGui::End();
}

void AssetInfoPanel::DisplayEntityInfo(const json& j)
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Entity Blueprint");
    ImGui::Separator();

    // Name
    std::string name = JsonHelper::GetString(j, "name", "Unnamed");
    ImGui::Text("Name: %s", name.c_str());

    // Description
    std::string description = JsonHelper::GetString(j, "description", "No description");
    ImGui::Text("Description:");
    ImGui::TextWrapped("%s", description.c_str());

    ImGui::Separator();

    // Components
    if (JsonHelper::IsArray(j, "components"))
    {
        size_t componentCount = JsonHelper::GetArraySize(j, "components");
        ImGui::Text("Components: %zu", componentCount);

        if (ImGui::TreeNode("Component List"))
        {
            JsonHelper::ForEachInArray(j, "components", [](const json& comp, size_t index) {
                std::string type = JsonHelper::GetString(comp, "type", "Unknown");
                ImGui::BulletText("%s", type.c_str());
                
                if (ImGui::IsItemHovered() && comp.contains("properties"))
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Properties:");
                    std::string props = comp["properties"].dump(2);
                    // Truncate if too long
                    if (props.length() > 200)
                        props = props.substr(0, 200) + "...";
                    ImGui::TextWrapped("%s", props.c_str());
                    ImGui::EndTooltip();
                }
            });

            ImGui::TreePop();
        }
    }
}

void AssetInfoPanel::DisplayBTInfo(const json& j)
{
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Behavior Tree");
    ImGui::Separator();

    // Name
    std::string name = JsonHelper::GetString(j, "name", "Unnamed");
    ImGui::Text("Name: %s", name.c_str());

    // Root node ID
    int rootNodeId = JsonHelper::GetInt(j, "rootNodeId", -1);
    ImGui::Text("Root Node ID: %d", rootNodeId);

    ImGui::Separator();

    // Nodes
    if (JsonHelper::IsArray(j, "nodes"))
    {
        size_t nodeCount = JsonHelper::GetArraySize(j, "nodes");
        ImGui::Text("Nodes: %zu", nodeCount);

        if (ImGui::TreeNode("Node List"))
        {
            JsonHelper::ForEachInArray(j, "nodes", [](const json& node, size_t index) {
                int id = JsonHelper::GetInt(node, "id", -1);
                std::string nodeName = JsonHelper::GetString(node, "name", "Unnamed Node");
                std::string type = JsonHelper::GetString(node, "type", "Unknown");

                ImGui::BulletText("[%d] %s (%s)", id, nodeName.c_str(), type.c_str());

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    if (node.contains("actionType"))
                    {
                        std::string actionType = JsonHelper::GetString(node, "actionType", "");
                        ImGui::Text("Action Type: %s", actionType.c_str());
                    }
                    if (node.contains("children"))
                    {
                        ImGui::Text("Has children");
                    }
                    ImGui::EndTooltip();
                }
            });

            ImGui::TreePop();
        }
    }
}

void AssetInfoPanel::DisplayHFSMInfo(const json& j)
{
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "Hierarchical Finite State Machine");
    ImGui::Separator();

    std::string name = JsonHelper::GetString(j, "name", "Unnamed");
    ImGui::Text("Name: %s", name.c_str());

    // States
    if (JsonHelper::IsArray(j, "states"))
    {
        size_t stateCount = JsonHelper::GetArraySize(j, "states");
        ImGui::Text("States: %zu", stateCount);
    }

    // Transitions
    if (JsonHelper::IsArray(j, "transitions"))
    {
        size_t transitionCount = JsonHelper::GetArraySize(j, "transitions");
        ImGui::Text("Transitions: %zu", transitionCount);
    }
}

void AssetInfoPanel::DisplayPrefabInfo(const json& j)
{
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 1.0f, 1.0f), "Prefab");
    ImGui::Separator();

    std::string name = JsonHelper::GetString(j, "name", "Unnamed");
    ImGui::Text("Name: %s", name.c_str());

    std::string description = JsonHelper::GetString(j, "description", "No description");
    ImGui::Text("Description:");
    ImGui::TextWrapped("%s", description.c_str());

    // Template
    if (j.contains("template"))
    {
        std::string templateName = JsonHelper::GetString(j, "template", "");
        if (!templateName.empty())
        {
            ImGui::Text("Based on template: %s", templateName.c_str());
        }
    }

    // Components (prefabs may have components too)
    if (JsonHelper::IsArray(j, "components"))
    {
        size_t componentCount = JsonHelper::GetArraySize(j, "components");
        ImGui::Text("Components: %zu", componentCount);
    }
}

void AssetInfoPanel::DisplayGenericInfo(const json& j)
{
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Asset");
    ImGui::Separator();

    // Display common fields
    if (j.contains("name"))
    {
        std::string name = JsonHelper::GetString(j, "name", "Unnamed");
        ImGui::Text("Name: %s", name.c_str());
    }

    if (j.contains("type"))
    {
        std::string type = JsonHelper::GetString(j, "type", "Unknown");
        ImGui::Text("Type: %s", type.c_str());
    }

    if (j.contains("description"))
    {
        std::string description = JsonHelper::GetString(j, "description", "");
        if (!description.empty())
        {
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", description.c_str());
        }
    }

    // Show raw JSON structure
    ImGui::Separator();
    if (ImGui::TreeNode("Raw JSON"))
    {
        std::string jsonStr = j.dump(2);
        ImGui::TextWrapped("%s", jsonStr.c_str());
        ImGui::TreePop();
    }
}

void AssetInfoPanel::DisplayLinkedResources(const json& j)
{
    ImGui::Text("Linked Resources:");

    std::vector<std::string> resources;

    // Recursively search for sprite paths
    std::function<void(const json&)> findResources = [&](const json& node) {
        if (node.is_object())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                const std::string& key = it.key();
                const json& value = it.value();

                // Look for common resource keys
                if ((key == "spritePath" || key == "texturePath" || 
                     key == "soundPath" || key == "audioPath" ||
                     key == "fxPath" || key == "effectPath") && value.is_string())
                {
                    std::string path = value.get<std::string>();
                    if (!path.empty())
                        resources.push_back(path);
                }

                // Recurse into nested objects/arrays
                if (value.is_object() || value.is_array())
                {
                    findResources(value);
                }
            }
        }
        else if (node.is_array())
        {
            for (const auto& item : node)
            {
                findResources(item);
            }
        }
    };

    findResources(j);

    if (resources.empty())
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  None");
    }
    else
    {
        for (const auto& res : resources)
        {
            ImGui::BulletText("%s", res.c_str());
        }
    }
}

void AssetInfoPanel::DisplayMetadata(const json& j)
{
    ImGui::Text("Metadata:");

    // Schema version
    if (j.contains("schema_version"))
    {
        int version = JsonHelper::GetInt(j, "schema_version", 0);
        ImGui::Text("  Schema Version: %d", version);
    }

    // Version
    if (j.contains("version"))
    {
        std::string version = JsonHelper::GetString(j, "version", "");
        if (!version.empty())
            ImGui::Text("  Version: %s", version.c_str());
    }

    // Author
    if (j.contains("author"))
    {
        std::string author = JsonHelper::GetString(j, "author", "");
        if (!author.empty())
            ImGui::Text("  Author: %s", author.c_str());
    }

    // Created date
    if (j.contains("created"))
    {
        std::string created = JsonHelper::GetString(j, "created", "");
        if (!created.empty())
            ImGui::Text("  Created: %s", created.c_str());
    }

    // Modified date
    if (j.contains("modified"))
    {
        std::string modified = JsonHelper::GetString(j, "modified", "");
        if (!modified.empty())
            ImGui::Text("  Modified: %s", modified.c_str());
    }
}

} // namespace Olympe
