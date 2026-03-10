/**
 * @file NodePresets.cpp
 * @brief NodePresetManager implementation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "NodePresets.h"

#include <fstream>

#include "../system/system_utils.h"

using json = nlohmann::json;

namespace Olympe {

// ============================================================================
// NodePreset serialisation
// ============================================================================

json NodePreset::ToJson() const
{
    json j          = json::object();
    j["name"]       = name;
    j["description"]= description;
    j["category"]   = category;
    j["nodeData"]   = nodeData;

    json ids = json::array();
    for (size_t i = 0; i < nodeIds.size(); ++i)
        ids.push_back(nodeIds[i]);
    j["nodeIds"] = ids;

    return j;
}

NodePreset NodePreset::FromJson(const json& j)
{
    NodePreset p;

    if (j.contains("name") && j["name"].is_string())
        p.name = j["name"].get<std::string>();

    if (j.contains("description") && j["description"].is_string())
        p.description = j["description"].get<std::string>();

    if (j.contains("category") && j["category"].is_string())
        p.category = j["category"].get<std::string>();

    if (j.contains("nodeData"))
        p.nodeData = j["nodeData"];

    if (j.contains("nodeIds") && j["nodeIds"].is_array())
    {
        const json& arr = j["nodeIds"];
        for (auto it = arr.begin(); it != arr.end(); ++it)
        {
            if (it->is_number_integer())
                p.nodeIds.push_back(it->get<int>());
        }
    }

    return p;
}

// ============================================================================
// NodePresetManager — Singleton
// ============================================================================

NodePresetManager& NodePresetManager::Get()
{
    static NodePresetManager s_Instance;
    return s_Instance;
}

NodePresetManager::NodePresetManager()
{
}

// ============================================================================
// Persistence
// ============================================================================

void NodePresetManager::LoadPresets(const std::string& path)
{
    Clear();

    std::ifstream ifs(path.c_str());
    if (!ifs.is_open())
    {
        SYSTEM_LOG << "[NodePresetManager] Cannot open '" << path << "' for reading."
                   << std::endl;
        return;
    }

    json root;
    try
    {
        ifs >> root;
    }
    catch (...)
    {
        SYSTEM_LOG << "[NodePresetManager] JSON parse error in '" << path << "'."
                   << std::endl;
        return;
    }

    if (!root.contains("presets") || !root["presets"].is_array())
    {
        SYSTEM_LOG << "[NodePresetManager] '" << path << "' has no 'presets' array."
                   << std::endl;
        return;
    }

    const json& arr = root["presets"];
    for (auto it = arr.begin(); it != arr.end(); ++it)
        m_Presets.push_back(NodePreset::FromJson(*it));

    SYSTEM_LOG << "[NodePresetManager] Loaded " << static_cast<int>(m_Presets.size())
               << " preset(s) from '" << path << "'." << std::endl;
}

void NodePresetManager::SavePresets(const std::string& path) const
{
    json root = json::object();
    json arr  = json::array();
    for (size_t i = 0; i < m_Presets.size(); ++i)
        arr.push_back(m_Presets[i].ToJson());
    root["presets"] = arr;

    std::ofstream ofs(path.c_str());
    if (!ofs.is_open())
    {
        SYSTEM_LOG << "[NodePresetManager] Cannot open '" << path << "' for writing."
                   << std::endl;
        return;
    }

    ofs << root.dump(2);
    SYSTEM_LOG << "[NodePresetManager] Saved " << static_cast<int>(m_Presets.size())
               << " preset(s) to '" << path << "'." << std::endl;
}

// ============================================================================
// CRUD
// ============================================================================

void NodePresetManager::AddPreset(const NodePreset& preset)
{
    // Replace if a preset with the same name already exists
    for (size_t i = 0; i < m_Presets.size(); ++i)
    {
        if (m_Presets[i].name == preset.name)
        {
            m_Presets[i] = preset;
            return;
        }
    }
    m_Presets.push_back(preset);
}

void NodePresetManager::RemovePreset(const std::string& name)
{
    for (size_t i = 0; i < m_Presets.size(); ++i)
    {
        if (m_Presets[i].name == name)
        {
            m_Presets.erase(m_Presets.begin() + static_cast<int>(i));
            return;
        }
    }
}

NodePreset* NodePresetManager::GetPreset(const std::string& name)
{
    for (size_t i = 0; i < m_Presets.size(); ++i)
    {
        if (m_Presets[i].name == name)
            return &m_Presets[i];
    }
    return nullptr;
}

std::vector<NodePreset> NodePresetManager::GetPresetsInCategory(
    const std::string& category) const
{
    std::vector<NodePreset> result;
    for (size_t i = 0; i < m_Presets.size(); ++i)
    {
        if (m_Presets[i].category == category)
            result.push_back(m_Presets[i]);
    }
    return result;
}

int NodePresetManager::GetPresetCount() const
{
    return static_cast<int>(m_Presets.size());
}

void NodePresetManager::Clear()
{
    m_Presets.clear();
}

} // namespace Olympe
