#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "./../../third_party/nlohmann/json.hpp"
#include "PrefabLoader.h"

namespace Olympe
{
    using json = nlohmann::json;

    class ParameterSchemaRegistry
    {
    public:
        static ParameterSchemaRegistry& Get();

        // Schema registration and retrieval
        void RegisterSchema(const ComponentSchema& schema);
        void UnregisterSchema(const std::string& componentName);
        bool HasSchema(const std::string& componentName) const;

        const ComponentSchema* GetSchema(const std::string& componentName) const;
        const std::vector<ComponentSchema>& GetAllSchemas() const;

        // Bulk operations
        void LoadSchemasFromFile(const std::string& filePath);
        void LoadSchemasFromDirectory(const std::string& directoryPath);
        void ClearAllSchemas();

        // Filtering and search
        std::vector<std::string> GetSchemaNames() const;
        std::vector<ComponentSchema> GetSchemasByCategory(const std::string& category) const;
        std::vector<ComponentSchema> SearchSchemas(const std::string& query) const;

        // Categories
        std::vector<std::string> GetCategories() const;
        std::vector<ComponentSchema> GetSchemasInCategory(const std::string& category) const;

        // Parameter definitions
        const ParameterDefinition* GetParameterDefinition(
            const std::string& componentName,
            const std::string& parameterName) const;

        std::vector<ParameterDefinition> GetParametersForComponent(
            const std::string& componentName) const;

        // Validation
        bool ValidateComponent(const ComponentData& component) const;
        bool ValidateParameter(
            const std::string& componentName,
            const std::string& parameterName,
            const std::string& value) const;

        // Caching and performance
        void RebuildCategoryIndex();
        void ClearCache();

        // Statistics
        size_t GetSchemaCount() const;
        size_t GetCategoryCount() const;
        size_t GetTotalParameterCount() const;

        // Deprecated schemas
        bool IsSchemaDeprecated(const std::string& componentName) const;
        std::vector<ComponentSchema> GetDeprecatedSchemas() const;

    private:
        ParameterSchemaRegistry();
        ~ParameterSchemaRegistry();

        // Prevent copying
        ParameterSchemaRegistry(const ParameterSchemaRegistry&) = delete;
        ParameterSchemaRegistry& operator=(const ParameterSchemaRegistry&) = delete;

        std::vector<ComponentSchema> m_schemas;
        std::map<std::string, size_t> m_schemaIndex;
        std::map<std::string, std::vector<size_t>> m_categoryIndex;

        void UpdateIndices();
        static ParameterSchemaRegistry* s_instance;
    };
}
