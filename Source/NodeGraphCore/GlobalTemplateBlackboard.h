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
// StringToVariableType is defined here; VariableTypeToString and
// GetDefaultValueForType are provided by TaskGraphTypes.h / TaskGraphTypes.cpp.
// ============================================================================

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
