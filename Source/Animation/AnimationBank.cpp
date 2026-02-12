/*
Olympe Engine V2 2025
Animation System - Animation Bank Implementation
*/

#include "Animation/AnimationBank.h"
#include "json_helper.h"
#include "DataManager.h"
#include "system/system_utils.h"
#include <fstream>

using json = nlohmann::json;

namespace OlympeAnimation
{
    // ========================================================================
    // AnimationBank Implementation
    // ========================================================================

    bool AnimationBank::LoadFromFile(const std::string& filePath)
    {
        json j;
        if (!JsonHelper::LoadJsonFromFile(filePath, j))
        {
            SYSTEM_LOG << "AnimationBank: Failed to load file: " << filePath << "\n";
            return false;
        }

        return ParseJSON(j.dump());
    }

    bool AnimationBank::ParseJSON(const std::string& jsonContent)
    {
        try
        {
            json j = json::parse(jsonContent);

            // Parse basic info
            m_bankName = JsonHelper::GetString(j, "bankName", "unknown");
            m_description = JsonHelper::GetString(j, "description", "");

            // Parse spritesheets
            if (j.contains("spritesheets") && j["spritesheets"].is_array())
            {
                for (const auto& sheetJson : j["spritesheets"])
                {
                    SpriteSheet sheet;
                    sheet.id = JsonHelper::GetString(sheetJson, "id", "");
                    sheet.path = JsonHelper::GetString(sheetJson, "path", "");
                    sheet.frameWidth = JsonHelper::GetInt(sheetJson, "frameWidth", 32);
                    sheet.frameHeight = JsonHelper::GetInt(sheetJson, "frameHeight", 32);
                    sheet.columns = JsonHelper::GetInt(sheetJson, "columns", 1);
                    sheet.rows = JsonHelper::GetInt(sheetJson, "rows", 1);
                    sheet.totalFrames = JsonHelper::GetInt(sheetJson, "totalFrames", 1);

                    // Parse hotspot
                    if (sheetJson.contains("hotspot") && sheetJson["hotspot"].is_object())
                    {
                        sheet.hotspot.x = JsonHelper::GetFloat(sheetJson["hotspot"], "x", 16.0f);
                        sheet.hotspot.y = JsonHelper::GetFloat(sheetJson["hotspot"], "y", 16.0f);
                    }
                    else
                    {
                        // Default to center of frame
                        sheet.hotspot.x = sheet.frameWidth / 2.0f;
                        sheet.hotspot.y = sheet.frameHeight / 2.0f;
                    }

                    // Load texture via DataManager
                    sheet.texture = DataManager::Get().GetTexture(sheet.id);
                    if (sheet.texture == nullptr && !sheet.path.empty())
                    {
                        DataManager::Get().PreloadTexture(sheet.id, sheet.path);
                        sheet.texture = DataManager::Get().GetTexture(sheet.id);
                    }

                    m_spritesheets[sheet.id] = sheet;
                }
            }

            // Parse animations
            if (j.contains("animations") && j["animations"].is_array())
            {
                for (const auto& animJson : j["animations"])
                {
                    Animation anim;
                    anim.name = JsonHelper::GetString(animJson, "name", "");
                    anim.spritesheetId = JsonHelper::GetString(animJson, "spritesheetId", "");
                    anim.startFrame = JsonHelper::GetInt(animJson, "startFrame", 0);
                    anim.endFrame = JsonHelper::GetInt(animJson, "endFrame", 0);
                    anim.framerate = JsonHelper::GetFloat(animJson, "framerate", 12.0f);
                    anim.looping = JsonHelper::GetBool(animJson, "looping", true);

                    // Parse events
                    if (animJson.contains("events") && animJson["events"].is_array())
                    {
                        for (const auto& eventJson : animJson["events"])
                        {
                            AnimationEvent event;
                            event.frame = JsonHelper::GetInt(eventJson, "frame", 0);
                            event.type = JsonHelper::GetString(eventJson, "type", "");
                            
                            // Store the entire "data" field as JSON string
                            if (eventJson.contains("data"))
                            {
                                event.dataJson = eventJson["data"].dump();
                            }

                            anim.events.push_back(event);
                        }
                    }

                    m_animations[anim.name] = anim;
                }
            }

            m_isValid = true;
            SYSTEM_LOG << "AnimationBank: Successfully loaded '" << m_bankName 
                      << "' with " << m_spritesheets.size() << " spritesheets and " 
                      << m_animations.size() << " animations\n";
            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "AnimationBank: Error parsing JSON: " << e.what() << "\n";
            return false;
        }
    }

    Animation* AnimationBank::GetAnimation(const std::string& name)
    {
        auto it = m_animations.find(name);
        if (it != m_animations.end())
            return &it->second;
        return nullptr;
    }

    const Animation* AnimationBank::GetAnimation(const std::string& name) const
    {
        auto it = m_animations.find(name);
        if (it != m_animations.end())
            return &it->second;
        return nullptr;
    }

    SpriteSheet* AnimationBank::GetSpriteSheet(const std::string& id)
    {
        auto it = m_spritesheets.find(id);
        if (it != m_spritesheets.end())
            return &it->second;
        return nullptr;
    }

    const SpriteSheet* AnimationBank::GetSpriteSheet(const std::string& id) const
    {
        auto it = m_spritesheets.find(id);
        if (it != m_spritesheets.end())
            return &it->second;
        return nullptr;
    }

} // namespace OlympeAnimation
