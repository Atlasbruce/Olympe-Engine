#pragma once

#include "../AI/BTEditorCommand.h"
#include "../AI/BehaviorTree.h"

namespace Olympe
{
    namespace NodeGraphShared
    {
        // Lightweight adapter that executes BTEditorCommand-derived commands
        // using an existing BTCommandStack. This lets shared code invoke editor
        // actions without depending on the command stack implementation.
        class CommandAdapter
        {
        public:
            CommandAdapter(BTCommandStack* stack, BehaviorTreeAsset* tree)
                : m_stack(stack), m_tree(tree) {}

            uint32_t AddNode(BTNodeType type, const std::string& name, const Vector& pos)
            {
                if (!m_stack || !m_tree) return 0;
                auto cmd = std::make_unique<AddNodeCommand>(m_tree, type, name, pos);
                m_stack->Execute(std::move(cmd));
                // Newly created node is appended to tree; return last node id
                if (!m_tree->nodes.empty())
                    return m_tree->nodes.back().id;
                return 0;
            }

            void DeleteNode(uint32_t nodeId)
            {
                if (!m_stack || !m_tree) return;
                auto cmd = std::make_unique<DeleteNodeCommand>(m_tree, nodeId);
                m_stack->Execute(std::move(cmd));
            }

            void ConnectNodes(uint32_t parentId, uint32_t childId)
            {
                if (!m_stack || !m_tree) return;
                auto cmd = std::make_unique<ConnectNodesCommand>(m_tree, parentId, childId);
                m_stack->Execute(std::move(cmd));
            }

            void DisconnectNodes(uint32_t parentId, uint32_t childId)
            {
                if (!m_stack || !m_tree) return;
                auto cmd = std::make_unique<DisconnectNodesCommand>(m_tree, parentId, childId);
                m_stack->Execute(std::move(cmd));
            }

            void MoveNode(uint32_t nodeId, const Vector& oldPos, const Vector& newPos)
            {
                if (!m_stack || !m_tree) return;
                auto cmd = std::make_unique<MoveNodeCommand>(m_tree, nodeId, oldPos, newPos);
                m_stack->Execute(std::move(cmd));
            }

        private:
            BTCommandStack* m_stack;
            BehaviorTreeAsset* m_tree;
        };
    }
}
