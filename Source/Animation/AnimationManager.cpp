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
#include <filesystem>
#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

namespace Olympe
{

// ========================================================================
// Animation Bank Loading
// ========================================================================

bool AnimationManager::LoadAnimationBank(const std::string& bankId, const std::string& filePath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation bank: " << bankId << " from " << filePath << "\n";
    
    // Check if file exists
    if (!std::ifstream(filePath).good())
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: File not found: " << filePath << "\n";
        return false;
    }
    
    // Parse JSON file
    std::ifstream file(filePath);
    json jsonData;
    
    try
    {
        file >> jsonData;
    }
    catch (const json::exception& e)
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: JSON parse error in " << filePath << ": " << e.what() << "\n";
        return false;
    }
    
    // Create animation bank
    auto bank = std::unique_ptr<AnimationBank>(new AnimationBank());
    bank->bankId = bankId;
    
    // Load spritesheet metadata
    if (jsonData.contains("frameWidth"))
        bank->frameWidth = jsonData["frameWidth"].get<int>();
    if (jsonData.contains("frameHeight"))
        bank->frameHeight = jsonData["frameHeight"].get<int>();
    if (jsonData.contains("columns"))
        bank->columns = jsonData["columns"].get<int>();
    if (jsonData.contains("spacing"))
        bank->spacing = jsonData["spacing"].get<int>();
    if (jsonData.contains("margin"))
        bank->margin = jsonData["margin"].get<int>();
    
    // Load animations
    if (!jsonData.contains("animations") || !jsonData["animations"].is_object())
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: No 'animations' object found in " << filePath << "\n";
        return false;
    }
    
    int animCount = 0;
    for (auto& [animName, animData] : jsonData["animations"].items())
    {
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
                    frame.srcRect = bank->CalculateFrameRect(startFrame + i);
                    frame.duration = frameDuration;
                    frame.hotSpot.x = bank->frameWidth / 2.0f;
                    frame.hotSpot.y = bank->frameHeight / 2.0f;
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
                frame.srcRect = bank->CalculateFrameRect(i);
                frame.duration = frameDuration;
                frame.hotSpot.x = bank->frameWidth / 2.0f;
                frame.hotSpot.y = bank->frameHeight / 2.0f;
                sequence.frames.push_back(frame);
            }
        }
        
        // Preload spritesheet texture via DataManager
        if (!sequence.spritesheetPath.empty())
        {
            std::string textureId = bankId + "_" + animName;
            DataManager::Get().PreloadSprite(textureId, sequence.spritesheetPath);
        }
        
        bank->animations[animName] = sequence;
        animCount++;
    }
    
    SYSTEM_LOG << "[AnimationManager] Loaded " << animCount << " animations for bank '" << bankId << "'\n";
    
    // Store bank
    m_banks[bankId] = std::move(bank);
    return true;
}

int AnimationManager::LoadAnimationBanksFromDirectory(const std::string& directoryPath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation banks from: " << directoryPath << "\n";
    
    int loadedCount = 0;
    
    try
    {
        // Check if directory exists
        if (!std::filesystem::exists(directoryPath))
        {
            SYSTEM_LOG << "[AnimationManager] Directory not found: " << directoryPath << "\n";
            return 0;
        }
        
        // Iterate through directory
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                std::string bankId = entry.path().stem().string();
                std::string filePath = entry.path().string();
                
                if (LoadAnimationBank(bankId, filePath))
                {
                    loadedCount++;
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        SYSTEM_LOG << "[AnimationManager] Filesystem error: " << e.what() << "\n";
    }
    
    SYSTEM_LOG << "[AnimationManager] Loaded " << loadedCount << " animation banks\n";
    return loadedCount;
}

// ========================================================================
// Animation Graph Loading
// ========================================================================

bool AnimationManager::LoadAnimationGraph(const std::string& graphId, const std::string& filePath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation graph: " << graphId << " from " << filePath << "\n";
    
    // Check if file exists
    if (!std::ifstream(filePath).good())
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: File not found: " << filePath << "\n";
        return false;
    }
    
    // Parse JSON file
    std::ifstream file(filePath);
    json jsonData;
    
    try
    {
        file >> jsonData;
    }
    catch (const json::exception& e)
    {
        SYSTEM_LOG << "[AnimationManager] ERROR: JSON parse error in " << filePath << ": " << e.what() << "\n";
        return false;
    }
    
    // Create animation graph
    auto graph = std::unique_ptr<AnimationGraph>(new AnimationGraph());
    graph->graphId = graphId;
    
    // Load default state
    if (jsonData.contains("defaultState"))
        graph->defaultState = jsonData["defaultState"].get<std::string>();
    
    // Load states
    if (jsonData.contains("states") && jsonData["states"].is_array())
    {
        for (auto& state : jsonData["states"])
        {
            graph->states.push_back(state.get<std::string>());
        }
    }
    
    // Load transitions
    if (jsonData.contains("transitions") && jsonData["transitions"].is_object())
    {
        for (auto& [fromState, toStates] : jsonData["transitions"].items())
        {
            if (toStates.is_array())
            {
                for (auto& toState : toStates)
                {
                    AnimationTransition transition;
                    transition.fromState = fromState;
                    transition.toState = toState.get<std::string>();
                    graph->transitions.push_back(transition);
                }
            }
        }
    }
    
    SYSTEM_LOG << "[AnimationManager] Loaded graph '" << graphId << "' with " 
               << graph->states.size() << " states and " 
               << graph->transitions.size() << " transitions\n";
    
    // Store graph
    m_graphs[graphId] = std::move(graph);
    return true;
}

