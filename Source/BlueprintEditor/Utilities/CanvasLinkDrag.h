#pragma once

#include "../../third_party/imgui/imgui.h"
#include "CanvasHitTesting.h"
#include <limits>
#include <vector>

namespace Olympe
{
    enum class CanvasPinDirection { Input, Output };

    /** Generic screen-space hit area for a graph pin. */
    struct CanvasPinHitArea
    {
        int nodeId = -1;
        int pinIndex = 0;
        CanvasPinDirection direction = CanvasPinDirection::Input;
        ImVec2 center = ImVec2(0.0f, 0.0f);
        float radius = 0.0f;

        bool Contains(const ImVec2& screenPos) const
        {
            return radius > 0.0f &&
                CanvasHitTesting::ContainsPointInCircle(screenPos, center, radius);
        }
    };

    inline const CanvasPinHitArea* FindCanvasPinAt(
        const std::vector<CanvasPinHitArea>& pins,
        const ImVec2& screenPos,
        CanvasPinDirection expectedDirection)
    {
        const CanvasPinHitArea* bestPin = nullptr;
        // Parentheses prevent the Windows max macro from expanding here.
        float bestDistanceSq = (std::numeric_limits<float>::max)();
        for (const CanvasPinHitArea& pin : pins)
        {
            if (pin.direction != expectedDirection || !pin.Contains(screenPos))
                continue;
            const float dx = screenPos.x - pin.center.x;
            const float dy = screenPos.y - pin.center.y;
            const float distanceSq = dx * dx + dy * dy;
            if (distanceSq < bestDistanceSq)
            {
                bestDistanceSq = distanceSq;
                bestPin = &pin;
            }
        }
        return bestPin;
    }

    /** Generic lifecycle state for a link dragged from a graph-specific source. */
    class CanvasLinkDrag
    {
    public:
        void Begin(int sourceNodeId, const ImVec2& previewEnd, int sourcePinIndex = 0)
        {
            m_sourceNodeId = sourceNodeId;
            m_sourcePinIndex = sourcePinIndex;
            m_previewEnd = previewEnd;
            ClearSnapTarget();
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
            m_sourcePinIndex = 0;
            ClearSnapTarget();
        }

        bool IsActive() const { return m_sourceNodeId >= 0; }
        int GetSourceNodeId() const { return m_sourceNodeId; }
        int GetSourcePinIndex() const { return m_sourcePinIndex; }
        const ImVec2& GetPreviewEnd() const { return m_previewEnd; }

        void SetSnapTarget(const CanvasPinHitArea& target)
        {
            m_hasSnapTarget = true;
            m_snapTarget = target;
            m_previewEnd = target.center;
        }

        void ClearSnapTarget()
        {
            m_hasSnapTarget = false;
            m_snapTarget = CanvasPinHitArea{};
        }

        bool HasSnapTarget() const { return m_hasSnapTarget; }
        const CanvasPinHitArea& GetSnapTarget() const { return m_snapTarget; }

    private:
        int m_sourceNodeId = -1;
        int m_sourcePinIndex = 0;
        ImVec2 m_previewEnd = ImVec2(0.0f, 0.0f);
        bool m_hasSnapTarget = false;
        CanvasPinHitArea m_snapTarget;
    };
}
