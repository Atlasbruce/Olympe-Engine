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
#include "NodeGraphPanel.h"
#include "Framework/CanvasToolbarRenderer.h"
#include "BTNodeGraphManager.h"
#include "../NodeGraphCore/NodeGraphManager.h"
#include "GraphExecutionTracer.h"
#include "ExecutionTestPanel.h"
#include "BehaviorTreeExecutor.h"
#include "../NodeGraphShared/BehaviorTreeGraphAdapter.h"
#include "../AI/AIGraphPlugin_BT/BTNodePalette.h"
#include "../AI/BehaviorTree.h"
#include "../DataManager.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"

#include <iostream>
#include <memory>
#include <set>

namespace Olympe {

// Type aliases for backward compatibility - modern schema only has GraphDocument/NodeData
using GraphDocument = Olympe::NodeGraphTypes::GraphDocument;
using NodeData = Olympe::NodeGraphTypes::NodeData;
using NodeId = Olympe::NodeGraphTypes::NodeId;

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

    // Initialize execution test panel (Phase 35)
    m_executionTestPanel = std::make_unique<ExecutionTestPanel>();
    m_executionTestPanel->Initialize();

    // Initialize tracer
    m_lastTracer = std::make_unique<GraphExecutionTracer>();

    // Phase 41 — Framework integration
    m_document = std::make_unique<BehaviorTreeGraphDocument>(this);
    m_framework = std::make_unique<CanvasFramework>(m_document.get());
    SYSTEM_LOG << "[BehaviorTreeRenderer] CanvasFramework initialized\n";
}

BehaviorTreeRenderer::~BehaviorTreeRenderer()
{
    // PHASE 58 FIX: Check singleton validity before accessing on destruction
    // Prevents use-after-free crash when NodeGraphManager is already destroyed
    // due to singleton destruction order violation (NodeGraphManager destroyed before TabManager)
    if (m_graphId >= 0 && NodeGraph::NodeGraphManager::IsValid())
    {
        NodeGraph::NodeGraphManager::Get().CloseGraph(GraphId{static_cast<uint32_t>(m_graphId)});
        m_graphId = -1;
    }
}

IGraphDocument* BehaviorTreeRenderer::GetDocument() const
{
    // Phase 44.2: Return the document adapter for framework integration
    // This allows TabManager to reuse the same document instance
    // instead of creating a new BehaviorTreeGraphDocument wrapper
    return m_document.get();
}

bool BehaviorTreeRenderer::CreateNew(const std::string& name)
{
    // Close any previously loaded graph
    if (m_graphId >= 0)
    {
        NodeGraph::NodeGraphManager::Get().CloseGraph(GraphId{static_cast<uint32_t>(m_graphId)});
        m_graphId = -1;
        m_filePath.clear();
    }

    // Reset ImNodes adapter for new graph (Phase 54: Fix drag-drop on new BT creation)
    m_imNodesAdapter = nullptr;

    // Create new empty graph and set it active immediately
    GraphId newGraphId = NodeGraph::NodeGraphManager::Get().CreateGraph(name, "BehaviorTree");
    if (newGraphId.value == 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Failed to create new BehaviorTree graph\n";
        return false;
    }

    m_graphId = static_cast<int>(newGraphId.value);
    NodeGraph::NodeGraphManager::Get().SetActiveGraph(newGraphId);

    // Phase 50.1.1: Clear filepath for new unsaved graph
    if (m_document)
    {
        m_document->SetFilePath("");
        SYSTEM_LOG << "[BehaviorTreeRenderer] Cleared filepath for new graph\n";
    }

    SYSTEM_LOG << "[BehaviorTreeRenderer] Created new BehaviorTree graph: '" << name 
               << "' (id=" << m_graphId << ")\n";
    return true;
}

void BehaviorTreeRenderer::Render()
{
    // Phase 60 FIX: Remove SetActiveGraph from render loop - it was spamming console
    // SetActiveGraph is only called once in Load() and CreateNew() when graph changes
    // Calling it every frame (60+/sec) causes console spam "[NodeGraphManager] Set active graph to ID X"
    RenderLayoutWithTabs();

    // Render execution test panel as overlay (displays simulation results)
    if (m_executionTestPanel)
    {
        m_executionTestPanel->Render();
    }

    // Phase 53: Disable legacy modal system - use framework only
    // Framework handles all Save/SaveAs modals exclusively.
    // Legacy DataManager modal is disabled to prevent ID conflicts with framework.
    // (Will be removed in future phase after complete framework migration)
    if (m_framework)
    {
        m_framework->RenderModals();
    }
    // LEGACY FALLBACK DISABLED (Phase 53):
    // else
    // {
    //     // Fallback to legacy file picker modal
    //     DataManager::Get().RenderFilePickerModal();
    // }
}

