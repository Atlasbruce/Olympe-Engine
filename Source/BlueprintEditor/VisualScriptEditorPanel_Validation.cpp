/**
 * @file VisualScriptEditorPanel_Validation.cpp
 * @brief TIER 0 - Blackboard validation and cleanup helpers
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"
#include <algorithm>

namespace Olympe {

// ============================================================================
// Blackboard validation helpers (BUG-002 Fix #1)
// ============================================================================

void VisualScriptEditorPanel::ValidateAndCleanBlackboardEntries()
{
    std::vector<BlackboardEntry>& entries = m_template.Blackboard;
    size_t before = entries.size();

    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [](const BlackboardEntry& e) {
                if (e.Key.empty()) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry with empty key\n";
                    return true;
                }
                if (e.Type == VariableType::None) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry '"
                               << e.Key << "' with VariableType::None\n";
                    return true;
                }
                return false;
            }),
        entries.end());

    size_t removed = before - entries.size();
    if (removed > 0)
    {
        SYSTEM_LOG << "[VSEditor] ValidateAndClean: removed " << removed
                   << " invalid blackboard entries\n";
        m_dirty = true;
    }
}

void VisualScriptEditorPanel::CommitPendingBlackboardEdits()
{
    for (std::unordered_map<int, std::string>::iterator it = m_pendingBlackboardEdits.begin();
         it != m_pendingBlackboardEdits.end(); ++it)
    {
        int idx = it->first;
        if (idx >= 0 && idx < static_cast<int>(m_template.Blackboard.size()))
        {
            m_template.Blackboard[static_cast<size_t>(idx)].Key = it->second;
        }
    }
    m_pendingBlackboardEdits.clear();
}

} // namespace Olympe
