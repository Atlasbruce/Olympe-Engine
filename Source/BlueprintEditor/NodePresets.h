/**
 * @file NodePresets.h
 * @brief Node preset / template system for frequently used node configurations (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * NodePresets allows users to save and recall commonly used node configurations.
 * A preset can cover a single node (its type and parameter defaults) or a set
 * of connected nodes (a mini-graph snippet).
 *
 * NodePresetManager is a Meyers singleton that owns all presets.
 * Presets are serialised to / deserialised from JSON.
 *
 * No ImGui dependency.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../third_party/nlohmann/json.hpp"

namespace Olympe {

// ============================================================================
// NodePreset
// ============================================================================

/**
 * @struct NodePreset
 * @brief A saved node configuration that can be applied to the active graph.
 */
struct NodePreset {
    std::string      name;         ///< Unique preset identifier
    std::string      description;  ///< Human-readable description
    std::string      category;     ///< Category for grouping in the UI
    nlohmann::json   nodeData;     ///< JSON snapshot of the node(s)
    std::vector<int> nodeIds;      ///< For multi-node presets: list of involved IDs

    nlohmann::json         ToJson()               const;
    static NodePreset      FromJson(const nlohmann::json& j);
};

// ============================================================================
// NodePresetManager
// ============================================================================

/**
 * @class NodePresetManager
 * @brief Singleton that stores, serialises, and retrieves NodePreset instances.
 *
 * Typical usage:
 * @code
 *   auto& pm = NodePresetManager::Get();
 *   pm.LoadPresets("Config/presets.json");
 *
 *   NodePreset p;
 *   p.name = "MyPatrol";
 *   p.category = "AI";
 *   pm.AddPreset(p);
 *
 *   auto* found = pm.GetPreset("MyPatrol");
 * @endcode
 */
class NodePresetManager {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /** @brief Returns the single shared instance. */
    static NodePresetManager& Get();

    // -----------------------------------------------------------------------
    // Persistence
    // -----------------------------------------------------------------------

    /**
     * @brief Loads presets from a JSON file at @p path.
     *        Existing presets are replaced.
     */
    void LoadPresets(const std::string& path);

    /**
     * @brief Saves all current presets to a JSON file at @p path.
     */
    void SavePresets(const std::string& path) const;

    // -----------------------------------------------------------------------
    // CRUD
    // -----------------------------------------------------------------------

    /** @brief Adds or replaces a preset (matched by name). */
    void AddPreset(const NodePreset& preset);

    /** @brief Removes the preset with the given name.  No-op if not found. */
    void RemovePreset(const std::string& name);

    /**
     * @brief Returns a pointer to the preset with the given name.
     * @return Pointer, or nullptr if not found.
     */
    NodePreset* GetPreset(const std::string& name);

    /**
     * @brief Returns all presets that belong to @p category.
     */
    std::vector<NodePreset> GetPresetsInCategory(const std::string& category) const;

    /** @brief Returns the total number of stored presets. */
    int GetPresetCount() const;

    /** @brief Removes all presets. */
    void Clear();

private:

    NodePresetManager();

    std::vector<NodePreset> m_Presets;
};

} // namespace Olympe
