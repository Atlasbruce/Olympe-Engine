/**
 * @file TabManager.cpp
 * @brief Implementation of the central multi-graph tab manager.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details C++14 compliant.
 */

#include "TabManager.h"
#include "VisualScriptRenderer.h"
#include "BehaviorTreeRenderer.h"
#include "NodeGraphPanel.h"
#include "PlaceholderEditor/PlaceholderGraphRenderer.h"
#include "PlaceholderEditor/PlaceholderGraphDocument.h"
#include "PlaceholderEditor/PlaceholderCanvas.h"
#include "Framework/VisualScriptGraphDocument.h"
#include "Framework/BehaviorTreeGraphDocument.h"
#include "Framework/CanvasModalRenderer.h"
#include "Framework/DocumentVersionManager.h"
#include "../DataManager.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/nlohmann/json.hpp"
#include "../system/system_utils.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

TabManager& TabManager::Get()
{
    static TabManager instance;
    return instance;
}

TabManager::TabManager()
    : m_nextTabNum(1)
    , m_nextTabIDNum(1)
    , m_showSaveAsDialog(false)
{
    std::memset(m_saveAsBuffer, 0, sizeof(m_saveAsBuffer));
}

TabManager::~TabManager()
{
    // EditorTab destructors now properly handle renderer/document cleanup
    // via explicit move semantics that transfer ownership
    m_tabs.clear();
}

// ============================================================================
// ID / Name helpers
// ============================================================================

std::string TabManager::NextTabID()
{
    std::ostringstream oss;
    oss << "tab_" << m_nextTabIDNum++;
    return oss.str();
}

std::string TabManager::DisplayNameFromPath(const std::string& filePath)
{
    if (filePath.empty())
        return "";

    // Find last separator
    size_t pos = filePath.find_last_of("/\\");
    if (pos == std::string::npos)
        return filePath;
    return filePath.substr(pos + 1);
}

// ============================================================================
// Graph type detection
// ============================================================================

std::string TabManager::DetectGraphType(const std::string& filePath)
{
    std::ifstream ifs(filePath.c_str());
    if (!ifs.good())
        return "Unknown";

    nlohmann::json root;
    try
    {
        ifs >> root;
    }
    catch (...)
    {
        return "Unknown";
    }

    if (!root.is_object())
        return "Unknown";

    // Explicit type fields
    if (root.contains("graphType") && root["graphType"].is_string())
    {
        std::string gt = root["graphType"].get<std::string>();
        if (gt == "VisualScript" || gt == "BehaviorTree" || gt == "AnimGraph" || gt == "Placeholder")
            return gt;
    }
    if (root.contains("blueprintType") && root["blueprintType"].is_string())
    {
        std::string bt = root["blueprintType"].get<std::string>();
        if (bt == "BehaviorTree")
            return "BehaviorTree";
        if (bt == "EntityPrefab")
            return "EntityPrefab";
    }

    // Structural heuristics
    int schemaVersion = 0;
    if (root.contains("schema_version") && root["schema_version"].is_number())
        schemaVersion = root["schema_version"].get<int>();

    if (schemaVersion == 4)
    {
        if (root.contains("nodes") && root.contains("execConnections"))
            return "VisualScript";
    }

    if (root.contains("rootNodeId") && root.contains("nodes"))
        return "BehaviorTree";

    if (root.contains("states") && root.contains("transitions"))
        return "AnimGraph";

    // Legacy BT v2 format
    if (root.contains("blueprintType"))
        return "BehaviorTree";

    return "Unknown";
}

// ============================================================================
// Tab creation
// ============================================================================

