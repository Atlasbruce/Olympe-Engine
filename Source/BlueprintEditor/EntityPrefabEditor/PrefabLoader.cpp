#include "PrefabLoader.h"
#include <fstream>

namespace Olympe
{
    using json = nlohmann::json;

    json ParameterDefinition::ToJson() const { json j; j["name"] = name; j["type"] = static_cast<int>(type); j["defaultValue"] = defaultValue; j["description"] = description; j["isRequired"] = isRequired; return j; }
    ParameterDefinition ParameterDefinition::FromJson(const json& data) { ParameterDefinition p; if (data.contains("name")) { p.name = data["name"].get<std::string>(); } if (data.contains("defaultValue")) { p.defaultValue = data["defaultValue"].get<std::string>(); } if (data.contains("isRequired")) { p.isRequired = data["isRequired"].get<bool>(); } return p; }

    json ComponentSchema::ToJson() const { json j; j["componentName"] = componentName; j["category"] = category; j["description"] = description; j["isDeprecated"] = isDeprecated; j["parameters"] = json::array(); for (size_t i = 0; i < parameters.size(); ++i) { j["parameters"].push_back(parameters[i].ToJson()); } return j; }
    ComponentSchema ComponentSchema::FromJson(const json& data) { ComponentSchema s; if (data.contains("componentName")) { s.componentName = data["componentName"].get<std::string>(); } if (data.contains("category")) { s.category = data["category"].get<std::string>(); } if (data.contains("description")) { s.description = data["description"].get<std::string>(); } if (data.contains("isDeprecated")) { s.isDeprecated = data["isDeprecated"].get<bool>(); } return s; }

    json ComponentData::ToJson() const { json j; j["componentType"] = componentType; j["componentName"] = componentName; j["enabled"] = enabled; j["properties"] = json::object(); for (auto it = properties.begin(); it != properties.end(); ++it) { j["properties"][it->first] = it->second; } return j; }
    ComponentData ComponentData::FromJson(const json& data) { ComponentData c; if (data.contains("componentType")) { c.componentType = data["componentType"].get<std::string>(); } if (data.contains("componentName")) { c.componentName = data["componentName"].get<std::string>(); } if (data.contains("enabled")) { c.enabled = data["enabled"].get<bool>(); } return c; }

    json EntityPrefab::ToJson() const { json j; j["prefabName"] = prefabName; j["prefabPath"] = prefabPath; j["schemaVersion"] = schemaVersion; j["components"] = json::array(); for (size_t i = 0; i < components.size(); ++i) { j["components"].push_back(components[i].ToJson()); } j["metadata"] = json::object(); for (auto it = metadata.begin(); it != metadata.end(); ++it) { j["metadata"][it->first] = it->second; } return j; }
    EntityPrefab EntityPrefab::FromJson(const json& data) { EntityPrefab p; if (data.contains("prefabName")) { p.prefabName = data["prefabName"].get<std::string>(); } if (data.contains("prefabPath")) { p.prefabPath = data["prefabPath"].get<std::string>(); } if (data.contains("schemaVersion")) { p.schemaVersion = data["schemaVersion"].get<int>(); } return p; }

    EntityPrefab PrefabLoader::LoadFromFile(const std::string& filePath) { EntityPrefab p; json d = LoadJsonFromFile(filePath); return EntityPrefab::FromJson(d); }
    void PrefabLoader::SaveToFile(const std::string& filePath, const EntityPrefab& prefab) { json d = prefab.ToJson(); SaveJsonToFile(filePath, d); }
    json PrefabLoader::LoadJsonFromFile(const std::string& filePath) { std::ifstream f(filePath); if (f.is_open()) { try { json d; f >> d; f.close(); return d; } catch (...) { f.close(); } } return json::object(); }
    void PrefabLoader::SaveJsonToFile(const std::string& filePath, const json& data) { std::ofstream f(filePath); if (f.is_open()) { f << data.dump(2); f.close(); } }
    ComponentSchema PrefabLoader::ParseComponentSchema(const json& schemaJson) { return ComponentSchema::FromJson(schemaJson); }
    std::vector<ComponentSchema> PrefabLoader::LoadAllSchemas(const std::string& schemasPath) { (void)schemasPath; return std::vector<ComponentSchema>(); }
    bool PrefabLoader::ValidatePrefab(const EntityPrefab& prefab) { (void)prefab; return true; }
    bool PrefabLoader::ValidateComponentData(const ComponentData& component, const ComponentSchema& schema) { (void)component; (void)schema; return true; }
    std::string PrefabLoader::GetSchemaVersion(const json& data) { if (data.contains("schemaVersion")) { return std::to_string(data["schemaVersion"].get<int>()); } return "1"; }
    ComponentSchema PrefabLoader::ParseSchemaV4(const json& data) { return ParseComponentSchema(data); }
    json PrefabLoader::SerializeSchemaV4(const EntityPrefab& prefab) { return prefab.ToJson(); }

} // namespace Olympe