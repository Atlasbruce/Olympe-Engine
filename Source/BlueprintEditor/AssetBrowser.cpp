/*
 * Olympe Blueprint Editor - Asset Browser Implementation
 * Frontend component that uses BlueprintEditor backend for asset data
 */

#include "AssetBrowser.h"
#include "BlueprintEditor.h"
#include "EntityInspectorManager.h"
#include "../third_party/imgui/imgui.h"
#include <algorithm>
#include <iostream>

namespace Olympe
{
    AssetBrowser::AssetBrowser()
        : m_TypeFilterSelection(0)
    {
        m_SearchBuffer[0] = '\0';
        m_AvailableTypes = {"All", "EntityBlueprint", "BehaviorTree", "Prefab", "Trigger", "FX", "Sound"};
    }

    AssetBrowser::~AssetBrowser()
    {
    }

    void AssetBrowser::Initialize(const std::string& assetsRootPath)
    {
        // Set the root path in the backend
        BlueprintEditor::Get().SetAssetRootPath(assetsRootPath);
        
        std::cout << "AssetBrowser: Initialized with root path: " << assetsRootPath << std::endl;
    }

    void AssetBrowser::Refresh()
    {
        // Delegate to backend
        BlueprintEditor::Get().RefreshAssets();
        
        std::cout << "AssetBrowser: Refreshed asset tree from backend" << std::endl;
    }

    bool AssetBrowser::PassesFilter(const std::shared_ptr<AssetNode>& node) const
    {
        // Directories always pass
        if (node->isDirectory)
            return true;

        // Apply type filter
        if (m_TypeFilterSelection > 0)
        {
            std::string selectedType = m_AvailableTypes[m_TypeFilterSelection];
            if (node->type != selectedType)
                return false;
        }

        // Apply search filter
        if (m_Filter.searchQuery.empty())
            return true;

        // Case-insensitive search in filename
        std::string lowerName = node->name;
        std::string lowerQuery = m_Filter.searchQuery;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

        return lowerName.find(lowerQuery) != std::string::npos;
    }

    void AssetBrowser::RenderFilterUI()
    {
        // Search box
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::InputText("##search", m_SearchBuffer, sizeof(m_SearchBuffer)))
        {
            m_Filter.searchQuery = m_SearchBuffer;
        }
        
        ImGui::SameLine();
        ImGui::Text("Search");