std::string TabManager::CreateNewTab(const std::string& graphType)
{
    std::ostringstream nameSS;
    nameSS << "Untitled-" << m_nextTabNum++;

    EditorTab tab;
    tab.tabID       = NextTabID();
    tab.displayName = nameSS.str();
    tab.filePath    = "";
    tab.graphType   = graphType;
    tab.isDirty     = false;
    tab.isActive    = false;

    if (graphType == "VisualScript")
    {
        VisualScriptRenderer* r = new VisualScriptRenderer();
        tab.renderer = r;

        // Phase 44.2: FIX - Use the renderer's internal document adapter
        // instead of creating a new VisualScriptGraphDocument.
        // This ensures framework and tab system use the same document object.
        tab.document = r->GetDocument();

        SYSTEM_LOG << "[TabManager::CreateNewTab] Created new VisualScript tab with document adapter\n";
    }
    else if (graphType == "EntityPrefab")
    {
        // PHASE 60: Use DocumentVersionManager + EntityPrefabEditorV2 Framework for consistency
        // This matches OpenFileInTab pattern, ensuring unified architecture for "New" and "Open" paths

        DocumentVersionManager& docManager = DocumentVersionManager::Get();

        SYSTEM_LOG << "[TabManager::CreateNewTab] Creating EntityPrefab via DocumentVersionManager\n";

        // Create document through framework factory
        IGraphDocument* doc = docManager.CreateNewDocument("EntityPrefab");
        if (!doc)
        {
            SYSTEM_LOG << "[TabManager::CreateNewTab] ERROR: DocumentVersionManager::CreateNewDocument() failed\n";
            return "";
        }
        SYSTEM_LOG << "[TabManager::CreateNewTab] Document created successfully\n";

        // Create renderer through framework factory
        IGraphRenderer* renderer = docManager.CreateRenderer("EntityPrefab", doc);
        if (!renderer)
        {
            SYSTEM_LOG << "[TabManager::CreateNewTab] ERROR: DocumentVersionManager::CreateRenderer() failed\n";
            delete doc;
            return "";
        }
        SYSTEM_LOG << "[TabManager::CreateNewTab] Renderer created successfully\n";

        tab.renderer = renderer;
        tab.document = doc;
        SYSTEM_LOG << "[TabManager::CreateNewTab] EntityPrefab tab fully initialized via framework\n";
    }
    else if (graphType == "BehaviorTree")
    {
        static NodeGraphPanel s_btPanel;
        static bool s_btPanelInit = false;
        if (!s_btPanelInit)
        {
            s_btPanel.Initialize();
            s_btPanelInit = true;
        }

        BehaviorTreeRenderer* r = new BehaviorTreeRenderer(s_btPanel);
        // Create new empty graph immediately so canvas appears on tab creation
        r->CreateNew(nameSS.str());
        tab.renderer = r;

        // Phase 44.2: FIX - Use the renderer's internal document adapter
        // instead of creating a new BehaviorTreeGraphDocument.
        // This ensures framework and tab system use the same document object.
        tab.document = r->GetDocument();

        SYSTEM_LOG << "[TabManager::CreateNewTab] Created new BehaviorTree tab with document adapter\n";
    }
    else if (graphType == "Placeholder")
    {
        // Phase 2: Placeholder test type for framework validation
        // NOTE: Don't create document/canvas here - let PlaceholderGraphRenderer handle it in CreateNewGraph()
        PlaceholderGraphRenderer* r = new PlaceholderGraphRenderer();
        r->CreateNewGraph();  // Initialize with sample data (creates document + canvas internally)

        tab.renderer = r;
        tab.document = r->GetDocument();  // Get the document created by renderer

        SYSTEM_LOG << "[TabManager::CreateNewTab] Created new Placeholder tab with sample graph\n";
    }
    else
    {
        // Unsupported graph type
        return "";
    }

    m_tabs.emplace_back(std::move(tab)); //m_tabs.push_back(tab);
    SetActiveTab(tab.tabID);
    //SYSTEM_LOG << "[TabManager] Created new tab: " << tab.displayName << " (" << graphType << ")\n";
    return tab.tabID;
}

