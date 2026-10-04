/**
 * @file VisualScriptGraphClipboard.h
 * @brief In-memory clipboard payload for Visual Script graph selections.
 */

#pragma once

#include "GraphClipboard.h"
#include "../../TaskSystem/TaskGraphTemplate.h"

#include <vector>

namespace Olympe {

/**
 * Carries a self-contained Visual Script subgraph between editor tabs.
 * Node definitions retain their VS-specific settings (dynamic pins,
 * Blackboard bindings, conditions and parameters); links are kept only when
 * both endpoints belong to the copied selection.
 */
class VisualScriptGraphClipboardPayload final : public IGraphClipboardPayload
{
public:
    struct NodeEntry
    {
        TaskNodeDefinition definition;
        float relativeX = 0.0f;
        float relativeY = 0.0f;
    };

    const char* GetGraphTypeId() const override { return "VisualScript"; }

    std::vector<NodeEntry> nodes;
    std::vector<ExecPinConnection> execConnections;
    std::vector<DataPinConnection> dataConnections;

    // Graph-scoped dependencies required by copied nodes when they are pasted
    // into a different Visual Script document.
    std::vector<ConditionPreset> presets;
    std::vector<VariableDefinition> localVariables;
    std::vector<BlackboardEntry> blackboardEntries;
    json globalVariableValues = json::object();
};

} // namespace Olympe
