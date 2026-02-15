/**
 * @file AnimationManager.cpp
 * @brief Implementation of AnimationManager
 * @author Nicolas Chereau
 * @date 2025
 */

#include "AnimationManager.h"
#include "../DataManager.h"
#include "../system/system_utils.h"
#include <fstream>
#include <algorithm>
#include "../third_party/nlohmann/json.hpp"

// C++14 compatible directory iteration
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

using json = nlohmann::json;

namespace Olympe
{

// ========================================================================
// Animation Bank Loading
// ========================================================================

bool AnimationManager::LoadAnimationBank(const std::string& bankJsonPath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation bank from " << bankJsonPath << "\n";
    
    // Check if file exists
    if (!std::ifstream(bankJsonPath).good())
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: File not found: " << bankJsonPath << "\n";
        return false;
    }
    
    // Parse JSON file
    std::ifstream file(bankJsonPath);
    json jsonData;
    
    try
    {
        file >> jsonData;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: JSON parse error in " << bankJsonPath << ": " << e.what() << "\n";
        return false;
    }
    
    // Get bank ID from JSON
    if (!jsonData.contains("id"))
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: No 'id' field found in " << bankJsonPath << "\n";
        return false;
    }
    std::string bankId = jsonData["id"].get<std::string>();
    
    // Create animation bank
    AnimationBank bank;
    bank.bankId = bankId;
    
    // Load spritesheet metadata
    if (jsonData.contains("frameWidth"))
        bank.frameWidth = jsonData["frameWidth"].get<int>();
    if (jsonData.contains("frameHeight"))
        bank.frameHeight = jsonData["frameHeight"].get<int>();
    if (jsonData.contains("columns"))
        bank.columns = jsonData["columns"].get<int>();
    if (jsonData.contains("spacing"))
        bank.spacing = jsonData["spacing"].get<int>();
    if (jsonData.contains("margin"))
        bank.margin = jsonData["margin"].get<int>();
    
    // Load animations
    if (!jsonData.contains("animations") || !jsonData["animations"].is_object())
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: No 'animations' object found in " << bankJsonPath << "\n";
        return false;
    }
    
    int animCount = 0;
    for (auto it = jsonData["animations"].begin(); it != jsonData["animations"].end(); ++it)
    {
        const std::string& animName = it.key();
        const json& animData = it.value();
        
        AnimationSequence sequence;
        sequence.name = animName;
        
        // Load spritesheet path
        if (animData.contains("spritesheet"))
            sequence.spritesheetPath = animData["spritesheet"].get<std::string>();
        
        // Load playback settings
        if (animData.contains("loop"))
            sequence.loop = animData["loop"].get<bool>();
        if (animData.contains("speed"))
            sequence.speed = animData["speed"].get<float>();
        if (animData.contains("nextAnimation"))
            sequence.nextAnimation = animData["nextAnimation"].get<std::string>();
        
        // Load frames
        if (animData.contains("frames"))
        {
            // Check if frames is an array or frameRange object
            if (animData["frames"].is_array())
            {
                // Explicit frame list
                for (auto& frameData : animData["frames"])
                {
                    AnimationFrame frame;
                    
                    if (frameData.contains("srcRect"))
                    {
                        frame.srcRect.x = frameData["srcRect"]["x"].get<float>();
                        frame.srcRect.y = frameData["srcRect"]["y"].get<float>();
                        frame.srcRect.w = frameData["srcRect"]["w"].get<float>();
                        frame.srcRect.h = frameData["srcRect"]["h"].get<float>();
                    }
                    
                    if (frameData.contains("duration"))
                        frame.duration = frameData["duration"].get<float>();
                    
                    if (frameData.contains("hotSpot"))
                    {
                        frame.hotSpot.x = frameData["hotSpot"]["x"].get<float>();
                        frame.hotSpot.y = frameData["hotSpot"]["y"].get<float>();
                    }
                    
                    if (frameData.contains("event"))
                        frame.eventName = frameData["event"].get<std::string>();
                    
                    sequence.frames.push_back(frame);
                }
            }
            else if (animData["frames"].is_object())
            {
                // FrameRange format: { "start": 0, "count": 128 }
                int startFrame = 0;
                int frameCount = 1;
                float frameDuration = 0.1f;
                
                if (animData["frames"].contains("start"))
                    startFrame = animData["frames"]["start"].get<int>();
                if (animData["frames"].contains("count"))
                    frameCount = animData["frames"]["count"].get<int>();
                if (animData.contains("frameDuration"))
                    frameDuration = animData["frameDuration"].get<float>();
                
                // Generate frames automatically using spritesheet layout
                for (int i = 0; i < frameCount; ++i)
                {
                    AnimationFrame frame;
                    frame.srcRect = bank.CalculateFrameRect(startFrame + i);
                    frame.duration = frameDuration;
                    frame.hotSpot.x = bank.frameWidth / 2.0f;
                    frame.hotSpot.y = bank.frameHeight / 2.0f;
                    sequence.frames.push_back(frame);
                }
            }
        }
        else if (animData.contains("frameDuration"))
        {
            // Legacy format: assume all frames in spritesheet with frameDuration
            float frameDuration = animData["frameDuration"].get<float>();
            int frameCount = animData.value("frameCount", 1);
            
            for (int i = 0; i < frameCount; ++i)
            {
                AnimationFrame frame;
                frame.srcRect = bank.CalculateFrameRect(i);
                frame.duration = frameDuration;
                frame.hotSpot.x = bank.frameWidth / 2.0f;
                frame.hotSpot.y = bank.frameHeight / 2.0f;
                sequence.frames.push_back(frame);
            }
        }
        
        // Preload spritesheet texture via DataManager
        if (!sequence.spritesheetPath.empty())
        {
            std::string textureId = bankId + "_" + animName;
            DataManager::Get().PreloadSprite(textureId, sequence.spritesheetPath);
        }
        
        bank.animations[animName] = sequence;
        animCount++;
    }
    
    SYSTEM_LOG << "[AnimationManager] Loaded " << animCount << " animations for bank '" << bankId << "'\n";
    
    // Store bank
    m_banks[bankId] = std::move(bank);
    return true;
}