std::string TabManager::OpenFileInTab(const std::string& filePath)
{
    if (filePath.empty())
        return "";

    // PHASE 51: LOAD CACHING - Check if already open
    SYSTEM_LOG << "[TabManager::OpenFileInTab] ENTRY: filePath=" << filePath << "\n";

    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs[i].filePath == filePath)
        {
            SYSTEM_LOG << "[TabManager::OpenFileInTab] File already open in tab: " << m_tabs[i].tabID << ", activating\n";
            SetActiveTab(m_tabs[i].tabID);
            return m_tabs[i].tabID;
        }
    }

    std::string graphType = DetectGraphType(filePath);
    SYSTEM_LOG << "[TabManager::OpenFileInTab] File not open, detected type: " << graphType << "\n";

    EditorTab tab;
    tab.tabID       = NextTabID();
    tab.displayName = DisplayNameFromPath(filePath);
    tab.filePath    = filePath;
    tab.graphType   = graphType;
    tab.isDirty     = false;
    tab.isActive    = false;

    if (graphType == "VisualScript")
    {
        VisualScriptRenderer* r = new VisualScriptRenderer();
        SYSTEM_LOG << "[TabManager::OpenFileInTab] VisualScriptRenderer created for tab: " << tab.tabID << "\n";

        if (!r->Load(filePath))
        {
            SYSTEM_LOG << "[TabManager::OpenFileInTab] ERROR: VisualScriptRenderer::Load() failed for " << filePath << "\n";
            delete r;
            return "";
        }
        SYSTEM_LOG << "[TabManager::OpenFileInTab] VisualScriptRenderer::Load() SUCCESS\n";
        tab.renderer = r;

        // Phase 44.2: FIX - Use the renderer's internal document adapter
        // instead of creating a new VisualScriptGraphDocument.
        // This ensures framework and tab system use the same document object.
        tab.document = r->GetDocument();
    }
    else if (graphType == "BehaviorTree")
    {
        // BehaviorTreeRenderer needs a NodeGraphPanel reference.
        // The editor is single-threaded, so the static is safe here.
        // All BT tabs share the same backing panel since they rely on the
        // singleton NodeGraphManager; each renderer switches the active graph.
        static NodeGraphPanel s_btPanel;
        static bool s_btPanelInit = false;
        if (!s_btPanelInit)
        {
            s_btPanel.Initialize();
            s_btPanelInit = true;
        }

        BehaviorTreeRenderer* r = new BehaviorTreeRenderer(s_btPanel);
        SYSTEM_LOG << "[TabManager::OpenFileInTab] BehaviorTreeRenderer created for tab: " << tab.tabID << "\n";

        if (!r->Load(filePath))
        {
            SYSTEM_LOG << "[TabManager::OpenFileInTab] ERROR: BehaviorTreeRenderer::Load() failed for " << filePath << "\n";
            delete r;
            return "";
        }
        SYSTEM_LOG << "[TabManager::OpenFileInTab] BehaviorTreeRenderer::Load() SUCCESS\n";

        tab.renderer = r;

        // Phase 44.2: FIX - Use the renderer's internal document adapter
        // instead of creating a new BehaviorTreeGraphDocument.
        // This ensures framework and tab system use the same document object.
        tab.document = r->GetDocument();
        SYSTEM_LOG << "[TabManager::OpenFileInTab] BehaviorTree document assigned to tab\n";
    }
    else if (graphType == "EntityPrefab")
    {
        // PHASE 2.1 CHUNK 3: Use DocumentVersionManager + EntityPrefabEditorV2 Framework
        // Load through centralized routing system instead of creating renderer directly
        // This enables fallback, version control, and consistent architecture

        DocumentVersionManager& docManager = DocumentVersionManager::Get();

        SYSTEM_LOG << "[TabManager::OpenFileInTab] Loading EntityPrefab via DocumentVersionManager\n";

        // Load document through framework
        IGraphDocument* doc = docManager.LoadDocument("EntityPrefab", filePath);
        if (!doc)
        {
            SYSTEM_LOG << "[TabManager::OpenFileInTab] ERROR: DocumentVersionManager::LoadDocument() failed\n";
            return "";
        }
        SYSTEM_LOG << "[TabManager::OpenFileInTab] Document loaded successfully\n";

        // Create renderer through framework factory
        IGraphRenderer* renderer = docManager.CreateRenderer("EntityPrefab", doc);
        if (!renderer)
        {
            SYSTEM_LOG << "[TabManager::OpenFileInTab] ERROR: DocumentVersionManager::CreateRenderer() failed\n";
            delete doc;
            return "";
        }
        SYSTEM_LOG << "[TabManager::OpenFileInTab] Renderer created successfully\n";

        tab.renderer = renderer;
        tab.document = doc;
        SYSTEM_LOG << "[TabManager::OpenFileInTab] EntityPrefab tab fully initialized via framework\n";
    }
    else if (graphType == "Placeholder")
    {
        // Phase 2: Placeholder test type - parallel development
        // NOTE: Don't create document/canvas here. Let Load() handle it.
        // This pattern mirrors VisualScriptRenderer which manages its own document.
        PlaceholderGraphRenderer* r = new PlaceholderGraphRenderer();
        SYSTEM_LOG << "[TabManager::OpenFileInTab] PlaceholderGraphRenderer created for tab: " << tab.tabID << "\n";

        if (!r->Load(filePath))
        {
            SYSTEM_LOG << "[TabManager::OpenFileInTab] ERROR: PlaceholderGraphRenderer::Load() failed for " << filePath << "\n";
            delete r;
            return "";
        }
        SYSTEM_LOG << "[TabManager::OpenFileInTab] PlaceholderGraphRenderer::Load() SUCCESS\n";

        tab.renderer = r;
        tab.document = r->GetDocument();  // Use renderer's document like VisualScriptRenderer does
        SYSTEM_LOG << "[TabManager::OpenFileInTab] Placeholder document assigned to tab\n";
    }
    else
    {
        // Fallback: try as VisualScript
        VisualScriptRenderer* r = new VisualScriptRenderer();
        SYSTEM_LOG << "[TabManager::OpenFileInTab] Fallback: VisualScriptRenderer created for tab: " << tab.tabID << "\n";

        if (!r->Load(filePath))
        {
            SYSTEM_LOG << "[TabManager::OpenFileInTab] ERROR: Fallback VisualScriptRenderer::Load() failed for " << filePath << "\n";
            delete r;
            return "";
        }
        SYSTEM_LOG << "[TabManager::OpenFileInTab] Fallback: VisualScriptRenderer::Load() SUCCESS\n";

        tab.graphType = "VisualScript";
        tab.renderer  = r;

        // Use the renderer's internal document adapter for consistency
        tab.document = r->GetDocument();
    }

    // CRITICAL FIX: Save tabID BEFORE move, as accessing tab.tabID after std::move() is undefined behavior
    std::string tabIDToReturn = tab.tabID;
    SYSTEM_LOG << "[TabManager::OpenFileInTab] DEBUG: About to add tab to m_tabs, tabID=" << tabIDToReturn << ", filePath=" << filePath << ", graphType=" << graphType << "\n";

    m_tabs.emplace_back(std::move(tab)); //m_tabs.push_back(tab);
    SYSTEM_LOG << "[TabManager::OpenFileInTab] DEBUG: Tab successfully added to m_tabs, total tabs now: " << m_tabs.size() << "\n";

    SetActiveTab(tabIDToReturn);
    SYSTEM_LOG << "[TabManager::OpenFileInTab] SUCCESS: Returning tabID=" << tabIDToReturn << "\n";
    return tabIDToReturn;
}

