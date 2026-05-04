#pragma once

#include <string>
#include <vector>
#include <memory>
#include "./../../vector.h"
#include "EntityPrefabGraphDocument.h"
#include "../Framework/CanvasModalRenderer.h"

// Forward declarations
class NodePropertiesPanel;
namespace Olympe { class EntityPrefabGraphDocumentV2; }

namespace Olympe
{
    class PropertyEditorPanel
    {
    public:
        PropertyEditorPanel();
        ~PropertyEditorPanel();

        void Initialize();
        void Render(EntityPrefabGraphDocument* document);
        void Render(Olympe::EntityPrefabGraphDocumentV2* document);  // Phase C: V2 Adapter

        // Selection management
        void SetSelectedNode(PrefabNodeId nodeId);
        PrefabNodeId GetSelectedNode() const;
        void ClearSelection();

        // Property editing
        bool HasSelectedNode() const;
        void ApplyChanges();

    private:
        PrefabNodeId m_selectedNodeId = InvalidNodeId;
        EntityPrefabGraphDocument* m_document = nullptr;
        Olympe::EntityPrefabGraphDocumentV2* m_documentV2 = nullptr;  // Phase C: V2 support

        // Rendering helpers
        void RenderNodeInfo();
        void RenderNodeProperties();
        void RenderNodeInfoV2();         // Phase C: V2 versions
        void RenderNodePropertiesV2();
    };
}
