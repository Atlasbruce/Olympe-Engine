#pragma once

#include "../../third_party/imgui/imgui.h"

namespace Olympe
{
    enum class CanvasContextTargetType
    {
        None,
        Canvas,
        Node,
        Link
    };

    /** Routes a canvas right-click to one stable context-menu target. */
    class CanvasContextMenu
    {
    public:
        void OpenForHitTest(int hoveredNodeId, int hoveredLinkId)
        {
            if (hoveredNodeId >= 0) {
                m_targetType = CanvasContextTargetType::Node;
                m_targetId = hoveredNodeId;
            } else if (hoveredLinkId >= 0) {
                m_targetType = CanvasContextTargetType::Link;
                m_targetId = hoveredLinkId;
            } else {
                m_targetType = CanvasContextTargetType::Canvas;
                m_targetId = -1;
            }
            ImGui::OpenPopup("##CanvasContextMenu");
        }

        bool Begin()
        {
            return m_targetType != CanvasContextTargetType::None &&
                   ImGui::BeginPopup("##CanvasContextMenu");
        }

        CanvasContextTargetType GetTargetType() const { return m_targetType; }
        int GetTargetId() const { return m_targetId; }

        void Clear()
        {
            m_targetType = CanvasContextTargetType::None;
            m_targetId = -1;
        }

    private:
        CanvasContextTargetType m_targetType = CanvasContextTargetType::None;
        int m_targetId = -1;
    };
}
