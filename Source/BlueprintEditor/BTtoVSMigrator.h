/**
 * @file BTtoVSMigrator.h
 * @brief Converts legacy BT v2 JSON graphs to ATS VS v4 JSON format (Phase 6).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * BTtoVSMigrator translates a "BehaviorTree" schema_version 2 (or lower) JSON
 * document to the ATS VisualScript schema_version 4 format consumed by
 * TaskGraphLoader.
 *
 * Node type mapping:
 *   BT v2 type       →  VS v4 TaskNodeType
 *   ─────────────────────────────────────────
 *   "Selector"       →  TaskNodeType::Branch   (conditions as children)
 *   "Sequence"       →  TaskNodeType::VSSequence
 *   "Action"         →  TaskNodeType::AtomicTask
 *   "Condition"      →  TaskNodeType::Branch   (single condition leaf)
 *   "Decorator"      →  TaskNodeType::AtomicTask (inline, stripped)
 *   "Root" / "Start" →  TaskNodeType::EntryPoint
 *
 * Connection mapping:
 *   BT parent→child  →  ExecPinConnection { parent, "Out", child, "In" }
 *
 * Blackboard mapping:
 *   BT blackboard entries are copied verbatim to the VS graph's Blackboard.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../TaskSystem/TaskGraphTemplate.h"
#include "../third_party/nlohmann/json.hpp"

namespace Olympe {

// ============================================================================
// BTtoVSMigrator
// ============================================================================

/**
 * @class BTtoVSMigrator
 * @brief Converts a BT v2 JSON object to a TaskGraphTemplate (VS v4).
 *
 * All methods are static; the class is a pure utility with no instance state.
 */
class BTtoVSMigrator {
public:

    /**
     * @brief Converts a BT v2 JSON document to a TaskGraphTemplate.
     *
     * @param btV2Json  Parsed JSON from a BT v2 asset file.
     * @param outErrors Receives human-readable error messages on failure.
     * @return          Populated TaskGraphTemplate (may be partial on error).
     */
    static TaskGraphTemplate Convert(const nlohmann::json& btV2Json,
                                      std::vector<std::string>& outErrors);

    /**
     * @brief Returns true if @p j looks like a BT v2 document.
     *
     * @details
     * Heuristic: document must contain @c "blueprintType" == "BehaviorTree"
     * and either no @c schema_version or @c schema_version <= 2.
     */
    static bool IsBTv2(const nlohmann::json& j);

    /**
     * @brief Returns true if @p nodeName indicates a BT Decorator node.
     *
     * @details
     * Some BT v2 files encode Decorator nodes as type "Action" with a
     * descriptive name (e.g. "Repeater", "Inverter", "Cooldown", "Timeout").
     * This helper centralises that heuristic so both the migrator and the
     * loader can use it consistently.
     */
    static bool IsDecoratorName(const std::string& nodeName);

private:

    /// Converts one BT node JSON object to a TaskNodeDefinition.
    static TaskNodeDefinition ConvertNode(const nlohmann::json& btNode,
                                           std::vector<std::string>& outErrors);

    /**
     * @brief Builds exec connections from the BT "children" tree structure.
     *
     * @details
     * Walks the node array and for each node that has a "children" array,
     * emits an ExecPinConnection { nodeID, "Out", childID, "In" }.
     */
    static std::vector<ExecPinConnection> ConvertConnections(
        const nlohmann::json& nodesArray,
        std::vector<std::string>& outErrors);

    /**
     * @brief Copies blackboard entries from the BT JSON to the VS template.
     */
    static void ConvertBlackboard(const nlohmann::json& btV2Json,
                                   TaskGraphTemplate&    vsGraph,
                                   std::vector<std::string>& outErrors);

    /// Maps a BT node type string to the closest TaskNodeType equivalent.
    static TaskNodeType MapNodeType(const std::string& btType);
};

} // namespace Olympe
