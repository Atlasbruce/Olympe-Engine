/*
 * Olympe Blueprint Editor GUI - Implementation
 */

#include "../include/BlueprintEditorGUI.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include <fstream>
#include <cstring>

using namespace Olympe::Blueprint;

namespace Olympe
{
    BlueprintEditorGUI::BlueprintEditorGUI()
        : m_HasUnsavedChanges(false)
        , m_SelectedComponentIndex(-1)
        , m_NextNodeId(0)
        , m_ShowDemoWindow(false)
        , m_ShowAddComponentDialog(false)
        , m_ShowAboutDialog(false)
        , m_RequestExit(false)
        , m_SelectedComponentType(0)
    {
        m_NewBlueprintNameBuffer[0] = '\0';
        m_FilepathBuffer[0] = '\0';
    }

    BlueprintEditorGUI::~BlueprintEditorGUI()
    {
    }

    void BlueprintEditorGUI::Initialize()
    {
        // Initialize ImNodes
        ImNodes::CreateContext();
        ImNodes::StyleColorsDark();
        
        // Configure ImNodes style
        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLines;
    }

    void BlueprintEditorGUI::Shutdown()
    {
        ImNodes::DestroyContext();
    }

    bool BlueprintEditorGUI::Render()
    {
        // Render menu bar in main viewport
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                    NewBlueprint();
                
                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                {
                    LoadBlueprint("../Blueprints/example_entity_simple.json");
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Save", "Ctrl+S", false, !m_CurrentBlueprint.name.empty()))
                    SaveBlueprint();
                
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, !m_CurrentBlueprint.name.empty()))
                    SaveBlueprintAs();
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Exit", "Ctrl+Q"))
                    m_RequestExit = true;
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Add Component", "Insert", false, !m_CurrentBlueprint.name.empty()))
                    m_ShowAddComponentDialog = true;
                
                if (ImGui::MenuItem("Remove Component", "Delete", false, 
                    m_SelectedComponentIndex >= 0))
                {
                    RemoveComponent(m_SelectedComponentIndex);
                }
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                    m_ShowAboutDialog = true;
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // Render components as separate windows
        RenderEntityPanel();
        RenderNodeEditor();
        RenderPropertyPanel();
        RenderStatusBar();

        // Dialogs
        if (m_ShowAddComponentDialog)
            RenderComponentAddDialog();

        // About dialog
        if (m_ShowAboutDialog)
        {
            ImGui::OpenPopup("About");
            if (ImGui::BeginPopupModal("About", &m_ShowAboutDialog, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Olympe Blueprint Editor - Phase 2");
                ImGui::Separator();
                ImGui::Text("Visual node-based editor for entity blueprints");
                ImGui::Text("Version: 2.0");
                ImGui::Text("Phase: Complete Visual Editor");
                ImGui::Text("Libraries:");
                ImGui::BulletText("ImGui for UI");
                ImGui::BulletText("ImNodes for node editing");
                ImGui::BulletText("SDL2 for window/rendering");
                if (ImGui::Button("Close", ImVec2(120, 0)))
                    m_ShowAboutDialog = false;
                ImGui::EndPopup();
            }
        }

        // Demo window for testing
        if (m_ShowDemoWindow)
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);

