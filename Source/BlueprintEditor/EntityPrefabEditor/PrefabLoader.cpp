#include "PrefabLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace OlympeEngine {

// ============================================================================
// Static Member Initialization
// ============================================================================

std::map<std::string, ComponentSchema> PrefabLoader::s_schemas;
bool PrefabLoader::s_schemasLoaded = false;
std::vector<std::string> PrefabLoader::s_validationErrors;

// ============================================================================
// File I/O Operations
// ============================================================================

EntityPrefab PrefabLoader::LoadFromFile(const std::string& filePath) {
    s_validationErrors.clear();

    // Check if file exists
    if (!fs::exists(filePath)) {
        s_validationErrors.push_back("File not found: " + filePath);
        return EntityPrefab();
    }

    try {
        // Read file content
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            s_validationErrors.push_back("Failed to open file: " + filePath);
            return EntityPrefab();
        }

        std::string content((std::istreambuf_iterator<char>(file)), 
                           std::istreambuf_iterator<char>());
        file.close();

        // Parse JSON and convert to EntityPrefab
        return ParseJsonPrefab(content);
    }
    catch (const std::exception& e) {
        s_validationErrors.push_back(std::string("Exception reading file: ") + e.what());
        return EntityPrefab();
    }
}

bool PrefabLoader::SaveToFile(const EntityPrefab& prefab, const std::string& filePath) {
    s_validationErrors.clear();

    try {
        // Convert to JSON
        std::string jsonContent = ToJsonString(prefab);

        // Create directory if needed
        fs::path path(filePath);
        fs::create_directories(path.parent_path());

        // Write file
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            s_validationErrors.push_back("Failed to open file for writing: " + filePath);
            return false;
        }

        file.write(jsonContent.c_str(), jsonContent.size());
        file.close();

        return true;
    }
    catch (const std::exception& e) {
        s_validationErrors.push_back(std::string("Exception writing file: ") + e.what());
        return false;
    }
}

bool PrefabLoader::ValidateFileExists(const std::string& filePath) {
    return fs::exists(filePath) && fs::is_regular_file(filePath);
}

// ============================================================================
// Schema Management
// ============================================================================

void PrefabLoader::LoadParameterSchemas(const std::string& schemaPath) {
    s_schemas.clear();
    s_validationErrors.clear();

    try {
        // Try to load single file
        if (fs::is_regular_file(schemaPath)) {
            std::ifstream file(schemaPath);
            if (!file.is_open()) {
                s_validationErrors.push_back("Cannot open schema file: " + schemaPath);
                return;
            }

            json schemaJson;
            file >> schemaJson;
            file.close();

            // Parse schemas from JSON array or object
            if (schemaJson.is_array()) {
                for (const auto& schema : schemaJson) {
                    ComponentSchema cs = ParseJsonComponentSchema(schema.dump());
                    s_schemas[cs.componentType] = cs;
                }
            }
        }
        // Or try to load directory
        else if (fs::is_directory(schemaPath)) {
            for (const auto& entry : fs::directory_iterator(schemaPath)) {
                if (entry.path().extension() == ".json") {
                    std::ifstream file(entry.path());
                    if (file.is_open()) {
                        json schemaJson;
                        file >> schemaJson;
                        file.close();

                        ComponentSchema cs = ParseJsonComponentSchema(schemaJson.dump());
                        s_schemas[cs.componentType] = cs;
                    }
                }
            }
        }

        s_schemasLoaded = true;
    }
    catch (const std::exception& e) {
        s_validationErrors.push_back(std::string("Exception loading schemas: ") + e.what());
        s_schemasLoaded = false;
    }
}

