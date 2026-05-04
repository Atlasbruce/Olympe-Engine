/**
 * @file EntityPrefabEditorV2.h
 * @brief Framework-compliant renderer adapter for Entity Prefab graphs (Phase 2.1 Chunk 2)
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * EntityPrefabEditorV2 implements the IGraphRenderer interface and wraps:
 * - EntityPrefabGraphDocumentV2 (data model from Chunk 1)
 * - PrefabCanvas (existing UI rendering container)
 *
 * This adapter enables EntityPrefab graphs to be managed by the framework
 * through DocumentVersionManager and TabManager, providing:
 * - Unified document creation/loading
 * - Consistent save/load lifecycle
 * - Framework modal integration
 * - Canvas state preservation across tab switches
 *
 * C++14 compliant — no C++17 features (std::optional, structured bindings, etc.)
 */

#pragma once

#include <string>
#include <memory>
#include "../Framework/GraphEditorBase.h"
#include "EntityPrefabGraphDocumentV2.h"
#include "PrefabCanvas.h"
#include "../../system/system_utils.h"
#include "ComponentPalettePanel.h"
#include "PropertyEditorPanel.h"
#include "../Utilities/CustomCanvasEditor.h"

namespace Olympe
{
    /**
     * @class EntityPrefabEditorV2
     * @brief Framework renderer adapter for Entity Prefab graphs (PHASE E: GraphEditorBase inheritance)
     *
     * Inherits from GraphEditorBase to get common functionality:
     * - Selection management (m_selectedNodeIds, SelectMultipleNodes, etc.)
     * - Pan/Zoom utilities (m_canvasOffset, m_canvasZoom, ResetPanZoom)
     * - Context menus (m_showContextMenu, RenderContextMenu)
     * - Keyboard shortcuts (HandleCommonShortcuts)
     * - Framework toolbar (m_framework, RenderCommonToolbar)
     *
     * Wraps EntityPrefabGraphDocumentV2 data model and PrefabCanvas UI
     * using template method pattern:
     * - InitializeCanvasEditor() - setup canvas
     * - RenderGraphContent() - render nodes/connections
     * - RenderTypeSpecificToolbar() - custom toolbar items
     *
     * Ownership: Owned by TabManager via IGraphRenderer*, deleted when tab closes
     * Lifetime: Constructor → first Render() → ... → last Render() → Destructor
     */
    class EntityPrefabEditorV2 : public GraphEditorBase
    {
    public:
        /**
         * @brief Constructor
         * @param document  Pointer to EntityPrefabGraphDocumentV2 data model
         *                  Must not be nullptr; ownership remains with caller
         *
         * @details
         * Initializes the renderer with a document and creates a PrefabCanvas
         * for UI rendering. The document pointer is stored but ownership
         * is NOT transferred (caller responsible for lifetime management).
         */
        explicit EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document);

        /**
         * @brief Destructor
         * @details
         * Cleans up allocated PrefabCanvas. Document is not deleted
         * (ownership remains with caller).
         */
        virtual ~EntityPrefabEditorV2();

        // ====================================================================
        // IGraphRenderer Implementation - Lifecycle
        // ====================================================================

        // ====================================================================
        // IGraphRenderer Implementation - File I/O
        // ====================================================================

        /**
         * @brief Loads an Entity Prefab graph from disk
         * @param path  Path to .json file (EntityPrefab v4 schema)
         * @return true if load succeeded, false on error
         * @details
         * Delegates to m_document->LoadFromFile(path).
         * Clears canvas dirty flag on successful load.
         */
        virtual bool Load(const std::string& path) override;

        /**
         * @brief Saves the current Entity Prefab graph to disk
         * @param path  Destination path. If empty, uses current file path.
         * @return true if save succeeded, false on error
         * @details
         * Delegates to m_document->SaveToFile(path).
         * Clears canvas dirty flag on successful save.
         */
        virtual bool Save(const std::string& path) override;

        /**
         * @brief Returns the graph type identifier
         * @return "EntityPrefab"
         */
        virtual std::string GetGraphType() const override;

        // ====================================================================
        // EntityPrefabEditorV2 Specific Methods
        // ====================================================================

        /**
         * @brief Get pointer to underlying canvas
         * @return Pointer to PrefabCanvas for direct access if needed
         */
        PrefabCanvas* GetCanvas() const;

