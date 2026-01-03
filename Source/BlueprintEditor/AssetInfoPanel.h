/*
 * Olympe Blueprint Editor - Asset Info Panel
 * Displays metadata and information about selected assets
 */

#pragma once

#include <string>
#include <vector>
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe
{
    using json = nlohmann::json;

    // Asset metadata structure
    struct AssetInfo
    {
        std::string filepath;
        std::string name;
        std::string type;
        std::string description;
        int componentCount;     // For EntityBlueprint
        int nodeCount;          // For BehaviorTree/HFSM
        std::vector<std::string> components;  // Component types for Entity
        std::vector<std::string> nodes;       // Node types for BT
        bool isValid;           // False if JSON is malformed
        std::string errorMessage;
        
        AssetInfo() : componentCount(0), nodeCount(0), isValid(false) {}
    };

    class AssetInfoPanel
    {
    public:
        AssetInfoPanel();
        ~AssetInfoPanel();

        // Load asset information from file
        void LoadAsset(const std::string& filepath);
        
        // Clear current asset info
        void Clear();
        
        // Render the info panel with ImGui
        void Render();
        
        // Check if panel has valid asset loaded
        bool HasAsset() const;
        
        // Get current asset info
        const AssetInfo& GetAssetInfo() const { return m_CurrentAsset; }

    private:
        // Parse different asset types
        void ParseEntityBlueprint(const json& j);
        void ParseBehaviorTree(const json& j);
        void ParseGenericAsset(const json& j);
        
        // Render different asset type details
        void RenderEntityBlueprintInfo();
        void RenderBehaviorTreeInfo();
        void RenderGenericInfo();
        void RenderErrorInfo();

    private:
        AssetInfo m_CurrentAsset;
    };
}
