/**
 * @file GlobalBlackboard.cpp
 * @brief Implémentation de GlobalBlackboard — Phase 1.3
 * @author Olympe Engine / Atlasbruce
 * @date 2026-03-08
 */

#include "GlobalBlackboard.h"
#include "../system/system_utils.h"

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Singleton
// ============================================================================

GlobalBlackboard& GlobalBlackboard::Get()
{
    static GlobalBlackboard instance;
    return instance;
}

// ============================================================================
// Persistance JSON
// ============================================================================

bool GlobalBlackboard::LoadFromFile(const std::string& filepath)
{
    json j;
    if (!JsonHelper::LoadJsonFromFile(filepath, j))
    {
        SYSTEM_LOG << "[GlobalBlackboard] Failed to load: " << filepath << std::endl;
        return false;
    }

    if (!FromJson(j))
    {
        SYSTEM_LOG << "[GlobalBlackboard] Failed to parse: " << filepath << std::endl;
        return false;
    }

    m_filePath = filepath;
    m_isDirty  = false;
    SYSTEM_LOG << "[GlobalBlackboard] Loaded from: " << filepath
               << " (" << m_blackboard.GetAll().size() << " variables)" << std::endl;
    return true;
}

bool GlobalBlackboard::SaveToFile(const std::string& filepath) const
{
    json j = ToJson();
    if (!JsonHelper::SaveJsonToFile(filepath, j))
    {
        SYSTEM_LOG << "[GlobalBlackboard] Failed to save: " << filepath << std::endl;
        return false;
    }
    SYSTEM_LOG << "[GlobalBlackboard] Saved to: " << filepath << std::endl;
    return true;
}

// ============================================================================
// Accès BlackboardSystem
// ============================================================================

BlackboardSystem& GlobalBlackboard::GetBlackboard()
{
    return m_blackboard;
}

const BlackboardSystem& GlobalBlackboard::GetBlackboard() const
{
    return m_blackboard;
}

// ============================================================================
// Raccourcis
// ============================================================================

bool GlobalBlackboard::HasVar(const std::string& name) const
{
    return m_blackboard.HasEntry(name);
}

const BlackboardValue* GlobalBlackboard::GetVar(const std::string& name) const
{
    return m_blackboard.GetEntry(name);
}

bool GlobalBlackboard::SetVar(const std::string& name, const BlackboardValue& value)
{
    bool ok = m_blackboard.SetValue(name, value);
    if (ok)
    {
        m_isDirty = true;
    }
    return ok;
}

bool GlobalBlackboard::CreateVar(const std::string& name, BlackboardType type,
                                  const BlackboardValue& initialValue)
{
    bool ok = m_blackboard.CreateEntry(name, type, initialValue);
    if (ok)
    {
        m_isDirty = true;
    }
    return ok;
}

bool GlobalBlackboard::RemoveVar(const std::string& name)
{
    bool ok = m_blackboard.RemoveEntry(name);
    if (ok)
    {
        m_isDirty = true;
    }
    return ok;
}

bool GlobalBlackboard::RenameVar(const std::string& oldName, const std::string& newName)
{
    bool ok = m_blackboard.RenameEntry(oldName, newName);
    if (ok)
    {
        m_isDirty = true;
    }
    return ok;
}

// ============================================================================
// Métadonnées
// ============================================================================

std::string GlobalBlackboard::GetFilePath() const
{
    return m_filePath;
}

bool GlobalBlackboard::IsDirty() const
{
    return m_isDirty;
}

void GlobalBlackboard::MarkDirty()
{
    m_isDirty = true;
}

void GlobalBlackboard::ClearDirty()
{
    m_isDirty = false;
}

// ============================================================================
// Sérialisation JSON v2
// ============================================================================

