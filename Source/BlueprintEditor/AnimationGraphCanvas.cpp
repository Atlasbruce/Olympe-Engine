#include "AnimationGraphCanvas.h"
#include "../system/system_utils.h"

namespace Olympe {

AnimationGraphCanvas::AnimationGraphCanvas()
{
}

void AnimationGraphCanvas::Initialize(AnimationGraphDocument* document)
{
    m_animationDocument = document;
    if (document)
        SYSTEM_LOG << "[AnimationGraphCanvas] Initialized with document\n";
}

void AnimationGraphCanvas::Render()
{
    if (m_hasPendingDrop)
    {
        m_hasPendingDrop = false;
        if (AcceptAnimationClipDropAtScreenPos(m_pendingDropClip, m_pendingDropX, m_pendingDropY))
        {
            SYSTEM_LOG << "[AnimationGraphCanvas] Node created from item=" << m_pendingDropClip << "\n";
        }
    }
    if (m_hasPendingTransitionDrag)
    {
        ImGui::TextDisabled("Drag to another state to create a transition");
    }
    PrefabCanvas::Render();

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const ImVec2 mousePos = ImGui::GetMousePos();
        UpdateSelectionAtScreenPos(mousePos.x, mousePos.y);
    }
    if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        const ImVec2 mousePos = ImGui::GetMousePos();
        FinishTransitionDragAtScreenPos(mousePos.x, mousePos.y);
    }
}

void AnimationGraphCanvas::QueueClipDrop(const std::string& clipName, float screenX, float screenY)
{
    m_hasPendingDrop = true;
    m_pendingDropClip = clipName;
    m_pendingDropX = screenX;
    m_pendingDropY = screenY;
}

bool AnimationGraphCanvas::AcceptAnimationClipDropAtScreenPos(const std::string& clipName, float screenX, float screenY)
{
    if (!m_animationDocument)
        return false;

    const Vector canvasSpace = ScreenToCanvas(screenX, screenY);
    SYSTEM_LOG << "[AnimationGraphCanvas] AcceptAnimationClipDropAtScreenPos clip=" << clipName
               << " screen=(" << screenX << "," << screenY << ")"
               << " canvas=(" << canvasSpace.x << "," << canvasSpace.y << ")\n";
    return m_animationDocument->AddStateNode(clipName, clipName, canvasSpace.x, canvasSpace.y);
}

bool AnimationGraphCanvas::RemoveStateAtScreenPos(float screenX, float screenY)
{
    if (!m_animationDocument)
        return false;

    const Vector canvasSpace = ScreenToCanvas(screenX, screenY);
    const nlohmann::json& data = m_animationDocument->GetData();
    if (!data.contains("states") || !data["states"].is_array())
        return false;

    for (size_t i = 0; i < data["states"].size(); ++i)
    {
        const nlohmann::json& state = data["states"][i];
        float x = state.value("x", 0.0f);
        float y = state.value("y", 0.0f);
        if (canvasSpace.x >= x && canvasSpace.x <= x + 180.0f &&
            canvasSpace.y >= y && canvasSpace.y <= y + 70.0f)
        {
            const std::string name = state.value("name", "");
            SYSTEM_LOG << "[AnimationGraphCanvas] RemoveStateAtScreenPos name=" << name << "\n";
            return m_animationDocument->RemoveState(name);
        }
    }
    return false;
}

bool AnimationGraphCanvas::BeginTransitionDragAtScreenPos(float screenX, float screenY)
{
    if (!m_animationDocument)
        return false;

    const Vector canvasSpace = ScreenToCanvas(screenX, screenY);
    const nlohmann::json& data = m_animationDocument->GetData();
    if (!data.contains("states") || !data["states"].is_array())
        return false;

    for (size_t i = 0; i < data["states"].size(); ++i)
    {
        const nlohmann::json& state = data["states"][i];
        float x = state.value("x", 0.0f);
        float y = state.value("y", 0.0f);
        if (canvasSpace.x >= x && canvasSpace.x <= x + 180.0f &&
            canvasSpace.y >= y && canvasSpace.y <= y + 70.0f)
        {
            m_hasPendingTransitionDrag = true;
            m_transitionFromIndex = static_cast<int>(i);
            return true;
        }
    }
    return false;
}

bool AnimationGraphCanvas::FinishTransitionDragAtScreenPos(float screenX, float screenY)
{
    if (!m_animationDocument || !m_hasPendingTransitionDrag)
        return false;

    const Vector canvasSpace = ScreenToCanvas(screenX, screenY);
    const nlohmann::json& data = m_animationDocument->GetData();
    if (!data.contains("states") || !data["states"].is_array())
    {
        m_hasPendingTransitionDrag = false;
        m_transitionFromIndex = -1;
        return false;
    }

    for (size_t i = 0; i < data["states"].size(); ++i)
    {
        const nlohmann::json& state = data["states"][i];
        float x = state.value("x", 0.0f);
        float y = state.value("y", 0.0f);
        if (canvasSpace.x >= x && canvasSpace.x <= x + 180.0f &&
            canvasSpace.y >= y && canvasSpace.y <= y + 70.0f)
        {
            if (static_cast<int>(i) != m_transitionFromIndex)
            {
                const std::string fromName = data["states"][m_transitionFromIndex].value("name", "");
                const std::string toName = state.value("name", "");
                m_hasPendingTransitionDrag = false;
                m_transitionFromIndex = -1;
                return m_animationDocument->AddTransition(fromName, toName, 0.1f);
            }
            break;
        }
    }

    m_hasPendingTransitionDrag = false;
    m_transitionFromIndex = -1;
    return false;
}

void AnimationGraphCanvas::UpdateSelectionAtScreenPos(float screenX, float screenY)
{
    if (!m_animationDocument)
        return;

    const Vector canvasSpace = ScreenToCanvas(screenX, screenY);
    const nlohmann::json& data = m_animationDocument->GetData();
    if (!data.contains("states") || !data["states"].is_array())
        return;

    for (size_t i = 0; i < data["states"].size(); ++i)
    {
        const nlohmann::json& state = data["states"][i];
        float x = state.value("x", 0.0f);
        float y = state.value("y", 0.0f);
        if (canvasSpace.x >= x && canvasSpace.x <= x + 180.0f &&
            canvasSpace.y >= y && canvasSpace.y <= y + 70.0f)
        {
            m_animationDocument->SetSelectedStateIndex(static_cast<int>(i));
            return;
        }
    }

    m_animationDocument->SetSelectedStateIndex(-1);
}

void AnimationGraphCanvas::RenderContextMenu()
{
    if (!ImGui::BeginPopup("##animgraph_context_menu"))
        return;

    if (m_animationDocument && ImGui::MenuItem("Auto layout"))
        m_animationDocument->AutoLayoutStates();

    if (m_animationDocument && ImGui::MenuItem("Generate default transitions"))
        m_animationDocument->GenerateDefaultTransitions();

    ImGui::Separator();
    if (ImGui::MenuItem("Close"))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

} // namespace Olympe
