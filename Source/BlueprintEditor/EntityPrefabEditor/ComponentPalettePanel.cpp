#include "ComponentPalettePanel.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../../system/system_utils.h"
#include "../../third_party/nlohmann/json.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <io.h>
#include <direct.h>

namespace Olympe
{
    ComponentPalettePanel::ComponentPalettePanel()
    {
        memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
        memset(m_draggedComponentBuffer, 0, sizeof(m_draggedComponentBuffer));
    }

    ComponentPalettePanel::~ComponentPalettePanel() { }

    void ComponentPalettePanel::Initialize()
    {
        SYSTEM_LOG << "[ComponentPalettePanel] Initializing...\n";

        // Log current working directory for debugging (Windows only)
        char cwd[_MAX_PATH];
        if (_getcwd(cwd, sizeof(cwd)) != nullptr)
        {
            SYSTEM_LOG << "[ComponentPalettePanel] Current working directory: " << cwd << "\n";
        }

        // Try to load from JSON file first (new format: Gamedata/EntityPrefab/ComponentsParameters.json)
        bool jsonLoaded = LoadComponentsFromJSON("Gamedata\\EntityPrefab\\ComponentsParameters.json");

        // If JSON load failed, try forward slash variant
        if (!jsonLoaded)
        {
            SYSTEM_LOG << "[ComponentPalettePanel] Attempting with forward slashes...\n";
            jsonLoaded = LoadComponentsFromJSON("Gamedata/EntityPrefab/ComponentsParameters.json");
        }

        // If JSON load failed, try absolute path as fallback
        if (!jsonLoaded)
        {
            char cwdBuffer[_MAX_PATH];
            if (_getcwd(cwdBuffer, sizeof(cwdBuffer)) != nullptr)
            {
                std::string absolutePath = std::string(cwdBuffer) + "\\Gamedata\\EntityPrefab\\ComponentsParameters.json";
                SYSTEM_LOG << "[ComponentPalettePanel] Attempting absolute path: " << absolutePath << "\n";
                jsonLoaded = LoadComponentsFromJSON(absolutePath);
            }
        }

        // If JSON load failed, try legacy location for backward compatibility
        if (!jsonLoaded)
        {
            SYSTEM_LOG << "[ComponentPalettePanel] Attempting to load from legacy location...\n";
            jsonLoaded = LoadComponentsFromJSON("Gamedata/PrefabEntities/ComponentsParameters.json");
        }

        // If JSON load failed, use hardcoded components as fallback
        if (!jsonLoaded)
        {
            SYSTEM_LOG << "[ComponentPalettePanel] Using hardcoded component types as fallback\n";
            RegisterComponentType("Transform", "Core", "Position, rotation, scale");
            RegisterComponentType("Identity", "Core", "Entity identity and naming");
            RegisterComponentType("Movement", "Physics", "Movement and velocity");
            RegisterComponentType("Sprite", "Graphics", "Sprite rendering");
            RegisterComponentType("Collision", "Physics", "Collision bounds");
            RegisterComponentType("Health", "Gameplay", "Health points system");
            RegisterComponentType("AIBlackboard", "AI", "AI data storage");
            RegisterComponentType("BehaviorTree", "AI", "Behavior tree execution");
            RegisterComponentType("VisualSprite", "Graphics", "Visual sprite data");
            RegisterComponentType("AnimationController", "Graphics", "Animation state machine");

            // Build categories list
            std::map<std::string, bool> categoryMap;
            for (size_t i = 0; i < m_componentTypes.size(); ++i)
            {
                categoryMap[m_componentTypes[i].category] = true;
            }
            for (auto it = categoryMap.begin(); it != categoryMap.end(); ++it)
            {
                m_categories.push_back(it->first);
            }

            // Sort categories alphabetically
            std::sort(m_categories.begin(), m_categories.end());
        }

        // Initialize category expanded state (all categories start expanded)
        m_categoryExpanded.resize(m_categories.size(), true);

        SYSTEM_LOG << "[ComponentPalettePanel] Initialization complete with " << m_componentTypes.size() << " component types\n";
    }