        // Type filter combo
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::BeginCombo("##typefilter", m_AvailableTypes[m_TypeFilterSelection].c_str()))
        {
            for (int i = 0; i < (int)m_AvailableTypes.size(); i++)
            {
                bool is_selected = (m_TypeFilterSelection == i);
                if (ImGui::Selectable(m_AvailableTypes[i].c_str(), is_selected))
                    m_TypeFilterSelection = i;
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::Text("Type Filter");

        // Refresh button
        ImGui::SameLine(0.0f, 20.0f);
        if (ImGui::Button("Refresh"))
        {
            Refresh();
        }

        ImGui::Separator();
    }

    void AssetBrowser::RenderTreeNode(const std::shared_ptr<AssetNode>& node)
    {
        if (!node)
            return;

        // Skip if doesn't pass filter
        if (!PassesFilter(node))
        {
            // But still check children for directories
            if (node->isDirectory)
            {
                for (const auto& child : node->children)
                    RenderTreeNode(child);
            }
            return;
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        
        if (node->fullPath == m_SelectedAssetPath)
            flags |= ImGuiTreeNodeFlags_Selected;

        if (!node->isDirectory)
            flags |= ImGuiTreeNodeFlags_Leaf;

        std::string label = node->name;
        if (!node->isDirectory && !node->type.empty())
            label += " [" + node->type + "]";

        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)node.get(), flags, "%s", label.c_str());

        // Handle selection (single click)
        if (ImGui::IsItemClicked())
        {
            if (!node->isDirectory)
            {
                m_SelectedAssetPath = node->fullPath;
                BlueprintEditor::Get().SelectAsset(node->fullPath);
                std::cout << "AssetBrowser: Selected asset: " << m_SelectedAssetPath << std::endl;
            }
        }

        // Handle double-click to open
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (!node->isDirectory)
            {
                std::cout << "AssetBrowser: Double-clicked asset: " << node->fullPath << std::endl;
                
                // Check if this is a BehaviorTree or HFSM - open in Node Graph Editor
                if (node->type == "BehaviorTree" || node->type == "HFSM")
                {
                    std::cout << "AssetBrowser: Opening " << node->type << " in Node Graph Editor" << std::endl;
                    BlueprintEditor::Get().OpenGraphInEditor(node->fullPath);
                }
                // Otherwise, use the legacy callback if set (for EntityBlueprint, etc.)
                else if (m_OnAssetOpen)
                {
                    std::cout << "AssetBrowser: Opening asset via callback" << std::endl;
                    m_OnAssetOpen(node->fullPath);
                }
            }
        }
        
        // Tooltip for double-click action
        if (ImGui::IsItemHovered())
        {
            if (node->type == "BehaviorTree" || node->type == "HFSM")
            {
                ImGui::SetTooltip("Double-click to open in Node Graph Editor");
            }
            else if (!node->isDirectory)
            {
                ImGui::SetTooltip("Double-click to open");
            }
        }

        if (node_open)
        {
            if (node->isDirectory)
            {
                for (const auto& child : node->children)
                    RenderTreeNode(child);
            }
            ImGui::TreePop();
        }
    }

    void AssetBrowser::Render()
    {
        if (ImGui::Begin("Asset Browser"))
        {
            // ===== USE TABS TO SEPARATE FILES AND RUNTIME ENTITIES =====
            if (ImGui::BeginTabBar("AssetBrowserTabs"))
            {
                // ===== TAB 1: Blueprint Files =====
                if (ImGui::BeginTabItem("Blueprint Files"))
                {
                    RenderFilterUI();
                    
                    ImGui::Separator();
                    
                    // Get asset tree from backend
                    auto rootNode = BlueprintEditor::Get().GetAssetTree();
                    
                    if (rootNode)
                    {
                        // Render the tree starting from children (skip root "Blueprints" node)
                        for (const auto& child : rootNode->children)
                            RenderTreeNode(child);
                    }
                    else
                    {
                        // Check if backend has an error
                        if (BlueprintEditor::Get().HasError())
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                                "Error: %s", BlueprintEditor::Get().GetLastError().c_str());
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                                "No blueprint files found.");
                        }
                    }
                    
                    ImGui::EndTabItem();
                }
                
                // ===== TAB 2: Runtime Entities =====
                if (ImGui::BeginTabItem("Runtime Entities"))
                {
                    ImGui::Text("Active Entities: %zu", BlueprintEditor::Get().GetRuntimeEntityCount());
                    ImGui::Separator();
                    
                    RenderRuntimeEntities();
                    
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
    
    void AssetBrowser::RenderRuntimeEntities()
    {
        // Get runtime entities from BlueprintEditor backend
        const auto& entities = BlueprintEditor::Get().GetRuntimeEntities();
        
        if (entities.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No runtime entities.");
            ImGui::TextWrapped("Create entities with World::CreateEntity() to see them here.");
            return;
        }
        
        // Use EntityInspectorManager to get entity names and info
        const auto& inspector = Olympe::EntityInspectorManager::Get();
        if (!inspector.IsInitialized())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                "Inspector not initialized.");
            return;
        }
        
        // Get current selection
        uint64_t selectedEntity = BlueprintEditor::Get().GetSelectedEntity();
        
        // Render each entity as a selectable item
        ImGui::BeginChild("RuntimeEntitiesScroll", ImVec2(0, 200), true);
        
        for (uint64_t entityId : entities)
        {
            bool isSelected = (selectedEntity == entityId);
            
            // Get entity info from inspector
            EntityInfo info = inspector.GetEntityInfo(entityId);
            std::string displayName = info.name;
            if (displayName.empty())
            {
                displayName = "Entity_" + std::to_string(entityId);
            }
            
            // Add component count badge
            displayName += " (" + std::to_string(info.componentTypes.size()) + " comp)";
            
            // Selectable item
            if (ImGui::Selectable(displayName.c_str(), isSelected))
            {
                BlueprintEditor::Get().SetSelectedEntity(entityId);
            }
            
            // Tooltip on hover
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Entity ID: %llu", entityId);
                ImGui::Text("Components: %zu", info.componentTypes.size());
                if (!info.componentTypes.empty())
                {
                    ImGui::Separator();
                    for (const auto& compType : info.componentTypes)
                    {
                        ImGui::BulletText("%s", compType.c_str());
                    }
                }
                ImGui::EndTooltip();
            }
        }
        
        ImGui::EndChild();
    }

    std::string AssetBrowser::GetSelectedAssetPath() const
    {
        return m_SelectedAssetPath;
    }

    bool AssetBrowser::HasSelection() const
    {
        return !m_SelectedAssetPath.empty();
    }

    void AssetBrowser::SetAssetOpenCallback(std::function<void(const std::string&)> callback)
    {
        m_OnAssetOpen = callback;
    }
}
