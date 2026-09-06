#include "AdditionalEditorPlugins.h"
#include "../../Source/third_party/imgui/imgui.h"

using json = nlohmann::json;

namespace Olympe
{
    json HFSMEditorPlugin::CreateNew(const std::string& name)
    {
        json root;
        root["schema_version"] = 2;
        root["blueprintType"] = "HFSM";
        root["name"] = name;
        root["description"] = "";
        root["data"]["initialState"] = "Idle";
        root["data"]["states"] = json::array();
        root["data"]["transitions"] = json::array();
        return root;
    }
    bool HFSMEditorPlugin::CanHandle(const json& blueprint) const { return blueprint.value("blueprintType", "") == "HFSM"; }
    std::vector<ValidationError> HFSMEditorPlugin::Validate(const json& blueprint) { std::vector<ValidationError> errors; if (!blueprint.contains("data")) errors.push_back(ValidationError(-1, "", "Missing data section", ErrorSeverity::Error)); return errors; }
    void HFSMEditorPlugin::RenderEditor(json&, EditorContext_st&) { ImGui::Text("HFSM Editor"); }
    void HFSMEditorPlugin::RenderProperties(const json&) { ImGui::Text("Type: HFSM"); }
    void HFSMEditorPlugin::RenderToolbar(json&) { ImGui::Button("Add State"); }

    json AnimationGraphEditorPlugin::CreateNew(const std::string& name)
    {
        json root;
        root["schema_version"] = 2;
        root["blueprintType"] = "AnimationGraph";
        root["name"] = name;
        root["description"] = "";
        root["defaultState"] = "Idle";
        root["sources"] = json::array();
        root["states"] = json::array();
        root["transitions"] = json::array();
        return root;
    }
    bool AnimationGraphEditorPlugin::CanHandle(const json& blueprint) const { return blueprint.value("blueprintType", "") == "AnimationGraph"; }
    std::vector<ValidationError> AnimationGraphEditorPlugin::Validate(const json& blueprint) { std::vector<ValidationError> errors; if (!blueprint.contains("sources") || !blueprint["sources"].is_array() || blueprint["sources"].empty()) errors.push_back(ValidationError(-1, "", "Animation graph must import at least one TSX source", ErrorSeverity::Error)); if (!blueprint.contains("states") || !blueprint["states"].is_array() || blueprint["states"].empty()) errors.push_back(ValidationError(-1, "", "Animation graph must define at least one state", ErrorSeverity::Error)); return errors; }
    void AnimationGraphEditorPlugin::RenderEditor(json& blueprintData, EditorContext_st&) { ImGui::Text("Animation Graph Editor"); ImGui::Text("Sources: %d", blueprintData.contains("sources") ? (int)blueprintData["sources"].size() : 0); ImGui::Text("States: %d", blueprintData.contains("states") ? (int)blueprintData["states"].size() : 0); }
    void AnimationGraphEditorPlugin::RenderProperties(const json&) { ImGui::Text("Type: Animation Graph"); }
    void AnimationGraphEditorPlugin::RenderToolbar(json&) { ImGui::Button("Import TSX"); ImGui::SameLine(); ImGui::Button("Add State"); }

    json ScriptedEventEditorPlugin::CreateNew(const std::string& name) { json root; root["schema_version"] = 2; root["blueprintType"] = "ScriptedEvent"; root["name"] = name; root["description"] = ""; root["data"]["sequence"] = json::array(); return root; }
    bool ScriptedEventEditorPlugin::CanHandle(const json& blueprint) const { return blueprint.value("blueprintType", "") == "ScriptedEvent"; }
    std::vector<ValidationError> ScriptedEventEditorPlugin::Validate(const json& blueprint) { std::vector<ValidationError> errors; if (!blueprint.contains("data")) errors.push_back(ValidationError(-1, "", "Missing data section", ErrorSeverity::Error)); return errors; }
    void ScriptedEventEditorPlugin::RenderEditor(json&, EditorContext_st&) { ImGui::Text("Scripted Event Editor"); }
    void ScriptedEventEditorPlugin::RenderProperties(const json&) { ImGui::Text("Type: Scripted Event"); }
    void ScriptedEventEditorPlugin::RenderToolbar(json&) { ImGui::Button("Add Step"); }

    json LevelDefinitionEditorPlugin::CreateNew(const std::string& name) { json root; root["schema_version"] = 2; root["blueprintType"] = "LevelDefinition"; root["name"] = name; root["description"] = ""; root["data"]["levelName"] = name; return root; }
    bool LevelDefinitionEditorPlugin::CanHandle(const json& blueprint) const { return blueprint.value("blueprintType", "") == "LevelDefinition"; }
    std::vector<ValidationError> LevelDefinitionEditorPlugin::Validate(const json& blueprint) { std::vector<ValidationError> errors; if (!blueprint.contains("data")) errors.push_back(ValidationError(-1, "", "Missing data section", ErrorSeverity::Error)); return errors; }
    void LevelDefinitionEditorPlugin::RenderEditor(json&, EditorContext_st&) { ImGui::Text("Level Definition Editor"); }
    void LevelDefinitionEditorPlugin::RenderProperties(const json&) { ImGui::Text("Type: Level Definition"); }
    void LevelDefinitionEditorPlugin::RenderToolbar(json&) { ImGui::Button("Add Entity"); }

    json UIMenuEditorPlugin::CreateNew(const std::string& name) { json root; root["schema_version"] = 2; root["blueprintType"] = "UIMenu"; root["name"] = name; root["description"] = ""; root["data"]["menuName"] = name; return root; }
    bool UIMenuEditorPlugin::CanHandle(const json& blueprint) const { return blueprint.value("blueprintType", "") == "UIMenu"; }
    std::vector<ValidationError> UIMenuEditorPlugin::Validate(const json& blueprint) { std::vector<ValidationError> errors; if (!blueprint.contains("data")) errors.push_back(ValidationError(-1, "", "Missing data section", ErrorSeverity::Error)); return errors; }
    void UIMenuEditorPlugin::RenderEditor(json&, EditorContext_st&) { ImGui::Text("UI Menu Editor"); }
    void UIMenuEditorPlugin::RenderProperties(const json&) { ImGui::Text("Type: UI Menu"); }
    void UIMenuEditorPlugin::RenderToolbar(json&) { ImGui::Button("Add UI Element"); }
}
