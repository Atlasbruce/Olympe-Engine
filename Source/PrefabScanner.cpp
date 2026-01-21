/*
 * PrefabScanner.cpp - Prefab Directory Scanner Implementation
 * 
 * Cross-platform directory scanning without std::filesystem.
 * Uses Windows API on Windows, POSIX on Unix/Linux.
 */

#include "PrefabScanner.h"
#include "ComponentDefinition.h"
#include "third_party/nlohmann/json.hpp"
#include "system/system_utils.h"
#include <fstream>
#include <algorithm>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

using json = nlohmann::json;

//=============================================================================
// PrefabRegistry Implementation
//=============================================================================

void PrefabRegistry::Register(const PrefabBlueprint& blueprint)
{
    if (blueprint.prefabName.empty()) return;
    
    m_blueprints[blueprint.prefabName] = blueprint;
    if (!blueprint.prefabType.empty())
    {
        m_typeToName[blueprint.prefabType] = blueprint.prefabName;
    }
}

const PrefabBlueprint* PrefabRegistry::Find(const std::string& name) const
{
    auto it = m_blueprints.find(name);
    return (it != m_blueprints.end()) ? &it->second : nullptr;
}

std::vector<const PrefabBlueprint*> PrefabRegistry::FindByType(const std::string& type) const
{
    std::vector<const PrefabBlueprint*> results;
    for (const auto& pair : m_blueprints)
    { 
		// Case-insensitive comparison -> convert both stings to uppercase
        std::string prefabType = pair.second.prefabType;
		std::transform(prefabType.begin(), prefabType.end(), prefabType.begin(), ::toupper);
		std::string searchType = type;
		std::transform(searchType.begin(), searchType.end(), searchType.begin(), ::toupper);

        if (prefabType == searchType)
        {
            results.push_back(&pair.second);
        }
    }
    return results;
}

std::vector<std::string> PrefabRegistry::GetAllPrefabNames() const
{
    std::vector<std::string> names;
    names.reserve(m_blueprints.size());
    for (const auto& pair : m_blueprints)
    {
        names.push_back(pair.first);
    }
    return names;
}

//=============================================================================
// PrefabScanner Implementation
//=============================================================================

PrefabScanner::PrefabScanner()
{
}

PrefabScanner::~PrefabScanner()
{
}

std::vector<PrefabBlueprint> PrefabScanner::ScanDirectory(const std::string& rootPath)
{
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\\\n";
    SYSTEM_LOG << "|         PREFAB DIRECTORY SCAN                                        |\n";
    SYSTEM_LOG << "|======================================================================|\n";
    SYSTEM_LOG << "| Path: " << rootPath << std::string(max(0, 63 - static_cast<int>(rootPath.length())), ' ') << "|" << endl;
    SYSTEM_LOG << "\\======================================================================/\n\n";

    std::vector<PrefabBlueprint> blueprints;
    std::vector<std::string> prefabFiles;
    
    // Scan directory recursively
    SYSTEM_LOG << "-> Scanning for .json files...\n";
#ifdef _WIN32
    ScanDirectoryRecursive_Windows(rootPath, prefabFiles);
#else
    ScanDirectoryRecursive_Unix(rootPath, prefabFiles);
#endif
    
    SYSTEM_LOG << "-> Found " << prefabFiles.size() << " file(s)\n\n";
    
    // Parse each prefab file
    SYSTEM_LOG << "-> Parsing prefab files...\n";
    int validCount = 0;
    int invalidCount = 0;
    int totalComponents = 0;
    int totalResources = 0;
    
    for (const auto& filepath : prefabFiles)
    {
        PrefabBlueprint blueprint = ParsePrefab(filepath);
        
        if (blueprint.isValid)
        {
            blueprints.push_back(blueprint);
            validCount++;
            totalComponents += static_cast<int>(blueprint.components.size());
            totalResources += static_cast<int>(blueprint.resources.spriteRefs.size() + 
                                              blueprint.resources.audioRefs.size() + 
                                              blueprint.resources.modelRefs.size());
            
            SYSTEM_LOG << "  -> " << blueprint.prefabName << " [" << blueprint.prefabType << "] "
                      << "(" << blueprint.components.size() << " components, " 
                      << (blueprint.resources.spriteRefs.size() + blueprint.resources.audioRefs.size() + blueprint.resources.modelRefs.size()) 
                      << " resources)\n";
        }
        else
        {
            invalidCount++;
            SYSTEM_LOG << "  x " << filepath << " (parse failed)";
            if (!blueprint.errors.empty())
            {
                SYSTEM_LOG << " - " << blueprint.errors[0];
            }
            SYSTEM_LOG << "\n";
        }
    }
    
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "/======================================================================\ \n";
    SYSTEM_LOG << "| SCAN COMPLETE                                                        | \n";
    SYSTEM_LOG << "|======================================================================| \n";
    SYSTEM_LOG << "| Valid Prefabs:   " << validCount
              << std::string(51 - std::to_string(validCount).length(), ' ') << "| \n";
    SYSTEM_LOG << "| Invalid Prefabs: " << invalidCount
              << std::string(51 - std::to_string(invalidCount).length(), ' ') << "| \n";
    SYSTEM_LOG << "| Total Components: " << totalComponents
              << std::string(50 - std::to_string(totalComponents).length(), ' ') << "| \n";
    SYSTEM_LOG << "| Total Resources:  " << totalResources
              << std::string(50 - std::to_string(totalResources).length(), ' ') << "| \n";
    SYSTEM_LOG << "\\======================================================================/ \n\n";
    
    return blueprints;
}

