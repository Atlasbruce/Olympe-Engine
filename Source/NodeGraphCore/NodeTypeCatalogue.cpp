/**
 * @file NodeTypeCatalogue.cpp
 * @brief Implémentation de NodeTypeCatalogue — Phase 1.2
 * @author Olympe Engine / Atlasbruce
 * @date 2026-03-08
 */

#include "NodeTypeCatalogue.h"
#include "../system/system_utils.h"
#include <algorithm>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#else
#  include <dirent.h>
#  include <sys/stat.h>
#endif

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Singleton
// ============================================================================

NodeTypeCatalogue& NodeTypeCatalogue::Get()
{
    static NodeTypeCatalogue instance;
    return instance;
}

// ============================================================================
// Chargement
// ============================================================================

bool NodeTypeCatalogue::LoadFromDirectory(const std::string& cataloguePath)
{
    m_cataloguePath = cataloguePath;
    m_types.clear();

    SYSTEM_LOG << "[NodeTypeCatalogue] Loading catalogues from: " << cataloguePath << std::endl;

    int loaded = 0;

#ifdef _WIN32
    // Windows: _findfirst / _findnext
    std::string pattern = cataloguePath + "/*.json";
    struct _finddata_t fd;
    intptr_t handle = _findfirst(pattern.c_str(), &fd);
    if (handle == -1)
    {
        SYSTEM_LOG << "[NodeTypeCatalogue] No JSON files found in: " << cataloguePath << std::endl;
        return false;
    }
    do
    {
        std::string filename(fd.name);
        // Skip backup files
        if (filename.find(".backup") != std::string::npos)
        {
            continue;
        }
        std::string fullPath = cataloguePath + "/" + filename;
        if (LoadCatalogueFile(fullPath))
        {
            ++loaded;
        }
    }
    while (_findnext(handle, &fd) == 0);
    _findclose(handle);
#else
    // POSIX: opendir / readdir
    DIR* dir = opendir(cataloguePath.c_str());
    if (dir == nullptr)
    {
        SYSTEM_LOG << "[NodeTypeCatalogue] Cannot open directory: " << cataloguePath << std::endl;
        return false;
    }
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename(entry->d_name);
        // Skip non-JSON and backup files
        if (filename.size() < 5)
        {
            continue;
        }
        if (filename.substr(filename.size() - 5) != ".json")
        {
            continue;
        }
        if (filename.find(".backup") != std::string::npos)
        {
            continue;
        }
        std::string fullPath = cataloguePath + "/" + filename;
        if (LoadCatalogueFile(fullPath))
        {
            ++loaded;
        }
    }
    closedir(dir);
#endif

    SYSTEM_LOG << "[NodeTypeCatalogue] Loaded " << loaded << " catalogue(s), "
               << m_types.size() << " types total" << std::endl;
    return loaded > 0;
}

bool NodeTypeCatalogue::LoadCatalogueFile(const std::string& filePath)
{
    json j;
    if (!JsonHelper::LoadJsonFromFile(filePath, j))
    {
        SYSTEM_LOG << "[NodeTypeCatalogue] Failed to load: " << filePath << std::endl;
        return false;
    }

    // Validate schema version
    int schemaVersion = JsonHelper::GetInt(j, "schema_version", 0);
    if (schemaVersion < 2)
    {
        SYSTEM_LOG << "[NodeTypeCatalogue] Skipping legacy catalogue (schema_version="
                   << schemaVersion << "): " << filePath << std::endl;
        return false;
    }

    // Determine catalogType from file or field
    std::string catalogType = JsonHelper::GetString(j, "catalogType", "");
    if (catalogType.empty())
    {
        SYSTEM_LOG << "[NodeTypeCatalogue] Missing 'catalogType' in: " << filePath << std::endl;
        return false;
    }

    if (!j.contains("types") || !j["types"].is_array())
    {
        SYSTEM_LOG << "[NodeTypeCatalogue] Missing 'types' array in: " << filePath << std::endl;
        return false;
    }

    size_t before = m_types.size();
    ParseCatalogueJson(j, catalogType);
    size_t added = m_types.size() - before;

    SYSTEM_LOG << "[NodeTypeCatalogue] Loaded " << added << " types from: " << filePath << std::endl;
    return added > 0;
}

// ============================================================================
// Accès
// ============================================================================

const NodeTypeDescriptor* NodeTypeCatalogue::GetType(const std::string& id) const
{
    for (size_t i = 0; i < m_types.size(); ++i)
    {
        if (m_types[i].id == id)
        {
            return &m_types[i];
        }
    }
    return nullptr;
}

std::vector<const NodeTypeDescriptor*> NodeTypeCatalogue::GetByCatalogType(const std::string& catalogType) const
{
    std::vector<const NodeTypeDescriptor*> result;
    for (size_t i = 0; i < m_types.size(); ++i)
    {
        if (m_types[i].catalogType == catalogType)
        {
            result.push_back(&m_types[i]);
        }
    }
    return result;
}

std::vector<const NodeTypeDescriptor*> NodeTypeCatalogue::GetByCategory(const std::string& category) const
{
    std::vector<const NodeTypeDescriptor*> result;
    for (size_t i = 0; i < m_types.size(); ++i)
    {
        if (m_types[i].category == category)
        {
            result.push_back(&m_types[i]);
        }
    }
    return result;
}

const std::vector<NodeTypeDescriptor>& NodeTypeCatalogue::GetAll() const
{
    return m_types;
}

bool NodeTypeCatalogue::HasType(const std::string& id) const
{
    return GetType(id) != nullptr;
}

