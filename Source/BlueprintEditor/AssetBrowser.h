/*
 * Olympe Blueprint Editor - Asset Browser
 * SDL3-compatible asset explorer for navigating Blueprint directories
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Olympe
{
    // Represents a file or folder in the asset tree
    struct AssetTreeNode
    {
        std::string name;           // Display name (filename without path)
        std::string fullPath;       // Complete file path
        std::string type;           // Asset type (EntityBlueprint, BehaviorTree, etc.)
        bool isDirectory;
        std::vector<std::shared_ptr<AssetTreeNode>> children;
        
        AssetTreeNode(const std::string& n, const std::string& path, bool isDir)
            : name(n), fullPath(path), isDirectory(isDir) {}
    };

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

        // Initialize and scan the assets directory
        void Initialize(const std::string& assetsRootPath);
        
        // Refresh asset tree (rescan directories)
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
        // Recursively scan directory and build tree
        std::shared_ptr<AssetTreeNode> ScanDirectory(const std::string& path);
        
        // Render tree node recursively
        void RenderTreeNode(const std::shared_ptr<AssetTreeNode>& node);
        
        // Apply filters to determine if node should be visible
        bool PassesFilter(const std::shared_ptr<AssetTreeNode>& node) const;
        
        // Detect asset type from JSON content
        std::string DetectAssetType(const std::string& filepath);
        
        // Render search and filter UI
        void RenderFilterUI();

    private:
        std::string m_RootPath;
        std::shared_ptr<AssetTreeNode> m_RootNode;
        std::string m_SelectedAssetPath;
        AssetFilter m_Filter;
        std::function<void(const std::string&)> m_OnAssetOpen;
        
        // UI state
        char m_SearchBuffer[256];
        int m_TypeFilterSelection;
        std::vector<std::string> m_AvailableTypes;
    };
}
