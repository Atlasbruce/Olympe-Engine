#pragma once

#include <string>
#include <vector>
#include <map>
#include "./../../third_party/nlohmann/json.hpp"

namespace Olympe
{
    using json = nlohmann::json;

    // Forward declarations
    struct ComponentData;
    struct ComponentSchema;
    struct ParameterDefinition;
    struct EntityPrefab;

    enum class ParameterType
    {
        Boolean,
        Integer,
        Float,
        String,
        Vector2,
        Vector3,
        Vector4,
        Color,
        Unknown
    };

    struct ParameterDefinition
    {
        std::string name;
        ParameterType type;
        std::string defaultValue;
        std::string description;
        bool isRequired;
        std::vector<std::string> allowedValues;  // For enum-like parameters

        ParameterDefinition()
            : type(ParameterType::Unknown), isRequired(false)
        {
        }

        explicit ParameterDefinition(const std::string& n)
            : name(n), type(ParameterType::Unknown), isRequired(false)
        {
        }

        json ToJson() const;
        static ParameterDefinition FromJson(const json& data);
    };

    struct ComponentSchema
    {
        std::string componentName;
        std::string category;
        std::string description;
        std::vector<ParameterDefinition> parameters;
        bool isDeprecated;

        ComponentSchema()
            : isDeprecated(false)
        {
        }

        explicit ComponentSchema(const std::string& name)
            : componentName(name), isDeprecated(false)
        {
        }

        json ToJson() const;
        static ComponentSchema FromJson(const json& data);
    };

    struct ComponentData
    {
        std::string componentType;
        std::string componentName;
        std::map<std::string, std::string> properties;
        bool enabled;

        ComponentData()
            : enabled(true)
        {
        }

        explicit ComponentData(const std::string& type)
            : componentType(type), enabled(true)
        {
        }

        json ToJson() const;
        static ComponentData FromJson(const json& data);
    };

    struct EntityPrefab
    {
        std::string prefabName;
        std::string prefabPath;
        std::vector<ComponentData> components;
        std::map<std::string, std::string> metadata;
        int schemaVersion;

        EntityPrefab()
            : schemaVersion(4)
        {
        }

        explicit EntityPrefab(const std::string& name)
            : prefabName(name), schemaVersion(4)
        {
        }

        json ToJson() const;
        static EntityPrefab FromJson(const json& data);
    };

    class PrefabLoader
    {
    public:
        static EntityPrefab LoadFromFile(const std::string& filePath);
        static void SaveToFile(const std::string& filePath, const EntityPrefab& prefab);

        static json LoadJsonFromFile(const std::string& filePath);
        static void SaveJsonToFile(const std::string& filePath, const json& data);

        static ComponentSchema ParseComponentSchema(const json& schemaJson);
        static std::vector<ComponentSchema> LoadAllSchemas(const std::string& schemasPath);

        static bool ValidatePrefab(const EntityPrefab& prefab);
        static bool ValidateComponentData(const ComponentData& component, const ComponentSchema& schema);

    private:
        PrefabLoader() = default;
        static std::string GetSchemaVersion(const json& data);
        static ComponentSchema ParseSchemaV4(const json& data);
        static json SerializeSchemaV4(const EntityPrefab& prefab);
    };
}
