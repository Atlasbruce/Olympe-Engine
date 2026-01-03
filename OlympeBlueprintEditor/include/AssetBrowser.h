/*
 * Olympe Blueprint Editor - Asset Browser
 * 
 * Multi-folder asset explorer for navigating, searching, and filtering
 * all types of assets in the Blueprints/ directory.
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace Olympe {

/**
 * Represents a single asset or directory in the file system
 */
struct AssetItem {
    std::string path;           // Full path to the asset
    std::string name;           // Display name (filename without path)
    std::string type;           // Asset type: "Entity", "BT", "HFSM", "Prefab", "Directory", etc.
    bool isDirectory;           // True if this is a directory
    
    AssetItem() : isDirectory(false) {}
    AssetItem(const std::string& p, const std::string& n, const std::string& t, bool isDir = false)
        : path(p), name(n), type(t), isDirectory(isDir) {}
};

/**
 * Asset Browser - Tree view explorer for all project assets
 * 
 * Features:
 * - Recursive directory scanning
 * - Search by filename
 * - Filter by asset type
 * - Double-click to load assets
 * - Automatic type detection
 */
class AssetBrowser {
public:
    AssetBrowser();
    ~AssetBrowser();

    /**
     * Initialize the browser with the root path to scan
     * @param rootPath Path to the Blueprints/ directory
     */
    void Init(const std::string& rootPath);

    /**
     * Render the asset browser panel
     */
    void Render();

    /**
     * Refresh the asset list (rescan file system)
     */
    void Refresh();

    /**
     * Get the currently selected asset
     * @return Pointer to selected asset, or nullptr if none selected
     */
    AssetItem* GetSelectedAsset();
    
    /**
     * Check if an asset was double-clicked (to trigger loading)
     * @return True if an asset was double-clicked this frame
     */
    bool WasAssetDoubleClicked() const { return m_AssetDoubleClicked; }

private:
    /**
     * Recursively scan a directory and populate the asset list
     * @param path Directory to scan
     */
    void ScanDirectory(const std::filesystem::path& path);

    /**
     * Render a tree node for a directory or file
     * @param path Path to render
     */
    void RenderTreeNode(const std::filesystem::path& path);

    /**
     * Filter assets based on search and type filters
     */
    void FilterAssets();

    /**
     * Determine asset type from file path
     * @param filepath Path to the asset file
     * @return Asset type string
     */
    std::string DetermineAssetType(const std::filesystem::path& filepath);

    /**
     * Check if an asset matches current filters
     * @param asset Asset to check
     * @return True if asset should be displayed
     */
    bool PassesFilters(const AssetItem& asset);

    // Root directory for scanning
    std::string m_RootPath;

    // All assets found in file system
    std::vector<AssetItem> m_AllAssets;

    // Filtered assets (based on search/type filters)
    std::vector<AssetItem> m_FilteredAssets;

    // Currently selected asset (index into m_FilteredAssets, -1 if none)
    int m_SelectedIndex;

    // Selected asset pointer for external access
    AssetItem* m_SelectedAsset;

    // UI state
    char m_SearchBuffer[256];
    bool m_AssetDoubleClicked;

    // Type filters
    bool m_ShowEntities;
    bool m_ShowBT;
    bool m_ShowHFSM;
    bool m_ShowPrefabs;
    bool m_ShowTriggers;
    bool m_ShowCollectibles;
    bool m_ShowFX;
    bool m_ShowSound;
    bool m_ShowMusic;
    bool m_ShowNavmesh;
    bool m_ShowWaypoints;
    bool m_ShowCollisionMap;
    bool m_ShowOther;
};

} // namespace Olympe
