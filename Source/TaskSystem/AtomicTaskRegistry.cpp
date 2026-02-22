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
    if (it == m_factories.end())
    {
        return nullptr;
    }
    return it->second();
}

// ============================================================================
// IsRegistered
// ============================================================================

bool AtomicTaskRegistry::IsRegistered(const std::string& id) const
{
    return m_factories.find(id) != m_factories.end();
}

} // namespace Olympe
