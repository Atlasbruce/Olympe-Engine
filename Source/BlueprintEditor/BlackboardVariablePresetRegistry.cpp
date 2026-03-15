/**
 * @file BlackboardVariablePresetRegistry.cpp
 * @brief Implementation of BlackboardVariablePresetRegistry.
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlackboardVariablePresetRegistry.h"

#include "../json_helper.h"
#include "../system/system_utils.h"

#include <algorithm>
#include <fstream>

namespace Olympe {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

/*static*/
BlackboardVariablePresetRegistry& BlackboardVariablePresetRegistry::Instance()
{
    static BlackboardVariablePresetRegistry s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// LoadFromFile
// ---------------------------------------------------------------------------

bool BlackboardVariablePresetRegistry::LoadFromFile(const std::string& filePath)
{
    Clear();

    json root;
    if (!JsonHelper::LoadJsonFromFile(filePath, root))
    {
        SYSTEM_LOG << "[BlackboardVariablePresetRegistry] LoadFromFile: failed to open '"
                   << filePath << "'\n";
        return false;
    }

    if (!root.contains("availableVariables") || !root["availableVariables"].is_array())
    {
        SYSTEM_LOG << "[BlackboardVariablePresetRegistry] LoadFromFile: missing or invalid"
                      " 'availableVariables' array in '" << filePath << "'\n";
        return false;
    }

    const json& arr = root["availableVariables"];
    m_presets.reserve(arr.size());

    for (size_t i = 0; i < arr.size(); ++i)
    {
        const json& item = arr[i];

        if (!item.contains("name") || !item["name"].is_string())
        {
            SYSTEM_LOG << "[BlackboardVariablePresetRegistry] LoadFromFile: entry " << i
                       << " missing 'name' — skipping\n";
            continue;
        }

        BlackboardVariablePreset preset;
        preset.name         = JsonHelper::GetString(item, "name");
        preset.type         = JsonHelper::GetString(item, "type");
        preset.description  = JsonHelper::GetString(item, "description");
        preset.category     = JsonHelper::GetString(item, "category");
        preset.defaultValue = JsonHelper::GetString(item, "default");

        if (preset.name.empty())
        {
            SYSTEM_LOG << "[BlackboardVariablePresetRegistry] LoadFromFile: entry " << i
                       << " has empty 'name' — skipping\n";
            continue;
        }

        if (m_nameIndex.find(preset.name) != m_nameIndex.end())
        {
            SYSTEM_LOG << "[BlackboardVariablePresetRegistry] LoadFromFile: duplicate preset '"
                       << preset.name << "' — skipping\n";
            continue;
        }

        m_nameIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }

    // Sort by name for stable ordering
    std::sort(m_presets.begin(), m_presets.end(),
              [](const BlackboardVariablePreset& a, const BlackboardVariablePreset& b) {
                  return a.name < b.name;
              });

    // Rebuild index after sort
    m_nameIndex.clear();
    for (size_t i = 0; i < m_presets.size(); ++i)
        m_nameIndex[m_presets[i].name] = i;

    SYSTEM_LOG << "[BlackboardVariablePresetRegistry] LoadFromFile: loaded "
               << m_presets.size() << " presets from '" << filePath << "'\n";
    return true;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

const std::vector<BlackboardVariablePreset>&
BlackboardVariablePresetRegistry::GetAllPresets() const
{
    return m_presets;
}

const BlackboardVariablePreset*
BlackboardVariablePresetRegistry::GetPreset(const std::string& name) const
{
    std::unordered_map<std::string, size_t>::const_iterator it = m_nameIndex.find(name);
    if (it == m_nameIndex.end())
        return nullptr;
    return &m_presets[it->second];
}

std::vector<BlackboardVariablePreset>
BlackboardVariablePresetRegistry::GetByCategory(const std::string& category) const
{
    std::vector<BlackboardVariablePreset> result;
    for (size_t i = 0; i < m_presets.size(); ++i)
    {
        if (m_presets[i].category == category)
            result.push_back(m_presets[i]);
    }
    return result;
}

std::vector<std::string> BlackboardVariablePresetRegistry::GetAllCategories() const
{
    std::vector<std::string> categories;
    for (size_t i = 0; i < m_presets.size(); ++i)
    {
        const std::string& cat = m_presets[i].category;
        if (cat.empty())
            continue;
        bool found = false;
        for (size_t j = 0; j < categories.size(); ++j)
        {
            if (categories[j] == cat)
            {
                found = true;
                break;
            }
        }
        if (!found)
            categories.push_back(cat);
    }
    std::sort(categories.begin(), categories.end());
    return categories;
}

bool BlackboardVariablePresetRegistry::HasPreset(const std::string& name) const
{
    return m_nameIndex.find(name) != m_nameIndex.end();
}

size_t BlackboardVariablePresetRegistry::GetCount() const
{
    return m_presets.size();
}

void BlackboardVariablePresetRegistry::Clear()
{
    m_presets.clear();
    m_nameIndex.clear();
}

} // namespace Olympe
