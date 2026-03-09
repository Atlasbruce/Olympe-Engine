/**
 * @file NodeSearchPalette.h
 * @brief Fuzzy-search palette for VS node types (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * NodeSearchPalette maintains a catalog of all available node types and
 * provides fuzzy-search over them.  Results are sorted by descending match
 * score.  The implementation has no ImGui dependency — UI code calls
 * FuzzySearch() and renders the returned vector however it likes.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @enum NodeSearchCategory
 * @brief Broad category for filtering node search results.
 */
enum class NodeSearchCategory {
    All,
    ControlFlow,
    Actions,
    Data
};

/**
 * @struct NodeSearchResult
 * @brief A single match returned by NodeSearchPalette::FuzzySearch().
 */
struct NodeSearchResult {
    std::string        typeName;     ///< Internal type identifier (e.g. "Sequence")
    std::string        displayName;  ///< Human-readable label shown in the palette
    NodeSearchCategory category;
    int                score;        ///< Fuzzy match score; higher = better
};

// ============================================================================
// NodeSearchPalette
// ============================================================================

/**
 * @class NodeSearchPalette
 * @brief Singleton fuzzy-search palette for VS node types.
 *
 * Typical usage:
 * @code
 *   auto& palette = NodeSearchPalette::Get();
 *   palette.Open();
 *   auto results = palette.FuzzySearch("mov");
 *   // render results...
 * @endcode
 */
class NodeSearchPalette {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the single shared instance.
     */
    static NodeSearchPalette& Get();

    // -----------------------------------------------------------------------
    // Open / close
    // -----------------------------------------------------------------------

    void Open();
    void Close();
    bool IsOpen() const;

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Fuzzy-searches the node catalog.
     *
     * @param query   The string typed by the user (may be empty).
     * @param filter  Category to restrict results to; defaults to All.
     * @return        Results sorted by score descending.
     */
    std::vector<NodeSearchResult> FuzzySearch(
        const std::string&  query,
        NodeSearchCategory  filter = NodeSearchCategory::All);

    /**
     * @brief Computes a fuzzy match score between @p query and @p candidate.
     *
     * @details
     * Returns 0 when there is no match.  Positive scores reflect quality:
     * exact match > substring match > scattered character match.
     */
    static int ComputeFuzzyScore(const std::string& query,
                                 const std::string& candidate);

    /**
     * @brief Returns all node types in the catalog (unfiltered).
     */
    const std::vector<NodeSearchResult>& GetAllNodes() const;

private:

    NodeSearchPalette();

    void InitNodeCatalog();

    bool                         m_IsOpen;
    std::vector<NodeSearchResult> m_NodeCatalog;
};

} // namespace Olympe
