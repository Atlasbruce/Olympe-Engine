/**
 * @file EntityBlackboard.cpp
 * @brief Implementation of EntityBlackboard (per-entity global+local blackboard instance)
 * @author Olympe Engine
 * @date 2026-03-26
 */

#include "EntityBlackboard.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"
#include "../system/system_utils.h"
#include "../json_helper.h"

#include <sstream>
#include <iomanip>

namespace Olympe {

EntityBlackboard::EntityBlackboard(uint32_t entityID)
    : m_entityID(entityID)
{
}

EntityBlackboard::~EntityBlackboard()
{
}

void EntityBlackboard::Initialize(const TaskGraphTemplate& tmpl,
                                 const std::unordered_map<std::string, TaskValue>* globalOverrides)
{
    SYSTEM_LOG << "[EntityBlackboard] Initialize: entity=" << m_entityID << "\n";
    LocalBlackboard::Initialize(tmpl);

    // Initialize global variables from the global template registry
    const GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

    for (const auto& globalDef : globalVars)
    {
        // Store the variable with its default value
        TaskValue varValue = globalDef.DefaultValue;

        // Apply override if provided
        if (globalOverrides != nullptr)
        {
            auto overrideIt = globalOverrides->find(globalDef.Key);
            if (overrideIt != globalOverrides->end())
            {
                varValue = overrideIt->second;
                SYSTEM_LOG << "[EntityBlackboard] Applied override for global '" << globalDef.Key << "'\n";
            }
        }

        m_globalVars[globalDef.Key] = varValue;
        m_globalTypes[globalDef.Key] = globalDef.Type;
    }

    SYSTEM_LOG << "[EntityBlackboard] Initialized " << m_globalVars.size() << " global variables\n";
}

void EntityBlackboard::Reset()
{
    SYSTEM_LOG << "[EntityBlackboard] Reset: entity=" << m_entityID << "\n";
    LocalBlackboard::Reset();

    // Reset global variables to their defaults
    const GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

    for (const auto& globalDef : globalVars)
    {
        m_globalVars[globalDef.Key] = globalDef.DefaultValue;
        m_globalTypes[globalDef.Key] = globalDef.Type;
    }
}

void EntityBlackboard::Clear()
{
    m_globalVars.clear();
    m_globalTypes.clear();
}

TaskValue EntityBlackboard::GetValueScoped(const std::string& varName) const
{
    std::string prefix, bareVarName;
    ParseScopedName(varName, prefix, bareVarName);

    if (prefix == "(L)")
    {
        return GetLocalValue(bareVarName);
    }
    else if (prefix == "(G)")
    {
        return GetGlobalValue(bareVarName);
    }
    else
    {
        // Bare name: local-first with fallback to global
        if (HasLocalVariable(bareVarName))
        {
            return GetLocalValue(bareVarName);
        }
        return GetGlobalValue(bareVarName);
    }
}

void EntityBlackboard::SetValueScoped(const std::string& varName, const TaskValue& value)
{
    std::string prefix, bareVarName;
    ParseScopedName(varName, prefix, bareVarName);

    if (prefix == "(L)")
    {
        SetLocalValue(bareVarName, value);
    }
    else if (prefix == "(G)")
    {
        SetGlobalValue(bareVarName, value);
    }
    else
    {
        // Bare name: local-first with fallback to global
        if (HasLocalVariable(bareVarName))
        {
            SetLocalValue(bareVarName, value);
        }
        else
        {
            SetGlobalValue(bareVarName, value);
        }
    }
}

TaskValue EntityBlackboard::GetGlobalValue(const std::string& globalKey) const
{
    auto it = m_globalVars.find(globalKey);
    if (it == m_globalVars.end())
    {
        SYSTEM_LOG << "[EntityBlackboard] ERROR: Global variable not found: '" << globalKey << "'\n";
        throw std::runtime_error("Global variable not found: " + globalKey);
    }
    return it->second;
}

void EntityBlackboard::SetGlobalValue(const std::string& globalKey, const TaskValue& value)
{
    auto typeIt = m_globalTypes.find(globalKey);
    if (typeIt == m_globalTypes.end())
    {
        SYSTEM_LOG << "[EntityBlackboard] ERROR: Global variable not found for set: '" << globalKey << "'\n";
        throw std::runtime_error("Global variable not found: " + globalKey);
    }

    // Type compatibility check
    VariableType expectedType = typeIt->second;
    VariableType providedType = value.GetType();

    // Allow None type (for initialization) or exact match
    if (providedType != VariableType::None && providedType != expectedType)
    {
        SYSTEM_LOG << "[EntityBlackboard] WARNING: Type mismatch for global '" << globalKey
                   << "' (expected " << VariableTypeToString(expectedType)
                   << ", got " << VariableTypeToString(providedType) << ")\n";
    }

    m_globalVars[globalKey] = value;
}

TaskValue EntityBlackboard::GetLocalValue(const std::string& localKey) const
{
    return LocalBlackboard::GetValue(localKey);
}

void EntityBlackboard::SetLocalValue(const std::string& localKey, const TaskValue& value)
{
    LocalBlackboard::SetValue(localKey, value);
}

bool EntityBlackboard::HasVariableScoped(const std::string& varName) const
{
    std::string prefix, bareVarName;
    ParseScopedName(varName, prefix, bareVarName);

    if (prefix == "(L)")
    {
        return HasLocalVariable(bareVarName);
    }
    else if (prefix == "(G)")
    {
        return HasGlobalVariable(bareVarName);
    }
    else
    {
        return HasLocalVariable(bareVarName) || HasGlobalVariable(bareVarName);
    }
}

bool EntityBlackboard::HasGlobalVariable(const std::string& globalKey) const
{
    return m_globalVars.find(globalKey) != m_globalVars.end();
}

bool EntityBlackboard::HasLocalVariable(const std::string& localKey) const
{
    return LocalBlackboard::HasVariable(localKey);
}

size_t EntityBlackboard::GetTotalVariableCount() const
{
    return GetLocalVariableCount() + GetGlobalVariableCount();
}

size_t EntityBlackboard::GetGlobalVariableCount() const
{
    return m_globalVars.size();
}

size_t EntityBlackboard::GetLocalVariableCount() const
{
    // TODO: Call appropriate LocalBlackboard method once interface is known
    return 0;  // Placeholder
}

uint32_t EntityBlackboard::GetEntityID() const
{
    return m_entityID;
}

std::string EntityBlackboard::DebugSummary() const
{
    std::ostringstream oss;
    oss << "EntityBlackboard(id=" << m_entityID
        << ", locals=" << GetLocalVariableCount()
        << ", globals=" << GetGlobalVariableCount() << ")";
    return oss.str();
}

void EntityBlackboard::DebugPrint() const
{
    SYSTEM_LOG << "=== " << DebugSummary() << " ===\n";
    SYSTEM_LOG << "Local variables: " << GetLocalVariableCount() << "\n";
    SYSTEM_LOG << "Global variables: " << GetGlobalVariableCount() << "\n";
}

json EntityBlackboard::ExportGlobalsToJson() const
{
    json obj = json::object();

    for (auto it = m_globalVars.begin(); it != m_globalVars.end(); ++it)
    {
        const std::string& varName = it->first;
        const TaskValue& value = it->second;
        if (value.IsNone())
        {
            obj[varName] = nullptr;
        }
        else
        {
            switch (value.GetType())
            {
                case VariableType::Bool:
                    obj[varName] = value.AsBool();
                    break;
                case VariableType::Int:
                    obj[varName] = value.AsInt();
                    break;
                case VariableType::Float:
                    obj[varName] = value.AsFloat();
                    break;
                case VariableType::String:
                    obj[varName] = value.AsString();
                    break;
                case VariableType::Vector:
                {
                    const ::Vector v = value.AsVector();
                    json vecObj;
                    vecObj["x"] = v.x;
                    vecObj["y"] = v.y;
                    vecObj["z"] = v.z;
                    obj[varName] = vecObj;
                    break;
                }
                case VariableType::EntityID:
                    obj[varName] = static_cast<int>(value.AsEntityID());
                    break;
                default:
                    obj[varName] = nullptr;
                    break;
            }
        }
    }

    return obj;
}

bool EntityBlackboard::ImportGlobalsFromJson(const json& data)
{
    if (!data.is_object())
    {
        return false;
    }

    bool success = true;

    for (auto it = data.begin(); it != data.end(); ++it)
    {
        const std::string& key = it.key();
        const json& valueNode = it.value();

        auto typeIt = m_globalTypes.find(key);
        if (typeIt == m_globalTypes.end())
        {
            SYSTEM_LOG << "[EntityBlackboard] WARNING: Global variable '" << key << "' not found in registry\n";
            success = false;
            continue;
        }

        VariableType varType = typeIt->second;

        try
        {
            TaskValue value;
            if (valueNode.is_null())
            {
                value = TaskValue();
            }
            else if (varType == VariableType::Bool && valueNode.is_boolean())
            {
                value = TaskValue(valueNode.get<bool>());
            }
            else if (varType == VariableType::Int && valueNode.is_number_integer())
            {
                value = TaskValue(valueNode.get<int>());
            }
            else if (varType == VariableType::Float && valueNode.is_number())
            {
                value = TaskValue(static_cast<float>(valueNode.get<double>()));
            }
            else if (varType == VariableType::String && valueNode.is_string())
            {
                value = TaskValue(valueNode.get<std::string>());
            }
            else if (varType == VariableType::Vector && valueNode.is_object() &&
                     valueNode.contains("x") && valueNode.contains("y") && valueNode.contains("z"))
            {
                const float x = valueNode["x"].get<float>();
                const float y = valueNode["y"].get<float>();
                const float z = valueNode["z"].get<float>();
                value = TaskValue(::Vector{x, y, z});
            }
            else if (varType == VariableType::EntityID && valueNode.is_number_integer())
            {
                value = TaskValue(valueNode.get<int>());
            }
            else
            {
                SYSTEM_LOG << "[EntityBlackboard] WARNING: Type mismatch for global '" << key << "'\n";
                success = false;
                continue;
            }

            m_globalVars[key] = value;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[EntityBlackboard] EXCEPTION importing global '" << key << "': " << e.what() << "\n";
            success = false;
        }
    }

    return success;
}

void EntityBlackboard::ParseScopedName(const std::string& scopedName,
                                     std::string& outPrefix,
                                     std::string& outName)
{
    // Check for (L) or (G) prefix
    if (scopedName.length() >= 3 && scopedName[0] == '(' && scopedName[2] == ')')
    {
        char scope = scopedName[1];
        if (scope == 'L' || scope == 'G')
        {
            outPrefix = std::string(1, '(') + scope + ")";
            outName = scopedName.substr(3);
            return;
        }
    }

    // No prefix: bare name
    outPrefix = "";
    outName = scopedName;
}

} // namespace Olympe
