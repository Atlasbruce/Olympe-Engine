/*
 * Olympe Blueprint Editor - Enum Catalog Manager
 * 
 * Centralized manager for loading and accessing Action, Condition, and Decorator type catalogs.
 * Provides validation, search, and UI helper functionality for the blueprint editor.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe
{
    /**
     * Information about a single enum type (Action, Condition, or Decorator)
     */
    struct EnumTypeInfo
    {
        std::string id;                             // Unique identifier (e.g., "MoveTo")
        std::string name;                           // Display name (e.g., "Move To")
        std::string description;                    // Detailed description
        std::string category;                       // Category for grouping (e.g., "Movement", "Combat")
        std::string tooltip;                        // Tooltip text for UI
        std::vector<nlohmann::json> parameters;     // Expected parameters with types and defaults
    };

    /**
     * Centralized manager for enum type catalogs
     * 
     * Singleton class that loads and manages catalog data for Actions, Conditions, and Decorators.
     * Provides fast lookup, validation, and UI helper functions.
     * 
     * Usage:
     *   EnumCatalogManager::Instance().LoadCatalogues("Blueprints/Catalogues/");
     *   auto actions = EnumCatalogManager::Instance().GetActionTypes();
     */
    class EnumCatalogManager
    {
    public:
        // Singleton access
        static EnumCatalogManager& Instance();

        // Prevent copying
        EnumCatalogManager(const EnumCatalogManager&) = delete;
        EnumCatalogManager& operator=(const EnumCatalogManager&) = delete;

        // Catalogue loading
        /**
         * Load all catalogues from the specified directory
         * @param cataloguesPath Path to the Catalogues directory (e.g., "Blueprints/Catalogues/")
         * @return true if all catalogues loaded successfully
         */
        bool LoadCatalogues(const std::string& cataloguesPath);

        /**
         * Reload all catalogues (hot reload)
         * @return true if reload was successful
         */
        bool ReloadCatalogues();

        // Access to type lists
        const std::vector<EnumTypeInfo>& GetActionTypes() const { return m_actionTypes; }
        const std::vector<EnumTypeInfo>& GetConditionTypes() const { return m_conditionTypes; }
        const std::vector<EnumTypeInfo>& GetDecoratorTypes() const { return m_decoratorTypes; }

        // Type lookup (returns nullptr if not found)
        const EnumTypeInfo* FindActionType(const std::string& id) const;
        const EnumTypeInfo* FindConditionType(const std::string& id) const;
        const EnumTypeInfo* FindDecoratorType(const std::string& id) const;

        // Validation
        bool IsValidActionType(const std::string& id) const;
        bool IsValidConditionType(const std::string& id) const;
        bool IsValidDecoratorType(const std::string& id) const;

        // UI Helpers - Get display names for dropdowns
        /**
         * Get array of C-string names for ImGui combos
         * Note: The returned pointers are valid until the next reload
         */
        std::vector<const char*> GetActionTypeNames() const;
        std::vector<const char*> GetConditionTypeNames() const;
        std::vector<const char*> GetDecoratorTypeNames() const;

        // Category helpers
        std::vector<std::string> GetActionCategories() const;
        std::vector<std::string> GetConditionCategories() const;
        std::vector<std::string> GetDecoratorCategories() const;
        
        std::vector<EnumTypeInfo> GetActionsByCategory(const std::string& category) const;
        std::vector<EnumTypeInfo> GetConditionsByCategory(const std::string& category) const;
        std::vector<EnumTypeInfo> GetDecoratorsByCategory(const std::string& category) const;

        // Error reporting
        const std::string& GetLastError() const { return m_lastError; }

    private:
        EnumCatalogManager() = default;
        ~EnumCatalogManager() = default;

        // Internal loading
        bool LoadCatalogue(const std::string& filepath, std::vector<EnumTypeInfo>& outTypes);
        void BuildLookupMaps();
        std::vector<std::string> GetCategories(const std::vector<EnumTypeInfo>& types) const;

        // Data storage
        std::string m_cataloguesPath;
        std::vector<EnumTypeInfo> m_actionTypes;
        std::vector<EnumTypeInfo> m_conditionTypes;
        std::vector<EnumTypeInfo> m_decoratorTypes;

        // Fast lookup maps (id -> index in vector)
        std::unordered_map<std::string, size_t> m_actionLookup;
        std::unordered_map<std::string, size_t> m_conditionLookup;
        std::unordered_map<std::string, size_t> m_decoratorLookup;

        // Error tracking
        std::string m_lastError;
    };

} // namespace Olympe
