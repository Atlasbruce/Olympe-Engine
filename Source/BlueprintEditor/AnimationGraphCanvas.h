#pragma once

#include "EntityPrefabEditor/PrefabCanvas.h"
#include "AnimationGraphDocument.h"

namespace Olympe {

class AnimationGraphCanvas : public PrefabCanvas
{
public:
    AnimationGraphCanvas();

    void Initialize(AnimationGraphDocument* document);
    void Render();
    void QueueClipDrop(const std::string& clipName, float screenX, float screenY);
    bool AcceptAnimationClipDropAtScreenPos(const std::string& clipName, float screenX, float screenY);
    bool RemoveStateAtScreenPos(float screenX, float screenY);
    bool BeginTransitionDragAtScreenPos(float screenX, float screenY);
    bool FinishTransitionDragAtScreenPos(float screenX, float screenY);
    void UpdateSelectionAtScreenPos(float screenX, float screenY);
    void RenderContextMenu();

private:
    AnimationGraphDocument* m_animationDocument = nullptr;
    bool m_hasPendingDrop = false;
    std::string m_pendingDropClip;
    float m_pendingDropX = 0.0f;
    float m_pendingDropY = 0.0f;
    bool m_hasPendingTransitionDrag = false;
    int m_transitionFromIndex = -1;
};

} // namespace Olympe
