/**
 * @file AtomicTaskUIRegistry.h
 * @brief UI-side registry of available atomic tasks with display metadata.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * AtomicTaskUIRegistry provides editor-facing information for atomic tasks
 * (display name, category, description) used to populate dropdown menus
 * in the properties panel.
 *
 * Built-in tasks are pre-registered via InitializeBuiltInTasks().
 * The runtime AtomicTaskRegistry is the authoritative source for which
 * tasks are actually executable; this registry adds UI metadata on top.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Olympe {

/**
 * @struct TaskSpec
 * @brief Display metadata for a single atomic task type.
 */
struct TaskSpec {
    std::string id;           ///< Task ID (matches AtomicTaskRegistry key)
    std::string displayName;  ///< Human-readable name (e.g. "Move To Goal")
    std::string category;     ///< Category for grouping (e.g. "Movement")
    std::string description;  ///< Short description for tooltip
};

/**
 * @class AtomicTaskUIRegistry
 * @brief Singleton registry mapping task IDs to TaskSpec metadata.
 */
class AtomicTaskUIRegistry {
public:

    /**
     * @brief Returns the singleton instance.
     *        Calls InitializeBuiltInTasks() on first access.
     */
    static AtomicTaskUIRegistry& Get();

    /**
     * @brief Registers a TaskSpec for the given task ID.
     * @param spec Spec to register (id must be non-empty).
     */
    void Register(const TaskSpec& spec);

    /**
     * @brief Returns the TaskSpec for the given id, or nullptr if not found.
     * @param id Task ID string.
     */
    const TaskSpec* GetTaskSpec(const std::string& id) const;

    /**
     * @brief Returns all registered task IDs (unordered).
     */
    std::vector<std::string> GetAllTaskIds() const;

    /**
     * @brief Returns all tasks belonging to the given category.
     * @param category Category string (e.g. "Movement").
     */
    std::vector<TaskSpec> GetTasksByCategory(const std::string& category) const;

    /**
     * @brief Returns all unique category names.
     */
    std::vector<std::string> GetAllCategories() const;

    /**
     * @brief Returns all tasks sorted by category then displayName,
     *        suitable for building a dropdown or combo box.
     */
    std::vector<TaskSpec> GetSortedForUI() const;

private:

    AtomicTaskUIRegistry();

    void InitializeBuiltInTasks();

    std::unordered_map<std::string, TaskSpec> m_specs;
};

} // namespace Olympe
