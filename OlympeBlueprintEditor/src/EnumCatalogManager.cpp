/*
 * Olympe Blueprint Editor - Enum Catalog Manager Implementation
 */

#include "../include/EnumCatalogManager.h"
#include "../../Source/json_helper.h"
#include <iostream>
#include <algorithm>
#include <set>

using json = nlohmann::json;

namespace Olympe
{
    // Singleton instance
    EnumCatalogManager& EnumCatalogManager::Instance()
    {
        static EnumCatalogManager instance;
        return instance;
    }

    // Load all catalogues
    bool EnumCatalogManager::LoadCatalogues(const std::string& cataloguesPath)
    {
        m_cataloguesPath = cataloguesPath;
        m_lastError.clear();

        // Clear existing data
        m_actionTypes.clear();
        m_conditionTypes.clear();
        m_decoratorTypes.clear();
        m_actionLookup.clear();
        m_conditionLookup.clear();
        m_decoratorLookup.clear();

        // Load each catalogue
        bool success = true;
        
        if (!LoadCatalogue(cataloguesPath + "ActionTypes.json", m_actionTypes))
        {
            std::cerr << "EnumCatalogManager: Failed to load ActionTypes.json" << std::endl;
            success = false;
        }

        if (!LoadCatalogue(cataloguesPath + "ConditionTypes.json", m_conditionTypes))
        {
            std::cerr << "EnumCatalogManager: Failed to load ConditionTypes.json" << std::endl;
            success = false;
        }

        if (!LoadCatalogue(cataloguesPath + "DecoratorTypes.json", m_decoratorTypes))
        {
            std::cerr << "EnumCatalogManager: Failed to load DecoratorTypes.json" << std::endl;
            success = false;
        }

        if (success)
        {
            BuildLookupMaps();
            std::cout << "EnumCatalogManager: Successfully loaded catalogues:" << std::endl;
            std::cout << "  - Actions: " << m_actionTypes.size() << std::endl;
            std::cout << "  - Conditions: " << m_conditionTypes.size() << std::endl;
            std::cout << "  - Decorators: " << m_decoratorTypes.size() << std::endl;
        }
        else
        {
            m_lastError = "Failed to load one or more catalogues";
        }

        return success;
    }

    // Reload catalogues
    bool EnumCatalogManager::ReloadCatalogues()
    {
        if (m_cataloguesPath.empty())
        {
            m_lastError = "No catalogue path set. Call LoadCatalogues() first.";
            return false;
        }

        std::cout << "EnumCatalogManager: Reloading catalogues..." << std::endl;
        return LoadCatalogues(m_cataloguesPath);
    }

    // Load a single catalogue file
    bool EnumCatalogManager::LoadCatalogue(const std::string& filepath, std::vector<EnumTypeInfo>& outTypes)
    {
        json j;
        if (!JsonHelper::LoadJsonFromFile(filepath, j))
        {
            m_lastError = "Failed to load file: " + filepath;
            return false;
        }

        // Validate format
        if (!j.contains("version") || !j.contains("types"))
        {
            m_lastError = "Invalid catalogue format in " + filepath;
            std::cerr << "EnumCatalogManager: Missing 'version' or 'types' in " << filepath << std::endl;
            return false;
        }

        if (!j["types"].is_array())
        {
            m_lastError = "Invalid 'types' field in " + filepath;
            std::cerr << "EnumCatalogManager: 'types' is not an array in " << filepath << std::endl;
            return false;
        }

        // Parse types
        const auto& typesArray = j["types"];
        for (size_t i = 0; i < typesArray.size(); ++i)
        {
            const auto& typeJson = typesArray[i];
            EnumTypeInfo info;

            // Required fields
            if (!typeJson.contains("id") || !typeJson["id"].is_string())
            {
                std::cerr << "EnumCatalogManager: Type missing 'id' in " << filepath << std::endl;
                continue;
            }
            info.id = typeJson["id"].get<std::string>();

            // Optional fields with defaults
            info.name = JsonHelper::GetString(typeJson, "name", info.id);
            info.description = JsonHelper::GetString(typeJson, "description", "");
            info.category = JsonHelper::GetString(typeJson, "category", "General");
            info.tooltip = JsonHelper::GetString(typeJson, "tooltip", info.description);

            // Parameters
            if (typeJson.contains("parameters") && typeJson["parameters"].is_array())
            {
                const auto& params = typeJson["parameters"];
                for (size_t j = 0; j < params.size(); ++j)
                {
                    info.parameters.push_back(params[j]);
                }
            }

            outTypes.push_back(info);
        }

        return true;
    }

