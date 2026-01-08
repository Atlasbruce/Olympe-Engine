/*
 * Olympe Blueprint Editor - Backend Implementation
 * 
 * Singleton backend managing business logic, state, and data
 * Completely separated from UI rendering (handled by BlueprintEditorGUI)
 */

#include "BlueprintEditor.h"
#include "EntityBlueprint.h"
#include "EnumCatalogManager.h"
#include "NodeGraphManager.h"
#include "EntityInspectorManager.h"
#include "TemplateManager.h"
#include "CommandSystem.h"
#include "BlueprintMigrator.h"
#include "BehaviorTreeEditorPlugin.h"
#include "EntityPrefabEditorPlugin.h"
#include "AdditionalEditorPlugins.h"
#include "../json_helper.h"
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
        m_CommandStack = new CommandStack();
        
        // Initialize plugin system
        InitializePlugins();
        
        // Scan assets on initialization
        RefreshAssets();
    }

    void BlueprintEditor::Shutdown()
    {
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
        
        std::cout << "BlueprintEditor: Scanning assets directory: " << m_AssetRootPath << std::endl;
        
        try
        {
            if (fs::exists(m_AssetRootPath) && fs::is_directory(m_AssetRootPath))
            {
                m_AssetTreeRoot = ScanDirectory(m_AssetRootPath);
                std::cout << "BlueprintEditor: Asset scan complete" << std::endl;
            }
            else
            {
                m_LastError = "Asset directory not found: " + m_AssetRootPath;
                std::cerr << "BlueprintEditor: " << m_LastError << std::endl;
                m_AssetTreeRoot = nullptr;
            }
        }
        catch (const std::exception& e)
        {
            m_LastError = std::string("Error scanning assets: ") + e.what();
            std::cerr << "BlueprintEditor: " << m_LastError << std::endl;
            m_AssetTreeRoot = nullptr;
        }
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
                    // Check if it's a JSON file
                    if (entry.path().extension() == ".json")
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
            json j;
            if (!JsonHelper::LoadJsonFromFile(filepath, j))
                return "Unknown";

            // Check for common type indicators
            if (j.contains("type"))
            {
                std::string type = j["type"].get<std::string>();
                if (type == "EntityBlueprint")
                    return "EntityBlueprint";
                if (type == "BehaviorTree")
                    return "BehaviorTree";
                if (type == "HFSM")
                    return "HFSM";
            }

            // Check for behavior tree structure (rootNodeId + nodes)
            if (j.contains("rootNodeId") && j.contains("nodes"))
                return "BehaviorTree";

            // Check for HFSM structure (states + transitions or initialState)
            if (j.contains("states") || j.contains("initialState"))
                return "HFSM";

            // Check for components (entity blueprint without explicit type)
            if (j.contains("components"))
                return "EntityBlueprint";

            return "Generic";
        }
        catch (const std::exception&)
        {
            return "Unknown";
        }
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
            
            // Determine asset type and parse accordingly
            if (j.contains("type"))
            {
                std::string type = JsonHelper::GetString(j, "type", "");
                metadata.type = type;
                
                if (type == "EntityBlueprint")
                {
                    ParseEntityBlueprint(j, metadata);
                }
                else
                {
                    metadata.name = JsonHelper::GetString(j, "name", metadata.name);
                    metadata.description = JsonHelper::GetString(j, "description", "");
                }
            }
            else if (j.contains("rootNodeId") && j.contains("nodes"))
            {
                // Behavior Tree
                metadata.type = "BehaviorTree";
                ParseBehaviorTree(j, metadata);
            }
            else if (j.contains("states") || j.contains("initialState"))
            {
                // HFSM (Hierarchical Finite State Machine)
                metadata.type = "HFSM";
                ParseHFSM(j, metadata);
            }
            else if (j.contains("components"))
            {
                // Entity Blueprint without explicit type
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

        if (j.contains("components") && j["components"].is_array())
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

        if (j.contains("nodes") && j["nodes"].is_array())
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
        }

        if (j.contains("rootNodeId"))
        {
            int rootId = j["rootNodeId"].get<int>();
            metadata.description += " - Root Node ID: " + std::to_string(rootId);
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
        std::cout << "BlueprintEditor: Opening graph " << assetPath << " in Node Graph Editor" << std::endl;
        
        // Detect asset type
        std::string assetType = DetectAssetType(assetPath);
        
        // Only open BehaviorTree and HFSM types
        if (assetType != "BehaviorTree" && assetType != "HFSM")
        {
            std::cerr << "BlueprintEditor: Cannot open asset type '" << assetType 
                     << "' in Node Graph Editor (only BehaviorTree and HFSM supported)" << std::endl;
            m_LastError = "Asset type '" + assetType + "' cannot be opened in Node Graph Editor";
            return;
        }
        
        // Use NodeGraphManager to load the graph
        int graphId = NodeGraphManager::Get().LoadGraph(assetPath);
        
        if (graphId < 0)
        {
            std::cerr << "BlueprintEditor: Failed to load graph from " << assetPath << std::endl;
            m_LastError = "Failed to load graph file: " + assetPath;
            return;
        }
        
        // Graph is now loaded and active in NodeGraphManager
        std::cout << "BlueprintEditor: Graph loaded with ID " << graphId << std::endl;
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
        for (auto& [type, plugin] : m_Plugins)
        {
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
        
        BlueprintMigrator migrator;
        int successCount = 0;
        int failCount = 0;
        int skippedCount = 0;
        
        std::vector<std::string> files = ScanBlueprintFiles(m_AssetRootPath);
        
        for (const auto& path : files)
        {
            try
            {
                json v1;
                if (!JsonHelper::LoadJsonFromFile(path, v1))
                {
                    std::cerr << "Failed to load: " << path << std::endl;
                    failCount++;
                    continue;
                }
                
                // Check if already v2
                if (migrator.IsV2(v1))
                {
                    std::cout << "Skipping (already v2): " << path << std::endl;
                    skippedCount++;
                    continue;
                }
                
                // Create backup
                std::string backupPath = path + ".v1.backup";
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
                
                // Migrate
                json v2 = migrator.MigrateToV2(v1);
                
                // Save
                std::ofstream file(path);
                if (!file.is_open())
                {
                    std::cerr << "Failed to open file for writing: " << path << std::endl;
                    failCount++;
                    continue;
                }
                
                file << v2.dump(2);
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

} // namespace Olympe
