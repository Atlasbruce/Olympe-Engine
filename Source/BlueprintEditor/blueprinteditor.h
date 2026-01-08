/*
 * Olympe Blueprint Editor - Backend (Business Logic)
 * 
 * Singleton backend for the Blueprint Editor managing:
 * - Editor state and lifecycle
 * - Asset management and current blueprint data
 * - Business logic and data models
 * - NO UI/GUI code (separated into BlueprintEditorGUI)
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
        void RefreshAssets();  // Rescan asset directory
        std::shared_ptr<AssetNode> GetAssetTree() const { return m_AssetTreeRoot; }
        
        // Asset queries
        std::vector<AssetMetadata> GetAllAssets() const;
        std::vector<AssetMetadata> GetAssetsByType(const std::string& type) const;
        std::vector<AssetMetadata> SearchAssets(const std::string& query) const;
        
        // Asset metadata
        AssetMetadata GetAssetMetadata(const std::string& filepath);
        bool IsAssetValid(const std::string& filepath) const;
        
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
        class CommandStack* GetCommandStack();
        
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
        std::string m_AssetRootPath;
        std::shared_ptr<AssetNode> m_AssetTreeRoot;
        
        // Error handling
        std::string m_LastError;
        
        // ===== B) Runtime Entity Tracking =====
        std::vector<uint64_t> m_RuntimeEntities;  // List of all runtime entities from World
        
        // ===== C) Entity Selection =====
        uint64_t m_SelectedEntity; // Currently selected entity (0 = none)
        
        // ===== Asset Selection =====
        std::string m_SelectedAssetPath; // Currently selected asset file path
        
        // ===== Phase 6: Command System =====
        class CommandStack* m_CommandStack;  // Undo/redo command stack
        
        // ===== Plugin System =====
        std::map<std::string, std::unique_ptr<class BlueprintEditorPlugin>> m_Plugins;
        
        // ===== Migration System =====
        bool m_ShowMigrationDialog;
        std::vector<std::string> m_BlueprintsToMigrate;
    };
}