#ifdef _WIN32
void PrefabScanner::ScanDirectoryRecursive_Windows(const std::string& path, std::vector<std::string>& outFiles)
{
    WIN32_FIND_DATAA findData;
    std::string searchPath = path + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    
    do
    {
        std::string filename = findData.cFileName;
        
        // Skip . and ..
        if (filename == "." || filename == "..")
            continue;
        
        std::string fullPath = path + "\\" + filename;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Recursively scan subdirectory
            ScanDirectoryRecursive_Windows(fullPath, outFiles);
        }
        else
        {
            // Check if it's a .json file
            if (filename.length() > 5 && 
                filename.substr(filename.length() - 5) == ".json")
            {
                outFiles.push_back(fullPath);
            }
        }
    } while (FindNextFileA(hFind, &findData) != 0);
    
    FindClose(hFind);
}
#else
void PrefabScanner::ScanDirectoryRecursive_Unix(const std::string& path, std::vector<std::string>& outFiles)
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
    {
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename = entry->d_name;
        
        // Skip . and ..
        if (filename == "." || filename == "..")
            continue;
        
        std::string fullPath = path + "/" + filename;
        
        // Check if it's a directory
        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == 0)
        {
            if (S_ISDIR(statbuf.st_mode))
            {
                // Recursively scan subdirectory
                ScanDirectoryRecursive_Unix(fullPath, outFiles);
            }
            else if (S_ISREG(statbuf.st_mode))
            {
                // Check if it's a .json file
                if (filename.length() > 5 && 
                    filename.substr(filename.length() - 5) == ".json")
                {
                    outFiles.push_back(fullPath);
                }
            }
        }
    }
    
    closedir(dir);
}
#endif

PrefabBlueprint PrefabScanner::ParsePrefab(const std::string& filepath)
{
    PrefabBlueprint blueprint;
    blueprint.filePath = filepath;
    blueprint.prefabName = RemoveExtension(GetFilename(filepath));
    
    // Read file
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        blueprint.errors.push_back("Failed to open file");
        return blueprint;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    try
    {
        json j = json::parse(content);
        
        // Extract top-level metadata
        if (j.contains("type"))
        {
            blueprint.prefabType = j["type"].get<std::string>();
        }
        else if (j.contains("blueprintType"))
        {
            blueprint.prefabType = j["blueprintType"].get<std::string>();
        }
        
        if (j.contains("schema_version"))
        {
            blueprint.version = std::to_string(j["schema_version"].get<int>());
        }
        
        if (j.contains("description"))
        {
            blueprint.description = j["description"].get<std::string>();
        }
        
        if (j.contains("name"))
        {
            blueprint.prefabName = j["name"].get<std::string>();
        }
        
        // Parse the "data" field
        if (j.contains("data") && j["data"].is_object())
        {
            json dataJson = j["data"];
            
            // Override prefab name from data if available
            if (dataJson.contains("prefabName"))
            {
                blueprint.prefabName = dataJson["prefabName"].get<std::string>();
            }
            
            // Parse components array
            if (dataJson.contains("components") && dataJson["components"].is_array())
            {
                json componentsArray = dataJson["components"];
                
                for (const auto& compJson : componentsArray)
                {
                    try
                    {
                        ComponentDefinition compDef = ComponentDefinition::FromJSON(compJson);
                        blueprint.components.push_back(compDef);
                    }
                    catch (const std::exception& e)
                    {
                        blueprint.errors.push_back(std::string("Component parse error: ") + e.what());
                    }
                }
                
                // Extract resource references from components
                ExtractResources(componentsArray, blueprint.resources);
            }
        }
        
        blueprint.isValid = true;
    }
    catch (const std::exception& e)
    {
        blueprint.errors.push_back(std::string("JSON parse error: ") + e.what());
        blueprint.isValid = false;
    }
    
    return blueprint;
}

