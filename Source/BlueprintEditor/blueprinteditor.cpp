/*
 * Olympe Blueprint Editor - Backend Implementation
 * 
 * Singleton backend managing business logic, state, and data
 * Completely separated from UI rendering (handled by BlueprintEditorGUI)
 */

#include "BlueprintEditor.h"
#include "EntityBlueprint.h"
#include "EditorContext.h"
#include "WorldBridge.h"
#include "EnumCatalogManager.h"
#include "BTNodeGraphManager.h"
#include "EntityInspectorManager.h"
#include "TemplateManager.h"
#include "BPCommandSystem.h"
#include "BlueprintMigrator.h"
#include "BlueprintValidator.h"
#include "BehaviorTreeEditorPlugin.h"
#include "EntityPrefabEditorPlugin.h"
#include "AdditionalEditorPlugins.h"
#include "SubgraphMigrator.h"
#include "TabManager.h"
#include "../TaskSystem/TaskGraphLoader.h"
#include "../Core/AssetManager.h"
#include "../json_helper.h"
#include "../system/system_utils.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

using namespace Olympe::Blueprint;

namespace Olympe
{
    // ========================================================================
    // BlueprintEditor Singleton Backend Implementation
    // ========================================================================
    
    BlueprintEditor& BlueprintEditor::Instance()
    {
        static BlueprintEditor instance;
        return instance;
    }

    BlueprintEditor::BlueprintEditor()
        : m_IsActive(false)
        , m_HasUnsavedChanges(false)
        , m_AssetRootPath("Blueprints")
        , m_GamedataRootPath("Gamedata")
        , m_SelectedEntity(0)  // 0 = INVALID_ENTITY_ID
        , m_CommandStack(nullptr)
        , m_ShowMigrationDialog(false)
    {
    }

    BlueprintEditor::~BlueprintEditor()
    {
        if (m_CommandStack)
        {
            delete m_CommandStack;
            m_CommandStack = nullptr;
        }
    }

    void BlueprintEditor::Initialize()
    {
        // Initialize backend state
        m_IsActive = false;
        m_HasUnsavedChanges = false;
        m_CurrentBlueprint = Blueprint::EntityBlueprint();
        m_CurrentFilepath = "";
        
        // Initialize asset management
        m_AssetTreeRoot = nullptr;
        m_LastError.clear();
        
        // Initialize catalog manager
        EnumCatalogManager::Get().Initialize();
        
        // Initialize node graph manager
        NodeGraphManager::Get().Initialize();
        
        // Initialize entity inspector manager
        EntityInspectorManager::Get().Initialize();
        
        // Initialize template manager
        TemplateManager::Get().Initialize();
        
        // Initialize command stack
        m_CommandStack = new Olympe::Blueprint::CommandStack();
        
        // Initialize plugin system
        InitializePlugins();

        // Load configuration
        LoadConfig("blueprint_editor_config.json");

        // Scan assets on initialization
        RefreshAssets();
    }
    
    void BlueprintEditor::InitializeRuntimeEditor()
    {
        std::cout << "[BlueprintEditor] Initializing Runtime Editor mode\n";
        // Initialize the EditorContext in Runtime mode
        EditorContext::Get().InitializeRuntime();
    }
    
    void BlueprintEditor::InitializeStandaloneEditor()
    {
        std::cout << "[BlueprintEditor] Initializing Standalone Editor mode\n";
        // Initialize the EditorContext in Standalone mode
        EditorContext::Get().InitializeStandalone();

        // Pre-load all ATS graphs found in Blueprints/ and Gamedata/ so that
        // they are validated and ready for the asset browser and graph panels.
        PreloadATSGraphs();
    }

    void BlueprintEditor::Shutdown()
    {
        // Save configuration before shutting down
        SaveConfig("blueprint_editor_config.json");

        // Unregister the task callback before shutting down panels so that
        // TaskSystem cannot fire callbacks into already-destroyed editor objects.
        WorldBridge_UnregisterTaskCallback();

        // Shutdown managers in reverse order
        if (m_CommandStack)
        {
            delete m_CommandStack;
            m_CommandStack = nullptr;
        }
        
        TemplateManager::Get().Shutdown();
        EntityInspectorManager::Get().Shutdown();
        NodeGraphManager::Get().Shutdown();
        EnumCatalogManager::Get().Shutdown();
        
        // Clean up backend resources
        m_CurrentBlueprint = Blueprint::EntityBlueprint();
        m_CurrentFilepath = "";
        m_HasUnsavedChanges = false;
    }

    void BlueprintEditor::Update(float deltaTime)
    {
        // Backend update logic (non-UI)
        // This is called by GameEngine when the editor is active
        
        // Update entity inspector (sync with World)
        EntityInspectorManager::Get().Update();
        
        // Can be used for background tasks, auto-save, etc.
        // For now, this is a placeholder for future backend logic
        // such as:
        // - Auto-save timer
        // - Asset watching/hot-reload
        // - Background compilation
        // - Validation
    }

    // Blueprint operations
    void BlueprintEditor::NewBlueprint(const std::string& name, const std::string& description)
    {
        m_CurrentBlueprint = Blueprint::EntityBlueprint(name);
        m_CurrentBlueprint.description = description;
        m_CurrentFilepath = "";
        m_HasUnsavedChanges = true;
    }

