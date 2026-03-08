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
 *   - schema_version == 4  : ATS Visual Scripting JSON format (Phase 1)
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
     * @note The caller (or AssetManager) owns the returned pointer and is responsible
     *       for deleting it when no longer needed.
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

    // -----------------------------------------------------------------------
    // ATS VS schema v4 parsers (Phase 1)
    // -----------------------------------------------------------------------

    /**
     * @brief Parses a schema v4 (ATS Visual Scripting) JSON into a TaskGraphTemplate.
     */
    static TaskGraphTemplate* ParseSchemaV4(const json& data,
                                            std::vector<std::string>& outErrors);

    /**
     * @brief Parses a single node JSON object (schema v4) into a TaskNodeDefinition.
     * @param nodeJson   The JSON object for one node.
     * @param graphType  Graph type string (used to resolve "Sequence" vs "VSSequence").
     * @param outErrors  Error/warning collector.
     * @return A populated TaskNodeDefinition.
     */
    static TaskNodeDefinition ParseNodeV4(const json& nodeJson,
                                          const std::string& graphType,
                                          std::vector<std::string>& outErrors);

    /**
     * @brief Parses the exec_pins array of a node JSON (informational in Phase 1).
     */
    static void ParseExecPins(const json& nodeJson, TaskNodeDefinition& nodeDef);

    /**
     * @brief Parses the data_pins array of a node JSON and fills nodeDef.DataPins.
     */
    static void ParseDataPins(const json& nodeJson, TaskNodeDefinition& nodeDef);

    /**
     * @brief Parses the blackboard array in the data section and fills tmpl->Blackboard.
     */
    static void ParseBlackboard(const json& dataSection,
                                TaskGraphTemplate* tmpl,
                                std::vector<std::string>& outErrors);

    /**
     * @brief Parses exec_connections in the data section and fills tmpl->ExecConnections.
     */
    static void ParseExecConnections(const json& dataSection,
                                     TaskGraphTemplate* tmpl);

    /**
     * @brief Parses data_connections in the data section and fills tmpl->DataConnections.
     */
    static void ParseDataConnections(const json& dataSection,
                                     TaskGraphTemplate* tmpl);

    // -----------------------------------------------------------------------
    // Conversion helpers (string → enum)
    // -----------------------------------------------------------------------

    /**
     * @brief Converts a node type string to a TaskNodeType enum value.
     * @param s         String representation of the node type.
     * @param graphType Graph type string ("VisualScript" or "BehaviorTree").
     * @param outOk     Set to false if the string is unrecognised.
     * @return The corresponding TaskNodeType, or TaskNodeType::AtomicTask on failure.
     */
    static TaskNodeType  StringToNodeType(const std::string& s,
                                          const std::string& graphType,
                                          bool& outOk);

    /**
     * @brief Converts a variable type string to a VariableType enum value.
     * @param s  String representation ("Bool", "Int", "Float", etc.).
     * @return The corresponding VariableType, or VariableType::None if unknown.
     */
    static VariableType  StringToVariableType(const std::string& s);

    /**
     * @brief Converts a data pin direction string ("Input"/"Output") to DataPinDir.
     */
    static DataPinDir    StringToDataPinDir(const std::string& s);

    /**
     * @brief Converts an exec pin role string to ExecPinRole.
     */
    static ExecPinRole   StringToExecPinRole(const std::string& s);

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

    /**
     * @brief Safely retrieves a child JSON value by key and copies it to @p outVal.
     *
     * Encapsulates the contains() + operator[] pattern so call sites do not need
     * to use the raw JSON API directly.
     *
     * @param obj    JSON object to query.
     * @param key    Key to look up.
     * @param outVal Receives the value when the key is present.
     * @return true if the key exists; false otherwise.
     */
    static bool GetChildValue(const json& obj, const std::string& key, json& outVal);

    /**
     * @brief Resolves the root node ID with fallback: checks top-level first, then data section.
     *
     * Some schemas store rootNodeId at the document root; others nest it under "data".
     * This helper prefers the top-level location and falls back to data section.
     *
     * @param data        Root JSON document.
     * @param dataSection The "data" sub-object.
     * @return The resolved root node ID, or -1 if absent from both locations.
     */
    static int ResolveRootNodeId(const json& data, const json& dataSection);

    // Prevent instantiation
    TaskGraphLoader() {}
};

} // namespace Olympe
