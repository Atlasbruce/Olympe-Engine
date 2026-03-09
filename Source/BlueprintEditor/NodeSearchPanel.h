/**
 * @file NodeSearchPanel.h
 * @brief Quick node-add search panel with fuzzy matching (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * NodeSearchPanel provides a floating search panel that the user can open
 * with the Space key.  It performs fuzzy scoring against a catalogue of
 * available node types and reports the user's choice via a callback.
 *
 * The implementation has no ImGui dependency — it is a pure data layer.
 * UI code calls OpenSearch(), UpdateQuery(), and reads GetResults().
 * When the user accepts a result, it calls ConfirmSelection() which
 * fires the OnNodeAdd callback.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Olympe {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @struct NodeSearchResult
 * @brief A single search result entry from the panel.
 */
struct NodeSearchResult {
    std::string name;           ///< Display name of the node type
    std::string category;       ///< Category label (e.g. "ControlFlow")
    std::string description;    ///< Short human-readable description
    std::string nodeType;       ///< Internal type key (e.g. "Sequence")
    float       relevanceScore; ///< Higher = better match
};

// ============================================================================
// NodeSearchPanel
// ============================================================================

/**
 * @class NodeSearchPanel
 * @brief Singleton fuzzy-search panel for adding nodes to the graph.
 *
 * Typical usage:
 * @code
 *   auto& sp = NodeSearchPanel::Get();
 *   sp.SetNodeAddCallback([](const std::string& type, float x, float y) {
 *       graph.AddNode(type, x, y);
 *   });
 *
 *   // On Space key:
 *   sp.OpenSearch(mouseX, mouseY);
 *
 *   // Each frame the panel is open:
 *   sp.UpdateQuery(currentSearchText);
 *   for (const auto& r : sp.GetResults())
 *       // ... render result ...
 *
 *   // On Enter / double-click:
 *   sp.ConfirmSelection(selectedIndex);
 * @endcode
 */
class NodeSearchPanel {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /** @brief Returns the single shared instance. */
    static NodeSearchPanel& Get();

    // -----------------------------------------------------------------------
    // Open / close
    // -----------------------------------------------------------------------

    /**
     * @brief Opens the search panel at the given graph-space position.
     * @param posX Spawn X for the new node (graph space).
     * @param posY Spawn Y for the new node (graph space).
     */
    void OpenSearch(float posX, float posY);

    /** @brief Closes the search panel and clears the query. */
    void CloseSearch();

    /** @brief Returns true if the panel is currently visible. */
    bool IsOpen() const;

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Recomputes the result list for the given query string.
     *        Call this every frame (or whenever the query changes).
     */
    void UpdateQuery(const std::string& query);

    /**
     * @brief Returns the current result list sorted by relevance (best first).
     */
    const std::vector<NodeSearchResult>& GetResults() const;

    // -----------------------------------------------------------------------
    // Selection
    // -----------------------------------------------------------------------

    /**
     * @brief Fires the OnNodeAdd callback for the result at @p index and
     *        closes the panel.  No-op if @p index is out of range.
     */
    void ConfirmSelection(int index);

    /**
     * @brief Returns the index of the currently highlighted result (-1 = none).
     */
    int GetSelectedIndex() const;

    /**
     * @brief Moves the selection up by one step (wraps around).
     */
    void SelectPrevious();

    /**
     * @brief Moves the selection down by one step (wraps around).
     */
    void SelectNext();

    // -----------------------------------------------------------------------
    // Callback
    // -----------------------------------------------------------------------

    /**
     * @brief Registers the callback that fires when the user confirms a node.
     *
     * @param callback  Signature: void(nodeType, spawnX, spawnY)
     */
    void SetNodeAddCallback(std::function<void(const std::string&, float, float)> callback);

    // -----------------------------------------------------------------------
    // Spawn position
    // -----------------------------------------------------------------------

    /** @brief Returns the X coordinate where the new node should be spawned. */
    float GetSpawnX() const;

    /** @brief Returns the Y coordinate where the new node should be spawned. */
    float GetSpawnY() const;

private:

    NodeSearchPanel();

    void InitNodeCatalogue();

    /// Compute a [0..1] relevance score; returns 0 if there is no match.
    static float ComputeScore(const std::string& query,
                              const std::string& candidate);

    bool                        m_IsOpen;
    float                       m_SpawnPosX;
    float                       m_SpawnPosY;
    std::vector<NodeSearchResult> m_Catalogue;   ///< Full unfiltered catalogue
    std::vector<NodeSearchResult> m_Results;     ///< Current filtered/sorted results
    int                         m_SelectedIndex;
    std::function<void(const std::string&, float, float)> m_OnNodeAdd;
};

} // namespace Olympe