    bool BlueprintEditor::LoadBlueprint(const std::string& filepath)
    {
        Blueprint::EntityBlueprint loaded = Blueprint::EntityBlueprint::LoadFromFile(filepath);
        
        if (loaded.name.empty())
        {
            return false;
        }
        
        m_CurrentBlueprint = loaded;
        m_CurrentFilepath = filepath;
        m_HasUnsavedChanges = false;
        return true;
    }

    bool BlueprintEditor::SaveBlueprint()
    {
        if (m_CurrentBlueprint.name.empty())
        {
            return false;
        }
        
        if (m_CurrentFilepath.empty())
        {
            // No filepath set - caller should use SaveBlueprintAs
            return false;
        }
        
        bool success = m_CurrentBlueprint.SaveToFile(m_CurrentFilepath);
        
        if (success)
        {
            m_HasUnsavedChanges = false;
        }
        
        return success;
    }

    bool BlueprintEditor::SaveBlueprintAs(const std::string& filepath)
    {
        if (m_CurrentBlueprint.name.empty())
        {
            return false;
        }
        
        bool success = m_CurrentBlueprint.SaveToFile(filepath);
        
        if (success)
        {
            m_CurrentFilepath = filepath;
            m_HasUnsavedChanges = false;
        }
        
        return success;
    }

    // ========================================================================
    // Asset Management Implementation
    // ========================================================================
    
    void BlueprintEditor::SetAssetRootPath(const std::string& path)
    {
        m_AssetRootPath = path;
        RefreshAssets();
    }
    
