#pragma once

#include <string>
#include <memory>
#include "./../../vector.h"
#include "EntityPrefabGraphDocument.h"

namespace Olympe
{
    class PropertyInspectorPrefab
    {
    public:
        PropertyInspectorPrefab();
        ~PropertyInspectorPrefab();

        void Initialize(EntityPrefabGraphDocument* document);
        void Render();
        void Update(float deltaTime);

        // Node selection
        void SelectNode(NodeId nodeId);
        void DeselectNode();
        NodeId GetSelectedNode() const;

        // Panel visibility and layout
        void SetPanelWidth(float width);
        float GetPanelWidth() const;

        void SetPanelHeight(float height);
        float GetPanelHeight() const;

        void SetPanelPosition(float x, float y);
        void GetPanelPosition(float& outX, float& outY) const;

        // Content rendering options
        void SetShowComponentInfo(bool show);
        bool GetShowComponentInfo() const;

        void SetShowPropertyGrid(bool show);
        bool GetShowPropertyGrid() const;

        void SetShowAddPropertyButton(bool show);
        bool GetShowAddPropertyButton() const;

        void SetReadOnly(bool readOnly);
        bool IsReadOnly() const;

        // Property editing
        void AddProperty(const std::string& key, const std::string& value);
        void RemoveProperty(const std::string& key);
        void SetProperty(const std::string& key, const std::string& value);
        std::string GetProperty(const std::string& key) const;

        // Component management
        void AddComponent(const std::string& componentType);
        void RemoveComponent(const std::string& componentType);
        void EnableComponent(const std::string& componentType, bool enabled);

        // Validation
        bool ValidatePropertyValue(const std::string& propertyName, const std::string& value) const;
        bool ValidateAllProperties() const;

    private:
        EntityPrefabGraphDocument* m_document = nullptr;
        NodeId m_selectedNodeId = InvalidNodeId;
        float m_panelWidth = 400.0f;
        float m_panelHeight = 600.0f;
        float m_panelX = 0.0f;
        float m_panelY = 0.0f;
        bool m_showComponentInfo = true;
        bool m_showPropertyGrid = true;
        bool m_showAddPropertyButton = true;
        bool m_readOnly = false;

        void RenderComponentInfo();
        void RenderPropertyGrid();
        void RenderAddPropertyPanel();
        void RenderComponentList();

        void OnPropertyChanged(const std::string& key, const std::string& value);
        void OnComponentAdded(const std::string& componentType);
        void OnComponentRemoved(const std::string& componentType);
    };
}
