/**
 * @file AIEditorGUI.cpp
 * @brief Implementation of AIEditorGUI
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "AIEditorGUI.h"
#include "AIEditorClipboard.h"
#include "AIEditorFileDialog.h"
#include "../../system/system_utils.h"
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imnodes/imnodes.h"
#include "../AIGraphPlugin_BT/BTGraphValidator.h"
#include "../../NodeGraphCore/Commands/CreateNodeCommand.h"
#include "../../NodeGraphCore/Commands/ConnectPinsCommand.h"
#include <cstring>

namespace Olympe {
namespace AI {

// ============================================================================
// Constructor / Destructor
// ============================================================================

AIEditorGUI::AIEditorGUI()
    : m_isActive(false)
    , m_showNodePalette(true)
    , m_showBlackboardPanel(false)
    , m_showSensesPanel(false)
    , m_showRuntimeDebugPanel(false)
    , m_imnodesContext(nullptr)
{
    std::memset(m_assetSearchFilter, 0, sizeof(m_assetSearchFilter));
}

AIEditorGUI::~AIEditorGUI()
{
    Shutdown();
}

// ============================================================================
// Initialize / Shutdown
// ============================================================================

bool AIEditorGUI::Initialize()
{
    SYSTEM_LOG << "[AIEditorGUI] Initializing..." << std::endl;
    
    // Create ImNodes context
    m_imnodesContext = ImNodes::CreateContext();
    if (m_imnodesContext == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] ERROR: Failed to create ImNodes context" << std::endl;
        return false;
    }
    ImNodes::SetCurrentContext(static_cast<ImNodesContext*>(m_imnodesContext));
    
    // Initialize BTNodeRegistry
    BTNodeRegistry& registry = BTNodeRegistry::Get();
    auto allTypes = registry.GetAllNodeTypes();
    SYSTEM_LOG << "[AIEditorGUI] Loaded " << allTypes.size() << " node types" << std::endl;
    
    // Create BTNodePalette
    m_nodePalette.reset(new BTNodePalette());
    
    // Scan AI graph directory
    ScanAIGraphDirectory("Blueprints/AI/");
    
    m_isActive = true;
    
    SYSTEM_LOG << "[AIEditorGUI] Initialization complete" << std::endl;
    return true;
}

void AIEditorGUI::Shutdown()
{
    SYSTEM_LOG << "[AIEditorGUI] Shutting down..." << std::endl;
    
    // Destroy ImNodes context
    if (m_imnodesContext != nullptr) {
        ImNodes::DestroyContext(static_cast<ImNodesContext*>(m_imnodesContext));
        m_imnodesContext = nullptr;
    }
    
    // Clear command stack
    m_commandStack.Clear();
    
    // Clear node palette
    m_nodePalette.reset();
    
    m_isActive = false;
}

// ============================================================================
// Update / Render
// ============================================================================

void AIEditorGUI::Update(float deltaTime)
{
    (void)deltaTime;
    
    // Early exit if editor is not active
    // When m_isActive is false, the editor is minimized or closed and should not process input
    if (!m_isActive) {
        return;
    }
    
    // Handle keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    bool ctrlPressed = io.KeyCtrl;
    bool shiftPressed = io.KeyShift;
    
    // Ctrl+Z - Undo
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_Z)) {
        MenuAction_Undo();
    }
    
    // Ctrl+Y or Ctrl+Shift+Z - Redo
    if ((ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_Y)) ||
        (ctrlPressed && shiftPressed && ImGui::IsKeyPressed(ImGuiKey_Z))) {
        MenuAction_Redo();
    }
    
    // Ctrl+C - Copy
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_C)) {
        MenuAction_Copy();
    }
    
    // Ctrl+X - Cut
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_X)) {
        MenuAction_Cut();
    }
    
    // Ctrl+V - Paste
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_V)) {
        MenuAction_Paste();
    }
    
    // Delete - Delete selected
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        MenuAction_Delete();
    }
    
    // Ctrl+A - Select All
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_A)) {
        MenuAction_SelectAll();
    }
    
    // Ctrl+N - New BT
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_N)) {
        MenuAction_NewBT();
    }
    
    // Ctrl+S - Save
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_S)) {
        MenuAction_Save();
    }
    
    // Ctrl+Shift+S - Save As
    if (ctrlPressed && shiftPressed && ImGui::IsKeyPressed(ImGuiKey_S)) {
        MenuAction_SaveAs();
    }
    
    // Ctrl+O - Open
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_O)) {
        MenuAction_Open();
    }
    
    // Ctrl+W - Close
    if (ctrlPressed && !shiftPressed && ImGui::IsKeyPressed(ImGuiKey_W)) {
        MenuAction_Close();
    }
}

void AIEditorGUI::Render()
{
    if (!m_isActive) {
        return;
    }
    
    // Main window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoCollapse
                           | ImGuiWindowFlags_NoBringToFrontOnFocus
                           | ImGuiWindowFlags_MenuBar;
    
    ImGui::Begin("AI Editor", nullptr, flags);
    
    // Menu bar
    RenderMenuBar();
    
    // 3-panel layout
    ImGui::BeginChild("AssetBrowser", ImVec2(250, 0), true);
    RenderAssetBrowser();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("NodeGraph", ImVec2(-350, 0), true);
    RenderNodeGraph();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("Inspector", ImVec2(0, 0), true);
    RenderInspector();
    ImGui::EndChild();
    
    ImGui::End();
    
    // Render node palette
    if (m_showNodePalette && m_nodePalette) {
        m_nodePalette->Render(&m_showNodePalette);
    }
    
    // Render specialized panels
    if (m_showBlackboardPanel) {
        RenderBlackboardPanel();
    }
    
    if (m_showSensesPanel) {
        RenderSensesPanel();
    }
    
    if (m_showRuntimeDebugPanel) {
        RenderRuntimeDebugPanel();
    }
}

// ============================================================================
// Menu Bar
// ============================================================================

void AIEditorGUI::RenderMenuBar()
{
    if (ImGui::BeginMenuBar()) {
        // File Menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Behavior Tree", "Ctrl+N")) {
                MenuAction_NewBT();
            }
            if (ImGui::MenuItem("New HFSM", "Ctrl+Shift+N")) {
                MenuAction_NewHFSM();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                MenuAction_Open();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                MenuAction_Save();
            }
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                MenuAction_SaveAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close", "Ctrl+W")) {
                MenuAction_Close();
            }
            ImGui::EndMenu();
        }
        
        // Edit Menu
        if (ImGui::BeginMenu("Edit")) {
            bool canUndo = m_commandStack.CanUndo();
            bool canRedo = m_commandStack.CanRedo();
            
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo)) {
                MenuAction_Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) {
                MenuAction_Redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                MenuAction_Cut();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                MenuAction_Copy();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                MenuAction_Paste();
            }
            if (ImGui::MenuItem("Delete", "Delete")) {
                MenuAction_Delete();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                MenuAction_SelectAll();
            }
            ImGui::EndMenu();
        }
        
        // View Menu
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Node Palette", nullptr, &m_showNodePalette);
            ImGui::MenuItem("Blackboard", nullptr, &m_showBlackboardPanel);
            ImGui::MenuItem("Senses Debug", nullptr, &m_showSensesPanel);
            ImGui::MenuItem("Runtime Debug", nullptr, &m_showRuntimeDebugPanel);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                MenuAction_ResetLayout();
            }
            ImGui::EndMenu();
        }
        
        // Help Menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                MenuAction_About();
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

// ============================================================================
// Asset Browser
// ============================================================================

void AIEditorGUI::RenderAssetBrowser()
{
    ImGui::Text("Asset Browser");
    ImGui::Separator();
    
    // Search filter
    ImGui::InputText("Search", m_assetSearchFilter, sizeof(m_assetSearchFilter));
    
    ImGui::Separator();
    
    // Asset list
    for (size_t i = 0; i < m_assetFiles.size(); ++i) {
        const std::string& file = m_assetFiles[i];
        
        // Apply search filter
        if (m_assetSearchFilter[0] != '\0') {
            if (file.find(m_assetSearchFilter) == std::string::npos) {
                continue;
            }
        }
        
        RenderAssetEntry(file, file);
    }
}

void AIEditorGUI::ScanAIGraphDirectory(const std::string& directory)
{
    // For now, just clear the list
    // In a real implementation, this would scan the filesystem
    m_assetFiles.clear();
    SYSTEM_LOG << "[AIEditorGUI] Scanned " << directory << std::endl;
}

void AIEditorGUI::RenderAssetEntry(const std::string& filename, const std::string& fullPath)
{
    if (ImGui::Selectable(filename.c_str())) {
        SYSTEM_LOG << "[AIEditorGUI] Selected asset: " << fullPath << std::endl;
        // TODO: Load asset on double-click
    }
}

// ============================================================================
// Node Graph
// ============================================================================

void AIEditorGUI::RenderNodeGraph()
{
    ImGui::Text("Node Graph");
    ImGui::Separator();
    
    // Get active graph
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    
    if (doc == nullptr) {
        ImGui::Text("No active graph. Create a new one from File menu.");
        return;
    }
    
    // Render canvas
    RenderNodeGraphCanvas();
}

void AIEditorGUI::RenderNodeGraphCanvas()
{
    ImNodes::SetCurrentContext(static_cast<ImNodesContext*>(m_imnodesContext));
    ImNodes::BeginNodeEditor();
    
    // Get active graph
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    
    if (doc != nullptr) {
        // Render nodes
        const std::vector<NodeGraph::NodeData>& nodes = doc->GetNodes();
        for (size_t i = 0; i < nodes.size(); ++i) {
            RenderNode(nodes[i].id);
        }
        
        // Render connections
        RenderConnections();
    }
    
    ImNodes::EndNodeEditor();
    
    // Handle interactions
    HandleNodeCreation();
    HandleNodeSelection();
    HandleLinkCreation();
}

void AIEditorGUI::RenderNode(NodeGraph::NodeId nodeId)
{
    // Get node data
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    if (doc == nullptr) {
        return;
    }
    
    const NodeGraph::NodeData* nodeData = doc->GetNode(nodeId);
    if (nodeData == nullptr) {
        return;
    }
    
    // Get node type info
    BTNodeRegistry& registry = BTNodeRegistry::Get();
    const BTNodeTypeInfo* typeInfo = registry.GetNodeTypeInfo(nodeData->type);
    
    if (typeInfo == nullptr) {
        return;
    }
    
    // Begin node
    int iNodeId = static_cast<int>(nodeData->id.value);
    ImNodes::BeginNode(iNodeId);
    
    // Title bar with color
    ImNodes::BeginNodeTitleBar();
    uint32_t color = typeInfo->color;
    ImGui::TextColored(
        ImVec4(
            ((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f,
            ((color >> 0) & 0xFF) / 255.0f,
            ((color >> 24) & 0xFF) / 255.0f
        ),
        "%s %s", typeInfo->icon.c_str(), nodeData->name.c_str()
    );
    ImNodes::EndNodeTitleBar();
    
    // Input pin
    int inputPinId = iNodeId * 1000;
    ImNodes::BeginInputAttribute(inputPinId);
    ImGui::Text("In");
    ImNodes::EndInputAttribute();
    
    // Parameters
    for (auto it = nodeData->parameters.begin(); it != nodeData->parameters.end(); ++it) {
        ImGui::Text("%s: %s", it->first.c_str(), it->second.c_str());
    }
    
    // Output pin
    int outputPinId = iNodeId * 1000 + 1;
    ImNodes::BeginOutputAttribute(outputPinId);
    ImGui::Text("Out");
    ImNodes::EndOutputAttribute();
    
    ImNodes::EndNode();
    
    // Set node position
    ImNodes::SetNodeGridSpacePos(iNodeId, ImVec2(nodeData->position.x, nodeData->position.y));
}

void AIEditorGUI::RenderConnections()
{
    // Get active graph
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    if (doc == nullptr) {
        return;
    }
    
    const std::vector<NodeGraph::LinkData>& links = doc->GetLinks();
    for (size_t i = 0; i < links.size(); ++i) {
        int linkId = static_cast<int>(links[i].id.value);
        int fromPin = static_cast<int>(links[i].fromPin.value);
        int toPin = static_cast<int>(links[i].toPin.value);
        
        ImNodes::Link(linkId, fromPin, toPin);
    }
}

void AIEditorGUI::HandleNodeCreation()
{
    // Check if node palette is dragging
    if (m_nodePalette && m_nodePalette->IsDragging()) {
        // TODO: Handle node creation from palette drag
    }
}

void AIEditorGUI::HandleNodeSelection()
{
    // Get selected nodes
    int numSelected = ImNodes::NumSelectedNodes();
    if (numSelected > 0) {
        m_selectedNodeIds.resize(static_cast<size_t>(numSelected));
        ImNodes::GetSelectedNodes(m_selectedNodeIds.data());
    }
    
    // Get selected links
    int numSelectedLinks = ImNodes::NumSelectedLinks();
    if (numSelectedLinks > 0) {
        m_selectedLinkIds.resize(static_cast<size_t>(numSelectedLinks));
        ImNodes::GetSelectedLinks(m_selectedLinkIds.data());
    }
}

void AIEditorGUI::HandleLinkCreation()
{
    int startPin = -1;
    int endPin = -1;
    
    if (ImNodes::IsLinkCreated(&startPin, &endPin)) {
        SYSTEM_LOG << "[AIEditorGUI] Link created: " << startPin << " -> " << endPin << std::endl;
        
        // TODO: Create ConnectPinsCommand and execute
        // For now, just connect directly
        NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
        NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
        if (doc != nullptr) {
            NodeGraph::PinId fromPin;
            fromPin.value = static_cast<uint32_t>(startPin);
            NodeGraph::PinId toPin;
            toPin.value = static_cast<uint32_t>(endPin);
            
            doc->ConnectPins(fromPin, toPin);
        }
    }
}

// ============================================================================
// Inspector
// ============================================================================

void AIEditorGUI::RenderInspector()
{
    ImGui::Text("Inspector");
    ImGui::Separator();
    
    // Show selected node properties
    if (!m_selectedNodeIds.empty()) {
        ImGui::Text("Selected Node: %d", m_selectedNodeIds[0]);
        // TODO: Show node properties
    } else {
        ImGui::Text("No selection");
    }
}

// ============================================================================
// AI-Specific Panels
// ============================================================================

void AIEditorGUI::RenderBlackboardPanel()
{
    ImGui::Begin("Blackboard", &m_showBlackboardPanel);
    
    ImGui::Text("Blackboard Variables");
    ImGui::Separator();
    
    // TODO: Implement blackboard variable list
    ImGui::Text("(Not yet implemented)");
    
    ImGui::End();
}

void AIEditorGUI::RenderSensesPanel()
{
    ImGui::Begin("AI Senses Debug", &m_showSensesPanel);
    
    ImGui::Text("AI Senses");
    ImGui::Separator();
    
    // TODO: Implement senses debug
    ImGui::Text("(Not yet implemented)");
    
    ImGui::End();
}

void AIEditorGUI::RenderRuntimeDebugPanel()
{
    ImGui::Begin("Runtime Debug", &m_showRuntimeDebugPanel);
    
    ImGui::Text("Runtime Execution");
    ImGui::Separator();
    
    // TODO: Implement runtime debug
    ImGui::Text("(Not yet implemented)");
    
    ImGui::End();
}

// ============================================================================
// Menu Actions
// ============================================================================

void AIEditorGUI::MenuAction_NewBT()
{
    SYSTEM_LOG << "[AIEditorGUI] Creating new Behavior Tree" << std::endl;
    
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");
    mgr.SetActiveGraph(id);
}

void AIEditorGUI::MenuAction_NewHFSM()
{
    SYSTEM_LOG << "[AIEditorGUI] Creating new HFSM" << std::endl;
    
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphId id = mgr.CreateGraph("AIGraph", "HFSM");
    mgr.SetActiveGraph(id);
}

void AIEditorGUI::MenuAction_Open()
{
    SYSTEM_LOG << "[AIEditorGUI] Open file dialog" << std::endl;
    
    // Open native file dialog
    std::string filepath = AIEditorFileDialog::OpenFile("json,btree", m_lastOpenPath);
    
    if (!filepath.empty()) {
        // Load the graph
        NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
        NodeGraph::GraphId id = mgr.LoadGraph(filepath);
        
        if (id.value != 0) {
            mgr.SetActiveGraph(id);
            m_lastOpenPath = ExtractDirectory(filepath);
            SYSTEM_LOG << "[AIEditorGUI] Loaded graph: " << filepath << std::endl;
        }
        else {
            SYSTEM_LOG << "[AIEditorGUI] ERROR: Failed to load graph from " << filepath << std::endl;
        }
    }
    else {
        // Check if there was an error (not just cancel)
        std::string error = AIEditorFileDialog::GetLastError();
        if (!error.empty()) {
            SYSTEM_LOG << "[AIEditorGUI] ERROR: " << error << std::endl;
        }
    }
}

void AIEditorGUI::MenuAction_Save()
{
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    
    if (doc == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] No active graph to save" << std::endl;
        return;
    }
    
    NodeGraph::GraphId activeId = mgr.GetActiveGraphId();
    
    // Try to save with existing filepath
    // In a real implementation, this would use the graph's stored filepath
    // For now, save to a default location
    std::string filepath = "Blueprints/AI/autosave_bt.json";
    
    bool success = mgr.SaveGraph(activeId, filepath);
    
    if (success) {
        SYSTEM_LOG << "[AIEditorGUI] Saved to: " << filepath << std::endl;
    } else {
        SYSTEM_LOG << "[AIEditorGUI] ERROR: Save failed" << std::endl;
    }
}

void AIEditorGUI::MenuAction_SaveAs()
{
    SYSTEM_LOG << "[AIEditorGUI] Save As dialog" << std::endl;
    
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    
    if (doc == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] No active graph to save" << std::endl;
        return;
    }
    
    // Use default name for now (graph metadata filepath not yet implemented)
    std::string defaultName = "new_ai_graph.json";
    
    // Open native save dialog
    std::string filepath = AIEditorFileDialog::SaveFile("json,btree", m_lastSavePath, defaultName);
    
    if (!filepath.empty()) {
        // Ensure file has proper extension
        if (!EndsWith(filepath, ".json") && !EndsWith(filepath, ".btree")) {
            filepath += ".json";
        }
        
        // Save the graph
        NodeGraph::GraphId activeId = mgr.GetActiveGraphId();
        bool success = mgr.SaveGraph(activeId, filepath);
        
        if (success) {
            m_lastSavePath = ExtractDirectory(filepath);
            SYSTEM_LOG << "[AIEditorGUI] Saved graph: " << filepath << std::endl;
        }
        else {
            SYSTEM_LOG << "[AIEditorGUI] ERROR: Save failed" << std::endl;
        }
    }
    else {
        // Check if there was an error (not just cancel)
        std::string error = AIEditorFileDialog::GetLastError();
        if (!error.empty()) {
            SYSTEM_LOG << "[AIEditorGUI] ERROR: " << error << std::endl;
        }
    }
}

void AIEditorGUI::MenuAction_Close()
{
    SYSTEM_LOG << "[AIEditorGUI] Close current graph" << std::endl;
    
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphId activeId = mgr.GetActiveGraphId();
    if (activeId.value != 0) {
        mgr.CloseGraph(activeId);
    }
}

void AIEditorGUI::MenuAction_Undo()
{
    if (m_commandStack.CanUndo()) {
        m_commandStack.Undo();
        SYSTEM_LOG << "[AIEditorGUI] Undo: " << m_commandStack.GetUndoDescription() << std::endl;
    }
}

void AIEditorGUI::MenuAction_Redo()
{
    if (m_commandStack.CanRedo()) {
        m_commandStack.Redo();
        SYSTEM_LOG << "[AIEditorGUI] Redo: " << m_commandStack.GetRedoDescription() << std::endl;
    }
}

void AIEditorGUI::MenuAction_Cut()
{
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    if (doc == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] No active graph for cut" << std::endl;
        return;
    }
    
    // Get selected nodes
    std::vector<NodeGraph::NodeId> selectedNodes;
    for (size_t i = 0; i < m_selectedNodeIds.size(); ++i) {
        NodeGraph::NodeId nodeId;
        nodeId.value = static_cast<uint32_t>(m_selectedNodeIds[i]);
        selectedNodes.push_back(nodeId);
    }
    
    if (selectedNodes.empty()) {
        SYSTEM_LOG << "[AIEditorGUI] No nodes selected for cut" << std::endl;
        return;
    }
    
    // Cut via clipboard
    AIEditorClipboard::Get().Cut(selectedNodes, doc);
    
    // Clear selection
    m_selectedNodeIds.clear();
    
    SYSTEM_LOG << "[AIEditorGUI] Cut " << selectedNodes.size() << " nodes" << std::endl;
}

void AIEditorGUI::MenuAction_Copy()
{
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    if (doc == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] No active graph for copy" << std::endl;
        return;
    }
    
    // Get selected nodes
    std::vector<NodeGraph::NodeId> selectedNodes;
    for (size_t i = 0; i < m_selectedNodeIds.size(); ++i) {
        NodeGraph::NodeId nodeId;
        nodeId.value = static_cast<uint32_t>(m_selectedNodeIds[i]);
        selectedNodes.push_back(nodeId);
    }
    
    if (selectedNodes.empty()) {
        SYSTEM_LOG << "[AIEditorGUI] No nodes selected for copy" << std::endl;
        return;
    }
    
    // Copy via clipboard
    AIEditorClipboard::Get().Copy(selectedNodes, doc);
    
    SYSTEM_LOG << "[AIEditorGUI] Copied " << selectedNodes.size() << " nodes" << std::endl;
}

void AIEditorGUI::MenuAction_Paste()
{
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    if (doc == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] No active graph for paste" << std::endl;
        return;
    }
    
    if (AIEditorClipboard::Get().IsEmpty()) {
        SYSTEM_LOG << "[AIEditorGUI] Clipboard is empty" << std::endl;
        return;
    }
    
    // Paste with 50px offset
    Vector pasteOffset(50.0f, 50.0f);
    std::vector<NodeGraph::NodeId> newNodeIds = AIEditorClipboard::Get().Paste(doc, pasteOffset);
    
    // Select pasted nodes
    m_selectedNodeIds.clear();
    for (size_t i = 0; i < newNodeIds.size(); ++i) {
        m_selectedNodeIds.push_back(static_cast<int>(newNodeIds[i].value));
    }
    
    SYSTEM_LOG << "[AIEditorGUI] Pasted " << newNodeIds.size() << " nodes" << std::endl;
}

void AIEditorGUI::MenuAction_Delete()
{
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    if (doc == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] No active graph for delete" << std::endl;
        return;
    }
    
    if (m_selectedNodeIds.empty()) {
        SYSTEM_LOG << "[AIEditorGUI] No nodes selected for deletion" << std::endl;
        return;
    }
    
    // Delete selected nodes
    for (size_t i = 0; i < m_selectedNodeIds.size(); ++i) {
        NodeGraph::NodeId nodeId;
        nodeId.value = static_cast<uint32_t>(m_selectedNodeIds[i]);
        doc->DeleteNode(nodeId);
    }
    
    SYSTEM_LOG << "[AIEditorGUI] Deleted " << m_selectedNodeIds.size() << " nodes" << std::endl;
    
    // Clear selection
    m_selectedNodeIds.clear();
}

void AIEditorGUI::MenuAction_SelectAll()
{
    NodeGraph::NodeGraphManager& mgr = NodeGraph::NodeGraphManager::Get();
    NodeGraph::GraphDocument* doc = mgr.GetActiveGraph();
    if (doc == nullptr) {
        SYSTEM_LOG << "[AIEditorGUI] No active graph for select all" << std::endl;
        return;
    }
    
    // Clear current selection
    m_selectedNodeIds.clear();
    
    // Select all nodes
    const std::vector<NodeGraph::NodeData>& nodes = doc->GetNodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        m_selectedNodeIds.push_back(static_cast<int>(nodes[i].id.value));
    }
    
    SYSTEM_LOG << "[AIEditorGUI] Selected all " << m_selectedNodeIds.size() << " nodes" << std::endl;
}

void AIEditorGUI::MenuAction_ResetLayout()
{
    SYSTEM_LOG << "[AIEditorGUI] Reset Layout" << std::endl;
    
    // Reset panel visibility
    m_showNodePalette = true;
    m_showBlackboardPanel = false;
    m_showSensesPanel = false;
    m_showRuntimeDebugPanel = false;
}

void AIEditorGUI::MenuAction_ShowNodePalette()
{
    m_showNodePalette = !m_showNodePalette;
}

void AIEditorGUI::MenuAction_ShowBlackboard()
{
    m_showBlackboardPanel = !m_showBlackboardPanel;
}

void AIEditorGUI::MenuAction_ShowSensesPanel()
{
    m_showSensesPanel = !m_showSensesPanel;
}

void AIEditorGUI::MenuAction_ShowRuntimeDebug()
{
    m_showRuntimeDebugPanel = !m_showRuntimeDebugPanel;
}

void AIEditorGUI::MenuAction_About()
{
    SYSTEM_LOG << "[AIEditorGUI] About dialog (not yet implemented)" << std::endl;
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string AIEditorGUI::ExtractDirectory(const std::string& filepath)
{
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return filepath.substr(0, lastSlash);
    }
    return std::string();
}

std::string AIEditorGUI::ExtractFilename(const std::string& filepath)
{
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return filepath.substr(lastSlash + 1);
    }
    return filepath;
}

bool AIEditorGUI::EndsWith(const std::string& str, const std::string& suffix)
{
    if (suffix.size() > str.size()) {
        return false;
    }
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace AI
} // namespace Olympe
