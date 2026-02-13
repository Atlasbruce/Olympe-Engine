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
#include "../third_party/nlohmann/json.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

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

// Validate path to prevent directory traversal attacks
static bool IsValidBehaviorTreePath(const std::string& treePath)
{
    // Maximum path length to prevent buffer issues downstream
    const size_t MAX_PATH_LENGTH = 512;
    
    if (treePath.empty() || treePath.length() > MAX_PATH_LENGTH)
        return false;
    
    // Check for directory traversal attempts
    if (treePath.find("..") != std::string::npos)
        return false;
    
    // Must start with valid prefix (prevent absolute paths)
    if (treePath[0] == '/' || treePath[0] == '\\')
        return false;
    
    // Check for path prefix - should start with "Blueprints/"
    if (treePath.find("Blueprints/") != 0)
        return false;
    
    // Basic character whitelist for paths (alphanumeric, /, _, -, .)
    for (char c : treePath)
    {
        if (!std::isalnum(c) && c != '/' && c != '_' && c != '-' && c != '.')
            return false;
    }
    
    return true;
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
                
                // Validate path for security
                if (!IsValidBehaviorTreePath(treePath))
                {
                    std::cerr << "[BTDepScanner] WARNING: Invalid or unsafe BT path rejected: " 
                              << treePath << " (in prefab: " << prefab.prefabName << ")\n";
                    continue;
                }
                
                uint32_t treeId = GenerateTreeIdFromPath(treePath);
                dependencies.emplace_back(treePath, treeId);
                
                std::cout << "[BTDepScanner]   Found BT dependency: " << treePath 
                          << " (ID=" << treeId << ")\n";
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
    
    // Limit number of prefabs to prevent resource exhaustion
    const size_t MAX_PREFABS = 5000;
    if (prefabNames.size() > MAX_PREFABS)
    {
        std::cerr << "[BTDepScanner] ERROR: Too many prefabs to scan (" << prefabNames.size() 
                  << " > " << MAX_PREFABS << "). Possible DoS attempt.\n";
        return allDependencies;
    }
    
    std::cout << "[BTDepScanner] Scanning " << prefabNames.size() << " prefabs for BT dependencies...\n";
    
    for (const auto& prefabName : prefabNames)
    {
        const PrefabBlueprint* prefab = PrefabFactory::Get().GetPrefabRegistry().Find(prefabName);
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

// Validate prefab name to prevent directory traversal
static bool IsValidPrefabName(const std::string& prefabName)
{
    const size_t MAX_PREFAB_NAME_LENGTH = 256;
    
    if (prefabName.empty() || prefabName.length() > MAX_PREFAB_NAME_LENGTH)
        return false;
    
    // Check for path traversal attempts
    if (prefabName.find("..") != std::string::npos)
        return false;
    
    // Should not contain path separators (prefabs are just names, not paths)
    if (prefabName.find('/') != std::string::npos || prefabName.find('\\') != std::string::npos)
        return false;
    
    // Basic character whitelist (alphanumeric, underscore, hyphen)
    for (char c : prefabName)
    {
        if (!std::isalnum(c) && c != '_' && c != '-')
            return false;
    }
    
    return true;
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
    
    // Limit number of layers to prevent resource exhaustion
    const size_t MAX_LAYERS = 1000;
    const auto& layers = levelJson["layers"];
    if (layers.size() > MAX_LAYERS)
    {
        std::cerr << "[BTDepScanner] ERROR: Too many layers in level JSON (" << layers.size() 
                  << " > " << MAX_LAYERS << "). Possible DoS attempt.\n";
        return prefabNames;
    }
    
    for (const auto& layer : layers)
    {
        // Look for object layers (where entities are defined)
        std::string layerType = layer.value("type", "");
        
        if (layerType == "objectgroup")
        {
            if (layer.contains("objects") && layer["objects"].is_array())
            {
                const auto& objects = layer["objects"];
                
                // Limit number of objects per layer to prevent resource exhaustion
                const size_t MAX_OBJECTS_PER_LAYER = 10000;
                if (objects.size() > MAX_OBJECTS_PER_LAYER)
                {
                    std::cerr << "[BTDepScanner] WARNING: Too many objects in layer (" << objects.size() 
                              << " > " << MAX_OBJECTS_PER_LAYER << "). Skipping layer to prevent DoS.\n";
                    continue;
                }
                
                for (const auto& obj : objects)
                {
                    // Extract prefab type (Tiled 1.8 and earlier use "type")
                    if (obj.contains("type") && obj["type"].is_string())
                    {
                        std::string prefabType = obj["type"].get<std::string>();
                        
                        // Validate prefab name for security
                        if (IsValidPrefabName(prefabType))
                        {
                            prefabNames.insert(prefabType);
                        }
                        else if (!prefabType.empty())
                        {
                            std::cerr << "[BTDepScanner] WARNING: Invalid prefab name rejected: " 
                                      << prefabType << "\n";
                        }
                    }
                    
                    // Tiled 1.9+ uses "class" instead of "type"
                    if (obj.contains("class") && obj["class"].is_string())
                    {
                        std::string prefabClass = obj["class"].get<std::string>();
                        
                        // Validate prefab name for security
                        if (IsValidPrefabName(prefabClass))
                        {
                            prefabNames.insert(prefabClass);
                        }
                        else if (!prefabClass.empty())
                        {
                            std::cerr << "[BTDepScanner] WARNING: Invalid prefab name rejected: " 
                                      << prefabClass << "\n";
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "[BTDepScanner] Extracted " << prefabNames.size() << " unique prefab types\n";
    return prefabNames;
}
