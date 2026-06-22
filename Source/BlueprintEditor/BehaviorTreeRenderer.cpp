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
// BehaviorTreeExecutor is provided by the editor runtime; do NOT include its .cpp here
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

BehaviorTreeAsset BehaviorTreeRenderer::ExportBehaviorTreeAsset() const
{
    BehaviorTreeAsset asset;
    if (m_graphId < 0)
        return asset;

    // Build BehaviorTreeAsset from NodeGraph::GraphDocument
    GraphId id{static_cast<uint32_t>(m_graphId)};
    GraphDocument* doc = NodeGraph::NodeGraphManager::Get().GetGraph(id);
    if (!doc)
        return asset;

    // Convert GraphDocument nodes to BehaviorTree nodes
    asset.name = doc->type + "_converted";
    // Grab nodes once and determine root node id: prefer explicit document root, otherwise infer from a BT_Root node
    const auto& nodes = doc->GetNodes();
    const auto& links = doc->GetLinks();
    int docRoot = doc->GetRootNodeId();
    if (docRoot == 0)
    {
        // Try to infer root: look for a node whose type contains 'Root' (e.g. 'BT_Root')
        for (const auto& n : nodes)
        {
            if (n.type.find("Root") != std::string::npos)
            {
                docRoot = static_cast<int>(n.id.value);
                SYSTEM_LOG << "[BehaviorTreeRenderer::ExportBehaviorTreeAsset] Inferred root node ID=" << docRoot << " from node type='" << n.type << "'\n";
                break;
            }
        }
    }

    asset.rootNodeId = static_cast<uint32_t>(docRoot);
    for (const auto& n : nodes)
    {
        BTNode bn;
        bn.id = static_cast<uint32_t>(n.id.value);
        bn.name = n.name;
        // Map type string heuristics
        if (n.type.find("Selector") != std::string::npos) bn.type = BTNodeType::Selector;
        else if (n.type.find("Sequence") != std::string::npos) bn.type = BTNodeType::Sequence;
        else if (n.type.find("Condition") != std::string::npos) bn.type = BTNodeType::Condition;
        else if (n.type.find("Action") != std::string::npos) bn.type = BTNodeType::Action;
        else if (n.type.find("Root") != std::string::npos) bn.type = BTNodeType::Root;
        else bn.type = BTNodeType::Action;

        bn.editorPosX = n.position.x;
        bn.editorPosY = n.position.y;

        // children: convert NodeId vector to uint32_t ids
        // We'll populate children from graph links below; keep any explicit children as fallback
        for (const auto& c : n.children)
        {
            bn.childIds.push_back(static_cast<uint32_t>(c.value));
        }

        // If node is an OnEvent root, record it
        if (n.type.find("OnEvent") != std::string::npos || n.type.find("On_Event") != std::string::npos)
        {
            asset.m_eventRootIds.push_back(bn.id);
        }

        asset.nodes.push_back(bn);
    }

    // Build child relationships from links (fromPin -> toPin) but be resilient to link orientation
    if (!links.empty())
    {
        // Map node id -> index in asset.nodes
        std::unordered_map<uint32_t, size_t> idIndex;
        for (size_t i = 0; i < asset.nodes.size(); ++i) idIndex[asset.nodes[i].id] = i;

        // Forward adjacency (from -> to)
        std::unordered_map<uint32_t, std::vector<uint32_t>> forwardAdj;
        std::unordered_map<uint32_t, std::vector<uint32_t>> reverseAdj;
        // Also record link ordering per source node so we can preserve explicit
        // link priority/index as authored in the editor (visible small numbers on links).
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, int>> linkOrder;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, int>> reverseLinkOrder;
        // Group links by source so we can sort siblings consistently by editor Y pos
        std::unordered_map<uint32_t, std::vector<uint32_t>> groupedBySource;
        std::unordered_map<uint32_t, std::vector<uint32_t>> groupedByTarget;
        for (const auto& l : links)
        {
            uint32_t fromNode = static_cast<uint32_t>(l.fromPin.value);
            uint32_t toNode = static_cast<uint32_t>(l.toPin.value);
            groupedBySource[fromNode].push_back(toNode);
            groupedByTarget[toNode].push_back(fromNode);
        }

        // For each source, sort its outgoing targets by the target node editor Y position
        for (auto& kv : groupedBySource)
        {
            uint32_t src = kv.first;
            auto& vec = kv.second;
            std::sort(vec.begin(), vec.end(), [&](uint32_t a, uint32_t b){
                const BTNode* na = asset.GetNode(a);
                const BTNode* nb = asset.GetNode(b);
                if (!na || !nb) return a < b;
                if (na->editorPosY != nb->editorPosY) return na->editorPosY < nb->editorPosY;
                return na->editorPosX < nb->editorPosX;
            });
            // populate forwardAdj and linkOrder from sorted vector
            for (size_t i = 0; i < vec.size(); ++i)
            {
                uint32_t toNode = vec[i];
                forwardAdj[src].push_back(toNode);
                linkOrder[src][toNode] = static_cast<int>(i);
            }
        }

        // For reverse adjacency (to -> from) sort parents by parent's editor Y
        for (auto& kv : groupedByTarget)
        {
            uint32_t tgt = kv.first;
            auto& vec = kv.second;
            std::sort(vec.begin(), vec.end(), [&](uint32_t a, uint32_t b){
                const BTNode* na = asset.GetNode(a);
                const BTNode* nb = asset.GetNode(b);
                if (!na || !nb) return a < b;
                if (na->editorPosY != nb->editorPosY) return na->editorPosY < nb->editorPosY;
                return na->editorPosX < nb->editorPosX;
            });
            for (size_t i = 0; i < vec.size(); ++i)
            {
                uint32_t fromNode = vec[i];
                reverseAdj[tgt].push_back(fromNode);
                reverseLinkOrder[tgt][fromNode] = static_cast<int>(i);
            }
        }

        auto applyAdj = [&](const std::unordered_map<uint32_t, std::vector<uint32_t>>& adj)
        {
            // Clear existing children then populate
            for (auto& bn : asset.nodes) bn.childIds.clear();
            for (const auto& kv : adj)
            {
                auto it = idIndex.find(kv.first);
                if (it == idIndex.end()) continue;
                auto& childVec = asset.nodes[it->second].childIds;
                for (uint32_t cid : kv.second)
                {
                    if (std::find(childVec.begin(), childVec.end(), cid) == childVec.end())
                        childVec.push_back(cid);
                }
            }

            // Sort children by editor Y position
            for (auto& bn : asset.nodes)
            {
                if (bn.childIds.size() <= 1) continue;
                std::sort(bn.childIds.begin(), bn.childIds.end(), [&](uint32_t a, uint32_t b)
                {
                    // Prefer explicit link order if available (editor-assigned child indices)
                    // Prefer explicit link order if available. Check both forward and reverse mappings
                    auto lof = linkOrder.find(bn.id);
                    if (lof != linkOrder.end())
                    {
                        auto &mapRef = lof->second;
                        auto ita = mapRef.find(a);
                        auto itb = mapRef.find(b);
                        if (ita != mapRef.end() && itb != mapRef.end())
                        {
                            return ita->second < itb->second;
                        }
                        if (ita != mapRef.end()) return true;
                        if (itb != mapRef.end()) return false;
                    }
                    auto rlo = reverseLinkOrder.find(bn.id);
                    if (rlo != reverseLinkOrder.end())
                    {
                        auto &mapRef = rlo->second;
                        auto ita = mapRef.find(a);
                        auto itb = mapRef.find(b);
                        if (ita != mapRef.end() && itb != mapRef.end())
                        {
                            return ita->second < itb->second;
                        }
                        if (ita != mapRef.end()) return true;
                        if (itb != mapRef.end()) return false;
                    }

                    const BTNode* na = asset.GetNode(a);
                    const BTNode* nb = asset.GetNode(b);
                    if (!na || !nb) return a < b;
                    return na->editorPosY < nb->editorPosY;
                });
            }
        };

        // Apply forward adjacency first
        applyAdj(forwardAdj);

        // If forward produced almost no connectivity (root has no children), try reverse orientation
        bool rootHasChildren = false;
        if (asset.rootNodeId != 0)
        {
            const BTNode* r = asset.GetNode(asset.rootNodeId);
            if (r && !r->childIds.empty()) rootHasChildren = true;
        }
        if (!rootHasChildren)
        {
            // Compute reachable count from root using current child lists
            auto reachableCount = [&]() -> size_t
            {
                if (asset.rootNodeId == 0) return 0;
                std::set<uint32_t> visited;
                std::vector<uint32_t> stack;
                stack.push_back(asset.rootNodeId);
                while (!stack.empty())
                {
                    uint32_t cur = stack.back(); stack.pop_back();
                    if (visited.count(cur)) continue;
                    visited.insert(cur);
                    const BTNode* curNode = asset.GetNode(cur);
                    if (!curNode) continue;
                    for (uint32_t c : curNode->childIds) stack.push_back(c);
                }
                return visited.size();
            }();

            if (reachableCount <= 1)
            {
                // Try reverse orientation
                SYSTEM_LOG << "[BehaviorTreeRenderer::ExportBehaviorTreeAsset] Forward links produced no children for root, attempting reversed orientation\n";
                applyAdj(reverseAdj);
            }
        }
    }

    // Spatial heuristic fallback: if root still has no children, pick nearest node to the right as child
    if (asset.rootNodeId != 0)
    {
        BTNode* rootBn = asset.GetNode(asset.rootNodeId);
        if (rootBn && rootBn->childIds.empty())
        {
            float bestDist = std::numeric_limits<float>::infinity();
            uint32_t bestId = 0;
            for (const auto& cand : asset.nodes)
            {
                if (cand.id == rootBn->id) continue;
                // Candidate should be to the right (higher X) and roughly same Y band
                if (cand.editorPosX <= rootBn->editorPosX) continue;
                float dx = cand.editorPosX - rootBn->editorPosX;
                float dy = cand.editorPosY - rootBn->editorPosY;
                float dist = sqrt(dx*dx + dy*dy);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestId = cand.id;
                }
            }
            if (bestId != 0)
            {
                SYSTEM_LOG << "[BehaviorTreeRenderer::ExportBehaviorTreeAsset] Spatial fallback: linking root " << rootBn->id << " -> " << bestId << "\n";
                rootBn->childIds.push_back(bestId);
            }
        }
    }

    // If root has no children after the forward link pass, try reversed link orientation
    if (asset.rootNodeId != 0)
    {
        const BTNode* rootNode = asset.GetNode(asset.rootNodeId);
        if (rootNode && rootNode->childIds.empty())
        {
            // Check if any link points TO the root (indicating reversed orientation)
            // Note: l.toPin is a pin id; convert to node id using pin/node scheme (pin = nodeId*2)
            bool foundIncomingToRoot = false;
            for (const auto& l : links)
            {
                uint32_t toNode = static_cast<uint32_t>(l.toPin.value) / 2u;
                if (toNode == asset.rootNodeId) { foundIncomingToRoot = true; break; }
            }

            if (foundIncomingToRoot)
            {
                SYSTEM_LOG << "[BehaviorTreeRenderer::ExportBehaviorTreeAsset] Detected reversed link orientation; rebuilding child lists (to -> from)\n";
                // Clear existing children
                for (auto& bn : asset.nodes) bn.childIds.clear();

                // Rebuild: parent = toPin, child = fromPin
                std::unordered_map<uint32_t, size_t> idIndex2;
                for (size_t i = 0; i < asset.nodes.size(); ++i) idIndex2[asset.nodes[i].id] = i;
                for (const auto& l : links)
                {
                    uint32_t parentId = static_cast<uint32_t>(l.toPin.value) / 2u;
                    uint32_t childId = static_cast<uint32_t>(l.fromPin.value) / 2u;
                    auto itP = idIndex2.find(parentId);
                    if (itP != idIndex2.end())
                    {
                        auto& childVec = asset.nodes[itP->second].childIds;
                        if (std::find(childVec.begin(), childVec.end(), childId) == childVec.end())
                            childVec.push_back(childId);
                    }
                }

                // Resort children by Y
                for (auto& bn : asset.nodes)
                {
                    if (bn.childIds.size() <= 1) continue;
                    std::sort(bn.childIds.begin(), bn.childIds.end(), [&](uint32_t a, uint32_t b)
                    {
                        const BTNode* na = asset.GetNode(a);
                        const BTNode* nb = asset.GetNode(b);
                        if (!na || !nb) return a < b;
                        return na->editorPosY < nb->editorPosY;
                    });
                }
            }
        }
    }

    return asset;
}

