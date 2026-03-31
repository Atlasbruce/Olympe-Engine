/**
 * @file SubgraphMigrator.h
 * @brief Phase 8 — Migrates legacy blueprint data to the flat-dictionary subgraph format.
 *
 * @details
 * **Legacy format** (any schema_version ≤ 4 that has data.nodes directly):
 * @code
 * { "data": { "nodes": [...], "links": [...] } }
 * @endcode
 *
 * **New flat-dictionary format** (schema_version 5):
 * @code
 * {
 *   "data": {
 *     "rootGraph":  { "nodes": [...], "links": [...] },
 *     "subgraphs":  {}
 *   }
 * }
 * @endcode
 *
 * SubgraphMigrator is stateless; all methods are const-correct or static.
 * C++14 compliant — no C++17/20 features.
 */

#pragma once

#include <string>
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe
{

/**
 * @class SubgraphMigrator
 * @brief Converts legacy blueprint JSON to the Phase 8 subgraph flat-dict format.
 *
 * Usage:
 * @code
 *   SubgraphMigrator m;
 *   if (m.NeedsMigration(blueprint))
 *       blueprint = m.Migrate(blueprint);
 * @endcode
 */
class SubgraphMigrator
{
public:
    SubgraphMigrator()  = default;
    ~SubgraphMigrator() = default;

    // -----------------------------------------------------------------------
    // Detection
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true when the blueprint uses the legacy format
     *        (data.nodes exists at the top level of the data object).
     *
     * Both BehaviorTree and HFSM variants are detected.
     */
    bool NeedsMigration(const nlohmann::json& blueprint) const;

    /**
     * @brief Returns true when the blueprint is already in the Phase 8 format
     *        (data.rootGraph exists).
     */
    bool IsNewFormat(const nlohmann::json& blueprint) const;

    // -----------------------------------------------------------------------
    // Migration
    // -----------------------------------------------------------------------

    /**
     * @brief Migrates a legacy blueprint to the flat-dictionary format.
     *
     * @param blueprint  Input JSON (may be legacy or already new format).
     * @return           Migrated JSON in new format.  If already new, a copy is returned.
     */
    nlohmann::json Migrate(const nlohmann::json& blueprint) const;

    // -----------------------------------------------------------------------
    // Subgraph helpers (used by the editor to build subgraph definitions)
    // -----------------------------------------------------------------------

    /**
     * @brief Creates an empty subgraph definition JSON object.
     *
     * @param name         Human-readable name shown in the tab.
     * @param uuid         Pre-generated UUID string for this subgraph.
     * @param blueprintType  "BehaviorTree" or "HFSM".
     * @return             JSON object suitable for insertion into data.subgraphs[uuid].
     */
    static nlohmann::json MakeEmptySubgraph(const std::string& name,
                                            const std::string& uuid,
                                            const std::string& blueprintType);

    /**
     * @brief Validates that every subgraphUUID referenced by SubGraph nodes
     *        actually exists in data.subgraphs and that there are no circular
     *        dependencies (A -> B -> A).
     *
     * @param blueprint  Full blueprint JSON in new format.
     * @param outError   Filled with a human-readable message when false is returned.
     * @return           true if the blueprint is self-consistent; false otherwise.
     */
    static bool ValidateSubgraphReferences(const nlohmann::json& blueprint,
                                           std::string& outError);

private:
    // Move data.nodes/data.links/data.rootNodeId into data.rootGraph.
    void MigrateDataSection(nlohmann::json& blueprint) const;

    // DFS-based cycle detection over subgraph UUID dependency graph.
    static bool HasCycle(const std::string& start,
                         const nlohmann::json& subgraphs,
                         std::vector<std::string>& visited,
                         std::vector<std::string>& inStack);

    // Collect all subgraphUUID values referenced within a graph object.
    static std::vector<std::string> CollectSubgraphRefs(const nlohmann::json& graphObj);
};

} // namespace Olympe
