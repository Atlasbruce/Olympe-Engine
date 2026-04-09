/**
 * @file BehaviorTreeRenderer.cpp
 * @brief IGraphRenderer adapter for BehaviorTree graphs.
 * @author Olympe Engine
 * @date 2026-03-11
 * @updated 2026-04-08 (Phase 2: Added split-panel layout with palette + property tabs)
 *
 * @details C++14 compliant.
 */

#include "BehaviorTreeRenderer.h"
#include "BTNodeGraphManager.h"
#include "../AI/AIGraphPlugin_BT/BTNodePalette.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"

#include <iostream>
#include <memory>

namespace Olympe {

BehaviorTreeRenderer::BehaviorTreeRenderer(NodeGraphPanel& panel)
    : m_panel(panel)
    , m_graphId(-1)
    , m_canvasPanelWidth(0.75f)
    , m_rightPanelTabSelection(0)
{
    // Create BTNodePalette for drag-drop node creation
    m_palette = std::make_unique<AI::BTNodePalette>();

    // Initialize property panel
    m_propertyPanel.Initialize();
}

BehaviorTreeRenderer::~BehaviorTreeRenderer()
{
    if (m_graphId >= 0)
    {
        NodeGraphManager::Get().CloseGraph(m_graphId);
        m_graphId = -1;
    }
}

bool BehaviorTreeRenderer::CreateNew(const std::string& name)
{
    // Close any previously loaded graph
    if (m_graphId >= 0)
    {
        NodeGraphManager::Get().CloseGraph(m_graphId);
        m_graphId = -1;
        m_filePath.clear();
    }

    // Create new empty graph and set it active immediately
    m_graphId = NodeGraphManager::Get().CreateGraph(name, "BehaviorTree");
    if (m_graphId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Failed to create new BehaviorTree graph\n";
        return false;
    }

    NodeGraphManager::Get().SetActiveGraph(m_graphId);
    SYSTEM_LOG << "[BehaviorTreeRenderer] Created new BehaviorTree graph: '" << name 
               << "' (id=" << m_graphId << ")\n";
    return true;
}

void BehaviorTreeRenderer::Render()
{
    if (m_graphId >= 0)
    {
        NodeGraphManager::Get().SetActiveGraph(m_graphId);
    }
    RenderLayoutWithTabs();
}

void BehaviorTreeRenderer::RenderLayoutWithTabs()
{
    // Suppress NodeGraphPanel's GraphTabs since we manage tabs in BehaviorTreeRenderer
    m_panel.m_SuppressGraphTabs = true;

    // Layout: Canvas (left, ~75%) | Resize Handle | Tabbed Right Panel (right, ~25%)
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float handleWidth = 4.0f;
    float canvasWidth = totalWidth * m_canvasPanelWidth;
    float rightPanelWidth = totalWidth - canvasWidth - handleWidth;

    ImVec2 regionMin = ImGui::GetCursorScreenPos();

    // Render canvas on the left
    ImGui::BeginChild("BTNodeCanvas", ImVec2(canvasWidth, 0), false, ImGuiWindowFlags_NoScrollbar);
    m_canvasScreenPos = ImGui::GetCursorScreenPos();  // Store canvas position for coordinate transforms
    m_panel.RenderContent();
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
    ImGui::BeginChild("BTRightPanel", ImVec2(rightPanelWidth, 0), true);
    RenderRightPanelTabs();
    ImGui::EndChild();

    // Create invisible drop target overlay covering the entire layout area
    ImVec2 layoutEnd = ImGui::GetCursorScreenPos();
    ImVec2 overlayMin = regionMin;
    ImVec2 overlayMax(regionMin.x + totalWidth, layoutEnd.y);

    ImGui::SetCursorScreenPos(overlayMin);
    ImGui::Dummy(ImVec2(totalWidth, layoutEnd.y - regionMin.y));

    // Accept drag-drop for node palette items
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BT_NODE_TYPE"))
        {
            const char* nodeTypeStr = (const char*)payload->Data;
            ImVec2 mousePos = ImGui::GetMousePos();
            AcceptNodeDrop(nodeTypeStr, mousePos.x, mousePos.y);
        }
        ImGui::EndDragDropTarget();
    }

    // Handle keyboard shortcuts for copy/paste/duplicate
    HandleKeyboardShortcuts();
}

