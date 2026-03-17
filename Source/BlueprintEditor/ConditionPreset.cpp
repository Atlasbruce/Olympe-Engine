/**
 * @file ConditionPreset.cpp
 * @brief Implementation of ConditionPreset and ConditionPresetRegistry (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPreset.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <cctype>
#include <iostream>

#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

namespace Olympe {

// ============================================================================
// ConditionPreset::GetPreview
// ============================================================================

static std::string FormatOperand(const std::string& mode,
                                  const std::string& variable,
                                  const std::string& pin,
                                  const TaskValue&   constVal)
{
    if (mode == "Variable")
    {
        return "[" + variable + "]";
    }
    else if (mode == "Pin")
    {
        return "[Pin:" + pin + "]";
    }
    else if (mode == "Const")
    {
        return "[" + constVal.to_string() + "]";
    }
    return "[?]";
}

std::string ConditionPreset::GetPreview() const
{
    const std::string left  = FormatOperand(condition.leftMode,
                                             condition.leftVariable,
                                             condition.leftPin,
                                             condition.leftConstValue);
    const std::string right = FormatOperand(condition.rightMode,
                                             condition.rightVariable,
                                             condition.rightPin,
                                             condition.rightConstValue);
    const std::string op    = condition.operatorStr.empty()
                               ? "?"
                               : condition.operatorStr;

    return left + " " + op + " " + right;
}

// ============================================================================
// ConditionPreset — Serialization
// ============================================================================

json ConditionPreset::ToJson() const
{
    json entry;
    entry["id"]   = id;
    entry["name"] = name;

    // Left side
    json left;
    left["mode"]     = condition.leftMode;
    left["variable"] = condition.leftVariable;
    left["pin"]      = condition.leftPin;
    if (condition.leftMode == "Const")
    {
        left["constType"]  = VariableTypeToString(condition.leftConstValue.GetType());
        left["constValue"] = condition.leftConstValue.to_string();
    }
    entry["left"] = left;

    entry["operator"] = condition.operatorStr;

    // Right side
    json right;
    right["mode"]     = condition.rightMode;
    right["variable"] = condition.rightVariable;
    right["pin"]      = condition.rightPin;
    if (condition.rightMode == "Const")
    {
        right["constType"]  = VariableTypeToString(condition.rightConstValue.GetType());
        right["constValue"] = condition.rightConstValue.to_string();
    }
    entry["right"] = right;

    return entry;
}

ConditionPreset ConditionPreset::FromJson(const json& j)
{
    ConditionPreset p;
    p.id   = j.value("id",   "");
    p.name = j.value("name", "");

    // Left side
    if (j.contains("left") && j["left"].is_object())
    {
        const auto& left = j["left"];
        p.condition.leftMode     = left.value("mode",     "");
        p.condition.leftVariable = left.value("variable", "");
        p.condition.leftPin      = left.value("pin",      "");

        const std::string leftType  = left.value("constType", "");
        const std::string leftValue = left.value("constValue", "");

        if (leftType == "Int")
        {
            p.condition.leftConstValue = TaskValue(std::stoi(leftValue.empty() ? "0" : leftValue));
        }
        else if (leftType == "Float")
        {
            p.condition.leftConstValue = TaskValue(std::stof(leftValue.empty() ? "0" : leftValue));
        }
        else if (leftType == "Bool")
        {
            p.condition.leftConstValue = TaskValue(leftValue == "true");
        }
        else if (leftType == "String")
        {
            p.condition.leftConstValue = TaskValue(leftValue);
        }
    }

    // Operator
    p.condition.operatorStr = j.value("operator", "");

    // Right side
    if (j.contains("right") && j["right"].is_object())
    {
        const auto& right = j["right"];
        p.condition.rightMode     = right.value("mode",     "");
        p.condition.rightVariable = right.value("variable", "");
        p.condition.rightPin      = right.value("pin",      "");

        const std::string rightType  = right.value("constType",  "");
        const std::string rightValue = right.value("constValue", "");

        if (rightType == "Int")
        {
            p.condition.rightConstValue = TaskValue(std::stoi(rightValue.empty() ? "0" : rightValue));
        }
        else if (rightType == "Float")
        {
            p.condition.rightConstValue = TaskValue(std::stof(rightValue.empty() ? "0" : rightValue));
        }
        else if (rightType == "Bool")
        {
            p.condition.rightConstValue = TaskValue(rightValue == "true");
        }
        else if (rightType == "String")
        {
            p.condition.rightConstValue = TaskValue(rightValue);
        }
    }

    return p;
}

// ============================================================================
// ConditionPresetRegistry — helpers
// ============================================================================

std::string ConditionPresetRegistry::GenerateID() const
{
    // Simple deterministic ID based on counter + size to avoid collisions
    // across calls without depending on <chrono> or <random> (C++14 safe).
    std::ostringstream oss;
    oss << "preset_" << (static_cast<int>(m_order.size()) + 1)
        << "_" << m_counter;
    // Ensure uniqueness: if this ID already exists, keep incrementing
    std::string candidate = oss.str();
    int attempt = 0;
    while (m_presets.count(candidate))
    {
        ++attempt;
        std::ostringstream retry;
        retry << "preset_" << (static_cast<int>(m_order.size()) + 1)
              << "_" << (m_counter + attempt);
        candidate = retry.str();
    }
    return candidate;
}

std::string ConditionPresetRegistry::ToLower(const std::string& s)
{
    std::string out = s;
    for (char& c : out)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

// ============================================================================
// ConditionPresetRegistry — CRUD
// ============================================================================

std::string ConditionPresetRegistry::AddPreset(const ConditionPreset& preset)
{
    ++m_counter;

    ConditionPreset entry = preset;

    // Assign ID if not provided
    if (entry.id.empty())
    {
        entry.id = GenerateID();
    }

    // Assign default name if not provided
    if (entry.name.empty())
    {
        entry.name = "Condition #" + std::to_string(m_counter);
    }

    m_presets[entry.id] = entry;
    m_order.push_back(entry.id);

    return entry.id;
}

bool ConditionPresetRegistry::RemovePreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
    {
        return false;
    }

    m_presets.erase(it);

    auto orderIt = std::find(m_order.begin(), m_order.end(), id);
    if (orderIt != m_order.end())
    {
        m_order.erase(orderIt);
    }

    return true;
}

bool ConditionPresetRegistry::UpdatePreset(const std::string& id,
                                            const ConditionPreset& preset)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
    {
        return false;
    }

    ConditionPreset updated = preset;
    updated.id = id; // Preserve the original ID
    it->second = updated;

    return true;
}

std::string ConditionPresetRegistry::DuplicatePreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
    {
        return std::string();
    }

    ConditionPreset copy = it->second;
    copy.id   = std::string(); // Let AddPreset generate a new ID
    copy.name = "Copy of " + it->second.name;

    return AddPreset(copy);
}

// ============================================================================
// ConditionPresetRegistry — lookup
// ============================================================================

const ConditionPreset* ConditionPresetRegistry::GetPreset(const std::string& id) const
{
    auto it = m_presets.find(id);
    return (it != m_presets.end()) ? &it->second : nullptr;
}

ConditionPreset* ConditionPresetRegistry::GetPreset(const std::string& id)
{
    auto it = m_presets.find(id);
    return (it != m_presets.end()) ? &it->second : nullptr;
}

bool ConditionPresetRegistry::HasPreset(const std::string& id) const
{
    return m_presets.count(id) > 0;
}

std::vector<ConditionPreset> ConditionPresetRegistry::GetAllPresets() const
{
    std::vector<ConditionPreset> result;
    result.reserve(m_order.size());
    for (const auto& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it != m_presets.end())
        {
            result.push_back(it->second);
        }
    }
    return result;
}

size_t ConditionPresetRegistry::GetCount() const
{
    return m_presets.size();
}

void ConditionPresetRegistry::Clear()
{
    m_presets.clear();
    m_order.clear();
    m_counter = 0;
}

// ============================================================================
// ConditionPresetRegistry — filter
// ============================================================================

std::vector<ConditionPreset>
ConditionPresetRegistry::GetFilteredPresets(const std::string& filter) const
{
    if (filter.empty())
    {
        return GetAllPresets();
    }

    const std::string lowerFilter = ToLower(filter);
    std::vector<ConditionPreset> result;

    for (const auto& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it == m_presets.end()) { continue; }

        const ConditionPreset& p = it->second;
        const std::string lowerName    = ToLower(p.name);
        const std::string lowerPreview = ToLower(p.GetPreview());

        if (lowerName.find(lowerFilter) != std::string::npos ||
            lowerPreview.find(lowerFilter) != std::string::npos)
        {
            result.push_back(p);
        }
    }

    return result;
}

// ============================================================================
// ConditionPresetRegistry — serialization
// ============================================================================

static std::string TaskValueToString(const TaskValue& v)
{
    return v.to_string();
}

bool ConditionPresetRegistry::LoadFromFile(const std::string& filepath)
{
    Clear();

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[ConditionPresetRegistry] Cannot open file: "
                  << filepath << std::endl;
        return false;
    }

    json root;
    try
    {
        file >> root;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ConditionPresetRegistry] JSON parse error: "
                  << e.what() << std::endl;
        return false;
    }

    if (!root.is_object() || !root.contains("presets") || !root["presets"].is_array())
    {
        std::cerr << "[ConditionPresetRegistry] Missing 'presets' array." << std::endl;
        return false;
    }

    for (const auto& entry : root["presets"])
    {
        if (!entry.is_object()) { continue; }

        ConditionPreset p;
        p.id   = entry.value("id",   "");
        p.name = entry.value("name", "");

        // Left side
        if (entry.contains("left") && entry["left"].is_object())
        {
            const auto& left = entry["left"];
            p.condition.leftMode     = left.value("mode",     "");
            p.condition.leftVariable = left.value("variable", "");
            p.condition.leftPin      = left.value("pin",      "");

            const std::string leftType  = left.value("constType", "");
            const std::string leftValue = left.value("constValue", "");

            if (leftType == "Int")
            {
                p.condition.leftConstValue = TaskValue(std::stoi(leftValue.empty() ? "0" : leftValue));
            }
            else if (leftType == "Float")
            {
                p.condition.leftConstValue = TaskValue(std::stof(leftValue.empty() ? "0" : leftValue));
            }
            else if (leftType == "Bool")
            {
                p.condition.leftConstValue = TaskValue(leftValue == "true");
            }
            else if (leftType == "String")
            {
                p.condition.leftConstValue = TaskValue(leftValue);
            }
        }

        // Operator
        p.condition.operatorStr = entry.value("operator", "");

        // Right side
        if (entry.contains("right") && entry["right"].is_object())
        {
            const auto& right = entry["right"];
            p.condition.rightMode     = right.value("mode",     "");
            p.condition.rightVariable = right.value("variable", "");
            p.condition.rightPin      = right.value("pin",      "");

            const std::string rightType  = right.value("constType",  "");
            const std::string rightValue = right.value("constValue", "");

            if (rightType == "Int")
            {
                p.condition.rightConstValue = TaskValue(std::stoi(rightValue.empty() ? "0" : rightValue));
            }
            else if (rightType == "Float")
            {
                p.condition.rightConstValue = TaskValue(std::stof(rightValue.empty() ? "0" : rightValue));
            }
            else if (rightType == "Bool")
            {
                p.condition.rightConstValue = TaskValue(rightValue == "true");
            }
            else if (rightType == "String")
            {
                p.condition.rightConstValue = TaskValue(rightValue);
            }
        }

        if (!p.id.empty())
        {
            m_presets[p.id] = p;
            m_order.push_back(p.id);
            ++m_counter;
        }
    }

    return true;
}

bool ConditionPresetRegistry::SaveToFile(const std::string& filepath) const
{
    json root;
    root["version"] = 1;
    root["presets"] = json::array();

    for (const auto& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it == m_presets.end()) { continue; }

        const ConditionPreset& p = it->second;
        const Condition&       c = p.condition;

        json entry;
        entry["id"]   = p.id;
        entry["name"] = p.name;

        // Left side
        json left;
        left["mode"]     = c.leftMode;
        left["variable"] = c.leftVariable;
        left["pin"]      = c.leftPin;
        if (c.leftMode == "Const")
        {
            left["constType"]  = VariableTypeToString(c.leftConstValue.GetType());
            left["constValue"] = TaskValueToString(c.leftConstValue);
        }
        entry["left"] = left;

        entry["operator"] = c.operatorStr;

        // Right side
        json right;
        right["mode"]     = c.rightMode;
        right["variable"] = c.rightVariable;
        right["pin"]      = c.rightPin;
        if (c.rightMode == "Const")
        {
            right["constType"]  = VariableTypeToString(c.rightConstValue.GetType());
            right["constValue"] = TaskValueToString(c.rightConstValue);
        }
        entry["right"] = right;

        root["presets"].push_back(entry);
    }

    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[ConditionPresetRegistry] Cannot write file: "
                  << filepath << std::endl;
        return false;
    }

    file << root.dump(4);
    return file.good();
}

} // namespace Olympe