    void ComponentPalettePanel::Render(EntityPrefabGraphDocument* document)
    {
        if (!document) { return; }

        ImGui::BeginChild("##ComponentPalette", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

        // ========== Dense UI Header (Blue background like VisualScriptEditor) ==========
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Selectable("Components", true, ImGuiSelectableFlags_None, ImVec2(0.f, 20.f));
        ImGui::PopStyleColor(4);

        // ========== Search bar (compact) ==========
        ImGui::PushItemWidth(-1.0f);
        ImGui::InputText("##ComponentSearch", m_searchBuffer, sizeof(m_searchBuffer), ImGuiInputTextFlags_CharsNoBlank);
        m_searchFilter = m_searchBuffer;
        ImGui::PopItemWidth();

        // ========== Category tabs (collapsible tree) ==========
        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
        RenderCategoryTabs();
        ImGui::PopStyleVar();

        // ========== Help text at bottom ==========
        ImGui::Spacing();
        ImGui::TextDisabled("Tip: Drag & drop components onto the graph to add them");

        ImGui::EndChild();
    }

    void ComponentPalettePanel::RenderSearchBar()
    {
        ImGui::TextUnformatted("Search:");
        ImGui::InputText("##search", m_searchBuffer, sizeof(m_searchBuffer));
    }

    void ComponentPalettePanel::RenderCategoryTabs()
    {
        // "All" category (non-collapsible, shows all components)
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.2f, 0.35f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.4f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.15f, 0.3f, 0.55f, 1.0f));

        bool allSelected = (m_selectedCategoryIndex == -1);
        if (ImGui::Selectable("All", allSelected, ImGuiSelectableFlags_None, ImVec2(0.f, CATEGORY_HEADER_HEIGHT)))
        {
            m_selectedCategoryIndex = -1;
        }
        ImGui::PopStyleColor(3);

        // Ensure m_categoryExpanded is initialized
        if (m_categoryExpanded.size() != m_categories.size())
        {
            m_categoryExpanded.resize(m_categories.size(), true);  // All categories start expanded
        }

