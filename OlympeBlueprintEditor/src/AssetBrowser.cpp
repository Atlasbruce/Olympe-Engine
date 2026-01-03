/*
 * Olympe Blueprint Editor - Asset Browser Implementation
 */

#include "../include/AssetBrowser.h"
#include "../third_party/imgui/imgui.h"
#include "../../Source/json_helper.h"
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace Olympe {

AssetBrowser::AssetBrowser()
    : m_SelectedIndex(-1)
    , m_SelectedAsset(nullptr)
    , m_AssetDoubleClicked(false)
    , m_ShowEntities(true)
    , m_ShowBT(true)
    , m_ShowHFSM(true)
    , m_ShowPrefabs(true)
    , m_ShowTriggers(true)
    , m_ShowCollectibles(true)
    , m_ShowFX(true)
    , m_ShowSound(true)
    , m_ShowMusic(true)
    , m_ShowNavmesh(true)
    , m_ShowWaypoints(true)
    , m_ShowCollisionMap(true)
    , m_ShowOther(true)
{
    m_SearchBuffer[0] = '\0';
}

AssetBrowser::~AssetBrowser()
{
}

void AssetBrowser::Init(const std::string& rootPath)
{
    m_RootPath = rootPath;
    Refresh();
}

void AssetBrowser::Refresh()
{
    m_AllAssets.clear();
    m_FilteredAssets.clear();
    m_SelectedIndex = -1;
    m_SelectedAsset = nullptr;

    if (!fs::exists(m_RootPath))
    {
        std::cerr << "AssetBrowser: Root path does not exist: " << m_RootPath << std::endl;
        return;
    }

    // Scan the root directory recursively
    ScanDirectory(m_RootPath);

    // Apply filters
    FilterAssets();
}

void AssetBrowser::ScanDirectory(const fs::path& path)
{
    try
    {
        if (!fs::exists(path) || !fs::is_directory(path))
            return;

        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                // Add directory to assets list
                std::string dirName = entry.path().filename().string();
                AssetItem dirItem(
                    entry.path().string(),
                    dirName,
                    "Directory",
                    true
                );
                m_AllAssets.push_back(dirItem);

                // Recursively scan subdirectories
                ScanDirectory(entry.path());
            }
            else if (entry.is_regular_file())
            {
                // Only process JSON files
                if (entry.path().extension() == ".json")
                {
                    std::string filename = entry.path().filename().string();
                    std::string assetType = DetermineAssetType(entry.path());
                    
                    AssetItem fileItem(
                        entry.path().string(),
                        filename,
                        assetType,
                        false
                    );
                    m_AllAssets.push_back(fileItem);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "AssetBrowser: Error scanning directory " << path << ": " << e.what() << std::endl;
    }
}

std::string AssetBrowser::DetermineAssetType(const fs::path& filepath)
{
    // Strategy 1: Determine by parent directory name
    fs::path parentPath = filepath.parent_path();
    std::string parentName = parentPath.filename().string();

    if (parentName == "Entities" || parentPath.string().find("/Entities") != std::string::npos)
        return "Entity";
    if (parentName == "AI" || parentPath.string().find("/AI") != std::string::npos)
        return "BehaviorTree";
    if (parentName == "HFSM" || parentPath.string().find("/HFSM") != std::string::npos)
        return "HFSM";
    if (parentName == "Prefabs" || parentPath.string().find("/Prefabs") != std::string::npos)
        return "Prefab";
    if (parentName == "Triggers" || parentPath.string().find("/Triggers") != std::string::npos)
        return "Trigger";
    if (parentName == "Collectibles" || parentPath.string().find("/Collectibles") != std::string::npos)
        return "Collectible";
    if (parentName == "FX" || parentPath.string().find("/FX") != std::string::npos)
        return "FX";
    if (parentName == "Sound" || parentPath.string().find("/Sound") != std::string::npos)
        return "Sound";
    if (parentName == "Music" || parentPath.string().find("/Music") != std::string::npos)
        return "Music";
    if (parentName == "Navmesh" || parentPath.string().find("/Navmesh") != std::string::npos)
        return "Navmesh";
    if (parentName == "Waypoints" || parentPath.string().find("/Waypoints") != std::string::npos)
        return "Waypoint";
    if (parentName == "CollisionMap" || parentPath.string().find("/CollisionMap") != std::string::npos)
        return "CollisionMap";

    // Strategy 2: Try to read JSON and check "type" field
    json assetJson;
    if (JsonHelper::LoadJsonFromFile(filepath.string(), assetJson))
    {
        if (assetJson.contains("type"))
        {
            std::string type = JsonHelper::GetString(assetJson, "type", "");
            if (!type.empty())
                return type;
        }

        // Check for behavior tree specific fields
        if (assetJson.contains("rootNodeId") && assetJson.contains("nodes"))
            return "BehaviorTree";

        // Check for entity blueprint fields
        if (assetJson.contains("components"))
            return "Entity";
    }

    // Default to "Unknown"
    return "Other";
}

bool AssetBrowser::PassesFilters(const AssetItem& asset)
{
    // Always show directories
    if (asset.isDirectory)
        return true;

    // Check type filters
    if (asset.type == "Entity" && !m_ShowEntities) return false;
    if (asset.type == "BehaviorTree" && !m_ShowBT) return false;
    if (asset.type == "HFSM" && !m_ShowHFSM) return false;
    if (asset.type == "Prefab" && !m_ShowPrefabs) return false;
    if (asset.type == "Trigger" && !m_ShowTriggers) return false;
    if (asset.type == "Collectible" && !m_ShowCollectibles) return false;
    if (asset.type == "FX" && !m_ShowFX) return false;
    if (asset.type == "Sound" && !m_ShowSound) return false;
    if (asset.type == "Music" && !m_ShowMusic) return false;
    if (asset.type == "Navmesh" && !m_ShowNavmesh) return false;
    if (asset.type == "Waypoint" && !m_ShowWaypoints) return false;
    if (asset.type == "CollisionMap" && !m_ShowCollisionMap) return false;
    if (asset.type == "Other" && !m_ShowOther) return false;

    // Check search filter (case-insensitive substring match)
    if (m_SearchBuffer[0] != '\0')
    {
        std::string searchStr = m_SearchBuffer;
        std::string assetName = asset.name;
        
        // Convert both to lowercase for case-insensitive search
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
        std::transform(assetName.begin(), assetName.end(), assetName.begin(), ::tolower);
        
        if (assetName.find(searchStr) == std::string::npos)
            return false;
    }

    return true;
}

void AssetBrowser::FilterAssets()
{
    m_FilteredAssets.clear();

    for (const auto& asset : m_AllAssets)
    {
        if (PassesFilters(asset))
        {
            m_FilteredAssets.push_back(asset);
        }
    }
}

void AssetBrowser::Render()
{
    m_AssetDoubleClicked = false;

    ImGui::Begin("Asset Browser");

    // Toolbar at the top
    if (ImGui::Button("Refresh"))
    {
        Refresh();
    }

    ImGui::SameLine();
    ImGui::Text("(%zu assets)", m_FilteredAssets.size());

    // Search bar
    ImGui::Separator();
    ImGui::Text("Search:");
    ImGui::SameLine();
    if (ImGui::InputText("##search", m_SearchBuffer, sizeof(m_SearchBuffer)))
    {
        FilterAssets();
    }

    // Type filters
    ImGui::Separator();
    ImGui::Text("Filter by Type:");
    
    bool filterChanged = false;
    
    if (ImGui::Checkbox("Entities", &m_ShowEntities)) filterChanged = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("BT", &m_ShowBT)) filterChanged = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("HFSM", &m_ShowHFSM)) filterChanged = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Prefabs", &m_ShowPrefabs)) filterChanged = true;

    if (ImGui::Checkbox("Triggers", &m_ShowTriggers)) filterChanged = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("FX", &m_ShowFX)) filterChanged = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Sound", &m_ShowSound)) filterChanged = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Music", &m_ShowMusic)) filterChanged = true;

    if (ImGui::Checkbox("Other", &m_ShowOther)) filterChanged = true;

    if (filterChanged)
    {
        FilterAssets();
    }

    ImGui::Separator();

    // Asset list
    ImGui::BeginChild("AssetList", ImVec2(0, 0), true);

    for (size_t i = 0; i < m_FilteredAssets.size(); ++i)
    {
        const auto& asset = m_FilteredAssets[i];
        bool isSelected = (m_SelectedIndex == (int)i);

        // Icon prefix based on type
        std::string icon = "📄";  // Default file icon
        if (asset.isDirectory)
            icon = "📁";
        else if (asset.type == "Entity")
            icon = "🎭";
        else if (asset.type == "BehaviorTree")
            icon = "🌳";
        else if (asset.type == "HFSM")
            icon = "⚙️";
        else if (asset.type == "Prefab")
            icon = "📦";
        else if (asset.type == "Sound" || asset.type == "Music")
            icon = "🔊";
        else if (asset.type == "FX")
            icon = "✨";

        std::string label = icon + " " + asset.name;

        if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
        {
            m_SelectedIndex = (int)i;
            m_SelectedAsset = const_cast<AssetItem*>(&m_FilteredAssets[i]);

            // Check for double-click
            if (ImGui::IsMouseDoubleClicked(0) && !asset.isDirectory)
            {
                m_AssetDoubleClicked = true;
            }
        }

        // Tooltip with full path
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Type: %s", asset.type.c_str());
            ImGui::Text("Path: %s", asset.path.c_str());
            ImGui::EndTooltip();
        }
    }

    ImGui::EndChild();

    ImGui::End();
}

AssetItem* AssetBrowser::GetSelectedAsset()
{
    return m_SelectedAsset;
}

} // namespace Olympe
