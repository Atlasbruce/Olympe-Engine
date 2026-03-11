/**
 * @file TabManager.h
 * @brief Central manager for the multi-graph tab system.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * TabManager maintains a list of EditorTab objects, each holding a pointer to
 * an IGraphRenderer.  It renders an ImGui tab bar and delegates canvas drawing
 * to the active tab's renderer.
 *
 * Usage (inside BlueprintEditorGUI):
 * @code
 *   TabManager::Get().RenderTabBar();
 *   TabManager::Get().RenderActiveCanvas();
 * @endcode
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include "IGraphRenderer.h"

#include <string>
#include <vector>

namespace Olympe {

// ============================================================================
// EditorTab
// ============================================================================

/**
 * @struct EditorTab
 * @brief Represents a single open graph in the editor.
 */
struct EditorTab
{
    std::string    tabID;       ///< Unique identifier (stringified int counter)
    std::string    displayName; ///< Label shown in the tab bar
    std::string    filePath;    ///< Full path, empty for unsaved graphs
    std::string    graphType;   ///< "VisualScript", "BehaviorTree", etc.
    bool           isDirty;     ///< True when there are unsaved changes
    bool           isActive;    ///< True when this is the currently selected tab

    IGraphRenderer* renderer;   ///< Owned pointer — deleted when tab is closed

    EditorTab()
        : isDirty(false)
        , isActive(false)
        , renderer(nullptr)
    {}
};

// ============================================================================
// TabManager
// ============================================================================

/**
 * @class TabManager
 * @brief Singleton that owns and manages all open graph editor tabs.
 */
class TabManager
{
public:
    ~TabManager();

    /** @brief Returns the global singleton instance. */
    static TabManager& Get();

    // ------------------------------------------------------------------
    // Tab creation / opening
    // ------------------------------------------------------------------

    /**
     * @brief Creates a new empty tab of the given graph type.
     * @param graphType  "VisualScript" or "BehaviorTree".
     * @return The new tab's tabID, or empty string on failure.
     */
    std::string CreateNewTab(const std::string& graphType);

    /**
     * @brief Opens a file in a new tab.
     * If the file is already open, activates that tab instead.
     * @param filePath  Full path to the .ats / .json file.
     * @return The tab's tabID, or empty string on failure.
     */
    std::string OpenFileInTab(const std::string& filePath);

    // ------------------------------------------------------------------
    // Navigation
    // ------------------------------------------------------------------

    void        SetActiveTab(const std::string& tabID);
    std::string GetActiveTabID() const;
    EditorTab*  GetActiveTab();
    EditorTab*  GetTab(const std::string& tabID);

    const std::vector<EditorTab>& GetAllTabs() const { return m_tabs; }

    // ------------------------------------------------------------------
    // Closing
    // ------------------------------------------------------------------

    /**
     * @brief Closes the given tab.
     * Shows an "unsaved changes" dialog when the tab is dirty.
     * @return false if the user chose Cancel.
     */
    bool CloseTab(const std::string& tabID);

    /**
     * @brief Attempts to close all tabs.
     * @return false if the user cancelled at any point.
     */
    bool CloseAllTabs();

    // ------------------------------------------------------------------
    // Saving
    // ------------------------------------------------------------------

    /** @brief Saves the active tab.  Opens "Save As" if no path is set. */
    bool SaveActiveTab();

    /**
     * @brief Saves the active tab to a specific path.
     * @return true on success.
     */
    bool SaveActiveTabAs(const std::string& path);

    // ------------------------------------------------------------------
    // Rendering
    // ------------------------------------------------------------------

    /** @brief Renders the horizontal tab bar (call before RenderActiveCanvas). */
    void RenderTabBar();

    /** @brief Renders the graph canvas of the active tab. */
    void RenderActiveCanvas();

    // ------------------------------------------------------------------
    // Query helpers
    // ------------------------------------------------------------------

    /** @brief Returns true when at least one tab has unsaved changes. */
    bool HasDirtyTabs() const;

    /** @brief Returns true when there are no open tabs. */
    bool IsEmpty() const { return m_tabs.empty(); }

private:
    TabManager();

    // Disable copy
    TabManager(const TabManager&);
    TabManager& operator=(const TabManager&);

    /**
     * @brief Shows a 3-button "Save / Don't Save / Cancel" modal dialog.
     * @param tab  Tab whose unsaved state is being queried.
     * @return  1 = Save, 0 = Don't Save, -1 = Cancel.
     */
    int ShowUnsavedDialog(const EditorTab& tab);

    /** @brief Shows the "Save As" input dialog for a tab without a path. */
    bool ShowSaveAsDialog(EditorTab* tab);

    /**
     * @brief Detects the graph type by inspecting the JSON contents.
     * @return "VisualScript", "BehaviorTree", or "Unknown".
     */
    static std::string DetectGraphType(const std::string& filePath);

    /** @brief Generates the next unique tabID. */
    std::string NextTabID();

    /** @brief Derives a display name from a file path (filename without dir). */
    static std::string DisplayNameFromPath(const std::string& filePath);

    /** @brief Deletes a renderer and removes its tab entry. */
    void DestroyTab(size_t index);

    // ------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------

    std::vector<EditorTab> m_tabs;
    std::string            m_activeTabID;
    int                    m_nextTabNum;      ///< Counter for "Untitled-N" names
    int                    m_nextTabIDNum;    ///< Counter for unique tab IDs

    // Pending close state (for deferred close when dialog is shown)
    std::string m_pendingCloseTabID;

    // Save As dialog state
    bool m_showSaveAsDialog;
    char m_saveAsBuffer[512];
    std::string m_saveAsTabID;
};

} // namespace Olympe