    void BlueprintEditor::RefreshAssets()
    {
        m_LastError.clear();
        
        if (m_AssetRootPath.empty())
        {
            m_LastError = "Asset root path is not set";
            std::cerr << "BlueprintEditor: " << m_LastError << std::endl;
            m_AssetTreeRoot = nullptr;
            return;
        }

        // Build a virtual root node that holds both Blueprints/ and Gamedata/ trees.
        auto virtualRoot = std::make_shared<AssetNode>("Assets", ".", true);

        std::cout << "BlueprintEditor: Scanning assets directory: " << m_AssetRootPath << std::endl;

        try
        {
            if (fs::exists(m_AssetRootPath) && fs::is_directory(m_AssetRootPath))
            {
                auto blueprintsTree = ScanDirectory(m_AssetRootPath);
                virtualRoot->children.push_back(blueprintsTree);
                std::cout << "BlueprintEditor: Blueprints scan complete" << std::endl;
            }
            else
            {
                std::cerr << "BlueprintEditor: Blueprints directory not found: "
                          << m_AssetRootPath << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            m_LastError = std::string("Error scanning blueprints: ") + e.what();
            std::cerr << "BlueprintEditor: " << m_LastError << std::endl;
        }

        // Also scan the Gamedata directory (contains .ats ATS task graphs).
        if (!m_GamedataRootPath.empty())
        {
            std::cout << "BlueprintEditor: Scanning gamedata directory: "
                      << m_GamedataRootPath << std::endl;
            try
            {
                if (fs::exists(m_GamedataRootPath) && fs::is_directory(m_GamedataRootPath))
                {
                    auto gamedataTree = ScanDirectory(m_GamedataRootPath);
                    virtualRoot->children.push_back(gamedataTree);
                    std::cout << "BlueprintEditor: Gamedata scan complete" << std::endl;
                }
                else
                {
                    std::cerr << "BlueprintEditor: Gamedata directory not found: "
                              << m_GamedataRootPath << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "BlueprintEditor: Error scanning gamedata: " << e.what() << std::endl;
            }
        }

        m_AssetTreeRoot = virtualRoot;
    }
    
    std::shared_ptr<AssetNode> BlueprintEditor::ScanDirectory(const std::string& path)
    {
        auto node = std::make_shared<AssetNode>(
            fs::path(path).filename().string(),
            path,
            true
        );

        try
        {
            for (const auto& entry : fs::directory_iterator(path))
            {
                std::string entryPath = entry.path().string();
                std::string filename = entry.path().filename().string();
                
                // Skip hidden files and directories
                if (filename[0] == '.')
                    continue;

                if (fs::is_directory(entry.path()))
                {
                    // Recursively scan subdirectories
                    auto childNode = ScanDirectory(entryPath);
                    node->children.push_back(childNode);
                }
                else if (fs::is_regular_file(entry.path()))
                {
                    // Accept .json blueprint files and .ats ATS task graph files.
                    const std::string ext = entry.path().extension().string();
                    if (ext == ".json" || ext == ".ats")
                    {
                        auto fileNode = std::make_shared<AssetNode>(
                            filename,
                            entryPath,
                            false
                        );
                        
                        // Detect asset type
                        fileNode->type = DetectAssetType(entryPath);
                        
                        node->children.push_back(fileNode);
                    }
                }
            }

            // Sort children: directories first, then files alphabetically
            std::sort(node->children.begin(), node->children.end(),
                [](const std::shared_ptr<AssetNode>& a, const std::shared_ptr<AssetNode>& b)
                {
                    if (a->isDirectory != b->isDirectory)
                        return a->isDirectory > b->isDirectory;
                    return a->name < b->name;
                });
        }
        catch (const std::exception& e)
        {
            std::cerr << "BlueprintEditor: Error scanning directory " << path << ": " << e.what() << std::endl;
        }

        return node;
    }
    
    std::string BlueprintEditor::DetectAssetType(const std::string& filepath)
    {
        try
        {
            // .ats files are ATS task graphs — detect type from their graphType field
            // without going through the full blueprint type-detection logic.
            const std::string ext = fs::path(filepath).extension().string();
            if (ext == ".ats")
            {
                json j;
                if (!JsonHelper::LoadJsonFromFile(filepath, j))
                    return "TaskGraph";

                if (j.contains("graphType"))
                    return j["graphType"].get<std::string>();

                return "TaskGraph";
            }

            json j;
            if (!JsonHelper::LoadJsonFromFile(filepath, j))
                return "Unknown";

            // Extract filename once for warning messages
            std::string filename = fs::path(filepath).filename().string();

            // Priority 1: Check explicit "graphType" field (VS v4 / ATS format).
            // This is the canonical field for schema_version 4 graphs stored as .json.
            // Must be checked before the legacy "type" field to avoid falling through
            // to structural detection and emitting a spurious WARNING.
            if (j.contains("graphType") && j["graphType"].is_string())
            {
                return j["graphType"].get<std::string>();
            }

            // Priority 2: Check explicit "type" field (v1 + v2 standardized)
            if (j.contains("type"))
            {
                std::string type = j["type"].get<std::string>();
                
                // Validate against blueprintType if present
                if (j.contains("blueprintType"))
                {
                    std::string blueprintType = j["blueprintType"].get<std::string>();
                    if (type != blueprintType)
                    {
                        std::cerr << "[BlueprintEditor] WARNING: type (" << type 
                                 << ") != blueprintType (" << blueprintType << ") in " << filename << std::endl;
                    }
                }
                
                return type;
            }
            
            // Priority 3: FALLBACK - Check "blueprintType" for old v2 files
            if (j.contains("blueprintType"))
            {
                std::string type = j["blueprintType"].get<std::string>();
                std::cerr << "[BlueprintEditor] WARNING: Using 'blueprintType' field (missing 'type') in " 
                         << filename << std::endl;
                return type;
            }

            // Helper lambda for logging structural detection warnings
            auto logStructuralDetection = [&filename](const std::string& detectedType) {
                std::cerr << "[BlueprintEditor] WARNING: No type information found in " << filename 
                         << ", using structural detection (detected: " << detectedType << ")" << std::endl;
            };

            // Priority 4: Structural detection for schema v2 (data wrapper)
            if (j.contains("data"))
            {
                const json& data = j["data"];
                if (data.contains("rootNodeId") && data.contains("nodes"))
                {
                    logStructuralDetection("BehaviorTree");
                    return "BehaviorTree";
                }
                if (data.contains("components"))
                {
                    logStructuralDetection("EntityPrefab");
                    return "EntityPrefab";
                }
            }

            // Priority 5: Structural detection for schema v1 (direct fields)
            if (j.contains("rootNodeId") && j.contains("nodes"))
            {
                logStructuralDetection("BehaviorTree");
                return "BehaviorTree";
            }

            if (j.contains("states") || j.contains("initialState"))
            {
                logStructuralDetection("HFSM");
                return "HFSM";
            }

            // Priority 6: Structural detection for Visual Script graphs
            if (j.contains("schema_version") || j.contains("ExecConnections") || j.contains("DataConnections"))
            {
                logStructuralDetection("VisualScript");
                return "VisualScript";
            }

            if (j.contains("components"))
            {
                logStructuralDetection("EntityBlueprint");
                return "EntityBlueprint";
            }

            std::cerr << "[BlueprintEditor] WARNING: Could not determine type for " << filename 
                     << ", defaulting to Generic" << std::endl;
            return "Generic";
        }
        catch (const std::exception& e)
        {
            std::string filename = fs::path(filepath).filename().string();
            std::cerr << "Error detecting asset type in " << filename << ": " << e.what() << std::endl;
            return "Unknown";
        }
    }

    // ========================================================================
    // PreloadATSGraphs – recursively load all .ats files from Blueprints/ and
    // Gamedata/ to validate them and warm up any caches.
    // ========================================================================

    void BlueprintEditor::PreloadATSGraphs()
    {
        std::cout << "[BlueprintEditor] PreloadATSGraphs: scanning "
                  << m_AssetRootPath << " and " << m_GamedataRootPath << std::endl;

        int loaded = 0;
        int failed  = 0;

        // Collect .ats files from the already-scanned asset tree (BFS).
        std::vector<std::string> atsFiles;

        auto collectAts = [&atsFiles](const std::shared_ptr<AssetNode>& root)
        {
            if (!root) return;
            std::vector<std::shared_ptr<AssetNode>> stack;
            stack.push_back(root);
            while (!stack.empty())
            {
                auto node = stack.back();
                stack.pop_back();
                if (!node) continue;
                if (!node->isDirectory)
                {
                    const std::string& p = node->fullPath;
                    if (p.size() > 4 && p.substr(p.size() - 4) == ".ats")
                    {
                        atsFiles.push_back(p);
                    }
                }
                for (const auto& child : node->children)
                    stack.push_back(child);
            }
        };

        collectAts(m_AssetTreeRoot);

        // Load and cache each .ats file via AssetManager so that the template
        // remains resident in memory and can be retrieved without further I/O.
        for (const auto& path : atsFiles)
        {
            std::vector<std::string> errors;
            AssetID id = AssetManager::Get().LoadTaskGraph(path, errors);
            if (id != INVALID_ASSET_ID)
            {
                std::cout << "[BlueprintEditor] PreloadATSGraphs: OK  " << path << std::endl;
                ++loaded;
            }
            else
            {
                std::cerr << "[BlueprintEditor] PreloadATSGraphs: FAIL " << path << std::endl;
                for (const auto& err : errors)
                    std::cerr << "  " << err << std::endl;
                ++failed;
            }
        }

        std::cout << "[BlueprintEditor] PreloadATSGraphs complete: "
                  << loaded << " loaded, " << failed << " failed" << std::endl;
    }
    
    std::vector<AssetMetadata> BlueprintEditor::GetAllAssets() const
    {
        std::vector<AssetMetadata> assets;
        if (m_AssetTreeRoot)
        {
            CollectAllAssets(m_AssetTreeRoot, assets);
        }
        return assets;
    }
    
    void BlueprintEditor::CollectAllAssets(const std::shared_ptr<AssetNode>& node, std::vector<AssetMetadata>& assets) const
    {
        if (!node)
            return;
            
        // Add files only, not directories
        if (!node->isDirectory)
        {
            AssetMetadata metadata;
            metadata.filepath = node->fullPath;
            metadata.name = node->name;
            metadata.type = node->type;
            metadata.isDirectory = false;
            
            // Parse full metadata (const_cast to call non-const method)
            const_cast<BlueprintEditor*>(this)->ParseAssetMetadata(node->fullPath, metadata);
            
            assets.push_back(metadata);
        }
        
        // Recursively process children
        for (const auto& child : node->children)
        {
            CollectAllAssets(child, assets);
        }
    }
    
    std::vector<AssetMetadata> BlueprintEditor::GetAssetsByType(const std::string& type) const
    {
        std::vector<AssetMetadata> allAssets = GetAllAssets();
        std::vector<AssetMetadata> filtered;
        
        for (const auto& asset : allAssets)
        {
            if (asset.type == type)
            {
                filtered.push_back(asset);
            }
        }
        
        return filtered;
    }
    
    std::vector<AssetMetadata> BlueprintEditor::SearchAssets(const std::string& query) const
    {
        std::vector<AssetMetadata> allAssets = GetAllAssets();
        std::vector<AssetMetadata> results;
        
        if (query.empty())
            return allAssets;
        
        // Case-insensitive search
        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        for (const auto& asset : allAssets)
        {
            std::string lowerName = asset.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName.find(lowerQuery) != std::string::npos)
            {
                results.push_back(asset);
            }
        }
        
        return results;
    }
    
    AssetMetadata BlueprintEditor::GetAssetMetadata(const std::string& filepath)
    {
        AssetMetadata metadata;
        metadata.filepath = filepath;
        
        try
        {
            // Get filename
            metadata.name = fs::path(filepath).filename().string();
            
            // Detect type
            metadata.type = DetectAssetType(filepath);
            
            // Parse detailed metadata
            ParseAssetMetadata(filepath, metadata);
        }
        catch (const std::exception& e)
        {
            metadata.isValid = false;
            metadata.errorMessage = std::string("Error loading asset: ") + e.what();
            std::cerr << "BlueprintEditor: " << metadata.errorMessage << std::endl;
        }
        
        return metadata;
    }
    
    void BlueprintEditor::ParseAssetMetadata(const std::string& filepath, AssetMetadata& metadata)
    {
        try
        {
            json j;
            if (!JsonHelper::LoadJsonFromFile(filepath, j))
            {
                metadata.isValid = false;
                metadata.errorMessage = "Failed to load JSON file";
                return;
            }
            
            // Priority 1: Check explicit "type" field
            if (j.contains("type"))
            {
                std::string type = JsonHelper::GetString(j, "type", "");
                metadata.type = type;
                
                if (type == "EntityBlueprint" || type == "EntityPrefab")
                {
                    ParseEntityBlueprint(j, metadata);
                }
                else if (type == "BehaviorTree")
                {
                    ParseBehaviorTree(j, metadata);
                }
                else if (type == "HFSM")
                {
                    ParseHFSM(j, metadata);
                }
                else
                {
                    metadata.name = JsonHelper::GetString(j, "name", metadata.name);
                    metadata.description = JsonHelper::GetString(j, "description", "");
                }
            }
            // Priority 2: FALLBACK - Check "blueprintType" for old v2 files
            else if (j.contains("blueprintType"))
            {
                std::string type = JsonHelper::GetString(j, "blueprintType", "");
                metadata.type = type;
                
                std::string filename = fs::path(filepath).filename().string();
                std::cerr << "[ParseAssetMetadata] Warning: Using deprecated 'blueprintType' field in " << filename << std::endl;
                
                if (type == "BehaviorTree")
                {
                    ParseBehaviorTree(j, metadata);
                }
                else if (type == "HFSM")
                {
                    ParseHFSM(j, metadata);
                }
                else if (type == "EntityBlueprint" || type == "EntityPrefab")
                {
                    ParseEntityBlueprint(j, metadata);
                }
                else
                {
                    metadata.name = JsonHelper::GetString(j, "name", metadata.name);
                    metadata.description = JsonHelper::GetString(j, "description", "");
                }
            }
            // Priority 3: Structural detection for schema v2 (data wrapper)
            else if (j.contains("data"))
            {
                const json& data = j["data"];
                if (data.contains("rootNodeId") && data.contains("nodes"))
                {
                    metadata.type = "BehaviorTree";
                    ParseBehaviorTree(j, metadata);
                }
                else if (data.contains("components"))
                {
                    metadata.type = "EntityPrefab";
                    ParseEntityBlueprint(j, metadata);
                }
                else
                {
                    metadata.type = "Generic";
                    metadata.name = JsonHelper::GetString(j, "name", metadata.name);
                    metadata.description = JsonHelper::GetString(j, "description", "");
                }
            }
            // Priority 4: Structural detection for schema v1 (direct fields)
            else if (j.contains("rootNodeId") && j.contains("nodes"))
            {
                metadata.type = "BehaviorTree";
                ParseBehaviorTree(j, metadata);
            }
            else if (j.contains("states") || j.contains("initialState"))
            {
                metadata.type = "HFSM";
                ParseHFSM(j, metadata);
            }
            else if (j.contains("components"))
            {
                metadata.type = "EntityBlueprint";
                ParseEntityBlueprint(j, metadata);
            }
            else
            {
                metadata.type = "Generic";
                metadata.name = JsonHelper::GetString(j, "name", metadata.name);
                metadata.description = JsonHelper::GetString(j, "description", "");
            }
            
            metadata.isValid = true;
        }
        catch (const std::exception& e)
        {
            metadata.isValid = false;
            metadata.errorMessage = std::string("JSON Parse Error: ") + e.what();
            std::cerr << "BlueprintEditor: " << metadata.errorMessage << std::endl;
        }
    }
    
    void BlueprintEditor::ParseEntityBlueprint(const json& j, AssetMetadata& metadata)
    {
        metadata.name = JsonHelper::GetString(j, "name", "Unnamed Entity");
        metadata.description = JsonHelper::GetString(j, "description", "");

        // Schema v2: Check for components in "data" wrapper
        if (j.contains("data") && j["data"].contains("components") && j["data"]["components"].is_array())
        {
            const auto& components = j["data"]["components"];
            metadata.componentCount = (int)components.size();
            
            // Extract component types
            for (size_t i = 0; i < components.size(); ++i)
            {
                const auto& comp = components[i];
                if (comp.contains("type") && comp["type"].is_string())
                {
                    std::string compType = JsonHelper::GetString(comp, "type", "Unknown");
                    metadata.components.push_back(compType);
                }
            }
        }
        // Schema v1: Check for components at top level
        else if (j.contains("components") && j["components"].is_array())
        {
            const auto& components = j["components"];
            metadata.componentCount = (int)components.size();
            
            // Extract component types
            for (size_t i = 0; i < components.size(); ++i)
            {
                const auto& comp = components[i];
                if (comp.contains("type") && comp["type"].is_string())
                {
                    std::string compType = JsonHelper::GetString(comp, "type", "Unknown");
                    metadata.components.push_back(compType);
                }
            }
        }
    }
    
    void BlueprintEditor::ParseBehaviorTree(const json& j, AssetMetadata& metadata)
    {
        metadata.name = JsonHelper::GetString(j, "name", "Unnamed Behavior Tree");
        metadata.description = "Behavior Tree AI Definition";

        // Schema v2: Check for nodes in "data" wrapper
        if (j.contains("data"))
        {
            const json& data = j["data"];
            if (data.contains("nodes") && data["nodes"].is_array())
            {
                const auto& nodes = data["nodes"];
                metadata.nodeCount = (int)nodes.size();
                
                // Extract node types
                for (size_t i = 0; i < nodes.size(); ++i)
                {
                    const auto& node = nodes[i];
                    if (node.contains("type") && node["type"].is_string())
                    {
                        std::string nodeType = JsonHelper::GetString(node, "type", "Unknown");
                        if (node.contains("name") && node["name"].is_string())
                        {
                            std::string nodeName = JsonHelper::GetString(node, "name", "");
                            metadata.nodes.push_back(nodeName + " (" + nodeType + ")");
                        }
                        else
                        {
                            metadata.nodes.push_back(nodeType);
                        }
                    }
                }
            }

            if (data.contains("rootNodeId"))
            {
                int rootId = data["rootNodeId"].get<int>();
                metadata.description += " - Root Node ID: " + std::to_string(rootId);
            }
        }
        // Schema v1: Check for nodes at top level
        else if (j.contains("nodes") && j["nodes"].is_array())
        {
            const auto& nodes = j["nodes"];
            metadata.nodeCount = (int)nodes.size();
            
            // Extract node types
            for (size_t i = 0; i < nodes.size(); ++i)
            {
                const auto& node = nodes[i];
                if (node.contains("type") && node["type"].is_string())
                {
                    std::string nodeType = JsonHelper::GetString(node, "type", "Unknown");
                    if (node.contains("name") && node["name"].is_string())
                    {
                        std::string nodeName = JsonHelper::GetString(node, "name", "");
                        metadata.nodes.push_back(nodeName + " (" + nodeType + ")");
                    }
                    else
                    {
                        metadata.nodes.push_back(nodeType);
                    }
                }
            }

            if (j.contains("rootNodeId"))
            {
                int rootId = j["rootNodeId"].get<int>();
                metadata.description += " - Root Node ID: " + std::to_string(rootId);
            }
        }
    }
    
    void BlueprintEditor::ParseHFSM(const json& j, AssetMetadata& metadata)
    {
        metadata.name = JsonHelper::GetString(j, "name", "Unnamed HFSM");
        metadata.description = "Hierarchical Finite State Machine";

        // Count states
        if (j.contains("states") && j["states"].is_array())
        {
            const auto& states = j["states"];
            metadata.nodeCount = (int)states.size();
            
            // Extract state names
            for (size_t i = 0; i < states.size(); ++i)
            {
                const auto& state = states[i];
                if (state.contains("name") && state["name"].is_string())
                {
                    std::string stateName = JsonHelper::GetString(state, "name", "");
                    std::string stateType = JsonHelper::GetString(state, "type", "State");
                    metadata.nodes.push_back(stateName + " (" + stateType + ")");
                }
            }
        }

        // Add initial state info
        if (j.contains("initialState"))
        {
            std::string initialState = JsonHelper::GetString(j, "initialState", "");
            if (!initialState.empty())
            {
                metadata.description += " - Initial State: " + initialState;
            }
        }
    }
    
    bool BlueprintEditor::IsAssetValid(const std::string& filepath) const
    {
        try
        {
            json j;
            return JsonHelper::LoadJsonFromFile(filepath, j);
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    // ========================================================================
    // B) Runtime Entity Management Implementation
    // ========================================================================
    
    void BlueprintEditor::NotifyEntityCreated(uint64_t entityId)
    {
        // Add to runtime entities list if not already present
        auto it = std::find(m_RuntimeEntities.begin(), m_RuntimeEntities.end(), entityId);
        if (it == m_RuntimeEntities.end())
        {
            m_RuntimeEntities.push_back(entityId);
            std::cout << "BlueprintEditor: Entity " << entityId << " created (total: " 
                     << m_RuntimeEntities.size() << ")" << std::endl;
        }
    }
    
    void BlueprintEditor::NotifyEntityDestroyed(uint64_t entityId)
    {
        // Remove from runtime entities list
        auto it = std::find(m_RuntimeEntities.begin(), m_RuntimeEntities.end(), entityId);
        if (it != m_RuntimeEntities.end())
        {
            m_RuntimeEntities.erase(it);
            std::cout << "BlueprintEditor: Entity " << entityId << " destroyed (total: " 
                     << m_RuntimeEntities.size() << ")" << std::endl;
            
            // If this was the selected entity, clear selection
            if (m_SelectedEntity == entityId)
            {
                m_SelectedEntity = 0; // INVALID_ENTITY_ID
            }
        }
    }

    // ========================================================================
    // C) Entity Selection Implementation
    // ========================================================================
    
    void BlueprintEditor::SetSelectedEntity(uint64_t entityId)
    {
        if (m_SelectedEntity != entityId)
        {
            m_SelectedEntity = entityId;
            std::cout << "BlueprintEditor: Selected entity " << entityId << std::endl;
            
            // All panels will automatically read this selection on next Render()
            // No explicit notification needed - reactive update pattern
        }
    }

    // ========================================================================
    // Asset Selection Implementation
    // ========================================================================
    
    void BlueprintEditor::SelectAsset(const std::string& assetPath)
    {
        if (m_SelectedAssetPath != assetPath)
        {
            m_SelectedAssetPath = assetPath;
            std::cout << "BlueprintEditor: Selected asset " << assetPath << std::endl;
        }
    }

    // ========================================================================
    // Graph Loading in Node Graph Editor
    // ========================================================================
    
    void BlueprintEditor::OpenGraphInEditor(const std::string& assetPath)
    {
        SYSTEM_LOG << "BlueprintEditor: Opening graph " << assetPath << " in editor\n";
        
        // Detect asset type
        std::string assetType = DetectAssetType(assetPath);

        // Route all graph types through TabManager
        if (assetType == "VisualScript" || assetType == "BehaviorTree" ||
            assetType == "HFSM"         || assetType == "TaskGraph"    ||
            assetType == "Generic")
        {
            std::string tabID = TabManager::Get().OpenFileInTab(assetPath);
            if (tabID.empty())
            {
                SYSTEM_LOG << "BlueprintEditor: TabManager failed to open: " << assetPath << "\n";
                m_LastError = "Failed to open graph file: " + assetPath;
            }
            return;
        }

        // Fallback: legacy BehaviorTree directly via NodeGraphManager
        if (assetType != "BehaviorTree" && assetType != "HFSM")
        {
            SYSTEM_LOG << "BlueprintEditor: Cannot open asset type '" << assetType << "'\n";
            m_LastError = "Asset type '" + assetType + "' cannot be opened in editor";
            return;
        }
        
        // Use NodeGraphManager to load the graph (legacy path)
        int graphId = NodeGraphManager::Get().LoadGraph(assetPath);
        if (graphId < 0)
        {
            SYSTEM_LOG << "BlueprintEditor: Failed to load graph from " << assetPath << "\n";
            m_LastError = "Failed to load graph file: " + assetPath;
            return;
        }
        SYSTEM_LOG << "BlueprintEditor: Graph loaded with ID " << graphId << "\n";
    }

    // ========================================================================
    // Phase 5: Template Management Implementation
    // ========================================================================
    
    bool BlueprintEditor::SaveCurrentAsTemplate(const std::string& name, 
                                                const std::string& description, 
                                                const std::string& category)
    {
        if (!HasBlueprint())
        {
            m_LastError = "No blueprint loaded to save as template";
            return false;
        }

        // Convert current blueprint to JSON
        json blueprintJson = m_CurrentBlueprint.ToJson();

        // Create template from current blueprint
        BlueprintTemplate tpl = TemplateManager::Get().CreateTemplateFromBlueprint(
            blueprintJson,
            name,
            description,
            category,
            "User"
        );

        // Save template
        if (!TemplateManager::Get().SaveTemplate(tpl))
        {
            m_LastError = "Failed to save template: " + TemplateManager::Get().GetLastError();
            return false;
        }

        std::cout << "Template saved: " << name << " (" << tpl.id << ")" << std::endl;
        return true;
    }

    bool BlueprintEditor::ApplyTemplate(const std::string& templateId)
    {
        json blueprintJson;
        
        if (!TemplateManager::Get().ApplyTemplateToBlueprint(templateId, blueprintJson))
        {
            m_LastError = "Failed to apply template: " + TemplateManager::Get().GetLastError();
            return false;
        }

        // Load the blueprint from JSON
        m_CurrentBlueprint = Blueprint::EntityBlueprint::FromJson(blueprintJson);
        m_CurrentFilepath = ""; // Clear filepath since this is a new blueprint from template
        m_HasUnsavedChanges = true;

        std::cout << "Template applied: " << templateId << std::endl;
        return true;
    }

    bool BlueprintEditor::DeleteTemplate(const std::string& templateId)
    {
        if (!TemplateManager::Get().DeleteTemplate(templateId))
        {
            m_LastError = "Failed to delete template: " + TemplateManager::Get().GetLastError();
            return false;
        }

        std::cout << "Template deleted: " << templateId << std::endl;
        return true;
    }

    void BlueprintEditor::ReloadTemplates()
    {
        TemplateManager::Get().RefreshTemplates();
        std::cout << "Templates reloaded" << std::endl;
    }

    // ========================================================================
    // Phase 6: Undo/Redo System Implementation
    // ========================================================================
    
    void BlueprintEditor::Undo()
    {
        if (m_CommandStack)
        {
            m_CommandStack->Undo();
            m_HasUnsavedChanges = true;
        }
    }

    void BlueprintEditor::Redo()
    {
        if (m_CommandStack)
        {
            m_CommandStack->Redo();
            m_HasUnsavedChanges = true;
        }
    }

    bool BlueprintEditor::CanUndo() const
    {
        return m_CommandStack && m_CommandStack->CanUndo();
    }

    bool BlueprintEditor::CanRedo() const
    {
        return m_CommandStack && m_CommandStack->CanRedo();
    }

    std::string BlueprintEditor::GetLastCommandDescription() const
    {
        if (m_CommandStack)
        {
            return m_CommandStack->GetLastCommandDescription();
        }
        return "";
    }

    std::string BlueprintEditor::GetNextRedoDescription() const
    {
        if (m_CommandStack)
        {
            return m_CommandStack->GetNextRedoDescription();
        }
        return "";
    }

    CommandStack* BlueprintEditor::GetCommandStack()
    {
        return m_CommandStack;
    }

    // ========================================================================
    // Plugin System Implementation
    // ========================================================================
    
    void BlueprintEditor::InitializePlugins()
    {
        std::cout << "BlueprintEditor: Initializing plugins..." << std::endl;
        
        // Register all plugins
        RegisterPlugin(std::make_unique<BehaviorTreeEditorPlugin>());
        RegisterPlugin(std::make_unique<HFSMEditorPlugin>());
        RegisterPlugin(std::make_unique<EntityPrefabEditorPlugin>());
        RegisterPlugin(std::make_unique<AnimationGraphEditorPlugin>());
        RegisterPlugin(std::make_unique<ScriptedEventEditorPlugin>());
        RegisterPlugin(std::make_unique<LevelDefinitionEditorPlugin>());
        RegisterPlugin(std::make_unique<UIMenuEditorPlugin>());
        
        std::cout << "BlueprintEditor: " << m_Plugins.size() << " plugins registered" << std::endl;
    }
    
    void BlueprintEditor::RegisterPlugin(std::unique_ptr<BlueprintEditorPlugin> plugin)
    {
        std::string type = plugin->GetBlueprintType();
        m_Plugins[type] = std::move(plugin);
        std::cout << "BlueprintEditor: Registered plugin: " << type << std::endl;
    }
    
    BlueprintEditorPlugin* BlueprintEditor::GetPlugin(const std::string& type)
    {
        auto it = m_Plugins.find(type);
        if (it != m_Plugins.end())
        {
            return it->second.get();
        }
        return nullptr;
    }
    
    BlueprintEditorPlugin* BlueprintEditor::DetectPlugin(const json& blueprint)
    {
        // V2 format: read blueprintType directly
        if (blueprint.contains("blueprintType"))
        {
            std::string type = blueprint["blueprintType"].get<std::string>();
            return GetPlugin(type);
        }
        
        // V1 format: use heuristic detection
        for (auto& pluginPair : m_Plugins)
        {
            auto& type = pluginPair.first;
            auto& plugin = pluginPair.second;
            if (plugin->CanHandle(blueprint))
            {
                return plugin.get();
            }
        }
        
        return nullptr;
    }

    // ========================================================================
    // Migration System Implementation
    // ========================================================================
    
    std::vector<std::string> BlueprintEditor::ScanBlueprintFiles(const std::string& directory)
    {
        std::vector<std::string> blueprintFiles;
        
        try
        {
            if (!fs::exists(directory) || !fs::is_directory(directory))
            {
                std::cerr << "BlueprintEditor: Directory not found: " << directory << std::endl;
                return blueprintFiles;
            }
            
            for (const auto& entry : fs::recursive_directory_iterator(directory))
            {
                if (fs::is_regular_file(entry.path()) && entry.path().extension() == ".json")
                {
                    blueprintFiles.push_back(entry.path().string());
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "BlueprintEditor: Error scanning directory: " << e.what() << std::endl;
        }
        
        return blueprintFiles;
    }
    
    void BlueprintEditor::MigrateAllBlueprints()
    {
        std::cout << "BlueprintEditor: Starting migration..." << std::endl;
        
        BlueprintMigrator v1Migrator;
        SubgraphMigrator  sgMigrator;
        int successCount = 0;
        int failCount = 0;
        int skippedCount = 0;
        
        std::vector<std::string> files = ScanBlueprintFiles(m_AssetRootPath);
        
        for (const auto& path : files)
        {
            try
            {
                json blueprint;
                if (!JsonHelper::LoadJsonFromFile(path, blueprint))
                {
                    std::cerr << "Failed to load: " << path << std::endl;
                    failCount++;
                    continue;
                }

                bool needsV2Migration = !v1Migrator.IsV2(blueprint);
                bool needsSGMigration = sgMigrator.NeedsMigration(
                    needsV2Migration ? v1Migrator.MigrateToV2(blueprint) : blueprint);

                if (!needsV2Migration && !needsSGMigration)
                {
                    std::cout << "Skipping (already current format): " << path << std::endl;
                    skippedCount++;
                    continue;
                }
                
                // Create backup before any write.
                std::string backupPath = path + ".pre_v5.backup";
                try
                {
                    fs::copy_file(path, backupPath, fs::copy_options::overwrite_existing);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to create backup for " << path << ": " << e.what() << std::endl;
                    failCount++;
                    continue;
                }
                
                // Step 1: v1 → v2 if needed.
                if (needsV2Migration)
                {
                    blueprint = v1Migrator.MigrateToV2(blueprint);
                    std::cout << "[Migration] v1→v2: " << path << std::endl;
                }

                // Step 2: legacy v2 → Phase 8 flat-dictionary format if needed.
                if (sgMigrator.NeedsMigration(blueprint))
                {
                    blueprint = sgMigrator.Migrate(blueprint);
                    std::cout << "[Migration] v2→v5 (subgraph): " << path << std::endl;
                }

                // Save final result.
                std::ofstream file(path);
                if (!file.is_open())
                {
                    std::cerr << "Failed to open file for writing: " << path << std::endl;
                    failCount++;
                    continue;
                }
                
                file << blueprint.dump(2);
                file.close();
                
                std::cout << "Migrated: " << path << std::endl;
                successCount++;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Migration failed for " << path << ": " << e.what() << std::endl;
                failCount++;
            }
        }
        
        std::cout << "Migration complete: " << successCount << " success, " 
                  << skippedCount << " skipped, " << failCount << " failed" << std::endl;

        // Refresh assets after migration
        RefreshAssets();
    }

    // ========================================================================
    // Configuration System Implementation
    // ========================================================================

    bool BlueprintEditor::LoadConfig(const std::string& configPath)
    {
        try
        {
            std::ifstream file(configPath);
            if (!file.is_open())
            {
                std::cout << "[BlueprintEditor] Config file not found: " << configPath 
                          << ", creating default config" << std::endl;

                // Create default config
                m_Config = json::object();
                m_Config["version"] = "1.0";
                m_Config["editor_mode"] = "standalone";

                m_Config["window"] = json::object();
                m_Config["window"]["width"] = 1920;
                m_Config["window"]["height"] = 1080;
                m_Config["window"]["maximized"] = true;

                m_Config["panels"] = json::object();
                m_Config["panels"]["asset_browser"] = json::object();
                m_Config["panels"]["asset_browser"]["visible"] = true;
                m_Config["panels"]["asset_browser"]["width"] = 400;
                m_Config["panels"]["node_graph"] = json::object();
                m_Config["panels"]["node_graph"]["visible"] = true;
                m_Config["panels"]["inspector"] = json::object();
                m_Config["panels"]["inspector"]["visible"] = true;
                m_Config["panels"]["inspector"]["width"] = 400;

                m_Config["layout"] = json::object();
                m_Config["layout"]["mode"] = "fixed";
                m_Config["layout"]["asset_browser_width"] = 400.0f;
                m_Config["layout"]["inspector_width"] = 400.0f;
                m_Config["layout"]["min_panel_width"] = 200.0f;
                m_Config["layout"]["splitter_size"] = 8.0f;

                SaveConfig(configPath);
                return true;
            }

            file >> m_Config;
            file.close();

            std::cout << "[BlueprintEditor] Loaded config from: " << configPath << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[BlueprintEditor] Error loading config: " << e.what() << std::endl;
            return false;
        }
    }

    bool BlueprintEditor::SaveConfig(const std::string& configPath)
    {
        try
        {
            std::ofstream file(configPath);
            if (!file.is_open())
            {
                std::cerr << "[BlueprintEditor] Failed to open config file for writing: " 
                          << configPath << std::endl;
                return false;
            }

            file << m_Config.dump(2);  // Pretty print with 2-space indent
            file.close();

            std::cout << "[BlueprintEditor] Saved config to: " << configPath << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[BlueprintEditor] Error saving config: " << e.what() << std::endl;
            return false;
        }
    }

} // namespace Olympe
