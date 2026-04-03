#include "PrefabCanvas.h"
#include "ComponentNodeRenderer.h"
#include "../../Source/third_party/imgui/imgui.h"

namespace Olympe
{
    PrefabCanvas::PrefabCanvas() : m_canvasZoom(1.0f), m_isPanning(false), m_gridSpacing(50.0f), m_showGrid(true), m_showDebugInfo(false)
    { m_renderer = std::make_unique<ComponentNodeRenderer>(); }

    PrefabCanvas::~PrefabCanvas() { }

    void PrefabCanvas::Initialize(EntityPrefabGraphDocument* document) { m_document = document; if (m_renderer) { m_renderer->Initialize(); } }

    void PrefabCanvas::Render()
    { 
        if (!m_document || !m_renderer) { return; }
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::BeginChild("##PrefabCanvas", ImVec2(0, 0), true);
        if (m_showGrid) { RenderGrid(); }
        RenderConnections();
        RenderNodes();
        if (m_showDebugInfo) { RenderDebugInfo(); }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void PrefabCanvas::Update(float deltaTime) { (void)deltaTime; }

    void PrefabCanvas::OnMouseMove(float x, float y) { m_lastMousePos = Vector(x, y, 0.0f); }
    void PrefabCanvas::OnMouseDown(int button, float x, float y) { (void)button; m_lastMousePos = Vector(x, y, 0.0f); m_isPanning = true; }
    void PrefabCanvas::OnMouseUp(int button, float x, float y) { (void)button; (void)x; (void)y; m_isPanning = false; }
    void PrefabCanvas::OnMouseScroll(float delta) { ZoomCanvas(delta * 0.1f, m_lastMousePos.x, m_lastMousePos.y); }
    void PrefabCanvas::OnKeyDown(int keyCode) { (void)keyCode; }
    void PrefabCanvas::OnKeyUp(int keyCode) { (void)keyCode; }

    void PrefabCanvas::PanCanvas(float deltaX, float deltaY) { m_canvasOffset.x += deltaX; m_canvasOffset.y += deltaY; }

    void PrefabCanvas::ZoomCanvas(float zoomDelta, float centerX, float centerY)
    { 
        float oldZoom = m_canvasZoom;
        m_canvasZoom += zoomDelta;
        if (m_canvasZoom < 0.1f) { m_canvasZoom = 0.1f; }
        if (m_canvasZoom > 3.0f) { m_canvasZoom = 3.0f; }
        float zoomRatio = m_canvasZoom / oldZoom;
        m_canvasOffset.x = centerX + (m_canvasOffset.x - centerX) * zoomRatio;
        m_canvasOffset.y = centerY + (m_canvasOffset.y - centerY) * zoomRatio;
    }

    void PrefabCanvas::ResetView() { m_canvasOffset = Vector(0.0f, 0.0f, 0.0f); m_canvasZoom = 1.0f; }
    void PrefabCanvas::FitToContent() { ResetView(); }

    NodeId PrefabCanvas::GetNodeAtPosition(float x, float y)
    { 
        if (!m_document || !m_renderer) { return InvalidNodeId; }
        Vector pos(x, y, 0.0f);
        const std::vector<ComponentNode>& nodes = m_document->GetAllNodes();
        for (size_t i = 0; i < nodes.size(); ++i)
        { if (m_renderer->IsPointInNode(pos, nodes[i])) { return nodes[i].nodeId; } }
        return InvalidNodeId;
    }

    void PrefabCanvas::SelectNodeAt(float x, float y, bool addToSelection) { (void)addToSelection; if (!m_document) { return; } NodeId nodeId = GetNodeAtPosition(x, y); if (nodeId != InvalidNodeId) { m_document->SelectNode(nodeId); } }
    void PrefabCanvas::ClearSelection() { if (m_document) { m_document->DeselectAll(); } }

    void PrefabCanvas::SetGridEnabled(bool enabled) { m_showGrid = enabled; }
    bool PrefabCanvas::IsGridEnabled() const { return m_showGrid; }
    void PrefabCanvas::SetGridSpacing(float spacing) { m_gridSpacing = spacing; }
    float PrefabCanvas::GetGridSpacing() const { return m_gridSpacing; }
    void PrefabCanvas::SetShowDebugInfo(bool show) { m_showDebugInfo = show; }
    bool PrefabCanvas::GetShowDebugInfo() const { return m_showDebugInfo; }

    Vector PrefabCanvas::GetCanvasOffset() const { return m_canvasOffset; }
    void PrefabCanvas::SetCanvasOffset(const Vector& offset) { m_canvasOffset = offset; }
    float PrefabCanvas::GetCanvasZoom() const { return m_canvasZoom; }
    void PrefabCanvas::SetCanvasZoom(float zoom) { m_canvasZoom = zoom; }

    Vector PrefabCanvas::ScreenToCanvas(float screenX, float screenY) const
    { Vector screen(screenX, screenY, 0.0f); screen.x = (screen.x - m_canvasOffset.x) / m_canvasZoom; screen.y = (screen.y - m_canvasOffset.y) / m_canvasZoom; return screen; }

    Vector PrefabCanvas::CanvasToScreen(float canvasX, float canvasY) const
    { Vector canvas(canvasX, canvasY, 0.0f); canvas.x = canvas.x * m_canvasZoom + m_canvasOffset.x; canvas.y = canvas.y * m_canvasZoom + m_canvasOffset.y; return canvas; }

    void PrefabCanvas::RenderGrid()
    { 
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImU32 gridColor = ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.4f, 0.3f));
        for (float x = 0; x < canvasSize.x; x += m_gridSpacing)
        { drawList->AddLine(ImVec2(canvasPos.x + x, canvasPos.y), ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y), gridColor); }
        for (float y = 0; y < canvasSize.y; y += m_gridSpacing)
        { drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + y), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y), gridColor); }
    }

    void PrefabCanvas::RenderNodes() { if (!m_document || !m_renderer) { return; } m_renderer->RenderNodes(m_document); }
    void PrefabCanvas::RenderConnections() { if (!m_document || !m_renderer) { return; } m_renderer->RenderConnections(m_document); }
    void PrefabCanvas::RenderDebugInfo() { ImGui::Text("Zoom: %.2f", m_canvasZoom); ImGui::Text("Offset: %.0f, %.0f", m_canvasOffset.x, m_canvasOffset.y); }
    void PrefabCanvas::RenderSelectionBox() { }

} // namespace Olympe
