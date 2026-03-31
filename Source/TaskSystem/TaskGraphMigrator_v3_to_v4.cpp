/**
 * @file TaskGraphMigrator_v3_to_v4.cpp
 * @brief Implementation of the v3->v4 task graph JSON migrator.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskGraphMigrator_v3_to_v4.h"
#include "../system/system_utils.h"

#include <fstream>
#include <sstream>

namespace Olympe {

// ============================================================================
// Migrate (public static)
// ============================================================================

bool TaskGraphMigrator_v3_to_v4::Migrate(const std::string& inputPath,
                                          const std::string& outputPath,
                                          std::vector<std::string>& outErrors)
{
    // --- Read input file ---
    std::ifstream inFile(inputPath);
    if (!inFile.is_open())
    {
        outErrors.push_back("TaskGraphMigrator: cannot open input file: " + inputPath);
        return false;
    }

    json v3data;
    try
    {
        inFile >> v3data;
    }
    catch (const std::exception& e)
    {
        outErrors.push_back(std::string("TaskGraphMigrator: JSON parse error: ") + e.what());
        return false;
    }
    inFile.close();

    // --- Migrate ---
    json v4data = MigrateJson(v3data, outErrors);
    if (v4data.empty())
    {
        return false;
    }

    // --- Write output file ---
    std::ofstream outFile(outputPath);
    if (!outFile.is_open())
    {
        outErrors.push_back("TaskGraphMigrator: cannot open output file: " + outputPath);
        return false;
    }

    try
    {
        outFile << v4data.dump(4);
    }
    catch (const std::exception& e)
    {
        outErrors.push_back(std::string("TaskGraphMigrator: JSON write error: ") + e.what());
        return false;
    }
    outFile.close();

    SYSTEM_LOG << "[TaskGraphMigrator] Migrated '" << inputPath
               << "' -> '" << outputPath << "' (schema v3 -> v4)\n";
    return true;
}

// ============================================================================
// MigrateJson (public static)
// ============================================================================

json TaskGraphMigrator_v3_to_v4::MigrateJson(const json& v3data,
                                               std::vector<std::string>& outErrors)
{
    // --- Validate schema_version ---
    if (!v3data.contains("schema_version") || v3data["schema_version"] != 3)
    {
        outErrors.push_back("TaskGraphMigrator: input is not schema_version 3");
        return json{};
    }

    json v4;

    // --- Copy metadata fields ---
    if (v3data.contains("name"))        v4["name"]        = v3data["name"];
    if (v3data.contains("description")) v4["description"] = v3data["description"];

    // Support both "blackboard" and "localBlackboard" naming conventions.
    if (v3data.contains("blackboard"))
    {
        v4["blackboard"] = v3data["blackboard"];
    }
    if (v3data.contains("localBlackboard"))
    {
        v4["localBlackboard"] = v3data["localBlackboard"];
    }

    // --- Set v4 fields ---
    v4["schema_version"] = 4;
    v4["graphType"]      = "VisualScript";

    // --- Migrate nodes ---
    json outNodes        = json::array();
    json execConnections = json::array();
    json dataConnections = json::array();

    if (v3data.contains("nodes") && v3data["nodes"].is_array())
    {
        for (const json& node : v3data["nodes"])
        {
            const int nodeID = node.value("nodeID", -1);

            // Build ExecConnections from NextOnSuccess / NextOnFailure.
            {
                const int nextSuccess = node.value("NextOnSuccess", -1);
                if (nextSuccess != -1)
                {
                    json conn;
                    conn["SourceNodeID"]  = nodeID;
                    conn["SourcePinName"] = "Out";
                    conn["TargetNodeID"]  = nextSuccess;
                    conn["TargetPinName"] = "In";
                    execConnections.push_back(conn);
                }
            }
            {
                const int nextFailure = node.value("NextOnFailure", -1);
                if (nextFailure != -1)
                {
                    json conn;
                    conn["SourceNodeID"]  = nodeID;
                    conn["SourcePinName"] = "OutFailure";
                    conn["TargetNodeID"]  = nextFailure;
                    conn["TargetPinName"] = "In";
                    execConnections.push_back(conn);
                }
            }

            // Build the v4 node object (copy relevant fields, drop v3-specific ones).
            json outNode;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                const std::string& key = it.key();
                if (key == "NextOnSuccess" || key == "NextOnFailure")
                {
                    // These fields are replaced by ExecConnections in v4.
                    continue;
                }
                outNode[key] = it.value();
            }
            outNodes.push_back(outNode);
        }
    }

    v4["nodes"]           = outNodes;
    v4["ExecConnections"] = execConnections;
    v4["DataConnections"] = dataConnections;

    SYSTEM_LOG << "[TaskGraphMigrator] MigrateJson: " << outNodes.size()
               << " nodes, " << execConnections.size() << " ExecConnections\n";

    return v4;
}

} // namespace Olympe