    // Build lookup maps for fast search
    void EnumCatalogManager::BuildLookupMaps()
    {
        m_actionLookup.clear();
        m_conditionLookup.clear();
        m_decoratorLookup.clear();

        for (size_t i = 0; i < m_actionTypes.size(); ++i)
        {
            m_actionLookup[m_actionTypes[i].id] = i;
        }

        for (size_t i = 0; i < m_conditionTypes.size(); ++i)
        {
            m_conditionLookup[m_conditionTypes[i].id] = i;
        }

        for (size_t i = 0; i < m_decoratorTypes.size(); ++i)
        {
            m_decoratorLookup[m_decoratorTypes[i].id] = i;
        }
    }

    // Find type by ID
    const EnumTypeInfo* EnumCatalogManager::FindActionType(const std::string& id) const
    {
        auto it = m_actionLookup.find(id);
        if (it != m_actionLookup.end())
        {
            return &m_actionTypes[it->second];
        }
        return nullptr;
    }

    const EnumTypeInfo* EnumCatalogManager::FindConditionType(const std::string& id) const
    {
        auto it = m_conditionLookup.find(id);
        if (it != m_conditionLookup.end())
        {
            return &m_conditionTypes[it->second];
        }
        return nullptr;
    }

    const EnumTypeInfo* EnumCatalogManager::FindDecoratorType(const std::string& id) const
    {
        auto it = m_decoratorLookup.find(id);
        if (it != m_decoratorLookup.end())
        {
            return &m_decoratorTypes[it->second];
        }
        return nullptr;
    }

    // Validation
    bool EnumCatalogManager::IsValidActionType(const std::string& id) const
    {
        return m_actionLookup.find(id) != m_actionLookup.end();
    }

    bool EnumCatalogManager::IsValidConditionType(const std::string& id) const
    {
        return m_conditionLookup.find(id) != m_conditionLookup.end();
    }

    bool EnumCatalogManager::IsValidDecoratorType(const std::string& id) const
    {
        return m_decoratorLookup.find(id) != m_decoratorLookup.end();
    }

    // Get display names for UI
    std::vector<const char*> EnumCatalogManager::GetActionTypeNames() const
    {
        std::vector<const char*> names;
        names.reserve(m_actionTypes.size());
        for (const auto& type : m_actionTypes)
        {
            names.push_back(type.name.c_str());
        }
        return names;
    }

    std::vector<const char*> EnumCatalogManager::GetConditionTypeNames() const
    {
        std::vector<const char*> names;
        names.reserve(m_conditionTypes.size());
        for (const auto& type : m_conditionTypes)
        {
            names.push_back(type.name.c_str());
        }
        return names;
    }

    std::vector<const char*> EnumCatalogManager::GetDecoratorTypeNames() const
    {
        std::vector<const char*> names;
        names.reserve(m_decoratorTypes.size());
        for (const auto& type : m_decoratorTypes)
        {
            names.push_back(type.name.c_str());
        }
        return names;
    }

    // Get unique categories
    std::vector<std::string> EnumCatalogManager::GetCategories(const std::vector<EnumTypeInfo>& types) const
    {
        std::set<std::string> categorySet;
        for (const auto& type : types)
        {
            if (!type.category.empty())
            {
                categorySet.insert(type.category);
            }
        }
        return std::vector<std::string>(categorySet.begin(), categorySet.end());
    }

    std::vector<std::string> EnumCatalogManager::GetActionCategories() const
    {
        return GetCategories(m_actionTypes);
    }

    std::vector<std::string> EnumCatalogManager::GetConditionCategories() const
    {
        return GetCategories(m_conditionTypes);
    }

    std::vector<std::string> EnumCatalogManager::GetDecoratorCategories() const
    {
        return GetCategories(m_decoratorTypes);
    }

    // Get types by category
    std::vector<EnumTypeInfo> EnumCatalogManager::GetActionsByCategory(const std::string& category) const
    {
        std::vector<EnumTypeInfo> result;
        for (const auto& type : m_actionTypes)
        {
            if (type.category == category)
            {
                result.push_back(type);
            }
        }
        return result;
    }

    std::vector<EnumTypeInfo> EnumCatalogManager::GetConditionsByCategory(const std::string& category) const
    {
        std::vector<EnumTypeInfo> result;
        for (const auto& type : m_conditionTypes)
        {
            if (type.category == category)
            {
                result.push_back(type);
            }
        }
        return result;
    }

    std::vector<EnumTypeInfo> EnumCatalogManager::GetDecoratorsByCategory(const std::string& category) const
    {
        std::vector<EnumTypeInfo> result;
        for (const auto& type : m_decoratorTypes)
        {
            if (type.category == category)
            {
                result.push_back(type);
            }
        }
        return result;
    }

} // namespace Olympe
