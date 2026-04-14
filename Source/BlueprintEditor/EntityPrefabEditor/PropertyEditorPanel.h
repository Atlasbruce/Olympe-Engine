#pragma once

#include <string>
#include <vector>
#include <memory>
#include "./../../vector.h"
#include "EntityPrefabGraphDocument.h"
#include "../Framework/CanvasModalRenderer.h"

// Forward declaration for NodePropertiesPanel stub (moved to PropertyEditorPanel)
class NodePropertiesPanel;

namespace Olympe
{
    class PropertyEditorPanel
    {
    public:
        PropertyEditorPanel();
        ~PropertyEditorPanel();

        void Initialize();
        void Render(EntityPrefabGraphDocument* document);

        // Selection management
        void SetSelectedNode(NodeId nodeId);
        NodeId GetSelectedNode() const;
        void ClearSelection();

        // Property editing
        bool HasSelectedNode() const;
        void ApplyChanges();

    private:
        NodeId m_selectedNodeId = InvalidNodeId;
        EntityPrefabGraphDocument* m_document = nullptr;

        // Rendering helpers
        void RenderNodeInfo();
        void RenderNodeProperties();
    };
}
