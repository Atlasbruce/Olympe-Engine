/*
 * Olympe Blueprint Editor - Asset Info Panel Implementation
 * Frontend component that uses BlueprintEditor backend for asset metadata
 */

#include "AssetInfoPanel.h"
#include "BlueprintEditor.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>

namespace Olympe
{
    AssetInfoPanel::AssetInfoPanel()
        : m_CurrentAsset(nullptr)
    {
    }

    AssetInfoPanel::~AssetInfoPanel()
    {
        // Don't delete m_CurrentAsset as it's managed by backend cache
    }

    void AssetInfoPanel::LoadAsset(const std::string& filepath)
    {
        Clear();
        
        m_LoadedFilepath = filepath;
        
        // Get asset metadata from backend
        static AssetMetadata cachedMetadata;  // Static to keep data alive
        cachedMetadata = BlueprintEditor::Get().GetAssetMetadata(filepath);
        m_CurrentAsset = &cachedMetadata;
        
        std::cout << "AssetInfoPanel: Loaded asset info from backend: " << filepath << std::endl;
    }

    void AssetInfoPanel::Clear()
    {
        m_CurrentAsset = nullptr;
        m_LoadedFilepath.clear();
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
            else if (!m_CurrentAsset->isValid)
            {
                RenderErrorInfo();
            }
            else
            {
                // Render based on asset type
                if (m_CurrentAsset->type == "EntityBlueprint")
                {
                    RenderEntityBlueprintInfo();
                }
                else if (m_CurrentAsset->type == "BehaviorTree")
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

        ImGui::Text("Name: %s", m_CurrentAsset->name.c_str());
        
        if (!m_CurrentAsset->description.empty())
        {
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", m_CurrentAsset->description.c_str());
        }

        ImGui::Spacing();
        ImGui::Text("Components: %d", m_CurrentAsset->componentCount);

        if (!m_CurrentAsset->components.empty())
        {
            if (ImGui::CollapsingHeader("Component List"))
            {
                for (const auto& comp : m_CurrentAsset->components)
                {
                    ImGui::BulletText("%s", comp.c_str());
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", m_CurrentAsset->filepath.c_str());
    }

    void AssetInfoPanel::RenderBehaviorTreeInfo()
    {
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.9f, 1.0f), "Behavior Tree");
        ImGui::Separator();

        ImGui::Text("Name: %s", m_CurrentAsset->name.c_str());
        
        if (!m_CurrentAsset->description.empty())
        {
            ImGui::TextWrapped("%s", m_CurrentAsset->description.c_str());
        }

        ImGui::Spacing();
        ImGui::Text("Nodes: %d", m_CurrentAsset->nodeCount);

        if (!m_CurrentAsset->nodes.empty())
        {
            if (ImGui::CollapsingHeader("Node List"))
            {
                for (const auto& node : m_CurrentAsset->nodes)
                {
                    ImGui::BulletText("%s", node.c_str());
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", m_CurrentAsset->filepath.c_str());
    }

    void AssetInfoPanel::RenderGenericInfo()
    {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "%s", m_CurrentAsset->type.c_str());
        ImGui::Separator();

        ImGui::Text("Name: %s", m_CurrentAsset->name.c_str());
        
        if (!m_CurrentAsset->description.empty())
        {
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", m_CurrentAsset->description.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", m_CurrentAsset->filepath.c_str());
    }

    void AssetInfoPanel::RenderErrorInfo()
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error Loading Asset");
        ImGui::Separator();

        ImGui::Text("File: %s", m_CurrentAsset->filepath.c_str());
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Error:");
        ImGui::TextWrapped("%s", m_CurrentAsset->errorMessage.c_str());
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), 
            "The JSON file may be corrupted or malformed. Please check the file syntax.");
    }

    bool AssetInfoPanel::HasAsset() const
    {
        return m_CurrentAsset != nullptr;
    }
}
