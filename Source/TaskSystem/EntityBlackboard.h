/**
 * @file EntityBlackboard.h
 * @brief Per-entity instance of global blackboard
 * @author Olympe Engine
 * @date 2026-03-26
 * 
 * @details Extends LocalBlackboard with global variable storage and scope-aware access.
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#ifndef OLYMPE_ENTITY_BLACKBOARD_H
#define OLYMPE_ENTITY_BLACKBOARD_H

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include "TaskGraphTypes.h"
#include "LocalBlackboard.h"
#include "../json_helper.h"

namespace Olympe {

// Forward declaration
class GlobalTemplateBlackboard;

class EntityBlackboard : public LocalBlackboard {
public:

    explicit EntityBlackboard(uint32_t entityID = 0);
    ~EntityBlackboard();

    void Initialize(const TaskGraphTemplate& tmpl,
                   const std::unordered_map<std::string, TaskValue>* globalOverrides = nullptr);

    void Reset();
    void Clear();

    TaskValue GetValueScoped(const std::string& varName) const;
    void SetValueScoped(const std::string& varName, const TaskValue& value);

    TaskValue GetGlobalValue(const std::string& globalKey) const;
    void SetGlobalValue(const std::string& globalKey, const TaskValue& value);

    TaskValue GetLocalValue(const std::string& localKey) const;
    void SetLocalValue(const std::string& localKey, const TaskValue& value);

    bool HasVariableScoped(const std::string& varName) const;
    bool HasGlobalVariable(const std::string& globalKey) const;
    bool HasLocalVariable(const std::string& localKey) const;

    size_t GetTotalVariableCount() const;
    size_t GetGlobalVariableCount() const;
    size_t GetLocalVariableCount() const;

    uint32_t GetEntityID() const;

    std::string DebugSummary() const;
    void DebugPrint() const;

    json ExportGlobalsToJson() const;
    bool ImportGlobalsFromJson(const json& data);

protected:

    uint32_t m_entityID;
    std::unordered_map<std::string, TaskValue> m_globalVars;
    std::unordered_map<std::string, VariableType> m_globalTypes;

private:

    static void ParseScopedName(const std::string& scopedName,
                               std::string& outPrefix,
                               std::string& outName);

};

} // namespace Olympe

#endif // OLYMPE_ENTITY_BLACKBOARD_H
