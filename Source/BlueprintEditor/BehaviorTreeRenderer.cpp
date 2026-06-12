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
#include "../PanelManager.h"  // Access to framework panel dimensions
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
    m_document->SetRenderer(this); // Phase 55: Bind this renderer wrapper to allow custom toolbar controls
    m_framework = std::make_unique<CanvasFramework>(m_document.get());

    // Initialize minimap renderer (Phase 70)
    m_minimap = std::make_unique<CanvasMinimapRenderer>();

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
    SYSTEM_LOG << "[BehaviorTreeRenderer::CreateNew] ENTER - Creating new graph: '" << name << "'\n";

    // Close any previously loaded graph to prevent resource leaks
    if (m_graphId >= 0 && NodeGraph::NodeGraphManager::IsValid())
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::CreateNew] Closing previous graph ID: " << m_graphId << "\n";
        NodeGraph::NodeGraphManager::Get().CloseGraph(GraphId{static_cast<uint32_t>(m_graphId)});
        m_graphId = -1;
        m_filePath.clear();
    }

    // Reset ImNodes adapter for new graph (Phase 54: Fix drag-drop on new BT creation)
    if (m_imNodesAdapter)
    {
        m_imNodesAdapter = nullptr; // Will be re-initialized in Render() with new graph ID
    }

    // Create new empty graph through singleton manager
    if (!NodeGraph::NodeGraphManager::IsValid())
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::CreateNew] ERROR: NodeGraphManager singleton is invalid!\n";
        return false;
    }

    GraphId newGraphId = NodeGraph::NodeGraphManager::Get().CreateGraph(name, "BehaviorTree");
    if (newGraphId.value == 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::CreateNew] ERROR: Failed to create new BehaviorTree graph via manager\n";
        return false;
    }

    m_graphId = static_cast<int>(newGraphId.value);
    
    // CRITICAL: Set the active graph immediately so any dependent systems (like palette/adapter) see it
    NodeGraph::NodeGraphManager::Get().SetActiveGraph(newGraphId);

    // Phase 50.1.1: Clear filepath for new unsaved graph
    if (m_document)
    {
        m_document->SetFilePath("");
        SYSTEM_LOG << "[BehaviorTreeRenderer::CreateNew] Document adapter updated with empty path\n";
    }

    SYSTEM_LOG << "[BehaviorTreeRenderer::CreateNew] SUCCESS - Created graph '" << name 
               << "' (ID=" << m_graphId << ")\n";
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
}

void BehaviorTreeRenderer::SetMinimapVisible(bool visible)
{
    m_minimapVisible = visible;
    if (m_canvasEditor)
        m_canvasEditor->SetMinimapVisible(visible);
}

void BehaviorTreeRenderer::SetMinimapSize(float size)
{
    m_minimapSize = size;
    if (m_canvasEditor)
        m_canvasEditor->SetMinimapSize(size);
}

