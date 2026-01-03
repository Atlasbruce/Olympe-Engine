/*
 * Olympe Blueprint Editor - Asset Info Panel
 * 
 * Displays detailed information about the currently selected asset
 */

#pragma once

#include <string>
#include "../../Source/third_party/nlohmann/json.hpp"

using json = nlohmann::json;

namespace Olympe {

/**
 * Asset Info Panel - Displays detailed information about selected assets
 * 
 * Shows:
 * - Asset type, name, description
 * - Component/node counts
 * - Linked resources (sprites, sounds, etc.)
 * - Metadata (version, author, dates)
 */
class AssetInfoPanel {
public:
    AssetInfoPanel();
    ~AssetInfoPanel();

    /**
     * Render the info panel for a specific asset
     * @param assetPath Full path to the asset file
     * @param assetType Type of asset (Entity, BehaviorTree, etc.)
     */
    void Render(const std::string& assetPath, const std::string& assetType);

    /**
     * Clear the panel (no asset selected)
     */
    void Clear();

private:
    /**
     * Display information for Entity blueprints
     */
    void DisplayEntityInfo(const json& j);

    /**
     * Display information for Behavior Trees
     */
    void DisplayBTInfo(const json& j);

    /**
     * Display information for HFSM (Hierarchical Finite State Machines)
     */
    void DisplayHFSMInfo(const json& j);

    /**
     * Display information for Prefabs
     */
    void DisplayPrefabInfo(const json& j);

    /**
     * Display generic information for unknown asset types
     */
    void DisplayGenericInfo(const json& j);

    /**
     * Extract and display linked resources (sprites, sounds, etc.)
     */
    void DisplayLinkedResources(const json& j);

    /**
     * Display metadata (version, author, dates, etc.)
     */
    void DisplayMetadata(const json& j);

    // Cache the last loaded asset to avoid reloading on every frame
    std::string m_LastAssetPath;
    json m_CachedJson;
    bool m_HasValidData;
};

} // namespace Olympe
