/**
 * @file ToggleNodeBreakpointCommand.h
 * @brief Command to toggle a node breakpoint (Phase 2.0)
 * @author Olympe Engine
 * @date 2026-02-19
 */

#pragma once

#include "../CommandSystem.h"
#include "../NodeAnnotations.h"

namespace Olympe {
namespace NodeGraph {

/**
 * @class ToggleNodeBreakpointCommand
 * @brief Toggles the breakpoint state of a node (undo/redo-able)
 */
class ToggleNodeBreakpointCommand : public ICommand {
public:
    /**
     * @brief Construct the command
     * @param annotations Annotations manager to operate on
     * @param nodeId Target node ID
     */
    ToggleNodeBreakpointCommand(NodeAnnotationsManager* annotations, int nodeId);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    NodeAnnotationsManager* m_annotations;
    int m_nodeId;
    bool m_previousState;
    bool m_newState;
};

} // namespace NodeGraph
} // namespace Olympe
