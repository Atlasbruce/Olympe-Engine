#pragma once

#include "../../third_party/imgui/imgui.h"

namespace Olympe
{
    /** Generic lifecycle state for a link dragged from a graph-specific source. */
    class CanvasLinkDrag
    {
    public:
        void Begin(int sourceNodeId, const ImVec2& previewEnd)
        {
            m_sourceNodeId = sourceNodeId;
            m_previewEnd = previewEnd;
        }

        void UpdatePreviewEnd(const ImVec2& previewEnd)
        {
            if (IsActive()) {
                m_previewEnd = previewEnd;
            }
        }

        int Complete()
        {
            const int sourceNodeId = m_sourceNodeId;
            Cancel();
            return sourceNodeId;
        }

        void Cancel()
        {
            m_sourceNodeId = -1;
        }

        bool IsActive() const { return m_sourceNodeId >= 0; }
        int GetSourceNodeId() const { return m_sourceNodeId; }
        const ImVec2& GetPreviewEnd() const { return m_previewEnd; }

    private:
        int m_sourceNodeId = -1;
        ImVec2 m_previewEnd = ImVec2(0.0f, 0.0f);
    };
}
