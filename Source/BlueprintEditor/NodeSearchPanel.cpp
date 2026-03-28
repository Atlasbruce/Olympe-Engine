/**
 * @file NodeSearchPanel.cpp
 * @brief NodeSearchPanel implementation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "NodeSearchPanel.h"

#include <algorithm>
#include <cctype>

#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Helpers
// ============================================================================

namespace {

std::string ToLowerNSP(const std::string& s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

} // anonymous namespace

// ============================================================================
// Singleton
// ============================================================================

NodeSearchPanel& NodeSearchPanel::Get()
{
    static NodeSearchPanel s_Instance;
    return s_Instance;
}

NodeSearchPanel::NodeSearchPanel()
    : m_IsOpen(false)
    , m_SpawnPosX(0.0f)
    , m_SpawnPosY(0.0f)
    , m_SelectedIndex(-1)
{
    InitNodeCatalogue();
}

// ============================================================================
// Catalogue initialisation
// ============================================================================

void NodeSearchPanel::InitNodeCatalogue()
{
    m_Catalogue.clear();

    auto add = [this](const std::string& type,
                      const std::string& name,
                      const std::string& category,
                      const std::string& desc)
    {
        NodeSearchResult r;
        r.nodeType       = type;
        r.name           = name;
        r.category       = category;
        r.description    = desc;
        r.relevanceScore = 0.0f;
        m_Catalogue.push_back(r);
    };

    // ---- Control Flow ----
    add("EntryPoint",  "Entry Point",    "ControlFlow", "Graph execution start node.");
    add("Sequence",    "Sequence",       "ControlFlow", "Executes children left-to-right.");
    add("Selector",    "Selector",       "ControlFlow", "Executes children until one succeeds.");
    add("Branch",      "Branch",         "ControlFlow", "Conditional fork (true/false).");
    add("Repeat",      "Repeat",         "ControlFlow", "Loops a subgraph N times.");
    add("Wait",        "Wait",           "ControlFlow", "Pauses execution for a duration.");
    add("SubGraph",    "Sub Graph",      "ControlFlow", "Delegates execution to an external graph.");

    // ---- Actions ----
    add("MoveToLocation", "Move To Location", "Actions", "Moves an entity to a target position.");
    add("Attack",         "Attack",           "Actions", "Triggers an attack on a target.");
    add("Flee",           "Flee",             "Actions", "Makes the entity flee from a threat.");
    add("Patrol",         "Patrol",           "Actions", "Cycles through patrol waypoints.");
    add("SetVariable",    "Set Variable",     "Actions", "Writes a value to the blackboard.");
    add("LogMessage",     "Log Message",      "Actions", "Emits a debug message.");

    // ---- Data (Phase 24.1) ----
    add("Variable",         "Variable",         "Data", "Data pure node - reads a blackboard variable.");
    add("MathOp",           "Math Operation",   "Data", "Data pure node - computes arithmetic operation.");
    add("GetBBValue",       "Get BB Value",     "Data", "Reads a value from the blackboard.");
    add("SetBBValue",       "Set BB Value",     "Data", "Writes a value to the blackboard.");

    SYSTEM_LOG << "[NodeSearchPanel] Catalogue initialised with "
               << static_cast<int>(m_Catalogue.size()) << " entries." << std::endl;
}

// ============================================================================
// Open / close
// ============================================================================

void NodeSearchPanel::OpenSearch(float posX, float posY)
{
    m_IsOpen        = true;
    m_SpawnPosX     = posX;
    m_SpawnPosY     = posY;
    m_SelectedIndex = -1;
    m_Results.clear();

    // Show entire catalogue by default
    UpdateQuery("");
}

void NodeSearchPanel::CloseSearch()
{
    m_IsOpen        = false;
    m_SelectedIndex = -1;
    m_Results.clear();
}

bool NodeSearchPanel::IsOpen() const
{
    return m_IsOpen;
}

// ============================================================================
// Search
// ============================================================================

float NodeSearchPanel::ComputeScore(const std::string& query,
                                    const std::string& candidate)
{
    if (query.empty())
        return 1.0f;

    const std::string q = ToLowerNSP(query);
    const std::string c = ToLowerNSP(candidate);

    if (c.empty())
        return 0.0f;

    // Exact match
    if (c == q)
        return 1.0f;

    // Prefix match
    if (c.size() >= q.size() && c.substr(0, q.size()) == q)
        return 0.8f;

    // Substring match
    if (c.find(q) != std::string::npos)
        return 0.6f;

    // Scattered character match
    size_t qi = 0;
    size_t gaps = 0;
    for (size_t ci = 0; ci < c.size() && qi < q.size(); ++ci)
    {
        if (c[ci] == q[qi])
            ++qi;
        else
            ++gaps;
    }
    if (qi == q.size())
    {
        float score = 0.4f - static_cast<float>(gaps) * 0.01f;
        return (score > 0.01f) ? score : 0.01f;
    }

    return 0.0f;
}

void NodeSearchPanel::UpdateQuery(const std::string& query)
{
    m_Results.clear();

    for (size_t i = 0; i < m_Catalogue.size(); ++i)
    {
        const NodeSearchResult& entry = m_Catalogue[i];

        float scoreByName = ComputeScore(query, entry.name);
        float scoreByType = ComputeScore(query, entry.nodeType);
        float scoreByDesc = ComputeScore(query, entry.description) * 0.5f;

        float best = scoreByName;
        if (scoreByType > best) best = scoreByType;
        if (scoreByDesc > best) best = scoreByDesc;

        if (best > 0.0f)
        {
            NodeSearchResult r = entry;
            r.relevanceScore   = best;
            m_Results.push_back(r);
        }
    }

    // Sort by relevance descending, then name ascending for stable order
    std::sort(m_Results.begin(), m_Results.end(),
        [](const NodeSearchResult& a, const NodeSearchResult& b) {
            if (a.relevanceScore != b.relevanceScore)
                return a.relevanceScore > b.relevanceScore;
            return a.name < b.name;
        });

    // Reset selection when results change
    m_SelectedIndex = m_Results.empty() ? -1 : 0;
}

const std::vector<NodeSearchResult>& NodeSearchPanel::GetResults() const
{
    return m_Results;
}

// ============================================================================
// Selection
// ============================================================================

void NodeSearchPanel::ConfirmSelection(int index)
{
    if (index < 0 || index >= static_cast<int>(m_Results.size()))
        return;

    const std::string& type = m_Results[static_cast<size_t>(index)].nodeType;
    if (m_OnNodeAdd)
        m_OnNodeAdd(type, m_SpawnPosX, m_SpawnPosY);

    CloseSearch();
}

int NodeSearchPanel::GetSelectedIndex() const
{
    return m_SelectedIndex;
}

void NodeSearchPanel::SelectPrevious()
{
    if (m_Results.empty())
        return;
    int n = static_cast<int>(m_Results.size());
    m_SelectedIndex = (m_SelectedIndex <= 0) ? (n - 1) : (m_SelectedIndex - 1);
}

void NodeSearchPanel::SelectNext()
{
    if (m_Results.empty())
        return;
    int n = static_cast<int>(m_Results.size());
    m_SelectedIndex = (m_SelectedIndex >= n - 1) ? 0 : (m_SelectedIndex + 1);
}

// ============================================================================
// Callback
// ============================================================================

void NodeSearchPanel::SetNodeAddCallback(
    std::function<void(const std::string&, float, float)> callback)
{
    m_OnNodeAdd = callback;
}

// ============================================================================
// Spawn position
// ============================================================================

float NodeSearchPanel::GetSpawnX() const { return m_SpawnPosX; }
float NodeSearchPanel::GetSpawnY() const { return m_SpawnPosY; }

} // namespace Olympe
