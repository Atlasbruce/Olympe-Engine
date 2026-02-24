/**
 * @file AtomicTaskRegistry.cpp
 * @brief Implementation of AtomicTaskRegistry singleton.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * C++14 compliant - no C++17/20 features.
 */

#include "AtomicTaskRegistry.h"
#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Singleton accessor
// ============================================================================

AtomicTaskRegistry& AtomicTaskRegistry::Get()
{
    static AtomicTaskRegistry s_instance;
    return s_instance;
}

// ============================================================================
// Register
// ============================================================================

void AtomicTaskRegistry::Register(const std::string& id, FactoryFn factory)
{
    m_factories[id] = std::move(factory);
}

// ============================================================================
// Create
// ============================================================================

std::unique_ptr<IAtomicTask> AtomicTaskRegistry::Create(const std::string& id) const
{
    auto it = m_factories.find(id);
    if (it != m_factories.end())
    {
        return it->second();
    }

    // ---------------------------------------------------------------------------
    // Legacy ID migration: support both "Task_Foo" (legacy) and "Foo" (short).
    //
    // If the requested ID was not found directly, attempt the complementary form:
    //   - Short ID  ("MoveToLocation")  -> try legacy form ("Task_MoveToLocation")
    //   - Legacy ID ("Task_MoveToLocation") -> try short form ("MoveToLocation")
    //
    // This allows existing task graphs that use legacy IDs to continue working
    // unchanged while new graphs may use the shorter editor-friendly form.
    // ---------------------------------------------------------------------------
    static const std::string prefix = "Task_";

    if (id.size() > prefix.size()
        && id.compare(0, prefix.size(), prefix) == 0)
    {
        // id starts with "Task_": try without the prefix (short form)
        std::string shortId = id.substr(prefix.size());
        auto it2 = m_factories.find(shortId);
        if (it2 != m_factories.end())
        {
            SYSTEM_LOG << "[AtomicTaskRegistry] Legacy ID '" << id
                       << "' resolved to short ID '" << shortId << "'\n";
            return it2->second();
        }
    }
    else
    {
        // id does not start with "Task_": try adding the prefix (legacy form)
        std::string legacyId = prefix + id;
        auto it2 = m_factories.find(legacyId);
        if (it2 != m_factories.end())
        {
            SYSTEM_LOG << "[AtomicTaskRegistry] Short ID '" << id
                       << "' resolved to legacy ID '" << legacyId << "'\n";
            return it2->second();
        }
    }

    return nullptr;
}

// ============================================================================
// IsRegistered
// ============================================================================

bool AtomicTaskRegistry::IsRegistered(const std::string& id) const
{
    return m_factories.find(id) != m_factories.end();
}

// ============================================================================
// GetAllTaskIDs
// ============================================================================

std::vector<std::string> AtomicTaskRegistry::GetAllTaskIDs() const
{
    std::vector<std::string> ids;
    ids.reserve(m_factories.size());
    for (auto it = m_factories.begin(); it != m_factories.end(); ++it)
    {
        ids.push_back(it->first);
    }
    return ids;
}

} // namespace Olympe
