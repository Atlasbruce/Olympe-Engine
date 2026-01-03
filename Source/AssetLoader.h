/*
 * Olympe Engine V2 - 2025
 * Nicolas Chereau
 * nchereau@gmail.com
 * 
 * Asset Loader - Unified API for loading and saving all asset types
 * 
 * This header provides a centralized interface for loading and saving
 * all types of game assets including:
 * - Entity Blueprints
 * - Prefabs
 * - Behavior Trees (BT)
 * - Hierarchical Finite State Machines (HFSM)
 * - Templates
 * 
 * All asset operations use the JsonHelper for consistent error handling
 * and data validation.
 */

#pragma once

#include "json_helper.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Olympe {
namespace Assets {

using json = nlohmann::json;

// ============================================================================
// Asset Type Enumeration
// ============================================================================

enum class AssetType
{
    Unknown,
    EntityBlueprint,
    Prefab,
    BehaviorTree,
    HFSM,
    Template,
    Custom
};

// Convert asset type to string
inline std::string AssetTypeToString(AssetType type)
{
    switch (type)
    {
        case AssetType::EntityBlueprint: return "EntityBlueprint";
        case AssetType::Prefab: return "Prefab";
        case AssetType::BehaviorTree: return "BehaviorTree";
        case AssetType::HFSM: return "HFSM";
        case AssetType::Template: return "Template";
        case AssetType::Custom: return "Custom";
        default: return "Unknown";
    }
}

// Convert string to asset type
inline AssetType StringToAssetType(const std::string& str)
{
    if (str == "EntityBlueprint") return AssetType::EntityBlueprint;
    if (str == "Prefab") return AssetType::Prefab;
    if (str == "BehaviorTree") return AssetType::BehaviorTree;
    if (str == "HFSM") return AssetType::HFSM;
    if (str == "Template") return AssetType::Template;
    if (str == "Custom") return AssetType::Custom;
    return AssetType::Unknown;
}

// ============================================================================
// Asset Base Structure
// ============================================================================

/**
 * Base asset information structure
 * All assets should contain these fields in their JSON
 */
struct AssetInfo
{
    int schema_version = 1;         // Schema version for backward compatibility
    AssetType type = AssetType::Unknown;
    std::string name;               // Asset name/identifier
    std::string description;        // Optional description
    std::string filepath;           // File path where asset is stored
    
    AssetInfo() = default;
    AssetInfo(AssetType t, const std::string& n) : type(t), name(n) {}
    
    // Serialize to JSON
    json ToJson() const
    {
        json j;
        j["schema_version"] = schema_version;
        j["type"] = AssetTypeToString(type);
        j["name"] = name;
        j["description"] = description;
        return j;
    }
    
    // Deserialize from JSON
    static AssetInfo FromJson(const json& j)
    {
        AssetInfo info;
        info.schema_version = JsonHelper::GetInt(j, "schema_version", 1);
        info.type = StringToAssetType(JsonHelper::GetString(j, "type", ""));
        info.name = JsonHelper::GetString(j, "name", "");
        info.description = JsonHelper::GetString(j, "description", "");
        return info;
    }
};

// ============================================================================
// Unified Asset Loader
// ============================================================================

/**
 * Unified Asset Loader class
 * Provides a single interface for loading and saving all asset types
 */
class AssetLoader
{
public:
    /**
     * Load an asset from a JSON file
     * @param filepath Path to the asset file
     * @param outJson Output JSON object containing the asset data
     * @return true if successful, false otherwise
     */
    static bool LoadAsset(const std::string& filepath, json& outJson)
    {
        if (!JsonHelper::LoadJsonFromFile(filepath, outJson))
        {
            // JsonHelper already logs the error, just return false
            return false;
        }
        
        // Validate that it has required fields
        if (!outJson.contains("type"))
        {
            std::cerr << "AssetLoader: Asset file " << filepath << " missing 'type' field" << std::endl;
            return false;
        }
        
        return true;
    }
    
    /**
     * Load an asset and retrieve its info
     * @param filepath Path to the asset file
     * @param outJson Output JSON object containing the asset data
     * @param outInfo Output asset info structure
     * @return true if successful, false otherwise
     */
    static bool LoadAssetWithInfo(const std::string& filepath, json& outJson, AssetInfo& outInfo)
    {
        if (!LoadAsset(filepath, outJson))
            return false;
        
        outInfo = AssetInfo::FromJson(outJson);
        outInfo.filepath = filepath;
        return true;
    }
    
    /**
     * Save an asset to a JSON file
     * @param filepath Path where to save the asset
     * @param assetJson JSON object containing the asset data
     * @param indent Number of spaces for indentation (default: 4)
     * @return true if successful, false otherwise
     */
    static bool SaveAsset(const std::string& filepath, const json& assetJson, int indent = 4)
    {
        // Validate asset has required fields
        if (!assetJson.contains("type") || !assetJson.contains("name"))
        {
            std::cerr << "AssetLoader: Asset must contain 'type' and 'name' fields" << std::endl;
            return false;
        }
        
        // JsonHelper logs on failure
        return JsonHelper::SaveJsonToFile(filepath, assetJson, indent);
    }
    
