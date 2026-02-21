/**
 * @file TaskGraphLoader.h
 * @brief Parses JSON task graphs and legacy BehaviorTree JSON into TaskGraphTemplate
 * @author Olympe Engine
 * @date 2026-02-21
 *
 * @details
 * TaskGraphLoader converts JSON files (schema v2 BehaviorTree or schema v3 TaskGraph)
 * into populated TaskGraphTemplate structures ready for use by the runtime.
 *
 * Supported schema versions:
 *   - schema_version == 2  : legacy BehaviorTree JSON (NodeGraphCore v2 format)
 *   - schema_version == 3  : native TaskGraph JSON format
 *   - schema_version absent: treated as version 2
 *
 * Node type mapping (schema v2):
 *   - "Selector"  -> TaskNodeType::Selector
 *   - "Sequence"  -> TaskNodeType::Sequence
 *   - "Parallel"  -> TaskNodeType::Parallel
 *   - "Repeater"  -> TaskNodeType::Decorator  (repeatCount stored in Parameters)
 *   - "Action"    -> TaskNodeType::AtomicTask  (AtomicTaskID = actionType)
 *   - "Condition" -> TaskNodeType::AtomicTask  (AtomicTaskID = conditionType)
 *   - unknown     -> TaskNodeType::AtomicTask  (AtomicTaskID = "unknown", warning logged)
 *
 * C++14 compliant - no std::variant, std::optional, or C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>

#include "TaskGraphTemplate.h"
#include "../json_helper.h"

namespace Olympe {

/**
 * @class TaskGraphLoader
 * @brief Static utility that loads TaskGraphTemplate from JSON files or JSON objects.
 *
 * @details
 * All methods are static; do not instantiate this class.
 * The caller owns the returned TaskGraphTemplate* and is responsible for deletion.
 */
class TaskGraphLoader {
public:

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * @brief Loads a TaskGraphTemplate from a JSON file on disk.
     *
     * @param path      Absolute or relative path to the JSON file.
     * @param outErrors Receives human-readable error messages if loading fails.
     * @return Pointer to a newly allocated TaskGraphTemplate on success,
     *         or nullptr if the file cannot be read, parsed, or validated.
     *
     * @note The caller is responsible for deleting the returned pointer.
     */
    static TaskGraphTemplate* LoadFromFile(const std::string& path,
                                           std::vector<std::string>& outErrors);

    /**
     * @brief Loads a TaskGraphTemplate from an already-parsed JSON object.
     *
     * @param data      Root JSON object (must contain schema_version and data fields).
     * @param outErrors Receives human-readable error messages if loading fails.
     * @return Pointer to a newly allocated TaskGraphTemplate on success,
     *         or nullptr if parsing or validation fails.
     *
     * @note The caller is responsible for deleting the returned pointer.
     */
    static TaskGraphTemplate* LoadFromJson(const json& data,
                                           std::vector<std::string>& outErrors);

    /**
     * @brief Validates a JSON object against the expected task graph schema.
     *
     * @param data      Root JSON object to validate.
     * @param outErrors Receives human-readable messages for each validation issue.
     * @return true if the JSON passes all structural checks; false otherwise.
     */
    static bool ValidateJson(const json& data,
                             std::vector<std::string>& outErrors);

private:

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Parses a schema v2 (BehaviorTree) JSON into a TaskGraphTemplate.
     */
    static TaskGraphTemplate* ParseSchemaV2(const json& data,
                                            std::vector<std::string>& outErrors);

    /**
     * @brief Parses a schema v3 (TaskGraph) JSON into a TaskGraphTemplate.
     */
    static TaskGraphTemplate* ParseSchemaV3(const json& data,
                                            std::vector<std::string>& outErrors);

    /**
     * @brief Parses a single node JSON object (schema v2) into a TaskNodeDefinition.
     * @param nodeJson  The JSON object for one node.
     * @param outErrors Error/warning collector.
     * @return A populated TaskNodeDefinition.
     */
    static TaskNodeDefinition ParseNodeV2(const json& nodeJson,
                                          std::vector<std::string>& outErrors);

    /**
     * @brief Parses a single node JSON object (schema v3) into a TaskNodeDefinition.
     * @param nodeJson  The JSON object for one node.
     * @param outErrors Error/warning collector.
     * @return A populated TaskNodeDefinition.
     */
    static TaskNodeDefinition ParseNodeV3(const json& nodeJson,
                                          std::vector<std::string>& outErrors);

    /**
     * @brief Parses a parameters JSON object and fills the Parameters map.
     *
     * For schema v2 flat parameters (primitive values): creates Literal bindings.
     * For structured bindings (object with "bindingType"): respects the bindingType field.
     *
     * @param paramsJson  The JSON object containing parameter key-value pairs.
     * @param outParams   The map to fill with parsed ParameterBinding entries.
     */
    static void ParseParameters(const json& paramsJson,
                                std::unordered_map<std::string, ParameterBinding>& outParams);

    /**
     * @brief Parses a primitive JSON value into a TaskValue.
     * @param val  A JSON value (bool, int, float, or string).
     * @return The corresponding TaskValue, or a default TaskValue if type is unsupported.
     */
    static TaskValue ParsePrimitiveValue(const json& val);

    // Prevent instantiation
    TaskGraphLoader() {}
};

} // namespace Olympe