json GlobalBlackboard::ToJson() const
{
    json j = json::object();
    j["schema_version"] = 2;
    j["type"]           = "GlobalBlackboard";

    // Metadata (author / timestamps left to the caller to fill)
    json meta = json::object();
    meta["description"] = "Variables globales partagées entre tous les graphes ATS";
    j["metadata"] = meta;

    // Variables — format plat (différent du format interne BlackboardSystem.ToJson())
    json varArray = json::array();
    const auto& entries = m_blackboard.GetAll();
    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        const std::string& name       = it->first;
        const BlackboardValue& val    = it->second;

        json entry = json::object();
        entry["name"] = name;

        switch (val.type)
        {
        case BlackboardType::Int:
            entry["type"]  = "Int";
            entry["value"] = val.intValue;
            break;
        case BlackboardType::Float:
            entry["type"]  = "Float";
            entry["value"] = val.floatValue;
            break;
        case BlackboardType::Bool:
            entry["type"]  = "Bool";
            entry["value"] = val.boolValue;
            break;
        case BlackboardType::String:
            entry["type"]  = "String";
            entry["value"] = val.stringValue;
            break;
        case BlackboardType::Vector3:
            // Sérialisé comme Vector (x, y) — le z est ignoré (ATS utilise 2D)
            entry["type"] = "Vector";
            entry["x"]    = val.vec3X;
            entry["y"]    = val.vec3Y;
            break;
        default:
            entry["type"]  = "Int";
            entry["value"] = 0;
            break;
        }
        varArray.push_back(entry);
    }
    j["variables"] = varArray;

    return j;
}

bool GlobalBlackboard::FromJson(const json& j)
{
    if (!j.is_object())
    {
        SYSTEM_LOG << "[GlobalBlackboard] FromJson: not a JSON object" << std::endl;
        return false;
    }

    int schemaVersion = JsonHelper::GetInt(j, "schema_version", 0);
    if (schemaVersion < 2)
    {
        SYSTEM_LOG << "[GlobalBlackboard] FromJson: unsupported schema_version "
                   << schemaVersion << std::endl;
        return false;
    }

    if (!j.contains("variables") || !j["variables"].is_array())
    {
        SYSTEM_LOG << "[GlobalBlackboard] FromJson: missing 'variables' array" << std::endl;
        return false;
    }

    // Vider le blackboard avant chargement
    const auto& currentEntries = m_blackboard.GetAll();
    std::vector<std::string> toRemove;
    for (auto it = currentEntries.begin(); it != currentEntries.end(); ++it)
    {
        toRemove.push_back(it->first);
    }
    for (size_t i = 0; i < toRemove.size(); ++i)
    {
        m_blackboard.RemoveEntry(toRemove[i]);
    }

    const json& variables = j["variables"];
    for (size_t i = 0; i < variables.size(); ++i)
    {
        const json& varJson = variables[i];
        if (!varJson.is_object())
        {
            continue;
        }

        std::string name    = JsonHelper::GetString(varJson, "name", "");
        std::string typeStr = JsonHelper::GetString(varJson, "type", "Int");

        if (name.empty())
        {
            continue;
        }

        BlackboardValue val;

        if (typeStr == "Int")
        {
            val.type     = BlackboardType::Int;
            val.intValue = JsonHelper::GetInt(varJson, "value", 0);
        }
        else if (typeStr == "Float")
        {
            val.type       = BlackboardType::Float;
            val.floatValue = JsonHelper::GetFloat(varJson, "value", 0.0f);
        }
        else if (typeStr == "Bool")
        {
            val.type      = BlackboardType::Bool;
            val.boolValue = JsonHelper::GetBool(varJson, "value", false);
        }
        else if (typeStr == "String")
        {
            val.type        = BlackboardType::String;
            val.stringValue = JsonHelper::GetString(varJson, "value", "");
        }
        else if (typeStr == "Vector")
        {
            // Format plat { "x": 0.0, "y": 0.0 } — stocké en Vector3 avec z=0
            val.type  = BlackboardType::Vector3;
            val.vec3X = JsonHelper::GetFloat(varJson, "x", 0.0f);
            val.vec3Y = JsonHelper::GetFloat(varJson, "y", 0.0f);
            val.vec3Z = 0.0f;
        }
        else
        {
            SYSTEM_LOG << "[GlobalBlackboard] Unknown type '" << typeStr
                       << "' for variable '" << name << "' — defaulting to Int" << std::endl;
            val.type     = BlackboardType::Int;
            val.intValue = 0;
        }

        m_blackboard.CreateEntry(name, val.type, val);
    }

    return true;
}

} // namespace NodeGraph
} // namespace Olympe
