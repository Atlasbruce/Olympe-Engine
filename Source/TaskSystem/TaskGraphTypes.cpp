/**
 * @file TaskGraphTypes.cpp
 * @brief Implementation of TaskValue for the Atomic Task System
 * @author Olympe Engine
 * @date 2026-02-20
 */

#include "TaskGraphTypes.h"

#include <stdexcept>
#include <string>
#include <sstream>

#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// TaskValue - Constructors
// ============================================================================

TaskValue::TaskValue()
    : m_vectorValue()
    , m_stringValue()
    , m_type(VariableType::None)
{
    m_data.entityValue = 0;
}

TaskValue::TaskValue(bool v)
    : m_vectorValue()
    , m_stringValue()
    , m_type(VariableType::Bool)
{
    m_data.boolValue = v;
}

TaskValue::TaskValue(int v)
    : m_vectorValue()
    , m_stringValue()
    , m_type(VariableType::Int)
{
    m_data.intValue = v;
}

TaskValue::TaskValue(float v)
    : m_vectorValue()
    , m_stringValue()
    , m_type(VariableType::Float)
{
    m_data.floatValue = v;
}

TaskValue::TaskValue(const ::Vector& v)
    : m_vectorValue(v)
    , m_stringValue()
    , m_type(VariableType::Vector)
{
    m_data.entityValue = 0;
}

TaskValue::TaskValue(EntityID v)
    : m_vectorValue()
    , m_stringValue()
    , m_type(VariableType::EntityID)
{
    m_data.entityValue = v;
}

TaskValue::TaskValue(const std::string& v)
    : m_vectorValue()
    , m_stringValue(v)
    , m_type(VariableType::String)
{
    m_data.entityValue = 0;
}

// ============================================================================
// TaskValue - Typed getters
// ============================================================================

bool TaskValue::AsBool() const
{
    if (m_type != VariableType::Bool)
    {
        throw std::runtime_error("[TaskValue] Type mismatch: expected Bool");
    }
    return m_data.boolValue;
}

int TaskValue::AsInt() const
{
    if (m_type != VariableType::Int)
    {
        throw std::runtime_error("[TaskValue] Type mismatch: expected Int");
    }
    return m_data.intValue;
}

float TaskValue::AsFloat() const
{
    if (m_type != VariableType::Float)
    {
        throw std::runtime_error("[TaskValue] Type mismatch: expected Float");
    }
    return m_data.floatValue;
}

::Vector TaskValue::AsVector() const
{
    if (m_type != VariableType::Vector)
    {
        throw std::runtime_error("[TaskValue] Type mismatch: expected Vector");
    }
    return m_vectorValue;
}

EntityID TaskValue::AsEntityID() const
{
    if (m_type != VariableType::EntityID)
    {
        throw std::runtime_error("[TaskValue] Type mismatch: expected EntityID");
    }
    return m_data.entityValue;
}

std::string TaskValue::AsString() const
{
    if (m_type != VariableType::String)
    {
        throw std::runtime_error("[TaskValue] Type mismatch: expected String");
    }
    return m_stringValue;
}

// ============================================================================
// TaskValue - Type queries
// ============================================================================

VariableType TaskValue::GetType() const
{
    return m_type;
}

bool TaskValue::IsNone() const
{
    return m_type == VariableType::None;
}

// ============================================================================
// TaskValue - String conversion (Fix #4 — never return empty)
// ============================================================================

std::string TaskValue::to_string() const
{
    switch (m_type)
    {
        case VariableType::Bool:
            return m_data.boolValue ? "true" : "false";
        case VariableType::Int:
            return std::to_string(m_data.intValue);
        case VariableType::Float:
        {
            std::ostringstream oss;
            oss << m_data.floatValue;
            return oss.str();
        }
        case VariableType::EntityID:
            return std::to_string(static_cast<unsigned long long>(m_data.entityValue));
        case VariableType::String:
            return m_stringValue.empty() ? "" : m_stringValue;
        case VariableType::None:
        default:
            SYSTEM_LOG << "[TaskValue] WARNING: to_string() called on None/unknown type, returning '0'\n";
            return "0";
    }
}

// ============================================================================
// Free functions
// ============================================================================

// Fix #3: VariableTypeToString — never returns garbage (fallback to "Int")
std::string VariableTypeToString(VariableType t)
{
    switch (t)
    {
        case VariableType::Bool:      return "Bool";
        case VariableType::Int:       return "Int";
        case VariableType::Float:     return "Float";
        case VariableType::Vector:    return "Vector";
        case VariableType::EntityID:  return "EntityID";
        case VariableType::String:    return "String";
        case VariableType::None:      return "None";
        case VariableType::List:      return "List";
        case VariableType::GlobalRef: return "GlobalRef";
        default:
            SYSTEM_LOG << "[VariableTypeToString] ERROR: Unknown type, defaulting to Int\n";
            return "Int";
    }
}

// UX Fix #1: GetDefaultValueForType — returns a type-consistent default value
TaskValue GetDefaultValueForType(VariableType type)
{
    switch (type)
    {
        case VariableType::Bool:     return TaskValue(false);
        case VariableType::Int:      return TaskValue(0);
        case VariableType::Float:    return TaskValue(0.0f);
        case VariableType::EntityID: return TaskValue(static_cast<EntityID>(0));
        case VariableType::String:   return TaskValue(std::string(""));
        case VariableType::Vector:   return TaskValue(::Vector());
        default:                     return TaskValue(0);
    }
}

} // namespace Olympe
