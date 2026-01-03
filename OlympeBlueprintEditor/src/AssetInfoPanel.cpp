/*
 * Olympe Blueprint Editor - Asset Info Panel Implementation
 */

#include "../include/AssetInfoPanel.h"
#include "../../Source/json_helper.h"
#include <iostream>
#include <fstream>

// Forward declare ImGui functions
namespace ImGui
{
    bool Begin(const char* name, bool* p_open = nullptr, int flags = 0);
    void End();
    void Text(const char* fmt, ...);
    void TextColored(const struct ImVec4& col, const char* fmt, ...);
    void TextWrapped(const char* fmt, ...);
    void Separator();
    bool CollapsingHeader(const char* label, int flags = 0);
    void BulletText(const char* fmt, ...);
    void Spacing();
}

struct ImVec4 { float x, y, z, w; ImVec4(float _x = 0, float _y = 0, float _z = 0, float _w = 0) : x(_x), y(_y), z(_z), w(_w) {} };

namespace Olympe
{
    AssetInfoPanel::AssetInfoPanel()
    {
    }

    AssetInfoPanel::~AssetInfoPanel()
    {
    }

    void AssetInfoPanel::LoadAsset(const std::string& filepath)
    {
        Clear();
        
        m_CurrentAsset.filepath = filepath;

        try
        {
            json j;
            if (!JsonHelper::LoadJsonFromFile(filepath, j))
            {
                m_CurrentAsset.isValid = false;
                m_CurrentAsset.errorMessage = "Failed to load JSON file: " + filepath;
                std::cerr << m_CurrentAsset.errorMessage << std::endl;
                return;
            }

            // Determine asset type and parse accordingly
            if (j.contains("type"))
            {
                std::string type = JsonHelper::GetString(j, "type", "");
                m_CurrentAsset.type = type;
                
                if (type == "EntityBlueprint")
                {
                    ParseEntityBlueprint(j);
                }
                else
                {
                    ParseGenericAsset(j);
                }
            }
            else if (j.contains("rootNodeId") && j.contains("nodes"))
            {
                // Behavior Tree
                m_CurrentAsset.type = "BehaviorTree";
                ParseBehaviorTree(j);
            }
            else if (j.contains("components"))
            {
                // Entity Blueprint without explicit type
                m_CurrentAsset.type = "EntityBlueprint";
                ParseEntityBlueprint(j);
            }
            else
            {
                m_CurrentAsset.type = "Generic";
                ParseGenericAsset(j);
            }

            m_CurrentAsset.isValid = true;
        }
        catch (const std::exception& e)
        {
            m_CurrentAsset.isValid = false;
            m_CurrentAsset.errorMessage = std::string("JSON Parse Error: ") + e.what();
            std::cerr << m_CurrentAsset.errorMessage << std::endl;
        }
    }

    void AssetInfoPanel::ParseEntityBlueprint(const json& j)
    {
        m_CurrentAsset.name = JsonHelper::GetString(j, "name", "Unnamed Entity");
        m_CurrentAsset.description = JsonHelper::GetString(j, "description", "");

        if (j.contains("components") && j["components"].is_array())
        {
            m_CurrentAsset.componentCount = (int)j["components"].size();
            
            // Extract component types
            for (const auto& comp : j["components"])
            {
                if (comp.contains("type"))
                {
                    std::string compType = comp["type"].get<std::string>();
                    m_CurrentAsset.components.push_back(compType);
                }
            }
        }
    }

    void AssetInfoPanel::ParseBehaviorTree(const json& j)
    {
        m_CurrentAsset.name = JsonHelper::GetString(j, "name", "Unnamed Behavior Tree");
        m_CurrentAsset.description = "Behavior Tree AI Definition";

        if (j.contains("nodes") && j["nodes"].is_array())
        {
            m_CurrentAsset.nodeCount = (int)j["nodes"].size();
            
            // Extract node types
            for (const auto& node : j["nodes"])
            {
                if (node.contains("type"))
                {
                    std::string nodeType = node["type"].get<std::string>();
                    if (node.contains("name"))
                    {
                        std::string nodeName = node["name"].get<std::string>();
                        m_CurrentAsset.nodes.push_back(nodeName + " (" + nodeType + ")");
                    }
                    else
                    {
                        m_CurrentAsset.nodes.push_back(nodeType);
                    }
                }
            }
        }

        if (j.contains("rootNodeId"))
        {
            int rootId = j["rootNodeId"].get<int>();
            m_CurrentAsset.description += " - Root Node ID: " + std::to_string(rootId);
        }
    }

