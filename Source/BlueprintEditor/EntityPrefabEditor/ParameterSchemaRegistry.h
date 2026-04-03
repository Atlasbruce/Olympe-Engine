#pragma once

#include "PrefabLoader.h"
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace OlympeEngine {

// ============================================================================
// ParameterSchemaRegistry - Caches and manages component schemas
// ============================================================================

class ParameterSchemaRegistry {
public:
    // Singleton Pattern
    static ParameterSchemaRegistry& Get();

    // Schema Loading
    void LoadSchemasFromDirectory(const std::string& schemaDirectory);
    void LoadSchemasFromFile(const std::string& jsonFilePath);
    void RegisterSchema(const ComponentSchema& schema);

    // Schema Queries
    const ComponentSchema* GetSchema(const std::string& componentType) const;
    bool HasSchema(const std::string& componentType) const;
    std::vector<std::string> GetAllComponentTypes() const;
    std::vector<ComponentSchema> GetAllSchemas() const;

    // Parameter Queries
    const ParameterDefinition* GetParameterDefinition(
        const std::string& componentType,
        const std::string& parameterName
    ) const;

    std::vector<ParameterDefinition> GetComponentParameters(
        const std::string& componentType
    ) const;

    bool ValidateParameterValue(
        const std::string& componentType,
        const std::string& parameterName,
        const std::string& value
    ) const;

    // Category-based Queries
    std::vector<std::string> GetComponentsByCategory(const std::string& category) const;
    std::vector<std::string> GetAllCategories() const;

    // Filtering & Search
    std::vector<std::string> SearchComponents(const std::string& query) const;
    std::vector<ComponentSchema> FilterByCategory(const std::string& category) const;

    // Schema Update
    void UpdateSchema(const ComponentSchema& schema);
    void RemoveSchema(const std::string& componentType);
    void ClearAllSchemas();

    // Statistics
    int GetSchemaCount() const;
    int GetTotalParameterCount() const;
    int GetParameterCount(const std::string& componentType) const;

    // Validation
    bool ValidateSchema(const ComponentSchema& schema) const;
    std::vector<std::string> GetSchemaValidationErrors() const;

    // Default Values
    std::string GetDefaultValue(
        const std::string& componentType,
        const std::string& parameterName
    ) const;

    // Export/Import
    std::string ExportSchemasAsJson() const;
    bool ImportSchemasFromJson(const std::string& jsonContent);

private:
    ParameterSchemaRegistry() = default;
    ~ParameterSchemaRegistry() = default;

    // Disable copy and assignment
    ParameterSchemaRegistry(const ParameterSchemaRegistry&) = delete;
    ParameterSchemaRegistry& operator=(const ParameterSchemaRegistry&) = delete;

    // Schema cache
    std::map<std::string, ComponentSchema> m_schemas;
    std::map<std::string, std::vector<std::string>> m_categoryIndex;

    // Helper methods
    void RebuildCategoryIndex();
    bool ValidateComponentSchema(const ComponentSchema& schema) const;
    bool ValidateParameterDefinition(const ParameterDefinition& param) const;
};

// ============================================================================
// ComponentLibrary - High-level component management
// ============================================================================

class ComponentLibrary {
public:
    static ComponentLibrary& Get();

    // Component Queries
    std::vector<std::string> GetAvailableComponents() const;
    std::vector<std::string> GetComponentsByCategory(const std::string& category) const;
    const ComponentSchema* GetComponentSchema(const std::string& componentType) const;

    // Component Creation
    ComponentData CreateComponent(
        const std::string& componentType,
        const std::string& instanceName
    ) const;

    ComponentData CreateComponentWithDefaults(const std::string& componentType) const;

    // Component Validation
    bool ValidateComponent(const ComponentData& component) const;
    std::vector<std::string> ValidateComponentAndGetErrors(const ComponentData& component) const;

    // Component Properties
    std::vector<std::string> GetComponentProperties(const std::string& componentType) const;
    std::string GetPropertyType(
        const std::string& componentType,
        const std::string& propertyName
    ) const;

    bool IsPropertyRequired(
        const std::string& componentType,
        const std::string& propertyName
    ) const;

    std::string GetPropertyDefaultValue(
        const std::string& componentType,
        const std::string& propertyName
    ) const;

    // Component Information
    std::string GetComponentDescription(const std::string& componentType) const;
    std::string GetComponentCategory(const std::string& componentType) const;

    // Search & Filter
    std::vector<std::string> SearchComponents(const std::string& query) const;
    std::vector<std::string> GetAllCategories() const;

    // Initialization
    void Initialize(const std::string& schemaDirectory);

private:
    ComponentLibrary() = default;
    ~ComponentLibrary() = default;

    // Disable copy and assignment
    ComponentLibrary(const ComponentLibrary&) = delete;
    ComponentLibrary& operator=(const ComponentLibrary&) = delete;

    bool m_initialized = false;
};

}  // namespace OlympeEngine
