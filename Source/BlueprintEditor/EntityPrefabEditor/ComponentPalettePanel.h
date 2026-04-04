#pragma once

#include <string>
#include <vector>
#include <map>
#include "EntityPrefabGraphDocument.h"

namespace Olympe
{
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

        // Render the palette panel
        void Render(EntityPrefabGraphDocument* document);

        // Get list of registered component types
        const std::vector<ComponentType>& GetComponentTypes() const;

        // Add a component type to the palette
        void RegisterComponentType(const std::string& name, const std::string& category, const std::string& description = "");

    private:
        std::vector<ComponentType> m_componentTypes;
        std::vector<std::string> m_categories;
        std::string m_searchFilter;
        int m_selectedCategoryIndex = 0;
        char m_searchBuffer[256];

        // Utility methods
        void RenderSearchBar();
        void RenderComponentList(EntityPrefabGraphDocument* document);
        void RenderCategoryTabs();
        void AddComponentToGraph(EntityPrefabGraphDocument* document, const ComponentType& componentType);
    };
}
