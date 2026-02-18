/**
 * @file ConnectPinsCommand.h
 * @brief Command for connecting pins
 * @author Olympe Engine
 * @date 2026-02-18
 */

#pragma once

#include "../CommandSystem.h"
#include "../GraphDocument.h"

namespace Olympe {
namespace NodeGraph {

/**
 * @class ConnectPinsCommand
 * @brief Creates a link between two pins
 */
class ConnectPinsCommand : public ICommand {
public:
    ConnectPinsCommand(GraphDocument* graph, PinId fromPin, PinId toPin);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;
    
private:
    GraphDocument* m_graph;
    PinId m_fromPin;
    PinId m_toPin;
    LinkId m_createdLinkId;
};

} // namespace NodeGraph
} // namespace Olympe
