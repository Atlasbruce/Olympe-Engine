#pragma once

#include "../BlueprintEditor/BPCommandSystem.h"
#include "../BlueprintEditor/BlueprintEditor.h"
#include <string>

namespace Olympe
{
    namespace NodeGraphShared
    {
        // Adapter to execute Blueprint editor commands through the Blueprint command stack.
        // This allows NodeGraphPanel to use a consistent adapter similar to the BT editor.
        class BlueprintAdapter
        {
        public:
            BlueprintAdapter(Blueprint::CommandStack* stack, int graphId)
                : m_stack(stack), m_graphId(std::to_string(graphId)) {}

            int CreateNode(const std::string& nodeType, float posX, float posY, const std::string& nodeName = "")
            {
                if (!m_stack) return 0;
                int createdId = -1;
                auto cmd = std::make_unique<Blueprint::CreateNodeCommand>(m_graphId, nodeType, posX, posY, nodeName, &createdId);
                m_stack->ExecuteCommand(std::move(cmd));
                return createdId >= 0 ? createdId : 0;
            }

            void DeleteNode(int nodeId)
            {
                if (!m_stack) return;
                auto cmd = std::make_unique<Blueprint::DeleteNodeCommand>(m_graphId, nodeId);
                m_stack->ExecuteCommand(std::move(cmd));
            }

            void MoveNode(int nodeId, float oldX, float oldY, float newX, float newY)
            {
                if (!m_stack) return;
                auto cmd = std::make_unique<Blueprint::MoveNodeCommand>(m_graphId, nodeId, oldX, oldY, newX, newY);
                m_stack->ExecuteCommand(std::move(cmd));
            }

            void ConnectNodes(int parentId, int childId)
            {
                if (!m_stack) return;
                auto cmd = std::make_unique<Blueprint::LinkNodesCommand>(m_graphId, parentId, childId);
                m_stack->ExecuteCommand(std::move(cmd));
            }

            void DisconnectNodes(int parentId, int childId)
            {
                if (!m_stack) return;
                auto cmd = std::make_unique<Blueprint::UnlinkNodesCommand>(m_graphId, parentId, childId);
                m_stack->ExecuteCommand(std::move(cmd));
            }

            void DuplicateNode(int nodeId)
            {
                if (!m_stack) return;
                auto cmd = std::make_unique<Blueprint::DuplicateNodeCommand>(m_graphId, nodeId);
                m_stack->ExecuteCommand(std::move(cmd));
            }

        private:
            Blueprint::CommandStack* m_stack;
            std::string m_graphId;
        };
    }
}