std::string PrefabScanner::DetectComponentType(const std::string& typeName)
{
    // Component type detection heuristics
    std::string lower = typeName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower.find("identity") != std::string::npos) return "Identity";
    if (lower.find("position") != std::string::npos) return "Position";
    if (lower.find("sprite") != std::string::npos || lower.find("visual") != std::string::npos) return "VisualSprite";
    if (lower.find("boundingbox") != std::string::npos || lower.find("collision") != std::string::npos) return "BoundingBox";
    if (lower.find("movement") != std::string::npos) return "Movement";
    if (lower.find("physics") != std::string::npos) return "PhysicsBody";
    if (lower.find("health") != std::string::npos) return "Health";
    if (lower.find("player") != std::string::npos && lower.find("binding") != std::string::npos) return "PlayerBinding";
    if (lower.find("controller") != std::string::npos) return "Controller";
    if (lower.find("audio") != std::string::npos || lower.find("sound") != std::string::npos) return "Audio";
    
    return typeName;
}

void PrefabScanner::ExtractResources(const nlohmann::json& componentsJson, ResourceRefs& outResources)
{
    // Common resource field names
    std::vector<std::string> spriteFields = {
        "sprite", "spritePath", "texture", "texturePath", "image", "imagePath"
    };
    
    std::vector<std::string> audioFields = {
        "audio", "audioPath", "sound", "soundPath", "music", "musicPath"
    };
    
    std::vector<std::string> modelFields = {
        "model", "modelPath", "mesh", "meshPath"
    };
    
    // Recursive search for resource references
    std::function<void(const json&)> searchJson;
    searchJson = [&](const json& obj)
    {
        if (obj.is_object())
        {
            for (auto it = obj.begin(); it != obj.end(); ++it)
            {
                std::string key = it.key();
                
                // Check for sprite references
                for (const auto& field : spriteFields)
                {
                    if (key == field && it.value().is_string())
                    {
                        std::string path = it.value().get<std::string>();
                        if (!path.empty())
                        {
                            outResources.spriteRefs.push_back(path);
                        }
                    }
                }
                
                // Check for audio references
                for (const auto& field : audioFields)
                {
                    if (key == field && it.value().is_string())
                    {
                        std::string path = it.value().get<std::string>();
                        if (!path.empty())
                        {
                            outResources.audioRefs.push_back(path);
                        }
                    }
                }
                
                // Check for model references
                for (const auto& field : modelFields)
                {
                    if (key == field && it.value().is_string())
                    {
                        std::string path = it.value().get<std::string>();
                        if (!path.empty())
                        {
                            outResources.modelRefs.push_back(path);
                        }
                    }
                }
                
                // Recurse into nested objects and arrays
                if (it.value().is_object() || it.value().is_array())
                {
                    searchJson(it.value());
                }
            }
        }
        else if (obj.is_array())
        {
            for (const auto& element : obj)
            {
                searchJson(element);
            }
        }
    };
    
    searchJson(componentsJson);
}

std::string PrefabScanner::GetFilename(const std::string& filepath)
{
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        return filepath.substr(pos + 1);
    }
    return filepath;
}

std::string PrefabScanner::RemoveExtension(const std::string& filename)
{
    size_t pos = filename.find_last_of(".");
    if (pos != std::string::npos)
    {
        return filename.substr(0, pos);
    }
    return filename;
}
