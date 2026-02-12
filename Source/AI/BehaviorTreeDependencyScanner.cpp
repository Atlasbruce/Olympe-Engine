/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

BehaviorTreeDependencyScanner implementation: Scans prefabs and levels to extract
behavior tree dependencies for automatic loading.
*/

#include "BehaviorTreeDependencyScanner.h"
#include "../PrefabScanner.h"
#include "../prefabfactory.h"
#include "../json_helper.h"
#include <iostream>
#include <algorithm>

// Generate consistent hash from string path using FNV-1a algorithm
uint32_t BehaviorTreeDependencyScanner::GenerateTreeIdFromPath(const std::string& treePath)
{
    // FNV-1a hash - simple but consistent hash function
    uint32_t hash = 2166136261u; // FNV-1a offset basis
    
    for (char c : treePath)
    {
        hash ^= static_cast<uint32_t>(c);
        hash *= 16777619u; // FNV-1a prime
    }
    
    return hash;
}

std::vector<BehaviorTreeDependencyScanner::BTDependency> 
BehaviorTreeDependencyScanner::ScanPrefab(const PrefabBlueprint& prefab)
{
    std::vector<BTDependency> dependencies;
    
    // Iterate through all components in the prefab
    for (const auto& compDef : prefab.components)
    {
        if (compDef.componentType == "BehaviorTreeRuntime_data" || 
            compDef.componentType == "BehaviorTreeRuntime")
        {
            // Look for treePath property
            const auto* treePathParam = compDef.GetParameter("treePath");
            if (treePathParam && treePathParam->type == ComponentParameter::Type::String)
            {
                std::string treePath = treePathParam->AsString();
                
                if (!treePath.empty())
                {
                    uint32_t treeId = GenerateTreeIdFromPath(treePath);
                    dependencies.emplace_back(treePath, treeId);
                    
                    std::cout << "[BTDepScanner]   Found BT dependency: " << treePath 
                              << " (ID=" << treeId << ")\n";
                }
            }
        }
    }
    
    return dependencies;
}

std::vector<BehaviorTreeDependencyScanner::BTDependency> 
BehaviorTreeDependencyScanner::ScanPrefabs(const std::vector<std::string>& prefabNames)
{
    std::vector<BTDependency> allDependencies;
    std::set<std::string> uniquePaths; // Prevent duplicates
    
    std::cout << "[BTDepScanner] Scanning " << prefabNames.size() << " prefabs for BT dependencies...\n";
    
    for (const auto& prefabName : prefabNames)
    {
        const PrefabBlueprint* prefab = PrefabFactory::Get().GetPrefab(prefabName);
        if (!prefab)
        {
            std::cerr << "[BTDepScanner] Warning: Prefab not found: " << prefabName << "\n";
            continue;
        }
        
        auto deps = ScanPrefab(*prefab);
        for (const auto& dep : deps)
        {
            // Only add unique paths
            if (uniquePaths.find(dep.treePath) == uniquePaths.end())
            {
                uniquePaths.insert(dep.treePath);
                allDependencies.push_back(dep);
            }
        }
    }
    
    std::cout << "[BTDepScanner] Found " << allDependencies.size() << " unique BT dependencies\n";
    return allDependencies;
}

std::set<std::string> BehaviorTreeDependencyScanner::ExtractPrefabsFromLevel(const nlohmann::json& levelJson)
{
    std::set<std::string> prefabNames;
    
    std::cout << "[BTDepScanner] Extracting prefab types from level...\n";
    
    // Parse layers in the level
    if (!levelJson.contains("layers") || !levelJson["layers"].is_array())
    {
        std::cerr << "[BTDepScanner] Warning: No 'layers' array in level JSON\n";
        return prefabNames;
    }
    
    for (const auto& layer : levelJson["layers"])
    {
        // Look for object layers (where entities are defined)
        std::string layerType = JsonHelper::GetString(layer, "type", "");
        
        if (layerType == "objectgroup")
        {
            if (layer.contains("objects") && layer["objects"].is_array())
            {
                for (const auto& obj : layer["objects"])
                {
                    // Extract prefab type (Tiled 1.8 and earlier use "type")
                    if (obj.contains("type") && obj["type"].is_string())
                    {
                        std::string prefabType = obj["type"].get<std::string>();
                        if (!prefabType.empty())
                        {
                            prefabNames.insert(prefabType);
                        }
                    }
                    
                    // Tiled 1.9+ uses "class" instead of "type"
                    if (obj.contains("class") && obj["class"].is_string())
                    {
                        std::string prefabClass = obj["class"].get<std::string>();
                        if (!prefabClass.empty())
                        {
                            prefabNames.insert(prefabClass);
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "[BTDepScanner] Extracted " << prefabNames.size() << " unique prefab types\n";
    return prefabNames;
}