        // Individual categories as collapsible trees
        for (size_t i = 0; i < m_categories.size(); ++i)
        {
            ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.2f, 0.35f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.4f, 0.7f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.15f, 0.3f, 0.55f, 1.0f));

            // Category header with TreeNodeEx for toggle
            bool categoryExpanded = ImGui::TreeNodeEx(
                m_categories[i].c_str(),
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding,
                m_categories[i].c_str()
            );

            ImGui::PopStyleColor(3);

            if (categoryExpanded)
            {
                m_categoryExpanded[i] = true;

                // Render components in this category
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));

                for (size_t j = 0; j < m_componentTypes.size(); ++j)
                {
                    const ComponentType& component = m_componentTypes[j];

                    // Filter by category
                    if (component.category != m_categories[i])
                    {
                        continue;
                    }

                    // Filter by search
                    if (!m_searchFilter.empty())
                    {
                        bool matches = false;
                        std::string nameLower = component.name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::string searchLower = m_searchFilter;
                        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
                        if (nameLower.find(searchLower) != std::string::npos)
                        {
                            matches = true;
                        }
                        if (!matches)
                        {
                            continue;
                        }
                    }

                    // Render compact component item
                    std::string label = component.name + "##" + std::to_string(i) + "_" + std::to_string(j);

                    // Use InvisibleButton for drag-drop support
                    ImGui::InvisibleButton(label.c_str(), ImVec2(-1.0f, COMPONENT_ITEM_HEIGHT));

                    // Draw background
                    ImVec2 itemMin = ImGui::GetItemRectMin();
                    ImVec2 itemMax = ImGui::GetItemRectMax();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();

                    if (ImGui::IsItemHovered())
                    {
                        drawList->AddRectFilled(itemMin, itemMax, ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.5f, 1.0f)));
                    }
                    else
                    {
                        drawList->AddRectFilled(itemMin, itemMax, ImGui::GetColorU32(ImVec4(0.25f, 0.25f, 0.3f, 1.0f)));
                    }

                    // Draw compact text
                    drawList->AddText(ImVec2(itemMin.x + COMPONENT_ITEM_PADDING_X, itemMin.y + COMPONENT_ITEM_PADDING_Y), 
                                      ImGui::GetColorU32(ImVec4(0.9f, 0.9f, 0.9f, 1.0f)), 
                                      component.name.c_str());

                    // Drag-and-drop source
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        strncpy_s(m_draggedComponentBuffer, sizeof(m_draggedComponentBuffer), 
                                  component.name.c_str(), _TRUNCATE);

                        ImGui::SetDragDropPayload("COMPONENT_TYPE", 
                                                m_draggedComponentBuffer, 
                                                strlen(m_draggedComponentBuffer) + 1);

                        ImGui::Text("%s", component.name.c_str());

                        ImGui::EndDragDropSource();
                    }

                    // Tooltip
                    if (ImGui::IsItemHovered() && !component.description.empty())
                    {
                        ImGui::SetTooltip("%s", component.description.c_str());
                    }
                }

                ImGui::PopStyleVar();
                ImGui::TreePop();
            }
            else
            {
                m_categoryExpanded[i] = false;
            }
        }
    }

    void ComponentPalettePanel::RenderComponentList(EntityPrefabGraphDocument* document)
    {
        // Component list rendering is now integrated into RenderCategoryTabs()
        // This method is kept for backward compatibility
    }

    void ComponentPalettePanel::AddComponentToGraph(EntityPrefabGraphDocument* document, const ComponentType& componentType)
    {
        if (!document) { return; }

        // Create new node at center of visible area
        // For now, add at origin (0, 0) - can be improved to position near mouse
        NodeId newNodeId = document->CreateComponentNode(componentType.name, componentType.name);

        // Position new node in a reasonable location
        ComponentNode* newNode = document->GetNode(newNodeId);
        if (newNode)
        {
            // Place new node at offset from center, or at reasonable default
            newNode->position = Vector(100.0f, 100.0f, 0.0f);
            newNode->size = Vector(150.0f, 80.0f, 0.0f);
            newNode->enabled = true;
        }

        SYSTEM_LOG << "[ComponentPalettePanel] Added component: " << componentType.name << " (id=" << newNodeId << ")\n";
    }

    const std::vector<ComponentType>& ComponentPalettePanel::GetComponentTypes() const
    {
        return m_componentTypes;
    }

    void ComponentPalettePanel::RegisterComponentType(const std::string& name, const std::string& category, const std::string& description)
    {
        m_componentTypes.push_back(ComponentType(name, category, description));
    }

    bool ComponentPalettePanel::LoadComponentsFromJSON(const std::string& filepath)
    {
        using nlohmann::json;

        SYSTEM_LOG << "[ComponentPalettePanel] Loading components from: " << filepath << "\n";

        try
        {
            // Check if file exists using Windows _access() (0 = exists)
            if (_access(filepath.c_str(), 0) == -1)
            {
                SYSTEM_LOG << "[ComponentPalettePanel] WARNING: File does not exist: " << filepath << "\n";
                return false;
            }

            std::ifstream file(filepath);
            if (!file.is_open())
            {
                SYSTEM_LOG << "[ComponentPalettePanel] WARNING: Could not open file: " << filepath << " (permissions or other I/O issue)\n";
                return false;
            }

            json jsonData;
            file >> jsonData;
            file.close();

            // Clear existing components (loaded from JSON replaces hardcoded)
            m_componentTypes.clear();
            m_categories.clear();

            // Try new format first (schemas array with componentType)
            if (jsonData.contains("schemas") && jsonData["schemas"].is_array())
            {
                SYSTEM_LOG << "[ComponentPalettePanel] Detected new JSON format (schemas array)\n";

                const json& schemasArray = jsonData["schemas"];
                SYSTEM_LOG << "[ComponentPalettePanel] Found " << schemasArray.size() << " component types\n";

                // Parse each component schema
                for (const auto& schemaJson : schemasArray)
                {
                    if (!schemaJson.contains("componentType"))
                    {
                        SYSTEM_LOG << "[ComponentPalettePanel] WARNING: Skipping schema missing componentType\n";
                        continue;
                    }

                    std::string componentType = schemaJson["componentType"].get<std::string>();
                    std::string category = ExtractCategoryFromComponentType(componentType);
                    std::string description = schemaJson.contains("description") 
                        ? schemaJson["description"].get<std::string>() 
                        : componentType;  // Use componentType as description if none provided

                    RegisterComponentType(componentType, category, description);
                    SYSTEM_LOG << "[ComponentPalettePanel] Loaded: " << componentType << " (" << category << ")\n";
                }
            }
            // Fallback to old format (components array with name)
            else if (jsonData.contains("components") && jsonData["components"].is_array())
            {
                SYSTEM_LOG << "[ComponentPalettePanel] Detected old JSON format (components array)\n";

                const json& componentsArray = jsonData["components"];
                SYSTEM_LOG << "[ComponentPalettePanel] Found " << componentsArray.size() << " component types\n";

                // Parse each component
                for (const auto& compJson : componentsArray)
                {
                    if (!compJson.contains("name") || !compJson.contains("category"))
                    {
                        SYSTEM_LOG << "[ComponentPalettePanel] WARNING: Skipping component missing name or category\n";
                        continue;
                    }

                    std::string name = compJson["name"].get<std::string>();
                    std::string category = compJson["category"].get<std::string>();
                    std::string description = compJson.contains("description") 
                        ? compJson["description"].get<std::string>() 
                        : "";

                    RegisterComponentType(name, category, description);
                    SYSTEM_LOG << "[ComponentPalettePanel] Loaded: " << name << " (" << category << ")\n";
                }
            }
            else
            {
                SYSTEM_LOG << "[ComponentPalettePanel] ERROR: JSON missing 'schemas' or 'components' array\n";
                return false;
            }

            // Rebuild categories list
            std::map<std::string, bool> categoryMap;
            for (size_t i = 0; i < m_componentTypes.size(); ++i)
            {
                categoryMap[m_componentTypes[i].category] = true;
            }
            for (auto it = categoryMap.begin(); it != categoryMap.end(); ++it)
            {
                m_categories.push_back(it->first);
            }

            // Sort categories alphabetically
            std::sort(m_categories.begin(), m_categories.end());

            SYSTEM_LOG << "[ComponentPalettePanel] Successfully loaded " << m_componentTypes.size() 
                      << " components from JSON (" << m_categories.size() << " categories)\n";
            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[ComponentPalettePanel] ERROR parsing JSON: " << e.what() << "\n";
            return false;
        }
    }

    std::string ComponentPalettePanel::ExtractCategoryFromComponentType(const std::string& componentType)
    {
        // Extract category from component type name
        // Examples: "Identity_data" → "Core", "Camera_data" → "Camera", "PhysicsBody_data" → "Physics"

        if (componentType.find("Identity") != std::string::npos) return "Core";
        if (componentType.find("Position") != std::string::npos) return "Core";
        if (componentType.find("GridSettings") != std::string::npos) return "Core";
        if (componentType.find("EditorContext") != std::string::npos) return "Core";

        if (componentType.find("Physics") != std::string::npos) return "Physics";
        if (componentType.find("Movement") != std::string::npos) return "Physics";
        if (componentType.find("Collision") != std::string::npos) return "Physics";
        if (componentType.find("BoundingBox") != std::string::npos) return "Physics";
        if (componentType.find("TriggerZone") != std::string::npos) return "Physics";
        if (componentType.find("NavigationAgent") != std::string::npos) return "Physics";

        if (componentType.find("Visual") != std::string::npos) return "Graphics";
        if (componentType.find("Animation") != std::string::npos) return "Graphics";
        if (componentType.find("Sprite") != std::string::npos) return "Graphics";
        if (componentType.find("FX") != std::string::npos) return "Graphics";

        if (componentType.find("Camera") != std::string::npos) return "Camera";

        if (componentType.find("AI") != std::string::npos) return "AI";
        if (componentType.find("Behavior") != std::string::npos) return "AI";
        if (componentType.find("Controller") != std::string::npos) return "AI";
        if (componentType.find("NPC") != std::string::npos) return "AI";
        if (componentType.find("InputMapping") != std::string::npos) return "AI";

        if (componentType.find("Audio") != std::string::npos) return "Audio";
        if (componentType.find("Sound") != std::string::npos) return "Audio";

        if (componentType.find("Health") != std::string::npos) return "Gameplay";
        if (componentType.find("Inventory") != std::string::npos) return "Gameplay";

        if (componentType.find("Player") != std::string::npos) return "Player";

        // Default category
        return "Other";
    }

} // namespace Olympe
