/**
 * @file SubgraphMigrator.cpp
 * @brief Phase 8 — SubgraphMigrator implementation.
 *
 * See SubgraphMigrator.h for the full API contract.
 */

#include "SubgraphMigrator.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace Olympe
{

// ============================================================================
// Detection helpers
// ============================================================================

bool SubgraphMigrator::NeedsMigration(const json& blueprint) const
{
    // Must have a "data" section to be a recognised blueprint.
    if (!blueprint.is_object() || !blueprint.contains("data"))
        return false;

    const json& data = blueprint["data"];
    if (!data.is_object())
        return false;

    // Legacy BehaviorTree: data.nodes exists directly.
    if (data.contains("nodes") && data["nodes"].is_array())
        return true;

    // Legacy HFSM: data.states exists directly.
    if (data.contains("states") && data["states"].is_array())
        return true;

    return false;
}

bool SubgraphMigrator::IsNewFormat(const json& blueprint) const
{
    if (!blueprint.is_object() || !blueprint.contains("data"))
        return false;

    const json& data = blueprint["data"];
    return data.is_object() && data.contains("rootGraph");
}

// ============================================================================
// Migration
// ============================================================================

nlohmann::json SubgraphMigrator::Migrate(const json& blueprint) const
{
    if (IsNewFormat(blueprint))
    {
        std::cout << "[SubgraphMigrator] Blueprint is already in new format — skipping migration.\n";
        return blueprint;
    }

    if (!NeedsMigration(blueprint))
    {
        std::cout << "[SubgraphMigrator] Blueprint does not require migration.\n";
        return blueprint;
    }

    std::cout << "[SubgraphMigrator] Migrating blueprint to flat-dictionary subgraph format...\n";

    json result = blueprint;
    MigrateDataSection(result);

    // Bump schema_version to 5 to signal the new format.
    result["schema_version"] = 5;

    std::cout << "[SubgraphMigrator] Migration complete.\n";
    return result;
}

void SubgraphMigrator::MigrateDataSection(json& blueprint) const
{
    const json& data = blueprint["data"];

    // Build the rootGraph object from whatever is in data right now.
    json rootGraph = json::object();

    // --- BehaviorTree fields ---
    if (data.contains("nodes"))
        rootGraph["nodes"] = data["nodes"];
    if (data.contains("links"))
        rootGraph["links"] = data["links"];
    if (data.contains("rootNodeId"))
        rootGraph["rootNodeId"] = data["rootNodeId"];

    // --- HFSM fields ---
    if (data.contains("states"))
        rootGraph["states"] = data["states"];
    if (data.contains("transitions"))
        rootGraph["transitions"] = data["transitions"];
    if (data.contains("initialState"))
        rootGraph["initialState"] = data["initialState"];

    // Build a new clean data section containing only rootGraph and subgraphs.
    json newData   = json::object();
    newData["rootGraph"]  = rootGraph;
    newData["subgraphs"]  = json::object();

    blueprint["data"] = newData;
}

// ============================================================================
// Subgraph helpers
// ============================================================================

json SubgraphMigrator::MakeEmptySubgraph(const std::string& name,
                                         const std::string& uuid,
                                         const std::string& blueprintType)
{
    json sg;
    sg["uuid"]          = uuid;
    sg["name"]          = name;
    sg["blueprintType"] = blueprintType;
    sg["inputPins"]     = json::array();
    sg["outputPins"]    = json::array();

    if (blueprintType == "HFSM")
    {
        sg["states"]      = json::array();
        sg["transitions"] = json::array();
        sg["initialState"] = "";
    }
    else
    {
        // Default: BehaviorTree layout
        sg["nodes"] = json::array();
        sg["links"] = json::array();
    }

    return sg;
}

// ============================================================================
// Validation
// ============================================================================

bool SubgraphMigrator::ValidateSubgraphReferences(const json& blueprint,
                                                   std::string& outError)
{
    if (!blueprint.is_object() || !blueprint.contains("data"))
    {
        outError = "Missing 'data' section.";
        return false;
    }

    const json& data = blueprint["data"];
    if (!data.contains("rootGraph") || !data.contains("subgraphs"))
    {
        outError = "Blueprint is not in new subgraph format (missing rootGraph or subgraphs).";
        return false;
    }

    const json& subgraphs = data["subgraphs"];
    if (!subgraphs.is_object())
    {
        outError = "data.subgraphs must be a JSON object.";
        return false;
    }

    // 1. Verify every referenced UUID exists.
    //    Collect refs from rootGraph first.
    std::vector<std::string> rootRefs = CollectSubgraphRefs(data["rootGraph"]);
    for (const auto& ref : rootRefs)
    {
        if (!subgraphs.contains(ref))
        {
            outError = "rootGraph references unknown subgraph UUID: " + ref;
            return false;
        }
    }

    // Collect refs from each subgraph and verify.
    for (auto it = subgraphs.begin(); it != subgraphs.end(); ++it)
    {
        std::vector<std::string> refs = CollectSubgraphRefs(it.value());
        for (const auto& ref : refs)
        {
            if (!subgraphs.contains(ref))
            {
                outError = "Subgraph '" + it.key() +
                           "' references unknown subgraph UUID: " + ref;
                return false;
            }
        }
    }

    // 2. Detect circular dependencies via DFS.
    for (auto it = subgraphs.begin(); it != subgraphs.end(); ++it)
    {
        std::vector<std::string> visited;
        std::vector<std::string> inStack;
        if (HasCycle(it.key(), subgraphs, visited, inStack))
        {
            outError = "Circular subgraph dependency detected starting from: " + it.key();
            return false;
        }
    }

    return true;
}

// ============================================================================
// Private helpers
// ============================================================================

bool SubgraphMigrator::HasCycle(const std::string& start,
                                 const json& subgraphs,
                                 std::vector<std::string>& visited,
                                 std::vector<std::string>& inStack)
{
    // Already fully explored — no cycle through this node.
    if (std::find(visited.begin(), visited.end(), start) != visited.end())
        return false;

    // In the current DFS stack — cycle found.
    if (std::find(inStack.begin(), inStack.end(), start) != inStack.end())
        return true;

    inStack.push_back(start);

    if (subgraphs.contains(start))
    {
        std::vector<std::string> refs = CollectSubgraphRefs(subgraphs[start]);
        for (const auto& ref : refs)
        {
            if (HasCycle(ref, subgraphs, visited, inStack))
                return true;
        }
    }

    inStack.erase(std::find(inStack.begin(), inStack.end(), start));
    visited.push_back(start);
    return false;
}

std::vector<std::string> SubgraphMigrator::CollectSubgraphRefs(const json& graphObj)
{
    std::vector<std::string> refs;
    if (!graphObj.is_object())
        return refs;

    // Look in "nodes" array (BehaviorTree format).
    if (graphObj.contains("nodes") && graphObj["nodes"].is_array())
    {
        for (const auto& node : graphObj["nodes"])
        {
            if (node.is_object() && node.contains("subgraphUUID"))
            {
                const auto& uuid = node["subgraphUUID"];
                if (uuid.is_string() && !uuid.get<std::string>().empty())
                    refs.push_back(uuid.get<std::string>());
            }
        }
    }

    // Look in "states" array (HFSM format).
    if (graphObj.contains("states") && graphObj["states"].is_array())
    {
        for (const auto& state : graphObj["states"])
        {
            if (state.is_object() && state.contains("subgraphUUID"))
            {
                const auto& uuid = state["subgraphUUID"];
                if (uuid.is_string() && !uuid.get<std::string>().empty())
                    refs.push_back(uuid.get<std::string>());
            }
        }
    }

    return refs;
}

} // namespace Olympe
