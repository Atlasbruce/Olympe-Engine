/**
 * @file ConditionPresetRegistry.cpp
 * @brief Implementation of ConditionPresetRegistry (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPresetRegistry.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace Olympe {

// ============================================================================
// Private helpers
// ============================================================================

/*static*/
std::string ConditionPresetRegistry::GenerateID()
{
    // Simple counter-based ID — sufficient for a single-threaded editor tool.
    static int s_counter = 0;
    ++s_counter;
    std::ostringstream oss;
    oss << "preset_" << s_counter;
    return oss.str();
}

static std::string ToLowerStr(const std::string& s)
{
    std::string out = s;
    for (char& c : out)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// ============================================================================
// CRUD
// ============================================================================

std::string ConditionPresetRegistry::CreatePreset(const ConditionPreset& preset)
{
    ConditionPreset entry = preset;

    if (entry.id.empty())
    {
        // Generate a unique ID (retry if collision)
        std::string candidate = GenerateID();
        while (m_presets.count(candidate))
            candidate = GenerateID();
        entry.id = candidate;
    }
    else if (m_presets.count(entry.id))
    {
        // ID already exists → no-op, return existing ID
        return entry.id;
    }

    if (entry.name.empty())
    {
        entry.name = "Condition #" + std::to_string(static_cast<int>(m_presets.size()) + 1);
    }

    m_presets[entry.id] = entry;
    m_order.push_back(entry.id);

    return entry.id;
}

ConditionPreset* ConditionPresetRegistry::GetPreset(const std::string& id)
{
    auto it = m_presets.find(id);
    return (it != m_presets.end()) ? &it->second : nullptr;
}

const ConditionPreset* ConditionPresetRegistry::GetPreset(const std::string& id) const
{
    auto it = m_presets.find(id);
    return (it != m_presets.end()) ? &it->second : nullptr;
}

void ConditionPresetRegistry::UpdatePreset(const std::string& id,
                                            const ConditionPreset& updated)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
    {
        m_errors.push_back("[ConditionPresetRegistry] UpdatePreset: ID not found: " + id);
        return;
    }

    ConditionPreset copy = updated;
    copy.id = id; // Preserve original ID
    it->second = copy;
}

void ConditionPresetRegistry::DeletePreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end()) { return; } // no-op

    m_presets.erase(it);

    auto orderIt = std::find(m_order.begin(), m_order.end(), id);
    if (orderIt != m_order.end())
        m_order.erase(orderIt);
}

std::string ConditionPresetRegistry::DuplicatePreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end()) { return std::string(); }

    ConditionPreset copy  = it->second;
    copy.id   = std::string(); // Let CreatePreset generate a new ID
    copy.name = copy.name + " (Copy)";

    return CreatePreset(copy);
}

// ============================================================================
// Query
// ============================================================================

std::vector<std::string> ConditionPresetRegistry::GetAllPresetIDs() const
{
    return m_order;
}

size_t ConditionPresetRegistry::GetPresetCount() const
{
    return m_presets.size();
}

std::vector<std::string>
ConditionPresetRegistry::FindPresetsByName(const std::string& substring) const
{
    std::vector<std::string> result;
    for (const auto& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it == m_presets.end()) { continue; }
        if (it->second.name.find(substring) != std::string::npos)
            result.push_back(id);
    }
    return result;
}

std::vector<ConditionPreset>
ConditionPresetRegistry::GetFilteredPresets(const std::string& filter) const
{
    std::vector<ConditionPreset> result;
    result.reserve(m_order.size());

    if (filter.empty())
    {
        for (const auto& id : m_order)
        {
            auto it = m_presets.find(id);
            if (it != m_presets.end())
                result.push_back(it->second);
        }
        return result;
    }

    const std::string lowerFilter = ToLowerStr(filter);

    for (const auto& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it == m_presets.end()) { continue; }

        const ConditionPreset& p = it->second;
        if (ToLowerStr(p.name).find(lowerFilter) != std::string::npos ||
            ToLowerStr(p.GetPreview()).find(lowerFilter) != std::string::npos)
        {
            result.push_back(p);
        }
    }

    return result;
}

// ============================================================================
// Validation
// ============================================================================

bool ConditionPresetRegistry::ValidatePresetID(const std::string& id) const
{
    return m_presets.count(id) > 0;
}

std::vector<std::string> ConditionPresetRegistry::GetAllErrors() const
{
    return m_errors;
}

void ConditionPresetRegistry::Clear()
{
    m_presets.clear();
    m_order.clear();
    m_errors.clear();
}

// ============================================================================
// Persistence
// ============================================================================

bool ConditionPresetRegistry::Load(const std::string& filepath)
{
    Clear();

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        m_errors.push_back("[ConditionPresetRegistry] Cannot open: " + filepath);
        return false;
    }

    nlohmann::json root;
    try
    {
        file >> root;
    }
    catch (const std::exception& e)
    {
        m_errors.push_back(
            std::string("[ConditionPresetRegistry] JSON parse error: ") + e.what());
        return false;
    }

    if (!root.is_object() || !root.contains("presets") || !root["presets"].is_array())
    {
        m_errors.push_back("[ConditionPresetRegistry] Missing 'presets' array.");
        return false;
    }

    for (const auto& entry : root["presets"])
    {
        if (!entry.is_object()) { continue; }
        ConditionPreset p = ConditionPreset::FromJson(entry);
        if (!p.id.empty())
        {
            m_presets[p.id] = p;
            m_order.push_back(p.id);
        }
    }

    return true;
}

bool ConditionPresetRegistry::Save(const std::string& filepath) const
{
    nlohmann::json root;
    root["version"] = 1;
    root["presets"] = nlohmann::json::array();

    for (const auto& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it != m_presets.end())
            root["presets"].push_back(it->second.ToJson());
    }

    std::ofstream file(filepath);
    if (!file.is_open())
    {
        m_errors.push_back("[ConditionPresetRegistry] Cannot write: " + filepath);
        return false;
    }

    file << root.dump(4);
    return file.good();
}

} // namespace Olympe
