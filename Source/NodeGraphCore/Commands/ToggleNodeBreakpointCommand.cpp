/**
 * @file ToggleNodeBreakpointCommand.cpp
 * @brief Implementation of ToggleNodeBreakpointCommand (Phase 2.0)
 * @author Olympe Engine
 * @date 2026-02-19
 */

#include "ToggleNodeBreakpointCommand.h"
#include "../../system/system_utils.h"

namespace Olympe {
namespace NodeGraph {

ToggleNodeBreakpointCommand::ToggleNodeBreakpointCommand(
    NodeAnnotationsManager* annotations,
    int nodeId)
    : m_annotations(annotations)
    , m_nodeId(nodeId)
    , m_previousState(false)
    , m_newState(false)
{
    if (m_annotations != nullptr)
    {
        const NodeAnnotation* ann = m_annotations->GetAnnotation(m_nodeId);
        m_previousState = (ann != nullptr) ? ann->hasBreakpoint : false;
        m_newState = !m_previousState;
    }
}

void ToggleNodeBreakpointCommand::Execute()
{
    if (m_annotations != nullptr)
    {
        m_annotations->SetBreakpoint(m_nodeId, m_newState);
        SYSTEM_LOG << "[ToggleBreakpoint] Node " << m_nodeId
                   << " breakpoint -> " << (m_newState ? "ON" : "OFF") << std::endl;
    }
}

void ToggleNodeBreakpointCommand::Undo()
{
    if (m_annotations != nullptr)
    {
        m_annotations->SetBreakpoint(m_nodeId, m_previousState);
        SYSTEM_LOG << "[ToggleBreakpoint] Undo node " << m_nodeId
                   << " breakpoint -> " << (m_previousState ? "ON" : "OFF") << std::endl;
    }
}

std::string ToggleNodeBreakpointCommand::GetDescription() const
{
    return std::string("Toggle Breakpoint on node ") + std::to_string(m_nodeId);
}

} // namespace NodeGraph
} // namespace Olympe
