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
#include <string>
#include <memory>

namespace Olympe
{
    // Forward declaration
    class BlueprintEditorGUI;

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
        void SetAssetRootPath(const std::string& path) { m_AssetRootPath = path; }

    private:
        // Private constructor/destructor for singleton
        BlueprintEditor();
        ~BlueprintEditor();
        
        // Disable copy and assignment
        BlueprintEditor(const BlueprintEditor&) = delete;
        BlueprintEditor& operator=(const BlueprintEditor&) = delete;

    private:
        // Editor state
        bool m_IsActive;
        bool m_HasUnsavedChanges;
        
        // Blueprint data
        Blueprint::EntityBlueprint m_CurrentBlueprint;
        std::string m_CurrentFilepath;
        
        // Asset paths
        std::string m_AssetRootPath;
    };
}
