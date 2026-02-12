/*
Olympe Engine V2 2025
Animation System - Animation Manager Implementation
*/

#include "Animation/AnimationManager.h"
#include "system/system_utils.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace OlympeAnimation
{
    // ========================================================================
    // AnimationManager Implementation
    // ========================================================================

    void AnimationManager::Init()
    {
        if (m_initialized)
            return;

        SYSTEM_LOG << "AnimationManager: Initializing...\n";
        m_initialized = true;
    }

    void AnimationManager::LoadAnimationBanks(const std::string& directoryPath)
    {
        SYSTEM_LOG << "AnimationManager: Loading animation banks from " << directoryPath << "\n";

        auto files = ScanDirectory(directoryPath);
        for (const auto& filePath : files)
        {
            LoadAnimationBank(filePath);
        }

        SYSTEM_LOG << "AnimationManager: Loaded " << m_banks.size() << " animation banks\n";
    }

    void AnimationManager::LoadAnimationGraphs(const std::string& directoryPath)
    {
        SYSTEM_LOG << "AnimationManager: Loading animation graphs from " << directoryPath << "\n";

        auto files = ScanDirectory(directoryPath);
        for (const auto& filePath : files)
        {
            LoadAnimationGraph(filePath);
        }

        SYSTEM_LOG << "AnimationManager: Loaded " << m_graphs.size() << " animation graphs\n";
    }

    bool AnimationManager::LoadAnimationBank(const std::string& filePath)
    {
        auto bank = std::make_unique<AnimationBank>();
        if (bank->LoadFromFile(filePath))
        {
            std::string bankName = bank->GetBankName();
            m_banks[bankName] = std::move(bank);
            return true;
        }
        return false;
    }

    bool AnimationManager::LoadAnimationGraph(const std::string& filePath)
    {
        auto graph = std::make_unique<AnimationGraph>();
        if (graph->LoadFromFile(filePath))
        {
            std::string graphName = graph->GetGraphName();
            m_graphs[graphName] = std::move(graph);
            return true;
        }
        return false;
    }

    AnimationBank* AnimationManager::GetBank(const std::string& bankName)
    {
        auto it = m_banks.find(bankName);
        if (it != m_banks.end())
            return it->second.get();
        return nullptr;
    }

    const AnimationBank* AnimationManager::GetBank(const std::string& bankName) const
    {
        auto it = m_banks.find(bankName);
        if (it != m_banks.end())
            return it->second.get();
        return nullptr;
    }

    AnimationGraph* AnimationManager::GetGraph(const std::string& graphName)
    {
        auto it = m_graphs.find(graphName);
        if (it != m_graphs.end())
            return it->second.get();
        return nullptr;
    }

    const AnimationGraph* AnimationManager::GetGraph(const std::string& graphName) const
    {
        auto it = m_graphs.find(graphName);
        if (it != m_graphs.end())
            return it->second.get();
        return nullptr;
    }

    void AnimationManager::Shutdown()
    {
        SYSTEM_LOG << "AnimationManager: Shutting down...\n";
        m_banks.clear();
        m_graphs.clear();
        m_initialized = false;
    }

    std::vector<std::string> AnimationManager::ScanDirectory(const std::string& directoryPath)
    {
        std::vector<std::string> files;

        try
        {
            if (!fs::exists(directoryPath))
            {
                SYSTEM_LOG << "AnimationManager: Directory not found: " << directoryPath << "\n";
                return files;
            }

            for (const auto& entry : fs::directory_iterator(directoryPath))
            {
                if (entry.is_regular_file())
                {
                    std::string path = entry.path().string();
                    // Check if file has .json extension
                    if (path.size() >= 5 && path.substr(path.size() - 5) == ".json")
                    {
                        files.push_back(path);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "AnimationManager: Error scanning directory " << directoryPath 
                      << ": " << e.what() << "\n";
        }

        return files;
    }

} // namespace OlympeAnimation
