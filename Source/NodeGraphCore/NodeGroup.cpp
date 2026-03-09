/**
 * @file NodeGroup.cpp
 * @brief GroupManager implementation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "NodeGroup.h"

#include "../system/system_utils.h"

using json = nlohmann::json;

namespace Olympe {

// ============================================================================
// NodeGroup serialisation
// ============================================================================

json NodeGroup::ToJson() const
{
    json j             = json::object();
    j["id"]            = id;
    j["name"]          = name;
    j["isCollapsed"]   = isCollapsed;
    j["collapsedPosX"] = collapsedPosX;
    j["collapsedPosY"] = collapsedPosY;
    j["headerColor"]   = static_cast<int>(headerColor);

    json ids = json::array();
    for (size_t i = 0; i < nodeIds.size(); ++i)
        ids.push_back(nodeIds[i]);
    j["nodeIds"] = ids;

    return j;
}

NodeGroup NodeGroup::FromJson(const json& j)
{
    NodeGroup g;

    if (j.contains("id") && j["id"].is_number_integer())
        g.id = j["id"].get<int>();

    if (j.contains("name") && j["name"].is_string())
        g.name = j["name"].get<std::string>();

    if (j.contains("isCollapsed") && j["isCollapsed"].is_boolean())
        g.isCollapsed = j["isCollapsed"].get<bool>();

    if (j.contains("collapsedPosX") && j["collapsedPosX"].is_number())
        g.collapsedPosX = j["collapsedPosX"].get<float>();

    if (j.contains("collapsedPosY") && j["collapsedPosY"].is_number())
        g.collapsedPosY = j["collapsedPosY"].get<float>();

    if (j.contains("headerColor") && j["headerColor"].is_number_integer())
        g.headerColor = j["headerColor"].get<unsigned int>();

    if (j.contains("nodeIds") && j["nodeIds"].is_array())
    {
        const json& arr = j["nodeIds"];
        for (auto it = arr.begin(); it != arr.end(); ++it)
        {
            if (it->is_number_integer())
                g.nodeIds.push_back(it->get<int>());
        }
    }

    return g;
}

// ============================================================================
// GroupManager — Singleton
// ============================================================================

GroupManager& GroupManager::Get()
{
    static GroupManager s_Instance;
    return s_Instance;
}

GroupManager::GroupManager()
    : m_NextGroupId(1)
{
}

// ============================================================================
// Lifecycle
// ============================================================================

int GroupManager::CreateGroup(const std::string& name, const std::vector<int>& nodeIds)
{
    NodeGroup g;
    g.id      = m_NextGroupId++;
    g.name    = name;
    g.nodeIds = nodeIds;
    m_Groups.push_back(g);
    return g.id;
}

void GroupManager::DeleteGroup(int groupId)
{
    for (size_t i = 0; i < m_Groups.size(); ++i)
    {
        if (m_Groups[i].id == groupId)
        {
            m_Groups.erase(m_Groups.begin() + static_cast<int>(i));
            return;
        }
    }
}

void GroupManager::CollapseGroup(int groupId)
{
    NodeGroup* g = GetGroup(groupId);
    if (g)
        g->isCollapsed = true;
}

void GroupManager::ExpandGroup(int groupId)
{
    NodeGroup* g = GetGroup(groupId);
    if (g)
        g->isCollapsed = false;
}

NodeGroup* GroupManager::GetGroup(int groupId)
{
    for (size_t i = 0; i < m_Groups.size(); ++i)
    {
        if (m_Groups[i].id == groupId)
            return &m_Groups[i];
    }
    return nullptr;
}

int GroupManager::GetGroupCount() const
{
    return static_cast<int>(m_Groups.size());
}

void GroupManager::Clear()
{
    m_Groups.clear();
    m_NextGroupId = 1;
}

// ============================================================================
// Serialisation
// ============================================================================

void GroupManager::SaveToJson(json& j) const
{
    json arr = json::array();
    for (size_t i = 0; i < m_Groups.size(); ++i)
        arr.push_back(m_Groups[i].ToJson());
    j["groups"] = arr;
}

void GroupManager::LoadFromJson(const json& j)
{
    Clear();
    if (!j.contains("groups") || !j["groups"].is_array())
        return;

    const json& arr = j["groups"];
    for (auto it = arr.begin(); it != arr.end(); ++it)
    {
        NodeGroup g = NodeGroup::FromJson(*it);
        if (g.id >= m_NextGroupId)
            m_NextGroupId = g.id + 1;
        m_Groups.push_back(g);
    }
}

} // namespace Olympe
