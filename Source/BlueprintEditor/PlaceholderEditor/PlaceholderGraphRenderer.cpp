#include "PlaceholderGraphRenderer.h"
#include "PlaceholderCanvas.h"
#include "../../system/system_utils.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace Olympe {

// Forward declarations for Phase 70-71 helper functions
std::vector<int> GetOrphanedNodes(const PlaceholderGraphDocument* document);
std::vector<int> GetRootNodes(const PlaceholderGraphDocument* document);
std::vector<int> GetLeafNodes(const PlaceholderGraphDocument* document);
bool TraverseNode(
    const PlaceholderGraphDocument* document,
    int nodeId,
    std::vector<int>& visited,
    std::vector<int>& path,
    int depth = 0);

PlaceholderGraphRenderer::PlaceholderGraphRenderer()
    : GraphEditorBase("PlaceholderEditor"), m_isLoading(false)
{
    // Initialize base members to nullptr
    m_document = nullptr;
    m_canvas = nullptr;
    std::cout << "[PlaceholderGraphRenderer] Created" << std::endl;
}

PlaceholderGraphRenderer::~PlaceholderGraphRenderer()
{
    // PHASE 72 FIX: Avoid calling LogAction in destructor
    // Clean up canvas (owned by this subclass)
    m_ownedCanvas.reset();
    m_canvas = nullptr; // Clear base member

    // Document is also owned as unique_ptr, will auto-delete
    m_ownedDocument.reset();
    m_document = nullptr; // Clear base member

    std::cout << "[PlaceholderGraphRenderer] Destroyed" << std::endl;
}

std::string PlaceholderGraphRenderer::GetGraphType() const
{
    return "Placeholder";
}

bool PlaceholderGraphRenderer::Load(const std::string& filePath)
{
    std::cout << "[PlaceholderGraphRenderer] Loading: " << filePath << std::endl;

    m_isLoading = true;

    // Create document if needed
    if (!m_ownedDocument)
    {
        m_ownedDocument = std::make_unique<PlaceholderGraphDocument>();
        m_document = (void*)m_ownedDocument.get(); // Set base member
    }

    // Load from file
    bool success = m_ownedDocument->Load(filePath);
    if (!success)
    {
        m_isLoading = false;
        return false;
    }

    // Initialize canvas (Phase 52 pattern: Initialize() must create all dependencies)
    if (!m_ownedCanvas)
    {
        m_ownedCanvas = std::make_unique<PlaceholderCanvas>();
        m_canvas = (void*)m_ownedCanvas.get(); // Set base member
    }
    m_ownedCanvas->Initialize(m_ownedDocument.get());
    m_ownedCanvas->SetRenderer(this);  // Phase 63.2: Set renderer reference for selection sync

    // Phase 4 Step 5 FIX: Create CanvasFramework for unified toolbar support
    // This is the CRITICAL pattern that VisualScriptEditorPanel uses!
    if (!m_framework)
    {
        m_framework = std::make_unique<CanvasFramework>(m_ownedDocument.get());
    }

    // Set document in framework (base class compatibility)
    SetDocument((void*)m_ownedDocument.get());

    m_isLoading = false;
    std::cout << "[PlaceholderGraphRenderer] Loaded successfully" << std::endl;
    return true;
}

bool PlaceholderGraphRenderer::Save(const std::string& filePath)
{
    std::cout << "[PlaceholderGraphRenderer] Saving: " << filePath << std::endl;

    PlaceholderGraphDocument* doc = GetDoc();
    if (!doc)
    {
        std::cout << "[PlaceholderGraphRenderer] ERROR: No document to save" << std::endl;
        return false;
    }

    return doc->Save(filePath);
}