int AnimationManager::LoadAnimationGraphsFromDirectory(const std::string& directoryPath)
{
    SYSTEM_LOG << "[AnimationManager] Loading animation graphs from: " << directoryPath << "\n";
    
    int loadedCount = 0;
    
    try
    {
        // Check if directory exists
        if (!std::filesystem::exists(directoryPath))
        {
            SYSTEM_LOG << "[AnimationManager] Directory not found: " << directoryPath << "\n";
            return 0;
        }
        
        // Iterate through directory
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                std::string graphId = entry.path().stem().string();
                std::string filePath = entry.path().string();
                
                if (LoadAnimationGraph(graphId, filePath))
                {
                    loadedCount++;
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        SYSTEM_LOG << "[AnimationManager] Filesystem error: " << e.what() << "\n";
    }
    
    SYSTEM_LOG << "[AnimationManager] Loaded " << loadedCount << " animation graphs\n";
    return loadedCount;
}

// ========================================================================
// Access Methods
// ========================================================================

const AnimationBank* AnimationManager::GetAnimationBank(const std::string& bankId) const
{
    auto it = m_banks.find(bankId);
    if (it != m_banks.end())
        return it->second.get();
    return nullptr;
}

const AnimationGraph* AnimationManager::GetAnimationGraph(const std::string& graphId) const
{
    auto it = m_graphs.find(graphId);
    if (it != m_graphs.end())
        return it->second.get();
    return nullptr;
}

const AnimationSequence* AnimationManager::GetAnimationSequence(const std::string& bankId, 
                                                                 const std::string& animName) const
{
    const AnimationBank* bank = GetAnimationBank(bankId);
    if (!bank)
        return nullptr;
    
    auto it = bank->animations.find(animName);
    if (it != bank->animations.end())
        return &it->second;
    
    return nullptr;
}

bool AnimationManager::HasAnimationBank(const std::string& bankId) const
{
    return m_banks.find(bankId) != m_banks.end();
}

bool AnimationManager::HasAnimationGraph(const std::string& graphId) const
{
    return m_graphs.find(graphId) != m_graphs.end();
}

void AnimationManager::Clear()
{
    m_banks.clear();
    m_graphs.clear();
    SYSTEM_LOG << "[AnimationManager] Cleared all animation data\n";
}

// ========================================================================
// Debug Methods
// ========================================================================

void AnimationManager::ListLoadedBanks() const
{
    SYSTEM_LOG << "[AnimationManager] Loaded Animation Banks:\n";
    for (const auto& [bankId, bank] : m_banks)
    {
        SYSTEM_LOG << "  - " << bankId << " (" << bank->animations.size() << " animations)\n";
        for (const auto& [animName, anim] : bank->animations)
        {
            SYSTEM_LOG << "    * " << animName << ": " << anim.frames.size() << " frames, "
                       << (anim.loop ? "looping" : "one-shot") << "\n";
        }
    }
}

void AnimationManager::ListLoadedGraphs() const
{
    SYSTEM_LOG << "[AnimationManager] Loaded Animation Graphs:\n";
    for (const auto& [graphId, graph] : m_graphs)
    {
        SYSTEM_LOG << "  - " << graphId << " (default: " << graph->defaultState << ")\n";
        SYSTEM_LOG << "    States: ";
        for (size_t i = 0; i < graph->states.size(); ++i)
        {
            SYSTEM_LOG << graph->states[i];
            if (i < graph->states.size() - 1)
                SYSTEM_LOG << ", ";
        }
        SYSTEM_LOG << "\n";
    }
}

} // namespace Olympe
