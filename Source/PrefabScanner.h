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
#include <set>

// Prefab entry with resource dependencies
struct PrefabEntry
{
    std::string prefabName;        // Short name (e.g., "player")
    std::string filePath;          // Full path to .json file
    std::string entityType;        // Entity type from prefab
    std::vector<std::string> spriteRefs;   // Sprite references in prefab
    std::vector<std::string> audioRefs;    // Audio references in prefab
    bool isValid;                  // Whether prefab JSON was parsed successfully
    
    PrefabEntry() : isValid(false) {}
    
    int GetTotalResourceCount() const 
    { 
        return static_cast<int>(spriteRefs.size() + audioRefs.size()); 
    }
};

// Registry of all discovered prefabs
struct PrefabRegistry
{
    std::map<std::string, std::vector<PrefabEntry>> prefabsByType;  // type -> prefabs
    std::map<std::string, PrefabEntry> prefabsByName;               // name -> prefab
    std::set<std::string> missingPrefabs;                          // Prefabs referenced but not found
    
    int GetTotalPrefabCount() const { return static_cast<int>(prefabsByName.size()); }
    int GetTypeCount() const { return static_cast<int>(prefabsByType.size()); }
    
    bool HasPrefab(const std::string& name) const
    {
        return prefabsByName.find(name) != prefabsByName.end();
    }
    
    const PrefabEntry* GetPrefab(const std::string& name) const
    {
        auto it = prefabsByName.find(name);
        return (it != prefabsByName.end()) ? &it->second : nullptr;
    }
};

// PrefabScanner: Scan directory for prefabs
class PrefabScanner
{
public:
    PrefabScanner();
    ~PrefabScanner();
    
    // Main entry point: Scan a directory for prefab JSON files
    PrefabRegistry ScanPrefabDirectory(const std::string& rootPath);
    
private:
    // Recursive directory scanning (platform-specific)
#ifdef _WIN32
    void ScanDirectoryRecursive_Windows(const std::string& path, std::vector<std::string>& outFiles);
#else
    void ScanDirectoryRecursive_Unix(const std::string& path, std::vector<std::string>& outFiles);
#endif
    
    // Parse a prefab JSON file and extract metadata
    bool ParsePrefabFile(const std::string& filepath, PrefabEntry& outEntry);
    
    // Extract resource references from JSON
    void ExtractResourceReferences(const std::string& jsonContent, PrefabEntry& entry);
    
    // Helper: Extract filename from path
    std::string GetFilename(const std::string& filepath);
    
    // Helper: Remove file extension
    std::string RemoveExtension(const std::string& filename);
};
