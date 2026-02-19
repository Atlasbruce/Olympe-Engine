/**
 * @file BlackboardSystem.cpp
 * @brief Implementation of BlackboardSystem (Phase 2.1)
 * @author Olympe Engine
 * @date 2026-02-19
 */

#include "BlackboardSystem.h"
#include "../system/system_utils.h"

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Query
// ============================================================================

bool BlackboardSystem::HasEntry(const std::string& name) const
{
    return m_entries.find(name) != m_entries.end();
}

const BlackboardValue* BlackboardSystem::GetEntry(const std::string& name) const
{
    auto it = m_entries.find(name);
    if (it != m_entries.end())
    {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// CRUD
// ============================================================================

bool BlackboardSystem::CreateEntry(const std::string& name, BlackboardType type,
                                   const BlackboardValue& initialValue)
{
    if (name.empty())
    {
        SYSTEM_LOG << "[BlackboardSystem] CreateEntry failed: empty name" << std::endl;
        return false;
    }

    if (HasEntry(name))
    {
        SYSTEM_LOG << "[BlackboardSystem] CreateEntry failed: duplicate name '"
                   << name << "'" << std::endl;
        return false;
    }

    BlackboardValue val = initialValue;
    val.type = type;
    m_entries[name] = val;
    SYSTEM_LOG << "[BlackboardSystem] Created entry '" << name
               << "' type=" << TypeToString(type) << std::endl;
    return true;
}

bool BlackboardSystem::RemoveEntry(const std::string& name)
{
    auto it = m_entries.find(name);
    if (it == m_entries.end())
    {
        SYSTEM_LOG << "[BlackboardSystem] RemoveEntry: not found '" << name << "'" << std::endl;
        return false;
    }
    m_entries.erase(it);
    SYSTEM_LOG << "[BlackboardSystem] Removed entry '" << name << "'" << std::endl;
    return true;
}

bool BlackboardSystem::RenameEntry(const std::string& oldName, const std::string& newName)
{
    if (newName.empty())
    {
        SYSTEM_LOG << "[BlackboardSystem] RenameEntry failed: empty new name" << std::endl;
        return false;
    }

    auto it = m_entries.find(oldName);
    if (it == m_entries.end())
    {
        SYSTEM_LOG << "[BlackboardSystem] RenameEntry: source not found '" << oldName << "'" << std::endl;
        return false;
    }

    if (HasEntry(newName))
    {
        SYSTEM_LOG << "[BlackboardSystem] RenameEntry failed: target name already exists '"
                   << newName << "'" << std::endl;
        return false;
    }

    BlackboardValue val = it->second;
    m_entries.erase(it);
    m_entries[newName] = val;
    SYSTEM_LOG << "[BlackboardSystem] Renamed '" << oldName << "' -> '" << newName << "'" << std::endl;
    return true;
}

bool BlackboardSystem::SetValue(const std::string& name, const BlackboardValue& value)
{
    auto it = m_entries.find(name);
    if (it == m_entries.end())
    {
        SYSTEM_LOG << "[BlackboardSystem] SetValue: not found '" << name << "'" << std::endl;
        return false;
    }

    if (it->second.type != value.type)
    {
        SYSTEM_LOG << "[BlackboardSystem] SetValue: type mismatch for '" << name << "'" << std::endl;
        return false;
    }

    it->second = value;
    return true;
}

// ============================================================================
// Iteration
// ============================================================================

const std::map<std::string, BlackboardValue>& BlackboardSystem::GetAll() const
{
    return m_entries;
}

// ============================================================================
// Serialization
// ============================================================================

json BlackboardSystem::ToJson() const
{
    json arr = json::array();
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        const std::string& name = it->first;
        const BlackboardValue& val = it->second;

        json entry = json::object();
        entry["name"] = name;
        entry["type"] = TypeToString(val.type);

        json valObj = json::object();
        switch (val.type)
        {
        case BlackboardType::Int:
            valObj["int"] = val.intValue;
            break;
        case BlackboardType::Float:
            valObj["float"] = val.floatValue;
            break;
        case BlackboardType::Bool:
            valObj["bool"] = val.boolValue;
            break;
        case BlackboardType::String:
            valObj["string"] = val.stringValue;
            break;
        case BlackboardType::Vector3:
            valObj["x"] = val.vec3X;
            valObj["y"] = val.vec3Y;
            valObj["z"] = val.vec3Z;
            break;
        default:
            break;
        }
        entry["value"] = valObj;
        arr.push_back(entry);
    }
    return arr;
}

void BlackboardSystem::FromJson(const json& j)
{
    m_entries.clear();

    if (!j.is_array())
    {
        return;
    }

    for (size_t i = 0; i < j.size(); ++i)
    {
        const json& entry = j[i];
        if (!entry.is_object())
        {
            continue;
        }

        std::string name = JsonHelper::GetString(entry, "name", "");
        if (name.empty())
        {
            continue;
        }

        std::string typeStr = JsonHelper::GetString(entry, "type", "Int");
        BlackboardType type = StringToType(typeStr);

        BlackboardValue val;
        val.type = type;

        if (entry.contains("value") && entry["value"].is_object())
        {
            const json& v = entry["value"];
            switch (type)
            {
            case BlackboardType::Int:
                val.intValue = JsonHelper::GetInt(v, "int", 0);
                break;
            case BlackboardType::Float:
                val.floatValue = JsonHelper::GetFloat(v, "float", 0.0f);
                break;
            case BlackboardType::Bool:
                val.boolValue = JsonHelper::GetBool(v, "bool", false);
                break;
            case BlackboardType::String:
                val.stringValue = JsonHelper::GetString(v, "string", "");
                break;
            case BlackboardType::Vector3:
                val.vec3X = JsonHelper::GetFloat(v, "x", 0.0f);
                val.vec3Y = JsonHelper::GetFloat(v, "y", 0.0f);
                val.vec3Z = JsonHelper::GetFloat(v, "z", 0.0f);
                break;
            default:
                break;
            }
        }

        m_entries[name] = val;
    }
}

// ============================================================================
// Private helpers
// ============================================================================

std::string BlackboardSystem::TypeToString(BlackboardType t)
{
    switch (t)
    {
    case BlackboardType::Int:     return "Int";
    case BlackboardType::Float:   return "Float";
    case BlackboardType::Bool:    return "Bool";
    case BlackboardType::String:  return "String";
    case BlackboardType::Vector3: return "Vector3";
    default:                      return "Int";
    }
}

BlackboardType BlackboardSystem::StringToType(const std::string& s)
{
    if (s == "Float")   return BlackboardType::Float;
    if (s == "Bool")    return BlackboardType::Bool;
    if (s == "String")  return BlackboardType::String;
    if (s == "Vector3") return BlackboardType::Vector3;
    return BlackboardType::Int;
}

} // namespace NodeGraph
} // namespace Olympe
