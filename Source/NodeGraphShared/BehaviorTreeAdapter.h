#pragma once

#include "../AI/BTGraphLayoutEngine.h"
#include "../AI/BehaviorTree.h"
#include <vector>

namespace Olympe
{
    namespace NodeGraphShared
    {
        class BehaviorTreeAdapter
        {
        public:
            BehaviorTreeAdapter(const ::BehaviorTreeAsset* tree, BTGraphLayoutEngine* layoutEngine)
                : m_tree(tree), m_layoutEngine(layoutEngine) {}

            inline std::vector<BTNodeLayout> ComputeLayout(float nodeSpacingX, float nodeSpacingY, float zoomFactor)
            {
                if (!m_tree || !m_layoutEngine)
                    return std::vector<BTNodeLayout>();
                return m_layoutEngine->ComputeLayout(m_tree, nodeSpacingX, nodeSpacingY, zoomFactor);
            }

            inline bool UpdateNodePosition(uint32_t nodeId, float x, float y)
            {
                if (!m_layoutEngine) return false;
                return m_layoutEngine->UpdateNodePosition(nodeId, x, y);
            }

            inline const BTNodeLayout* GetNodeLayout(uint32_t nodeId) const
            {
                if (!m_layoutEngine) return nullptr;
                return m_layoutEngine->GetNodeLayout(nodeId);
            }

        private:
            const ::BehaviorTreeAsset* m_tree;
            BTGraphLayoutEngine* m_layoutEngine;
        };
    }
}