void BehaviorTreeRenderer::RenderLayoutWithTabs()
{
    // Suppress NodeGraphPanel's GraphTabs since we manage tabs in BehaviorTreeRenderer
    m_panel.m_SuppressGraphTabs = true;

    // Phase 50.1: Suppress legacy modals when framework toolbar is active
    // The framework toolbar (CanvasToolbarRenderer) handles Save/SaveAs modals,
    // so we don't render the legacy NodeGraphPanel modals to avoid conflicts
    m_panel.m_SuppressLegacyModals = true;

    // Phase 41 — Framework integration: Use unified toolbar if available
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->Render();
        ImGui::SameLine();
    }

    // BehaviorTree-specific controls (framework handles Save/SaveAs/Browse)
    if (ImGui::Button("Run Graph", ImVec2(100, 0)))
    {
        OnRunGraphClicked();
    }

    ImGui::SameLine();
    ImGui::Text("(Ctrl+Shift+R to run)");

    ImGui::Separator();

    // Layout: Canvas (left, ~75%) | Resize Handle | Tabbed Right Panel (right, ~25%)
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float handleWidth = 4.0f;
    float canvasWidth = totalWidth * m_canvasPanelWidth;
    float rightPanelWidth = totalWidth - canvasWidth - handleWidth;

    ImVec2 regionMin = ImGui::GetCursorScreenPos();

    // Render canvas on the left with ImNodes adapter (Phase 50.3)
    ImGui::BeginChild("BTNodeCanvas", ImVec2(canvasWidth, 0), false, ImGuiWindowFlags_NoScrollbar);
    m_canvasScreenPos = ImGui::GetCursorScreenPos();  // Store canvas position for coordinate transforms

    // Initialize ImNodes adapter if needed
    if (!m_imNodesAdapter && m_graphId >= 0)
    {
        m_imNodesAdapter = std::make_unique<BehaviorTreeImNodesAdapter>();
        m_imNodesAdapter->Initialize(m_graphId);
    }

    // Render using ImNodes adapter
    if (m_imNodesAdapter)
    {
        m_imNodesAdapter->Render();
    }

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

    // LOAD TRACKING: Log entry point
    static std::set<std::string> s_loadingStack;
    static int s_loadCallDepth = 0;
    s_loadCallDepth++;
    std::string indent(s_loadCallDepth * 2, ' ');

    SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] ENTRY: path=" << path << " (call depth: " << s_loadCallDepth << ")\n";

    // Detect circular/multiple loads
    if (s_loadingStack.count(path) > 0)
    {
        SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] WARNING: Already loading this file (circular load?), depth=" << s_loadingStack.size() << "\n";
    }
    s_loadingStack.insert(path);

    // Close the previously loaded graph, if any.
    if (m_graphId >= 0)
    {
        SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] Closing previous graph (id=" << m_graphId << ")\n";
        NodeGraph::NodeGraphManager::Get().CloseGraph(GraphId{static_cast<uint32_t>(m_graphId)});
        m_graphId = -1;
    }

    SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] About to call NodeGraphManager::LoadGraph()\n";
    GraphId newGraphId = NodeGraph::NodeGraphManager::Get().LoadGraph(path);
    if (newGraphId.value == 0)
    {
        SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] ERROR: LoadGraph() returned invalid id, returning false\n";
        s_loadingStack.erase(path);
        s_loadCallDepth--;
        return false;
    }

    m_graphId  = static_cast<int>(newGraphId.value);
    m_filePath = path;
    SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] Graph loaded, id=" << m_graphId << ", setting as active\n";
    NodeGraph::NodeGraphManager::Get().SetActiveGraph(newGraphId);

    // Phase 50.1.1: CRITICAL - Sync filepath to framework document
    // This ensures CanvasToolbarRenderer sees the loaded filepath
    // so Save button works directly (no SaveAs modal)
    if (m_document)
    {
        m_document->SetFilePath(path);
        SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] Synced filepath to document: " << path << "\n";
    }

    SYSTEM_LOG << indent << "[BehaviorTreeRenderer::Load] SUCCESS: loaded BT graph, returning true (call depth: " << s_loadCallDepth << ")\n";

    s_loadingStack.erase(path);
    s_loadCallDepth--;
    return true;
}

bool BehaviorTreeRenderer::Save(const std::string& path)
{
    SYSTEM_LOG << "[BehaviorTreeRenderer::Save] ENTER path: " << path << "\n";
    if (m_graphId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::Save] EXIT: Invalid graph ID\n";
        return false;
    }

    const std::string savePath = path.empty() ? m_filePath : path;
    if (savePath.empty())
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::Save] EXIT: Empty save path\n";
        return false;
    }

    GraphId graphId{static_cast<uint32_t>(m_graphId)};
    SYSTEM_LOG << "[BehaviorTreeRenderer::Save] Calling NodeGraphManager::SaveGraph(id=" << m_graphId << ", path=" << savePath << ")\n";
    bool ok = NodeGraph::NodeGraphManager::Get().SaveGraph(graphId, savePath);
    SYSTEM_LOG << "[BehaviorTreeRenderer::Save] SaveGraph result: " << (ok ? "SUCCESS" : "FAILED") << "\n";
    if (ok && !path.empty())
        m_filePath = path;
    return ok;
}

