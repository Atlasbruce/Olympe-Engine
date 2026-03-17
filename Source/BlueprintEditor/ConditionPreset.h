/**
 * @file ConditionPreset.h
 * @brief Core data models for the Condition Preset system (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * Defines the ConditionPreset, NodeConditionRef, and ConditionPresetRegistry
 * types that form the foundation of the global condition preset library.
 *
 * A ConditionPreset is a named, reusable condition expression (e.g.
 * "[mHealth] <= [2]") stored globally in
 * "./Blueprints/Presets/condition_presets.json".  Nodes reference presets by
 * ID via NodeConditionRef; multiple refs per node are joined with AND/OR.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "TaskSystem/TaskGraphTypes.h"

namespace Olympe {

// ============================================================================
// LogicalOp — logical operator between consecutive condition references
// ============================================================================

/**
 * @enum LogicalOp
 * @brief Logical operator that joins two condition expressions.
 */
enum class LogicalOp : uint8_t {
    And = 0, ///< Both conditions must be true
    Or  = 1  ///< At least one condition must be true
};

// ============================================================================
// ConditionPreset — a named, reusable condition expression
// ============================================================================

/**
 * @struct ConditionPreset
 * @brief A named, reusable condition stored in the global preset library.
 *
 * @details
 * Wraps an existing Condition with a unique ID and display name.
 * The preview string (e.g. "[mHealth] <= [2]") is computed on demand.
 */
struct ConditionPreset {
    std::string id;        ///< Unique identifier (UUID-style string)
    std::string name;      ///< Human-readable name (e.g. "Condition #1")
    Condition   condition; ///< The underlying condition expression

    /**
     * @brief Generates a short preview string for the condition.
     *
     * Examples:
     *   "[mHealth] <= [2]"
     *   "[Pin:Node#42.Out] != [Pin:Node#43.Out]"
     *   "[mSpeed] == [Pin:Node#44.Out]"
     *
     * @return Formatted preview string.
     */
    std::string GetPreview() const;
};

// ============================================================================
// NodeConditionRef — a reference from a node to a preset + logical operator
// ============================================================================

/**
 * @struct NodeConditionRef
 * @brief Links a node to a ConditionPreset with an associated logical operator.
 *
 * @details
 * The logicalOp is ignored for the first element in a list; subsequent
 * elements join with AND or OR.
 *
 * Example:
 *   { presetID="preset_001", logicalOp=And }
 *   { presetID="preset_003", logicalOp=Or  }
 *   → evaluates as: preset_001 AND preset_003 OR ...
 */
struct NodeConditionRef {
    std::string presetID;                   ///< ID of the referenced ConditionPreset
    LogicalOp   logicalOp = LogicalOp::And; ///< Operator linking to the previous condition
};

// ============================================================================
// ConditionPresetRegistry — CRUD + serialization
// ============================================================================

/**
 * @class ConditionPresetRegistry
 * @brief Manages the global pool of ConditionPresets with CRUD and I/O.
 *
 * @details
 * Presets are stored in insertion order and can be looked up by ID.
 * The registry loads/saves from "./Blueprints/Presets/condition_presets.json".
 *
 * Usage:
 * @code
 *   ConditionPresetRegistry registry;
 *   registry.LoadFromFile("./Blueprints/Presets/condition_presets.json");
 *
 *   ConditionPreset p;
 *   p.name = "Condition #1";
 *   p.condition.leftMode = "Variable";
 *   p.condition.leftVariable = "mHealth";
 *   p.condition.operatorStr = "<=";
 *   p.condition.rightMode = "Const";
 *   p.condition.rightConstValue = TaskValue(2);
 *   std::string id = registry.AddPreset(p);
 * @endcode
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */
class ConditionPresetRegistry {
public:

    ConditionPresetRegistry() = default;

    // Non-copyable to avoid accidental copies of the full map
    ConditionPresetRegistry(const ConditionPresetRegistry&) = delete;
    ConditionPresetRegistry& operator=(const ConditionPresetRegistry&) = delete;

    // -----------------------------------------------------------------------
    // CRUD operations
    // -----------------------------------------------------------------------

    /**
     * @brief Adds a new preset to the registry.
     *
     * If preset.id is empty, a unique ID is generated automatically.
     * If preset.name is empty, a default name ("Condition #N") is assigned.
     *
     * @param preset  Preset to add (id may be empty; name is optional).
     * @return The assigned preset ID.
     */
    std::string AddPreset(const ConditionPreset& preset);

    /**
     * @brief Removes a preset by ID.
     * @param id  Preset ID to remove.
     * @return true if the preset was found and removed, false otherwise.
     */
    bool RemovePreset(const std::string& id);

    /**
     * @brief Replaces the data for an existing preset.
     * @param id      ID of the preset to update.
     * @param preset  New data (the id field of preset is ignored; the id
     *                parameter governs which entry is updated).
     * @return true if the preset was found and updated.
     */
    bool UpdatePreset(const std::string& id, const ConditionPreset& preset);

    /**
     * @brief Creates a copy of an existing preset with a new unique ID.
     *
     * The copy receives the name "Copy of <original name>".
     *
     * @param id  ID of the preset to duplicate.
     * @return The new preset's ID, or an empty string if id was not found.
     */
    std::string DuplicatePreset(const std::string& id);

    // -----------------------------------------------------------------------
    // Lookup
    // -----------------------------------------------------------------------

    /**
     * @brief Returns a pointer to the preset with the given ID, or nullptr.
     * @param id  Preset ID.
     */
    const ConditionPreset* GetPreset(const std::string& id) const;

    /**
     * @brief Returns a mutable pointer to the preset with the given ID.
     * @param id  Preset ID.
     */
    ConditionPreset* GetPreset(const std::string& id);

    /**
     * @brief Returns true if a preset with the given ID exists.
     */
    bool HasPreset(const std::string& id) const;

    /**
     * @brief Returns all presets in insertion order.
     */
    std::vector<ConditionPreset> GetAllPresets() const;

    /**
     * @brief Returns the number of presets in the registry.
     */
    size_t GetCount() const;

    /**
     * @brief Removes all presets.
     */
    void Clear();

    // -----------------------------------------------------------------------
    // Search / filter
    // -----------------------------------------------------------------------

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
    // Serialization
    // -----------------------------------------------------------------------

    /**
     * @brief Loads presets from a JSON file.
     *
     * Clears existing data before loading.  On error, logs and returns false.
     *
     * @param filepath  Path to the JSON file.
     * @return true on success.
     */
    bool LoadFromFile(const std::string& filepath);

    /**
     * @brief Saves presets to a JSON file.
     *
     * @param filepath  Path to write.
     * @return true on success.
     */
    bool SaveToFile(const std::string& filepath) const;

    // -----------------------------------------------------------------------
    // Ordering
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the ordered list of preset IDs (insertion order).
     */
    const std::vector<std::string>& GetOrder() const { return m_order; }

private:

    /**
     * @brief Generates a unique, collision-free preset ID.
     */
    std::string GenerateID() const;

    /**
     * @brief Converts a string to lower-case for case-insensitive comparison.
     */
    static std::string ToLower(const std::string& s);

    std::map<std::string, ConditionPreset> m_presets; ///< ID → ConditionPreset
    std::vector<std::string>               m_order;   ///< Insertion-order list of IDs
    int                                    m_counter = 0; ///< For auto-naming
};

} // namespace Olympe
