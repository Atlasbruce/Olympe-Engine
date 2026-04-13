#include "EntityPrefabRenderer.h"
#include "EntityPrefabGraphDocument.h"
#include "PrefabLoader.h"
#include "../../system/system_utils.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../Utilities/CustomCanvasEditor.h"
#include "../Framework/CanvasToolbarRenderer.h"
#include "../../DataManager.h"
#include <memory>

namespace Olympe {

EntityPrefabRenderer::EntityPrefabRenderer(PrefabCanvas& canvas)
    : m_canvas(canvas), m_filePath(""), m_isDirty(false), m_canvasPanelWidth(0.75f)
{
    // Initialize component palette with available types
    m_componentPalette.Initialize();

    // NEW: Initialize canvas editor adapter with CustomCanvasEditor for zoom support
    // Will be created once we have canvas dimensions in first Render() call
    m_canvasEditor = nullptr;  // Deferred initialization

    // Phase 41: Initialize unified framework with document
    // NOTE: Framework is initialized with EntityPrefabGraphDocument* (non-owning reference)
    // Document is owned by PrefabCanvas and managed separately
    EntityPrefabGraphDocument* document = m_canvas.GetDocument();
    if (document)
    {
        m_framework = std::make_unique<CanvasFramework>(document);
        SYSTEM_LOG << "[EntityPrefabRenderer] CanvasFramework initialized for EntityPrefab\n";
    }
}


EntityPrefabRenderer::~EntityPrefabRenderer()
{
}

void EntityPrefabRenderer::Render()
{
    RenderLayoutWithTabs();

    // Phase 41: Render unified framework toolbar + modals
    // This handles Save/SaveAs/Browse buttons with consistent UI
    if (m_framework)
    {
        m_framework->RenderModals();
    }
    else
    {
        // Fallback to legacy modal handling if framework not initialized
        DataManager::Get().RenderFilePickerModal();
        if (!DataManager::Get().IsFilePickerModalOpen()) {
            std::string selectedFile = DataManager::Get().GetSelectedFileFromModal();
            if (!selectedFile.empty()) {
                Load(selectedFile);
            }
        }

        DataManager::Get().RenderSaveFilePickerModal();
        if (!DataManager::Get().IsSaveFilePickerModalOpen()) {
            std::string selectedFile = DataManager::Get().GetSelectedSaveFile();
            if (!selectedFile.empty()) {
                Save(selectedFile);
            }
        }
    }
}

void EntityPrefabRenderer::RenderLayoutWithTabs()
{
    // Phase 41: Render unified framework toolbar (Save/SaveAs/Browse buttons)
    // This toolbar is consistent across all graph editors (VisualScript, BehaviorTree, EntityPrefab)
    if (m_framework)
    {
        m_framework->GetToolbar()->Render();
    }
    else
    {
        // Fallback: Render legacy minimap toolbar
        RenderToolbar();
    }

    // Layout: Canvas (left, ~75%) | Resize Handle | Tabbed Right Panel (right, ~25%)
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float handleWidth = 4.0f;
    float canvasWidth = totalWidth * m_canvasPanelWidth;
    float rightPanelWidth = totalWidth - canvasWidth - handleWidth;

    ImVec2 regionMin = ImGui::GetCursorScreenPos();

    // Render canvas on the left
    ImGui::BeginChild("EntityPrefabCanvas", ImVec2(canvasWidth, -1.0f), false, ImGuiWindowFlags_NoScrollbar);

    // NEW: Initialize canvas editor on first frame (when we have canvas dimensions)
    ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    if (!m_canvasEditor)
    {
        // First frame: create CustomCanvasEditor adapter with zoom support
        m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
            "PrefabCanvas",
            canvasScreenPos,
            canvasSize,
            1.0f,  // initial zoom
            0.1f,  // min zoom
            3.0f   // max zoom
        );
        SYSTEM_LOG << "[EntityPrefabRenderer] CustomCanvasEditor initialized\n";

        // CRITICAL: Pass adapter reference to PrefabCanvas so it can use it!
        m_canvas.SetCanvasEditor(m_canvasEditor.get());

        // CRITICAL: Initialize minimap visibility and settings
        m_canvasEditor->SetMinimapVisible(m_minimapVisible);
        m_canvasEditor->SetMinimapSize(m_minimapSize);
        m_canvasEditor->SetMinimapPosition(m_minimapPosition);
        SYSTEM_LOG << "[EntityPrefabRenderer] Minimap initialized (visible=" << m_minimapVisible << ")\n";
    }
    else
    {
        // Update canvas position/size (may change on window resize)
        // We can't directly update these, but CustomCanvasEditor stores them as const members
        // For now, we'll reinitialize if size changes significantly
        ImVec2 currentSize = ImGui::GetContentRegionAvail();
        if (currentSize.x != m_canvasEditor->GetCanvasSize().x || 
            currentSize.y != m_canvasEditor->GetCanvasSize().y)
        {
            // CRITICAL FIX: Save state BEFORE destroying old adapter!
            // If we call GetCanvasOffset() after std::make_unique, m_canvasEditor in PrefabCanvas
            // still points to the destroyed old adapter, causing a use-after-free crash.
            float oldZoom = m_canvasEditor->GetZoom();
            ImVec2 oldPan = m_canvasEditor->GetPan();

            // PHASE 37 FIX: Save minimap state before recreating adapter
            bool oldMinimapVisible = m_canvasEditor->IsMinimapVisible();
            float oldMinimapSize = m_canvasEditor->GetMinimapSize();
            int oldMinimapPosition = m_canvasEditor->GetMinimapPosition();

            // Size changed, reinitialize - old adapter is destroyed here
            m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
                "PrefabCanvas",
                canvasScreenPos,
                currentSize,
                oldZoom,  // preserve zoom (already extracted)
                0.1f, 3.0f
            );

            // Restore pan to NEW adapter
            m_canvasEditor->SetPan(oldPan);

            // PHASE 37 FIX: Restore minimap state to NEW adapter
            m_canvasEditor->SetMinimapVisible(oldMinimapVisible);
            m_canvasEditor->SetMinimapSize(oldMinimapSize);
            m_canvasEditor->SetMinimapPosition(oldMinimapPosition);

            // CRITICAL: Update reference immediately after creation!
            m_canvas.SetCanvasEditor(m_canvasEditor.get());
        }
    }

    // NEW: Use canvas editor BeginRender to handle input
    m_canvasEditor->BeginRender();

    m_canvas.Render();

    // NEW: Use canvas editor EndRender to finalize
    m_canvasEditor->EndRender();

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

    // Render right panel with tabs
    ImGui::BeginChild("RightPanel", ImVec2(rightPanelWidth, -1.0f), true);
    RenderRightPanelTabs();
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

