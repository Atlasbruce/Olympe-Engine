/**
 * @file VisualScriptGraphDocument.h
 * @brief Adapter wrapper implementing IGraphDocument for VisualScript graphs.
 * @author Olympe Engine - Phase 41 Framework Integration
 * @date 2026-03-09
 *
 * @details
 * VisualScriptGraphDocument is an adapter that wraps VisualScriptEditorPanel
 * and exposes it through the IGraphDocument interface. This enables uniform
 * handling of all graph types (VisualScript, BehaviorTree, EntityPrefab) in the
 * unified framework.
 *
 * The adapter delegates Load/Save/metadata operations to existing VisualScriptEditorPanel
 * methods, minimizing changes to working code while providing polymorphic
 * document handling through the IGraphDocument interface.
 *
 * Pattern: Adapter (structural design pattern)
 * - Adapter: VisualScriptGraphDocument
 * - Adaptee: VisualScriptEditorPanel
 * - Target Interface: IGraphDocument
 */

#pragma once

#include "IGraphDocument.h"
#include <string>
#include <memory>

namespace Olympe
{
    // Forward declarations
    class VisualScriptEditorPanel;
    class TaskGraphTemplate;

    /**
     * @class VisualScriptGraphDocument
     * @brief Adapter implementing IGraphDocument for VisualScript editor data.
     *
     * This class wraps a VisualScriptEditorPanel instance and provides the
     * IGraphDocument interface for unified graph document handling. The design
     * uses composition (contains a pointer) rather than inheritance, allowing
     * the adapter to work with existing VisualScriptEditorPanel code without
     * modification.
     *
     * Typical usage:
     * @code
     *   auto vsPanel = std::make_shared<VisualScriptEditorPanel>();
     *   auto document = std::make_shared<VisualScriptGraphDocument>(vsPanel.get());
     *
     *   // Now use through IGraphDocument interface
     *   IGraphDocument* doc = document.get();
     *   doc->Load(filepath);
     *   doc->Save(filepath);
     *   std::string name = doc->GetName();
     *   DocumentType type = doc->GetType();
     * @endcode
     */
    class VisualScriptGraphDocument : public IGraphDocument
    {
    public:
        /**
         * @brief Construct adapter with reference to the VisualScriptEditorPanel.
         * @param vsPanel Pointer to VisualScriptEditorPanel instance.
         *               The adapter holds a weak reference (non-owning).
         */
        explicit VisualScriptGraphDocument(VisualScriptEditorPanel* vsPanel);

        /**
         * @brief Destructor.
         */
        virtual ~VisualScriptGraphDocument() = default;

        // ===================================================================
        // IGraphDocument Interface Implementation
        // ===================================================================

        /**
         * @brief Load a Visual Script graph from file.
         * 
         * Delegates to VisualScriptEditorPanel::LoadTemplate() by:
         * 1. Loading the JSON file as a TaskGraphTemplate
         * 2. Passing the template to LoadTemplate() with the file path
         * 3. Storing the file path for subsequent Save() operations
         *
         * @param filePath  Path to the .json file to load.
         * @return true on success, false on failure.
         */
        virtual bool Load(const std::string& filePath) override;

        /**
         * @brief Save the Visual Script graph to file.
         * 
         * Delegates to VisualScriptEditorPanel::SaveAs() to save to the
         * specified file path, then updates internal path tracking.
         *
         * @param filePath  Path where the graph should be saved.
         * @return true on success, false on failure.
         */
        virtual bool Save(const std::string& filePath) override;

        /**
         * @brief Get the display name for this document.
         * 
         * Returns the filename portion of the current path, or a default
         * name if no file is loaded.
         *
         * @return Human-readable name for the graph (e.g., "graph_name.json").
         */
        virtual std::string GetName() const override;

        /**
         * @brief Get the document type.
         * @return DocumentType::VISUAL_SCRIPT
         */
        virtual DocumentType GetType() const override;

        /**
         * @brief Get the file path of the currently loaded graph.
         * @return Path string (may be empty if unsaved).
         */
        virtual std::string GetFilePath() const override;

        /**
         * @brief Set the file path for this document.
         * @param path New file path.
         */
        virtual void SetFilePath(const std::string& path) override;

        /**
         * @brief Check if the document has unsaved changes.
         * @return true if dirty, false if clean.
         */
        virtual bool IsDirty() const override;

        /**
         * @brief Get the graph renderer.
         * 
         * Note: For VisualScript, rendering is handled by VisualScriptEditorPanel
         * itself through ImNodes. This method returns nullptr as VisualScript
         * doesn't use the separate IGraphRenderer interface.
         *
         * @return nullptr (rendering handled directly by editor panel)
         */
        virtual IGraphRenderer* GetRenderer() override;

        /**
         * @brief Get the graph renderer (const version).
         * @return nullptr (const version)
         */
        virtual const IGraphRenderer* GetRenderer() const override;

        /**
         * @brief Notify the document of modifications.
         * 
         * This method is called by the framework when the document is modified
         * externally. Sets the dirty flag in the VisualScriptEditorPanel.
         */
        virtual void OnDocumentModified() override;

        // ===================================================================
        // Adapter-Specific Methods
        // ===================================================================

        /**
         * @brief Get the wrapped VisualScriptEditorPanel.
         * @return Pointer to the VisualScriptEditorPanel (may be nullptr).
         */
        VisualScriptEditorPanel* GetVisualScriptPanel() { return m_vsPanel; }

        /**
         * @brief Get the wrapped VisualScriptEditorPanel (const version).
         * @return Const pointer to the VisualScriptEditorPanel (may be nullptr).
         */
        const VisualScriptEditorPanel* GetVisualScriptPanel() const { return m_vsPanel; }

    private:
        /**
         * @brief Pointer to the wrapped VisualScriptEditorPanel.
         * 
         * This is a non-owning reference. The adapter does not manage
         * the lifetime of the panel; the panel is owned by the tab system.
         */
        VisualScriptEditorPanel* m_vsPanel = nullptr;

        /**
         * @brief Cached file path for the current graph.
         * 
         * Tracks the path separate from the panel to maintain consistency
         * with the IGraphDocument interface contract.
         */
        std::string m_filePath;
    };

} // namespace Olympe
