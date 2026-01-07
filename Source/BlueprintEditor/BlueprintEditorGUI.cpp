/*
 * Olympe Blueprint Editor GUI - Frontend Implementation
 * All data operations delegate to BlueprintEditor backend
 */

#include "BlueprintEditorGUI.h"
#include "BlueprintEditor.h"
#include "TemplateBrowserPanel.h"
#include "HistoryPanel.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include <iostream>

using namespace Olympe::Blueprint;

namespace Olympe
{
    BlueprintEditorGUI::BlueprintEditorGUI()
        : m_SelectedComponentIndex(-1)
        , m_NextNodeId(0)
        , m_ShowDemoWindow(false)
        , m_ShowAddComponentDialog(false)
        , m_ShowAboutDialog(false)
        , m_SelectedComponentType(0)
        , m_ShowAssetBrowser(true)
        , m_ShowAssetInfo(true)
        , m_ShowInspector(true)
        , m_ShowNodeGraph(true)
        , m_ShowEntities(true)
        , m_ShowEntityProperties(true)
        , m_ShowComponentGraph(true)
        , m_ShowPropertyPanel(true)
        , m_ShowTemplateBrowser(false)
        , m_ShowHistory(false)
        , m_ShowPreferences(false)
        , m_ShowShortcuts(false)
        , m_TemplateBrowserPanel(nullptr)
        , m_HistoryPanel(nullptr)
    {
        m_NewBlueprintNameBuffer[0] = '\0';
        m_FilepathBuffer[0] = '\0';
    }

    BlueprintEditorGUI::~BlueprintEditorGUI()
    {
        if (m_HistoryPanel)
        {
            delete m_HistoryPanel;
            m_HistoryPanel = nullptr;
        }
        
        if (m_TemplateBrowserPanel)
        {
            delete m_TemplateBrowserPanel;
            m_TemplateBrowserPanel = nullptr;
        }
    }

    void BlueprintEditorGUI::Initialize()
    {
        // Initialize ImNodes
        ImNodes::CreateContext();
        ImNodes::StyleColorsDark();
        
        // Configure ImNodes style
        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLines;
        
        // Initialize Asset Browser with Blueprints directory
        m_AssetBrowser.Initialize("Blueprints");
        
        // Set up callback for asset selection
        m_AssetBrowser.SetAssetOpenCallback([this](const std::string& path) {
            LoadBlueprint(path);
        });
        
        // Initialize new panels
        m_NodeGraphPanel.Initialize();
        m_EntitiesPanel.Initialize();
        m_InspectorPanel.Initialize();
        
        // Initialize template browser panel
        m_TemplateBrowserPanel = new TemplateBrowserPanel();
        m_TemplateBrowserPanel->Initialize();
        
        // Initialize history panel
        m_HistoryPanel = new HistoryPanel();
        m_HistoryPanel->Initialize();
    }

    void BlueprintEditorGUI::Shutdown()
    {
        // Shutdown panels
        if (m_HistoryPanel)
        {
            m_HistoryPanel->Shutdown();
            delete m_HistoryPanel;
            m_HistoryPanel = nullptr;
        }
        
        if (m_TemplateBrowserPanel)
        {
            m_TemplateBrowserPanel->Shutdown();
            delete m_TemplateBrowserPanel;
            m_TemplateBrowserPanel = nullptr;
        }
        
        m_InspectorPanel.Shutdown();
        m_EntitiesPanel.Shutdown();
        m_NodeGraphPanel.Shutdown();
        
        ImNodes::DestroyContext();
    }