void BehaviorTreeRenderer::SetMinimapPosition(int pos)
{
    m_minimapPosition = static_cast<MinimapPosition>(pos);
    if (m_canvasEditor)
        m_canvasEditor->SetMinimapPosition(pos);
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
    }

    ImGui::Separator();

    // Layout: Canvas (left) | Resize Handle | Tabbed Right Panel (right)
    // Use framework panel dimensions from PanelManager
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float handleWidth = 4.0f;
    float rightPanelWidth = static_cast<float>(PanelManager::InspectorPanelWidth);  // Framework default: 300px
    if (rightPanelWidth < 200.0f) rightPanelWidth = 200.0f;  // Minimum width
    if (rightPanelWidth > totalWidth * 0.6f) rightPanelWidth = totalWidth * 0.6f;  // Max 60% of total
    float canvasWidth = totalWidth - rightPanelWidth - handleWidth;

    ImVec2 regionMin = ImGui::GetCursorScreenPos();

    // Render canvas on the left with ImNodes adapter (Phase 50.3)
    ImGui::BeginChild("BTNodeCanvas", ImVec2(canvasWidth, 0), false, ImGuiWindowFlags_NoScrollbar);
    
    // Phase 61 FIX: Capture canvas screen position BEFORE rendering adapter
    // This position is required by AcceptNodeDrop for coordinate transformation.
    m_canvasScreenPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    // PHASE 70: Use standardized ImNodesCanvasEditor for framework parity
    if (!m_canvasEditor && m_graphId >= 0)
    {
        m_canvasEditor = std::make_unique<ImNodesCanvasEditor>("BehaviorTree", m_canvasScreenPos, canvasSize);
    }

    if (m_canvasEditor)
    {
        m_canvasEditor->SetCanvasScreenPos(m_canvasScreenPos);
        m_canvasEditor->SetCanvasSize(canvasSize);
    }

    // Initialize ImNodes adapter if needed
    if (!m_imNodesAdapter && m_graphId >= 0)
    {
        m_imNodesAdapter = std::make_unique<BehaviorTreeImNodesAdapter>();
        m_imNodesAdapter->Initialize(m_graphId);
    }

    // Render using ImNodes adapter
    if (m_imNodesAdapter && m_canvasEditor)
    {
        // LEGACY RESTORATION: Enable Zoom and Multiple Selection for BT
        // ImNodes supports zoom via its IO system.
        ImNodesIO& io = ImNodes::GetIO();
        io.EmulateThreeButtonMouse.Modifier = &ImGui::GetIO().KeyAlt; // Alt + Left to pan
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyAlt;
        // Multiple Select is enabled by default in ImNodes if no modifier is set, 
        // but we ensure it works with Ctrl for standard behavior
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        // PHASE 72 FIX: Do NOT wrap adapter->Render() with canvasEditor->BeginRender()/EndRender()
        // BehaviorTreeImNodesAdapter already manages its own ImNodes scope via BeginNodeEditor/EndNodeEditor.
        // Wrapping it causes double calls to BeginNodeEditor, resulting in:
        // "Assertion failed: GImNodes->CurrentScope == ImNodesScope_None"
        
        m_imNodesAdapter->Render([this]() {
            // Render minimap overlay INSIDE the node editor scope
            if (m_minimapVisible && m_minimap)
            {
                m_minimap->SetVisible(true);
                m_minimap->SetSize(m_minimapSize);
                m_minimap->SetPosition(m_minimapPosition);
                m_minimap->RenderImNodes();
            }
        });

        // PHASE 78: Sync selection from ImNodes to Property Panel
        int selectedNodeId = m_imNodesAdapter->GetSelectedNodeId();
        if (selectedNodeId != m_propertyPanel.m_selectedNodeId)
        {
            if (selectedNodeId != -1)
                m_propertyPanel.SetSelectedNode(m_graphId, selectedNodeId);
            else
                m_propertyPanel.ClearSelection();
        }
    }
    else if (m_imNodesAdapter)
    {
        // Fallback for safety if canvasEditor is missing
        m_imNodesAdapter->Render([this]() {
            // Render minimap overlay INSIDE the node editor scope
            if (m_minimapVisible && m_minimap)
            {
                m_minimap->SetVisible(true);
                m_minimap->SetSize(m_minimapSize);
                m_minimap->SetPosition(m_minimapPosition);
                m_minimap->RenderImNodes();
            }
        });
    }

    ImGui::EndChild();

    // PHASE 77 FIX: OVERLAY DRAG-DROP (Parity with EntityPrefab)
    // We create an invisible overlay over the canvas area to catch drag-drop events 
    // that ImNodes might otherwise consume or ignore.
    ImVec2 canvasEnd = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(m_canvasScreenPos);

    // Transparent dummy to act as drop target
    ImGui::Dummy(canvasSize); 

    // Visual Feedback (Yellow Highlight)
    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload && activePayload->IsDataType("BT_NODE_TYPE"))
    {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRect(m_canvasScreenPos, ImVec2(m_canvasScreenPos.x + canvasSize.x, m_canvasScreenPos.y + canvasSize.y), IM_COL32(255, 255, 0, 255), 0.0f, 0, 4.0f);
        }
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BT_NODE_TYPE"))
        {
            const char* nodeTypeStr = (const char*)payload->Data;
            ImVec2 mousePos = ImGui::GetMousePos();
            SYSTEM_LOG << "[BehaviorTreeRenderer] NODE DROP RECEIVED VIA OVERLAY: " << nodeTypeStr << "\n";
            AcceptNodeDrop(nodeTypeStr, mousePos.x, mousePos.y);
        }
        ImGui::EndDragDropTarget();
    }

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
        // Update framework panel width directly
        PanelManager::InspectorPanelWidth -= static_cast<int>(ImGui::GetIO().MouseDelta.x);
        if (PanelManager::InspectorPanelWidth < 200) PanelManager::InspectorPanelWidth = 200;
        if (PanelManager::InspectorPanelWidth > static_cast<int>(totalWidth * 0.6f))
            PanelManager::InspectorPanelWidth = static_cast<int>(totalWidth * 0.6f);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Render right panel with tabs
    ImGui::BeginChild("BTRightPanel", ImVec2(rightPanelWidth, 0), true);
    RenderRightPanelTabs();
    ImGui::EndChild();

    // Handle keyboard shortcuts for copy/paste/duplicate
    HandleKeyboardShortcuts();

    // PHASE 78: Render context menu
    RenderContextMenu();
}