bool BehaviorTreeRenderer::IsDirty() const
{
    // Phase 59 FIX: Check graph document dirty flag
    // Enables proper Save button state and unsaved indicator in UI
    if (m_graphId < 0 || !NodeGraph::NodeGraphManager::IsValid())
    {
        return false; // No valid graph = no unsaved changes
    }

    GraphId id{static_cast<uint32_t>(m_graphId)};
    GraphDocument* graph = NodeGraph::NodeGraphManager::Get().GetGraph(id);
    if (!graph)
    {
        return false; // Graph not found
    }

    return graph->IsDirty();
}

std::string BehaviorTreeRenderer::GetGraphType() const
{
    return "BehaviorTree";
}

std::string BehaviorTreeRenderer::GetCurrentPath() const
{
    return m_filePath;
}

void BehaviorTreeRenderer::SetFilePath(const std::string& path)
{
    SYSTEM_LOG << "[BehaviorTreeRenderer::SetFilePath] Setting filepath to: " << path << "\n";
    m_filePath = path;

    // Update document adapter if it exists
    if (m_document)
    {
        m_document->SetFilePath(path);
        SYSTEM_LOG << "[BehaviorTreeRenderer::SetFilePath] Document updated with filepath\n";
    }
    else
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::SetFilePath] WARNING: No document adapter available\n";
    }
}

void BehaviorTreeRenderer::AcceptNodeDrop(const std::string& nodeType, float screenX, float screenY)
{
    // Phase 60 FIX: Implement AcceptNodeDrop for drag-drop node creation
    // This method is called by the drag-drop target when user drops a node from the palette

    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] Node type: " << nodeType 
               << " at screen pos (" << screenX << ", " << screenY << ")\n";

    if (m_graphId < 0 || !NodeGraph::NodeGraphManager::IsValid())
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] ERROR: Invalid graph ID or destroyed singleton\n";
        return;
    }

    // Get the active graph
    GraphId id{static_cast<uint32_t>(m_graphId)};
    GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(id);
    if (!graphDoc)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] ERROR: Graph not found\n";
        return;
    }

    // PHASE 61 DIAGNOSTIC: Log graph state and pointer address
    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] DIAGNOSTIC: m_graphId=" << m_graphId 
               << " graphDoc=" << static_cast<void*>(graphDoc)
               << " nodeCount_before=" << graphDoc->GetNodes().size() << "\n";

    // Transform screen coordinates to canvas coordinates
    // The canvas screen position was saved during Render() in m_canvasScreenPos
    ImVec2 canvasPos = ImGui::GetWindowPos();
    ImVec2 mouseInCanvas(screenX - m_canvasScreenPos.x, screenY - m_canvasScreenPos.y);

    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] Canvas position: (" 
               << mouseInCanvas.x << ", " << mouseInCanvas.y << ")\n";

    // Get nodes vector and find next available ID
    std::vector<NodeData>& nodes = graphDoc->GetNodesRef();
    uint32_t maxNodeId = 0;
    for (const auto& node : nodes)
    {
        if (node.id.value > maxNodeId)
            maxNodeId = node.id.value;
    }
    uint32_t newNodeId = maxNodeId + 1;

    // Create new node
    NodeData newNode;
    newNode.id = NodeId{newNodeId};
    newNode.type = nodeType;
    newNode.name = nodeType + "_" + std::to_string(newNodeId);  // Generate unique name
    newNode.position.x = mouseInCanvas.x;
    newNode.position.y = mouseInCanvas.y;

    // Add to nodes vector
    nodes.push_back(newNode);

    // Mark graph as dirty so Save button becomes active
    graphDoc->SetDirty(true);

    // PHASE 61 DIAGNOSTIC: Verify node was added
    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] DIAGNOSTIC: nodeCount_after=" 
               << graphDoc->GetNodes().size() << " newNode.id=" << newNode.id.value 
               << " newNode.position=(" << newNode.position.x << "," << newNode.position.y << ")\n";

    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] SUCCESS: Created node #" << newNodeId
               << " (type=" << nodeType << ", name=" << newNode.name << ")\n";
}

void BehaviorTreeRenderer::HandleKeyboardShortcuts()
{
    // Phase 59 FIX: Minimal keyboard shortcut handler
    // TODO: Full implementation deferred to future phase
    // Keyboard shortcuts will be handled via framework toolbar integration
    // (toolbar already captures Ctrl+S via ImGui input system)
}

void BehaviorTreeRenderer::OnRunGraphClicked()
{
    // TODO: Reimplement with modern NodeGraphTypes schema
    // Old code used NodeGraph* graph with deprecated GetAllNodes, CreateNode methods
    SYSTEM_LOG << "[BehaviorTreeRenderer::OnRunGraphClicked] DEPRECATED - awaiting reimplementation\n";
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

// Phase 43: Framework modal rendering
void BehaviorTreeRenderer::RenderFrameworkModals()
{
    // Phase 43: Render framework modals (Save/SaveAs/Browse)
    // The CanvasFramework instance coordinates with CanvasToolbarRenderer
    // to render all framework-based modals.
    if (m_framework)
    {
        m_framework->RenderModals();
    }
}

} // namespace Olympe