const ComponentSchema* PrefabLoader::GetComponentSchema(const std::string& componentType) {
    auto it = s_schemas.find(componentType);
    if (it != s_schemas.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> PrefabLoader::GetAvailableComponentTypes() {
    std::vector<std::string> types;
    for (const auto& [type, schema] : s_schemas) {
        types.push_back(type);
    }
    return types;
}

bool PrefabLoader::IsComponentSchemaLoaded(const std::string& componentType) {
    return s_schemas.find(componentType) != s_schemas.end();
}

// ============================================================================
// Validation
// ============================================================================

bool PrefabLoader::ValidateAgainstSchemas(const EntityPrefab& prefab) {
    s_validationErrors.clear();
    bool isValid = true;

    // Check schema version
    if (prefab.schemaVersion != 4) {
        s_validationErrors.push_back("Invalid schema version: " + std::to_string(prefab.schemaVersion));
        isValid = false;
    }

    // Check each component
    for (const auto& component : prefab.components) {
        if (!ValidateComponent(component)) {
            isValid = false;
        }
    }

    return isValid;
}

bool PrefabLoader::ValidateComponent(const ComponentData& component) {
    // Check if schema exists
    auto schema = GetComponentSchema(component.type);
    if (!schema) {
        s_validationErrors.push_back("Unknown component type: " + component.type);
        return false;
    }

    // Validate required properties
    for (const auto& param : schema->parameters) {
        if (param.required) {
            if (component.properties.find(param.name) == component.properties.end()) {
                s_validationErrors.push_back(
                    "Missing required property '" + param.name + 
                    "' in component '" + component.type + "'"
                );
                return false;
            }
        }
    }

    // Validate property values
    for (const auto& [propName, propValue] : component.properties) {
        if (!ValidateComponentProperty(component.type, propName, propValue)) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> PrefabLoader::GetValidationErrors() {
    return s_validationErrors;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string PrefabLoader::GetPrefabFileName(const std::string& filePath) {
    fs::path path(filePath);
    return path.filename().string();
}

std::string PrefabLoader::GetPrefabDirectory(const std::string& filePath) {
    fs::path path(filePath);
    return path.parent_path().string();
}

bool PrefabLoader::IsValidPrefabFile(const std::string& filePath) {
    if (!ValidateFileExists(filePath)) {
        return false;
    }

    try {
        std::ifstream file(filePath);
        json j;
        file >> j;
        file.close();

        // Check for required fields
        return j.contains("schema_version") && 
               j.contains("name") && 
               j.contains("data");
    }
    catch (...) {
        return false;
    }
}

// ============================================================================
// JSON Parsing
// ============================================================================

EntityPrefab PrefabLoader::ParseJsonPrefab(const std::string& jsonContent) {
    EntityPrefab prefab;

    try {
        json j = json::parse(jsonContent);

        // Parse basic fields
        if (j.contains("schema_version")) {
            prefab.schemaVersion = j["schema_version"].get<int>();
        }
        if (j.contains("name")) {
            prefab.name = j["name"].get<std::string>();
        }
        if (j.contains("metadata")) {
            if (j["metadata"].contains("author")) {
                prefab.author = j["metadata"]["author"].get<std::string>();
            }
            if (j["metadata"].contains("created")) {
                prefab.created = j["metadata"]["created"].get<std::string>();
            }
        }
        if (j.contains("modified")) {
            prefab.modified = j["modified"].get<std::string>();
        }

        // Parse components
        if (j.contains("data") && j["data"].contains("components")) {
            for (const auto& compJson : j["data"]["components"]) {
                ComponentData component;
                component.type = compJson["type"].get<std::string>();
                component.name = compJson.value("name", component.type);

                if (compJson.contains("properties")) {
                    for (auto& [key, value] : compJson["properties"].items()) {
                        component.properties[key] = value.dump();
                    }
                }

                component.enabled = compJson.value("enabled", true);
                prefab.components.push_back(component);
            }
        }

        // Parse custom metadata
        if (j.contains("metadata")) {
            for (auto& [key, value] : j["metadata"].items()) {
                if (key != "author" && key != "created") {
                    prefab.metadata[key] = value.dump();
                }
            }
        }
    }
    catch (const std::exception& e) {
        s_validationErrors.push_back(std::string("JSON parse error: ") + e.what());
    }

    return prefab;
}

ComponentSchema PrefabLoader::ParseJsonComponentSchema(const std::string& jsonContent) {
    ComponentSchema schema;

    try {
        json j = json::parse(jsonContent);

        schema.componentType = j.value("componentType", "");
        schema.category = j.value("category", "");
        schema.description = j.value("description", "");

        // Parse parameters
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
    }
    catch (const std::exception& e) {
        s_validationErrors.push_back(std::string("Schema parse error: ") + e.what());
    }

    return schema;
}

std::string PrefabLoader::ToJsonString(const EntityPrefab& prefab) {
    json j;

    j["schema_version"] = prefab.schemaVersion;
    j["name"] = prefab.name;
    j["metadata"]["author"] = prefab.author;
    j["metadata"]["created"] = prefab.created;
    j["modified"] = prefab.modified;

    // Add components
    json components = json::array();
    for (const auto& component : prefab.components) {
        json compJson;
        compJson["type"] = component.type;
        compJson["name"] = component.name;
        compJson["enabled"] = component.enabled;

        json props;
        for (const auto& [key, value] : component.properties) {
            props[key] = json::parse(value);
        }
        compJson["properties"] = props;

        components.push_back(compJson);
    }
    j["data"]["components"] = components;

    // Add custom metadata
    for (const auto& [key, value] : prefab.metadata) {
        j["metadata"][key] = json::parse(value);
    }

    return j.dump(2);  // Pretty print with 2-space indentation
}

// ============================================================================
// Property Validation Helper
// ============================================================================

bool PrefabLoader::ValidateComponentProperty(
    const std::string& componentType,
    const std::string& propertyName,
    const std::string& propertyValue
) {
    auto schema = GetComponentSchema(componentType);
    if (!schema) {
        return false;
    }

    // Find parameter definition
    auto it = std::find_if(
        schema->parameters.begin(),
        schema->parameters.end(),
        [&propertyName](const ParameterDefinition& p) { return p.name == propertyName; }
    );

    if (it == schema->parameters.end()) {
        s_validationErrors.push_back(
            "Unknown property '" + propertyName + "' for component '" + componentType + "'"
        );
        return false;
    }

    // Validate based on type
    const auto& paramType = it->type;
    try {
        if (paramType == "int") {
            std::stoi(propertyValue);
        }
        else if (paramType == "float") {
            std::stof(propertyValue);
        }
        else if (paramType == "bool") {
            // Accept true/false or 0/1
        }
        // String types don't need special validation

        // Validate enum values if applicable
        if (!it->enumValues.empty()) {
            auto enumIt = std::find(it->enumValues.begin(), it->enumValues.end(), propertyValue);
            if (enumIt == it->enumValues.end()) {
                s_validationErrors.push_back(
                    "Invalid enum value '" + propertyValue + "' for property '" + propertyName + "'"
                );
                return false;
            }
        }
    }
    catch (...) {
        s_validationErrors.push_back(
            "Invalid value type for property '" + propertyName + "' (expected " + paramType + ")"
        );
        return false;
    }

    return true;
}

}  // namespace OlympeEngine
