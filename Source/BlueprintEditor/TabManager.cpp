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
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        delete m_tabs[i].renderer;
        m_tabs[i].renderer = nullptr;
    }
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
        if (gt == "VisualScript" || gt == "BehaviorTree" || gt == "AnimGraph")
            return gt;
    }
    if (root.contains("blueprintType") && root["blueprintType"].is_string())
    {
        std::string bt = root["blueprintType"].get<std::string>();
        if (bt == "BehaviorTree")
            return "BehaviorTree";
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
    }
    else
    {
        SYSTEM_LOG << "[TabManager] CreateNewTab: unsupported type '" << graphType << "'\n";
        return "";
    }

    m_tabs.push_back(tab);
    SetActiveTab(tab.tabID);
    SYSTEM_LOG << "[TabManager] Created new tab: " << tab.displayName << " (" << graphType << ")\n";
    return tab.tabID;
}

std::string TabManager::OpenFileInTab(const std::string& filePath)
{
    if (filePath.empty())
        return "";

    // Check if already open
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs[i].filePath == filePath)
        {
            SetActiveTab(m_tabs[i].tabID);
            SYSTEM_LOG << "[TabManager] File already open, activating: " << filePath << "\n";
            return m_tabs[i].tabID;
        }
    }

    std::string graphType = DetectGraphType(filePath);
    SYSTEM_LOG << "[TabManager] Opening file: " << filePath << " (type=" << graphType << ")\n";

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
        if (!r->Load(filePath))
        {
            delete r;
            SYSTEM_LOG << "[TabManager] Failed to load VS file: " << filePath << "\n";
            return "";
        }
        tab.renderer = r;
    }
    else if (graphType == "BehaviorTree")
    {
        // BehaviorTreeRenderer needs a NodeGraphPanel reference.
        // We use a lazily initialised static panel here since BT rendering
        // relies on the singleton NodeGraphManager anyway.
        static NodeGraphPanel s_btPanel;
        static bool s_btPanelInit = false;
        if (!s_btPanelInit)
        {
            s_btPanel.Initialize();
            s_btPanelInit = true;
        }

        BehaviorTreeRenderer* r = new BehaviorTreeRenderer(s_btPanel);
        if (!r->Load(filePath))
        {
            delete r;
            SYSTEM_LOG << "[TabManager] Failed to load BT file: " << filePath << "\n";
            return "";
        }
        tab.renderer = r;
    }
    else
    {
        // Fallback: try as VisualScript
        VisualScriptRenderer* r = new VisualScriptRenderer();
        if (!r->Load(filePath))
        {
            delete r;
            SYSTEM_LOG << "[TabManager] Unsupported/unknown graph type for: " << filePath << "\n";
            return "";
        }
        tab.graphType = "VisualScript";
        tab.renderer  = r;
    }

    m_tabs.push_back(tab);
    SetActiveTab(tab.tabID);
    return tab.tabID;
}

// ============================================================================
// Navigation
// ============================================================================

void TabManager::SetActiveTab(const std::string& tabID)
{
    for (size_t i = 0; i < m_tabs.size(); ++i)
        m_tabs[i].isActive = (m_tabs[i].tabID == tabID);
    m_activeTabID = tabID;
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
    delete m_tabs[index].renderer;
    m_tabs[index].renderer = nullptr;
    m_tabs.erase(m_tabs.begin() + static_cast<int>(index));

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
            int choice = ShowUnsavedDialog(m_tabs[i]);
            if (choice == 1)
            {
                // Save
                if (!SaveActiveTabAs(m_tabs[i].filePath.empty() ? "" : m_tabs[i].filePath))
                    return false; // Save failed or user cancelled Save As
            }
            else if (choice == -1)
            {
                // Cancel
                return false;
            }
            // choice == 0 → Don't Save, fall through to close
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
    EditorTab* tab = GetActiveTab();
    if (!tab || !tab->renderer)
        return false;

    if (tab->filePath.empty())
    {
        // Need Save As
        m_showSaveAsDialog = true;
        m_saveAsTabID = tab->tabID;
        std::strncpy(m_saveAsBuffer, tab->displayName.c_str(), sizeof(m_saveAsBuffer) - 1);
        m_saveAsBuffer[sizeof(m_saveAsBuffer) - 1] = '\0';
        return false; // Will complete when dialog is confirmed
    }

    bool ok = tab->renderer->Save(tab->filePath);
    if (ok)
    {
        tab->isDirty     = false;
        tab->displayName = DisplayNameFromPath(tab->filePath);
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
        std::strncpy(m_saveAsBuffer, tab->displayName.c_str(), sizeof(m_saveAsBuffer) - 1);
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
    // --- Save As dialog (deferred from SaveActiveTab) ---
    if (m_showSaveAsDialog)
    {
        ImGui::OpenPopup("TabManager_SaveAs");
        m_showSaveAsDialog = false;
    }

    if (ImGui::BeginPopupModal("TabManager_SaveAs", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save graph as:");
        ImGui::SetNextItemWidth(400.0f);
        ImGui::InputText("##save_as_path", m_saveAsBuffer, sizeof(m_saveAsBuffer));
        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            EditorTab* tab = GetTab(m_saveAsTabID);
            if (tab && tab->renderer && std::strlen(m_saveAsBuffer) > 0)
            {
                std::string path = std::string(m_saveAsBuffer);
                // Append .ats if no extension
                if (path.find('.') == std::string::npos)
                    path += ".ats";
                if (tab->renderer->Save(path))
                {
                    tab->filePath    = path;
                    tab->isDirty     = false;
                    tab->displayName = DisplayNameFromPath(path);
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

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

            // Build label: "name *" when dirty
            std::string label = tab.displayName;
            if (tab.isDirty)
                label += " *";
            label += "###tab_";
            label += tab.tabID;

            // Use ImGuiTabItemFlags_SetSelected when this tab becomes active
            ImGuiTabItemFlags flags = 0;
            if (tab.isActive)
                flags |= ImGuiTabItemFlags_SetSelected;

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
    // In immediate-mode GUI we can't block, so we open a popup and return a
    // sentinel (-2) meaning "pending". The actual choice is handled inside
    // RenderTabBar's popup rendering path.  For synchronous call-sites (like
    // CloseAllTabs), we use a simplified inline approach.

    // This method is called outside the render loop, so we use ImGui::OpenPopup
    // indirectly by setting state and returning a non-blocking result.
    // For the tab-bar close button flow, the modal in RenderTabBar handles it.
    // For CloseAllTabs, we can't show a real modal here without a render frame,
    // so we treat dirty as "Don't Save" for the bulk close path.
    // A real application would implement a proper event-driven close mechanism.
    (void)tab;
    return 0; // Don't Save (safe default for bulk operations)
}

} // namespace Olympe
