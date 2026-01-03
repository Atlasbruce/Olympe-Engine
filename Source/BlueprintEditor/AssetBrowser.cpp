/*
 * Olympe Blueprint Editor - Asset Browser Implementation
 */

#include "AssetBrowser.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/nlohmann/json.hpp"
#include "../json_helper.h"
#include <algorithm>
#include <iostream>
#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using json = nlohmann::json;

namespace Olympe
{
    AssetBrowser::AssetBrowser()
        : m_TypeFilterSelection(0)
    {
        m_SearchBuffer[0] = '\0';
        m_AvailableTypes = {"All", "EntityBlueprint", "BehaviorTree", "Prefab", "Trigger", "FX", "Sound"};
    }

    AssetBrowser::~AssetBrowser()
    {
    }

    void AssetBrowser::Initialize(const std::string& assetsRootPath)
    {
        m_RootPath = assetsRootPath;
        Refresh();
    }

    void AssetBrowser::Refresh()
    {
        if (m_RootPath.empty())
            return;

        std::cout << "Scanning assets directory: " << m_RootPath << std::endl;
        
        try
        {
            if (fs::exists(m_RootPath) && fs::is_directory(m_RootPath))
            {
                m_RootNode = ScanDirectory(m_RootPath);
                std::cout << "Asset scan complete" << std::endl;
            }
            else
            {
                std::cerr << "Asset directory not found: " << m_RootPath << std::endl;
                m_RootNode = nullptr;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error scanning assets: " << e.what() << std::endl;
            m_RootNode = nullptr;
        }
    }

    std::shared_ptr<AssetTreeNode> AssetBrowser::ScanDirectory(const std::string& path)
    {
        auto node = std::make_shared<AssetTreeNode>(
            fs::path(path).filename().string(),
            path,
            true
        );

        try
        {
            for (const auto& entry : fs::directory_iterator(path))
            {
                std::string entryPath = entry.path().string();
                std::string filename = entry.path().filename().string();
                
                // Skip hidden files and directories
                if (filename[0] == '.')
                    continue;

                if (fs::is_directory(entry.path()))
                {
                    // Recursively scan subdirectories
                    auto childNode = ScanDirectory(entryPath);
                    node->children.push_back(childNode);
                }
                else if (fs::is_regular_file(entry.path()))
                {
                    // Check if it's a JSON file
                    if (entry.path().extension() == ".json")
                    {
                        auto fileNode = std::make_shared<AssetTreeNode>(
                            filename,
                            entryPath,
                            false
                        );
                        
                        // Detect asset type
                        fileNode->type = DetectAssetType(entryPath);
                        
                        node->children.push_back(fileNode);
                    }
                }
            }

            // Sort children: directories first, then files alphabetically
            std::sort(node->children.begin(), node->children.end(),
                [](const std::shared_ptr<AssetTreeNode>& a, const std::shared_ptr<AssetTreeNode>& b)
                {
                    if (a->isDirectory != b->isDirectory)
                        return a->isDirectory > b->isDirectory;
                    return a->name < b->name;
                });
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error scanning directory " << path << ": " << e.what() << std::endl;
        }

        return node;
    }

    std::string AssetBrowser::DetectAssetType(const std::string& filepath)
    {
        try
        {
            json j;
            if (!JsonHelper::LoadJsonFromFile(filepath, j))
                return "Unknown";

            // Check for common type indicators
            if (j.contains("type"))
            {
                std::string type = j["type"].get<std::string>();
                if (type == "EntityBlueprint")
                    return "EntityBlueprint";
            }

            // Check for behavior tree structure
            if (j.contains("rootNodeId") && j.contains("nodes"))
                return "BehaviorTree";

            // Check for components (entity blueprint without explicit type)
            if (j.contains("components"))
                return "EntityBlueprint";

            return "Generic";
        }
        catch (const std::exception&)
        {
            return "Unknown";
        }
    }

    bool AssetBrowser::PassesFilter(const std::shared_ptr<AssetTreeNode>& node) const
    {
        // Directories always pass
        if (node->isDirectory)
            return true;

        // Apply type filter
        if (m_TypeFilterSelection > 0)
        {
            std::string selectedType = m_AvailableTypes[m_TypeFilterSelection];
            if (node->type != selectedType)
                return false;
        }

        // Apply search filter
        if (m_Filter.searchQuery.empty())
            return true;

        // Case-insensitive search in filename
        std::string lowerName = node->name;
        std::string lowerQuery = m_Filter.searchQuery;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

        return lowerName.find(lowerQuery) != std::string::npos;
    }

    void AssetBrowser::RenderFilterUI()
    {
        // Search box
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::InputText("##search", m_SearchBuffer, sizeof(m_SearchBuffer)))
        {
            m_Filter.searchQuery = m_SearchBuffer;
        }
        
        ImGui::SameLine();
        ImGui::Text("Search");

        // Type filter combo
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::BeginCombo("##typefilter", m_AvailableTypes[m_TypeFilterSelection].c_str()))
        {
            for (int i = 0; i < (int)m_AvailableTypes.size(); i++)
            {
                bool is_selected = (m_TypeFilterSelection == i);
                if (ImGui::Selectable(m_AvailableTypes[i].c_str(), is_selected))
                    m_TypeFilterSelection = i;
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::Text("Type Filter");

        // Refresh button
        ImGui::SameLine(0.0f, 20.0f);
        if (ImGui::Button("Refresh"))
        {
            Refresh();
        }

        ImGui::Separator();
    }

    void AssetBrowser::RenderTreeNode(const std::shared_ptr<AssetTreeNode>& node)
    {
        if (!node)
            return;

        // Skip if doesn't pass filter
        if (!PassesFilter(node))
        {
            // But still check children for directories
            if (node->isDirectory)
            {
                for (const auto& child : node->children)
                    RenderTreeNode(child);
            }
            return;
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        
        if (node->fullPath == m_SelectedAssetPath)
            flags |= ImGuiTreeNodeFlags_Selected;

        if (!node->isDirectory)
            flags |= ImGuiTreeNodeFlags_Leaf;

        std::string label = node->name;
        if (!node->isDirectory && !node->type.empty())
            label += " [" + node->type + "]";

        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)node.get(), flags, "%s", label.c_str());

        // Handle selection
        if (ImGui::IsItemClicked())
        {
            if (!node->isDirectory)
            {
                m_SelectedAssetPath = node->fullPath;
                std::cout << "Selected asset: " << m_SelectedAssetPath << std::endl;
            }
        }

        // Handle double-click to open
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (!node->isDirectory && m_OnAssetOpen)
            {
                std::cout << "Opening asset: " << node->fullPath << std::endl;
                m_OnAssetOpen(node->fullPath);
            }
        }

        if (node_open)
        {
            if (node->isDirectory)
            {
                for (const auto& child : node->children)
                    RenderTreeNode(child);
            }
            ImGui::TreePop();
        }
    }

    void AssetBrowser::Render()
    {
        if (ImGui::Begin("Asset Browser"))
        {
            RenderFilterUI();

            if (m_RootNode)
            {
                // Render the tree starting from children (skip root "Blueprints" node)
                for (const auto& child : m_RootNode->children)
                    RenderTreeNode(child);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No assets found. Click Refresh.");
            }
        }
        ImGui::End();
    }

    std::string AssetBrowser::GetSelectedAssetPath() const
    {
        return m_SelectedAssetPath;
    }

    bool AssetBrowser::HasSelection() const
    {
        return !m_SelectedAssetPath.empty();
    }

    void AssetBrowser::SetAssetOpenCallback(std::function<void(const std::string&)> callback)
    {
        m_OnAssetOpen = callback;
    }
}
