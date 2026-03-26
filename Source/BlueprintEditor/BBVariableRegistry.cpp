/**
 * @file BBVariableRegistry.cpp
 * @brief Implementation of BBVariableRegistry.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BBVariableRegistry.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"

#include <algorithm>

namespace Olympe {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const char* VariableTypeName(VariableType t)
{
    switch (t)
    {
        case VariableType::Bool:      return "Bool";
        case VariableType::Int:       return "Int";
        case VariableType::Float:     return "Float";
        case VariableType::Vector:    return "Vector";
        case VariableType::EntityID:  return "EntityID";
        case VariableType::String:    return "String";
        case VariableType::List:      return "List";
        case VariableType::GlobalRef: return "GlobalRef";
        default:                      return "None";
    }
}

// ---------------------------------------------------------------------------
// BBVariableRegistry
// ---------------------------------------------------------------------------

void BBVariableRegistry::LoadFromTemplate(const TaskGraphTemplate& tmpl)
{
    m_vars.clear();
    m_vars.reserve(tmpl.Blackboard.size());

    // Load local and global variables from the template
    for (size_t i = 0; i < tmpl.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = tmpl.Blackboard[i];
        if (entry.Key.empty())
            continue;

        VarSpec spec;
        spec.name         = entry.Key;
        spec.type         = entry.Type;
        spec.isGlobal     = entry.IsGlobal;
        spec.displayLabel = FormatDisplayLabel(entry.Key, entry.Type, entry.IsGlobal);
        m_vars.push_back(spec);
    }

    // Phase 24: Also load global variables from GlobalTemplateBlackboard registry
    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

    for (const auto& globalEntry : globalVars)
    {
        // Check if this global variable is already in the template (avoid duplicates)
        bool alreadyExists = false;
        for (const auto& existing : m_vars)
        {
            if (existing.name == globalEntry.Key && existing.isGlobal)
            {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists)
        {
            VarSpec spec;
            spec.name         = globalEntry.Key;
            spec.type         = globalEntry.Type;
            spec.isGlobal     = true;
            spec.displayLabel = FormatDisplayLabel(globalEntry.Key, globalEntry.Type, true);
            m_vars.push_back(spec);
        }
    }

    // Sort by name
    std::sort(m_vars.begin(), m_vars.end(),
              [](const VarSpec& a, const VarSpec& b) {
                  return a.name < b.name;
              });
}

const std::vector<VarSpec>& BBVariableRegistry::GetAllVariables() const
{
    return m_vars;
}

std::vector<VarSpec> BBVariableRegistry::GetVariablesByType(VariableType type) const
{
    std::vector<VarSpec> result;
    for (size_t i = 0; i < m_vars.size(); ++i)
    {
        if (m_vars[i].type == type)
            result.push_back(m_vars[i]);
    }
    return result;
}

std::vector<VarSpec> BBVariableRegistry::GetLocalVariables() const
{
    std::vector<VarSpec> result;
    for (size_t i = 0; i < m_vars.size(); ++i)
    {
        if (!m_vars[i].isGlobal)
            result.push_back(m_vars[i]);
    }
    return result;
}

std::vector<VarSpec> BBVariableRegistry::GetGlobalVariables() const
{
    std::vector<VarSpec> result;
    for (size_t i = 0; i < m_vars.size(); ++i)
    {
        if (m_vars[i].isGlobal)
            result.push_back(m_vars[i]);
    }
    return result;
}

/*static*/
std::string BBVariableRegistry::FormatDisplayLabel(const std::string& name,
                                                    VariableType type,
                                                    bool isGlobal)
{
    return name + " (" + VariableTypeName(type) + ", " +
           (isGlobal ? "global" : "local") + ")";
}

bool BBVariableRegistry::HasVariable(const std::string& name) const
{
    for (size_t i = 0; i < m_vars.size(); ++i)
    {
        if (m_vars[i].name == name)
            return true;
    }
    return false;
}

size_t BBVariableRegistry::GetCount() const
{
    return m_vars.size();
}

} // namespace Olympe
