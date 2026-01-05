/*
 * Olympe Blueprint Editor - Enum Catalog Manager Implementation
 */

#include "EnumCatalogManager.h"
#include "../json_helper.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace Olympe
{
    // Singleton instance
    EnumCatalogManager& EnumCatalogManager::Instance()
    {
        static EnumCatalogManager instance;
        return instance;
    }

    EnumCatalogManager::EnumCatalogManager()
        : m_Initialized(false)
        , m_CatalogsPath("Blueprints/Catalogues/")
    {
    }

    EnumCatalogManager::~EnumCatalogManager()
    {
        Shutdown();
    }

    void EnumCatalogManager::Initialize()
    {
        if (m_Initialized)
            return;

        std::cout << "[EnumCatalogManager] Initializing...\n";

        // Load all catalogs
        if (!LoadCatalogs(m_CatalogsPath))
        {
            std::cerr << "[EnumCatalogManager] Failed to load catalogs: " << m_LastError << "\n";
            return;
        }

        m_Initialized = true;
        std::cout << "[EnumCatalogManager] Initialized successfully\n";
    }

    void EnumCatalogManager::Shutdown()
    {
        if (!m_Initialized)
            return;

        std::cout << "[EnumCatalogManager] Shutting down...\n";

        m_ActionCatalog.reset();
        m_ConditionCatalog.reset();
        m_DecoratorCatalog.reset();

        m_Initialized = false;
    }

    bool EnumCatalogManager::LoadCatalogs(const std::string& catalogsPath)
    {
        m_CatalogsPath = catalogsPath;
        m_LastError.clear();

        // Load each catalog
        bool success = true;

        if (!LoadCatalog(m_CatalogsPath + "ActionTypes.json"))
        {
            std::cerr << "[EnumCatalogManager] Failed to load ActionTypes catalog\n";
            success = false;
        }

        if (!LoadCatalog(m_CatalogsPath + "ConditionTypes.json"))
        {
            std::cerr << "[EnumCatalogManager] Failed to load ConditionTypes catalog\n";
            success = false;
        }

        if (!LoadCatalog(m_CatalogsPath + "DecoratorTypes.json"))
        {
            std::cerr << "[EnumCatalogManager] Failed to load DecoratorTypes catalog\n";
            success = false;
        }

        return success;
    }

    bool EnumCatalogManager::LoadCatalog(const std::string& filepath)
    {
        std::cout << "[EnumCatalogManager] Loading catalog: " << filepath << "\n";

        // Read JSON file
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            m_LastError = "Failed to open file: " + filepath;
            return false;
        }

        json j;
        try
        {
            file >> j;
        }
        catch (const json::exception& e)
        {
            m_LastError = "JSON parse error: " + std::string(e.what());
            return false;
        }

        // Parse catalog
        auto catalog = std::make_unique<EnumCatalog>();
        catalog->filepath = filepath;

        if (!ParseCatalogJson(j, *catalog))
        {
            m_LastError = "Failed to parse catalog: " + filepath;
            return false;
        }

        // Validate catalog
        std::string validationError;
        if (!ValidateCatalog(*catalog, validationError))
        {
            m_LastError = "Catalog validation failed: " + validationError;
            return false;
        }

        // Store catalog based on type
        if (catalog->catalogType == "Actions")
        {
            m_ActionCatalog = std::move(catalog);
            std::cout << "[EnumCatalogManager] Loaded " << m_ActionCatalog->types.size() << " action types\n";
        }
        else if (catalog->catalogType == "Conditions")
        {
            m_ConditionCatalog = std::move(catalog);
            std::cout << "[EnumCatalogManager] Loaded " << m_ConditionCatalog->types.size() << " condition types\n";
        }
        else if (catalog->catalogType == "Decorators")
        {
            m_DecoratorCatalog = std::move(catalog);
            std::cout << "[EnumCatalogManager] Loaded " << m_DecoratorCatalog->types.size() << " decorator types\n";
        }
        else
        {
            m_LastError = "Unknown catalog type: " + catalog->catalogType;
            return false;
        }

        return true;
    }

    bool EnumCatalogManager::ReloadCatalogs()
    {
        std::cout << "[EnumCatalogManager] Hot-reloading catalogs...\n";
        
        // Clear existing catalogs
        m_ActionCatalog.reset();
        m_ConditionCatalog.reset();
        m_DecoratorCatalog.reset();

        // Reload from disk
        return LoadCatalogs(m_CatalogsPath);
    }

    bool EnumCatalogManager::ParseCatalogJson(const nlohmann::json& j, EnumCatalog& catalog)
    {
        // Parse version
        catalog.version = JsonHelper::GetString(j, "version", "1.0");

        // Parse catalog type
        catalog.catalogType = JsonHelper::GetString(j, "catalogType", "");
        if (catalog.catalogType.empty())
        {
            m_LastError = "Missing catalogType field";
            return false;
        }

        // Parse types array
        if (!JsonHelper::IsArray(j, "types"))
        {
            m_LastError = "Missing or invalid 'types' array";
            return false;
        }

        JsonHelper::ForEachInArray(j, "types", [&](const json& typeJson, size_t idx)
        {
            CatalogType type;
            if (ParseCatalogType(typeJson, type))
            {
                catalog.types.push_back(type);
            }
        });

        return true;
    }

    bool EnumCatalogManager::ParseCatalogType(const nlohmann::json& j, CatalogType& type)
    {
        type.id = JsonHelper::GetString(j, "id", "");
        type.name = JsonHelper::GetString(j, "name", type.id);
        type.description = JsonHelper::GetString(j, "description", "");
        type.category = JsonHelper::GetString(j, "category", "");
        type.tooltip = JsonHelper::GetString(j, "tooltip", type.description);

        // Parse parameters if present
        if (JsonHelper::IsArray(j, "parameters"))
        {
            JsonHelper::ForEachInArray(j, "parameters", [&](const json& paramJson, size_t idx)
            {
                CatalogParameter param;
                if (ParseCatalogParameter(paramJson, param))
                {
                    type.parameters.push_back(param);
                }
            });
        }

        return !type.id.empty();
    }

    bool EnumCatalogManager::ParseCatalogParameter(const nlohmann::json& j, CatalogParameter& param)
    {
        param.name = JsonHelper::GetString(j, "name", "");
        param.type = JsonHelper::GetString(j, "type", "string");
        param.required = JsonHelper::GetBool(j, "required", false);

        // Get default value
        if (j.contains("default"))
        {
            const auto& defaultVal = j["default"];
            if (defaultVal.is_string())
                param.defaultValue = defaultVal.get<std::string>();
            else if (defaultVal.is_number())
                param.defaultValue = std::to_string(defaultVal.get<double>());
            else if (defaultVal.is_boolean())
                param.defaultValue = defaultVal.get<bool>() ? "true" : "false";
        }

        return !param.name.empty();
    }

    bool EnumCatalogManager::ValidateCatalog(const EnumCatalog& catalog, std::string& errorMsg)
    {
        if (catalog.types.empty())
        {
            errorMsg = "Catalog has no types";
            return false;
        }

        // Check for duplicate IDs
        std::map<std::string, int> idCount;
        for (const auto& type : catalog.types)
        {
            if (type.id.empty())
            {
                errorMsg = "Type with empty ID found";
                return false;
            }
            idCount[type.id]++;
        }

        for (const auto& pair : idCount)
        {
            if (pair.second > 1)
            {
                errorMsg = "Duplicate type ID: " + pair.first;
                return false;
            }
        }

        return true;
    }

    // Type queries
    std::vector<std::string> EnumCatalogManager::GetActionTypes() const
    {
        if (m_ActionCatalog)
            return m_ActionCatalog->GetTypeIds();
        return {};
    }

    std::vector<std::string> EnumCatalogManager::GetConditionTypes() const
    {
        if (m_ConditionCatalog)
            return m_ConditionCatalog->GetTypeIds();
        return {};
    }

    std::vector<std::string> EnumCatalogManager::GetDecoratorTypes() const
    {
        if (m_DecoratorCatalog)
            return m_DecoratorCatalog->GetTypeIds();
        return {};
    }

    // Type lookup
    const CatalogType* EnumCatalogManager::FindActionType(const std::string& id) const
    {
        if (m_ActionCatalog)
            return m_ActionCatalog->FindType(id);
        return nullptr;
    }

    const CatalogType* EnumCatalogManager::FindConditionType(const std::string& id) const
    {
        if (m_ConditionCatalog)
            return m_ConditionCatalog->FindType(id);
        return nullptr;
    }

    const CatalogType* EnumCatalogManager::FindDecoratorType(const std::string& id) const
    {
        if (m_DecoratorCatalog)
            return m_DecoratorCatalog->FindType(id);
        return nullptr;
    }

    // Validation
    bool EnumCatalogManager::IsValidActionType(const std::string& id) const
    {
        return FindActionType(id) != nullptr;
    }

    bool EnumCatalogManager::IsValidConditionType(const std::string& id) const
    {
        return FindConditionType(id) != nullptr;
    }

    bool EnumCatalogManager::IsValidDecoratorType(const std::string& id) const
    {
        return FindDecoratorType(id) != nullptr;
    }
}