    void AssetInfoPanel::ParseGenericAsset(const json& j)
    {
        m_CurrentAsset.name = JsonHelper::GetString(j, "name", "Unknown Asset");
        m_CurrentAsset.description = JsonHelper::GetString(j, "description", "Generic JSON Asset");
    }

    void AssetInfoPanel::Clear()
    {
        m_CurrentAsset = AssetInfo();
    }

    void AssetInfoPanel::Render()
    {
        if (ImGui::Begin("Asset Info"))
        {
            if (!HasAsset())
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No asset selected");
                ImGui::Text("Select an asset from the Asset Browser to view details.");
            }
            else if (!m_CurrentAsset.isValid)
            {
                RenderErrorInfo();
            }
            else
            {
                // Render based on asset type
                if (m_CurrentAsset.type == "EntityBlueprint")
                {
                    RenderEntityBlueprintInfo();
                }
                else if (m_CurrentAsset.type == "BehaviorTree")
                {
                    RenderBehaviorTreeInfo();
                }
                else
                {
                    RenderGenericInfo();
                }
            }
        }
        ImGui::End();
    }

    void AssetInfoPanel::RenderEntityBlueprintInfo()
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Entity Blueprint");
        ImGui::Separator();

        ImGui::Text("Name: %s", m_CurrentAsset.name.c_str());
        
        if (!m_CurrentAsset.description.empty())
        {
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", m_CurrentAsset.description.c_str());
        }

        ImGui::Spacing();
        ImGui::Text("Components: %d", m_CurrentAsset.componentCount);

        if (!m_CurrentAsset.components.empty())
        {
            if (ImGui::CollapsingHeader("Component List"))
            {
                for (const auto& comp : m_CurrentAsset.components)
                {
                    ImGui::BulletText("%s", comp.c_str());
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", m_CurrentAsset.filepath.c_str());
    }

    void AssetInfoPanel::RenderBehaviorTreeInfo()
    {
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.9f, 1.0f), "Behavior Tree");
        ImGui::Separator();

        ImGui::Text("Name: %s", m_CurrentAsset.name.c_str());
        
        if (!m_CurrentAsset.description.empty())
        {
            ImGui::TextWrapped("%s", m_CurrentAsset.description.c_str());
        }

        ImGui::Spacing();
        ImGui::Text("Nodes: %d", m_CurrentAsset.nodeCount);

        if (!m_CurrentAsset.nodes.empty())
        {
            if (ImGui::CollapsingHeader("Node List"))
            {
                for (const auto& node : m_CurrentAsset.nodes)
                {
                    ImGui::BulletText("%s", node.c_str());
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", m_CurrentAsset.filepath.c_str());
    }

    void AssetInfoPanel::RenderGenericInfo()
    {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "%s", m_CurrentAsset.type.c_str());
        ImGui::Separator();

        ImGui::Text("Name: %s", m_CurrentAsset.name.c_str());
        
        if (!m_CurrentAsset.description.empty())
        {
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", m_CurrentAsset.description.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", m_CurrentAsset.filepath.c_str());
    }

    void AssetInfoPanel::RenderErrorInfo()
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error Loading Asset");
        ImGui::Separator();

        ImGui::Text("File: %s", m_CurrentAsset.filepath.c_str());
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Error:");
        ImGui::TextWrapped("%s", m_CurrentAsset.errorMessage.c_str());
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), 
            "The JSON file may be corrupted or malformed. Please check the file syntax.");
    }

    bool AssetInfoPanel::HasAsset() const
    {
        return !m_CurrentAsset.filepath.empty();
    }
}
