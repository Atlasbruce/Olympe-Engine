/**
 * @file AssetManager.cpp
 * @brief Implementation of AssetManager: TaskGraph loading, caching, and retrieval.
 * @author Olympe Engine
 * @date 2026-02-22
 */

#include "Core/AssetManager.h"

#include "TaskSystem/TaskGraphLoader.h"
#include "system/system_utils.h"

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

AssetManager& AssetManager::Get()
{
    static AssetManager s_instance;
    return s_instance;
}

// ============================================================================
// Utilities
// ============================================================================

AssetID AssetManager::ComputeAssetID(const std::string& path)
{
    if (path.empty())
    {
        return INVALID_ASSET_ID;
    }

    // 32-bit FNV-1a hash
    static const uint32_t FNV_PRIME  = 16777619u;
    static const uint32_t FNV_OFFSET = 2166136261u;

    uint32_t hash = FNV_OFFSET;
    for (size_t i = 0; i < path.size(); ++i)
    {
        hash ^= static_cast<uint32_t>(static_cast<unsigned char>(path[i]));
        hash *= FNV_PRIME;
    }

    // Ensure we never return the sentinel value for a non-empty path.
    if (hash == INVALID_ASSET_ID)
    {
        hash = 1u;
    }

    return hash;
}

// ============================================================================
// TaskGraph asset API
// ============================================================================

AssetID AssetManager::LoadTaskGraph(const std::string& path,
                                    std::vector<std::string>& outErrors)
{
    if (path.empty())
    {
        outErrors.push_back("[AssetManager] LoadTaskGraph: path is empty");
        return INVALID_ASSET_ID;
    }

    const AssetID id = ComputeAssetID(path);

    // Return cached entry immediately if already loaded.
    auto it = m_taskGraphs.find(id);
    if (it != m_taskGraphs.end())
    {
        SYSTEM_LOG << "[AssetManager] LoadTaskGraph: cache hit for '"
                   << path << "' (id=" << id << ")" << std::endl;
        return id;
    }

    // Load from disk via TaskGraphLoader.
    TaskGraphTemplate* raw = TaskGraphLoader::LoadFromFile(path, outErrors);
    if (raw == nullptr)
    {
        SYSTEM_LOG << "[AssetManager] LoadTaskGraph: failed to load '"
                   << path << "'" << std::endl;
        return INVALID_ASSET_ID;
    }

    m_taskGraphs[id] = std::unique_ptr<TaskGraphTemplate>(raw);

    SYSTEM_LOG << "[AssetManager] LoadTaskGraph: loaded '"
               << path << "' (id=" << id << ")" << std::endl;

    return id;
}

const TaskGraphTemplate* AssetManager::GetTaskGraph(AssetID id) const
{
    auto it = m_taskGraphs.find(id);
    if (it == m_taskGraphs.end())
    {
        return nullptr;
    }
    return it->second.get();
}

void AssetManager::UnloadTaskGraph(AssetID id)
{
    auto it = m_taskGraphs.find(id);
    if (it == m_taskGraphs.end())
    {
        SYSTEM_LOG << "[AssetManager] UnloadTaskGraph: id=" << id
                   << " not found in cache" << std::endl;
        return;
    }

    m_taskGraphs.erase(it);

    SYSTEM_LOG << "[AssetManager] UnloadTaskGraph: released id=" << id << std::endl;
}

} // namespace Olympe
