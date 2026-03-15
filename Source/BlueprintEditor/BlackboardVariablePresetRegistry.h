/**
 * @file BlackboardVariablePresetRegistry.h
 * @brief Singleton registry for blackboard variable presets loaded from JSON config.
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * BlackboardVariablePresetRegistry loads the pool of available blackboard
 * variable presets from `Assets/Config/BlackboardVariablePresets.json`.
 * It exposes O(1) lookup by name, category filtering, and metadata access.
 *
 * Use the singleton via Instance() at editor startup.  Call LoadFromFile()
 * once to populate.  The registry is read-only after loading.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace Olympe {

/**
 * @struct BlackboardVariablePreset
 * @brief Metadata for a single blackboard variable preset entry.
 */
struct BlackboardVariablePreset {
    std::string name;        ///< Variable key (e.g. "targetDistance")
    std::string type;        ///< Type string as declared in JSON (e.g. "Float")
    std::string description; ///< Human-readable description
    std::string category;    ///< Category label (e.g. "Targeting", "Movement")
    std::string defaultValue;///< Default value as string (e.g. "9999.0", "false")
};

/**
 * @class BlackboardVariablePresetRegistry
 * @brief Singleton registry for global blackboard variable presets.
 *
 * @details
 * Loaded from `Assets/Config/BlackboardVariablePresets.json` at editor startup.
 * Provides O(1) lookup by name via internal index map.
 * Category queries are O(n) on the preset list but cached on first access.
 *
 * Thread-safety: read-only after LoadFromFile() — no locking required.
 */
class BlackboardVariablePresetRegistry {
public:

    /**
     * @brief Returns the singleton instance.
     * @return Reference to the singleton.
     */
    static BlackboardVariablePresetRegistry& Instance();

    // Non-copyable, non-movable (singleton)
    BlackboardVariablePresetRegistry(const BlackboardVariablePresetRegistry&) = delete;
    BlackboardVariablePresetRegistry& operator=(const BlackboardVariablePresetRegistry&) = delete;

    /**
     * @brief Loads presets from a JSON file.
     *
     * Clears any previously loaded data before loading.
     * On parse error, logs and returns false; registry remains empty.
     *
     * @param filePath Absolute or relative path to the JSON preset file.
     * @return true on success, false on file/parse error.
     */
    bool LoadFromFile(const std::string& filePath);

    /**
     * @brief Returns all loaded presets, sorted by name.
     *
     * Thread-safe: protected by an internal mutex so that concurrent calls
     * from a FileWatcher thread and the Save path cannot race.
     * @return Const reference to the internal preset list.
     */
    const std::vector<BlackboardVariablePreset>& GetAllPresets() const;

    /**
     * @brief Returns a pointer to the preset with the given name, or nullptr.
     * @param name Variable name to look up (case-sensitive).
     * @return Pointer to preset if found, nullptr otherwise.
     */
    const BlackboardVariablePreset* GetPreset(const std::string& name) const;

    /**
     * @brief Returns all presets belonging to a given category.
     * @param category Category label to filter by (case-sensitive).
     * @return Vector of matching presets (may be empty).
     */
    std::vector<BlackboardVariablePreset> GetByCategory(const std::string& category) const;

    /**
     * @brief Returns all unique category names present in the registry.
     * @return Sorted vector of category strings.
     */
    std::vector<std::string> GetAllCategories() const;

    /**
     * @brief Returns true if a preset with the given name exists.
     * @param name Variable name to test.
     */
    bool HasPreset(const std::string& name) const;

    /**
     * @brief Returns the number of loaded presets.
     */
    size_t GetCount() const;

    /**
     * @brief Clears all loaded presets and resets the registry.
     */
    void Clear();

private:

    BlackboardVariablePresetRegistry() = default;

    mutable std::mutex                                       m_mutex;   ///< Guards m_presets during concurrent access
    std::vector<BlackboardVariablePreset>                    m_presets;
    std::unordered_map<std::string, size_t>                  m_nameIndex;  ///< name → index in m_presets
};

} // namespace Olympe
