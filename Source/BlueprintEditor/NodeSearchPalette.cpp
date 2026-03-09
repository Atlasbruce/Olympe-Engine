/**
 * @file NodeSearchPalette.cpp
 * @brief Fuzzy-search palette implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "NodeSearchPalette.h"

#include <algorithm>
#include <cctype>

#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

NodeSearchPalette& NodeSearchPalette::Get()
{
    static NodeSearchPalette s_Instance;
    return s_Instance;
}

// ============================================================================
// Construction
// ============================================================================

NodeSearchPalette::NodeSearchPalette()
    : m_IsOpen(false)
{
    InitNodeCatalog();
}

// ============================================================================
// Open / close
// ============================================================================

void NodeSearchPalette::Open()
{
    m_IsOpen = true;
}

void NodeSearchPalette::Close()
{
    m_IsOpen = false;
}

bool NodeSearchPalette::IsOpen() const
{
    return m_IsOpen;
}

// ============================================================================
// Catalog initialisation
// ============================================================================

void NodeSearchPalette::InitNodeCatalog()
{
    m_NodeCatalog.clear();

    // ---- Control Flow nodes ----
    auto addCF = [this](const std::string& type, const std::string& display) {
        NodeSearchResult r;
        r.typeName    = type;
        r.displayName = display;
        r.category    = NodeSearchCategory::ControlFlow;
        r.score       = 0;
        m_NodeCatalog.push_back(r);
    };

    addCF("Sequence",    "Sequence");
    addCF("Selector",    "Selector");
    addCF("Branch",      "Branch");
    addCF("EntryPoint",  "Entry Point");
    addCF("SubGraph",    "Sub Graph");
    addCF("Repeat",      "Repeat");
    addCF("Wait",        "Wait");

    // ---- Action nodes ----
    auto addAction = [this](const std::string& type, const std::string& display) {
        NodeSearchResult r;
        r.typeName    = type;
        r.displayName = display;
        r.category    = NodeSearchCategory::Actions;
        r.score       = 0;
        m_NodeCatalog.push_back(r);
    };

    addAction("MoveToLocation", "Move To Location");
    addAction("Attack",         "Attack");
    addAction("Flee",           "Flee");
    addAction("Patrol",         "Patrol");
    addAction("SetVariable",    "Set Variable");
    addAction("LogMessage",     "Log Message");

    // ---- Data nodes ----
    auto addData = [this](const std::string& type, const std::string& display) {
        NodeSearchResult r;
        r.typeName    = type;
        r.displayName = display;
        r.category    = NodeSearchCategory::Data;
        r.score       = 0;
        m_NodeCatalog.push_back(r);
    };

    addData("GetVariable",      "Get Variable");
    addData("SetBlackboardKey", "Set Blackboard Key");

    SYSTEM_LOG << "[NodeSearchPalette] Catalog initialised with "
               << static_cast<int>(m_NodeCatalog.size()) << " node types."
               << std::endl;
}

// ============================================================================
// FuzzySearch
// ============================================================================

std::vector<NodeSearchResult> NodeSearchPalette::FuzzySearch(
    const std::string& query,
    NodeSearchCategory filter)
{
    std::vector<NodeSearchResult> results;

    for (size_t i = 0; i < m_NodeCatalog.size(); ++i)
    {
        const NodeSearchResult& entry = m_NodeCatalog[i];

        if (filter != NodeSearchCategory::All && entry.category != filter)
            continue;

        int score = 0;

        if (query.empty())
        {
            // Empty query — return everything with equal score
            score = 1;
        }
        else
        {
            // Score against both typeName and displayName; take the higher
            int s1 = ComputeFuzzyScore(query, entry.typeName);
            int s2 = ComputeFuzzyScore(query, entry.displayName);
            score  = (s1 > s2) ? s1 : s2;
        }

        if (score > 0)
        {
            NodeSearchResult r = entry;
            r.score = score;
            results.push_back(r);
        }
    }

    // Sort by score descending, then displayName ascending for stable order
    std::sort(results.begin(), results.end(),
        [](const NodeSearchResult& a, const NodeSearchResult& b) {
            if (a.score != b.score)
                return a.score > b.score;
            return a.displayName < b.displayName;
        });

    return results;
}

// ============================================================================
// ComputeFuzzyScore
// ============================================================================

static std::string ToLower(const std::string& s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

int NodeSearchPalette::ComputeFuzzyScore(const std::string& query,
                                         const std::string& candidate)
{
    if (query.empty())
        return 1;

    const std::string q = ToLower(query);
    const std::string c = ToLower(candidate);

    if (c.empty())
        return 0;

    // Exact match
    if (c == q)
        return 1000;

    // Prefix match
    if (c.substr(0, q.size()) == q)
        return 800;

    // Substring match
    if (c.find(q) != std::string::npos)
        return 600;

    // Scattered character match: every character in query appears in candidate
    // in order.  Score based on how close together the characters are.
    size_t qi = 0;
    size_t ci = 0;
    int    gaps = 0;

    while (qi < q.size() && ci < c.size())
    {
        if (q[qi] == c[ci])
        {
            ++qi;
        }
        else
        {
            ++gaps;
        }
        ++ci;
    }

    if (qi == q.size())
    {
        // All characters matched; penalise by gap count
        int score = 400 - gaps * 5;
        return (score > 1) ? score : 1;
    }

    return 0;
}

// ============================================================================
// GetAllNodes
// ============================================================================

const std::vector<NodeSearchResult>& NodeSearchPalette::GetAllNodes() const
{
    return m_NodeCatalog;
}

} // namespace Olympe
