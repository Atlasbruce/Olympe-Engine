/**
 * @file GraphMigrator.h
 * @brief Migration system for JSON versions
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Detects and migrates JSON files from legacy and v1 formats to v2 unified schema.
 */

#pragma once

#include "GraphDocument.h"
#include "../json_helper.h"

namespace Olympe {
namespace NodeGraph {

/**
 * @class GraphMigrator
 * @brief Handles migration from old JSON formats to v2
 */
class GraphMigrator {
public:
    /**
     * @brief Load graph with automatic migration
     * @param j JSON object
     * @return GraphDocument with migrated data
     */
    static GraphDocument LoadWithMigration(const json& j);
    
private:
    /**
     * @brief Detect schema version
     * @param j JSON object
     * @return Version number (0 = legacy BT, 1 = v1 blueprint, 2 = v2)
     */
    static int DetectSchemaVersion(const json& j);
    
    /**
     * @brief Migrate legacy BT format to v2
     * @param v1 Legacy JSON
     * @return v2 JSON
     */
    static json MigrateLegacyBTToV2(const json& v1);
    
    /**
     * @brief Migrate v1 Blueprint format to v2
     * @param v1 v1 JSON
     * @return v2 JSON
     */
    static json MigrateV1BlueprintToV2(const json& v1);
};

} // namespace NodeGraph
} // namespace Olympe
