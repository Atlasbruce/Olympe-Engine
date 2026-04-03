#include "ParameterSchemaRegistry.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace OlympeEngine {

// ============================================================================
// ParameterSchemaRegistry Implementation
// ============================================================================

ParameterSchemaRegistry& ParameterSchemaRegistry::Get() {
    static ParameterSchemaRegistry instance;
    return instance;
}

void ParameterSchemaRegistry::LoadSchemasFromDirectory(const std::string& schemaDirectory) {
    if (!fs::exists(schemaDirectory)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(schemaDirectory)) {
        if (entry.path().extension() == ".json") {
            LoadSchemasFromFile(entry.path().string());
        }
    }

    RebuildCategoryIndex();
}

void ParameterSchemaRegistry::LoadSchemasFromFile(const std::string& jsonFilePath) {
    try {
        std::ifstream file(jsonFilePath);
        if (!file.is_open()) return;

        json j;
        file >> j;
        file.close();

        // Handle both single schema and array of schemas
        if (j.is_array()) {
            for (const auto& schemaJson : j) {
                ComponentSchema schema;
                schema.componentType = schemaJson.value("componentType", "");
                schema.category = schemaJson.value("category", "");
                schema.description = schemaJson.value("description", "");

                if (schemaJson.contains("parameters")) {
                    for (const auto& paramJson : schemaJson["parameters"]) {
                        ParameterDefinition param;
                        param.name = paramJson["name"].get<std::string>();
                        param.type = paramJson["type"].get<std::string>();
                        param.defaultValue = paramJson.value("default", "");
                        param.required = paramJson.value("required", false);
                        param.description = paramJson.value("description", "");

                        if (paramJson.contains("enum")) {
                            param.enumValues = paramJson["enum"].get<std::vector<std::string>>();
                        }

                        schema.parameters.push_back(param);
                    }
                }

                m_schemas[schema.componentType] = schema;
            }
        } else {
            ComponentSchema schema;
            schema.componentType = j.value("componentType", "");
            schema.category = j.value("category", "");
            schema.description = j.value("description", "");

            if (j.contains("parameters")) {
                for (const auto& paramJson : j["parameters"]) {
                    ParameterDefinition param;
                    param.name = paramJson["name"].get<std::string>();
                    param.type = paramJson["type"].get<std::string>();
                    param.defaultValue = paramJson.value("default", "");
                    param.required = paramJson.value("required", false);
                    param.description = paramJson.value("description", "");

                    if (paramJson.contains("enum")) {
                        param.enumValues = paramJson["enum"].get<std::vector<std::string>>();
                    }

                    schema.parameters.push_back(param);
                }
            }

            m_schemas[schema.componentType] = schema;
        }
    }
    catch (const std::exception&) {
        // Silent failure - invalid schema file
    }
}

void ParameterSchemaRegistry::RegisterSchema(const ComponentSchema& schema) {
    m_schemas[schema.componentType] = schema;
    RebuildCategoryIndex();
}

// ============================================================================
// Schema Queries
// ============================================================================

