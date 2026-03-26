/**
 * @file GlobalTemplateBlackboard.cpp
 * @brief Implementation of GlobalTemplateBlackboard singleton
 * @author Olympe Engine
 * @date 2026-03-26
 */

#include "GlobalTemplateBlackboard.h"
#include "../system/system_utils.h"
#include "../json_helper.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace Olympe {

GlobalTemplateBlackboard& GlobalTemplateBlackboard::Get()
{
    static GlobalTemplateBlackboard instance;
    static bool initialized = false;

    // Auto-load register on first access
    if (!initialized)
    {
        initialized = true;
        if (instance.m_variables.empty())
        {
            SYSTEM_LOG << "[GlobalTemplateBlackboard::Get] Auto-loading register from file\n";
            if (!instance.LoadFromFile("./Config/global_blackboard_register.json"))
            {
                SYSTEM_LOG << "[GlobalTemplateBlackboard::Get] WARNING: Failed to load register file\n";
                // Don't fail - just start with empty registry
            }
        }
    }

    return instance;
}

bool GlobalTemplateBlackboard::LoadFromFile(const std::string& configPath)
{
    SYSTEM_LOG << "[GlobalTemplateBlackboard] LoadFromFile: '" << configPath << "'\n";
    Clear();

    std::ifstream ifs(configPath);
    if (!ifs.is_open())
    {
        SYSTEM_LOG << "[GlobalTemplateBlackboard] LoadFromFile FAILED: file not found\n";
        return false;
    }

    try
    {
        json root;
        ifs >> root;
        ifs.close();

        if (!root.contains("variables") || !root["variables"].is_array())
        {
            SYSTEM_LOG << "[GlobalTemplateBlackboard] No variables array found\n";
            return true;
        }

        const json& varsArray = root["variables"];
        for (const auto& varObj : varsArray)
        {
            if (!varObj.contains("key") || !varObj["key"].is_string())
                continue;

            const std::string key = varObj["key"].get<std::string>();
            VariableType type = VariableType::None;

            if (varObj.contains("type") && varObj["type"].is_string())
                type = StringToVariableType(varObj["type"].get<std::string>());

            if (type == VariableType::None)
                continue;

            TaskValue defaultValue = GetDefaultValueForType(type);
            if (varObj.contains("defaultValue"))
            {
                try
                {
                    const auto& valNode = varObj["defaultValue"];
                    switch (type)
                    {
                        case VariableType::Bool:
                            if (valNode.is_boolean())
                                defaultValue = TaskValue(valNode.get<bool>());
                            break;
                        case VariableType::Int:
                            if (valNode.is_number_integer())
                                defaultValue = TaskValue(valNode.get<int>());
                            break;
                        case VariableType::Float:
                            if (valNode.is_number())
                                defaultValue = TaskValue(static_cast<float>(valNode.get<double>()));
                            break;
                        case VariableType::String:
                            if (valNode.is_string())
                                defaultValue = TaskValue(valNode.get<std::string>());
                            break;
                        case VariableType::Vector:
                            if (valNode.is_object() && valNode.contains("x") && valNode.contains("y") && valNode.contains("z"))
                            {
                                const float x = valNode["x"].get<float>();
                                const float y = valNode["y"].get<float>();
                                const float z = valNode["z"].get<float>();
                                defaultValue = TaskValue(::Vector{x, y, z});
                            }
                            break;
                        case VariableType::EntityID:
                            if (valNode.is_number_integer())
                                defaultValue = TaskValue(valNode.get<int>());
                            break;
                        default:
                            break;
                    }
                }
                catch (...) {}
            }
            // Phase 24: Also check "value" field (backward compat with register file format)
            else if (varObj.contains("value"))
            {
                try
                {
                    const auto& valNode = varObj["value"];
                    switch (type)
                    {
                        case VariableType::Bool:
                            if (valNode.is_boolean())
                                defaultValue = TaskValue(valNode.get<bool>());
                            break;
                        case VariableType::Int:
                            if (valNode.is_number_integer())
                                defaultValue = TaskValue(valNode.get<int>());
                            break;
                        case VariableType::Float:
                            if (valNode.is_number())
                                defaultValue = TaskValue(static_cast<float>(valNode.get<double>()));
                            break;
                        case VariableType::String:
                            if (valNode.is_string())
                                defaultValue = TaskValue(valNode.get<std::string>());
                            break;
                        case VariableType::Vector:
                            if (valNode.is_object() && valNode.contains("x") && valNode.contains("y") && valNode.contains("z"))
                            {
                                const float x = valNode["x"].get<float>();
                                const float y = valNode["y"].get<float>();
                                const float z = valNode["z"].get<float>();
                                defaultValue = TaskValue(::Vector{x, y, z});
                            }
                            break;
                        case VariableType::EntityID:
                            if (valNode.is_number_integer())
                                defaultValue = TaskValue(valNode.get<int>());
                            break;
                        default:
                            break;
                    }
                }
                catch (...) {}
            }

            std::string description;
            if (varObj.contains("description") && varObj["description"].is_string())
                description = varObj["description"].get<std::string>();

            bool isPersistent = false;
            if (varObj.contains("isPersistent") && varObj["isPersistent"].is_boolean())
                isPersistent = varObj["isPersistent"].get<bool>();
            else if (varObj.contains("persistent") && varObj["persistent"].is_boolean())
                isPersistent = varObj["persistent"].get<bool>();

            AddVariable(key, type, defaultValue, description, isPersistent);
        }

        SYSTEM_LOG << "[GlobalTemplateBlackboard] Loaded " << m_variables.size() << " variables\n";
        return true;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[GlobalTemplateBlackboard] LoadFromFile EXCEPTION: " << e.what() << "\n";
        ifs.close();
        return false;
    }
}

