#pragma once

#include <string>
#include <vector>
#include <map>
#include "EntityPrefabGraphDocument.h"

namespace Olympe
{
    // Forward declaration
    class EntityPrefabGraphDocumentV2;
    // Available component types that can be added to entity prefabs
    struct ComponentType
    {
        std::string name;
        std::string category;
        std::string description;

        ComponentType() = default;
        ComponentType(const std::string& n, const std::string& c, const std::string& d = "")
            : name(n), category(c), description(d) { }
    };

    class ComponentPalettePanel
    {
    public:
        ComponentPalettePanel();
        ~ComponentPalettePanel();

        // Initialize with available components
        void Initialize();

        // Render the palette panel (V1 document)
        void Render(EntityPrefabGraphDocument* document);

        // Render the palette panel (V2 document - Phase C Adapter)
        void Render(EntityPrefabGraphDocumentV2* document);

        // Get list of registered component types
        const std::vector<ComponentType>& GetComponentTypes() const;

        // Add a component type to the palette
        void RegisterComponentType(const std::string& name, const std::string& category, const std::string& description = "");

        // Load components from JSON file (./Gamedata/PrefabEntities/ComponentsParameters.json)
        bool LoadComponentsFromJSON(const std::string& filepath);

    private:
        // ========== UI Styling Constants (Dense Layout) ==========
        static constexpr float CATEGORY_HEADER_HEIGHT = 22.0f;
        static constexpr float COMPONENT_ITEM_HEIGHT = 18.0f;
        static constexpr float COMPONENT_ITEM_PADDING_X = 5.0f;
        static constexpr float COMPONENT_ITEM_PADDING_Y = 2.0f;

        std::vector<ComponentType> m_componentTypes;
        std::vector<std::string> m_categories;
        std::vector<bool> m_categoryExpanded;  // Track expanded/collapsed state for each category
        std::string m_searchFilter;
        int m_selectedCategoryIndex = 0;
        char m_searchBuffer[256];
        char m_draggedComponentBuffer[256];  // Buffer for drag-drop payload (must be stable)

        // Utility methods
        void RenderSearchBar();
        void RenderComponentList(EntityPrefabGraphDocument* document);
        void RenderCategoryTabs();
        void AddComponentToGraph(EntityPrefabGraphDocument* document, const ComponentType& componentType);
        void AddComponentToGraph(EntityPrefabGraphDocumentV2* document, const ComponentType& componentType);

        // Category extraction from component type name
        std::string ExtractCategoryFromComponentType(const std::string& componentType);
    };
}