const ComponentSchema* ParameterSchemaRegistry::GetSchema(const std::string& componentType) const {
    auto it = m_schemas.find(componentType);
    if (it != m_schemas.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ParameterSchemaRegistry::HasSchema(const std::string& componentType) const {
    return m_schemas.find(componentType) != m_schemas.end();
}

std::vector<std::string> ParameterSchemaRegistry::GetAllComponentTypes() const {
    std::vector<std::string> types;
    for (const auto& [type, schema] : m_schemas) {
        types.push_back(type);
    }
    return types;
}

std::vector<ComponentSchema> ParameterSchemaRegistry::GetAllSchemas() const {
    std::vector<ComponentSchema> schemas;
    for (const auto& [type, schema] : m_schemas) {
        schemas.push_back(schema);
    }
    return schemas;
}

// ============================================================================
// Parameter Queries
// ============================================================================

const ParameterDefinition* ParameterSchemaRegistry::GetParameterDefinition(
    const std::string& componentType,
    const std::string& parameterName
) const {
    auto schema = GetSchema(componentType);
    if (!schema) return nullptr;

    auto it = std::find_if(
        schema->parameters.begin(),
        schema->parameters.end(),
        [&parameterName](const ParameterDefinition& p) { return p.name == parameterName; }
    );

    if (it != schema->parameters.end()) {
        return &(*it);
    }
    return nullptr;
}

std::vector<ParameterDefinition> ParameterSchemaRegistry::GetComponentParameters(
    const std::string& componentType
) const {
    auto schema = GetSchema(componentType);
    if (schema) {
        return schema->parameters;
    }
    return {};
}

bool ParameterSchemaRegistry::ValidateParameterValue(
    const std::string& componentType,
    const std::string& parameterName,
    const std::string& value
) const {
    auto paramDef = GetParameterDefinition(componentType, parameterName);
    if (!paramDef) return false;

    // Basic type validation
    if (paramDef->type == "int") {
        try {
            std::stoi(value);
            return true;
        } catch (...) {
            return false;
        }
    } else if (paramDef->type == "float") {
        try {
            std::stof(value);
            return true;
        } catch (...) {
            return false;
        }
    } else if (paramDef->type == "bool") {
        return value == "true" || value == "false" || value == "0" || value == "1";
    }

    // Enum validation
    if (!paramDef->enumValues.empty()) {
        return std::find(paramDef->enumValues.begin(), paramDef->enumValues.end(), value)
            != paramDef->enumValues.end();
    }

    return true;
}

// ============================================================================
// Category Queries
// ============================================================================

std::vector<std::string> ParameterSchemaRegistry::GetComponentsByCategory(
    const std::string& category
) const {
    auto it = m_categoryIndex.find(category);
    if (it != m_categoryIndex.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> ParameterSchemaRegistry::GetAllCategories() const {
    std::vector<std::string> categories;
    for (const auto& [category, types] : m_categoryIndex) {
        categories.push_back(category);
    }
    return categories;
}

// ============================================================================
// Search & Filter
// ============================================================================

std::vector<std::string> ParameterSchemaRegistry::SearchComponents(const std::string& query) const {
    std::vector<std::string> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& [type, schema] : m_schemas) {
        std::string lowerType = type;
        std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

        if (lowerType.find(lowerQuery) != std::string::npos) {
            results.push_back(type);
        }
    }

    return results;
}

std::vector<ComponentSchema> ParameterSchemaRegistry::FilterByCategory(
    const std::string& category
) const {
    std::vector<ComponentSchema> results;
    auto types = GetComponentsByCategory(category);

    for (const auto& type : types) {
        auto schema = GetSchema(type);
        if (schema) {
            results.push_back(*schema);
        }
    }

    return results;
}

// ============================================================================
// Schema Management
// ============================================================================

void ParameterSchemaRegistry::UpdateSchema(const ComponentSchema& schema) {
    m_schemas[schema.componentType] = schema;
    RebuildCategoryIndex();
}

void ParameterSchemaRegistry::RemoveSchema(const std::string& componentType) {
    m_schemas.erase(componentType);
    RebuildCategoryIndex();
}

void ParameterSchemaRegistry::ClearAllSchemas() {
    m_schemas.clear();
    m_categoryIndex.clear();
}

// ============================================================================
// Statistics
// ============================================================================

int ParameterSchemaRegistry::GetSchemaCount() const {
    return static_cast<int>(m_schemas.size());
}

int ParameterSchemaRegistry::GetTotalParameterCount() const {
    int count = 0;
    for (const auto& [type, schema] : m_schemas) {
        count += static_cast<int>(schema.parameters.size());
    }
    return count;
}

int ParameterSchemaRegistry::GetParameterCount(const std::string& componentType) const {
    auto schema = GetSchema(componentType);
    if (schema) {
        return static_cast<int>(schema->parameters.size());
    }
    return 0;
}

// ============================================================================
// Validation
// ============================================================================

bool ParameterSchemaRegistry::ValidateSchema(const ComponentSchema& schema) const {
    if (schema.componentType.empty()) return false;

    for (const auto& param : schema.parameters) {
        if (!ValidateParameterDefinition(param)) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> ParameterSchemaRegistry::GetSchemaValidationErrors() const {
    return {};  // Placeholder
}

// ============================================================================
// Default Values
// ============================================================================

std::string ParameterSchemaRegistry::GetDefaultValue(
    const std::string& componentType,
    const std::string& parameterName
) const {
    auto paramDef = GetParameterDefinition(componentType, parameterName);
    if (paramDef) {
        return paramDef->defaultValue;
    }
    return "";
}

// ============================================================================
// Export/Import
// ============================================================================

std::string ParameterSchemaRegistry::ExportSchemasAsJson() const {
    json j = json::array();

    for (const auto& [type, schema] : m_schemas) {
        json schemaJson;
        schemaJson["componentType"] = schema.componentType;
        schemaJson["category"] = schema.category;
        schemaJson["description"] = schema.description;

        json params = json::array();
        for (const auto& param : schema.parameters) {
            json paramJson;
            paramJson["name"] = param.name;
            paramJson["type"] = param.type;
            paramJson["default"] = param.defaultValue;
            paramJson["required"] = param.required;
            paramJson["description"] = param.description;

            if (!param.enumValues.empty()) {
                paramJson["enum"] = param.enumValues;
            }

            params.push_back(paramJson);
        }
        schemaJson["parameters"] = params;

        j.push_back(schemaJson);
    }

    return j.dump(2);
}

bool ParameterSchemaRegistry::ImportSchemasFromJson(const std::string& jsonContent) {
    try {
        json j = json::parse(jsonContent);
        m_schemas.clear();

        if (j.is_array()) {
            for (const auto& schemaJson : j) {
                ComponentSchema schema;
                schema.componentType = schemaJson["componentType"].get<std::string>();
                schema.category = schemaJson.value("category", "");
                schema.description = schemaJson.value("description", "");

                if (schemaJson.contains("parameters")) {
                    for (const auto& paramJson : schemaJson["parameters"]) {
                        ParameterDefinition param;
                        param.name = paramJson["name"].get<std::string>();
                        param.type = paramJson["type"].get<std::string>();
                        param.defaultValue = paramJson.value("default", "");
                        param.required = paramJson.value("required", false);
                        param.description = paramJson.value("description", "");

                        if (paramJson.contains("enum")) {
                            param.enumValues = paramJson["enum"].get<std::vector<std::string>>();
                        }

                        schema.parameters.push_back(param);
                    }
                }

                m_schemas[schema.componentType] = schema;
            }
        }

        RebuildCategoryIndex();
        return true;
    }
    catch (...) {
        return false;
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

void ParameterSchemaRegistry::RebuildCategoryIndex() {
    m_categoryIndex.clear();

    for (const auto& [type, schema] : m_schemas) {
        m_categoryIndex[schema.category].push_back(type);
    }
}

bool ParameterSchemaRegistry::ValidateComponentSchema(const ComponentSchema& schema) const {
    return ValidateSchema(schema);
}

bool ParameterSchemaRegistry::ValidateParameterDefinition(const ParameterDefinition& param) const {
    if (param.name.empty() || param.type.empty()) {
        return false;
    }
    return true;
}

// ============================================================================
// ComponentLibrary Implementation
// ============================================================================

ComponentLibrary& ComponentLibrary::Get() {
    static ComponentLibrary instance;
    return instance;
}

std::vector<std::string> ComponentLibrary::GetAvailableComponents() const {
    return ParameterSchemaRegistry::Get().GetAllComponentTypes();
}

std::vector<std::string> ComponentLibrary::GetComponentsByCategory(const std::string& category) const {
    return ParameterSchemaRegistry::Get().GetComponentsByCategory(category);
}

const ComponentSchema* ComponentLibrary::GetComponentSchema(const std::string& componentType) const {
    return ParameterSchemaRegistry::Get().GetSchema(componentType);
}

ComponentData ComponentLibrary::CreateComponent(
    const std::string& componentType,
    const std::string& instanceName
) const {
    ComponentData data;
    data.type = componentType;
    data.name = instanceName;
    data.enabled = true;

    auto schema = GetComponentSchema(componentType);
    if (schema) {
        for (const auto& param : schema->parameters) {
            data.properties[param.name] = param.defaultValue;
        }
    }

    return data;
}

ComponentData ComponentLibrary::CreateComponentWithDefaults(const std::string& componentType) const {
    return CreateComponent(componentType, componentType);
}

bool ComponentLibrary::ValidateComponent(const ComponentData& component) const {
    auto schema = GetComponentSchema(component.type);
    if (!schema) return false;

    for (const auto& param : schema->parameters) {
        if (param.required && component.properties.find(param.name) == component.properties.end()) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> ComponentLibrary::ValidateComponentAndGetErrors(
    const ComponentData& component
) const {
    std::vector<std::string> errors;

    auto schema = GetComponentSchema(component.type);
    if (!schema) {
        errors.push_back("Unknown component type: " + component.type);
        return errors;
    }

    for (const auto& param : schema->parameters) {
        if (param.required && component.properties.find(param.name) == component.properties.end()) {
            errors.push_back("Missing required property: " + param.name);
        }
    }

    return errors;
}

std::vector<std::string> ComponentLibrary::GetComponentProperties(const std::string& componentType) const {
    std::vector<std::string> properties;
    auto schema = GetComponentSchema(componentType);

    if (schema) {
        for (const auto& param : schema->parameters) {
            properties.push_back(param.name);
        }
    }

    return properties;
}

std::string ComponentLibrary::GetPropertyType(
    const std::string& componentType,
    const std::string& propertyName
) const {
    auto schema = GetComponentSchema(componentType);
    if (!schema) return "";

    for (const auto& param : schema->parameters) {
        if (param.name == propertyName) {
            return param.type;
        }
    }

    return "";
}

bool ComponentLibrary::IsPropertyRequired(
    const std::string& componentType,
    const std::string& propertyName
) const {
    auto schema = GetComponentSchema(componentType);
    if (!schema) return false;

    for (const auto& param : schema->parameters) {
        if (param.name == propertyName) {
            return param.required;
        }
    }

    return false;
}

std::string ComponentLibrary::GetPropertyDefaultValue(
    const std::string& componentType,
    const std::string& propertyName
) const {
    auto schema = GetComponentSchema(componentType);
    if (!schema) return "";

    for (const auto& param : schema->parameters) {
        if (param.name == propertyName) {
            return param.defaultValue;
        }
    }

    return "";
}

std::string ComponentLibrary::GetComponentDescription(const std::string& componentType) const {
    auto schema = GetComponentSchema(componentType);
    if (schema) {
        return schema->description;
    }
    return "";
}

std::string ComponentLibrary::GetComponentCategory(const std::string& componentType) const {
    auto schema = GetComponentSchema(componentType);
    if (schema) {
        return schema->category;
    }
    return "";
}

std::vector<std::string> ComponentLibrary::SearchComponents(const std::string& query) const {
    return ParameterSchemaRegistry::Get().SearchComponents(query);
}

std::vector<std::string> ComponentLibrary::GetAllCategories() const {
    return ParameterSchemaRegistry::Get().GetAllCategories();
}

void ComponentLibrary::Initialize(const std::string& schemaDirectory) {
    ParameterSchemaRegistry::Get().LoadSchemasFromDirectory(schemaDirectory);
    m_initialized = true;
}

}  // namespace OlympeEngine
