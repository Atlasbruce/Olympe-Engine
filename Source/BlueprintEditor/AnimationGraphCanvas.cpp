#include "AnimationGraphCanvas.h"
#include "AnimationGraphDocument.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"

namespace Olympe {

AnimationGraphCanvas::AnimationGraphCanvas() {}

void AnimationGraphCanvas::Initialize(AnimationGraphDocument* document)
{
    m_animationDocument = document;
}

void AnimationGraphCanvas::Render()
{
    PrefabCanvas::Render();

    if (!m_animationDocument)
    {
        return;
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATION_GRAPH_CLIP"))
        {
            const char* droppedClip = static_cast<const char*>(payload->Data);
            if (droppedClip && *droppedClip)
            {
                std::string stateName = m_animationDocument->MakeUniqueStateName(droppedClip);
                m_animationDocument->AddStateFromClip(stateName, droppedClip);
                SYSTEM_LOG << "[AnimationGraphCanvas] Dropped clip '" << droppedClip << "' as state '" << stateName << "'\n";
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (m_animationDocument && m_animationDocument->GetSources().empty())
    {
        ImGui::TextDisabled("No TSX sources imported yet.");
    }
}

void AnimationGraphCanvas::QueueClipDrop(const std::string& clipName, float screenX, float screenY)
{
    m_pendingDropClip = clipName;
    m_pendingDropX = screenX;
    m_pendingDropY = screenY;
    m_hasPendingDrop = true;
}

void AnimationGraphCanvas::QueueStateDrop(const std::string& stateName, const std::string& clipName, float screenX, float screenY)
{
    m_pendingDropState = stateName;
    m_pendingDropClip = clipName;
    m_pendingDropX = screenX;
    m_pendingDropY = screenY;
    m_hasPendingDrop = true;
}

bool AnimationGraphCanvas::AcceptAnimationClipDropAtScreenPos(const std::string& clipName, float screenX, float screenY)
{
    if (!m_animationDocument)
    {
        return false;
    }
    const std::string stateName = m_animationDocument->MakeUniqueStateName(clipName);
    return m_animationDocument->AddStateFromClip(stateName, clipName);
}

bool AnimationGraphCanvas::AcceptAnimationStateDropAtScreenPos(const std::string& stateName, const std::string& clipName, float screenX, float screenY)
{
    if (!m_animationDocument)
    {
        return false;
    }
    return m_animationDocument->AddStateFromClip(stateName, clipName);
}

bool AnimationGraphCanvas::FinishTransitionDragAtScreenPos(float screenX, float screenY)
{
    return false;
}

void AnimationGraphCanvas::UpdateSelectionAtScreenPos(float screenX, float screenY) {}

void AnimationGraphCanvas::RenderContextMenu() {}

} // namespace Olympe
