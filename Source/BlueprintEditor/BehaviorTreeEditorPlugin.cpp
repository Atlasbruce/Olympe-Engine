/*
 * Olympe Blueprint Editor - Behavior Tree Plugin Implementation
 */

#include "BehaviorTreeEditorPlugin.h"
#include "../../Source/third_party/imgui/imgui.h"
#include <chrono>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace Olympe
{
    BehaviorTreeEditorPlugin::BehaviorTreeEditorPlugin()
    {
    }
    
    BehaviorTreeEditorPlugin::~BehaviorTreeEditorPlugin()
    {
    }
    
    std::string BehaviorTreeEditorPlugin::GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        
        // Use localtime_s for MSVC, localtime_r for other platforms
        #ifdef _MSC_VER
            std::tm timeinfo;
            localtime_s(&timeinfo, &time);
            ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
        #else
            ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
        #endif
        
        return ss.str();
    }
    
    json BehaviorTreeEditorPlugin::CreateNew(const std::string& name)
    {
        json bt;
        bt["schema_version"] = 2;
        bt["blueprintType"] = "BehaviorTree";
        bt["name"] = name;
        bt["description"] = "";
        bt["metadata"]["author"] = "Atlasbruce";
        bt["metadata"]["created"] = GetCurrentTimestamp();
        bt["metadata"]["lastModified"] = GetCurrentTimestamp();
        bt["metadata"]["tags"] = json::array();
        bt["editorState"]["zoom"] = 1.0;
        bt["editorState"]["scrollOffset"] = nlohmann::json::object();
        bt["editorState"]["scrollOffset"]["x"] = 0;
        bt["editorState"]["scrollOffset"]["y"] = 0;
        
        // Root node by default
        bt["data"]["rootNodeId"] = 1;
        json rootNode;
        rootNode["id"] = 1;
        rootNode["name"] = "Root Selector";
        rootNode["type"] = "Selector";
        nlohmann::json position = nlohmann::json::object();
        position["x"] = 400;
        position["y"] = 300;
        rootNode["position"] = position;
        rootNode["children"] = json::array();
        rootNode["parameters"] = json::object();
        bt["data"]["nodes"] = json::array();
        bt["data"]["nodes"].push_back(rootNode);
        
        return bt;
    }
    
    bool BehaviorTreeEditorPlugin::CanHandle(const json& blueprint) const
    {
        // V2 format check
        if (blueprint.contains("blueprintType") && 
            blueprint["blueprintType"].get<std::string>() == "BehaviorTree")
        {
            return true;
        }
        
        // V1 format heuristic
        return blueprint.contains("rootNodeId") && 
               blueprint.contains("nodes") &&
               !blueprint.contains("states"); // Not an HFSM
    }
    
    std::vector<ValidationError> BehaviorTreeEditorPlugin::Validate(const json& blueprint)
    {
        std::vector<ValidationError> errors;
        
        // Check if data section exists
        if (!blueprint.contains("data"))
        {
            errors.push_back(ValidationError(-1, "", "Missing 'data' section", ErrorSeverity::Error));
            return errors;
        }
        
        const json& data = blueprint["data"];
        
        // Check root node exists
        int rootId = data.contains("rootNodeId") ? data["rootNodeId"].get<int>() : -1;
        if (rootId == -1)
        {
            errors.push_back(ValidationError(-1, "", "Missing rootNodeId", ErrorSeverity::Error));
            return errors;
        }
        
        bool rootFound = false;
        
        if (data.contains("nodes") && data["nodes"].is_array())
        {
            for (size_t i = 0; i < data["nodes"].size(); ++i)
            {
                const nlohmann::json& nodeObj = data["nodes"][i];
                int nodeId = nodeObj.contains("id") && nodeObj["id"].is_number() ? nodeObj["id"].get<int>() : -1;
                if (nodeId == rootId)
                {
                    rootFound = true;
                    break;
                }
            }
        }
        
        if (!rootFound)
        {
            errors.push_back(ValidationError(-1, "", "Root node not found in nodes list", ErrorSeverity::Error));
        }
        
        // Verify all children exist
        if (data.contains("nodes") && data["nodes"].is_array())
        {
            for (size_t i = 0; i < data["nodes"].size(); ++i)
            {
                const nlohmann::json& node = data["nodes"][i];
                if (node.contains("children") && node["children"].is_array())
                {
                    for (size_t j = 0; j < node["children"].size(); ++j)
                    {
                        const nlohmann::json& childIdJson = node["children"][j];
                        int cid = childIdJson.is_number() ? childIdJson.get<int>() : -1;
                        bool found = false;

                        for (size_t k = 0; k < data["nodes"].size(); ++k)
                        {
                            const nlohmann::json& n = data["nodes"][k];
                            if (n.is_object() && n.contains("id") && n["id"].is_number() && n["id"].get<int>() == cid)
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            int nodeId = node.contains("id") && node["id"].is_number() ? node["id"].get<int>() : -1;
                            std::string nodeIdStr = std::to_string(nodeId);
                            errors.push_back(ValidationError(-1, 
                                nodeIdStr,
                                "Child node " + std::to_string(cid) + " not found",
                                ErrorSeverity::Error
                            ));
                        }
                    }
                }
            }
        }
        
        return errors;
    }
    
    void BehaviorTreeEditorPlugin::RenderEditor(json& blueprintData, EditorContext& ctx)
    {
        if (!blueprintData.contains("data"))
        {
            ImGui::Text("Invalid blueprint: missing 'data' section");
            return;
        }
        
        RenderBehaviorTreeGraph(blueprintData["data"], ctx);
    }
    
    void BehaviorTreeEditorPlugin::RenderProperties(const json& blueprintData)
    {
        ImGui::Text("Type: Behavior Tree");
        
        if (blueprintData.contains("data") && blueprintData["data"].contains("nodes"))
        {
            ImGui::Text("Nodes: %d", (int)blueprintData["data"]["nodes"].size());
        }
        else
        {
            ImGui::Text("Nodes: 0");
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Validate Tree", ImVec2(-1, 0)))
        {
            // Validation logic would go here
            ImGui::OpenPopup("Validation Results");
        }
    }
    
    void BehaviorTreeEditorPlugin::RenderToolbar(json& blueprintData)
    {
        if (ImGui::Button("Add Node"))
        {
            ImGui::OpenPopup("AddNodePopup");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Validate"))
        {
            // Trigger validation
        }
    }
    
    void BehaviorTreeEditorPlugin::RenderBehaviorTreeGraph(json& data, EditorContext& ctx)
    {
        // This would integrate with the existing NodeGraphPanel
        // For now, just show a placeholder
        ImGui::Text("Behavior Tree Graph");
        ImGui::Text("Root Node ID: %d", data.value("rootNodeId", -1));
        
        if (data.contains("nodes") && data["nodes"].is_array())
        {
            ImGui::Separator();
            ImGui::Text("Nodes:");
            
            for (size_t i = 0; i < data["nodes"].size(); ++i)
            {
                const nlohmann::json& node = data["nodes"][i];
                std::string nodeName = node.is_object() && node.contains("name") ? node["name"].get<std::string>() : "Unnamed";
                std::string nodeType = node.is_object() && node.contains("type") ? node["type"].get<std::string>() : "Unknown";
                int nodeId = node.value("id", -1);
                
                ImGui::BulletText("[%d] %s (%s)", nodeId, nodeName.c_str(), nodeType.c_str());
            }
        }
    }
}
