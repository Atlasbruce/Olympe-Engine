/**
 * @file VSConnectionValidator.h
 * @brief Stateless validator for exec connections in Visual Script graphs (Phase 20-B).
 * @author Olympe Engine
 * @date 2026-03-13
 *
 * @details
 * VSConnectionValidator provides static validation helpers that are called
 * from VisualScriptEditorPanel::RenderCanvas() before creating a new exec
 * connection via ConnectExec(). The validator is intentionally stateless so
 * that it can be called directly from unit tests without requiring ImGui or
 * ImNodes context.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem,
 * std::string_view.
 */

#pragma once

#include "../TaskSystem/TaskGraphTemplate.h"
#include <string>

namespace Olympe {

/**
 * @class VSConnectionValidator
 * @brief Stateless validator for exec connections in VS graphs.
 *
 * @details
 * All methods are static. The class holds no state.
 *
 * C++14 compliant.
 */
class VSConnectionValidator
{
public:
    /**
     * @brief Returns true if adding an exec connection from srcNodeID/srcPinName
     *        to dstNodeID would be valid (no self-loop, no duplicate output pin,
     *        no cycle).
     *        Logs the rejection reason via SYSTEM_LOG if returning false.
     *
     * @param graph       The current graph template to validate against.
     * @param srcNodeID   Source node identifier.
     * @param srcPinName  Source exec-out pin name (e.g. "Out", "Then").
     * @param dstNodeID   Destination node identifier.
     * @return true  The connection is valid and may be created.
     * @return false The connection would produce an invalid graph state.
     */
    static bool IsExecConnectionValid(const TaskGraphTemplate& graph,
                                      int srcNodeID,
                                      const std::string& srcPinName,
                                      int dstNodeID);
};

} // namespace Olympe
