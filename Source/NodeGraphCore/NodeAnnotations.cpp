/**
 * @file NodeAnnotations.cpp
 * @brief Implementation of NodeAnnotationsManager (Phase 2.0)
 * @author Olympe Engine
 * @date 2026-02-19
 */

#include "NodeAnnotations.h"
#include "../system/system_utils.h"

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Query
// ============================================================================

bool NodeAnnotationsManager::HasAnnotation(int nodeId) const
{
    return m_annotations.find(nodeId) != m_annotations.end();
}

NodeAnnotation* NodeAnnotationsManager::GetAnnotation(int nodeId)
{
    auto it = m_annotations.find(nodeId);
    if (it != m_annotations.end())
    {
        return &it->second;
    }
    return nullptr;
}

const NodeAnnotation* NodeAnnotationsManager::GetAnnotation(int nodeId) const
{
    auto it = m_annotations.find(nodeId);
    if (it != m_annotations.end())
    {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// Mutation
// ============================================================================

void NodeAnnotationsManager::SetBreakpoint(int nodeId, bool enabled)
{
    NodeAnnotation& ann = GetOrCreate(nodeId);
    ann.hasBreakpoint = enabled;
    SYSTEM_LOG << "[NodeAnnotations] Node " << nodeId
               << " breakpoint: " << (enabled ? "ON" : "OFF") << std::endl;
}

void NodeAnnotationsManager::SetComment(int nodeId, const std::string& text)
{
    NodeAnnotation& ann = GetOrCreate(nodeId);
    ann.comment = text;
    SYSTEM_LOG << "[NodeAnnotations] Node " << nodeId << " comment set" << std::endl;
}

void NodeAnnotationsManager::SetColor(int nodeId, float r, float g, float b, float a)
{
    NodeAnnotation& ann = GetOrCreate(nodeId);
    ann.colorR = r;
    ann.colorG = g;
    ann.colorB = b;
    ann.colorA = a;
}

void NodeAnnotationsManager::ClearAnnotation(int nodeId)
{
    m_annotations.erase(nodeId);
    SYSTEM_LOG << "[NodeAnnotations] Node " << nodeId << " annotation cleared" << std::endl;
}

// ============================================================================
// Accessors
// ============================================================================

const std::map<int, NodeAnnotation>& NodeAnnotationsManager::GetAll() const
{
    return m_annotations;
}

// ============================================================================
// Serialization
// ============================================================================

json NodeAnnotationsManager::ToJson() const
{
    json arr = json::array();
    for (auto it = m_annotations.begin(); it != m_annotations.end(); ++it)
    {
        const NodeAnnotation& ann = it->second;
        json entry = json::object();
        entry["nodeId"] = ann.nodeId;
        entry["hasBreakpoint"] = ann.hasBreakpoint;
        entry["comment"] = ann.comment;
        json colorObj = json::object();
        colorObj["r"] = ann.colorR;
        colorObj["g"] = ann.colorG;
        colorObj["b"] = ann.colorB;
        colorObj["a"] = ann.colorA;
        entry["color"] = colorObj;
        arr.push_back(entry);
    }
    return arr;
}

void NodeAnnotationsManager::FromJson(const json& j)
{
    m_annotations.clear();

    if (!j.is_array())
    {
        return;
    }

    for (size_t i = 0; i < j.size(); ++i)
    {
        const json& entry = j[i];
        if (!entry.is_object())
        {
            continue;
        }

        int nodeId = JsonHelper::GetInt(entry, "nodeId", 0);
        if (nodeId <= 0)
        {
            continue;
        }

        NodeAnnotation ann;
        ann.nodeId = nodeId;
        ann.hasBreakpoint = JsonHelper::GetBool(entry, "hasBreakpoint", false);
        ann.comment = JsonHelper::GetString(entry, "comment", "");

        if (entry.contains("color") && entry["color"].is_object())
        {
            const json& col = entry["color"];
            ann.colorR = JsonHelper::GetFloat(col, "r", 1.0f);
            ann.colorG = JsonHelper::GetFloat(col, "g", 1.0f);
            ann.colorB = JsonHelper::GetFloat(col, "b", 1.0f);
            ann.colorA = JsonHelper::GetFloat(col, "a", 1.0f);
        }

        m_annotations[nodeId] = ann;
    }
}

// ============================================================================
// Private helpers
// ============================================================================

NodeAnnotation& NodeAnnotationsManager::GetOrCreate(int nodeId)
{
    auto it = m_annotations.find(nodeId);
    if (it != m_annotations.end())
    {
        return it->second;
    }

    NodeAnnotation ann;
    ann.nodeId = nodeId;
    m_annotations[nodeId] = ann;
    return m_annotations[nodeId];
}

} // namespace NodeGraph
} // namespace Olympe
