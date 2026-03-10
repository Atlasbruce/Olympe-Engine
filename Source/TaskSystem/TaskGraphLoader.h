/**
 * @file TaskGraphLoader.h
 * @brief Clean schema v4 parser for ATS Visual Script task graphs.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * TaskGraphLoader v4 is a ground-up rewrite of the legacy loader.
 * It natively parses schema v4 ATS VisualScript JSON (flat structure).
 * For schema v3, it delegates to TaskGraphMigrator_v3_to_v4 before parsing.
 * For schema v2 and below, it retains minimal backward-compatible parsing
 * so that existing BehaviorTree assets continue to load.
 *
 * Schema v4 (flat, ATS VisualScript):
 * @code
 * {
 *   "schema_version": 4,
 *   "id": "uuid-string",
 *   "name": "string",
 *   "graphType": "VisualScript",
 *   "blackboard": [ { "key": "k", "type": "Int|Float|Bool|String|Vector", "value": ... } ],
 *   "nodes": [
 *     { "id": 0, "type": "EntryPoint|AtomicTask|...", "label": "...",
 *       "position": { "x": 0, "y": 0 },
 *       "params": { "key": "value" },
 *       "taskType": "Task_Wait",
 *       "delaySeconds": 0.0,
 *       "bbKey": "scope:key",
 *       "subGraphPath": "path",
 *       "conditionKey": "scope:key",
 *       "conditionOp": "==/!=/</>/<=/>=" ,
 *       "conditionValue": any,
 *       "mathOp": "+|-|*|/",
 *       "mathInputA": "scope:key",
 *       "mathInputB": "scope:key",
 *       "mathOutput": "scope:key",
 *       "switchKey": "scope:key" }
 *   ],
 *   "execConnections": [ { "fromNode": 0, "fromPin": "Completed", "toNode": 1 } ],
 *   "dataConnections": [ { "fromNode": 0, "fromPin": "value", "toNode": 1, "toPin": "input" } ]
 * }
 * @endcode
 *
 * C++14 compliant - no std::filesystem, no C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>

#include "TaskGraphTemplate.h"
#include "../json_helper.h"

namespace Olympe {

/**
 * @class TaskGraphLoader
 * @brief Static utility that loads TaskGraphTemplate from schema v4 JSON.
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
     * @param path      Path to the JSON file (absolute or relative to working dir).
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
     * Dispatch logic:
     *   - schema_version == 4: ParseSchemaV4 (flat ATS VS format).
     *   - schema_version == 3: run TaskGraphMigrator_v3_to_v4::MigrateJson(), then ParseSchemaV4.
     *   - schema_version <= 2 or absent: ParseSchemaV4 with nested data.nodes support.
     *
     * @param data      Root JSON object.
     * @param outErrors Receives human-readable error messages if loading fails.
     * @return Pointer to a newly allocated TaskGraphTemplate on success,
     *         or nullptr if parsing or validation fails.
     */
    static TaskGraphTemplate* LoadFromJson(const json& data,
                                           std::vector<std::string>& outErrors);

    /**
     * @brief Validates a JSON object against the expected task graph schema.
     *
     * For v4 flat format: checks for top-level "nodes" array.
     * For v2/v3 nested format: checks for "data.nodes" array.
     *
     * @param data      Root JSON object to validate.
     * @param outErrors Receives human-readable messages for each validation issue.
     * @return true if the JSON passes structural checks; false otherwise.
     */
    static bool ValidateJson(const json& data,
                             std::vector<std::string>& outErrors);

    /**
     * @brief Returns true if the given file path exists and can be opened.
     *
     * C++14 compatible implementation using std::ifstream.
     *
     * @param path File path to check.
     * @return true if the file can be opened for reading.
     */
    static bool FileExists(const std::string& path);

    /**
     * @brief Recursively scans a directory for .ats task graph files.
     *
     * C++14 compatible — uses POSIX dirent.h on non-Windows, and Win32 API
     * on Windows. Logs a warning if the directory does not exist.
     *
     * @param dir  Directory to scan (absolute or relative to working dir).
     * @return Sorted list of full paths to every .ats file found.
     */
    static std::vector<std::string> ScanTaskGraphDirectory(const std::string& dir);

private:

    // -----------------------------------------------------------------------
    // v4 (flat ATS Visual Scripting format) – primary path
    // -----------------------------------------------------------------------

    /**
     * @brief Parses a schema v4 flat JSON into a TaskGraphTemplate.
     *
     * Supports both the canonical flat format ("nodes", "execConnections",
     * "dataConnections" at root) and the legacy nested format
     * ("data.nodes", "data.exec_connections") for backward compatibility
     * with existing JSON assets.
     */
    static TaskGraphTemplate* ParseSchemaV4(const json& data,
                                            std::vector<std::string>& outErrors);

    /**
     * @brief Parses a single node JSON (v4 flat format) into a TaskNodeDefinition.
     *
     * Accepts both the new field names ("id", "type", "params", "label") and
     * the legacy field names ("nodeID", "nodeType", "parameters", "nodeName").
     */
    static TaskNodeDefinition ParseNodeV4(const json& nodeJson,
                                          const std::string& graphType,
                                          std::vector<std::string>& outErrors);

    /**
     * @brief Parses the blackboard array (v4) and fills tmpl->Blackboard.
     *
     * Supports both "value" (new) and "default" (legacy) for the initial value.
     * Supports both "key" (new) and "Key" (legacy PascalCase) field names.
     */
    static void ParseBlackboardV4(const json& root,
                                  TaskGraphTemplate* tmpl,
                                  std::vector<std::string>& outErrors);

    /**
     * @brief Parses execConnections (v4) and fills tmpl->ExecConnections.
     *
     * Accepts "execConnections" (new camelCase), "ExecConnections" (PascalCase),
     * and "data.exec_connections" (legacy nested).
     */
    static void ParseExecConnectionsV4(const json& root,
                                       TaskGraphTemplate* tmpl);

    /**
     * @brief Parses dataConnections (v4) and fills tmpl->DataConnections.
     *
     * Accepts "dataConnections" (new camelCase), "DataConnections" (PascalCase),
     * and "data.data_connections" (legacy nested).
     */
    static void ParseDataConnectionsV4(const json& root,
                                       TaskGraphTemplate* tmpl);

    // -----------------------------------------------------------------------
    // Shared helpers
    // -----------------------------------------------------------------------

    static void ParseParameters(const json& paramsJson,
                                std::unordered_map<std::string, ParameterBinding>& outParams);

    static TaskValue     ParsePrimitiveValue(const json& val);
    static bool          GetChildValue(const json& obj, const std::string& key, json& outVal);
    static int           ResolveRootNodeId(const json& data, const json& dataSection);

    static TaskNodeType  StringToNodeType(const std::string& s,
                                          const std::string& graphType,
                                          bool& outOk);
    static VariableType  StringToVariableType(const std::string& s);
    static DataPinDir    StringToDataPinDir(const std::string& s);
    static ExecPinRole   StringToExecPinRole(const std::string& s);

    // Prevent instantiation
    TaskGraphLoader() {}
};

} // namespace Olympe
