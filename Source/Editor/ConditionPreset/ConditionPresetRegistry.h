/**
 * @file ConditionPresetRegistry.h
 * @brief Global registry for ConditionPreset objects, with CRUD, persistence, and validation.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * ConditionPresetRegistry owns all ConditionPreset instances for a project.
 * It is persisted to and loaded from
 * `./Blueprints/Presets/condition_presets.json`.
 *
 * Typical usage:
 * @code
 *   ConditionPresetRegistry registry;
 *   registry.Load("./Blueprints/Presets/condition_presets.json");
 *
 *   ConditionPreset p;
 *   p.name = "Condition #1";
 *   p.left  = Operand::CreateVariable("mHealth");
 *   p.op    = ComparisonOp::LessEqual;
 *   p.right = Operand::CreateConst(2.0);
 *   std::string id = registry.CreatePreset(p);
 *
 *   registry.Save("./Blueprints/Presets/condition_presets.json");
 * @endcode
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <map>
#include <string>
#include <vector>
#include "ConditionPreset.h"
#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {

/**
 * @class ConditionPresetRegistry
 * @brief Manages the global pool of ConditionPreset objects.
 *
 * @details
 * Presets are stored in insertion order using a std::map keyed by UUID.
 * An ordered list (m_order) preserves display order in the UI.
 */
class ConditionPresetRegistry {
public:

    ConditionPresetRegistry() = default;

    // Non-copyable (owns presets)
    ConditionPresetRegistry(const ConditionPresetRegistry&) = delete;
    ConditionPresetRegistry& operator=(const ConditionPresetRegistry&) = delete;

    // -----------------------------------------------------------------------
    // CRUD
    // -----------------------------------------------------------------------

    /**
     * @brief Adds a preset to the registry.
     *
     * If `preset.id` is empty a new UUID is generated.  If the ID already
     * exists the operation is a no-op and the existing ID is returned.
     *
     * @param preset Preset to add (id may be empty — will be assigned).
     * @return The UUID of the stored preset.
     */
    std::string CreatePreset(const ConditionPreset& preset);

    /**
     * @brief Returns a mutable pointer to the preset, or nullptr if not found.
     * @param id Preset UUID.
     */
    ConditionPreset* GetPreset(const std::string& id);

    /**
     * @brief Returns a const pointer to the preset, or nullptr if not found.
     * @param id Preset UUID.
     */
    const ConditionPreset* GetPreset(const std::string& id) const;

    /**
     * @brief Replaces the data of an existing preset (id must already exist).
     *
     * If the ID is not found, logs an error and returns without modifying state.
     * The preset's id field in `updated` is forced to match `id`.
     *
     * @param id      Preset UUID to update.
     * @param updated New preset data.
     */
    void UpdatePreset(const std::string& id, const ConditionPreset& updated);

    /**
     * @brief Removes a preset from the registry.
     *
     * If the ID is not found, the call is a no-op.
     * @param id Preset UUID to remove.
     */
    void DeletePreset(const std::string& id);

    /**
     * @brief Creates an independent copy of an existing preset with a new UUID.
     *
     * The copy's name is suffixed with " (Copy)".
     * Returns the new UUID, or an empty string if the source ID is not found.
     *
     * @param id Source preset UUID.
     * @return UUID of the newly created duplicate.
     */
    std::string DuplicatePreset(const std::string& id);

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    /**
     * @brief Returns all preset UUIDs in display order.
     */
    std::vector<std::string> GetAllPresetIDs() const;

    /**
     * @brief Returns the total number of presets in the registry.
     */
    size_t GetPresetCount() const;

    /**
     * @brief Returns UUIDs of all presets whose name contains a substring.
     *
     * Case-sensitive search.
     * @param substring Search string.
     */
    std::vector<std::string> FindPresetsByName(const std::string& substring) const;

    /**
     * @brief Returns presets whose name or preview contains the filter string.
     *
     * Case-insensitive substring match.  Empty filter returns all presets.
     *
     * @param filter  Substring to search for (case-insensitive).
     * @return Matching presets in insertion order.
     */
    std::vector<ConditionPreset> GetFilteredPresets(const std::string& filter) const;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true when a preset with the given UUID exists.
     */
    bool ValidatePresetID(const std::string& id) const;

    /**
     * @brief Returns all error messages accumulated since the last Clear().
     */
    std::vector<std::string> GetAllErrors() const;

    /**
     * @brief Clears all presets and resets error state.
     */
    void Clear();

    // -----------------------------------------------------------------------
    // Persistence
    // -----------------------------------------------------------------------

    /**
     * @brief Loads presets from a JSON file (clears existing data first).
     *
     * Expected format:
     * @code{.json}
     * {
     *   "version": 1,
     *   "presets": [ ... ]
     * }
     * @endcode
     *
     * @param filepath Path to the JSON file.
     * @return true on success, false on file/parse error.
     */
    bool Load(const std::string& filepath);

    /**
     * @brief Saves all presets to a JSON file.
     *
     * Creates the file if it does not exist; overwrites if it does.
     * Parent directories must already exist.
     *
     * @param filepath Destination path.
     * @return true on success, false on write error.
     */
    bool Save(const std::string& filepath) const;

private:

    /// @brief Generates a new UUID suitable for a preset ID.
    static std::string GenerateID();

    std::map<std::string, ConditionPreset> m_presets; ///< UUID → ConditionPreset
    std::vector<std::string>               m_order;   ///< UUIDs in insertion order
    mutable std::vector<std::string>       m_errors;  ///< Accumulated error messages
};

} // namespace Olympe