// ============================================================================
// Navigation
// ============================================================================

void TabManager::SetActiveTab(const std::string& tabID)
{
    // Phase 35.0: Save previous tab's canvas state before switching
    EditorTab* previousTab = GetActiveTab();
    if (previousTab && previousTab->renderer)
    {
        previousTab->renderer->SaveCanvasState();
    }

    // Update active tab
    for (size_t i = 0; i < m_tabs.size(); ++i)
        m_tabs[i].isActive = (m_tabs[i].tabID == tabID);
    m_activeTabID = tabID;

    // Request a one-shot programmatic selection for this tab.
    // The flag is consumed (cleared) during the very next RenderTabBar() call
    // so that subsequent user-initiated tab clicks are not overridden.
    m_pendingSelectTabID = tabID;

    // Phase 35.0: Restore new tab's canvas state after switching
    EditorTab* newTab = GetActiveTab();
    if (newTab && newTab->renderer)
    {
        newTab->renderer->RestoreCanvasState();
    }
}

std::string TabManager::GetActiveTabID() const
{
    return m_activeTabID;
}

EditorTab* TabManager::GetActiveTab()
{
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs[i].tabID == m_activeTabID)
            return &m_tabs[i];
    }
    return nullptr;
}

EditorTab* TabManager::GetTab(const std::string& tabID)
{
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs[i].tabID == tabID)
            return &m_tabs[i];
    }
    return nullptr;
}

// ============================================================================
// Closing
// ============================================================================

void TabManager::DestroyTab(size_t index)
{
    if (index >= m_tabs.size())
        return;

    const std::string closedID = m_tabs[index].tabID;
    // PHASE 55 FIX: Removed explicit 'delete renderer' - EditorTab destructor handles all cleanup
    // when m_tabs.erase() is called. Explicit deletion was causing double-delete of renderer
    // and potentially corrupting the document pointer before destructor tried to delete it.
    // The EditorTab move semantics and destructor properly manage ownership transfer.
    m_tabs.erase(m_tabs.begin() + static_cast<std::vector<EditorTab>::difference_type>(index));

    // Update active tab
    if (m_activeTabID == closedID)
    {
        if (!m_tabs.empty())
        {
            size_t newActive = (index < m_tabs.size()) ? index : m_tabs.size() - 1;
            m_activeTabID = m_tabs[newActive].tabID;
            m_tabs[newActive].isActive = true;
        }
        else
        {
            m_activeTabID = "";
        }
    }
}