void NodeTypeCatalogue::Reload()
{
    if (m_cataloguePath.empty())
    {
        SYSTEM_LOG << "[NodeTypeCatalogue] Reload: no catalogue path set" << std::endl;
        return;
    }
    LoadFromDirectory(m_cataloguePath);
}

size_t NodeTypeCatalogue::Count() const
{
    return m_types.size();
}

// ============================================================================
// Parsing JSON v2
// ============================================================================

void NodeTypeCatalogue::ParseCatalogueJson(const json& j, const std::string& catalogType)
{
    const json& typesArray = j["types"];

    for (size_t i = 0; i < typesArray.size(); ++i)
    {
        const json& typeJson = typesArray[i];
        if (!typeJson.is_object())
        {
            continue;
        }

        NodeTypeDescriptor desc;
        desc.id          = JsonHelper::GetString(typeJson, "id", "");
        desc.name        = JsonHelper::GetString(typeJson, "name", "");
        desc.category    = JsonHelper::GetString(typeJson, "category", "");
        desc.catalogType = catalogType;
        desc.description = JsonHelper::GetString(typeJson, "description", "");
        desc.tooltip     = JsonHelper::GetString(typeJson, "tooltip", "");
        desc.color       = JsonHelper::GetString(typeJson, "color", "#888888");

        if (desc.id.empty())
        {
            SYSTEM_LOG << "[NodeTypeCatalogue] Skipping type with empty id" << std::endl;
            continue;
        }

        // Pins
        if (typeJson.contains("pins") && typeJson["pins"].is_object())
        {
            const json& pinsJson = typeJson["pins"];

            // Input pins
            if (pinsJson.contains("inputs") && pinsJson["inputs"].is_array())
            {
                const json& inputsArr = pinsJson["inputs"];
                for (size_t p = 0; p < inputsArr.size(); ++p)
                {
                    const json& pinJson = inputsArr[p];
                    PinDescriptor pin;
                    pin.id       = JsonHelper::GetString(pinJson, "id", "");
                    pin.label    = JsonHelper::GetString(pinJson, "label", "");
                    pin.type     = StringToPinType(JsonHelper::GetString(pinJson, "type", "Any"));
                    pin.required = JsonHelper::GetBool(pinJson, "required", false);
                    desc.inputPins.push_back(pin);
                }
            }

            // Output pins
            if (pinsJson.contains("outputs") && pinsJson["outputs"].is_array())
            {
                const json& outputsArr = pinsJson["outputs"];
                for (size_t p = 0; p < outputsArr.size(); ++p)
                {
                    const json& pinJson = outputsArr[p];
                    PinDescriptor pin;
                    pin.id       = JsonHelper::GetString(pinJson, "id", "");
                    pin.label    = JsonHelper::GetString(pinJson, "label", "");
                    pin.type     = StringToPinType(JsonHelper::GetString(pinJson, "type", "Any"));
                    pin.required = JsonHelper::GetBool(pinJson, "required", false);
                    desc.outputPins.push_back(pin);
                }
            }
        }

        // Parameters
        if (typeJson.contains("parameters") && typeJson["parameters"].is_array())
        {
            const json& paramsArr = typeJson["parameters"];
            for (size_t p = 0; p < paramsArr.size(); ++p)
            {
                const json& paramJson = paramsArr[p];
                ParameterDescriptor param;
                param.name         = JsonHelper::GetString(paramJson, "name", "");
                param.type         = StringToPinType(JsonHelper::GetString(paramJson, "type", "Any"));
                param.required     = JsonHelper::GetBool(paramJson, "required", false);
                param.defaultValue = JsonHelper::GetString(paramJson, "default", "");
                param.description  = JsonHelper::GetString(paramJson, "description", "");
                desc.parameters.push_back(param);
            }
        }

        // Skip duplicate IDs (first wins)
        if (HasType(desc.id))
        {
            SYSTEM_LOG << "[NodeTypeCatalogue] Duplicate type id '" << desc.id
                       << "' — keeping first registration" << std::endl;
            continue;
        }

        m_types.push_back(desc);
    }
}

// ============================================================================
// Conversion PinType <-> string
// ============================================================================

PinType NodeTypeCatalogue::StringToPinType(const std::string& s)
{
    if (s == "Exec")      return PinType::Exec;
    if (s == "Int")       return PinType::Int;
    if (s == "Float")     return PinType::Float;
    if (s == "Bool")      return PinType::Bool;
    if (s == "String")    return PinType::String;
    if (s == "Vector")    return PinType::Vector;
    if (s == "EntityRef") return PinType::EntityRef;
    if (s == "GraphRef")  return PinType::GraphRef;
    if (s == "AnimRef")   return PinType::AnimRef;
    if (s == "SpriteRef") return PinType::SpriteRef;
    return PinType::Any;
}

std::string NodeTypeCatalogue::PinTypeToString(PinType t)
{
    switch (t)
    {
    case PinType::Exec:      return "Exec";
    case PinType::Int:       return "Int";
    case PinType::Float:     return "Float";
    case PinType::Bool:      return "Bool";
    case PinType::String:    return "String";
    case PinType::Vector:    return "Vector";
    case PinType::EntityRef: return "EntityRef";
    case PinType::GraphRef:  return "GraphRef";
    case PinType::AnimRef:   return "AnimRef";
    case PinType::SpriteRef: return "SpriteRef";
    case PinType::Any:       return "Any";
    default:                 return "Any";
    }
}

} // namespace NodeGraph
} // namespace Olympe