bool GlobalTemplateBlackboard::SaveToFile(const std::string& configPath) const
{
    SYSTEM_LOG << "[GlobalTemplateBlackboard] SaveToFile: '" << configPath << "'\n";

    try
    {
        json root;
        root["version"] = 1;
        json varsArray = json::array();

        for (const auto& var : m_variables)
        {
            json varObj;
            varObj["key"] = var.Key;
            varObj["type"] = VariableTypeToString(var.Type);
            varObj["description"] = var.Description;
            varObj["persistent"] = var.IsPersistent;

            if (!var.DefaultValue.IsNone())
            {
                switch (var.Type)
                {
                    case VariableType::Bool:
                        varObj["defaultValue"] = var.DefaultValue.AsBool();
                        break;
                    case VariableType::Int:
                        varObj["defaultValue"] = var.DefaultValue.AsInt();
                        break;
                    case VariableType::Float:
                        varObj["defaultValue"] = var.DefaultValue.AsFloat();
                        break;
                    case VariableType::String:
                        varObj["defaultValue"] = var.DefaultValue.AsString();
                        break;
                    case VariableType::Vector:
                    {
                        const ::Vector v = var.DefaultValue.AsVector();
                        json vec;
                        vec["x"] = v.x;
                        vec["y"] = v.y;
                        vec["z"] = v.z;
                        varObj["defaultValue"] = vec;
                        break;
                    }
                    case VariableType::EntityID:
                        varObj["defaultValue"] = static_cast<int>(var.DefaultValue.AsEntityID());
                        break;
                    default:
                        break;
                }
            }

            varsArray.push_back(varObj);
        }

        root["variables"] = varsArray;

        std::ofstream ofs(configPath);
        if (!ofs.is_open())
        {
            SYSTEM_LOG << "[GlobalTemplateBlackboard] SaveToFile FAILED\n";
            return false;
        }

        ofs << root.dump(2);
        ofs.close();

        SYSTEM_LOG << "[GlobalTemplateBlackboard] SaveToFile SUCCESS\n";
        return true;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[GlobalTemplateBlackboard] SaveToFile EXCEPTION: " << e.what() << "\n";
        return false;
    }
}

void GlobalTemplateBlackboard::Clear()
{
    m_nameToIndex.clear();
    m_variables.clear();
}

