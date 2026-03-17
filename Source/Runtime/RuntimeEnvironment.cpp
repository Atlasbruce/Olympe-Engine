/**
 * @file RuntimeEnvironment.cpp
 * @brief Implementation of RuntimeEnvironment.
 * @author Olympe Engine
 * @date 2026-03-17
 */

#include "RuntimeEnvironment.h"

namespace Olympe {

// ---------------------------------------------------------------------------
// Blackboard variables
// ---------------------------------------------------------------------------

void RuntimeEnvironment::SetBlackboardVariable(const std::string& key, float value)
{
    m_blackboardVariables[key] = value;
}

bool RuntimeEnvironment::GetBlackboardVariable(const std::string& key, float& outValue) const
{
    auto it = m_blackboardVariables.find(key);
    if (it == m_blackboardVariables.end())
        return false;
    outValue = it->second;
    return true;
}

bool RuntimeEnvironment::HasBlackboardVariable(const std::string& key) const
{
    return m_blackboardVariables.find(key) != m_blackboardVariables.end();
}

// ---------------------------------------------------------------------------
// Dynamic pin values
// ---------------------------------------------------------------------------

void RuntimeEnvironment::SetDynamicPinValue(const std::string& pinID, float value)
{
    m_dynamicPinValues[pinID] = value;
}

bool RuntimeEnvironment::GetDynamicPinValue(const std::string& pinID, float& outValue) const
{
    auto it = m_dynamicPinValues.find(pinID);
    if (it == m_dynamicPinValues.end())
        return false;
    outValue = it->second;
    return true;
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

void RuntimeEnvironment::Clear()
{
    m_blackboardVariables.clear();
    m_dynamicPinValues.clear();
}

} // namespace Olympe
