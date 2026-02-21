/**
 * @file LocalBlackboard.cpp
 * @brief Implementation of LocalBlackboard for the Atomic Task System
 * @author Olympe Engine
 * @date 2026-02-20
 */

#include "LocalBlackboard.h"
#include "TaskGraphTemplate.h"

#include <stdexcept>
#include <string>

#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

LocalBlackboard::LocalBlackboard()
{
}

// ============================================================================
// Lifecycle
// ============================================================================

void LocalBlackboard::Initialize(const TaskGraphTemplate& tmpl)
{
    m_variables.clear();
    m_defaults.clear();
    m_types.clear();

    for (size_t i = 0; i < tmpl.LocalVariables.size(); ++i)
    {
        const VariableDefinition& def = tmpl.LocalVariables[i];
        m_variables[def.Name]  = def.DefaultValue;
        m_defaults[def.Name]   = def.DefaultValue;
        m_types[def.Name]      = def.Type;
    }

    SYSTEM_LOG << "[LocalBlackboard] Initialized with " << tmpl.LocalVariables.size()
               << " variables from template '" << tmpl.Name << "'" << std::endl;
}

void LocalBlackboard::Reset()
{
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it)
    {
        m_variables[it->first] = it->second;
    }

    SYSTEM_LOG << "[LocalBlackboard] Reset to defaults" << std::endl;
}

// ============================================================================
// Variable access
// ============================================================================

TaskValue LocalBlackboard::GetValue(const std::string& varName) const
{
    auto it = m_variables.find(varName);
    if (it == m_variables.end())
    {
        throw std::runtime_error("[LocalBlackboard] Unknown variable: " + varName);
    }
    return it->second;
}

void LocalBlackboard::SetValue(const std::string& varName, const TaskValue& value)
{
    auto typeIt = m_types.find(varName);
    if (typeIt == m_types.end())
    {
        throw std::runtime_error("[LocalBlackboard] Unknown variable: " + varName);
    }

    if (value.GetType() != typeIt->second)
    {
        throw std::runtime_error("[LocalBlackboard] Type mismatch for variable: " + varName);
    }

    m_variables[varName] = value;
}

// ============================================================================
// Queries
// ============================================================================

bool LocalBlackboard::HasVariable(const std::string& varName) const
{
    return m_variables.find(varName) != m_variables.end();
}

std::vector<std::string> LocalBlackboard::GetVariableNames() const
{
    std::vector<std::string> names;
    names.reserve(m_variables.size());

    for (auto it = m_variables.begin(); it != m_variables.end(); ++it)
    {
        names.push_back(it->first);
    }

    return names;
}

} // namespace Olympe
