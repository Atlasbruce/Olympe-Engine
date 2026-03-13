/**
 * @file VSConnectionValidator.cpp
 * @brief Stateless validator for exec connections in Visual Script graphs (Phase 20-B).
 * @author Olympe Engine
 * @date 2026-03-13
 *
 * @details
 * Implements three checks before a new exec connection is added:
 *   Check A — Self-loop  (srcNodeID == dstNodeID)
 *   Check B — Duplicate output pin  (srcPinName already has an outgoing connection)
 *   Check C — Cycle detection via iterative DFS
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem,
 * std::string_view.
 */

#include "VSConnectionValidator.h"
#include "../system/system_utils.h"

#include <map>
#include <vector>
#include <unordered_set>

namespace Olympe {

// ---------------------------------------------------------------------------
// VSConnectionValidator::IsExecConnectionValid
// ---------------------------------------------------------------------------

bool VSConnectionValidator::IsExecConnectionValid(const TaskGraphTemplate& graph,
                                                   int srcNodeID,
                                                   const std::string& srcPinName,
                                                   int dstNodeID)
{
    // ------------------------------------------------------------------
    // Check A — Self-loop
    // ------------------------------------------------------------------
    if (srcNodeID == dstNodeID)
    {
        SYSTEM_LOG << "[VSEditor] Connection rejected: self-loop on node #"
                   << srcNodeID << "\n";
        return false;
    }

    // ------------------------------------------------------------------
    // Check B — Duplicate output pin
    // ------------------------------------------------------------------
    for (size_t i = 0; i < graph.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& c = graph.ExecConnections[i];
        if (c.SourceNodeID == srcNodeID && c.SourcePinName == srcPinName)
        {
            SYSTEM_LOG << "[VSEditor] Connection rejected: pin "
                       << srcNodeID << "." << srcPinName
                       << " already has an outgoing connection\n";
            return false;
        }
    }

    // ------------------------------------------------------------------
    // Check C — Cycle detection via iterative DFS
    //
    // Build an adjacency list from existing exec connections, then DFS
    // from dstNodeID. If we can reach srcNodeID, adding src -> dst would
    // close a cycle.
    // ------------------------------------------------------------------
    std::map<int, std::vector<int> > adj;
    for (size_t i = 0; i < graph.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& c = graph.ExecConnections[i];
        adj[c.SourceNodeID].push_back(c.TargetNodeID);
    }

    std::vector<int> stack;
    std::unordered_set<int> visited;
    stack.push_back(dstNodeID);
    while (!stack.empty())
    {
        int cur = stack.back();
        stack.pop_back();
        if (visited.count(cur) > 0)
            continue;
        visited.insert(cur);
        if (cur == srcNodeID)
        {
            SYSTEM_LOG << "[VSEditor] Connection rejected: would create a cycle ("
                       << srcNodeID << " -> " << dstNodeID << ")\n";
            return false;
        }
        std::map<int, std::vector<int> >::const_iterator it = adj.find(cur);
        if (it != adj.end())
        {
            for (size_t j = 0; j < it->second.size(); ++j)
                stack.push_back(it->second[j]);
        }
    }

    return true;
}

} // namespace Olympe
