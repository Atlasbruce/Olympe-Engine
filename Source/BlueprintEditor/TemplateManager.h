/*
 * Olympe Blueprint Editor - Template Manager
 * 
 * Manages blueprint templates for reusability and productivity
 * Provides template CRUD operations, categorization, and persistence
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe
{
    using json = nlohmann::json;

    /**
     * BlueprintTemplate - Template metadata and data
     * Stores a complete blueprint that can be reused as a template
     */
    struct BlueprintTemplate
    {
        std::string id;                  // UUID unique identifier
        std::string name;                // Template display name
        std::string description;         // User description
        std::string category;            // Category (AI, Character, Enemy, etc.)
        std::string author;              // Creator name
        std::string version;             // Template version
        json blueprintData;              // Complete blueprint JSON data
        std::string thumbnailPath;       // Optional preview image path
        time_t createdDate;              // Creation timestamp
        time_t modifiedDate;             // Last modification timestamp

        BlueprintTemplate();
        
        // Serialization
        json ToJson() const;
        static BlueprintTemplate FromJson(const json& j);
        
        // File I/O
        bool SaveToFile(const std::string& filepath) const;
        static BlueprintTemplate LoadFromFile(const std::string& filepath);
    };

    /**
     * TemplateManager - Manages blueprint templates
     * Singleton manager for template catalog and operations
     */
    class TemplateManager
    {
    public:
        static TemplateManager& Instance();
        static TemplateManager& Get() { return Instance(); }

        // Lifecycle
        void Initialize(const std::string& templatesPath = "Blueprints/Templates");
        void Shutdown();

        // Template catalog management
        bool LoadTemplates(const std::string& templatesPath);
        bool SaveTemplate(const BlueprintTemplate& tpl);
        bool DeleteTemplate(const std::string& templateId);
        bool RefreshTemplates(); // Rescan template directory

        // Template access
        const std::vector<BlueprintTemplate>& GetAllTemplates() const { return m_Templates; }
        const BlueprintTemplate* FindTemplate(const std::string& id) const;
        std::vector<BlueprintTemplate> GetTemplatesByCategory(const std::string& category) const;
        std::vector<std::string> GetAllCategories() const;

        // Template application
        bool ApplyTemplateToBlueprint(const std::string& templateId, json& targetBlueprint);

        // Template creation from existing blueprint
        BlueprintTemplate CreateTemplateFromBlueprint(
            const json& blueprint,
            const std::string& name,
            const std::string& description,
            const std::string& category,
            const std::string& author = "User"
        );

        // State queries
        bool IsInitialized() const { return m_Initialized; }
        const std::string& GetTemplatesPath() const { return m_TemplatesPath; }
        std::string GetLastError() const { return m_LastError; }
        bool HasError() const { return !m_LastError.empty(); }
        void ClearError() { m_LastError.clear(); }

    private:
        // Private constructor for singleton
        TemplateManager();
        ~TemplateManager();
        
        // Disable copy and assignment
        TemplateManager(const TemplateManager&) = delete;
        TemplateManager& operator=(const TemplateManager&) = delete;

        // Helper methods
        std::string GenerateUUID() const;
        void ScanTemplateDirectory();
        bool EnsureDirectoryExists(const std::string& path);

    private:
        bool m_Initialized;
        std::string m_TemplatesPath;
        std::vector<BlueprintTemplate> m_Templates;
        std::string m_LastError;
    };
}
