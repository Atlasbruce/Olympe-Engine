/*
 * PrefabScanner.h - Prefab Directory Scanner for Phase 2
 * 
 * Responsible for scanning GameData/Prefab directory and building a registry
 * of available prefabs with their resource dependencies.
 * 
 * Cross-platform implementation (Windows/Unix) without std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include "ComponentDefinition.h"

// Resource references extracted from prefab
struct ResourceRefs
{
    std::vector<std::string> spriteRefs;
    std::vector<std::string> audioRefs;
    std::vector<std::string> modelRefs;
};

// Complete prefab blueprint with component definitions
struct PrefabBlueprint
{
    std::string prefabName;
    std::string prefabType;
    std::string filePath;
    std::string version;
    std::string description;
    std::vector<ComponentDefinition> components;
    ResourceRefs resources;
    bool isValid;
	bool isDynamic = false;
    std::vector<std::string> errors;
    
    PrefabBlueprint() : isValid(false) {}
};

// Registry of all discovered prefabs
class PrefabRegistry
{
public:
    PrefabRegistry() = default;
    
    void Register(const PrefabBlueprint& blueprint);
    const PrefabBlueprint* Find(const std::string& name) const;
    std::vector<const PrefabBlueprint*> FindByType(const std::string& type) const;
    std::vector<std::string> GetAllPrefabNames() const;
    int GetCount() const { return static_cast<int>(m_blueprints.size()); }
    
private:
    std::map<std::string, PrefabBlueprint> m_blueprints;  // name -> blueprint
    std::map<std::string, std::string> m_typeToName;      // type -> name
};

// PrefabScanner: Scan directory for prefabs
class PrefabScanner
{
public:
    PrefabScanner();
    ~PrefabScanner();
    
    std::vector<PrefabBlueprint> ScanDirectory(const std::string& rootPath);
    
private:
    PrefabBlueprint ParsePrefab(const std::string& filepath);
    
    // Recursive directory scanning (platform-specific)
#ifdef _WIN32
    void ScanDirectoryRecursive_Windows(const std::string& path, std::vector<std::string>& outFiles);
#else
    void ScanDirectoryRecursive_Unix(const std::string& path, std::vector<std::string>& outFiles);
#endif
    
    // Helper methods
    std::string DetectComponentType(const std::string& typeName);
    void ExtractResources(const nlohmann::json& componentsJson, ResourceRefs& outResources);
    std::string GetFilename(const std::string& filepath);
    std::string RemoveExtension(const std::string& filename);
};