void EntityPrefabRenderer::RenderRightPanelTabs()
{
    // Tab bar for Component Palette and Property Editor
    ImGui::TextUnformatted("Right Panel");
    ImGui::Separator();

    if (ImGui::BeginTabBar("RightPanelTabs", ImGuiTabBarFlags_None))
    {
        // Tab 0: Component Palette
        if (ImGui::BeginTabItem("Components"))
        {
            m_componentPalette.Render(m_canvas.GetDocument());
            ImGui::EndTabItem();
        }

        // Tab 1: Property Editor
        if (ImGui::BeginTabItem("Properties"))
        {
            // Connect property panel to selected nodes from canvas
            const std::vector<NodeId>& selectedNodes = m_canvas.GetDocument()->GetSelectedNodes();
            if (selectedNodes.size() > 0)
            {
                // Select first selected node for property editing
                m_propertyEditor.SetSelectedNode(selectedNodes[0]);
            }

            m_propertyEditor.Render(m_canvas.GetDocument());
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
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

// Phase 35.0: Canvas state management
void EntityPrefabRenderer::SaveCanvasState()
{
    // Capture current canvas pan/zoom from CustomCanvasEditor
    if (m_canvasEditor)
    {
        ImVec2 pan = m_canvasEditor->GetPan();
        m_savedCanvasState.panX = pan.x;
        m_savedCanvasState.panY = pan.y;
        m_savedCanvasState.zoom = m_canvasEditor->GetZoom();
    }
}

void EntityPrefabRenderer::RestoreCanvasState()
{
    // Restore previously saved canvas pan/zoom
    if (m_canvasEditor)
    {
        ImVec2 savedPan(m_savedCanvasState.panX, m_savedCanvasState.panY);
        m_canvasEditor->SetPan(savedPan);
        m_canvasEditor->SetZoom(m_savedCanvasState.zoom);
    }
}

std::string EntityPrefabRenderer::GetCanvasStateJSON() const
{
    // Return empty for now - can be extended to persist canvas state in JSON files
    return "";
}

void EntityPrefabRenderer::SetCanvasStateJSON(const std::string& json)
{
    // Parse and restore from JSON - can be extended for persistence
    (void)json;
}

void EntityPrefabRenderer::RenderToolbar()
{
    // Phase 37 — Minimap toolbar controls
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::BeginChild("##ToolbarEntityPrefab", ImVec2(0, 40.0f), true);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Minimap:");
    ImGui::SameLine();

    // Visibility checkbox
    if (ImGui::Checkbox("##minimap_visible_ep", &m_minimapVisible))
    {
        if (m_canvasEditor)
            m_canvasEditor->SetMinimapVisible(m_minimapVisible);
    }
    ImGui::SameLine();

    // Size slider
    if (ImGui::DragFloat("Size##minimap_ep", &m_minimapSize, 0.01f, 0.05f, 0.5f))
    {
        m_minimapSize = std::max(0.05f, std::min(0.5f, m_minimapSize));
        if (m_canvasEditor)
            m_canvasEditor->SetMinimapSize(m_minimapSize);
    }
    ImGui::SameLine();

    // Position combo
    const char* positionLabels[] = { "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right" };
    if (ImGui::Combo("Position##minimap_ep", &m_minimapPosition, positionLabels, 4, -1))
    {
        if (m_canvasEditor)
            m_canvasEditor->SetMinimapPosition(m_minimapPosition);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace Olympe
