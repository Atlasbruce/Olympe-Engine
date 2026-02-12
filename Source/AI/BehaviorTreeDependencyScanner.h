/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

BehaviorTreeDependencyScanner purpose: Scan entity prefabs to extract required
Behavior Tree dependencies before level instantiation.
*/

#pragma once
#include <string>
#include <vector>
#include <set>
#include <cstdint>

// Forward declarations
struct PrefabBlueprint;
namespace nlohmann { class json; }

class BehaviorTreeDependencyScanner
{
public:
    struct BTDependency
    {
        std::string treePath;       // "Blueprints/AI/npc_wander.json"
        uint32_t suggestedTreeId;   // Unique ID generated from path hash
        
        BTDependency() : suggestedTreeId(0) {}
        BTDependency(const std::string& path, uint32_t id) 
            : treePath(path), suggestedTreeId(id) {}
    };
    
    // Scan a single prefab for BT requirements
    static std::vector<BTDependency> ScanPrefab(const PrefabBlueprint& prefab);
    
    // Scan multiple prefabs (used during level loading)
    static std::vector<BTDependency> ScanPrefabs(const std::vector<std::string>& prefabNames);
    
    // Extract all prefab types used in a level TMJ/JSON
    static std::set<std::string> ExtractPrefabsFromLevel(const nlohmann::json& levelJson);
    
    // Generate unique tree ID from path (consistent hash)
    static uint32_t GenerateTreeIdFromPath(const std::string& treePath);
    
private:
    BehaviorTreeDependencyScanner() = delete; // Static class only
};
