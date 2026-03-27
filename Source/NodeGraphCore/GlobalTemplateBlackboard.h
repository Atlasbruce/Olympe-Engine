/**
 * @file GlobalTemplateBlackboard.h
 * @brief Project-scope blackboard variable registry
 * @author Olympe Engine
 * @date 2026-03-26
 */

#ifndef OLYMPE_GLOBAL_TEMPLATE_BLACKBOARD_H
#define OLYMPE_GLOBAL_TEMPLATE_BLACKBOARD_H

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "../TaskSystem/TaskGraphTypes.h"

namespace Olympe {

// ============================================================================
// VariableType conversion helpers (local to this component)
// ============================================================================

static inline std::string VariableTypeToString(VariableType type)
{
    switch (type)
    {
        case VariableType::Bool:     return "Bool";
        case VariableType::Int:      return "Int";
        case VariableType::Float:    return "Float";
        case VariableType::String:   return "String";
        case VariableType::Vector:   return "Vector";
        case VariableType::EntityID: return "EntityID";
        case VariableType::None:
        default:                     return "None";
    }
}

static inline VariableType StringToVariableType(const std::string& str)
{
    if (str == "Bool")     return VariableType::Bool;
    if (str == "Int")      return VariableType::Int;
    if (str == "Float")    return VariableType::Float;
    if (str == "String")   return VariableType::String;
    if (str == "Vector")   return VariableType::Vector;
    if (str == "EntityID") return VariableType::EntityID;
    return VariableType::None;
}

static inline TaskValue GetDefaultValueForType(VariableType type)
{
    switch (type)
    {
        case VariableType::Bool:
            return TaskValue(false);
        case VariableType::Int:
            return TaskValue(0);
        case VariableType::Float:
            return TaskValue(0.0f);
        case VariableType::String:
            return TaskValue(std::string(""));
        case VariableType::Vector:
            return TaskValue(::Vector{0.0f, 0.0f, 0.0f});
        case VariableType::EntityID:
            return TaskValue(0);
        case VariableType::None:
        default:
            return TaskValue();
    }
}

struct GlobalEntryDefinition {
    std::string Key;
    VariableType Type = VariableType::None;
    TaskValue DefaultValue;
    std::string Description;
    bool IsPersistent = false;
};

class GlobalTemplateBlackboard {
public:
    static GlobalTemplateBlackboard& Get();

    /// Force reload of the registry from file (useful for hot reload)
    static void Reload();

    bool LoadFromFile(const std::string& configPath = "./Config/global_blackboard_register.json");
    bool SaveToFile(const std::string& configPath = "") const;  // Empty = use last loaded path
    void Clear();

    bool AddVariable(const std::string& key,
                     VariableType type,
                     const TaskValue& defaultValue,
                     const std::string& description = "",
                     bool isPersistent = false);

    bool UpdateVariable(const std::string& key,
                        const TaskValue& defaultValue,
                        const std::string& description = "");

    bool RemoveVariable(const std::string& key);

    bool HasVariable(const std::string& key) const;
    const GlobalEntryDefinition* GetVariable(const std::string& key) const;
    VariableType GetVariableType(const std::string& key) const;
    TaskValue GetDefaultValue(const std::string& key) const;
    const std::vector<GlobalEntryDefinition>& GetAllVariables() const;
    size_t GetVariableCount() const;

    static bool IsTypeCompatible(VariableType existingType, VariableType newType);
    bool HasConflicts() const;

    std::string DebugSummary() const;
    void DebugPrint() const;

private:
    GlobalTemplateBlackboard() = default;
    ~GlobalTemplateBlackboard() = default;

    GlobalTemplateBlackboard(const GlobalTemplateBlackboard&) = delete;
    GlobalTemplateBlackboard& operator=(const GlobalTemplateBlackboard&) = delete;
    GlobalTemplateBlackboard(GlobalTemplateBlackboard&&) = delete;
    GlobalTemplateBlackboard& operator=(GlobalTemplateBlackboard&&) = delete;

    static bool IsValidVariableName(const std::string& name);

    /// Track the last successfully loaded path for consistent saves
    mutable std::string m_lastLoadedPath;

    std::unordered_map<std::string, size_t> m_nameToIndex;
    std::vector<GlobalEntryDefinition> m_variables;
};

} // namespace Olympe

#endif // OLYMPE_GLOBAL_TEMPLATE_BLACKBOARD_H
