/**
 * @file TaskGraphTypes.cpp
 * @brief Implementation of TaskValue for the Atomic Task System
 * @author Olympe Engine
 * @date 2026-02-20
 */

#include "TaskGraphTypes.h"

#include <stdexcept>
#include <string>

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

} // namespace Olympe