        return !m_RequestExit;
    }

    // Menu bar is now integrated in the main Render() function

    void BlueprintEditorGUI::RenderEntityPanel()
    {
        ImGui::Begin("Entity Properties");

        if (!m_CurrentBlueprint.name.empty())
        {
            ImGui::Text("Blueprint: %s", m_CurrentBlueprint.name.c_str());
            if (m_HasUnsavedChanges)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "*");
            }
            
            ImGui::Separator();

            // Description
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", m_CurrentBlueprint.description.c_str());
            
            ImGui::Separator();

            // Component list
            ImGui::Text("Components (%zu)", m_CurrentBlueprint.components.size());
            
            for (size_t i = 0; i < m_CurrentBlueprint.components.size(); ++i)
            {
                const auto& comp = m_CurrentBlueprint.components[i];
                bool selected = (m_SelectedComponentIndex == (int)i);
                
                if (ImGui::Selectable(comp.type.c_str(), selected))
                {
                    m_SelectedComponentIndex = (int)i;
                }
                
                // Right-click context menu
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    ImGui::OpenPopup("component_context");
                    m_SelectedComponentIndex = (int)i;
                }
            }

            // Context menu
            if (ImGui::BeginPopup("component_context"))
            {
                if (ImGui::MenuItem("Remove"))
                {
                    RemoveComponent(m_SelectedComponentIndex);
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            if (ImGui::Button("Add Component", ImVec2(-1, 0)))
            {
                m_ShowAddComponentDialog = true;
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No blueprint loaded");
            ImGui::Text("");
            ImGui::Text("Use File > New or File > Open");
            ImGui::Text("to get started");
        }

        ImGui::End();
    }

    void BlueprintEditorGUI::RenderNodeEditor()
    {
        ImGui::Begin("Component Graph");

        if (!m_CurrentBlueprint.name.empty())
        {
            ImNodes::BeginNodeEditor();

            // Render each component as a node
            for (size_t i = 0; i < m_CurrentBlueprint.components.size(); ++i)
            {
                const auto& comp = m_CurrentBlueprint.components[i];
                int node_id = (int)i;
                
                ImNodes::BeginNode(node_id);
                
                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(comp.type.c_str());
                ImNodes::EndNodeTitleBar();
                
                // Node content - show key properties
                ImGui::PushItemWidth(120.0f);
                
                // Display type only (minimal JSON doesn't support iteration)
                ImGui::TextDisabled("Type: %s", comp.type.c_str());
                ImGui::TextDisabled("Click to view properties");
                
                ImGui::PopItemWidth();
                
                ImNodes::EndNode();
                
                // Store or initialize node position
                if (m_NodePositions.find(node_id) == m_NodePositions.end())
                {
                    // Initial positioning in a grid
                    float x = 100.0f + (i % 3) * 250.0f;
                    float y = 100.0f + (i / 3) * 150.0f;
                    m_NodePositions[node_id] = ImVec2(x, y);
                    ImNodes::SetNodeGridSpacePos(node_id, m_NodePositions[node_id]);
                }
            }

            // Handle node selection
            int num_selected = ImNodes::NumSelectedNodes();
            if (num_selected > 0)
            {
                int* selected_nodes = new int[num_selected];
                ImNodes::GetSelectedNodes(selected_nodes);
                m_SelectedComponentIndex = selected_nodes[0];
                delete[] selected_nodes;
            }

            ImNodes::EndNodeEditor();
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No blueprint loaded");
            ImGui::Text("");
            ImGui::Text("Components will appear as nodes here");
        }

        ImGui::End();
    }

    void BlueprintEditorGUI::RenderPropertyPanel()
    {
        ImGui::Begin("Properties");

        if (m_SelectedComponentIndex >= 0 && 
            m_SelectedComponentIndex < (int)m_CurrentBlueprint.components.size())
        {
            auto& comp = m_CurrentBlueprint.components[m_SelectedComponentIndex];
            
            ImGui::Text("Component: %s", comp.type.c_str());
            ImGui::Separator();

            // Display properties as JSON (minimal JSON doesn't support iteration)
            ImGui::Text("Properties:");
            std::string props_json = comp.properties.dump(2);
            ImGui::TextWrapped("%s", props_json.c_str());
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), 
                "Note: Use console editor (Phase 1) for property editing");
            ImGui::Text("Phase 2 property editing requires full JSON library");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                "Select a component to view properties");
        }

        ImGui::End();
    }

    void BlueprintEditorGUI::RenderStatusBar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
        ImGui::Begin("##StatusBar", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
        
        if (!m_CurrentBlueprint.name.empty())
        {
            ImGui::Text("Blueprint: %s", m_CurrentBlueprint.name.c_str());
            ImGui::SameLine();
            ImGui::Text(" | Components: %zu", m_CurrentBlueprint.components.size());
            
            if (m_HasUnsavedChanges)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "| Modified");
            }
            
            if (!m_CurrentFilepath.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("| %s", m_CurrentFilepath.c_str());
            }
        }
        else
        {
            ImGui::Text("Ready | No blueprint loaded");
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void BlueprintEditorGUI::RenderComponentAddDialog()
    {
        ImGui::OpenPopup("Add Component");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Add Component", &m_ShowAddComponentDialog, 
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Select component type:");
            ImGui::Separator();
            
            const char* component_types[] = {
                "Position", "BoundingBox", "VisualSprite", "Movement", 
                "PhysicsBody", "Health", "AIBehavior", "TriggerZone",
                "Animation", "AudioSource", "Inventory", "PlayerController"
            };
            
            ImGui::ListBox("##component_types", &m_SelectedComponentType, 
                component_types, IM_ARRAYSIZE(component_types), 8);
            
            ImGui::Separator();
            
            if (ImGui::Button("Add", ImVec2(120, 0)))
            {
                AddComponent(component_types[m_SelectedComponentType]);
                m_ShowAddComponentDialog = false;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_ShowAddComponentDialog = false;
            }
            
            ImGui::EndPopup();
        }
    }

    void BlueprintEditorGUI::NewBlueprint()
    {
        // Simple new blueprint creation
        m_CurrentBlueprint = EntityBlueprint("NewBlueprint");
        m_CurrentBlueprint.description = "A new entity blueprint";
        m_CurrentFilepath.clear();
        m_HasUnsavedChanges = true;
        m_SelectedComponentIndex = -1;
        m_NodePositions.clear();
    }

    void BlueprintEditorGUI::LoadBlueprint(const std::string& filepath)
    {
        auto loaded = EntityBlueprint::LoadFromFile(filepath);
        if (!loaded.name.empty())
        {
            m_CurrentBlueprint = loaded;
            m_CurrentFilepath = filepath;
            m_HasUnsavedChanges = false;
            m_SelectedComponentIndex = -1;
            m_NodePositions.clear();
        }
    }

    void BlueprintEditorGUI::SaveBlueprint()
    {
        if (m_CurrentFilepath.empty())
        {
            // Default save location
            m_CurrentFilepath = "../Blueprints/" + m_CurrentBlueprint.name + ".json";
        }

        if (m_CurrentBlueprint.SaveToFile(m_CurrentFilepath))
        {
            m_HasUnsavedChanges = false;
        }
    }

    void BlueprintEditorGUI::SaveBlueprintAs()
    {
        // For now, use a default pattern
        m_CurrentFilepath = "../Blueprints/" + m_CurrentBlueprint.name + "_copy.json";
        SaveBlueprint();
    }

    void BlueprintEditorGUI::AddComponent(const std::string& type)
    {
        ComponentData newComp;
        newComp.type = type;
        
        // Create default properties based on type
        if (type == "Position")
        {
            newComp = CreatePositionComponent(0, 0);
        }
        else if (type == "BoundingBox")
        {
            newComp = CreateBoundingBoxComponent(0, 0, 32, 32);
        }
        else if (type == "VisualSprite")
        {
            newComp = CreateVisualSpriteComponent("Resources/sprite.png", 0, 0, 32, 32);
        }
        else if (type == "Movement")
        {
            newComp = CreateMovementComponent(1, 0, 0, 0);
        }
        else if (type == "PhysicsBody")
        {
            newComp = CreatePhysicsBodyComponent(1.0f, 100.0f);
        }
        else if (type == "Health")
        {
            newComp = CreateHealthComponent(100, 100);
        }
        else if (type == "AIBehavior")
        {
            newComp = CreateAIBehaviorComponent("idle");
        }
        else
        {
            // Generic component
            newComp.type = type;
            newComp.properties = json::object();
        }
        
        m_CurrentBlueprint.AddComponent(newComp.type, newComp.properties);
        m_HasUnsavedChanges = true;
    }

    void BlueprintEditorGUI::RemoveComponent(int index)
    {
        if (index >= 0 && index < (int)m_CurrentBlueprint.components.size())
        {
            m_CurrentBlueprint.components.erase(
                m_CurrentBlueprint.components.begin() + index
            );
            m_SelectedComponentIndex = -1;
            m_HasUnsavedChanges = true;
        }
    }
}
