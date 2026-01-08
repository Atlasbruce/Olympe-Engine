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
        ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
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
        bt["editorState"]["scrollOffset"] = {{"x", 0}, {"y", 0}};
        
        // Root node by default
        bt["data"]["rootNodeId"] = 1;
        json rootNode;
        rootNode["id"] = 1;
        rootNode["name"] = "Root Selector";
        rootNode["type"] = "Selector";
        rootNode["position"] = {{"x", 400}, {"y", 300}};
        rootNode["children"] = json::array();
        rootNode["parameters"] = json::object();
        bt["data"]["nodes"] = json::array({rootNode});
        
        return bt;
    }
    
    bool BehaviorTreeEditorPlugin::CanHandle(const json& blueprint) const
    {
        // V2 format check
        if (blueprint.contains("blueprintType") && 
            blueprint["blueprintType"] == "BehaviorTree")
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
            errors.push_back(ValidationError("", "Missing 'data' section", ErrorSeverity::Error));
            return errors;
        }
        
        const json& data = blueprint["data"];
        
        // Check root node exists
        int rootId = data.value("rootNodeId", -1);
        if (rootId == -1)
        {
            errors.push_back(ValidationError("", "Missing rootNodeId", ErrorSeverity::Error));
            return errors;
        }
        
        bool rootFound = false;
        
        if (data.contains("nodes") && data["nodes"].is_array())
        {
            for (auto& node : data["nodes"])
            {
                if (node.value("id", -1) == rootId)
                {
                    rootFound = true;
                    break;
                }
            }
        }
        
        if (!rootFound)
        {
            errors.push_back(ValidationError("", "Root node not found in nodes list", ErrorSeverity::Error));
        }
        
        // Verify all children exist
        if (data.contains("nodes"))
        {
            for (auto& node : data["nodes"])
            {
                if (node.contains("children") && node["children"].is_array())
                {
                    for (auto& childId : node["children"])
                    {
                        int cid = childId.get<int>();
                        bool found = false;
                        
                        for (auto& n : data["nodes"])
                        {
                            if (n.value("id", -1) == cid)
                            {
                                found = true;
                                break;
                            }
                        }
                        
                        if (!found)
                        {
                            std::string nodeIdStr = std::to_string(node.value("id", -1));
                            errors.push_back(ValidationError(
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
            
            for (auto& node : data["nodes"])
            {
                std::string nodeName = node.value("name", "Unnamed");
                std::string nodeType = node.value("type", "Unknown");
                int nodeId = node.value("id", -1);
                
                ImGui::BulletText("[%d] %s (%s)", nodeId, nodeName.c_str(), nodeType.c_str());
            }
        }
    }
}
