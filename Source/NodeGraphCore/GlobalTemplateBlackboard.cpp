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
#include <chrono>
#include <ctime>

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
            // Try multiple path variations
            const std::vector<std::string> pathsToTry = {
                "./Config/global_blackboard_register.json",
                "Config/global_blackboard_register.json",
                "../Config/global_blackboard_register.json",
                "../../Config/global_blackboard_register.json"
            };

            bool loaded = false;
            for (const auto& path : pathsToTry)
            {
                if (instance.LoadFromFile(path))
                {
                    loaded = true;
                    break;
                }
            }

            if (!loaded)
            {
                SYSTEM_LOG << "[GlobalTemplateBlackboard::Get] WARNING: Failed to load register from any path\n";
            }
        }
    }

    return instance;
}

void GlobalTemplateBlackboard::Reload()
{
    // Clear the current registry
    Get().Clear();

    // Force reload from file
    const std::vector<std::string> pathsToTry = {
        "./Config/global_blackboard_register.json",
        "Config/global_blackboard_register.json",
        "../Config/global_blackboard_register.json",
        "../../Config/global_blackboard_register.json"
    };

    bool loaded = false;
    for (const auto& path : pathsToTry)
    {
        if (Get().LoadFromFile(path))
        {
            SYSTEM_LOG << "[GlobalTemplateBlackboard::Reload] Successfully reloaded from: " << path << "\n";
            loaded = true;
            break;
        }
    }

    if (!loaded)
    {
        SYSTEM_LOG << "[GlobalTemplateBlackboard::Reload] WARNING: Failed to reload registry from any path\n";
    }
}

bool GlobalTemplateBlackboard::LoadFromFile(const std::string& configPath)
{
    Clear();

    std::ifstream ifs(configPath);
    if (!ifs.is_open())
    {
        return false;
    }

    try
    {
        json root;
        ifs >> root;
        ifs.close();

        if (!root.contains("variables") || !root["variables"].is_array())
        {
            return true;
        }

        const json& varsArray = root["variables"];
        SYSTEM_LOG << "[GlobalTemplateBlackboard::LoadFromFile] Found variables array with " << varsArray.size() << " entries\n";

        int loadedCount = 0;
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
            loadedCount++;
        }

        // Track the successfully loaded path for later saves
        m_lastLoadedPath = configPath;

        SYSTEM_LOG << "[GlobalTemplateBlackboard::LoadFromFile] Successfully loaded " << loadedCount << " variables\n";
        return true;
    }
    catch (const std::exception& e)
    {
        ifs.close();
        return false;
    }
}

bool GlobalTemplateBlackboard::SaveToFile(const std::string& configPath) const
{
    // If no path provided, use the last loaded path
    std::string pathToUse = configPath.empty() ? m_lastLoadedPath : configPath;

    // If still no path, use default
    if (pathToUse.empty())
    {
        pathToUse = "./Config/global_blackboard_register.json";
    }

    SYSTEM_LOG << "[GlobalTemplateBlackboard] SaveToFile: '" << pathToUse << "'\n";

    try
    {
        json root;

        // Try to preserve existing metadata from the file
        std::ifstream existingFile(pathToUse);
        if (existingFile.is_open())
        {
            try
            {
                json existingRoot;
                existingFile >> existingRoot;
                existingFile.close();

                // Preserve schema_version, name, description from existing file
                if (existingRoot.contains("schema_version"))
                    root["schema_version"] = existingRoot["schema_version"];
                else
                    root["schema_version"] = 1;

                if (existingRoot.contains("name"))
                    root["name"] = existingRoot["name"];
                else
                    root["name"] = "Global Blackboard Register";

                if (existingRoot.contains("description"))
                    root["description"] = existingRoot["description"];
                else
                    root["description"] = "Project-scope global variable definitions accessible to all entities";
            }
            catch (...)
            {
                // If we can't parse existing file, start fresh
                root["schema_version"] = 1;
                root["name"] = "Global Blackboard Register";
                root["description"] = "Project-scope global variable definitions accessible to all entities";
            }
        }
        else
        {
            // New file - set defaults
            root["schema_version"] = 1;
            root["name"] = "Global Blackboard Register";
            root["description"] = "Project-scope global variable definitions accessible to all entities";
        }

        // Get current timestamp in ISO8601 format
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        char timestamp[32];
        std::tm tm_info;
#ifdef _WIN32
        gmtime_s(&tm_info, &time_t_now);
#else
        tm_info = *gmtime(&time_t_now);
#endif
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &tm_info);
        root["lastModified"] = std::string(timestamp);

        // Build the variables array
        json varsArray = json::array();
        for (const auto& var : m_variables)
        {
            json varObj;
            varObj["key"] = var.Key;
            varObj["type"] = VariableTypeToString(var.Type);
            varObj["description"] = var.Description;
            varObj["isPersistent"] = var.IsPersistent;

            // Use "value" field for backward compatibility with existing registry
            if (!var.DefaultValue.IsNone())
            {
                switch (var.Type)
                {
                    case VariableType::Bool:
                        varObj["value"] = var.DefaultValue.AsBool();
                        break;
                    case VariableType::Int:
                        varObj["value"] = var.DefaultValue.AsInt();
                        break;
                    case VariableType::Float:
                        varObj["value"] = var.DefaultValue.AsFloat();
                        break;
                    case VariableType::String:
                        varObj["value"] = var.DefaultValue.AsString();
                        break;
                    case VariableType::Vector:
                    {
                        const ::Vector v = var.DefaultValue.AsVector();
                        json vec;
                        vec["x"] = v.x;
                        vec["y"] = v.y;
                        vec["z"] = v.z;
                        varObj["value"] = vec;
                        break;
                    }
                    case VariableType::EntityID:
                        varObj["value"] = static_cast<int>(var.DefaultValue.AsEntityID());
                        break;
                    default:
                        break;
                }
            }
            else
            {
                // Set default value even if DefaultValue is None
                switch (var.Type)
                {
                    case VariableType::Bool:
                        varObj["value"] = false;
                        break;
                    case VariableType::Int:
                        varObj["value"] = 0;
                        break;
                    case VariableType::Float:
                        varObj["value"] = 0.0f;
                        break;
                    case VariableType::String:
                        varObj["value"] = "";
                        break;
                    case VariableType::Vector:
                    {
                        json vec;
                        vec["x"] = 0.0f;
                        vec["y"] = 0.0f;
                        vec["z"] = 0.0f;
                        varObj["value"] = vec;
                        break;
                    }
                    case VariableType::EntityID:
                        varObj["value"] = 0;
                        break;
                    default:
                        break;
                }
            }

            varsArray.push_back(varObj);
        }

        root["variables"] = varsArray;

        std::ofstream ofs(pathToUse);
        if (!ofs.is_open())
        {
            SYSTEM_LOG << "[GlobalTemplateBlackboard] SaveToFile FAILED: Could not open file for writing\n";
            return false;
        }

        ofs << root.dump(2);
        ofs.close();

        SYSTEM_LOG << "[GlobalTemplateBlackboard] SaveToFile SUCCESS: Saved " << m_variables.size() << " variables to '" << pathToUse << "'\n";
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
    m_lastLoadedPath.clear();  // Reset path when clearing
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