bool GlobalTemplateBlackboard::AddVariable(const std::string& key,
                                           VariableType type,
                                           const TaskValue& defaultValue,
                                           const std::string& description,
                                           bool isPersistent)
{
    if (key.empty() || !IsValidVariableName(key) || type == VariableType::None)
        return false;

    if (!defaultValue.IsNone() && defaultValue.GetType() != type)
        return false;

    auto it = m_nameToIndex.find(key);
    if (it != m_nameToIndex.end())
    {
        if (m_variables[it->second].Type != type)
            return false;
        m_variables[it->second].DefaultValue = defaultValue;
        m_variables[it->second].Description = description;
        m_variables[it->second].IsPersistent = isPersistent;
        return true;
    }

    size_t newIndex = m_variables.size();
    GlobalEntryDefinition def;
    def.Key = key;
    def.Type = type;
    def.DefaultValue = defaultValue.IsNone() ? GetDefaultValueForType(type) : defaultValue;
    def.Description = description;
    def.IsPersistent = isPersistent;

    m_variables.push_back(def);
    m_nameToIndex[key] = newIndex;

    return true;
}

bool GlobalTemplateBlackboard::UpdateVariable(const std::string& key,
                                              const TaskValue& defaultValue,
                                              const std::string& description)
{
    auto it = m_nameToIndex.find(key);
    if (it == m_nameToIndex.end())
        return false;

    GlobalEntryDefinition& def = m_variables[it->second];

    if (!defaultValue.IsNone() && defaultValue.GetType() != def.Type)
        return false;

    def.DefaultValue = defaultValue;
    if (!description.empty())
        def.Description = description;

    return true;
}

bool GlobalTemplateBlackboard::RemoveVariable(const std::string& key)
{
    auto it = m_nameToIndex.find(key);
    if (it == m_nameToIndex.end())
        return false;

    size_t idx = it->second;
    m_variables.erase(m_variables.begin() + idx);
    m_nameToIndex.erase(it);

    m_nameToIndex.clear();
    for (size_t i = 0; i < m_variables.size(); ++i)
        m_nameToIndex[m_variables[i].Key] = i;

    return true;
}

bool GlobalTemplateBlackboard::HasVariable(const std::string& key) const
{
    return m_nameToIndex.find(key) != m_nameToIndex.end();
}

const GlobalEntryDefinition* GlobalTemplateBlackboard::GetVariable(const std::string& key) const
{
    auto it = m_nameToIndex.find(key);
    if (it == m_nameToIndex.end())
        return nullptr;
    return &m_variables[it->second];
}

VariableType GlobalTemplateBlackboard::GetVariableType(const std::string& key) const
{
    const GlobalEntryDefinition* def = GetVariable(key);
    return def ? def->Type : VariableType::None;
}

TaskValue GlobalTemplateBlackboard::GetDefaultValue(const std::string& key) const
{
    const GlobalEntryDefinition* def = GetVariable(key);
    return def ? def->DefaultValue : TaskValue();
}

const std::vector<GlobalEntryDefinition>& GlobalTemplateBlackboard::GetAllVariables() const
{
    return m_variables;
}

size_t GlobalTemplateBlackboard::GetVariableCount() const
{
    return m_variables.size();
}

bool GlobalTemplateBlackboard::IsTypeCompatible(VariableType existingType, VariableType newType)
{
    if (existingType == VariableType::None)
        return true;
    return existingType == newType;
}

bool GlobalTemplateBlackboard::HasConflicts() const
{
    for (const auto& var : m_variables)
    {
        if (var.Type == VariableType::None || var.Key.empty())
            return true;
    }
    return false;
}

std::string GlobalTemplateBlackboard::DebugSummary() const
{
    std::string summary = "[GlobalTemplateBlackboard] " + std::to_string(m_variables.size()) + " variables\n";
    for (const auto& var : m_variables)
    {
        summary += "  - " + var.Key + " (" + VariableTypeToString(var.Type) + ")\n";
    }
    return summary;
}

void GlobalTemplateBlackboard::DebugPrint() const
{
    SYSTEM_LOG << DebugSummary();
}

bool GlobalTemplateBlackboard::IsValidVariableName(const std::string& name)
{
    if (name.empty())
        return false;

    if (!std::isalpha(name[0]) && name[0] != '_')
        return false;

    for (size_t i = 1; i < name.length(); ++i)
    {
        if (!std::isalnum(name[i]) && name[i] != '_')
            return false;
    }

    return true;
}

} // namespace Olympe