bool AnimationManager::LoadAnimationBanksFromDirectory(const std::string& directoryPath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation banks from: " << directoryPath << "\n";
    
    int loadedCount = 0;
    
#ifdef _WIN32
    // Windows directory iteration
    WIN32_FIND_DATAA findData;
    std::string searchPath = directoryPath + "/*.json";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string fileName = findData.cFileName;
                std::string filePath = directoryPath + "/" + fileName;
                
                if (LoadAnimationBank(filePath))
                {
                    loadedCount++;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    // POSIX directory iteration
    DIR* dir = opendir(directoryPath.c_str());
    if (dir)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL)
        {
            std::string fileName = entry->d_name;
            
            // Skip . and ..
            if (fileName == "." || fileName == "..")
                continue;
            
            // Check if it's a .json file
            if (fileName.length() > 5 && fileName.substr(fileName.length() - 5) == ".json")
            {
                std::string filePath = directoryPath + "/" + fileName;
                
                // Check if it's a regular file
                struct stat st;
                if (stat(filePath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
                {
                    if (LoadAnimationBank(filePath))
                    {
                        loadedCount++;
                    }
                }
            }
        }
        closedir(dir);
    }
    else
    {
        SYSTEM_LOG << "[AnimationManager] Directory not found: " << directoryPath << "\n";
    }
#endif
    
    SYSTEM_LOG << "[AnimationManager] Loaded " << loadedCount << " animation banks\n";
    return (loadedCount > 0);
}

// ========================================================================
// Animation Graph Loading
// ========================================================================

bool AnimationManager::LoadAnimationGraph(const std::string& graphJsonPath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation graph from " << graphJsonPath << "\n";
    
    // Check if file exists
    if (!std::ifstream(graphJsonPath).good())
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: File not found: " << graphJsonPath << "\n";
        return false;
    }
    
    // Parse JSON file
    std::ifstream file(graphJsonPath);
    json jsonData;
    
    try
    {
        file >> jsonData;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: JSON parse error in " << graphJsonPath << ": " << e.what() << "\n";
        return false;
    }
    
    // Get graph ID from JSON
    if (!jsonData.contains("id"))
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: No 'id' field found in " << graphJsonPath << "\n";
        return false;
    }
    std::string graphId = jsonData["id"].get<std::string>();
    
    // Create animation graph
    AnimationGraph graph;
    graph.graphId = graphId;
    
    // Load default state
    if (jsonData.contains("defaultState"))
        graph.defaultState = jsonData["defaultState"].get<std::string>();
    
    // Load states
    if (jsonData.contains("states") && jsonData["states"].is_array())
    {
        for (auto& stateData : jsonData["states"])
        {
            AnimationState state;
            if (stateData.contains("name"))
                state.name = stateData["name"].get<std::string>();
            if (stateData.contains("animation"))
                state.animation = stateData["animation"].get<std::string>();
            if (stateData.contains("transitions") && stateData["transitions"].is_array())
            {
                for (auto& trans : stateData["transitions"])
                {
                    state.transitions.push_back(trans.get<std::string>());
                }
            }
            graph.states[state.name] = state;
        }
    }
    
    SYSTEM_LOG << "[AnimationManager] Loaded graph '" << graphId << "' with " 
               << graph.states.size() << " states\n";
    
    // Store graph
    m_graphs[graphId] = graph;
    return true;
}