        /**
         * @brief Get pointer to underlying document
         * @return Pointer to EntityPrefabGraphDocumentV2
         */
        EntityPrefabGraphDocumentV2* GetDocument() const;

    protected:
        // ====================================================================
        // GraphEditorBase Template Method Overrides (PHASE E - Framework Integration)
        // ====================================================================

        /**
         * @brief Initialize canvas editor (called by base class Render())
         * @details
         * Creates PrefabCanvas and CustomCanvasEditor adapter if not already initialized.
         * Sets up document reference and canvas/editor bindings.
         */
        virtual void InitializeCanvasEditor() override;

        /**
         * @brief Render graph content (nodes, connections, canvas)
         * @details
         * Called by GraphEditorBase::Render() template method after toolbar.
         * Delegates to m_canvas->Render() which handles:
         * - Grid display
         * - Node rendering
         * - Connection rendering
         * - Interactive overlays (selection rectangle, connection preview)
         */
        virtual void RenderGraphContent() override;

        /**
         * @brief Override common toolbar to include framework buttons (Save/SaveAs)
         */
        virtual void RenderCommonToolbar() override;

        /**
         * @brief Render type-specific toolbar items
         * @details
         * Called by GraphEditorBase::RenderCommonToolbar() to add custom buttons.
         * EntityPrefab uses default toolbar (no custom items needed).
         */
        virtual void RenderTypeSpecificToolbar() override {}

        /**
         * @brief Render type-specific panels (right panel tabs)
         * @details
         * Called by GraphEditorBase::Render() after graph content.
         * Renders Component Palette and Property Editor tabs.
         */
        virtual void RenderTypePanels() override;

        /**
         * @brief Override selection rectangle handler
         * @param rectStart  Top-left corner of selection rectangle (canvas coords)
         * @param rectEnd    Bottom-right corner of selection rectangle (canvas coords)
         * @details
         * Delegates to PrefabCanvas::SelectNodesInRectangle() which performs AABB test
         * and updates base class m_selectedNodeIds vector.
         */
        virtual void SelectNodesInRectangle(const ImVec2& rectStart, const ImVec2& rectEnd) override;

        /**
         * @brief Override modals to show framework dialogs (Phase 75.1 FIX)
         */
        virtual void RenderModals() override;

        /**
         * @brief Delete selected nodes from document
         * @details
         * Iterates m_selectedNodeIds and calls m_document->DeleteNode() for each.
         * Marks document dirty and clears selection.
         */
        virtual void DeleteSelectedNodes() override;

        /**
         * @brief Move selected nodes by delta
         * @param deltaX  X offset in canvas coordinates
         * @param deltaY  Y offset in canvas coordinates
         * @details
         * Iterates m_selectedNodeIds and updates node positions in document.
         * Marks document dirty.
         */
        virtual void MoveSelectedNodes(float deltaX, float deltaY) override;

    private:
        // ====================================================================
        // Member Variables - Core (Using parent members with casts)
        // ====================================================================

        // Helper accessors for type safety
        EntityPrefabGraphDocumentV2* GetDoc() const { return static_cast<EntityPrefabGraphDocumentV2*>(m_document); }
        PrefabCanvas* GetCanvasPtr() const { return static_cast<PrefabCanvas*>(m_canvas); }

        // ====================================================================
        // Member Variables - UI Panels (Phase 0)
        // ====================================================================
        ComponentPalettePanel m_componentPalette;         ///< Component creation palette
        PropertyEditorPanel m_propertyEditor;             ///< Property editor panel
        std::unique_ptr<CustomCanvasEditor> m_canvasEditor;  ///< Canvas adapter (zoom/pan)

        // NOTE: m_framework is now inherited from GraphEditorBase (base class manages it)

        // ====================================================================
        // Member Variables - Layout State (Phase 0)
        // ====================================================================

        // NOTE: Minimap state now in m_canvasEditor (CustomCanvasEditor manages minimap)
        // NOTE: Grid state now in base class m_gridVisible (GraphEditorBase manages it)

        // ====================================================================
        // Private Helper Methods (Layout Rendering)
        // ====================================================================
        void RenderRightPanelTabs();       ///< Phase 4: Right panel tabs (Palette + Properties)

        void LogAction(const std::string& action) const;
    };

} // namespace Olympe