void BehaviorTreeRenderer::RenderVerificationLogsPanel()
{
    // Mirror VisualScriptEditorPanel::RenderVerificationLogsPanel behaviour but using m_verificationLogs
    if (!m_verificationDone)
    {
        ImGui::TextDisabled("(Click 'Verify' or 'Run' button to generate output)");
        return;
    }

    // Status line
    bool hasErrors = false;
    for (const auto& l : m_verificationLogs) if (l.find("[ERROR]") != std::string::npos) { hasErrors = true; break; }

    if (hasErrors)
        ImGui::TextColored(ImVec4(1.0f,0.3f,0.3f,1.0f), "[ERROR] Graph has errors");
    else if (!m_verificationLogs.empty())
        ImGui::TextColored(ImVec4(0.3f,1.0f,0.3f,1.0f), "[OK] Graph validated (warnings/info may exist)");
    else
        ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1.0f), "[OK] No validation logs");

    ImGui::Separator();

    ImGui::BeginChild("BehaviorTreeVerificationLogsChild", ImVec2(0,0), true);
    if (m_verificationLogs.empty())
    {
        ImGui::TextDisabled("(No logs to display)");
    }
    else
    {
        for (const auto& line : m_verificationLogs)
        {
            ImVec4 col = ImVec4(0.9f,0.9f,0.9f,1.0f);
            if (line.find("[ERROR]") != std::string::npos) col = ImVec4(1.0f,0.3f,0.3f,1.0f);
            else if (line.find("[WARN]") != std::string::npos) col = ImVec4(1.0f,0.85f,0.0f,1.0f);
            else if (line.find("[SIMULATION]") != std::string::npos) col = ImVec4(0.4f,1.0f,0.4f,1.0f);
            ImGui::TextColored(col, "%s", line.c_str());
        }
    }
    ImGui::EndChild();
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

    // If verify modal requested, render a simple modal using ImGui
    // Migrate BT validation/simulation messages into the unified Verification Output
    if (m_verificationDone && !m_verificationLogs.empty())
    {
        // nothing here - logs rendered via RenderVerificationLogsPanel()
    }

    if (m_showRunModal)
    {
        m_showRunModal = false;
        ImGui::OpenPopup("BT Run");
    }

    if (ImGui::BeginPopupModal("BT Run", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("BehaviorTree Run not available: could not resolve asset or adapter failed.");
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Render verification logs panel area in the left column when active
    // The main GUI container (BlueprintEditorGUI) expects RenderVerificationLogsPanel on the renderer
    // Provide the implementation here for BehaviorTreeRenderer so logs appear in the left panel
    // Note: The container will call the renderer's RenderVerificationLogsPanel() via dynamic dispatch

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
        // Use GetSelectedNodes to be robust when multiple selection is enabled
        std::vector<int> selNodes = m_imNodesAdapter->GetSelectedNodes();
        int selectedNodeId = -1;
        if (!selNodes.empty())
        {
            // Convert ImNodes uid to canonical NodeId before using with GraphDocument
            selectedNodeId = m_imNodesAdapter->GetCanonicalNodeIdFromUid(selNodes[0]);
        }
        if (selectedNodeId != m_propertyPanel.m_selectedNodeId)
        {
            if (selectedNodeId != -1)
            {
                // Only set selection if node id is valid and exists in graph (guard against stale/invalid ids)
                NodeGraphTypes::GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(NodeGraphTypes::GraphId{static_cast<uint32_t>(m_graphId)});
                if (graphDoc && graphDoc->GetNode(NodeGraphTypes::NodeId{static_cast<uint32_t>(selectedNodeId)}))
                {
                    m_propertyPanel.SetSelectedNode(m_graphId, selectedNodeId);
                    // Auto-switch right panel to Properties when a node is selected
                    // but mark transient flag so user can explicitly switch back to Palette
                    m_rightPanelTabSelection = 1;
                    m_forceOpenPropertiesOnce = true;
                }
                else
                {
                    // Invalid id detected; clear selection to avoid log spam
                    m_propertyPanel.ClearSelection();
                }
            }
            else
            {
                m_propertyPanel.ClearSelection();
            }
        }

        // PHASE 86: Handle ImNodes context menu trigger (right-click on node/link/editor)
        // Only open popup when the user actually right-clicked to avoid spurious opens
        int hoveredNode = -1, hoveredLink = -1;
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            if (m_imNodesAdapter->HandleContextMenuTrigger(&hoveredNode, &hoveredLink))
            {
                // Cache hovered ids into renderer transient members so RenderContextMenu
                // can act on the hovered link even though ImNodes selection may change.
                m_contextHoveredNode = hoveredNode;
                m_contextHoveredLink = hoveredLink;

                // Open the canvas popup used by this renderer
                ImGui::OpenPopup("BT_Canvas_Context_Menu");
                SYSTEM_LOG << "[BehaviorTreeRenderer] Opened BT_Canvas_Context_Menu (hoveredNode=" << hoveredNode << ", hoveredLink=" << hoveredLink << ")\n";
            }
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

        // Render context menu immediately inside the same child/window scope
        // so BeginPopup can resolve the OpenPopup() call made above.
        RenderContextMenu();

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

    // LEGACY RESTORATION: Popup opened explicitly via ImGui::OpenPopup("BT_Canvas_Context_Menu")
    // Use BeginPopup to match the explicit OpenPopup call performed on right-click.
    if (ImGui::BeginPopup("BT_Canvas_Context_Menu"))
    {
        // Use cached hovered ids (set at popup open) so we can act on a hovered link
        // even if ImNodes selection changes after HandleContextMenuTrigger returned.
        int popupNodeId = m_contextHoveredNode != -1 ? m_contextHoveredNode : selectedNodeId;
        int popupLinkId = m_contextHoveredLink;

        if (popupNodeId != -1)
        {
            ImGui::Text("Node Actions (#%d)", popupNodeId);
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
                    int canonicalId = m_imNodesAdapter->GetCanonicalNodeIdFromUid(popupNodeId);
                    SYSTEM_LOG << "[BehaviorTreeRenderer] Delete Node requested popupUid=" << popupNodeId << " canonicalId=" << canonicalId << std::endl;
                    if (canonicalId != -1)
                    {
                        bool ok = graphDoc->DeleteNode(NodeGraphTypes::NodeId{ static_cast<uint32_t>(canonicalId) });
                        SYSTEM_LOG << "[BehaviorTreeRenderer] DeleteNode result=" << ok << " for canonicalId=" << canonicalId << std::endl;
                    }
                    else
                    {
                        SYSTEM_LOG << "[BehaviorTreeRenderer] WARNING: DeleteNode failed - could not map popupUid=" << popupNodeId << " to canonical id" << std::endl;
                    }
                    m_propertyPanel.ClearSelection();
                    graphDoc->SetDirty(true);
                }
            }
        }
        else if (popupLinkId != -1)
        {
            ImGui::Text("Link Actions (#%d)", popupLinkId);
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Link", "Del"))
            {
                auto& manager = NodeGraph::NodeGraphManager::Get();
                NodeGraphTypes::GraphDocument* graphDoc = manager.GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
                if (graphDoc)
                {
                    // ImNodes link UIDs map to index in graphDoc->GetLinks() (we used linkUid = i when drawing)
                    // To be safe, check bounds and then call DisconnectLink using the LinkId stored in the document.
                    const auto& links = graphDoc->GetLinks();
                    if (popupLinkId >= 0 && static_cast<size_t>(popupLinkId) < links.size())
                    {
                        NodeGraphTypes::LinkId lid{ static_cast<uint32_t>(links[popupLinkId].id.value) };
                        graphDoc->DisconnectLink(lid);
                        graphDoc->SetDirty(true);
                    }
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
                {
                    int uid = m_imNodesAdapter->GetUidFromCanonicalNodeId(static_cast<uint32_t>(node.id.value));
                    if (uid != -1) ImNodes::SelectNode(uid);
                }
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
            // If user explicitly opened Palette, clear transient auto-open flag so they keep control
            m_forceOpenPropertiesOnce = false;
            ImGui::EndTabItem();
        }

        // Tab 1: Node Properties
        {
            ImGuiTabItemFlags propsFlags = ImGuiTabItemFlags_None;
            // If we just auto-switched due to selection, hint ImGui to open Properties once
            if (m_forceOpenPropertiesOnce) propsFlags |= ImGuiTabItemFlags_SetSelected;

            if (ImGui::BeginTabItem("Properties", nullptr, propsFlags))
            {
                // Ensure property panel selection is synced with adapter if needed
                if (!m_propertyPanel.HasSelectedNode())
                {
                    // Try to source selection from ImNodes adapter as fallback
                    if (m_imNodesAdapter && m_graphId >= 0)
                    {
                        int sel = m_imNodesAdapter->GetSelectedNodeId();
                        if (sel >= 0)
                        {
                            m_propertyPanel.SetSelectedNode(m_graphId, sel);
                        }
                    }
                }

                // Render property panel (will display "No node selected" when appropriate)
                m_propertyPanel.Render();
                ImGui::EndTabItem();
            }
            else
            {
                // If tab is not opened and there is no selection, ensure panel is cleared
                if (!m_propertyPanel.HasSelectedNode())
                {
                    m_propertyPanel.ClearSelection();
                }
            }
            // If we had auto-opened properties due to selection, allow user to return to Palette
            if (m_forceOpenPropertiesOnce && !m_propertyPanel.HasSelectedNode())
            {
                // selection cleared, reset transient flag
                m_forceOpenPropertiesOnce = false;
            }
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

    // Phase 92 FIX: Ensure the ImNodes adapter re-applies persisted node positions
    // for this newly loaded graph. Without reinitializing the adapter cache, a tab
    // switch can reuse stale "initialized" state and leave nodes collapsed at the
    // origin or with the previous tab's layout.
    if (m_imNodesAdapter)
    {
        m_imNodesAdapter->Initialize(m_graphId);
    }

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
    SYSTEM_LOG << "[BehaviorTreeRenderer::OnVerifyGraphClicked] Running verification for BehaviorTree...\n";

    if (m_graphId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::OnVerifyGraphClicked] No graph loaded - aborting verification\n";
        return;
    }

    auto* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
    if (!graphDoc)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::OnVerifyGraphClicked] ERROR: GraphDocument not found\n";
        return;
    }

    // Run BT validator and copy messages into verification logs for the left panel
    auto messages = AI::BTGraphValidator::ValidateGraph(graphDoc);
    m_verificationLogs.clear();
    for (const auto& m : messages)
    {
        std::string prefix = (m.severity == AI::BTValidationSeverity::Error) ? "[ERROR] " :
                             (m.severity == AI::BTValidationSeverity::Warning) ? "[WARN] " : "[INFO] ";
        std::string line = prefix + m.message;
        if (m.nodeId != 0) line += " (Node: " + std::to_string(m.nodeId) + ")";
        m_verificationLogs.push_back(line);
    }
    m_verificationDone = true;
    SYSTEM_LOG << "[BehaviorTreeRenderer::OnVerifyGraphClicked] Validation produced " << m_verificationLogs.size() << " log lines\n";
}

