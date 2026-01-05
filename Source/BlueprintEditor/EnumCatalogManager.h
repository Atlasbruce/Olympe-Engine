/*
 * Olympe Blueprint Editor - Enum Catalog Manager
 * 
 * Manages dynamic enum catalogs for Actions, Conditions, and Decorators
 * Loads type definitions from JSON files and provides validation/query API
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe
{
    // Parameter definition for catalog types
    struct CatalogParameter
    {
        std::string name;
        std::string type;       // "string", "float", "int", "bool", "array"
        bool required = false;
        std::string defaultValue;
        
        CatalogParameter() = default;
        CatalogParameter(const std::string& n, const std::string& t, bool req = false, const std::string& def = "")
            : name(n), type(t), required(req), defaultValue(def) {}
    };

    // Type definition from catalog
    struct CatalogType
    {
        std::string id;
        std::string name;
        std::string description;
        std::string category;
        std::string tooltip;
        std::vector<CatalogParameter> parameters;
        
        CatalogType() = default;
    };

    // Catalog for a specific enum type (Actions, Conditions, Decorators)
    struct EnumCatalog
    {
        std::string version;
        std::string catalogType;
        std::vector<CatalogType> types;
        std::string filepath;       // Source file for hot-reload
        
        EnumCatalog() = default;
        
        // Find type by ID
        const CatalogType* FindType(const std::string& id) const
        {
            for (const auto& type : types)
            {
                if (type.id == id)
                    return &type;
            }
            return nullptr;
        }
        
        // Get all type IDs
        std::vector<std::string> GetTypeIds() const
        {
            std::vector<std::string> ids;
            for (const auto& type : types)
                ids.push_back(type.id);
            return ids;
        }
        
        // Get all type names (for UI display)
        std::vector<std::string> GetTypeNames() const
        {
            std::vector<std::string> names;
            for (const auto& type : types)
                names.push_back(type.name);
            return names;
        }
    };

    /**
     * EnumCatalogManager - Singleton manager for enum catalogs
     * Loads and manages dynamic type definitions for the node editor
     */
    class EnumCatalogManager
    {
    public:
        // Singleton access
        static EnumCatalogManager& Instance();
        static EnumCatalogManager& Get() { return Instance(); }

        // Lifecycle
        void Initialize();
        void Shutdown();

        // Catalog loading
        bool LoadCatalogs(const std::string& catalogsPath);
        bool LoadCatalog(const std::string& filepath);
        bool ReloadCatalogs();  // Hot-reload support

        // Catalog access
        const EnumCatalog* GetActionCatalog() const { return m_ActionCatalog.get(); }
        const EnumCatalog* GetConditionCatalog() const { return m_ConditionCatalog.get(); }
        const EnumCatalog* GetDecoratorCatalog() const { return m_DecoratorCatalog.get(); }

        // Type queries
        std::vector<std::string> GetActionTypes() const;
        std::vector<std::string> GetConditionTypes() const;
        std::vector<std::string> GetDecoratorTypes() const;

        // Type lookup
        const CatalogType* FindActionType(const std::string& id) const;
        const CatalogType* FindConditionType(const std::string& id) const;
        const CatalogType* FindDecoratorType(const std::string& id) const;

        // Validation
        bool IsValidActionType(const std::string& id) const;
        bool IsValidConditionType(const std::string& id) const;
        bool IsValidDecoratorType(const std::string& id) const;

        // State
        bool IsInitialized() const { return m_Initialized; }
        std::string GetLastError() const { return m_LastError; }
        std::string GetCatalogsPath() const { return m_CatalogsPath; }

    private:
        // Private constructor for singleton
        EnumCatalogManager();
        ~EnumCatalogManager();
        
        // Disable copy
        EnumCatalogManager(const EnumCatalogManager&) = delete;
        EnumCatalogManager& operator=(const EnumCatalogManager&) = delete;

        // Parsing helpers
        bool ParseCatalogJson(const nlohmann::json& j, EnumCatalog& catalog);
        bool ParseCatalogType(const nlohmann::json& j, CatalogType& type);
        bool ParseCatalogParameter(const nlohmann::json& j, CatalogParameter& param);

        // Validation helpers
        bool ValidateCatalog(const EnumCatalog& catalog, std::string& errorMsg);

    private:
        bool m_Initialized;
        std::string m_CatalogsPath;
        std::string m_LastError;

        // Catalogs (smart pointers for easy replacement during hot-reload)
        std::unique_ptr<EnumCatalog> m_ActionCatalog;
        std::unique_ptr<EnumCatalog> m_ConditionCatalog;
        std::unique_ptr<EnumCatalog> m_DecoratorCatalog;
    };
}
