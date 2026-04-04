#include "EntityPrefabRenderer.h"
#include "EntityPrefabGraphDocument.h"
#include "PrefabLoader.h"
#include "../../system/system_utils.h"
#include "../../Source/third_party/imgui/imgui.h"
#include <memory>

namespace Olympe {

EntityPrefabRenderer::EntityPrefabRenderer(PrefabCanvas& canvas)
    : m_canvas(canvas), m_filePath(""), m_isDirty(false), m_canvasPanelWidth(0.75f)
{
    // Initialize component palette with available types
    m_componentPalette.Initialize();
}

EntityPrefabRenderer::~EntityPrefabRenderer()
{
}

void EntityPrefabRenderer::Render()
{
    // Layout: Canvas (left, ~75%) | Resize Handle | Component Palette (right, ~25%)
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float handleWidth = 4.0f;
    float canvasWidth = totalWidth * m_canvasPanelWidth;
    float paletteWidth = totalWidth - canvasWidth - handleWidth;

    ImVec2 regionMin = ImGui::GetCursorScreenPos();

    // Render canvas on the left
    ImGui::BeginChild("EntityPrefabCanvas", ImVec2(canvasWidth, 0), false, ImGuiWindowFlags_NoScrollbar);
    m_canvas.Render();
    ImGui::EndChild();

    ImVec2 canvasEnd = ImGui::GetCursorScreenPos();

    ImGui::SameLine();

    // Resize handle
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##resizeHandle", ImVec2(handleWidth, -1.0f));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_canvasPanelWidth += ImGui::GetIO().MouseDelta.x / totalWidth;
        if (m_canvasPanelWidth < 0.5f) m_canvasPanelWidth = 0.5f;
        if (m_canvasPanelWidth > 0.9f) m_canvasPanelWidth = 0.9f;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Render component palette on the right
    ImGui::BeginChild("ComponentPalette", ImVec2(paletteWidth, 0), false, ImGuiWindowFlags_NoScrollbar);
    m_componentPalette.Render(m_canvas.GetDocument());
    ImGui::EndChild();

    // After both panels rendered, create an invisible overlay for drag-drop target
    // This covers the canvas area and accepts drops
    ImVec2 canvasMin = regionMin;
    ImVec2 canvasMax(regionMin.x + canvasWidth, canvasEnd.y);

    ImGui::SetCursorScreenPos(canvasMin);
    ImGui::PushClipRect(canvasMin, canvasMax, false);
    ImGui::Dummy(ImVec2(canvasWidth, canvasEnd.y - canvasMin.y));

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT_TYPE"))
        {
            const char* componentName = (const char*)payload->Data;

            // Use the canvas's coordinate transformation with stored canvas position
            m_canvas.AcceptComponentDropAtScreenPos(componentName, componentName, ImGui::GetMousePos().x, ImGui::GetMousePos().y);
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopClipRect();
}

bool EntityPrefabRenderer::Load(const std::string& path)
{
    m_filePath = path;

    try
    {
        // Load JSON file
        nlohmann::json jsonData = PrefabLoader::LoadJsonFromFile(path);
        SYSTEM_LOG << "[EntityPrefabRenderer] Loaded JSON from: " << path << "\n";

        // Verify it's an EntityPrefab
        if (jsonData.contains("blueprintType"))
        {
            std::string blueprintType = jsonData["blueprintType"].get<std::string>();
            SYSTEM_LOG << "[EntityPrefabRenderer] blueprintType: " << blueprintType << "\n";
            if (blueprintType != "EntityPrefab")
            {
                SYSTEM_LOG << "[EntityPrefabRenderer] ERROR: Not an EntityPrefab type\n";
                return false;
            }
        }

        // Get or create document
        EntityPrefabGraphDocument* document = m_canvas.GetDocument();
        if (document == nullptr)
        {
            SYSTEM_LOG << "[EntityPrefabRenderer] ERROR: GetDocument() returned nullptr\n";
            return false;
        }

        SYSTEM_LOG << "[EntityPrefabRenderer] Document obtained, loading from file...\n";

        // Load from JSON into document
        if (!document->LoadFromFile(path))
        {
            SYSTEM_LOG << "[EntityPrefabRenderer] ERROR: document->LoadFromFile() failed\n";
            return false;
        }

        SYSTEM_LOG << "[EntityPrefabRenderer] Successfully loaded prefab\n";
        m_isDirty = false;
        return true;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[EntityPrefabRenderer] EXCEPTION: " << e.what() << "\n";
        return false;
    }
}

bool EntityPrefabRenderer::Save(const std::string& path)
{
    std::string savePath = path.empty() ? m_filePath : path;

    if (savePath.empty())
    {
        return false;
    }

    try
    {
        EntityPrefabGraphDocument* document = m_canvas.GetDocument();
        if (document == nullptr)
        {
            return false;
        }

        // Save document to file
        if (!document->SaveToFile(savePath))
        {
            return false;
        }

        m_isDirty = false;
        m_filePath = savePath;
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool EntityPrefabRenderer::IsDirty() const
{
    // Check both the document's dirty flag and track modifications
    EntityPrefabGraphDocument* document = m_canvas.GetDocument();
    if (document != nullptr)
    {
        return document->IsDirty();
    }
    return m_isDirty;
}

std::string EntityPrefabRenderer::GetGraphType() const
{
    return "EntityPrefab";
}

std::string EntityPrefabRenderer::GetCurrentPath() const
{
    return m_filePath;
}

} // namespace Olympe