bool TabManager::CloseTab(const std::string& tabID)
{
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs[i].tabID != tabID)
            continue;

        // Sync dirty flag from renderer
        if (m_tabs[i].renderer)
            m_tabs[i].isDirty = m_tabs[i].renderer->IsDirty();

        if (m_tabs[i].isDirty)
        {
            // Dirty tabs get a deferred modal handled by RenderTabBar().
            // Set m_pendingCloseTabID so the modal fires on the next frame.
            if (m_pendingCloseTabID.empty())
                m_pendingCloseTabID = tabID;
            return false; // Actual close deferred to modal callback
        }

        DestroyTab(i);
        return true;
    }
    return true;
}

bool TabManager::CloseAllTabs()
{
    // Iterate from back to front so indices don't shift
    for (int i = static_cast<int>(m_tabs.size()) - 1; i >= 0; --i)
    {
        if (m_tabs[i].renderer)
            m_tabs[i].isDirty = m_tabs[i].renderer->IsDirty();

        if (m_tabs[i].isDirty)
        {
            SetActiveTab(m_tabs[i].tabID);
            int choice = ShowUnsavedDialog(m_tabs[i]);
            if (choice == 1)
            {
                if (!SaveActiveTabAs(m_tabs[i].filePath))
                    return false;
            }
            else if (choice == -1)
            {
                return false;
            }
        }

        DestroyTab(static_cast<size_t>(i));
    }
    return true;
}

// ============================================================================
// Saving
// ============================================================================

bool TabManager::SaveActiveTab()
{
    SYSTEM_LOG << "[TabManager::SaveActiveTab] ENTER\n";
    EditorTab* tab = GetActiveTab();
    if (!tab || !tab->renderer)
    {
        SYSTEM_LOG << "[TabManager::SaveActiveTab] EXIT: No active tab or renderer\n";
        return false;
    }

    SYSTEM_LOG << "[TabManager::SaveActiveTab] Active tab: " << tab->displayName << " (type: " << tab->graphType << ")\n";

    if (tab->filePath.empty())
    {
        // Need Save As
        m_showSaveAsDialog = true;
        m_saveAsTabID = tab->tabID;
        strncpy_s(m_saveAsBuffer, sizeof(m_saveAsBuffer), tab->displayName.c_str(), _TRUNCATE);
        m_saveAsBuffer[sizeof(m_saveAsBuffer) - 1] = '\0';
        return false; // Will complete when dialog is confirmed
    }

    //SYSTEM_LOG << "[TabManager] SaveActiveTab: saving tab '" << tab->displayName
    //           << "' to '" << tab->filePath << "'\n";
    SYSTEM_LOG << "[TabManager::SaveActiveTab] Calling tab->renderer->Save() for: " << tab->filePath << "\n";
    bool ok = tab->renderer->Save(tab->filePath);
    SYSTEM_LOG << "[TabManager::SaveActiveTab] Save result: " << (ok ? "SUCCESS" : "FAILED") << "\n";
    if (ok)
    {
        tab->isDirty     = false;
        tab->displayName = DisplayNameFromPath(tab->filePath);
        //SYSTEM_LOG << "[TabManager] SaveActiveTab: succeeded for '" << tab->filePath << "'\n";
    }
    else
    {
        //SYSTEM_LOG << "[TabManager] SaveActiveTab: FAILED for '" << tab->filePath << "'\n";
    }
    return ok;
}

