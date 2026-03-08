/**
 * @file TaskGraphMigrator_v3_to_v4.h
 * @brief Migrates task graph JSON from schema v3 (BT-style) to schema v4 (VS-style).
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Schema v3 uses a flat node list with `NextOnSuccess` / `NextOnFailure` integer
 * fields to express control flow (Behavior-Tree-style).
 *
 * Schema v4 replaces those fields with explicit `ExecConnections` and
 * `DataConnections` arrays (Visual-Script-style), adds `graphType`, and
 * bumps `schema_version` to 4.
 *
 * @par Usage
 * @code
 *   std::vector<std::string> errors;
 *   bool ok = TaskGraphMigrator_v3_to_v4::Migrate("input_v3.json",
 *                                                  "output_v4.json",
 *                                                  errors);
 *   if (!ok) {
 *       for (auto& e : errors) std::cerr << e << "\n";
 *   }
 * @endcode
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>

#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

namespace Olympe {

/**
 * @class TaskGraphMigrator_v3_to_v4
 * @brief Converts task graph JSON from schema v3 to schema v4 (VisualScript).
 *
 * @details
 * Migration rules applied by MigrateJson():
 *  1. Checks `schema_version == 3`; adds error and returns `{}` if not.
 *  2. Copies `name`, `description`, `blackboard` / `localBlackboard`.
 *  3. For each node in `nodes`:
 *     - If `NextOnSuccess != -1`: adds an ExecConnection
 *       `{ NodeID, "Out", NextOnSuccess, "In" }`.
 *     - If `NextOnFailure != -1`: adds an ExecConnection
 *       `{ NodeID, "OutFailure", NextOnFailure, "In" }`.
 *     - Copies node fields (`nodeType`, `nodeID`, `nodeName`, `parameters`).
 *     - Removes `NextOnSuccess` / `NextOnFailure` from the output node object.
 *  4. Inserts `"graphType": "VisualScript"` and `"schema_version": 4`.
 *  5. Inserts an empty `"DataConnections": []` array.
 */
class TaskGraphMigrator_v3_to_v4 {
public:

    /**
     * @brief Migrates a v3 JSON file to v4 and writes the result to disk.
     *
     * @param inputPath   Path to the v3 JSON file to read.
     * @param outputPath  Path where the v4 JSON file will be written.
     * @param outErrors   Receives human-readable error descriptions on failure.
     * @return true if the migration succeeded; false otherwise.
     */
    static bool Migrate(const std::string& inputPath,
                        const std::string& outputPath,
                        std::vector<std::string>& outErrors);

    /**
     * @brief Performs the v3→v4 JSON transformation in memory.
     *
     * @param v3data    Parsed JSON object representing the v3 graph.
     * @param outErrors Receives human-readable error descriptions on failure.
     * @return Transformed v4 JSON object, or `{}` on error.
     */
    static json MigrateJson(const json& v3data,
                            std::vector<std::string>& outErrors);
};

} // namespace Olympe