    /**
     * Get asset type from a file without loading the entire asset
     * @param filepath Path to the asset file
     * @return AssetType or AssetType::Unknown if failed
     */
    static AssetType GetAssetType(const std::string& filepath)
    {
        json j;
        if (!JsonHelper::LoadJsonFromFile(filepath, j))
            return AssetType::Unknown;
        
        return StringToAssetType(JsonHelper::GetString(j, "type", ""));
    }
    
    /**
     * Validate an asset JSON structure
     * @param assetJson The JSON to validate
     * @param requiredFields Additional required fields beyond type and name
     * @return true if valid, false otherwise
     */
    static bool ValidateAsset(const json& assetJson, const std::vector<std::string>& requiredFields = {})
    {
        // Check base required fields
        std::vector<std::string> baseFields = {"type", "name"};
        if (!JsonHelper::ValidateKeys(assetJson, baseFields))
            return false;
        
        // Check additional required fields
        if (!requiredFields.empty())
        {
            if (!JsonHelper::ValidateKeys(assetJson, requiredFields))
                return false;
        }
        
        return true;
    }
    
    /**
     * Load multiple assets from a directory
     * @param directory Path to the directory containing assets
     * @param extension File extension to filter (e.g., ".json")
     * @param callback Function to call for each loaded asset
     * @return Number of assets successfully loaded
     */
    static int LoadAssetsFromDirectory(
        const std::string& directory,
        const std::string& extension,
        std::function<void(const std::string&, const json&, const AssetInfo&)> callback)
    {
        // Note: This is a placeholder - actual directory iteration would require
        // platform-specific code or std::filesystem (C++17)
        std::cerr << "AssetLoader::LoadAssetsFromDirectory: Not implemented yet" << std::endl;
        return 0;
    }
};

// ============================================================================
// Specialized Asset Loaders
// ============================================================================

/**
 * Entity Blueprint Loader
 * Specialized functions for loading entity blueprints
 */
class EntityBlueprintLoader
{
public:
    static bool LoadEntityBlueprint(const std::string& filepath, json& outJson)
    {
        if (!AssetLoader::LoadAsset(filepath, outJson))
            return false;
        
        // Validate it's an entity blueprint
        AssetType type = StringToAssetType(JsonHelper::GetString(outJson, "type", ""));
        if (type != AssetType::EntityBlueprint)
        {
            std::cerr << "EntityBlueprintLoader: File is not an EntityBlueprint" << std::endl;
            return false;
        }
        
        // Validate required fields
        std::vector<std::string> requiredFields = {"components"};
        if (!AssetLoader::ValidateAsset(outJson, requiredFields))
            return false;
        
        return true;
    }
    
    static bool SaveEntityBlueprint(const std::string& filepath, const json& blueprintJson)
    {
        return AssetLoader::SaveAsset(filepath, blueprintJson);
    }
};

/**
 * Prefab Loader
 * Specialized functions for loading prefabs
 */
class PrefabLoader
{
public:
    static bool LoadPrefab(const std::string& filepath, json& outJson)
    {
        if (!AssetLoader::LoadAsset(filepath, outJson))
            return false;
        
        AssetType type = StringToAssetType(JsonHelper::GetString(outJson, "type", ""));
        if (type != AssetType::Prefab)
        {
            std::cerr << "PrefabLoader: File is not a Prefab" << std::endl;
            return false;
        }
        
        return true;
    }
    
    static bool SavePrefab(const std::string& filepath, const json& prefabJson)
    {
        return AssetLoader::SaveAsset(filepath, prefabJson);
    }
};

/**
 * Behavior Tree Loader
 * Specialized functions for loading behavior trees
 */
class BehaviorTreeLoader
{
public:
    static bool LoadBehaviorTree(const std::string& filepath, json& outJson)
    {
        if (!AssetLoader::LoadAsset(filepath, outJson))
            return false;
        
        AssetType type = StringToAssetType(JsonHelper::GetString(outJson, "type", ""));
        if (type != AssetType::BehaviorTree)
        {
            std::cerr << "BehaviorTreeLoader: File is not a BehaviorTree" << std::endl;
            return false;
        }
        
        std::vector<std::string> requiredFields = {"nodes"};
        if (!AssetLoader::ValidateAsset(outJson, requiredFields))
            return false;
        
        return true;
    }
    
    static bool SaveBehaviorTree(const std::string& filepath, const json& btJson)
    {
        return AssetLoader::SaveAsset(filepath, btJson);
    }
};

/**
 * HFSM Loader
 * Specialized functions for loading Hierarchical Finite State Machines
 */
class HFSMLoader
{
public:
    static bool LoadHFSM(const std::string& filepath, json& outJson)
    {
        if (!AssetLoader::LoadAsset(filepath, outJson))
            return false;
        
        AssetType type = StringToAssetType(JsonHelper::GetString(outJson, "type", ""));
        if (type != AssetType::HFSM)
        {
            std::cerr << "HFSMLoader: File is not an HFSM" << std::endl;
            return false;
        }
        
        std::vector<std::string> requiredFields = {"states"};
        if (!AssetLoader::ValidateAsset(outJson, requiredFields))
            return false;
        
        return true;
    }
    
    static bool SaveHFSM(const std::string& filepath, const json& hfsmJson)
    {
        return AssetLoader::SaveAsset(filepath, hfsmJson);
    }
};

} // namespace Assets
} // namespace Olympe
