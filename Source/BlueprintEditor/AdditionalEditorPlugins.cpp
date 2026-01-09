/*
 * Olympe Blueprint Editor - Additional Editor Plugins Implementation
 */

#include "AdditionalEditorPlugins.h"
#include "../../Source/third_party/imgui/imgui.h"
#include <chrono>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace Olympe
{
    static std::string GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        std::tm timeInfo;
        #ifdef _WIN32
        localtime_s(&timeInfo, &time);
        #else
        localtime_r(&time, &timeInfo);
        #endif
        ss << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }

    // ========================================================================
    // HFSM Editor Plugin
    // ========================================================================
    
    json HFSMEditorPlugin::CreateNew(const std::string& name)
    {
        json hfsm;
        hfsm["schema_version"] = 2;
        hfsm["blueprintType"] = "HFSM";
        hfsm["name"] = name;
        hfsm["description"] = "";
        hfsm["metadata"]["author"] = "Atlasbruce";
        hfsm["metadata"]["created"] = GetCurrentTimestamp();
        hfsm["metadata"]["tags"] = json::array();
        hfsm["editorState"]["zoom"] = 1.0;
        hfsm["editorState"]["scrollOffset"] = nlohmann::json::object();
        hfsm["editorState"]["scrollOffset"]["x"] = 0;
        hfsm["editorState"]["scrollOffset"]["y"] = 0;
        hfsm["data"]["initialState"] = "Idle";
        hfsm["data"]["states"] = json::array();
        hfsm["data"]["transitions"] = json::array();
        return hfsm;
    }
    
    bool HFSMEditorPlugin::CanHandle(const json& blueprint) const
    {
        if (blueprint.contains("blueprintType") && blueprint["blueprintType"].is_string() && blueprint["blueprintType"].get<std::string>() == "HFSM")
            return true;
        return blueprint.contains("states") || blueprint.contains("initialState");
    }
    
    std::vector<ValidationError> HFSMEditorPlugin::Validate(const json& blueprint)
    {
        std::vector<ValidationError> errors;
        if (!blueprint.contains("data"))
        {
            ValidationError error(-1, "", "Missing data section", ErrorSeverity::Error);
            errors.push_back(error);
        }
        return errors;
    }
    
    void HFSMEditorPlugin::RenderEditor(json& blueprintData, EditorContext& ctx)
    {
        ImGui::Text("HFSM Editor");
        ImGui::Text("States: %d", blueprintData.contains("data") && blueprintData["data"].contains("states") ? 
                    (int)blueprintData["data"]["states"].size() : 0);
    }
    
    void HFSMEditorPlugin::RenderProperties(const json& blueprintData)
    {
        ImGui::Text("Type: HFSM");
    }
    
    void HFSMEditorPlugin::RenderToolbar(json& blueprintData)
    {
        ImGui::Button("Add State");
    }

    // ========================================================================
    // Animation Graph Editor Plugin
    // ========================================================================
    
    json AnimationGraphEditorPlugin::CreateNew(const std::string& name)
    {
        json anim;
        anim["schema_version"] = 2;
        anim["blueprintType"] = "AnimationGraph";
        anim["name"] = name;
        anim["description"] = "";
        anim["metadata"]["author"] = "Atlasbruce";
        anim["metadata"]["created"] = GetCurrentTimestamp();
        anim["metadata"]["tags"] = json::array();
        anim["editorState"]["zoom"] = 1.0;
        anim["editorState"]["scrollOffset"] = nlohmann::json::object();
        anim["editorState"]["scrollOffset"]["x"] = 0;
        anim["editorState"]["scrollOffset"]["y"] = 0;
        anim["data"]["initialState"] = "Idle";
        anim["data"]["states"] = json::array();
        anim["data"]["transitions"] = json::array();
        return anim;
    }
    
    bool AnimationGraphEditorPlugin::CanHandle(const json& blueprint) const
    {
        if (blueprint.contains("blueprintType") && blueprint["blueprintType"].is_string() && blueprint["blueprintType"].get<std::string>() == "AnimationGraph")
            return true;
        return false;
    }
    
    std::vector<ValidationError> AnimationGraphEditorPlugin::Validate(const json& blueprint)
    {
        std::vector<ValidationError> errors;
        if (!blueprint.contains("data"))
        {
            ValidationError error(-1, "", "Missing data section", ErrorSeverity::Error);
            errors.push_back(error);
        }
        return errors;
    }
    
    void AnimationGraphEditorPlugin::RenderEditor(json& blueprintData, EditorContext& ctx)
    {
        ImGui::Text("Animation Graph Editor");
        if (blueprintData.contains("data") && blueprintData["data"].contains("states"))
        {
            const json& states = blueprintData["data"]["states"];
            ImGui::Text("Animation States: %d", (int)states.size());
            
            for (size_t i = 0; i < states.size(); ++i)
            {
                const json& state = states[i];
                std::string stateName = state.contains("name") && state["name"].is_string()
                    ? state["name"].get<std::string>()
                    : "Unnamed";
                ImGui::BulletText("%s", stateName.c_str());
            }
        }
    }
    
    void AnimationGraphEditorPlugin::RenderProperties(const json& blueprintData)
    {
        ImGui::Text("Type: Animation Graph");
    }
    
    void AnimationGraphEditorPlugin::RenderToolbar(json& blueprintData)
    {
        ImGui::Button("Add Animation State");
    }

    // ========================================================================
    // Scripted Event Editor Plugin
    // ========================================================================
    
    json ScriptedEventEditorPlugin::CreateNew(const std::string& name)
    {
        json event;
        event["schema_version"] = 2;
        event["blueprintType"] = "ScriptedEvent";
        event["name"] = name;
        event["description"] = "";
        event["metadata"]["author"] = "Atlasbruce";
        event["metadata"]["created"] = GetCurrentTimestamp();
        event["metadata"]["tags"] = json::array();
        event["editorState"]["zoom"] = 1.0;
        event["editorState"]["scrollOffset"] = nlohmann::json::object();
        event["editorState"]["scrollOffset"]["x"] = 0;
        event["editorState"]["scrollOffset"]["y"] = 0;
        event["data"]["triggerType"] = "Manual";
        event["data"]["oneShot"] = true;
        event["data"]["sequence"] = json::array();
        return event;
    }
    
    bool ScriptedEventEditorPlugin::CanHandle(const json& blueprint) const
    {
        if (blueprint.contains("blueprintType") && blueprint["blueprintType"].is_string() && blueprint["blueprintType"].get<std::string>() == "ScriptedEvent")
            return true;
        return false;
    }
    
    std::vector<ValidationError> ScriptedEventEditorPlugin::Validate(const json& blueprint)
    {
        std::vector<ValidationError> errors;
        if (!blueprint.contains("data"))
        {
            ValidationError error(-1, "", "Missing data section", ErrorSeverity::Error);
            errors.push_back(error);
        }
        return errors;
    }
    
    void ScriptedEventEditorPlugin::RenderEditor(json& blueprintData, EditorContext& ctx)
    {
        ImGui::Text("Scripted Event Editor");
        if (blueprintData.contains("data") && blueprintData["data"].contains("sequence"))
        {
            const json& sequence = blueprintData["data"]["sequence"];
            ImGui::Text("Steps: %d", (int)sequence.size());
            
            for (size_t i = 0; i < sequence.size(); ++i)
            {
                const json& step = sequence[i];
                std::string stepType = step.contains("type") && step["type"].is_string()
                    ? step["type"].get<std::string>()
                    : "Unknown";
                int stepNum = step.contains("step") && step["step"].is_number()
                    ? step["step"].get<int>()
                    : 0;
                ImGui::BulletText("Step %d: %s", stepNum, stepType.c_str());
            }
        }
    }
    
    void ScriptedEventEditorPlugin::RenderProperties(const json& blueprintData)
    {
        ImGui::Text("Type: Scripted Event");
    }
    
    void ScriptedEventEditorPlugin::RenderToolbar(json& blueprintData)
    {
        ImGui::Button("Add Step");
    }

    // ========================================================================
    // Level Definition Editor Plugin
    // ========================================================================
    
    json LevelDefinitionEditorPlugin::CreateNew(const std::string& name)
    {
        json level;
        level["schema_version"] = 2;
        level["blueprintType"] = "LevelDefinition";
        level["name"] = name;
        level["description"] = "";
        level["metadata"]["author"] = "Atlasbruce";
        level["metadata"]["created"] = GetCurrentTimestamp();
        level["metadata"]["tags"] = json::array();
        level["editorState"]["zoom"] = 0.5;
        level["editorState"]["scrollOffset"] = nlohmann::json::object();
        level["editorState"]["scrollOffset"]["x"] = 0;
        level["editorState"]["scrollOffset"]["y"] = 0;
        level["data"]["levelName"] = name;
        nlohmann::json worldSize = nlohmann::json::object();
        worldSize["width"] = 1024;
        worldSize["height"] = 768;
        level["data"]["worldSize"] = worldSize;
        level["data"]["entities"] = json::array();
        return level;
    }
    
    bool LevelDefinitionEditorPlugin::CanHandle(const json& blueprint) const
    {
        if (blueprint.contains("blueprintType") && blueprint["blueprintType"].is_string() && blueprint["blueprintType"].get<std::string>() == "LevelDefinition")
            return true;
        return false;
    }
    
    std::vector<ValidationError> LevelDefinitionEditorPlugin::Validate(const json& blueprint)
    {
        std::vector<ValidationError> errors;
        if (!blueprint.contains("data"))
        {
            ValidationError error(-1, "", "Missing data section", ErrorSeverity::Error);
            errors.push_back(error);
        }
        return errors;
    }
    
    void LevelDefinitionEditorPlugin::RenderEditor(json& blueprintData, EditorContext& ctx)
    {
        ImGui::Text("Level Definition Editor");
        if (blueprintData.contains("data"))
        {
            auto& data = blueprintData["data"];
            ImGui::Text("Level: %s", (data.contains("levelName") && data["levelName"].is_string() ? data["levelName"].get<std::string>() : "Unnamed").c_str());
            
            if (data.contains("entities"))
                ImGui::Text("Entities: %d", (int)data["entities"].size());
            
            if (data.contains("objectives"))
                ImGui::Text("Objectives: %d", (int)data["objectives"].size());
        }
    }
    
    void LevelDefinitionEditorPlugin::RenderProperties(const json& blueprintData)
    {
        ImGui::Text("Type: Level Definition");
    }
    
    void LevelDefinitionEditorPlugin::RenderToolbar(json& blueprintData)
    {
        ImGui::Button("Add Entity");
        ImGui::SameLine();
        ImGui::Button("Add Objective");
    }

    // ========================================================================
    // UI Menu Editor Plugin
    // ========================================================================
    
    json UIMenuEditorPlugin::CreateNew(const std::string& name)
    {
        json menu;
        menu["schema_version"] = 2;
        menu["blueprintType"] = "UIMenu";
        menu["name"] = name;
        menu["description"] = "";
        menu["metadata"]["author"] = "Atlasbruce";
        menu["metadata"]["created"] = GetCurrentTimestamp();
        menu["metadata"]["tags"] = json::array();
        menu["editorState"]["zoom"] = 1.0;
        menu["editorState"]["scrollOffset"] = nlohmann::json::object();
        menu["editorState"]["scrollOffset"]["x"] = 0;
        menu["editorState"]["scrollOffset"]["y"] = 0;
        menu["data"]["menuName"] = name;
        menu["data"]["elements"] = json::array();
        return menu;
    }
    
    bool UIMenuEditorPlugin::CanHandle(const json& blueprint) const
    {
        if (blueprint.contains("blueprintType") && blueprint["blueprintType"].is_string() && blueprint["blueprintType"].get<std::string>() == "UIMenu")
            return true;
        return false;
    }
    
    std::vector<ValidationError> UIMenuEditorPlugin::Validate(const json& blueprint)
    {
        std::vector<ValidationError> errors;
        if (!blueprint.contains("data"))
        {
            ValidationError error(-1, "", "Missing data section", ErrorSeverity::Error);
            errors.push_back(error);
        }
        return errors;
    }
    
    void UIMenuEditorPlugin::RenderEditor(json& blueprintData, EditorContext& ctx)
    {
        ImGui::Text("UI Menu Editor");
        if (blueprintData.contains("data") && blueprintData["data"].contains("elements"))
        {
            const json& elements = blueprintData["data"]["elements"];
            ImGui::Text("UI Elements: %d", (int)elements.size());
            
            for (size_t i = 0; i < elements.size(); ++i)
            {
                const json& element = elements[i];
                std::string elemType = element.contains("type") && element["type"].is_string()
                    ? element["type"].get<std::string>()
                    : "Unknown";
                std::string elemId = element.contains("id") && element["id"].is_string()
                    ? element["id"].get<std::string>()
                    : "unnamed";
                ImGui::BulletText("%s (%s)", elemId.c_str(), elemType.c_str());
            }
        }
    }
    
    void UIMenuEditorPlugin::RenderProperties(const json& blueprintData)
    {
        ImGui::Text("Type: UI Menu");
    }
    
    void UIMenuEditorPlugin::RenderToolbar(json& blueprintData)
    {
        ImGui::Button("Add UI Element");
    }
}
