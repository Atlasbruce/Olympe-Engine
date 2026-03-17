/**
 * @file ConditionPresetRegistry.cpp
 * @brief Implementation of ConditionPresetRegistry (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPresetRegistry.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace Olympe {

// ============================================================================
// CRUD
// ============================================================================

std::string ConditionPresetRegistry::CreatePreset(const ConditionPreset& preset)
{
    // If the ID is already in the registry, return it unchanged (no-op)
    if (!preset.id.empty() && m_presets.count(preset.id) != 0)
        return preset.id;

    ConditionPreset toStore = preset;

    if (toStore.id.empty())
        toStore.id = GenerateID();

    m_presets[toStore.id] = toStore;
    m_order.push_back(toStore.id);
    return toStore.id;
}

ConditionPreset* ConditionPresetRegistry::GetPreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
        return nullptr;
    return &it->second;
}

const ConditionPreset*
ConditionPresetRegistry::GetPreset(const std::string& id) const
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
        return nullptr;
    return &it->second;
}

void ConditionPresetRegistry::UpdatePreset(const std::string& id,
                                            const ConditionPreset& updated)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
    {
        m_errors.push_back("UpdatePreset: ID not found: " + id);
        return;
    }
    ConditionPreset copy = updated;
    copy.id = id; // force ID to match
    it->second = copy;
}

void ConditionPresetRegistry::DeletePreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
        return;

    m_presets.erase(it);
    m_order.erase(std::remove(m_order.begin(), m_order.end(), id), m_order.end());
}

std::string ConditionPresetRegistry::DuplicatePreset(const std::string& id)
{
    const ConditionPreset* src = GetPreset(id);
    if (!src)
        return "";

    ConditionPreset copy = *src;
    copy.id   = "";           // will be re-assigned
    copy.name = src->name + " (Copy)";
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
        if (it == m_presets.end())
            continue;

        const std::string& name = it->second.name;
        if (name.find(substring) != std::string::npos)
            result.push_back(id);
    }
    return result;
}

// ============================================================================
// Validation
// ============================================================================

bool ConditionPresetRegistry::ValidatePresetID(const std::string& id) const
{
    return m_presets.count(id) != 0;
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
    m_errors.clear();

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        m_errors.push_back("Load: cannot open file: " + filepath);
        return false;
    }

    // Read file content
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    nlohmann::json root;
    try
    {
        root = nlohmann::json::parse(content);
    }
    catch (...)
    {
        m_errors.push_back("Load: JSON parse error in file: " + filepath);
        return false;
    }

    Clear();

    if (!root.is_object() || !root.contains("presets"))
    {
        m_errors.push_back("Load: missing 'presets' key in: " + filepath);
        return false;
    }

    const auto& presetsArr = root["presets"];
    if (!presetsArr.is_array())
    {
        m_errors.push_back("Load: 'presets' is not an array in: " + filepath);
        return false;
    }

    for (const auto& item : presetsArr)
    {
        ConditionPreset p = ConditionPreset::FromJson(item);
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
    m_errors.clear();

    nlohmann::json root = nlohmann::json::object();
    root["version"] = 1;

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it != m_presets.end())
            arr.push_back(it->second.ToJson());
    }
    root["presets"] = arr;

    std::ofstream file(filepath);
    if (!file.is_open())
    {
        m_errors.push_back("Save: cannot open file for writing: " + filepath);
        return false;
    }

    file << root.dump(4);
    return file.good();
}

// ============================================================================
// UUID generation
// ============================================================================

/*static*/
std::string ConditionPresetRegistry::GenerateID()
{
    static bool seeded = false;
    if (!seeded)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    unsigned int b[16];
    for (int i = 0; i < 16; ++i)
        b[i] = static_cast<unsigned int>(std::rand()) & 0xFF;

    b[6] = (b[6] & 0x0F) | 0x40; // version 4
    b[8] = (b[8] & 0x3F) | 0x80; // variant

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            oss << '-';
        oss << std::setw(2) << b[i];
    }

    return "preset_" + oss.str();
}

} // namespace Olympe
