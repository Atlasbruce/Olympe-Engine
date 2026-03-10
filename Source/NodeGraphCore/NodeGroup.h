/**
 * @file NodeGroup.h
 * @brief Node grouping and collapse system for node graphs (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * NodeGroup allows the user to logically associate a set of nodes under a
 * named, coloured header.  A collapsed group renders only the header bar,
 * hiding the individual nodes to reduce visual clutter.
 *
 * GroupManager is a Meyers singleton that owns all groups.  It is UI-agnostic;
 * the rendering layer reads the group data each frame.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../third_party/nlohmann/json.hpp"

namespace Olympe {

// ============================================================================
// NodeGroup
// ============================================================================

/**
 * @struct NodeGroup
 * @brief A named set of node IDs that can be collapsed into a single header.
 */
struct NodeGroup {
    int                  id             = -1;          ///< Unique group ID
    std::string          name;                         ///< Display name for the group
    std::vector<int>     nodeIds;                      ///< IDs of member nodes
    bool                 isCollapsed    = false;       ///< True = render as header only
    float                collapsedPosX  = 0.0f;        ///< Position used when collapsed
    float                collapsedPosY  = 0.0f;
    unsigned int         headerColor    = 0xFF4444FFu; ///< RGBA packed (default: blue)

    nlohmann::json           ToJson()               const;
    static NodeGroup         FromJson(const nlohmann::json& j);
};

// ============================================================================
// GroupManager
// ============================================================================

/**
 * @class GroupManager
 * @brief Singleton that owns all NodeGroup instances for the active graph.
 *
 * Typical usage:
 * @code
 *   auto& gm = GroupManager::Get();
 *   std::vector<int> ids = {1, 2, 3};
 *   int gid = gm.CreateGroup("Patrol Logic", ids);
 *   gm.CollapseGroup(gid);
 * @endcode
 */
class GroupManager {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /** @brief Returns the single shared instance. */
    static GroupManager& Get();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Creates a new group from the given node IDs.
     * @return The assigned group ID.
     */
    int  CreateGroup(const std::string& name, const std::vector<int>& nodeIds);

    /** @brief Deletes the group with the given ID.  No-op if not found. */
    void DeleteGroup(int groupId);

    /** @brief Marks the group as collapsed.  No-op if not found. */
    void CollapseGroup(int groupId);

    /** @brief Marks the group as expanded.  No-op if not found. */
    void ExpandGroup(int groupId);

    /**
     * @brief Returns a pointer to the group with the given ID.
     * @return Pointer, or nullptr if not found.
     */
    NodeGroup* GetGroup(int groupId);

    /** @brief Returns the total number of groups. */
    int  GetGroupCount() const;

    /** @brief Removes all groups and resets the ID counter. */
    void Clear();

    // -----------------------------------------------------------------------
    // Serialisation
    // -----------------------------------------------------------------------

    void SaveToJson(nlohmann::json& j)        const;
    void LoadFromJson(const nlohmann::json& j);

private:

    GroupManager();

    std::vector<NodeGroup> m_Groups;
    int                    m_NextGroupId;
};

} // namespace Olympe