void BehaviorTreeRenderer::RenderContextMenu()
{
    if (!m_imNodesAdapter) return;

    int selectedNodeId = m_imNodesAdapter->GetSelectedNodeId();

    // LEGACY RESTORATION: Use popup triggered by right click anywhere or on node
    if (ImGui::BeginPopupContextWindow("BT_Canvas_Context_Menu"))
    {
        if (selectedNodeId != -1)
        {
            ImGui::Text("Node Actions (#%d)", selectedNodeId);
            ImGui::Separator();

            if (ImGui::MenuItem("Set as Root"))
            {
                auto& manager = NodeGraph::NodeGraphManager::Get();
                NodeGraphTypes::GraphDocument* graphDoc = manager.GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
                if (graphDoc)
                {
                    // LEGACY RESTORATION: Set root node ID in metadata
                    graphDoc->metadata["rootNodeId"] = selectedNodeId;
                    graphDoc->rootNodeId = NodeGraphTypes::NodeId{ static_cast<uint32_t>(selectedNodeId) };
                    graphDoc->SetDirty(true);
                }
            }

            // LEGACY RESTORATION: Breakpoint and Node State
            auto* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
            auto* node = graphDoc ? graphDoc->GetNode(NodeGraphTypes::NodeId{ static_cast<uint32_t>(selectedNodeId) }) : nullptr;

            if (node)
            {
                bool isBreakpoint = node->parameters.count("breakpoint") && node->parameters.at("breakpoint") == "true";
                if (ImGui::MenuItem("Toggle Breakpoint", "B", isBreakpoint))
                {
                    node->parameters["breakpoint"] = isBreakpoint ? "false" : "true";
                    graphDoc->SetDirty(true);
                }

                bool isDisabled = node->parameters.count("disabled") && node->parameters.at("disabled") == "true";
                if (ImGui::MenuItem("Disable Node", nullptr, isDisabled))
                {
                    node->parameters["disabled"] = isDisabled ? "false" : "true";
                    graphDoc->SetDirty(true);
                }
            }

            if (ImGui::BeginMenu("Reorder Children"))
            {
                ImGui::MenuItem("Move Left / Up (stub)");
                ImGui::MenuItem("Move Right / Down (stub)");
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Delete Node", "Del"))
            {
                auto& manager = NodeGraph::NodeGraphManager::Get();
                NodeGraphTypes::GraphDocument* graphDoc = manager.GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
                if (graphDoc)
                {
                    graphDoc->DeleteNode(NodeGraphTypes::NodeId{ static_cast<uint32_t>(selectedNodeId) });
                    m_propertyPanel.ClearSelection();
                    graphDoc->SetDirty(true);
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Canvas Actions");
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A"))
            {
                auto* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
                if (graphDoc)
                {
                    for (const auto& node : graphDoc->GetNodes())
                        ImNodes::SelectNode(static_cast<int>(node.id.value));
                }
            }
            if (ImGui::MenuItem("Reset View"))
            {
                ImNodes::EditorContextResetPanning(ImVec2(0, 0));
            }
        }
        ImGui::EndPopup();
    }
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

    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] ENTRY - nodeType: " << nodeType << "\n";

    if (m_graphId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] ERROR: Invalid graph ID (m_graphId=" << m_graphId << ")\n";
        return;
    }

    if (!NodeGraph::NodeGraphManager::IsValid())
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] ERROR: NodeGraphManager singleton is invalid!\n";
        return;
    }

    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] Checkpoint 1: Coordinate transformation start\n";

    // PHASE 78 FIX: Direct coordinate calculation to avoid crash in m_canvasEditor->ScreenToCanvas()
    // m_canvasEditor->ScreenToCanvas calls ImNodes::EditorContextGetPanning(), which CRASHES 
    // if called when the BT ImNodes context is not current or outside of its render scope.

    ImVec2 mouseInCanvas;

    // 1. Get relative position to canvas
    float relX = screenX - m_canvasScreenPos.x;
    float relY = screenY - m_canvasScreenPos.y;

    // 2. Adjust for Panning (from ImNodes adapter if available)
    ImVec2 pan = { 0, 0 };
    if (m_imNodesAdapter)
    {
        // We use the adapter's context awareness if possible
        // But to be even safer, we'll try-catch or check existence in memory
        // For now, simpler: retrieve pan through the adapter which holds the context
        pan = m_imNodesAdapter->GetPanning(); 
    }

    mouseInCanvas.x = relX - pan.x;
    mouseInCanvas.y = relY - pan.y;

    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] Checkpoint 2: Node creation at canvas-logical (" 
               << mouseInCanvas.x << ", " << mouseInCanvas.y << ") [Pan: " << pan.x << "," << pan.y << "]\n";

    // Get the active graph
    GraphId id{static_cast<uint32_t>(m_graphId)};
    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] Checkpoint 3: Fetching graph document for ID " << m_graphId << "\n";

    GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(id);
    if (!graphDoc)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] ERROR: Graph not found for ID " << m_graphId << "\n";
        return;
    }

    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] Checkpoint 4: Accessing nodes vector (graphDoc=" << static_cast<void*>(graphDoc) << ")\n";

    // PHASE 61 DIAGNOSTIC: Log graph state and pointer address
    SYSTEM_LOG << "[BehaviorTreeRenderer::AcceptNodeDrop] DIAGNOSTIC: nodeCount_before=" << graphDoc->GetNodes().size() << "\n";

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
    // PHASE 81: Implement editor keyboard shortcuts (Delete, Select All)
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) return;

    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        if (m_imNodesAdapter)
        {
            int selectedNodeId = m_imNodesAdapter->GetSelectedNodeId();
            if (selectedNodeId != -1)
            {
                // Retrieve the active graph
                NodeGraphTypes::GraphId id{ static_cast<uint32_t>(m_graphId) };
                NodeGraphTypes::GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(id);
                if (graphDoc)
                {
                    if (graphDoc->DeleteNode(NodeGraphTypes::NodeId{ static_cast<uint32_t>(selectedNodeId) }))
                    {
                        SYSTEM_LOG << "[BehaviorTreeRenderer] Deleted node #" << selectedNodeId << "\n";
                        m_propertyPanel.ClearSelection();
                        graphDoc->SetDirty(true);
                    }
                }
            }
        }
    }

    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_A))
        {
            // LEGACY RESTORATION: Select All logic
            auto* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
            if (graphDoc)
            {
                const auto& nodes = graphDoc->GetNodes();
                for (const auto& node : nodes)
                {
                    ImNodes::SelectNode(static_cast<int>(node.id.value));
                }
            }
        }
    }
}

void BehaviorTreeRenderer::OnVerifyGraphClicked()
{
    SYSTEM_LOG << "[BehaviorTreeRenderer::OnVerifyGraphClicked] Running verification...\n";
    // Si la vérification est supportée à l'avenir pour BT, l'intégrer ici.
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

