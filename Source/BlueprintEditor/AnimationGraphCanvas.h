#pragma once

#include "EntityPrefabEditor\PrefabCanvas.h"
#include "..\vector.h"
#include <string>

namespace Olympe {

class AnimationGraphDocument;

class AnimationGraphCanvas : public PrefabCanvas
{
public:
    AnimationGraphCanvas();
    ~AnimationGraphCanvas() = default;

    void Initialize(AnimationGraphDocument* document);
    void Render() override;
    void QueueClipDrop(const std::string& clipName, float screenX, float screenY);

private:
    bool AcceptAnimationClipDropAtScreenPos(const std::string& clipName, float screenX, float screenY);
    bool FinishTransitionDragAtScreenPos(float screenX, float screenY);
    void UpdateSelectionAtScreenPos(float screenX, float screenY);
    void RenderContextMenu();

    AnimationGraphDocument* m_animationDocument = nullptr;

    // Drop handling state
    bool m_hasPendingDrop = false;
    std::string m_pendingDropClip;
    float m_pendingDropX = 0.0f;
    float m_pendingDropY = 0.0f;

    // Transition drag state (inherited from PrefabCanvas as needed)
    bool m_hasPendingTransitionDrag = false;
    int m_transitionFromIndex = -1;
    Vector m_linkDragStartPos;
};

} // namespace Olympe