void PlaceholderGraphRenderer::InitializeCanvasEditor()
{
    std::cout << "[PlaceholderGraphRenderer] InitializeCanvasEditor" << std::endl;

    if (!m_ownedDocument)
    {
        m_ownedDocument = std::make_unique<PlaceholderGraphDocument>();
        m_document = (void*)m_ownedDocument.get();
    }

    if (!m_ownedCanvas)
    {
        m_ownedCanvas = std::make_unique<PlaceholderCanvas>();
        m_canvas = (void*)m_ownedCanvas.get();
        m_ownedCanvas->Initialize(m_ownedDocument.get());
        m_ownedCanvas->SetRenderer(this);  // Phase 63.2: Set renderer reference for selection sync
    }

    // Phase 4 Step 5 FIX: Create CanvasFramework for unified toolbar support
    if (!m_framework)
    {
        m_framework = std::make_unique<CanvasFramework>(m_ownedDocument.get());
    }

    // Phase 4: Initialize PropertyEditorPanel for rendering properties
    if (!m_propertyEditor)
    {
        m_propertyEditor = std::make_unique<PlaceholderPropertyEditorPanel>();
        m_propertyEditor->Initialize(m_ownedDocument.get());
    }
}

void PlaceholderGraphRenderer::RenderCommonToolbar()
{
    // Phase 61 FIX: Use custom toolbar instead of BeginMenuBar (not available in BeginChild)
    // Render framework toolbar (Save, SaveAs, Browse) alongside common controls

    // ---- FRAMEWORK TOOLBAR ----
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->Render();
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::Separator();
        ImGui::SameLine(0.0f, 20.0f);
    }

    // ---- COMMON TOOLBAR ----
    // Phase 64: Connect toolbar state to canvas
    if (ImGui::Checkbox("Grid##toolbar", &m_gridVisible))
    {
        PlaceholderCanvas* canvas = GetCanvasPtr();
        if (canvas) canvas->SetGridVisible(m_gridVisible);
    }
    ImGui::SameLine(0.0f, 10.0f);

    if (ImGui::Button("Reset View##btn", ImVec2(80, 0))) {
        PlaceholderCanvas* canvas = GetCanvasPtr();
        if (canvas) canvas->ResetPanZoom();
    }
    ImGui::SameLine(0.0f, 10.0f);

    if (ImGui::Checkbox("Minimap##toolbar", &m_minimapVisible))
    {
        PlaceholderCanvas* canvas = GetCanvasPtr();
        if (canvas) canvas->SetMinimapVisible(m_minimapVisible);
    }
}

