/**
 * @file BlackboardSystem.h
 * @brief Blackboard system for shared graph variables (Phase 2.1)
 * @author Olympe Engine
 * @date 2026-02-19
 *
 * @details
 * Provides a serializable, type-aware container of named variables that can
 * be shared across AI graph nodes. Supports Int, Float, Bool, String, Vector3.
 */

#pragma once

#include "../json_helper.h"
#include <string>
#include <map>

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Blackboard types
// ============================================================================

/**
 * @enum BlackboardType
 * @brief Supported variable types in the blackboard
 */
enum class BlackboardType : uint8_t {
    Int = 0,
    Float,
    Bool,
    String,
    Vector3
};

/**
 * @struct BlackboardValue
 * @brief Stores a typed value for a blackboard entry.
 *
 * The active field is determined by the `type` member.
 */
struct BlackboardValue {
    BlackboardType type = BlackboardType::Int;
    int   intValue    = 0;
    float floatValue  = 0.0f;
    bool  boolValue   = false;
    std::string stringValue;
    float vec3X = 0.0f;
    float vec3Y = 0.0f;
    float vec3Z = 0.0f;
};

// ============================================================================
// BlackboardSystem
// ============================================================================

/**
 * @class BlackboardSystem
 * @brief Manages named blackboard variables for a graph
 */
class BlackboardSystem {
public:
    BlackboardSystem() = default;
    ~BlackboardSystem() = default;

    /**
     * @brief Check if an entry exists
     * @param name Variable name
     */
    bool HasEntry(const std::string& name) const;

    /**
     * @brief Get a const pointer to an entry
     * @param name Variable name
     * @return Const pointer or nullptr if not found
     */
    const BlackboardValue* GetEntry(const std::string& name) const;

    /**
     * @brief Create a new entry
     * @param name Variable name (must be unique)
     * @param type Variable type
     * @param initialValue Initial value
     * @return true if created, false if name already exists
     */
    bool CreateEntry(const std::string& name, BlackboardType type,
                     const BlackboardValue& initialValue);

    /**
     * @brief Remove an entry
     * @param name Variable name
     * @return true if removed, false if not found
     */
    bool RemoveEntry(const std::string& name);

    /**
     * @brief Rename an entry
     * @param oldName Current variable name
     * @param newName New variable name
     * @return true if renamed, false on error (not found, or newName already used)
     */
    bool RenameEntry(const std::string& oldName, const std::string& newName);

    /**
     * @brief Update the value of an existing entry
     * @param name Variable name
     * @param value New value (type must match the entry's type)
     * @return true if updated, false if not found or type mismatch
     */
    bool SetValue(const std::string& name, const BlackboardValue& value);

    /**
     * @brief Get all entries (for UI / serialization)
     * @return Const reference to internal map
     */
    const std::map<std::string, BlackboardValue>& GetAll() const;

    /**
     * @brief Serialize blackboard to JSON
     * @return JSON array of variable objects
     */
    json ToJson() const;

    /**
     * @brief Deserialize blackboard from JSON
     * @param j JSON array produced by ToJson()
     */
    void FromJson(const json& j);

private:
    std::map<std::string, BlackboardValue> m_entries;

    /**
     * @brief Convert a BlackboardType to its string representation
     */
    static std::string TypeToString(BlackboardType t);

    /**
     * @brief Convert a string to BlackboardType (returns Int on unknown)
     */
    static BlackboardType StringToType(const std::string& s);
};

} // namespace NodeGraph
} // namespace Olympe
