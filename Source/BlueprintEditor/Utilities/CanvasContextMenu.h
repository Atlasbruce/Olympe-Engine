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
        // A canvas owns its popup identity.  The default preserves the
        // existing Placeholder integration; editors hosted in nested ImGui
        // children (such as Animation Graph) should provide their own ID.
        explicit CanvasContextMenu(const char* popupId = "##CanvasContextMenu")
            : m_popupId(popupId)
        {
        }

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
            ImGui::OpenPopup(m_popupId);
        }

        bool Begin()
        {
            if (m_targetType == CanvasContextTargetType::None)
                return false;
            if (ImGui::BeginPopup(m_popupId))
                return true;

            // The user dismissed the popup outside of a menu item.  Release
            // the routing target so normal canvas input resumes next frame.
            Clear();
            return false;
        }

        bool IsOpen() const
        {
            return m_targetType != CanvasContextTargetType::None &&
                   ImGui::IsPopupOpen(m_popupId);
        }

        CanvasContextTargetType GetTargetType() const { return m_targetType; }
        int GetTargetId() const { return m_targetId; }

        void Clear()
        {
            m_targetType = CanvasContextTargetType::None;
            m_targetId = -1;
        }

    private:
        const char* m_popupId;
        CanvasContextTargetType m_targetType = CanvasContextTargetType::None;
        int m_targetId = -1;
    };
}
