/**
 * @file ConditionPresetRegistry.cpp
 * @brief Implementation of ConditionPresetRegistry — CRUD, Load/Save.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPresetRegistry.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace Olympe {

// ---------------------------------------------------------------------------
// Private: UUID generator
// ---------------------------------------------------------------------------

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

    // UUID v4 version and variant bits
    b[6] = (b[6] & 0x0F) | 0x40;
    b[8] = (b[8] & 0x3F) | 0x80;

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

// ---------------------------------------------------------------------------
// CRUD
// ---------------------------------------------------------------------------

std::string ConditionPresetRegistry::CreatePreset(const ConditionPreset& preset)
{
    ConditionPreset p = preset;

    if (p.id.empty())
        p.id = GenerateID();

    // If the ID already exists, return the existing one without overwriting.
    if (m_presets.find(p.id) != m_presets.end())
        return p.id;

    m_presets[p.id] = p;
    m_order.push_back(p.id);
    return p.id;
}

ConditionPreset* ConditionPresetRegistry::GetPreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
        return nullptr;
    return &it->second;
}

const ConditionPreset* ConditionPresetRegistry::GetPreset(const std::string& id) const
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
        m_errors.push_back("[ConditionPresetRegistry] UpdatePreset: ID not found: " + id);
        return;
    }

    ConditionPreset u = updated;
    u.id = id; // ensure ID field is consistent
    it->second = u;
}

void ConditionPresetRegistry::DeletePreset(const std::string& id)
{
    m_presets.erase(id);
    m_order.erase(
        std::remove(m_order.begin(), m_order.end(), id),
        m_order.end());
}

std::string ConditionPresetRegistry::DuplicatePreset(const std::string& id)
{
    auto it = m_presets.find(id);
    if (it == m_presets.end())
    {
        m_errors.push_back("[ConditionPresetRegistry] DuplicatePreset: ID not found: " + id);
        return "";
    }

    ConditionPreset copy = it->second;
    copy.id   = GenerateID();
    copy.name = copy.name + " (Copy)";

    m_presets[copy.id] = copy;
    m_order.push_back(copy.id);
    return copy.id;
}

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

std::vector<std::string> ConditionPresetRegistry::GetAllPresetIDs() const
{
    return m_order;
}

size_t ConditionPresetRegistry::GetPresetCount() const
{
    return m_presets.size();
}

std::vector<std::string> ConditionPresetRegistry::FindPresetsByName(
    const std::string& substring) const
{
    std::vector<std::string> result;
    for (const std::string& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it != m_presets.end())
        {
            if (it->second.name.find(substring) != std::string::npos)
                result.push_back(id);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool ConditionPresetRegistry::ValidatePresetID(const std::string& id) const
{
    return m_presets.find(id) != m_presets.end();
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

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

bool ConditionPresetRegistry::Load(const std::string& filepath)
{
    Clear();

    std::ifstream ifs(filepath.c_str());
    if (!ifs.is_open())
    {
        m_errors.push_back("[ConditionPresetRegistry] Load: cannot open '" + filepath + "'");
        return false;
    }

    nlohmann::json root;
    try
    {
        ifs >> root;
    }
    catch (const std::exception& e)
    {
        m_errors.push_back(std::string("[ConditionPresetRegistry] Load: parse error: ") + e.what());
        return false;
    }

    if (!root.contains("presets") || !root["presets"].is_array())
    {
        m_errors.push_back("[ConditionPresetRegistry] Load: missing 'presets' array in '"
                           + filepath + "'");
        return false;
    }

    const nlohmann::json& arr = root["presets"];
    for (size_t i = 0; i < arr.size(); ++i)
    {
        ConditionPreset p = ConditionPreset::FromJson(arr[i]);

        if (p.id.empty())
        {
            m_errors.push_back("[ConditionPresetRegistry] Load: entry " +
                               std::to_string(i) + " has empty id — skipping");
            continue;
        }

        if (m_presets.find(p.id) != m_presets.end())
        {
            m_errors.push_back("[ConditionPresetRegistry] Load: duplicate ID '" +
                               p.id + "' at entry " + std::to_string(i) + " — skipping");
            continue;
        }

        m_presets[p.id] = p;
        m_order.push_back(p.id);
    }

    return true;
}

bool ConditionPresetRegistry::Save(const std::string& filepath) const
{
    std::ofstream ofs(filepath.c_str());
    if (!ofs.is_open())
    {
        m_errors.push_back("[ConditionPresetRegistry] Save: cannot open '" + filepath + "' for writing");
        return false;
    }

    nlohmann::json root = nlohmann::json::object();
    root["version"] = 1;

    nlohmann::json arr = nlohmann::json::array();
    for (const std::string& id : m_order)
    {
        auto it = m_presets.find(id);
        if (it != m_presets.end())
            arr.push_back(it->second.ToJson());
    }
    root["presets"] = arr;

    ofs << root.dump(2);
    return ofs.good();
}

} // namespace Olympe