bool AnimationManager::LoadAnimationGraphsFromDirectory(const std::string& directoryPath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation graphs from: " << directoryPath << "\n";
    
    int loadedCount = 0;
    
#ifdef _WIN32
    // Windows directory iteration
    WIN32_FIND_DATAA findData;
    std::string searchPath = directoryPath + "/*.json";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string fileName = findData.cFileName;
                std::string filePath = directoryPath + "/" + fileName;
                
                if (LoadAnimationGraph(filePath))
                {
                    loadedCount++;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    // POSIX directory iteration
    DIR* dir = opendir(directoryPath.c_str());
    if (dir)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL)
        {
            std::string fileName = entry->d_name;
            
            // Skip . and ..
            if (fileName == "." || fileName == "..")
                continue;
            
            // Check if it's a .json file
            if (fileName.length() > 5 && fileName.substr(fileName.length() - 5) == ".json")
            {
                std::string filePath = directoryPath + "/" + fileName;
                
                // Check if it's a regular file
                struct stat st;
                if (stat(filePath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
                {
                    if (LoadAnimationGraph(filePath))
                    {
                        loadedCount++;
                    }
                }
            }
        }
        closedir(dir);
    }
    else
    {
        SYSTEM_LOG << "[AnimationManager] Directory not found: " << directoryPath << "\n";
    }
#endif
    
    SYSTEM_LOG << "[AnimationManager] Loaded " << loadedCount << " animation graphs\n";
    return (loadedCount > 0);
}

// ========================================================================
// Access Methods
// ========================================================================

const AnimationBank* AnimationManager::GetBank(const std::string& bankId) const
{
    auto it = m_banks.find(bankId);
    if (it != m_banks.end())
        return &it->second;
    return nullptr;
}

const AnimationGraph* AnimationManager::GetGraph(const std::string& graphId) const
{
    auto it = m_graphs.find(graphId);
    if (it != m_graphs.end())
        return &it->second;
    return nullptr;
}

// ========================================================================
// Debug Methods
// ========================================================================

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

    const Olympe::AnimationBank& bank = bankIt->second;

    // Find the animation within the bank
    auto animIt = bank.animations.find(animName);
    if (animIt == bank.animations.end())
    {
        // Animation not found in bank
        return nullptr;
    }

    // Return pointer to the animation sequence
    return &animIt->second;
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

    return bankIt->second.animations.find(animName) != bankIt->second.animations.end();
}

void AnimationManager::ListLoadedBanks() const
{
    SYSTEM_LOG << "[AnimationManager] Loaded Animation Banks:\n";
    for (auto it = m_banks.begin(); it != m_banks.end(); ++it)
    {
        const std::string& bankId = it->first;
        const AnimationBank& bank = it->second;
        
        SYSTEM_LOG << "  - " << bankId << " (" << bank.animations.size() << " animations)\n";
        for (auto animIt = bank.animations.begin(); animIt != bank.animations.end(); ++animIt)
        {
            const std::string& animName = animIt->first;
            const AnimationSequence& anim = animIt->second;
            
            SYSTEM_LOG << "    * " << animName << ": " << anim.frames.size() << " frames, "
                       << (anim.loop ? "looping" : "one-shot") << "\n";
        }
    }
}

void AnimationManager::ListLoadedGraphs() const
{
    SYSTEM_LOG << "[AnimationManager] Loaded Animation Graphs:\n";
    for (auto it = m_graphs.begin(); it != m_graphs.end(); ++it)
    {
        const std::string& graphId = it->first;
        const AnimationGraph& graph = it->second;
        
        SYSTEM_LOG << "  - " << graphId << " (default: " << graph.defaultState << ", " << graph.states.size() << " states)\n";
    }
}

} // namespace Olympe
