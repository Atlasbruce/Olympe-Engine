/*
 * Olympe Blueprint Editor - Backend (Business Logic)
 * 
 * Singleton backend for the Blueprint Editor managing:
 * - Editor state and lifecycle
 * - Asset management and current blueprint data
 * - Business logic and data models
 * - NO UI/GUI code (separated into BlueprintEditorGUI)
 * 
 * ╔════════════════════════════════════════════════════════════════════════════╗
 * ║                       DOCUMENTATION INDEX                                  ║
 * ╚════════════════════════════════════════════════════════════════════════════╝
 * 
 * Complete documentation available in Docs/ directory:
 * 
 * 📚 PRIMARY GUIDES:
 *    • Docs/Blueprint_Editor_User_Guide_v4.md
 *      -> Step-by-step workflows, CRUD operations, debugging (20 min read)
 * 
 *    • Docs/Blueprint_Editor_Advanced_Systems.md
 *      -> SubGraphs, Profiler, Templates, Validation (45 min read)
 * 
 *    • Docs/Blueprint_Editor_Visual_Diagrams.md
 *      -> Interactive Mermaid flowcharts (15 min read)
 * 
 *    • Docs/README_Documentation_Index.md
 *      -> Master index with learning paths and topic search
 * 
 * ⚠️  DEPRECATED (DO NOT USE):
 *    • Docs/Blueprint Editor Features.md
 *      -> Legacy BT v2 documentation (replaced by v4 guides above)
 * 
 * ╔════════════════════════════════════════════════════════════════════════════╗
 * ║                       CURRENT SYSTEM (v4)                                  ║
 * ╚════════════════════════════════════════════════════════════════════════════╝
 * 
 * EDITOR:           VisualScriptEditorPanel (Phase 5)
 * FILE FORMAT:      .ats or .json with schema_version: 4
 * GRAPH TYPE:       "VisualScript" (ATS Visual Script)
 * LOADER:           TaskGraphLoader::ParseSchemaV4() (primary path)
 * EXECUTOR:         VSGraphExecutor::ExecuteFrame()
 * 
 * NODE TYPES:       EntryPoint, Branch, Sequence, While, AtomicTask,
 *                   GetBBValue, SetBBValue, MathOp, SubGraph, DoOnce, Delay
 * 
 * CONNECTIONS:      ExecConnections (flow control) + DataConnections (typed data)
 * 
 * ╔════════════════════════════════════════════════════════════════════════════╗
 * ║                       ADVANCED FEATURES                                    ║
 * ╚════════════════════════════════════════════════════════════════════════════╝
 * 
 * SUBGRAPHS (Phase 8):
 *    • Modular graph composition (depth-limited to 4)
 *    • Cycle detection via SubGraphCallStack
 *    • Schema v5 format (rootGraph + subgraphs dictionary)
 *    • Tab-based navigation in editor
 *    -> Docs: Docs/Blueprint_Editor_Advanced_Systems.md -> "SubGraph System"
 * 
 * PERFORMANCE PROFILER (Phase 5):
 *    • Per-node execution metrics (avg/max/total time)
 *    • Frame timeline (last 60 frames)
 *    • Hotspot table sorted by average time
 *    • CSV export for external analysis
 *    -> Docs: Docs/Blueprint_Editor_Advanced_Systems.md -> "Performance Profiler"
 * 
 * DEBUG SYSTEM (Phase 5):
 *    • Breakpoints (F9 toggle, runtime check)
 *    • Step controls (F5 Continue, F10 Step Over, F11 Step Into)
 *    • Call stack inspection
 *    • Live Blackboard variable watch
 *    -> Docs: Docs/Blueprint_Editor_Advanced_Systems.md -> "Debug System"
 * 
 * BLACKBOARD SYSTEM (Phase 2.1):
 *    • Variable types: Int, Float, Bool, String, Vector3
 *    • Scoping: local: (per-entity) / global: (world-shared)
 *    • GetBBValue / SetBBValue nodes for read/write
 *    -> Docs: Docs/Blueprint_Editor_User_Guide_v4.md -> Section 6
 * 
 * TEMPLATE MANAGER (Phase 5):
 *    • Save graphs as reusable templates
 *    • Template catalog with categorization (AI, Character, Enemy)
 *    • Apply templates to current graph
 *    -> Docs: Docs/Blueprint_Editor_Advanced_Systems.md -> "Template Manager"
 * 
 * VALIDATION SYSTEM:
 *    • Real-time error detection (cycles, type mismatches, missing properties)
 *    • Severity levels: Critical, Error, Warning, Info
 *    • Click errors to navigate to affected nodes
 *    -> Docs: Docs/Blueprint_Editor_Advanced_Systems.md -> "Validation System"
 * 
 * COMMAND SYSTEM (Undo/Redo):
 *    • Full undo/redo support (Ctrl+Z / Ctrl+Y)
 *    • Command pattern with CreateNode, DeleteNode, CreateLink
 *    • Composite commands for batch operations
 *    -> Docs: Docs/Blueprint_Editor_Advanced_Systems.md -> "Command System"
 * 
 * ╔════════════════════════════════════════════════════════════════════════════╗
 * ║                       QUICK START                                          ║
 * ╚════════════════════════════════════════════════════════════════════════════╝
 * 
 * 1. View -> VS Graph Editor -> New Graph
 * 2. Right-click -> Flow Control -> EntryPoint
 * 3. Right-click -> Flow Control -> Branch
 * 4. Drag from "Out" pin to "In" pin
 * 5. Select Branch node -> Properties -> Set "ConditionKey" = "local:HasEnemy"
 * 6. View -> Blackboard -> Add Variable: "HasEnemy" (Bool, default: false)
 * 7. Right-click -> Tasks -> AtomicTask
 * 8. Ctrl+S to save as "my_ai.ats"
 * 
 * -> Full tutorial: Docs/Blueprint_Editor_User_Guide_v4.md -> Section 10
 */