void BehaviorTreeRenderer::RenderRightPanelTabs()
{
    if (ImGui::BeginTabBar("BTRightPanelTabs", ImGuiTabBarFlags_None))
    {
        // Tab 0: Node Palette
        if (ImGui::BeginTabItem("Palette"))
        {
            if (m_palette)
            {
                bool paletteOpen = true;
                m_palette->Render(&paletteOpen);
            }
            ImGui::EndTabItem();
        }

        // Tab 1: Node Properties
        if (ImGui::BeginTabItem("Properties"))
        {
            // Wire selected node from canvas to property panel
            int selectedNodeId = m_panel.m_SelectedNodeId;
            if (selectedNodeId >= 0 && m_graphId >= 0)
            {
                if (!m_propertyPanel.HasSelectedNode() || m_propertyPanel.m_selectedNodeId != selectedNodeId)
                {
                    m_propertyPanel.SetSelectedNode(m_graphId, selectedNodeId);
                }
            }
            else if (selectedNodeId < 0)
            {
                m_propertyPanel.ClearSelection();
            }

            m_propertyPanel.Render();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

bool BehaviorTreeRenderer::Load(const std::string& path)
{
    if (path.empty())
        return false;

    // Close the previously loaded graph, if any.
    if (m_graphId >= 0)
    {
        NodeGraphManager::Get().CloseGraph(m_graphId);
        m_graphId = -1;
    }

    int newId = NodeGraphManager::Get().LoadGraph(path);
    if (newId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Failed to load graph: " << path << "\n";
        return false;
    }

    m_graphId  = newId;
    m_filePath = path;
    NodeGraphManager::Get().SetActiveGraph(m_graphId);
    SYSTEM_LOG << "[BehaviorTreeRenderer] Loaded BT graph: " << path
               << " (id=" << m_graphId << ")\n";
    return true;
}

bool BehaviorTreeRenderer::Save(const std::string& path)
{
    if (m_graphId < 0)
        return false;

    const std::string savePath = path.empty() ? m_filePath : path;
    if (savePath.empty())
        return false;

    bool ok = NodeGraphManager::Get().SaveGraph(m_graphId, savePath);
    if (ok && !path.empty())
        m_filePath = path;
    return ok;
}

bool BehaviorTreeRenderer::IsDirty() const
{
    if (m_graphId < 0)
        return false;

    const NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_graphId);
    return (graph != nullptr) && graph->IsDirty();
}

std::string BehaviorTreeRenderer::GetGraphType() const
{
    return "BehaviorTree";
}

std::string BehaviorTreeRenderer::GetCurrentPath() const
{
    return m_filePath;
}

void BehaviorTreeRenderer::AcceptNodeDrop(const std::string& nodeType, float screenX, float screenY)
{
    // Validate graph is active
    if (m_graphId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Cannot drop node: no active graph\n";
        return;
    }

    NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_graphId);
    if (!graph)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Cannot drop node: graph not found (id=" << m_graphId << ")\n";
        return;
    }

    // Convert nodeType string format (e.g., "BT_Selector") to enum
    // StringToNodeType expects just the suffix part (e.g., "Selector")
    std::string typeNameForConversion = nodeType;

    // Remove "BT_" prefix if present for conversion to NodeType enum
    if (typeNameForConversion.length() > 3 && typeNameForConversion.substr(0, 3) == "BT_")
    {
        typeNameForConversion = typeNameForConversion.substr(3);
    }

    NodeType enumType = StringToNodeType(typeNameForConversion);

    // Transform screen coordinates to canvas coordinates
    // Formula: canvas = (screen - canvasScreenPos - offset) / zoom
    // For BehaviorTree, we don't have explicit pan/zoom yet, so use direct transformation
    ImVec2 relativePos(screenX - m_canvasScreenPos.x, screenY - m_canvasScreenPos.y);

    // Create the new node at the drop position
    int newNodeId = graph->CreateNode(enumType, relativePos.x, relativePos.y, nodeType);

    if (newNodeId >= 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Created node from drop: type=" << nodeType
                   << ", id=" << newNodeId << ", pos=(" << relativePos.x << "," << relativePos.y << ")\n";

        // Mark graph as dirty for save tracking
        graph->MarkDirty();
    }
    else
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Failed to create node from drop: type=" << nodeType << "\n";
    }
}

void BehaviorTreeRenderer::HandleKeyboardShortcuts()
{
    // Check that we have an active graph
    if (m_graphId < 0)
        return;

    NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_graphId);
    if (!graph)
        return;

    // Get keyboard state
    bool ctrlPressed = ImGui::GetIO().KeyCtrl;

    // Ctrl+C: Copy selected node
    if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_C))
    {
        int selectedNodeId = m_panel.m_SelectedNodeId;
        if (selectedNodeId >= 0)
        {
            std::vector<int> nodeIds = { selectedNodeId };
            graph->CopyNodesToClipboard(nodeIds);
            SYSTEM_LOG << "[BehaviorTreeRenderer] Copied node " << selectedNodeId << "\n";
        }
    }

    // Ctrl+V: Paste nodes from clipboard
    if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_V))
    {
        if (!graph->m_clipboardData.empty())
        {
            std::vector<int> pastedNodeIds = graph->PasteNodesFromClipboard(30.0f, 30.0f);
            if (!pastedNodeIds.empty())
            {
                SYSTEM_LOG << "[BehaviorTreeRenderer] Pasted " << pastedNodeIds.size() << " node(s)\n";
                // Select first pasted node for convenience
                m_panel.m_SelectedNodeId = pastedNodeIds[0];
            }
        }
    }

    // Ctrl+D: Duplicate selected node
    if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_D))
    {
        int selectedNodeId = m_panel.m_SelectedNodeId;
        if (selectedNodeId >= 0)
        {
            std::vector<int> nodeIds = { selectedNodeId };
            std::vector<int> duplicatedNodeIds = graph->DuplicateNodes(nodeIds, 30.0f, 30.0f);
            if (!duplicatedNodeIds.empty())
            {
                SYSTEM_LOG << "[BehaviorTreeRenderer] Duplicated node " << selectedNodeId << "\n";
                // Select first duplicated node
                m_panel.m_SelectedNodeId = duplicatedNodeIds[0];
            }
        }
    }
}

// Phase 35.0: Canvas state management
void BehaviorTreeRenderer::SaveCanvasState()
{
    // For BehaviorTree, save the canvas screen position
    // This helps preserve viewport context
    m_savedCanvasState.canvasOffset = m_canvasScreenPos;
}

void BehaviorTreeRenderer::RestoreCanvasState()
{
    // Restore previously saved canvas offset
    m_canvasScreenPos = m_savedCanvasState.canvasOffset;
}

std::string BehaviorTreeRenderer::GetCanvasStateJSON() const
{
    // Return empty for now - can be extended to persist canvas state in JSON files
    return "";
}

void BehaviorTreeRenderer::SetCanvasStateJSON(const std::string& json)
{
    // Parse and restore from JSON - can be extended for persistence
    (void)json;
}

} // namespace Olympe
