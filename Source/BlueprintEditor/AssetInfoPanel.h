/*
 * Olympe Blueprint Editor - Asset Info Panel
 * Frontend component that uses BlueprintEditor backend for asset metadata
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe
{
    // Forward declarations
    class BlueprintEditor;
    struct AssetMetadata;

    class AssetInfoPanel
    {
    public:
        AssetInfoPanel();
        ~AssetInfoPanel();

        // Load asset information from file (delegates to backend)
        void LoadAsset(const std::string& filepath);
        
        // Clear current asset info
        void Clear();
        
        // Render the info panel with ImGui
        void Render();
        
        // Check if panel has valid asset loaded
        bool HasAsset() const;
        
        // Get current asset info (from cached backend data)
        const AssetMetadata& GetAssetInfo() const { return *m_CurrentAsset; }

    private:
        // Render different asset type details
        void RenderEntityBlueprintInfo();
        void RenderBehaviorTreeInfo();
        void RenderGenericInfo();
        void RenderErrorInfo();

    private:
        AssetMetadata* m_CurrentAsset;  // Points to cached backend data
        std::string m_LoadedFilepath;    // Track which file is loaded
    };
}
