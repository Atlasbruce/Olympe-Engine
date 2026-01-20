/*
 * PrefabScanner.cpp - Prefab Directory Scanner Implementation
 * 
 * Cross-platform directory scanning without std::filesystem.
 * Uses Windows API on Windows, POSIX on Unix/Linux.
 */

#include "PrefabScanner.h"
#include "third_party/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

using json = nlohmann::json;

PrefabScanner::PrefabScanner()
{
}

PrefabScanner::~PrefabScanner()
{
}

PrefabRegistry PrefabScanner::ScanPrefabDirectory(const std::string& rootPath)
{
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         PREFAB DIRECTORY SCAN                                        ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Path: " << rootPath << std::string(std::max(0, 63 - static_cast<int>(rootPath.length())), ' ') << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

    PrefabRegistry registry;
    std::vector<std::string> prefabFiles;
    
    // Scan directory recursively
    std::cout << "→ Scanning for .json files...\n";
#ifdef _WIN32
    ScanDirectoryRecursive_Windows(rootPath, prefabFiles);
#else
    ScanDirectoryRecursive_Unix(rootPath, prefabFiles);
#endif
    
    std::cout << "✓ Found " << prefabFiles.size() << " file(s)\n\n";
    
    // Parse each prefab file
    std::cout << "→ Parsing prefab files...\n";
    int validCount = 0;
    int invalidCount = 0;
    
    for (const auto& filepath : prefabFiles)
    {
        PrefabEntry entry;
        if (ParsePrefabFile(filepath, entry))
        {
            registry.prefabsByName[entry.prefabName] = entry;
            registry.prefabsByType[entry.entityType].push_back(entry);
            validCount++;
            
            std::cout << "  ✓ " << entry.prefabName << " [" << entry.entityType << "] "
                      << "(" << entry.spriteRefs.size() << " sprites, " 
                      << entry.audioRefs.size() << " audio)\n";
        }
        else
        {
            invalidCount++;
            std::cout << "  ✗ " << filepath << " (parse failed)\n";
        }
    }
    
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ SCAN COMPLETE                                                        ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Valid Prefabs:   " << validCount
              << std::string(51 - std::to_string(validCount).length(), ' ') << "║\n";
    std::cout << "║ Invalid Prefabs: " << invalidCount
              << std::string(51 - std::to_string(invalidCount).length(), ' ') << "║\n";
    std::cout << "║ Entity Types:    " << registry.GetTypeCount()
              << std::string(51 - std::to_string(registry.GetTypeCount()).length(), ' ') << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";
    
    return registry;
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

bool PrefabScanner::ParsePrefabFile(const std::string& filepath, PrefabEntry& outEntry)
{
    // Read file
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    try
    {
        json j = json::parse(content);
        
        // Extract prefab name from filename
        outEntry.filePath = filepath;
        outEntry.prefabName = RemoveExtension(GetFilename(filepath));
        
        // Extract entity type
        if (j.contains("entityType"))
        {
            outEntry.entityType = j["entityType"].get<std::string>();
        }
        else if (j.contains("type"))
        {
            outEntry.entityType = j["type"].get<std::string>();
        }
        else
        {
            outEntry.entityType = "undefined";
        }
        
        // Extract resource references
        ExtractResourceReferences(content, outEntry);
        
        outEntry.isValid = true;
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

void PrefabScanner::ExtractResourceReferences(const std::string& jsonContent, PrefabEntry& entry)
{
    try
    {
        json j = json::parse(jsonContent);
        
        // Common sprite reference fields
        std::vector<std::string> spriteFields = {
            "sprite", "spritePath", "texture", "texturePath", "image", "imagePath"
        };
        
        // Common audio reference fields
        std::vector<std::string> audioFields = {
            "audio", "audioPath", "sound", "soundPath", "music", "musicPath"
        };
        
        // Recursively search for resource references
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
                                entry.spriteRefs.push_back(path);
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
                                entry.audioRefs.push_back(path);
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
        
        searchJson(j);
    }
    catch (const std::exception& e)
    {
        // Silently fail - entry will have empty resource refs
    }
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
