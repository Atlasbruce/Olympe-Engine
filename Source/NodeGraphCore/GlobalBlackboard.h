/**
 * @file GlobalBlackboard.h
 * @brief Singleton GlobalBlackboard for shared TaskValue variables across VS graphs.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * GlobalBlackboard provides a process-wide key/value store using TaskValue.
 * It is accessible from any VS graph node via the "global:" scope prefix in
 * LocalBlackboard::GetValueScoped / SetValueScoped.
 *
 * The singleton is implemented with the Meyers pattern (C++14 compliant).
 * The header-only implementation avoids ODR issues when included in multiple
 * translation units; the singleton local static is guaranteed to be initialized
 * exactly once by the C++11/14 standard.
 *
 * JSON persistence is provided as a separate opt-in (SaveToJson / LoadFromJson)
 * and is not required for basic runtime operation.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <unordered_map>

#include "../TaskSystem/TaskGraphTypes.h"
#include "../system/system_utils.h"

namespace Olympe {

/**
 * @class GlobalBlackboard
 * @brief Process-wide singleton blackboard storing TaskValue entries.
 *
 * @details
 * All VS graph nodes sharing the same process can read and write
 * GlobalBlackboard variables using the "global:" scope prefix.
 *
 * Example:
 * @code
 *   // Write:
 *   GlobalBlackboard::Get().SetVar("GamePhase", TaskValue(std::string("Combat")));
 *
 *   // Read:
 *   TaskValue v = GlobalBlackboard::Get().GetVar("GamePhase");
 * @endcode
 */
class GlobalBlackboard {
public:

    /**
     * @brief Returns the singleton instance.
     * @return Reference to the single GlobalBlackboard instance.
     */
    static GlobalBlackboard& Get()
    {
        static GlobalBlackboard instance;
        return instance;
    }

    /**
     * @brief Reads a variable by key.
     *
     * @param key  Variable name (without "global:" prefix).
     * @return Stored TaskValue, or default TaskValue() (None) if not found.
     */
    TaskValue GetVar(const std::string& key) const
    {
        auto it = m_store.find(key);
        if (it == m_store.end())
        {
            return TaskValue();
        }
        return it->second;
    }

    /**
     * @brief Writes or creates a variable.
     *
     * @param key    Variable name (without "global:" prefix).
     * @param value  Value to store.
     */
    void SetVar(const std::string& key, const TaskValue& value)
    {
        m_store[key] = value;
        m_dirty = true;
    }

    /**
     * @brief Returns true if any variable has been modified since the last ClearDirty().
     */
    bool IsDirty() const  { return m_dirty; }

    /**
     * @brief Marks the blackboard as modified (called automatically by SetVar).
     */
    void MarkDirty()  { m_dirty = true; }

    /**
     * @brief Clears the dirty flag (call after persisting).
     */
    void ClearDirty() { m_dirty = false; }

    /**
     * @brief Checks whether a variable exists.
     * @param key  Variable name (without "global:" prefix).
     * @return true if the variable exists.
     */
    bool HasVar(const std::string& key) const
    {
        return m_store.find(key) != m_store.end();
    }

    /**
     * @brief Removes all variables (useful in tests / level transitions).
     */
    void Clear()
    {
        m_store.clear();
        m_dirty = false;
    }

private:
    GlobalBlackboard()  = default;
    ~GlobalBlackboard() = default;

    // Non-copyable, non-movable.
    GlobalBlackboard(const GlobalBlackboard&)            = delete;
    GlobalBlackboard& operator=(const GlobalBlackboard&) = delete;
    GlobalBlackboard(GlobalBlackboard&&)                 = delete;
    GlobalBlackboard& operator=(GlobalBlackboard&&)      = delete;

    std::unordered_map<std::string, TaskValue> m_store;
    bool m_dirty = false;
};

} // namespace Olympe
