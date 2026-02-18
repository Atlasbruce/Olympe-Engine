/**
 * @file ConnectPinsCommand.cpp
 * @brief Implementation of ConnectPinsCommand
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "ConnectPinsCommand.h"

namespace Olympe {
namespace NodeGraph {

ConnectPinsCommand::ConnectPinsCommand(GraphDocument* graph, PinId fromPin, PinId toPin)
    : m_graph(graph)
    , m_fromPin(fromPin)
    , m_toPin(toPin)
{
}

void ConnectPinsCommand::Execute()
{
    if (m_graph != nullptr)
    {
        m_createdLinkId = m_graph->ConnectPins(m_fromPin, m_toPin);
    }
}

void ConnectPinsCommand::Undo()
{
    if (m_graph != nullptr && m_createdLinkId.value != 0)
    {
        m_graph->DisconnectLink(m_createdLinkId);
    }
}

std::string ConnectPinsCommand::GetDescription() const
{
    return "Connect Pins";
}

} // namespace NodeGraph
} // namespace Olympe
