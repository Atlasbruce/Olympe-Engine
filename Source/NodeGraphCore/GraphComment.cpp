/**
 * @file GraphComment.cpp
 * @brief CommentManager and GraphComment serialisation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "GraphComment.h"

#include "../system/system_utils.h"

using json = nlohmann::json;

namespace Olympe {

// ============================================================================
// GraphComment serialisation
// ============================================================================

json GraphComment::ToJson() const
{
    json j     = json::object();
    j["id"]    = id;
    j["text"]  = text;
    j["posX"]  = posX;
    j["posY"]  = posY;
    j["w"]     = width;
    j["h"]     = height;
    j["color"] = static_cast<int>(color);
    j["font"]  = fontSize;
    j["vis"]   = isVisible;
    return j;
}

GraphComment GraphComment::FromJson(const json& j)
{
    GraphComment c;

    if (j.contains("id") && j["id"].is_number_integer())
        c.id = j["id"].get<int>();

    if (j.contains("text") && j["text"].is_string())
        c.text = j["text"].get<std::string>();

    if (j.contains("posX") && j["posX"].is_number())
        c.posX = j["posX"].get<float>();

    if (j.contains("posY") && j["posY"].is_number())
        c.posY = j["posY"].get<float>();

    if (j.contains("w") && j["w"].is_number())
        c.width = j["w"].get<float>();

    if (j.contains("h") && j["h"].is_number())
        c.height = j["h"].get<float>();

    if (j.contains("color") && j["color"].is_number_integer())
        c.color = j["color"].get<unsigned int>();

    if (j.contains("font") && j["font"].is_number())
        c.fontSize = j["font"].get<float>();

    if (j.contains("vis") && j["vis"].is_boolean())
        c.isVisible = j["vis"].get<bool>();

    return c;
}

// ============================================================================
// CommentManager — Singleton
// ============================================================================

CommentManager& CommentManager::Get()
{
    static CommentManager s_Instance;
    return s_Instance;
}

CommentManager::CommentManager()
    : m_NextCommentId(1)
{
}

// ============================================================================
// CRUD
// ============================================================================

int CommentManager::AddComment(const GraphComment& comment)
{
    GraphComment c  = comment;
    c.id            = m_NextCommentId++;
    m_Comments.push_back(c);
    return c.id;
}

void CommentManager::RemoveComment(int commentId)
{
    for (size_t i = 0; i < m_Comments.size(); ++i)
    {
        if (m_Comments[i].id == commentId)
        {
            m_Comments.erase(m_Comments.begin() + static_cast<int>(i));
            return;
        }
    }
}

GraphComment* CommentManager::GetComment(int commentId)
{
    for (size_t i = 0; i < m_Comments.size(); ++i)
    {
        if (m_Comments[i].id == commentId)
            return &m_Comments[i];
    }
    return nullptr;
}

void CommentManager::UpdateComment(int commentId, const GraphComment& comment)
{
    for (size_t i = 0; i < m_Comments.size(); ++i)
    {
        if (m_Comments[i].id == commentId)
        {
            GraphComment updated = comment;
            updated.id = commentId;  // preserve ID
            m_Comments[i] = updated;
            return;
        }
    }
    SYSTEM_LOG << "[CommentManager] UpdateComment: ID " << commentId
               << " not found." << std::endl;
}

int CommentManager::GetCommentCount() const
{
    return static_cast<int>(m_Comments.size());
}

void CommentManager::Clear()
{
    m_Comments.clear();
    m_NextCommentId = 1;
}

// ============================================================================
// Serialisation
// ============================================================================

void CommentManager::SaveToJson(json& j) const
{
    json arr = json::array();
    for (size_t i = 0; i < m_Comments.size(); ++i)
        arr.push_back(m_Comments[i].ToJson());
    j["comments"] = arr;
}

void CommentManager::LoadFromJson(const json& j)
{
    Clear();
    if (!j.contains("comments") || !j["comments"].is_array())
        return;

    const json& arr = j["comments"];
    for (auto it = arr.begin(); it != arr.end(); ++it)
    {
        GraphComment c = GraphComment::FromJson(*it);
        // Re-insert preserving original ID but updating the counter
        if (c.id >= m_NextCommentId)
            m_NextCommentId = c.id + 1;
        m_Comments.push_back(c);
    }
}

} // namespace Olympe
