/*
 * PrefabScanner.cpp - Prefab Directory Scanner Implementation
 * 
 * Cross-platform directory scanning without std::filesystem.
 * Uses Windows API on Windows, POSIX on Unix/Linux.
 */

#include "PrefabScanner.h"
#include "ComponentDefinition.h"
#include "ParameterSchema.h"
#include "third_party/nlohmann/json.hpp"
#include "system/system_utils.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>

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
    
    // -> Type already normalized in upstream, use direct comparison
    for (const auto& pair : m_blueprints)
    { 
        if (pair.second.prefabType == type)
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

std::string PrefabScanner::ToUpper(const std::string& str) const
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::vector<PrefabBlueprint> PrefabScanner::ScanDirectory(const std::string& rootPath)
{
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "+======================================================================+\\n";
    SYSTEM_LOG << "|         PREFAB DIRECTORY SCAN                                        |\n";
    SYSTEM_LOG << "|======================================================================|\n";
    SYSTEM_LOG << "| Path: " << rootPath << std::string(max(0, 63 - static_cast<int>(rootPath.length())), ' ') << "|" << std::endl;
    SYSTEM_LOG << "\\======================================================================+\n\n";

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
    SYSTEM_LOG << "+======================================================================+ \n";
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
    SYSTEM_LOG << "\\======================================================================+ \n\n";
    
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
        
        // CRITICAL FIX: Extract prefabType from Identity_data::entityType first
        blueprint.prefabType = ExtractPrefabType(j);
        
        // Fallback to top-level metadata if extraction failed
        if (blueprint.prefabType.empty())
        {
            if (j.contains("type"))
            {
                std::string typeValue = j["type"].get<std::string>();
                if (typeValue != "EntityPrefab" && typeValue != "Player")
                {
                    blueprint.prefabType = typeValue;
                }
            }
            else if (j.contains("blueprintType"))
            {
                blueprint.prefabType = j["blueprintType"].get<std::string>();
            }
        }
        
        // Final fallback: use prefabName
        if (blueprint.prefabType.empty())
        {
            blueprint.prefabType = blueprint.prefabName;
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
    
    // Auto-discover schemas from this prefab if valid
    if (blueprint.isValid)
    {
        ParameterSchemaRegistry::GetInstance().DiscoverSchemasFromPrefab(blueprint);
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

//=============================================================================
// NEW: Synonym System Implementation
//=============================================================================

int LevenshteinDistance(const std::string& s1, const std::string& s2)
{
    const size_t m = s1.size();
    const size_t n = s2.size();
    
    if (m == 0) return static_cast<int>(n);
    if (n == 0) return static_cast<int>(m);
    
    std::vector<std::vector<int>> costs(m + 1, std::vector<int>(n + 1));
    
    for (size_t i = 0; i <= m; ++i) costs[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= n; ++j) costs[0][j] = static_cast<int>(j);
    
    for (size_t i = 1; i <= m; ++i)
    {
        for (size_t j = 1; j <= n; ++j)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            costs[i][j] = min(
                min(costs[i - 1][j] + 1,        // suppression
                         costs[i][j - 1] + 1),       // insertion
                costs[i - 1][j - 1] + cost           // substitution
            );
        }
    }
    
    return costs[m][n];
}

float PrefabScanner::FuzzyMatch(const std::string& str1, const std::string& str2) const
{
    if (str1.empty() || str2.empty()) return 0.0f;
    if (str1 == str2) return 1.0f;
    
    int distance = LevenshteinDistance(str1, str2);
    int maxLen = (int) max(str1.length(), str2.length());
    
    if (maxLen == 0) return 1.0f;
    
    return 1.0f - (static_cast<float>(distance) / static_cast<float>(maxLen));
}

bool PrefabScanner::LoadSynonymRegistry(const std::string& directory)
{
    std::string filepath = directory + "/EntityPrefabSynonymsRegister.json";
    
    SYSTEM_LOG << "Step 1/3: Loading synonym registry...\n";
    SYSTEM_LOG << "  Loading: " << filepath << "\n";
    
    // Check if file exists
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        SYSTEM_LOG << "  /!\\ Synonym registry not found, using default behavior\n";
        return false;
    }
    
    try
    {
        json j;
        file >> j;
        file.close();
        
        // Parse fallback behavior
        if (j.contains("fallbackBehavior") && j["fallbackBehavior"].is_object())
        {
            const json& fb = j["fallbackBehavior"];
            m_caseSensitive = fb.value("caseSensitive", false);
            m_enableFuzzyMatching = fb.value("enableFuzzyMatching", true);
            m_fuzzyThreshold = fb.value("fuzzyThreshold", 0.8f);
            m_logUnmatchedTypes = fb.value("logUnmatchedTypes", true);
        }
        
        // Parse categories
        int totalCategories = 0;
        if (j.contains("categories") && j["categories"].is_object())
        {
            for (const auto& pair : j["categories"].items())
            {
                const std::string& category = pair.first;
                const nlohmann::json& types = *pair.second;
                
                if (types.is_array())
                {
                    std::vector<std::string> typeList;
                    for (const auto& type : types)
                    {
                        if (type.is_string())
                        {
                            typeList.push_back(type.get<std::string>());
                        }
                    }
                    m_categoryToTypes[category] = typeList;
                    totalCategories++;
                }
            }
        }
        
        // Parse canonical types + synonyms
        int totalSynonyms = 0;
        if (j.contains("canonicalTypes") && j["canonicalTypes"].is_object())
        {
            for (const auto& pair : j["canonicalTypes"].items())
            {
                const std::string& canonical = pair.first;
                const nlohmann::json& info = *pair.second;
                
                SynonymInfo synInfo;
                synInfo.canonicalType = canonical;
                
                if (info.contains("description") && info["description"].is_string())
                {
                    synInfo.description = info["description"].get<std::string>();
                }
                
                if (info.contains("prefabFile") && info["prefabFile"].is_string())
                {
                    synInfo.prefabFile = info["prefabFile"].get<std::string>();
                }
                
                // Register canonical type (exact match)
                m_synonymToCanonical[canonical] = canonical;
                
                // Register uppercase version if case-insensitive
                if (!m_caseSensitive)
                {
                    m_synonymToCanonical[ToUpper(canonical)] = canonical;
                }
                
                // Register synonyms
                if (info.contains("synonyms") && info["synonyms"].is_array())
                {
                    for (const auto& synonym : info["synonyms"])
                    {
                        if (synonym.is_string())
                        {
                            std::string synStr = synonym.get<std::string>();
                            synInfo.synonyms.push_back(synStr);
                            m_synonymToCanonical[synStr] = canonical;
                            
                            // Register uppercase version if case-insensitive
                            if (!m_caseSensitive)
                            {
                                m_synonymToCanonical[ToUpper(synStr)] = canonical;
                            }
                            
                            totalSynonyms++;
                        }
                    }
                }
                
                m_canonicalTypes[canonical] = synInfo;
            }
        }
        
        SYSTEM_LOG << "  -> Loaded " << m_canonicalTypes.size() << " canonical types with " 
                   << totalSynonyms << " synonyms\n";
        if (totalCategories > 0)
        {
            SYSTEM_LOG << "  -> Loaded " << totalCategories << " categories\n";
        }
        SYSTEM_LOG << "  Settings: case-sensitive=" << (m_caseSensitive ? "yes" : "no") 
                   << ", fuzzy-matching=" << (m_enableFuzzyMatching ? "yes" : "no") << "\n";
        
        return true;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "  /!\\ Failed to parse synonym registry: " << e.what() << "\n";
        return false;
    }
}

std::string PrefabScanner::ExtractPrefabType(const nlohmann::json& prefabJson)
{
    // Priority 1: Identity_data::entityType
    if (prefabJson.contains("data") && prefabJson["data"].is_object())
    {
        const json& dataJson = prefabJson["data"];
        if (dataJson.contains("components") && dataJson["components"].is_array())
        {
            for (const auto& comp : dataJson["components"])
            {
                if (comp.contains("type") && comp["type"].is_string())
                {
                    std::string compType = comp["type"].get<std::string>();
                    if (compType == "Identity_data" || compType == "Identity")
                    {
                        if (comp.contains("properties") && comp["properties"].is_object())
                        {
                            const json& props = comp["properties"];
                            if (props.contains("entityType") && props["entityType"].is_string())
                            {
                                return props["entityType"].get<std::string>();
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Priority 2: Top-level "type" (if not "EntityPrefab")
    if (prefabJson.contains("type") && prefabJson["type"].is_string())
    {
        std::string type = prefabJson["type"].get<std::string>();
        if (type != "EntityPrefab")
        {
            return type;
        }
    }
    
    return "";
}

std::string PrefabScanner::NormalizeType(const std::string& type) const
{
    if (type.empty()) return type;
    
    // 1. Direct lookup (exact match)
    auto it = m_synonymToCanonical.find(type);
    if (it != m_synonymToCanonical.end())
    {
        return it->second;
    }
    
    // 2. Case-insensitive lookup (if enabled)
    if (!m_caseSensitive)
    {
        std::string upperType = ToUpper(type);
        it = m_synonymToCanonical.find(upperType);
        if (it != m_synonymToCanonical.end())
        {
            return it->second;
        }
    }
    
    // 3. Fuzzy matching (if enabled)
    if (m_enableFuzzyMatching)
    {
        std::string bestMatch;
        float bestScore = 0.0f;
        
        for (const auto& pair : m_canonicalTypes)
        {
            const std::string& canonical = pair.first;
            float score = FuzzyMatch(type, canonical);
            
            if (score > bestScore && score >= m_fuzzyThreshold)
            {
                bestScore = score;
                bestMatch = canonical;
            }
            
            // Also check synonyms
            for (const auto& synonym : pair.second.synonyms)
            {
                score = FuzzyMatch(type, synonym);
                if (score > bestScore && score >= m_fuzzyThreshold)
                {
                    bestScore = score;
                    bestMatch = canonical;
                }
            }
        }
        
        if (!bestMatch.empty())
        {
            SYSTEM_LOG << "  🔍 Fuzzy match: '" << type << "' -> '" << bestMatch 
                       << "' (score: " << bestScore << ")\n";
            return bestMatch;
        }
    }
    
    // 4. Fallback: return original type
    if (m_logUnmatchedTypes && !m_synonymToCanonical.empty())
    {
        SYSTEM_LOG << "  /!\\ Unmatched type: '" << type << "'\n";
    }
    
    return type;
}

bool PrefabScanner::AreTypesEquivalent(const std::string& type1, const std::string& type2) const
{
    if (type1 == type2) return true;
    
    std::string normalized1 = NormalizeType(type1);
    std::string normalized2 = NormalizeType(type2);
    
    return normalized1 == normalized2;
}

bool PrefabScanner::IsTypeRegistered(const std::string& type) const
{
    return m_synonymToCanonical.find(type) != m_synonymToCanonical.end() ||
           m_synonymToCanonical.find(ToUpper(type)) != m_synonymToCanonical.end();
}

bool PrefabScanner::GetCanonicalInfo(const std::string& type, std::string& outCanonical, 
                                      std::string& outPrefabFile) const
{
    // Try to find the canonical type for the given type
    std::string canonical = NormalizeType(type);
    
    // Check if we found a valid canonical type
    auto it = m_canonicalTypes.find(canonical);
    if (it != m_canonicalTypes.end())
    {
        outCanonical = it->second.canonicalType;
        outPrefabFile = it->second.prefabFile;
        return true;
    }
    
    // Not found
    outCanonical.clear();
    outPrefabFile.clear();
    return false;
}

PrefabRegistry PrefabScanner::Initialize(const std::string& prefabDirectory)
{
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "+===========================================================+\n";
    SYSTEM_LOG << "| PREFAB SCANNER: INITIALIZATION                            |\n";
    SYSTEM_LOG << "+===========================================================+\n";
    SYSTEM_LOG << "Directory: " << prefabDirectory << "\n\n";
    
    PrefabRegistry registry;
    
    // Step 1: Load parameter schemas from JSON
    SYSTEM_LOG << "Step 1/4: Loading parameter schemas...\n";
    std::string schemaPath = prefabDirectory + "/ParameterSchemas.json";
    if (!ParameterSchemaRegistry::GetInstance().LoadFromJSON(schemaPath))
    {
        SYSTEM_LOG << "  x Failed to load parameter schemas from: " << schemaPath << "\n";
        SYSTEM_LOG << "  -> Using built-in schemas as fallback\n";
        // Note: Built-in schemas are already initialized via EnsureInitialized()
    }
    else
    {
        size_t schemaCount = ParameterSchemaRegistry::GetInstance().GetSchemaCount();
        SYSTEM_LOG << "  ✓ Loaded " << schemaCount << " parameter schemas from JSON\n";
    }
    
    // Step 2: Load synonym registry
    SYSTEM_LOG << "\nStep 2/4: Loading synonym registry...\n";
    LoadSynonymRegistry(prefabDirectory);
    
    // Step 3: Scan directory for prefab files
    std::vector<std::string> prefabFiles;
    SYSTEM_LOG << "\nStep 3/4: Scanning prefab directory...\n";
    
#ifdef _WIN32
    ScanDirectoryRecursive_Windows(prefabDirectory, prefabFiles);
#else
    ScanDirectoryRecursive_Unix(prefabDirectory, prefabFiles);
#endif
    
    // Filter out the synonym registry and parameter schemas files
    prefabFiles.erase(
        std::remove_if(prefabFiles.begin(), prefabFiles.end(),
            [](const std::string& file) {
                return file.find("EntityPrefabSynonymsRegister.json") != std::string::npos ||
                       file.find("ParameterSchemas.json") != std::string::npos;
            }),
        prefabFiles.end()
    );
    
    SYSTEM_LOG << "  -> Found " << prefabFiles.size() << " .json file(s)\n";
    
    // Step 4: Parse prefabs
    SYSTEM_LOG << "\nStep 4/4: Parsing prefabs...\n";
    
    int validCount = 0;
    int invalidCount = 0;
    
    for (const auto& filepath : prefabFiles)
    {
        PrefabBlueprint blueprint = ParsePrefab(filepath);
        
        if (blueprint.isValid)
        {
            // Normalize the prefab type
            if (!blueprint.prefabType.empty())
            {
                std::string originalType = blueprint.prefabType;
                blueprint.prefabType = NormalizeType(blueprint.prefabType);
                
                if (originalType != blueprint.prefabType)
                {
                    SYSTEM_LOG << "  -> Normalized type: '" << originalType << "' -> '" 
                               << blueprint.prefabType << "' for " << blueprint.prefabName << "\n";
                }
            }
            
            // Assign categories based on prefab type
            for (const auto& categoryPair : m_categoryToTypes)
            {
                const std::string& category = categoryPair.first;
                const std::vector<std::string>& types = categoryPair.second;
                
                // Check if this blueprint's type is in the category
                if (std::find(types.begin(), types.end(), blueprint.prefabType) != types.end())
                {
                    blueprint.AddCategory(category);
                }
            }
            
            registry.Register(blueprint);
            validCount++;
            
            std::string categoryStr = "";
            if (!blueprint.categories.empty())
            {
                categoryStr = " [Categories: ";
                for (size_t i = 0; i < blueprint.categories.size(); i++)
                {
                    categoryStr += blueprint.categories[i];
                    if (i < blueprint.categories.size() - 1) categoryStr += ", ";
                }
                categoryStr += "]";
            }
            
            SYSTEM_LOG << "  -> " << blueprint.prefabName << " [" << blueprint.prefabType << "] "
                       << "(" << blueprint.components.size() << " components)" << categoryStr << "\n";
        }
        else
        {
            invalidCount++;
            SYSTEM_LOG << "  X " << filepath << " (parse failed)\n";
        }
    }
    
    SYSTEM_LOG << "\n";
    SYSTEM_LOG << "+===========================================================+\n";
    SYSTEM_LOG << "| PREFAB SCANNER: INITIALIZATION COMPLETE                   |\n";
    SYSTEM_LOG << "+===========================================================+\n";
    SYSTEM_LOG << "| Total Files Scanned:  " << std::left << std::setw(33) << prefabFiles.size() << "|\n";
    SYSTEM_LOG << "| Valid Prefabs:        " << std::left << std::setw(33) << validCount << "|\n";
    SYSTEM_LOG << "| Invalid Prefabs:      " << std::left << std::setw(33) << invalidCount << "|\n";
    SYSTEM_LOG << "| Canonical Types:      " << std::left << std::setw(33) << m_canonicalTypes.size() << "|\n";
    SYSTEM_LOG << "| Total Synonyms:       " << std::left << std::setw(33) << m_synonymToCanonical.size() << "|\n";
    SYSTEM_LOG << "+===========================================================+\n\n";
    
    return registry;
}
