#include "ComponentPalettePanel.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../../system/system_utils.h"
#include "../../third_party/nlohmann/json.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>

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

        // Try to load from JSON file first
        bool jsonLoaded = LoadComponentsFromJSON("./Gamedata/PrefabEntities/ComponentsParameters.json");

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

        SYSTEM_LOG << "[ComponentPalettePanel] Initialization complete with " << m_componentTypes.size() << " component types\n";
    }

    void ComponentPalettePanel::Render(EntityPrefabGraphDocument* document)
    {
        if (!document) { return; }

        ImGui::BeginChild("##ComponentPalette", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::TextUnformatted("Components");
        ImGui::Separator();

        // Search bar
        ImGui::InputText("##ComponentSearch", m_searchBuffer, sizeof(m_searchBuffer), ImGuiInputTextFlags_CharsNoBlank);
        m_searchFilter = m_searchBuffer;

        ImGui::Separator();

        // Category tabs
        RenderCategoryTabs();

        ImGui::Separator();

        // Component list
        RenderComponentList(document);

        ImGui::EndChild();
    }

    void ComponentPalettePanel::RenderSearchBar()
    {
        ImGui::TextUnformatted("Search:");
        ImGui::InputText("##search", m_searchBuffer, sizeof(m_searchBuffer));
    }

    void ComponentPalettePanel::RenderCategoryTabs()
    {
        // "All" category
        if (ImGui::Selectable("All", m_selectedCategoryIndex == -1))
        {
            m_selectedCategoryIndex = -1;
        }

        // Individual categories
        for (size_t i = 0; i < m_categories.size(); ++i)
        {
            if (ImGui::Selectable(m_categories[i].c_str(), m_selectedCategoryIndex == (int)i))
            {
                m_selectedCategoryIndex = (int)i;
            }
        }
    }

    void ComponentPalettePanel::RenderComponentList(EntityPrefabGraphDocument* document)
    {
        ImGui::BeginChild("##ComponentList", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        for (size_t i = 0; i < m_componentTypes.size(); ++i)
        {
            const ComponentType& component = m_componentTypes[i];

            // Filter by category
            if (m_selectedCategoryIndex >= 0 && component.category != m_categories[m_selectedCategoryIndex])
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

            // Render component item as button with drag-drop support
            std::string label = component.name + "##" + component.name + std::to_string(i);

            // Use Invisible button so drag-drop works
            ImGui::InvisibleButton(label.c_str(), ImVec2(-1.0f, 30.0f));

            // Draw background to show it's hoverable
            ImVec2 itemMin = ImGui::GetItemRectMin();
            ImVec2 itemMax = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            if (ImGui::IsItemHovered())
            {
                drawList->AddRectFilled(itemMin, itemMax, ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.5f, 1.0f)));
            }
            else
            {
                drawList->AddRectFilled(itemMin, itemMax, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.35f, 1.0f)));
            }

            drawList->AddText(ImVec2(itemMin.x + 5.0f, itemMin.y + 6.0f), ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), component.name.c_str());

            // Drag-and-drop source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                // Copy component name to stable buffer for drag-drop payload
                strncpy_s(m_draggedComponentBuffer, sizeof(m_draggedComponentBuffer), 
                          component.name.c_str(), _TRUNCATE);

                // Set payload with stable buffer
                ImGui::SetDragDropPayload("COMPONENT_TYPE", 
                                        m_draggedComponentBuffer, 
                                        strlen(m_draggedComponentBuffer) + 1);

                // Display preview during drag
                ImGui::Text("Adding: %s", component.name.c_str());

                ImGui::EndDragDropSource();
            }

            // Tooltip
            if (ImGui::IsItemHovered() && !component.description.empty())
            {
                ImGui::SetTooltip("%s", component.description.c_str());
            }
        }

        ImGui::EndChild();
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
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                SYSTEM_LOG << "[ComponentPalettePanel] WARNING: Could not open file: " << filepath << ", using hardcoded components\n";
                return false;
            }

            json jsonData;
            file >> jsonData;
            file.close();

            // Clear existing components (loaded from JSON replaces hardcoded)
            m_componentTypes.clear();
            m_categories.clear();

            // Validate structure
            if (!jsonData.contains("components") || !jsonData["components"].is_array())
            {
                SYSTEM_LOG << "[ComponentPalettePanel] ERROR: JSON missing 'components' array\n";
                return false;
            }

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

} // namespace Olympe
