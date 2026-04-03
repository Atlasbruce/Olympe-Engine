#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace OlympeEngine {

// ============================================================================
// Data Structures for Entity Prefab System
// ============================================================================

struct ParameterDefinition {
    std::string name;
    std::string type;          // int, float, bool, string, Vector3, etc.
    std::string defaultValue;
    bool required = false;
    std::string description;
    std::vector<std::string> enumValues;  // For enum types
};

struct ComponentSchema {
    std::string componentType;
    std::string category;
    std::string description;
    std::vector<ParameterDefinition> parameters;
};

struct ComponentData {
    std::string type;
    std::string name;
    std::map<std::string, std::string> properties;  // Property name -> value
    bool enabled = true;
};

struct EntityPrefab {
    int schemaVersion = 4;
    std::string name;
    std::string author;
    std::string created;
    std::string modified;
    std::vector<ComponentData> components;
    std::map<std::string, std::string> metadata;  // Custom metadata
};

// ============================================================================
// PrefabLoader - File I/O and Schema Management
// ============================================================================

class PrefabLoader {
public:
    // File I/O Operations
    static EntityPrefab LoadFromFile(const std::string& filePath);
    static bool SaveToFile(const EntityPrefab& prefab, const std::string& filePath);
    static bool ValidateFileExists(const std::string& filePath);

    // Schema Management
    static void LoadParameterSchemas(const std::string& schemaPath);
    static const ComponentSchema* GetComponentSchema(const std::string& componentType);
    static std::vector<std::string> GetAvailableComponentTypes();
    static bool IsComponentSchemaLoaded(const std::string& componentType);

    // Validation
    static bool ValidateAgainstSchemas(const EntityPrefab& prefab);
    static bool ValidateComponent(const ComponentData& component);
    static std::vector<std::string> GetValidationErrors();

    // Utility
    static std::string GetPrefabFileName(const std::string& filePath);
    static std::string GetPrefabDirectory(const std::string& filePath);
    static bool IsValidPrefabFile(const std::string& filePath);

private:
    // Internal schema cache
    static std::map<std::string, ComponentSchema> s_schemas;
    static bool s_schemasLoaded;
    static std::vector<std::string> s_validationErrors;

    // JSON Parsing helpers
    static EntityPrefab ParseJsonPrefab(const std::string& jsonContent);
    static ComponentSchema ParseJsonComponentSchema(const std::string& jsonContent);
    static std::string ToJsonString(const EntityPrefab& prefab);

    // Validation helpers
    static bool ValidateComponentProperty(
        const std::string& componentType,
        const std::string& propertyName,
        const std::string& propertyValue
    );
};

}  // namespace OlympeEngine