    void BlueprintEditorGUI::Render()
    {
        // Only render if the backend is active
        if (!BlueprintEditor::Get().IsActive())
        {
            return;
        }
        
        // Handle keyboard shortcuts
        HandleKeyboardShortcuts();

        // Get backend reference for data access
        auto& backend = BlueprintEditor::Get();
        const auto& blueprint = backend.GetCurrentBlueprint();
        
        // Render menu bar in main viewport
        if (ImGui::BeginMainMenuBar())
        {
            // ===== D) FILE MENU =====
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Blueprint", "Ctrl+N"))
                    NewBlueprint();
                
                if (ImGui::MenuItem("Open Blueprint...", "Ctrl+O"))
                {
                    // TODO: File dialog - for now use example
                    LoadBlueprint("Blueprints/AI/guard_patrol.json");
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Save", "Ctrl+S", false, backend.HasBlueprint()))
                    SaveBlueprint();
                
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, backend.HasBlueprint()))
                    SaveBlueprintAs();
                
                ImGui::Separator();
                
                // Phase 5: Template menu items
                if (ImGui::MenuItem("Save as Template...", "Ctrl+Shift+T", false, backend.HasBlueprint()))
                {
                    if (m_TemplateBrowserPanel)
                    {
                        m_ShowTemplateBrowser = true;
                        // Trigger the modal by accessing the panel
                    }
                }
                
                if (ImGui::MenuItem("Template Browser", nullptr, &m_ShowTemplateBrowser))
                {
                    // Toggle template browser visibility
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Reload Assets"))
                {
                    backend.RefreshAssets();
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Exit Editor", "F2"))
                    backend.SetActive(false);
                
                ImGui::EndMenu();
            }

            // ===== D) EDIT MENU =====
            if (ImGui::BeginMenu("Edit"))
            {
                // Phase 6: Undo/Redo
                bool canUndo = backend.CanUndo();
                bool canRedo = backend.CanRedo();
                
                std::string undoLabel = "Undo";
                if (canUndo)
                {
                    undoLabel += ": " + backend.GetLastCommandDescription();
                }
                
                if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUndo))
                {
                    backend.Undo();
                }
                
                std::string redoLabel = "Redo";
                if (canRedo)
                {
                    redoLabel += ": " + backend.GetNextRedoDescription();
                }
                
                if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, canRedo))
                {
                    backend.Redo();
                }
                
                ImGui::Separator();
                
                // Component operations
                if (ImGui::MenuItem("Add Component", "Insert", false, backend.HasBlueprint()))
                    m_ShowAddComponentDialog = true;
                
                if (ImGui::MenuItem("Remove Component", "Delete", false, 
                    m_SelectedComponentIndex >= 0))
                {
                    RemoveComponent(m_SelectedComponentIndex);
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Preferences..."))
                {
                    m_ShowPreferences = true;
                }
                
                ImGui::EndMenu();
            }

            // ===== D) VIEW MENU =====
            if (ImGui::BeginMenu("View"))
            {
                ImGui::Text("Panels:");
                ImGui::Separator();
                
                ImGui::MenuItem("Asset Browser", nullptr, &m_ShowAssetBrowser);
                ImGui::MenuItem("Asset Info", nullptr, &m_ShowAssetInfo);
                ImGui::MenuItem("Runtime Entities", nullptr, &m_ShowEntities);
                ImGui::MenuItem("Inspector", nullptr, &m_ShowInspector);
                ImGui::MenuItem("Node Graph", nullptr, &m_ShowNodeGraph);
                ImGui::MenuItem("Entity Properties", nullptr, &m_ShowEntityProperties);
                ImGui::MenuItem("Component Graph", nullptr, &m_ShowComponentGraph);
                ImGui::MenuItem("Property Panel", nullptr, &m_ShowPropertyPanel);
                ImGui::MenuItem("Template Browser", nullptr, &m_ShowTemplateBrowser);  // Phase 5
                ImGui::MenuItem("History", nullptr, &m_ShowHistory);  // Phase 6
                
                ImGui::Separator();
                
                ImGui::Text("Debug:");
                ImGui::Separator();
                ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Reset Layout"))
                {
                    // Reset all panels to visible
                    m_ShowAssetBrowser = true;
                    m_ShowAssetInfo = true;
                    m_ShowInspector = true;
                    m_ShowNodeGraph = true;
                    m_ShowEntities = true;
                    m_ShowEntityProperties = true;
                    m_ShowComponentGraph = true;
                    m_ShowPropertyPanel = true;
                }
                
                ImGui::EndMenu();
            }

            // ===== D) HELP MENU =====
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Documentation"))
                {
                    // TODO: Open documentation URL or local help
                    std::cout << "Opening documentation..." << std::endl;
                }
                
                if (ImGui::MenuItem("Keyboard Shortcuts"))
                {
                    m_ShowShortcuts = true;
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("About Olympe Engine"))
                    m_ShowAboutDialog = true;
                
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // D) Render panels conditionally based on visibility flags
        if (m_ShowAssetBrowser)
            m_AssetBrowser.Render();
        
        // Update asset info panel when selection changes
        if (m_ShowAssetInfo)
        {
            if (m_AssetBrowser.HasSelection())
            {
                std::string selectedPath = m_AssetBrowser.GetSelectedAssetPath();
                // Only reload if selection changed
                if (!m_AssetInfoPanel.HasAsset() || m_AssetInfoPanel.GetLoadedFilepath() != selectedPath)
                {
                    m_AssetInfoPanel.LoadAsset(selectedPath);
                }
            }
            m_AssetInfoPanel.Render();
        }
        
        // === Phase 3: Node Graph Editor ===
        if (m_ShowNodeGraph)
            m_NodeGraphPanel.Render();
        
        // === Phase 4: Runtime Entities and Inspector ===
        if (m_ShowEntities)
            m_EntitiesPanel.Render();
            
        if (m_ShowInspector)
            m_InspectorPanel.Render();
        
        // === Phase 5: Template Browser ===
        if (m_ShowTemplateBrowser && m_TemplateBrowserPanel)
            m_TemplateBrowserPanel->Render();
        
        // === Phase 6: History Panel ===
        if (m_ShowHistory && m_HistoryPanel)
            m_HistoryPanel->Render();

        // Render components as separate windows
        if (m_ShowEntityProperties)
            RenderEntityPanel();
            
        if (m_ShowComponentGraph)
            RenderNodeEditor();
            
        if (m_ShowPropertyPanel)
            RenderPropertyPanel();
            
        RenderStatusBar();

        // Dialogs
        if (m_ShowAddComponentDialog)
            RenderComponentAddDialog();
            
        if (m_ShowPreferences)
            RenderPreferencesDialog();
            
        if (m_ShowShortcuts)
            RenderShortcutsDialog();

        // About dialog
        if (m_ShowAboutDialog)
        {
            ImGui::OpenPopup("About");
            if (ImGui::BeginPopupModal("About", &m_ShowAboutDialog, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Olympe Blueprint Editor");
                ImGui::Separator();
                ImGui::Text("Visual node-based editor for entity blueprints");
                ImGui::Text("Version: 2.0");
                ImGui::Text("Status: Interactive panels, entity synchronization, full menus");
                ImGui::Separator();
                ImGui::Text("Libraries:");
                ImGui::BulletText("ImGui for UI");
                ImGui::BulletText("ImNodes for node editing");
                ImGui::BulletText("SDL3 for window/rendering");
                ImGui::Separator();
                ImGui::Text("Features:");
                ImGui::BulletText("Interactive, dockable panels");
                ImGui::BulletText("Runtime entity tracking");
                ImGui::BulletText("Synchronized panel selection");
                ImGui::BulletText("Full menu system");
                if (ImGui::Button("Close", ImVec2(120, 0)))
                    m_ShowAboutDialog = false;
                ImGui::EndPopup();
            }
        }

        // Demo window for testing
        if (m_ShowDemoWindow)
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);
    }

    // Menu bar is now integrated in the main Render() function

    void BlueprintEditorGUI::RenderMenuBar()
    {
    }

    void BlueprintEditorGUI::RenderEntityPanel()
    {
        // Get backend reference
        auto& backend = BlueprintEditor::Get();
        const auto& blueprint = backend.GetCurrentBlueprint();
        
        ImGui::Begin("Entity Properties");

        if (backend.HasBlueprint())
        {
            ImGui::Text("Blueprint: %s", blueprint.name.c_str());
            if (backend.HasUnsavedChanges())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "*");
            }
            
            ImGui::Separator();

            // Description
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", blueprint.description.c_str());
            
            ImGui::Separator();

            // Component list
            ImGui::Text("Components (%zu)", blueprint.components.size());
            
            for (size_t i = 0; i < blueprint.components.size(); ++i)
            {
                const auto& comp = blueprint.components[i];
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
        // Get backend reference
        auto& backend = BlueprintEditor::Get();
        const auto& blueprint = backend.GetCurrentBlueprint();
        
        ImGui::Begin("Component Graph");

        if (backend.HasBlueprint())
        {
            ImNodes::BeginNodeEditor();

            // Render each component as a node
            for (size_t i = 0; i < blueprint.components.size(); ++i)
            {
                const auto& comp = blueprint.components[i];
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

            ImNodes::EndNodeEditor();

            // Handle node selection
            int num_selected = ImNodes::NumSelectedNodes();
            if (num_selected > 0)
            {
                int* selected_nodes = new int[num_selected];
                ImNodes::GetSelectedNodes(selected_nodes);
                m_SelectedComponentIndex = selected_nodes[0];
                delete[] selected_nodes;
            }
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
        // Get backend reference
        auto& backend = BlueprintEditor::Get();
        const auto& blueprint = backend.GetCurrentBlueprint();
        
        ImGui::Begin("Properties");

        if (m_SelectedComponentIndex >= 0 && 
            m_SelectedComponentIndex < (int)blueprint.components.size())
        {
            const auto& comp = blueprint.components[m_SelectedComponentIndex];
            
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
        // Get backend reference
        auto& backend = BlueprintEditor::Get();
        const auto& blueprint = backend.GetCurrentBlueprint();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
        ImGui::Begin("##StatusBar", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
        
        if (backend.HasBlueprint())
        {
            ImGui::Text("Blueprint: %s", blueprint.name.c_str());
            ImGui::SameLine();
            ImGui::Text(" | Components: %zu", blueprint.components.size());
            
            if (backend.HasUnsavedChanges())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "| Modified");
            }
            
            const std::string& filepath = backend.GetCurrentFilepath();
            if (!filepath.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("| %s", filepath.c_str());
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
        // Delegate to backend
        BlueprintEditor::Get().NewBlueprint("NewBlueprint", "A new entity blueprint");
        
        // Reset UI state
        m_SelectedComponentIndex = -1;
        m_NodePositions.clear();
    }

    void BlueprintEditorGUI::LoadBlueprint(const std::string& filepath)
    {
        // Delegate to backend
        if (BlueprintEditor::Get().LoadBlueprint(filepath))
        {
            // Reset UI state on successful load
            m_SelectedComponentIndex = -1;
            m_NodePositions.clear();
        }
    }

    void BlueprintEditorGUI::SaveBlueprint()
    {
        auto& backend = BlueprintEditor::Get();
        
        if (backend.GetCurrentFilepath().empty())
        {
            // Default save location if no filepath set
            const std::string& name = backend.GetCurrentBlueprint().name;
            std::string filepath = "../Blueprints/" + name + ".json";
            backend.SaveBlueprintAs(filepath);
        }
        else
        {
            backend.SaveBlueprint();
        }
    }

    void BlueprintEditorGUI::SaveBlueprintAs()
    {
        auto& backend = BlueprintEditor::Get();
        
        // For now, use a default pattern (in real implementation, would show file dialog)
        const std::string& name = backend.GetCurrentBlueprint().name;
        std::string filepath = "../Blueprints/" + name + "_copy.json";
        backend.SaveBlueprintAs(filepath);
    }

    void BlueprintEditorGUI::AddComponent(const std::string& type)
    {
        // Get mutable blueprint from backend
        auto& blueprint = BlueprintEditor::Get().GetCurrentBlueprintMutable();
        
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
        
        // Add to backend blueprint
        blueprint.AddComponent(newComp.type, newComp.properties);
        
        // Mark as modified in backend
        BlueprintEditor::Get().MarkAsModified();
    }

    void BlueprintEditorGUI::RemoveComponent(int index)
    {
        // Get mutable blueprint from backend
        auto& blueprint = BlueprintEditor::Get().GetCurrentBlueprintMutable();
        
        if (index >= 0 && index < (int)blueprint.components.size())
        {
            blueprint.components.erase(
                blueprint.components.begin() + index
            );
            m_SelectedComponentIndex = -1;
            
            // Mark as modified in backend
            BlueprintEditor::Get().MarkAsModified();
        }
    }
    
    // D) Additional dialog implementations
    void BlueprintEditorGUI::RenderPreferencesDialog()
    {
        ImGui::OpenPopup("Preferences");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Appearing);
        
        if (ImGui::BeginPopupModal("Preferences", &m_ShowPreferences))
        {
            ImGui::Text("Editor Preferences");
            ImGui::Separator();
            
            ImGui::TextWrapped("Preferences coming soon...");
            ImGui::Spacing();
            
            ImGui::Text("Planned settings:");
            ImGui::BulletText("Auto-save interval");
            ImGui::BulletText("Theme selection");
            ImGui::BulletText("Grid snap settings");
            ImGui::BulletText("Default component properties");
            
            ImGui::Separator();
            
            if (ImGui::Button("Close", ImVec2(120, 0)))
            {
                m_ShowPreferences = false;
            }
            
            ImGui::EndPopup();
        }
    }
    
    void BlueprintEditorGUI::RenderShortcutsDialog()
    {
        ImGui::OpenPopup("Keyboard Shortcuts");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_Appearing);
        
        if (ImGui::BeginPopupModal("Keyboard Shortcuts", &m_ShowShortcuts))
        {
            ImGui::Text("Keyboard Shortcuts");
            ImGui::Separator();
            
            ImGui::Columns(2, "shortcuts");
            ImGui::SetColumnWidth(0, 200);
            
            ImGui::Text("Editor Control:");
            ImGui::Separator();
            ImGui::Text("F2"); ImGui::NextColumn(); ImGui::Text("Toggle Blueprint Editor"); ImGui::NextColumn();
            ImGui::Text("Escape"); ImGui::NextColumn(); ImGui::Text("Exit Application"); ImGui::NextColumn();
            
            ImGui::Spacing();
            ImGui::Text("File Operations:");
            ImGui::Separator();
            ImGui::Text("Ctrl+N"); ImGui::NextColumn(); ImGui::Text("New Blueprint"); ImGui::NextColumn();
            ImGui::Text("Ctrl+O"); ImGui::NextColumn(); ImGui::Text("Open Blueprint"); ImGui::NextColumn();
            ImGui::Text("Ctrl+S"); ImGui::NextColumn(); ImGui::Text("Save"); ImGui::NextColumn();
            ImGui::Text("Ctrl+Shift+S"); ImGui::NextColumn(); ImGui::Text("Save As"); ImGui::NextColumn();
            
            ImGui::Spacing();
            ImGui::Text("Edit Operations:");
            ImGui::Separator();
            ImGui::Text("Ctrl+Z"); ImGui::NextColumn(); ImGui::Text("Undo"); ImGui::NextColumn();
            ImGui::Text("Ctrl+Y"); ImGui::NextColumn(); ImGui::Text("Redo"); ImGui::NextColumn();
            ImGui::Text("Insert"); ImGui::NextColumn(); ImGui::Text("Add Component"); ImGui::NextColumn();
            ImGui::Text("Delete"); ImGui::NextColumn(); ImGui::Text("Remove Component"); ImGui::NextColumn();
            
            ImGui::Columns(1);
            ImGui::Separator();
            
            if (ImGui::Button("Close", ImVec2(120, 0)))
            {
                m_ShowShortcuts = false;
            }
            
            ImGui::EndPopup();
        }
    }
    
    // Phase 6: Keyboard shortcuts handler
    void BlueprintEditorGUI::HandleKeyboardShortcuts()
    {
        ImGuiIO& io = ImGui::GetIO();
        auto& backend = BlueprintEditor::Get();
        
        // Don't process shortcuts if typing in a text field
        if (io.WantTextInput)
        {
            return;
        }
        
        // Ctrl+Z : Undo
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift)
        {
            if (backend.CanUndo())
            {
                backend.Undo();
            }
        }
        
        // Ctrl+Y or Ctrl+Shift+Z : Redo
        if ((io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) ||
            (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))
        {
            if (backend.CanRedo())
            {
                backend.Redo();
            }
        }
        
        // Ctrl+S : Save
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) && !io.KeyShift)
        {
            if (backend.HasBlueprint())
            {
                SaveBlueprint();
            }
        }
        
        // Ctrl+Shift+S : Save As
        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            if (backend.HasBlueprint())
            {
                SaveBlueprintAs();
            }
        }
        
        // Ctrl+N : New Blueprint
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
        {
            NewBlueprint();
        }
        
        // Ctrl+O : Open Blueprint
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
        {
            // TODO: File dialog
            LoadBlueprint("../Blueprints/example_entity_simple.json");
        }
        
        // Ctrl+Shift+T : Save as Template
        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_T))
        {
            if (backend.HasBlueprint())
            {
                m_ShowTemplateBrowser = true;
            }
        }
    }
}