#pragma once

#include "EntityBlueprint.h"
#include "BlueprintEditorPlugin.h"
#include "../../Source/third_party/nlohmann/json.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace Olympe
{
    // Forward declaration
    class BlueprintEditorGUI;
    
    // Forward declare Blueprint namespace types
    namespace Blueprint
    {
        class CommandStack;
    }
    
    // Use nlohmann json
    using json = nlohmann::json;

    // Asset metadata structure for backend
    struct AssetMetadata
    {
        std::string filepath;       // Full path to asset file
        std::string name;           // Asset name (from JSON or filename)
        std::string type;           // Asset type (EntityBlueprint, BehaviorTree, etc.)
        std::string description;    // Asset description
        bool isDirectory;           // True if this is a directory
        int componentCount;         // For EntityBlueprint
        int nodeCount;              // For BehaviorTree
        std::vector<std::string> components;  // Component types
        std::vector<std::string> nodes;       // Node types
        bool isValid;               // False if JSON is malformed
        std::string errorMessage;   // Error message if not valid
        
        AssetMetadata() : isDirectory(false), componentCount(0), nodeCount(0), isValid(false) {}
    };

    // Asset tree node structure for backend
    struct AssetNode
    {
        std::string name;           // Display name (filename without path)
        std::string fullPath;       // Complete file path
        std::string type;           // Asset type
        bool isDirectory;
        std::vector<std::shared_ptr<AssetNode>> children;
        
        AssetNode(const std::string& n, const std::string& path, bool isDir)
            : name(n), fullPath(path), isDirectory(isDir) {}
    };

    /**
     * BlueprintEditor Singleton Backend
     * Manages all business logic, state, and data for the Blueprint Editor
     * Completely separated from UI rendering (handled by BlueprintEditorGUI)
     */
    class BlueprintEditor
    {
    public:
        // Singleton access
        static BlueprintEditor& Instance();
        static BlueprintEditor& Get() { return Instance(); }

        // Lifecycle methods
        void Initialize();
        void Shutdown();
        void Update(float deltaTime);  // Called by GameEngine when active
        
        // Editor mode initialization
        void InitializeRuntimeEditor();    // Initialize in Runtime mode (read-only)
        void InitializeStandaloneEditor(); // Initialize in Standalone mode (full CRUD)

        // Editor state
        bool IsActive() const { return m_IsActive; }
        void ToggleActive() { m_IsActive = !m_IsActive; }
        void SetActive(bool active) { m_IsActive = active; }

        // Blueprint operations
        void NewBlueprint(const std::string& name, const std::string& description = "");
        bool LoadBlueprint(const std::string& filepath);
        bool SaveBlueprint();
        bool SaveBlueprintAs(const std::string& filepath);
        
        // Blueprint access (const for read-only, non-const for editing)
        const Blueprint::EntityBlueprint& GetCurrentBlueprint() const { return m_CurrentBlueprint; }
        Blueprint::EntityBlueprint& GetCurrentBlueprintMutable() { return m_CurrentBlueprint; }
        
        // State queries
        bool HasBlueprint() const { return !m_CurrentBlueprint.name.empty(); }
        bool HasUnsavedChanges() const { return m_HasUnsavedChanges; }
        const std::string& GetCurrentFilepath() const { return m_CurrentFilepath; }
        
        // State modification
        void MarkAsModified() { m_HasUnsavedChanges = true; }
        void ClearModified() { m_HasUnsavedChanges = false; }
        
        // Asset management
        std::string GetAssetRootPath() const { return m_AssetRootPath; }
        void SetAssetRootPath(const std::string& path);
        
        // Asset scanning and retrieval
        void RefreshAssets();  // Rescan asset directories (Blueprints + Gamedata)
        std::shared_ptr<AssetNode> GetAssetTree() const { return m_AssetTreeRoot; }
        
        // Pre-load all ATS graphs found under Blueprints/ and Gamedata/ into
        // TaskGraphLoader. Called automatically by InitializeStandaloneEditor().
        void PreloadATSGraphs();
        
        // Asset queries
        std::vector<AssetMetadata> GetAllAssets() const;
        std::vector<AssetMetadata> GetAssetsByType(const std::string& type) const;
        std::vector<AssetMetadata> SearchAssets(const std::string& query) const;
        
        // Asset metadata
        AssetMetadata GetAssetMetadata(const std::string& filepath);
        bool IsAssetValid(const std::string& filepath) const;

        // Asset metadata cache invalidation
        void InvalidateAssetMetadataCache();  // Clear all cached metadata

        // Asset type detection
        std::string DetectAssetType(const std::string& filepath);
        
        // Error handling
        std::string GetLastError() const { return m_LastError; }
        bool HasError() const { return !m_LastError.empty(); }
        void ClearError() { m_LastError.clear(); }
        
        // ===== B) Runtime Entity Management =====
        // World bridge: notification hooks for entity lifecycle
        void NotifyEntityCreated(uint64_t entityId);
        void NotifyEntityDestroyed(uint64_t entityId);
        
        // Runtime entity queries
        const std::vector<uint64_t>& GetRuntimeEntities() const { return m_RuntimeEntities; }
        size_t GetRuntimeEntityCount() const { return m_RuntimeEntities.size(); }
        
        // ===== C) Entity Selection for Panel Synchronization =====
        void SetSelectedEntity(uint64_t entityId);
        uint64_t GetSelectedEntity() const { return m_SelectedEntity; }
        bool HasSelectedEntity() const { return m_SelectedEntity != 0; } // 0 = INVALID_ENTITY_ID
        
        // ===== Asset Selection for Panel Synchronization =====
        void SelectAsset(const std::string& assetPath);
        std::string GetSelectedAssetPath() const { return m_SelectedAssetPath; }
        bool HasSelectedAsset() const { return !m_SelectedAssetPath.empty(); }
        
        // ===== Graph Loading in Node Graph Editor =====
        // Opens a BehaviorTree or HFSM asset in the Node Graph Editor
        void OpenGraphInEditor(const std::string& assetPath);
        
        // ===== Phase 5: Template Management =====
        // Save current blueprint as template
        bool SaveCurrentAsTemplate(const std::string& name, const std::string& description, const std::string& category);
        
        // Apply template to current blueprint
        bool ApplyTemplate(const std::string& templateId);
        
        // Delete a template
        bool DeleteTemplate(const std::string& templateId);
        
        // Reload templates from disk
        void ReloadTemplates();
        
        // ===== Phase 6: Undo/Redo System =====
        void Undo();
        void Redo();
        bool CanUndo() const;
        bool CanRedo() const;
        std::string GetLastCommandDescription() const;
        std::string GetNextRedoDescription() const;
        
        // Command stack access for history panel
        Blueprint::CommandStack* GetCommandStack();
        
        // ===== Plugin System =====
        void InitializePlugins();
        void RegisterPlugin(std::unique_ptr<class BlueprintEditorPlugin> plugin);
        class BlueprintEditorPlugin* GetPlugin(const std::string& type);
        class BlueprintEditorPlugin* DetectPlugin(const json& blueprint);
        
        // ===== Migration System =====
        void MigrateAllBlueprints();
        std::vector<std::string> ScanBlueprintFiles(const std::string& directory);
        bool ShowMigrationDialog() const { return m_ShowMigrationDialog; }
        void SetShowMigrationDialog(bool show) { m_ShowMigrationDialog = show; }

        // ===== Configuration System =====
        bool LoadConfig(const std::string& configPath = "blueprint_editor_config.json");
        bool SaveConfig(const std::string& configPath = "blueprint_editor_config.json");
        const json& GetConfig() const { return m_Config; }
        json& GetConfigMutable() { return m_Config; }

    private:
        // Private constructor/destructor for singleton
        BlueprintEditor();
        ~BlueprintEditor();
        
        // Disable copy and assignment
        BlueprintEditor(const BlueprintEditor&) = delete;
        BlueprintEditor& operator=(const BlueprintEditor&) = delete;
        
        // Asset management helpers
        std::shared_ptr<AssetNode> ScanDirectory(const std::string& path);
        void ParseAssetMetadata(const std::string& filepath, AssetMetadata& metadata);
        void ParseEntityBlueprint(const json& j, AssetMetadata& metadata);
        void ParseBehaviorTree(const json& j, AssetMetadata& metadata);
        void ParseHFSM(const json& j, AssetMetadata& metadata);
        void CollectAllAssets(const std::shared_ptr<AssetNode>& node, std::vector<AssetMetadata>& assets) const;

    private:
        // Editor state
        bool m_IsActive;
        bool m_HasUnsavedChanges;
        
        // Blueprint data
        Blueprint::EntityBlueprint m_CurrentBlueprint;
        std::string m_CurrentFilepath;
        
        // Asset paths and tree
        std::string m_AssetRootPath;     // Blueprints directory (default: "Blueprints")
        std::string m_GamedataRootPath;  // Gamedata directory (default: "Gamedata")
        std::shared_ptr<AssetNode> m_AssetTreeRoot;
        
        // Error handling
        std::string m_LastError;
        
        // ===== B) Runtime Entity Tracking =====
        std::vector<uint64_t> m_RuntimeEntities;  // List of all runtime entities from World
        
        // ===== C) Entity Selection =====
        uint64_t m_SelectedEntity; // Currently selected entity (0 = none)
        
        // ===== Asset Selection =====
        std::string m_SelectedAssetPath; // Currently selected asset file path

        // ===== Asset Metadata Cache (Performance Optimization) =====
        // Cache to avoid reloading and reparsing JSON for the same asset every frame
        std::map<std::string, AssetMetadata> m_AssetMetadataCache;  // filepath -> metadata
        std::string m_LastCachedAssetPath;  // Track previous selection for cache invalidation

        // ===== Phase 6: Command System =====
        Blueprint::CommandStack* m_CommandStack;  // Undo/redo command stack

        // ===== Plugin System =====
        std::map<std::string, std::unique_ptr<class BlueprintEditorPlugin>> m_Plugins;

        // ===== Migration System =====
        bool m_ShowMigrationDialog;
        std::vector<std::string> m_BlueprintsToMigrate;

        // ===== Configuration System =====
        json m_Config;  // Editor configuration (loaded from JSON)
    };
}
