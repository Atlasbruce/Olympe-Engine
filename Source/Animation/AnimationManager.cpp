/*
Olympe Engine V2 2025
Animation System - Animation Manager Implementation
*/


#include "AnimationManager.h"
#include "../system/system_utils.h"
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

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


    const Olympe::AnimationSequence* AnimationManager::GetAnimationSequence(
        const std::string& bankId,
        const std::string& animName
    ) const
    {
        // Find the bank
        auto bankIt = m_banks.find(bankId);
        if (bankIt == m_banks.end())
        {
            // Bank not found
            return nullptr;
        }

        const AnimationBank* bank = bankIt->second.get();
        if (!bank)
        {
            return nullptr;
        }

        // Get the animation from the bank using the public interface
        const Animation* anim = bank->GetAnimation(animName);
        if (!anim)
        {
            // Animation not found in bank
            return nullptr;
        }

        // Note: This function returns Olympe::AnimationSequence* but AnimationBank 
        // contains OlympeAnimation::Animation. This may need further review.
        // For now, returning nullptr as the types are incompatible.
        return nullptr;
    }

    bool AnimationManager::HasAnimation(
        const std::string& bankId,
        const std::string& animName
    ) const
    {
        auto bankIt = m_banks.find(bankId);
        if (bankIt == m_banks.end())
        {
            return false;
        }

        const AnimationBank* bank = bankIt->second.get();
        if (!bank)
        {
            return false;
        }

        // Use the public GetAnimation method to check if animation exists
        return bank->GetAnimation(animName) != nullptr;
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

#ifdef _WIN32
        // Windows implementation
        std::string searchPattern = directoryPath + "\\*.json";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            SYSTEM_LOG << "AnimationManager: Directory not found or empty: " << directoryPath << "\n";
            return files;
        }

        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string fileName = findData.cFileName;
                std::string fullPath = directoryPath + "\\" + fileName;
                files.push_back(fullPath);
            }
        } while (FindNextFileA(hFind, &findData) != 0);

        FindClose(hFind);
#else
        // POSIX implementation (Linux/Mac)
        DIR* dir = opendir(directoryPath.c_str());
        if (!dir)
        {
            SYSTEM_LOG << "AnimationManager: Directory not found: " << directoryPath << "\n";
            return files;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string fileName = entry->d_name;
            
            // Skip . and ..
            if (fileName == "." || fileName == "..")
                continue;

            // Check for .json extension
            size_t dotPos = fileName.find_last_of('.');
            if (dotPos != std::string::npos && fileName.substr(dotPos) == ".json")
            {
                std::string fullPath = directoryPath + "/" + fileName;
                
                // Verify it's a regular file
                struct stat statbuf;
                if (stat(fullPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode))
                {
                    files.push_back(fullPath);
                }
            }
        }

        closedir(dir);
#endif

        return files;
    }

} // namespace OlympeAnimation