void PlaceholderGraphRenderer::RenderGraphContent()
{
    // Phase 4 Step 5 Feature #1: Unified toolbar + canvas + properties layout
    // NOTE: Framework toolbar is now rendered by RenderCommonToolbar()
    // which delegates to CanvasFramework. We removed manual toolbar rendering here.

    PlaceholderCanvas* canvas = GetCanvasPtr();
    if (!canvas)
        return;

    // Phase 64: Sync canvas selection with base class
    SyncCanvasSelectionWithBase();

    // ---- TYPE-SPECIFIC TOOLBAR ----
    // Verify, Run Graph, Minimap, Size controls
    RenderTypeSpecificToolbar();
    ImGui::Separator();

    // ---- THREE-COLUMN LAYOUT: Canvas + Palette + Properties ----
    // Left: Canvas with palette (Canvas 75% | Palette 25%)
    // Right: Properties panel with tabs (resizable width)

    // Get available space
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float totalHeight = ImGui::GetContentRegionAvail().y;

    // Use framework panel width from PanelManager
    float propertiesPanelWidth = static_cast<float>(PanelManager::InspectorPanelWidth);
    if (propertiesPanelWidth < 200.0f) propertiesPanelWidth = 200.0f;  // Minimum width
    if (propertiesPanelWidth > totalWidth * 0.60f) propertiesPanelWidth = totalWidth * 0.60f;  // Max 60%

    float handleWidth = 6.0f;
    float canvasAreaWidth = totalWidth - propertiesPanelWidth - handleWidth;

    // ---- LEFT: Canvas ----
    ImGui::BeginChild("PlaceholderCanvas", ImVec2(canvasAreaWidth, 0), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    canvas->Render();
    ImGui::EndChild();

    // ---- DRAG-DROP OVERLAY FOR PALETTE INSTANTIATION ----
    // Store canvas region for overlay positioning and coordinate transformation
    ImVec2 canvasRegionMin = ImGui::GetItemRectMin();
    ImVec2 canvasRegionMax(canvasRegionMin.x + canvasAreaWidth, ImGui::GetItemRectMax().y);

    // Create invisible target overlay that covers canvas area
    ImGui::SetCursorScreenPos(canvasRegionMin);
    ImGui::PushClipRect(canvasRegionMin, canvasRegionMax, false);
    ImGui::Dummy(ImVec2(canvasAreaWidth, canvasRegionMax.y - canvasRegionMin.y));

    // Accept drag-drop payload for node instantiation from palette
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLACEHOLDER_NODE_TYPE"))
        {
            int nodeTypeValue = *static_cast<const int*>(payload->Data);
            PlaceholderNodeType nodeType = static_cast<PlaceholderNodeType>(nodeTypeValue);
            ImVec2 mousePos = ImGui::GetMousePos();
            // Phase 68 FIX: Pass canvas region info directly to avoid context issues
            canvas->AcceptNodeDropAtCanvasPosition(nodeType, mousePos, canvasRegionMin, canvas->GetCanvasZoom());
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopClipRect();

    ImGui::SameLine();

    // ---- MAIN RESIZE HANDLE ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##placeholder_resize", ImVec2(handleWidth, -1.0f));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        // Update framework panel width directly
        PanelManager::InspectorPanelWidth -= static_cast<int>(ImGui::GetIO().MouseDelta.x);
        if (PanelManager::InspectorPanelWidth < 200) PanelManager::InspectorPanelWidth = 200;
        if (PanelManager::InspectorPanelWidth > static_cast<int>(totalWidth * 0.60f))
            PanelManager::InspectorPanelWidth = static_cast<int>(totalWidth * 0.60f);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // ---- RIGHT: Properties Panel (Unified Tab-Based Layout) ----
    // PHASE 4 PRIORITY 1 FIX: Simplified to unified tab-based layout
    // Tabs now render at TOP of right panel, content BELOW
    ImGui::BeginChild("PlaceholderRightPanel", ImVec2(propertiesPanelWidth, 0), true);

    // Render tabs and content directly (single unified panel)
    RenderRightPanelTabs();

    ImGui::EndChild();  // End PlaceholderRightPanel
}

void PlaceholderGraphRenderer::RenderRightPanelTabs()
{
    // Phase 4 Priority 1 Fix: Tabs render at TOP of right panel
    if (ImGui::BeginTabBar("RightPanelTabs", ImGuiTabBarFlags_None))
    {
        // Tab 0: Components (Phase 64.1: Node palette)
        if (ImGui::BeginTabItem("Components"))
        {
            if (!m_palette) {
                m_palette = std::make_unique<PlaceholderNodePalette>();
            }
            m_palette->Render();
            ImGui::EndTabItem();
        }

        // Tab 1: Node - Property editor with Apply/Reset buttons
        if (ImGui::BeginTabItem("Node"))
        {
            if (m_propertyEditor)
            {
                m_propertyEditor->Render(m_selectedNodeIds);

                // Apply/Reset buttons for batch property editing
                if (m_propertyEditor->HasChanges())
                {
                    ImGui::Separator();
                    ImGui::BeginGroup();

                    // Apply button
                    if (ImGui::Button("Apply", ImVec2(80, 0)))
                    {
                        std::cout << "[PlaceholderGraphRenderer] Applying property changes\n";
                        m_propertyEditor->ApplyChanges();
                        MarkDirty();
                    }

                    ImGui::SameLine();

                    // Reset button
                    if (ImGui::Button("Reset", ImVec2(80, 0)))
                    {
                        std::cout << "[PlaceholderGraphRenderer] Resetting property changes\n";
                        m_propertyEditor->ResetChanges();
                    }

                    ImGui::EndGroup();
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void PlaceholderGraphRenderer::RenderFrameworkModals()
{
    // Phase 73: Delegate modal rendering to framework toolbar
    // The SaveAs/Browse modals are managed by CanvasToolbarRenderer and must be
    // rendered separately (after all content in the frame cycle) to prevent ImGui ID conflicts
    // This pattern is used by EntityPrefabRenderer and BehaviorTreeRenderer

    if (m_framework && m_framework->GetToolbar())
    {
        // log Spam SYSTEM_LOG << "[PlaceholderGraphRenderer::RenderFrameworkModals] Rendering toolbar modals\n";
        m_framework->GetToolbar()->RenderModals();
    }
}

// ============================================================================
// Phase 70-71: Graph Verification and Traversal Helpers (Implementations)
// ============================================================================

std::vector<int> GetOrphanedNodes(const PlaceholderGraphDocument* document)
{
    std::vector<int> orphaned;
    if (!document) return orphaned;

    const auto& nodes = document->GetAllNodes();
    const auto& connections = document->GetAllConnections();

    for (const auto& node : nodes)
    {
        bool hasConnections = false;
        for (const auto& conn : connections)
        {
            if (conn.fromNodeId == node.nodeId || conn.toNodeId == node.nodeId)
            {
                hasConnections = true;
                break;
            }
        }
        if (!hasConnections)
        {
            orphaned.push_back(node.nodeId);
        }
    }
    return orphaned;
}

std::vector<int> GetRootNodes(const PlaceholderGraphDocument* document)
{
    std::vector<int> roots;
    if (!document) return roots;

    const auto& nodes = document->GetAllNodes();
    const auto& connections = document->GetAllConnections();

    for (const auto& node : nodes)
    {
        bool hasIncoming = false;
        for (const auto& conn : connections)
        {
            if (conn.toNodeId == node.nodeId)
            {
                hasIncoming = true;
                break;
            }
        }
        if (!hasIncoming)
        {
            roots.push_back(node.nodeId);
        }
    }
    return roots;
}

std::vector<int> GetLeafNodes(const PlaceholderGraphDocument* document)
{
    std::vector<int> leaves;
    if (!document) return leaves;

    const auto& nodes = document->GetAllNodes();
    const auto& connections = document->GetAllConnections();

    for (const auto& node : nodes)
    {
        bool hasOutgoing = false;
        for (const auto& conn : connections)
        {
            if (conn.fromNodeId == node.nodeId)
            {
                hasOutgoing = true;
                break;
            }
        }
        if (!hasOutgoing)
        {
            leaves.push_back(node.nodeId);
        }
    }
    return leaves;
}

bool TraverseNode(
    const PlaceholderGraphDocument* document,
    int nodeId,
    std::vector<int>& visited,
    std::vector<int>& path,
    int depth)
{
    const int MAX_DEPTH = 1000;
    if (depth > MAX_DEPTH) return false;

    path.push_back(nodeId);
    visited.push_back(nodeId);

    const auto& connections = document->GetAllConnections();
    std::vector<int> childNodes;
    for (const auto& conn : connections)
    {
        if (conn.fromNodeId == nodeId)
        {
            childNodes.push_back(conn.toNodeId);
        }
    }

    if (childNodes.empty()) return true;

    for (int childId : childNodes)
    {
        if (std::find(visited.begin(), visited.end(), childId) != visited.end())
        {
            return false;  // Cycle detected
        }
        if (!TraverseNode(document, childId, visited, path, depth + 1))
        {
            return false;
        }
    }
    return true;
}

void PlaceholderGraphRenderer::RenderTypeSpecificToolbar()
{
    // Phase 62 FIX: Removed Minimap checkbox (already in RenderCommonToolbar - was duplicate)

    // Phase 70: Verify button
    if (ImGui::Button("Verify##placeholder"))
    {
        PlaceholderGraphDocument* doc = GetDoc();
        if (doc)
        {
            SYSTEM_LOG << "[PlaceholderGraphRenderer] Verify button clicked\n";
            std::cout << "[PlaceholderGraphRenderer] === GRAPH VERIFICATION ===\n";

            // Phase 72: Clear previous logs and populate with new verification results
            m_verificationLogs.clear();

            // Phase 70: Get verification info
            auto orphanedNodeIds = GetOrphanedNodes(doc);
            bool isValid = orphanedNodeIds.empty();

            // Phase 72: Add formatted logs for panel display
            std::string statusStr = isValid ? "VALID" : "INVALID";
            m_verificationLogs.push_back("[VERIFICATION] Graph Status: " + statusStr);
            m_verificationLogs.push_back("[VERIFICATION] Total Nodes: " + std::to_string(doc->GetNodeCount()));
            m_verificationLogs.push_back("[VERIFICATION] Total Connections: " + std::to_string(doc->GetAllConnections().size()));
            m_verificationLogs.push_back("[VERIFICATION] Orphaned Nodes: " + std::to_string(orphanedNodeIds.size()));

            if (!orphanedNodeIds.empty())
            {
                std::string orphanList = "[VERIFICATION] Orphaned Node IDs: ";
                for (size_t i = 0; i < orphanedNodeIds.size(); ++i)
                {
                    if (i > 0) orphanList += ", ";
                    orphanList += std::to_string(orphanedNodeIds[i]);
                }
                m_verificationLogs.push_back(orphanList);
            }

            // Also log to SYSTEM_LOG for console debugging
            for (const auto& log : m_verificationLogs)
            {
                SYSTEM_LOG << log << "\n";
                std::cout << log << "\n";
            }

            m_verificationDone = true;
        }
    }
    ImGui::SameLine();

    // Phase 71: Run Graph button
    if (ImGui::Button("Run Graph##placeholder"))
    {
        PlaceholderGraphDocument* doc = GetDoc();
        if (doc)
        {
            SYSTEM_LOG << "[PlaceholderGraphRenderer] Run Graph button clicked\n";
            std::cout << "[PlaceholderGraphRenderer] === GRAPH EXECUTION ===\n";

            // Phase 72: Clear previous logs and populate with new traversal results
            m_verificationLogs.clear();

            // Phase 71: Get root nodes
            auto rootNodeIds = GetRootNodes(doc);
            auto leafNodeIds = GetLeafNodes(doc);

            // Phase 72: Add formatted logs for panel display
            m_verificationLogs.push_back("[EXECUTION] Graph Traversal Started");

            // Build root nodes string
            std::string rootStr = "[EXECUTION] Root Nodes: " + std::to_string(rootNodeIds.size()) + " - [";
            for (size_t i = 0; i < rootNodeIds.size(); ++i)
            {
                if (i > 0) rootStr += ", ";
                rootStr += std::to_string(rootNodeIds[i]);
            }
            rootStr += "]";
            m_verificationLogs.push_back(rootStr);

            // Build leaf nodes string
            std::string leafStr = "[EXECUTION] Leaf Nodes: " + std::to_string(leafNodeIds.size()) + " - [";
            for (size_t i = 0; i < leafNodeIds.size(); ++i)
            {
                if (i > 0) leafStr += ", ";
                leafStr += std::to_string(leafNodeIds[i]);
            }
            leafStr += "]";
            m_verificationLogs.push_back(leafStr);

            // Traverse from each root node
            std::vector<int> allVisited;
            bool succeeded = true;
            for (int rootId : rootNodeIds)
            {
                std::vector<int> visited;
                std::vector<int> currentPath;

                if (TraverseNode(doc, rootId, visited, currentPath))
                {
                    std::string pathStr = "[EXECUTION] Path from " + std::to_string(rootId) + ": ";
                    for (size_t i = 0; i < currentPath.size(); ++i)
                    {
                        if (i > 0) pathStr += " → ";
                        pathStr += std::to_string(currentPath[i]);
                    }
                    m_verificationLogs.push_back(pathStr);

                    for (int nodeId : currentPath)
                    {
                        if (std::find(allVisited.begin(), allVisited.end(), nodeId) == allVisited.end())
                        {
                            allVisited.push_back(nodeId);
                        }
                    }
                }
                else
                {
                    succeeded = false;
                    m_verificationLogs.push_back("[ERROR] Cycle or traversal error from node " + std::to_string(rootId));
                }
            }

            m_verificationLogs.push_back("[EXECUTION] Total Unique Nodes Visited: " + std::to_string(allVisited.size()));
            m_verificationLogs.push_back("[EXECUTION] Traversal Status: " + std::string(succeeded ? "SUCCESS" : "FAILED"));

            // Also log to SYSTEM_LOG for console debugging
            for (const auto& log : m_verificationLogs)
            {
                SYSTEM_LOG << log << "\n";
                std::cout << log << "\n";
            }

            m_verificationDone = true;
        }
    }
}

void PlaceholderGraphRenderer::RenderTypePanels()
{
    // Phase 4 Step 4: Render tabbed right panel
    // NOTE: RenderRightPanelTabs() is called from RenderGraphContent() Part B
    // DO NOT duplicate here - this was causing tabs to render twice
    // This method is a no-op in the new layout architecture
}

void PlaceholderGraphRenderer::HandleTypeSpecificShortcuts()
{
    // Placeholder type doesn't have special shortcuts yet
    // (common shortcuts handled by GraphEditorBase)
}

void PlaceholderGraphRenderer::SelectNodesInRectangle(const ImVec2& rectStart, const ImVec2& rectEnd)
{
    // Phase 3: AABB hit detection for nodes in rectangle
    PlaceholderGraphDocument* doc = GetDoc();
    if (!doc) return;

    m_selectedNodeIds.clear();  // Clear previous selection

    // Get all nodes from document
    for (int nodeId = 1; nodeId <= doc->GetNodeCount(); nodeId++) {
        PlaceholderNode* node = doc->GetNode(nodeId);
        if (!node) continue;

        // AABB intersection test
        ImVec2 nodeMin(node->posX, node->posY);
        ImVec2 nodeMax(node->posX + node->width, node->posY + node->height);

        // Check if node overlaps with selection rectangle
        if (!(rectEnd.x < nodeMin.x || rectStart.x > nodeMax.x ||
              rectEnd.y < nodeMin.y || rectStart.y > nodeMax.y)) {
            m_selectedNodeIds.push_back(nodeId);
        }
    }
}

void PlaceholderGraphRenderer::SyncCanvasSelectionWithBase()
{
    // Phase 64: Synchronize canvas single selection with base class vector selection
    // Get selected node from canvas
    PlaceholderCanvas* canvas = GetCanvasPtr();
    if (canvas) {
        int canvasSelectedNodeId = canvas->GetSelectedNodeId();
        if (canvasSelectedNodeId >= 0) {
            // Update base class selected nodes
            m_selectedNodeIds.clear();
            m_selectedNodeIds.push_back(canvasSelectedNodeId);
        } else if (!m_selectedNodeIds.empty() && m_selectedNodeIds.size() == 1) {
            // If base class has single selection but canvas doesn't, update canvas
            canvas->SetSelectedNodeId(m_selectedNodeIds[0]);
        }
    }
}

void PlaceholderGraphRenderer::DeleteSelectedNodes()
{
    // Phase 3: Delete all selected nodes from document
    PlaceholderGraphDocument* doc = GetDoc();
    if (!doc || m_selectedNodeIds.empty()) return;

    std::cout << "[PlaceholderGraphRenderer] DeleteSelectedNodes: " << m_selectedNodeIds.size() << " node(s)\n";

    // Delete in reverse order to maintain IDs
    for (auto it = m_selectedNodeIds.rbegin(); it != m_selectedNodeIds.rend(); ++it) {
        doc->DeleteNode(*it);
    }

    m_selectedNodeIds.clear();
    doc->SetDirty(true);
}

void PlaceholderGraphRenderer::MoveSelectedNodes(float deltaX, float deltaY)
{
    // Phase 3: Move all selected nodes by delta
    PlaceholderGraphDocument* doc = GetDoc();
    if (!doc || m_selectedNodeIds.empty()) return;

    std::cout << "[PlaceholderGraphRenderer] MoveSelectedNodes: delta=(" << deltaX << ", " << deltaY << ")\n";

    for (int nodeId : m_selectedNodeIds) {
        PlaceholderNode* node = doc->GetNode(nodeId);
        if (node) {
            doc->SetNodePosition(nodeId, node->posX + deltaX, node->posY + deltaY);
        }
    }

    doc->SetDirty(true);
}

void PlaceholderGraphRenderer::UpdateSelectedNodesProperty(const std::string& propName, const std::string& propValue)
{
    // Phase 4 Step 3: Batch property editing - apply property change to all selected nodes
    PlaceholderGraphDocument* doc = GetDoc();
    if (!doc || m_selectedNodeIds.empty()) return;

    std::cout << "[PlaceholderGraphRenderer] UpdateSelectedNodesProperty: " << propName << " = " << propValue << " for " << m_selectedNodeIds.size() << " node(s)\n";

    // Apply property to each selected node
    for (int nodeId : m_selectedNodeIds)
    {
        PlaceholderNode* node = doc->GetNode(nodeId);
        if (!node) continue;

        // Map property name to setter
        if (propName == "title")
        {
            doc->SetNodeTitle(nodeId, propValue);
        }
        else if (propName == "filepath")
        {
            doc->SetNodeFilepath(nodeId, propValue);
        }
        else if (propName == "posX")
        {
            try {
                float x = std::stof(propValue);
                doc->SetNodePosition(nodeId, x, node->posY);
            } catch (...) {
                std::cerr << "[PlaceholderGraphRenderer] Invalid posX value: " << propValue << "\n";
            }
        }
        else if (propName == "posY")
        {
            try {
                float y = std::stof(propValue);
                doc->SetNodePosition(nodeId, node->posX, y);
            } catch (...) {
                std::cerr << "[PlaceholderGraphRenderer] Invalid posY value: " << propValue << "\n";
            }
        }
        else if (propName == "width")
        {
            try {
                int width = std::stoi(propValue);
                node->width = width;
            } catch (...) {
                std::cerr << "[PlaceholderGraphRenderer] Invalid width value: " << propValue << "\n";
            }
        }
        else if (propName == "height")
        {
            try {
                int height = std::stoi(propValue);
                node->height = height;
            } catch (...) {
                std::cerr << "[PlaceholderGraphRenderer] Invalid height value: " << propValue << "\n";
            }
        }
        else if (propName == "enabled")
        {
            bool enabled = (propValue == "true" || propValue == "1");
            doc->SetNodeEnabled(nodeId, enabled);
        }
    }

    doc->SetDirty(true);
}

void PlaceholderGraphRenderer::CreateNewGraph()
{
    std::cout << "[PlaceholderGraphRenderer] Creating new graph" << std::endl;

    if (!m_ownedDocument)
    {
        m_ownedDocument = std::make_unique<PlaceholderGraphDocument>();
        m_document = (void*)m_ownedDocument.get();
    }
    else
    {
        m_ownedDocument->Clear();
    }

    // Create sample nodes for demonstration
    m_ownedDocument->CreateNode(PlaceholderNodeType::Blue, "Blue Node", 100.0f, 100.0f);
    m_ownedDocument->CreateNode(PlaceholderNodeType::Green, "Green Node", 300.0f, 100.0f);
    m_ownedDocument->CreateNode(PlaceholderNodeType::Magenta, "Magenta Node", 200.0f, 250.0f);

    // Create sample connections
    m_ownedDocument->CreateConnection(1, 3);
    m_ownedDocument->CreateConnection(2, 3);

    if (!m_ownedCanvas)
    {
        m_ownedCanvas = std::make_unique<PlaceholderCanvas>();
        m_canvas = (void*)m_ownedCanvas.get();
        m_ownedCanvas->Initialize(m_ownedDocument.get());
        m_ownedCanvas->SetRenderer(this);  // Phase 63.2: Set renderer reference for selection sync
    }

    // Phase 53 Final Fix: Initialize framework for unified toolbar
    // This ensures m_framework is created when CreateNewGraph() is called
    // so that RenderCommonToolbar() can render [Save][SaveAs][Browse] buttons
    if (!m_framework)
    {
        m_framework = std::make_unique<CanvasFramework>(m_ownedDocument.get());
    }

    // Phase 64: Initialize PropertyEditorPanel if needed
    if (!m_propertyEditor)
    {
        m_propertyEditor = std::make_unique<PlaceholderPropertyEditorPanel>();
        m_propertyEditor->Initialize(m_ownedDocument.get());
    }

    SetDocument((void*)m_ownedDocument.get());
}

// ============================================================================
// Phase 69: Save/SaveAs Integration
// ============================================================================

bool PlaceholderGraphRenderer::ExecuteSave(const std::string& filePath)
{
    PlaceholderGraphDocument* doc = GetDoc();
    if (!doc)
    {
        std::cerr << "[PlaceholderGraphRenderer::ExecuteSave] ERROR: No document to save\n";
        SYSTEM_LOG << "[PlaceholderGraphRenderer::ExecuteSave] ERROR: No document to save\n";
        return false;
    }

    SYSTEM_LOG << "[PlaceholderGraphRenderer::ExecuteSave] Saving to: " << filePath << "\n";
    std::cout << "[PlaceholderGraphRenderer::ExecuteSave] Saving to: " << filePath << "\n";

    // Set filepath before saving (framework pattern)
    doc->SetFilePath(filePath);

    // Call document's Save method (handles JSON serialization)
    bool success = doc->Save(filePath);

    if (success)
    {
        SYSTEM_LOG << "[PlaceholderGraphRenderer::ExecuteSave] ✓ Successfully saved " << filePath << "\n";
        std::cout << "[PlaceholderGraphRenderer::ExecuteSave] ✓ Successfully saved\n";
        doc->SetDirty(false);
    }
    else
    {
        SYSTEM_LOG << "[PlaceholderGraphRenderer::ExecuteSave] ✗ Save failed!\n";
        std::cerr << "[PlaceholderGraphRenderer::ExecuteSave] ✗ Save failed!\n";
    }

    return success;
}

    bool PlaceholderGraphRenderer::ExecuteSaveAs(const std::string& filePath)
    {
        PlaceholderGraphDocument* doc = GetDoc();
        if (!doc)
        {
            std::cerr << "[PlaceholderGraphRenderer::ExecuteSaveAs] ERROR: No document to save\n";
            SYSTEM_LOG << "[PlaceholderGraphRenderer::ExecuteSaveAs] ERROR: No document to save\n";
            return false;
        }

        SYSTEM_LOG << "[PlaceholderGraphRenderer::ExecuteSaveAs] SaveAs to: " << filePath << "\n";
        std::cout << "[PlaceholderGraphRenderer::ExecuteSaveAs] SaveAs to: " << filePath << "\n";

        // Simply delegate to ExecuteSave (pattern: SaveAs = Save with new path)
        return ExecuteSave(filePath);
    }

    // ============================================================================
    // Phase 72: Verification Output Panel (Legacy Pattern from VisualScriptEditorPanel)
    // ============================================================================

    void PlaceholderGraphRenderer::RenderVerificationLogsPanel()
    {
        // Note: The header "Verification Output" is rendered by the container (BlueprintEditorGUI),
        // so we only render the content here (status + logs).

        if (!m_verificationDone)
        {
            ImGui::TextDisabled("(Click 'Verify' or 'Run Graph' button to generate output)");
            return;
        }

        // Display verification result summary
        ImGui::Spacing();

        // Status line with color coding
        if (m_verificationLogs.empty())
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[OK] Graph is valid - no issues found");
        }
        else
        {
            // Check if any errors in logs
            bool hasErrors = false;
            for (const auto& log : m_verificationLogs)
            {
                if (log.find("[ERROR]") != std::string::npos)
                {
                    hasErrors = true;
                    break;
                }
            }

            if (hasErrors)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                    "[ERROR] Graph has issues");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f),
                    "[INFO] Graph verification complete");
            }
        }

        ImGui::Separator();

        // Display all logs
        ImGui::BeginChild("VerificationLogsChild", ImVec2(0, 0), true);

        if (!m_verificationLogs.empty())
        {
            for (size_t i = 0; i < m_verificationLogs.size(); ++i)
            {
                const std::string& logEntry = m_verificationLogs[i];

                // Color-code based on log entry content (matching legacy pattern)
                ImVec4 logColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Default: white

                if (logEntry.find("[ERROR]") != std::string::npos)
                    logColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
                else if (logEntry.find("[WARN]") != std::string::npos)
                    logColor = ImVec4(1.0f, 0.85f, 0.0f, 1.0f); // Yellow
                else if (logEntry.find("[INFO]") != std::string::npos)
                    logColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);  // Light blue
                else if (logEntry.find("[EXECUTION]") != std::string::npos)
                    logColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);  // Green
                else if (logEntry.find("[VERIFICATION]") != std::string::npos)
                    logColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);  // Light blue
                else if (logEntry.find("[SUCCESS]") != std::string::npos)
                    logColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);  // Bright green
                else if (logEntry.find("===") != std::string::npos)
                    logColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // Gray (separator)

                ImGui::TextColored(logColor, "%s", logEntry.c_str());
            }
        }
        else
        {
            ImGui::TextDisabled("No verification logs");
        }

        ImGui::EndChild();
    }

} // namespace Olympe
