/*
 * Olympe Blueprint Editor - Asset Browser
 * Frontend component that uses BlueprintEditor backend for asset data
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Olympe
{
    // Forward declarations
    class BlueprintEditor;
    struct AssetNode;

    // Asset filter options
    struct AssetFilter
    {
        std::string searchQuery;    // Text search in filename
        std::string typeFilter;     // Filter by asset type (empty = all)
        
        AssetFilter() = default;
    };

    class AssetBrowser
    {
    public:
        AssetBrowser();
        ~AssetBrowser();

        // Initialize the asset browser (sets up UI state)
        void Initialize(const std::string& assetsRootPath);
        
        // Refresh asset tree from backend
        void Refresh();
        
        // Render the asset browser panel with ImGui
        void Render();
        
        // Get currently selected asset path (empty if none selected)
        std::string GetSelectedAssetPath() const;
        
        // Check if an asset is selected
        bool HasSelection() const;
        
        // Set callback for when an asset is double-clicked/opened
        void SetAssetOpenCallback(std::function<void(const std::string&)> callback);

    private:
        // Render tree node recursively (uses backend data)
        void RenderTreeNode(const std::shared_ptr<AssetNode>& node);
        
        // Render runtime entities section (B)
        void RenderRuntimeEntities();
        
        // Apply filters to determine if node should be visible
        bool PassesFilter(const std::shared_ptr<AssetNode>& node) const;
        
        // Render search and filter UI
        void RenderFilterUI();

    private:
        std::string m_SelectedAssetPath;
        AssetFilter m_Filter;
        std::function<void(const std::string&)> m_OnAssetOpen;
        
        // UI state
        char m_SearchBuffer[256];
        int m_TypeFilterSelection;
        std::vector<std::string> m_AvailableTypes;
    };
}