void BehaviorTreeRenderer::OnRunGraphClicked()
{
    SYSTEM_LOG << "[BehaviorTreeRenderer::OnRunGraphClicked] Run requested - starting adapter+simulator pipeline\n";

    if (m_graphId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::OnRunGraphClicked] No graph loaded - aborting run\n";
        return;
    }

    // Get the BehaviorTree asset from the NodeGraphManager / document
    auto* graphDoc = NodeGraph::NodeGraphManager::Get().GetGraph(NodeGraphTypes::GraphId{ static_cast<uint32_t>(m_graphId) });
    if (!graphDoc)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::OnRunGraphClicked] ERROR: GraphDocument not found\n";
        return;
    }

    // Extract BehaviorTreeAsset via the document adapter if available
    BehaviorTreeAsset btAsset;
    bool haveAsset = false;
    // Export BehaviorTreeAsset from this renderer (build from GraphDocument)
    btAsset = ExportBehaviorTreeAsset();
    if (btAsset.nodes.empty() && btAsset.rootNodeId == 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer::OnRunGraphClicked] ERROR: Unable to obtain BehaviorTree asset for simulation\n";
        m_verificationLogs.clear();
        m_verificationLogs.push_back("[ERROR] Unable to obtain BehaviorTree asset for simulation");
        m_verificationDone = true;
        return;
    }

    // Execute BehaviorTree natively using BehaviorTreeExecutor (no adapter required)
    m_lastTracer->Reset();
    m_executionTestPanel->SetVisible(true);

    BehaviorTreeExecutor executor;
    BTStatus execStatus = executor.ExecuteTree(btAsset, *m_lastTracer);

    // Simple BT-specific formatting of tracer (indent by depth, add status symbols)
    m_verificationLogs.clear();
    const auto& events = m_lastTracer->GetEvents();
    if (events.empty())
    {
        m_verificationLogs.push_back("[SIMULATION] [No execution events recorded]");
    }
    else
    {
        for (const auto& ev : events)
        {
            const BTNode* node = btAsset.GetNode(static_cast<uint32_t>(ev.nodeId));
            if (!node) continue;
            int32_t depth = 0;
            // compute depth via parent walk (simple, safe approach)
            uint32_t current = static_cast<uint32_t>(ev.nodeId);
            while (true)
            {
                uint32_t parent = 0;
                for (const auto& n : btAsset.nodes)
                {
                    for (auto cid : n.childIds)
                    {
                        if (cid == current) { parent = n.id; break; }
                    }
                    if (parent != 0) break;
                }
                if (parent == 0) break;
                depth++;
                current = parent;
            }

            std::string indent(depth * 2, ' ');
            std::string statusSymbol = "  ";
            if (ev.message.find("SUCCESS") != std::string::npos) statusSymbol = "✓ ";
            else if (ev.message.find("FAILURE") != std::string::npos) statusSymbol = "✗ ";
            else if (ev.message.find("RUNNING") != std::string::npos) statusSymbol = "⊙ ";

            std::ostringstream line;
            line << "[SIMULATION] " << indent << statusSymbol << node->name;
            if (!ev.message.empty()) line << " → " << ev.message;
            m_verificationLogs.push_back(line.str());
        }
    }

    // Push simple summary
    {
        std::ostringstream os;
        os << "[SIMULATION SUMMARY] Executed nodes=" << m_lastTracer->GetEvents().size() << " Status=" << static_cast<int>(execStatus);
        m_verificationLogs.push_back(os.str());
    }

    m_verificationDone = true;

    // Populate ExecutionTestPanel display with the tracer for detailed table view
    m_executionTestPanel->DisplayTrace(*m_lastTracer);
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