bool TabManager::SaveActiveTabAs(const std::string& path)
{
    EditorTab* tab = GetActiveTab();
    if (!tab || !tab->renderer)
        return false;

    if (path.empty())
    {
        // Show save-as dialog
        m_showSaveAsDialog = true;
        m_saveAsTabID = tab->tabID;
        strncpy_s(m_saveAsBuffer, sizeof(m_saveAsBuffer), tab->displayName.c_str(), _TRUNCATE);
        m_saveAsBuffer[sizeof(m_saveAsBuffer) - 1] = '\0';
        return false;
    }

    bool ok = tab->renderer->Save(path);
         if (ok)
         {
             tab->filePath    = path;
             tab->isDirty     = false;
             tab->displayName = DisplayNameFromPath(path);
         }
         return ok;
     }

    // Phase 44.2.1: Unified save notification for framework
    void TabManager::OnGraphDocumentSaved(IGraphDocument* document, const std::string& filePath)
    {
        if (!document || filePath.empty())
        {
            return;
        }

        // Find the tab containing this document and update it
        for (size_t i = 0; i < m_tabs.size(); ++i)
        {
            if (m_tabs[i].document == document)
            {
                m_tabs[i].filePath = filePath;
                m_tabs[i].isDirty = false;
                m_tabs[i].displayName = DisplayNameFromPath(filePath);
                SYSTEM_LOG << "[TabManager] Updated tab display name to: " << m_tabs[i].displayName << "\n";
                break;
            }
        }
    }

     // ============================================================================
     // Query
     // ============================================================================

bool TabManager::HasDirtyTabs() const
{
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs[i].isDirty)
            return true;
        if (m_tabs[i].renderer && m_tabs[i].renderer->IsDirty())
            return true;
    }
    return false;
}

// ============================================================================
// Rendering
// ============================================================================

