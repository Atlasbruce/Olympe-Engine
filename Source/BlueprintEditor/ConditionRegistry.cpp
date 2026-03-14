/**
 * @file ConditionRegistry.cpp
 * @brief Implementation of ConditionRegistry with built-in conditions.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionRegistry.h"

#include <algorithm>

namespace Olympe {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

ConditionRegistry& ConditionRegistry::Get()
{
    static ConditionRegistry instance;
    return instance;
}

ConditionRegistry::ConditionRegistry()
{
    InitializeBuiltInConditions();
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void ConditionRegistry::Register(const ConditionSpec& spec)
{
    if (!spec.id.empty())
        m_specs[spec.id] = spec;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const ConditionSpec* ConditionRegistry::GetConditionSpec(const std::string& id) const
{
    auto it = m_specs.find(id);
    return (it != m_specs.end()) ? &it->second : nullptr;
}

std::vector<std::string> ConditionRegistry::GetAllConditionIds() const
{
    std::vector<std::string> ids;
    ids.reserve(m_specs.size());
    for (const auto& kv : m_specs)
        ids.push_back(kv.first);
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::vector<ConditionSpec> ConditionRegistry::GetAllConditions() const
{
    std::vector<ConditionSpec> result;
    result.reserve(m_specs.size());
    for (const auto& kv : m_specs)
        result.push_back(kv.second);
    std::sort(result.begin(), result.end(),
              [](const ConditionSpec& a, const ConditionSpec& b) {
                  return a.id < b.id;
              });
    return result;
}

// ---------------------------------------------------------------------------
// Built-in condition specs
// ---------------------------------------------------------------------------

void ConditionRegistry::InitializeBuiltInConditions()
{
    // ---- CompareValue ----
    {
        ConditionSpec spec;
        spec.id          = "CompareValue";
        spec.displayName = "Compare Value";
        spec.description = "Compare a blackboard variable against a literal value.";
        spec.parameters.push_back({"Key",      ParameterBindingType::LocalVariable,
                                   "Blackboard key to read", true});
        spec.parameters.push_back({"Operator", ParameterBindingType::ComparisonOp,
                                   "Comparison operator (==, !=, <, <=, >, >=)", true});
        spec.parameters.push_back({"Value",    ParameterBindingType::Literal,
                                   "Literal value to compare against", true});
        Register(spec);
    }

    // ---- IsSet ----
    {
        ConditionSpec spec;
        spec.id          = "IsSet";
        spec.displayName = "Is Set";
        spec.description = "True if the blackboard key has been assigned a value.";
        spec.parameters.push_back({"Key", ParameterBindingType::LocalVariable,
                                   "Blackboard key to check", true});
        Register(spec);
    }

    // ---- IsNotSet ----
    {
        ConditionSpec spec;
        spec.id          = "IsNotSet";
        spec.displayName = "Is Not Set";
        spec.description = "True if the blackboard key has not been assigned a value.";
        spec.parameters.push_back({"Key", ParameterBindingType::LocalVariable,
                                   "Blackboard key to check", true});
        Register(spec);
    }

    // ---- InRange ----
    {
        ConditionSpec spec;
        spec.id          = "InRange";
        spec.displayName = "In Range";
        spec.description = "True if a blackboard value is within [Min, Max].";
        spec.parameters.push_back({"Key", ParameterBindingType::LocalVariable,
                                   "Blackboard key to read", true});
        spec.parameters.push_back({"Min", ParameterBindingType::Literal,
                                   "Minimum value (inclusive)", true});
        spec.parameters.push_back({"Max", ParameterBindingType::Literal,
                                   "Maximum value (inclusive)", true});
        Register(spec);
    }

    // ---- RandomChance ----
    {
        ConditionSpec spec;
        spec.id          = "RandomChance";
        spec.displayName = "Random Chance";
        spec.description = "True with a given probability [0.0 .. 1.0].";
        spec.parameters.push_back({"Probability", ParameterBindingType::Literal,
                                   "Probability of returning true (0.0 to 1.0)", true});
        Register(spec);
    }
}

} // namespace Olympe
