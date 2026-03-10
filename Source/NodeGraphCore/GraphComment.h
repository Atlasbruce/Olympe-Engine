/**
 * @file GraphComment.h
 * @brief Visual comment/annotation system for node graphs (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * GraphComment represents a coloured rectangular annotation that can be
 * overlaid on the graph canvas.  CommentManager manages the collection and
 * provides serialisation helpers.
 *
 * No ImGui dependency — UI code reads the data and draws however it likes.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../third_party/nlohmann/json.hpp"

namespace Olympe {

// ============================================================================
// GraphComment
// ============================================================================

/**
 * @struct GraphComment
 * @brief A rectangular comment box placed on the graph canvas.
 */
struct GraphComment {
    int          id        = -1;           ///< Unique comment ID (assigned by CommentManager)
    std::string  text;                     ///< Text content displayed inside the box
    float        posX      = 0.0f;         ///< Left edge in graph space
    float        posY      = 0.0f;         ///< Top edge in graph space
    float        width     = 200.0f;       ///< Box width in graph space
    float        height    = 80.0f;        ///< Box height in graph space
    unsigned int color     = 0xFFFFAA33u;  ///< RGBA packed colour (default: amber)
    float        fontSize  = 14.0f;        ///< Font size in points
    bool         isVisible = true;         ///< Whether the comment is drawn

    nlohmann::json              ToJson()               const;
    static GraphComment         FromJson(const nlohmann::json& j);
};

// ============================================================================
// CommentManager
// ============================================================================

/**
 * @class CommentManager
 * @brief Singleton that owns all GraphComment instances for the active graph.
 *
 * Typical usage:
 * @code
 *   auto& cm = CommentManager::Get();
 *   GraphComment c;
 *   c.text = "Patrol loop entry";
 *   c.posX = 100.0f;  c.posY = 200.0f;
 *   int id = cm.AddComment(c);
 *   cm.RemoveComment(id);
 * @endcode
 */
class CommentManager {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /** @brief Returns the single shared instance. */
    static CommentManager& Get();

    // -----------------------------------------------------------------------
    // CRUD
    // -----------------------------------------------------------------------

    /**
     * @brief Adds a comment and assigns it a unique ID.
     * @return The assigned comment ID.
     */
    int  AddComment(const GraphComment& comment);

    /** @brief Removes the comment with the given ID.  No-op if not found. */
    void RemoveComment(int commentId);

    /**
     * @brief Returns a pointer to the comment with the given ID.
     * @return Pointer, or nullptr if not found.
     */
    GraphComment* GetComment(int commentId);

    /** @brief Replaces the stored comment data for the given ID. */
    void UpdateComment(int commentId, const GraphComment& comment);

    /** @brief Returns the total number of comments managed. */
    int  GetCommentCount() const;

    /** @brief Removes all comments and resets the ID counter. */
    void Clear();

    // -----------------------------------------------------------------------
    // Serialisation
    // -----------------------------------------------------------------------

    void SaveToJson(nlohmann::json& j)        const;
    void LoadFromJson(const nlohmann::json& j);

private:

    CommentManager();

    std::vector<GraphComment> m_Comments;
    int                       m_NextCommentId;
};

} // namespace Olympe