void TabManager::RenderTabBar()
{
    // --- Unsaved dialog (deferred) ---
    if (!m_pendingCloseTabID.empty())
    {
        ImGui::OpenPopup("TabManager_UnsavedClose");
    }

    if (ImGui::BeginPopupModal("TabManager_UnsavedClose", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        EditorTab* tab = GetTab(m_pendingCloseTabID);
        if (tab)
        {
            ImGui::Text("The graph \"%s\" has unsaved changes.",
                        tab->displayName.c_str());
            ImGui::Text("Do you want to save before closing?");
            ImGui::Separator();

            if (ImGui::Button("Save", ImVec2(100, 0)))
            {
                SetActiveTab(m_pendingCloseTabID);
                SaveActiveTab();
                // Close after saving
                for (size_t i = 0; i < m_tabs.size(); ++i)
                {
                    if (m_tabs[i].tabID == m_pendingCloseTabID)
                    {
                        DestroyTab(i);
                        break;
                    }
                }
                m_pendingCloseTabID = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Don't Save", ImVec2(100, 0)))
            {
                for (size_t i = 0; i < m_tabs.size(); ++i)
                {
                    if (m_tabs[i].tabID == m_pendingCloseTabID)
                    {
                        DestroyTab(i);
                        break;
                    }
                }
                m_pendingCloseTabID = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                m_pendingCloseTabID = "";
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            m_pendingCloseTabID = "";
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // --- Tab bar ---
    if (m_tabs.empty())
    {
        ImGui::TextDisabled("No graph open. Double-click a file to open it.");
        return;
    }

    // Handle centralized SaveAs modal (Phase 40 Part 5 - Browse Centralization)
    if (m_showSaveAsDialog)
    {
        DataManager& dm = DataManager::Get();
        EditorTab* tab = GetActiveTab();
        if (tab)
        {
            std::string suggestedName = tab->displayName;
            // Remove asterisk if present
            size_t asterisk = suggestedName.find('*');
            if (asterisk != std::string::npos)
                suggestedName = suggestedName.substr(0, asterisk);

            // Determine file type for SaveFilePickerModal
            std::string graphType = DetectGraphType(tab->filePath.empty() ? "" : tab->filePath);
            Olympe::SaveFileType fileType = Olympe::SaveFileType::Blueprint; // Default
            if (graphType == "BehaviorTree")
                fileType = Olympe::SaveFileType::BehaviorTree;
            else if (graphType == "VisualScript")
                fileType = Olympe::SaveFileType::Blueprint;
            else if (graphType == "EntityPrefab")
                fileType = Olympe::SaveFileType::EntityPrefab;

            // Open centralized modal
            dm.OpenSaveFilePickerModal(fileType, "Gamedata/", suggestedName);
            m_showSaveAsDialog = false;
        }
    }

    // Phase 44.2 FIX: Render centralized save modal via CanvasModalRenderer (unified dispatcher)
    // Previously used DataManager::RenderSaveFilePickerModal() which caused desynchronization
    // because CanvasToolbarRenderer opens modal via CanvasModalRenderer dispatcher (Phase 44.1)
    CanvasModalRenderer::Get().RenderSaveFilePickerModal();

    // Handle SaveAs result from unified dispatcher
    if (!CanvasModalRenderer::Get().IsSaveFileModalOpen()) {
        std::string selectedFile = CanvasModalRenderer::Get().GetSelectedSaveFilePath();
        if (!selectedFile.empty()) {
            EditorTab* tab = GetActiveTab();
            if (tab && tab->renderer) {
                if (tab->renderer->Save(selectedFile)) {
                    tab->filePath = selectedFile;
                    tab->isDirty = false;
                    tab->displayName = DisplayNameFromPath(selectedFile);
                    SYSTEM_LOG << "[TabManager] SaveAs: saved to '" << selectedFile << "'\n";
                    CanvasModalRenderer::Get().CloseSaveFileModal();
                } else {
                    SYSTEM_LOG << "[TabManager] SaveAs: FAILED to save to '" << selectedFile << "'\n";
                }
            }
        }
    }

    // Phase 42: Render unified SubGraph file picker modal (centralized rendering point)
    // This ensures all three editors (VisualScript, BehaviorTree, EntityPrefab) use
    // the same modal rendering, preventing duplicate zones and inconsistent behavior
    CanvasModalRenderer::Get().RenderSubGraphFilePickerModal();

    ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_Reorderable |
                                   ImGuiTabBarFlags_AutoSelectNewTabs;

    if (ImGui::BeginTabBar("GraphTabBar", tabBarFlags))
    {
        for (size_t i = 0; i < m_tabs.size(); ++i)
        {
            EditorTab& tab = m_tabs[i];

            // Sync dirty flag from renderer
            if (tab.renderer)
                tab.isDirty = tab.renderer->IsDirty();

            // Sync file path and display name from renderer.
            // This covers the case where the panel's own Save/Save As dialog
            // was used directly (not via TabManager::SaveActiveTab).
            if (tab.renderer)
            {
                std::string rendererPath = tab.renderer->GetCurrentPath();
                if (!rendererPath.empty() && rendererPath != tab.filePath)
                {
                    tab.filePath    = rendererPath;
                    tab.displayName = DisplayNameFromPath(rendererPath);
                }
            }

            // Build label: "name *" when dirty
            std::string label = tab.displayName;
            if (tab.isDirty)
                label += " *";
            label += "###tab_";
            label += tab.tabID;

            // Apply ImGuiTabItemFlags_SetSelected only once (one-shot) when
            // this tab was programmatically activated.  The flag is cleared
            // immediately after use so that subsequent user-initiated tab
            // clicks are not suppressed.
            ImGuiTabItemFlags flags = 0;
            if (tab.tabID == m_pendingSelectTabID)
            {
                flags |= ImGuiTabItemFlags_SetSelected;
                m_pendingSelectTabID = ""; // consume the one-shot request
            }

            // Close button
            bool open = true;
            if (ImGui::BeginTabItem(label.c_str(), &open, flags))
            {
                if (m_activeTabID != tab.tabID)
                    SetActiveTab(tab.tabID);
                ImGui::EndTabItem();
            }

            // Handle close via X button
            if (!open && m_pendingCloseTabID.empty())
            {
                if (tab.isDirty)
                {
                    // Defer to modal dialog
                    m_pendingCloseTabID = tab.tabID;
                }
                else
                {
                    DestroyTab(i);
                    break; // m_tabs may be smaller now
                }
            }
        }

        // "+" button for new VS graph
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing |
                                       ImGuiTabItemFlags_NoTooltip))
        {
            CreateNewTab("VisualScript");
        }

        ImGui::EndTabBar();
    }
}

void TabManager::RenderActiveCanvas()
{
    EditorTab* tab = GetActiveTab();
    if (!tab || !tab->renderer)
    {
        ImGui::TextDisabled("No graph open.");
        return;
    }
    tab->renderer->Render();
}

// ============================================================================
// Dialogs
// ============================================================================

int TabManager::ShowUnsavedDialog(const EditorTab& tab)
{
    // NOTE: ImGui is immediate-mode; true blocking modal dialogs cannot be
    // shown outside a render frame.  Interactive unsaved-change prompts are
    // handled by the deferred modal inside RenderTabBar() (triggered by the
    // [X] close button).
    //
    // This fallback is only reached by CloseAllTabs(), which is called from
    // keyboard shortcuts and File > Close All.  In that context there is no
    // safe way to block for user input, so we discard unsaved changes.
    // CloseTab() (single tab) always uses the deferred RenderTabBar modal and
    // therefore never calls this method for dirty tabs.
    (void)tab;
    return 0; // Don't Save
}

} // namespace Olympe
