/**
 * @file BehaviorTreeGraphDocument.h
 * @brief Adapter wrapper implementing IGraphDocument for BehaviorTree graphs.
 * @author Olympe Engine - Phase 41 Framework Integration
 * @date 2026-03-09
 *
 * @details
 * BehaviorTreeGraphDocument is an adapter that wraps BehaviorTreeRenderer
 * and exposes it through the IGraphDocument interface. This enables uniform
 * handling of all graph types (VisualScript, BehaviorTree, EntityPrefab) in the
 * unified framework.
 *
 * The adapter delegates Load/Save/metadata operations to existing BehaviorTreeRenderer
 * methods, minimizing changes to working code while providing polymorphic
 * document handling through the IGraphDocument interface.
 *
 * Pattern: Adapter (structural design pattern)
 * - Adapter: BehaviorTreeGraphDocument
 * - Adaptee: BehaviorTreeRenderer
 * - Target Interface: IGraphDocument
 */

#pragma once

#include "IGraphDocument.h"
#include <string>
#include <memory>

namespace Olympe
{
    // Forward declarations
    class BehaviorTreeRenderer;

    /**
     * @class BehaviorTreeGraphDocument
     * @brief Adapter implementing IGraphDocument for BehaviorTree renderer data.
     *
     * This class wraps a BehaviorTreeRenderer instance and provides the
     * IGraphDocument interface for unified graph document handling. The design
     * uses composition (contains a pointer) rather than inheritance, allowing
     * the adapter to work with existing BehaviorTreeRenderer code without
     * modification.
     *
     * Typical usage:
     * @code
     *   auto btRenderer = std::make_shared<BehaviorTreeRenderer>(panel);
     *   auto document = std::make_shared<BehaviorTreeGraphDocument>(btRenderer.get());
     *
     *   // Now use through IGraphDocument interface
     *   IGraphDocument* doc = document.get();
     *   doc->Load(filepath);
     *   doc->Save(filepath);
     *   std::string name = doc->GetName();
     *   DocumentType type = doc->GetType();
     * @endcode
     */
    class BehaviorTreeGraphDocument : public IGraphDocument
    {
    public:
        /**
         * @brief Construct adapter with reference to the BehaviorTreeRenderer.
         * @param btRenderer Pointer to BehaviorTreeRenderer instance.
         *                   The adapter holds a weak reference (non-owning).
         */
        explicit BehaviorTreeGraphDocument(BehaviorTreeRenderer* btRenderer);

        /**
         * @brief Destructor.
         */
        virtual ~BehaviorTreeGraphDocument() = default;

        // ===================================================================
        // IGraphDocument Interface Implementation
        // ===================================================================

        /**
         * @brief Load a BehaviorTree graph from file.
         * 
         * Delegates to BehaviorTreeRenderer::Load() by:
         * 1. Calling Load(filepath) on the renderer
         * 2. Storing the file path for subsequent operations
         *
         * @param filePath  Path to the BT graph file to load.
         * @return true on success, false on failure.
         */
        virtual bool Load(const std::string& filePath) override;

        /**
         * @brief Save the BehaviorTree graph to file.
         * 
         * Delegates to BehaviorTreeRenderer::Save() to save to the
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
         * @return Human-readable name for the graph (e.g., "behavior_tree.json").
         */
        virtual std::string GetName() const override;

        /**
         * @brief Get the document type.
         * @return DocumentType::BEHAVIOR_TREE
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
         * Note: For BehaviorTree, rendering is handled by BehaviorTreeRenderer
         * itself through imnodes. This method returns nullptr as BehaviorTree
         * doesn't use the separate IGraphRenderer interface.
         *
         * @return nullptr (rendering handled directly by renderer)
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
         * externally. For BehaviorTree, this is a notification hook for future
         * framework integration.
         */
        virtual void OnDocumentModified() override;

        // ===================================================================
        // Adapter-Specific Methods
        // ===================================================================

        /**
         * @brief Get the wrapped BehaviorTreeRenderer.
         * @return Pointer to the BehaviorTreeRenderer (may be nullptr).
         */
        BehaviorTreeRenderer* GetBehaviorTreeRenderer() { return m_btRenderer; }

        /**
         * @brief Get the wrapped BehaviorTreeRenderer (const version).
         * @return Const pointer to the BehaviorTreeRenderer (may be nullptr).
         */
        const BehaviorTreeRenderer* GetBehaviorTreeRenderer() const { return m_btRenderer; }

    private:
        /**
         * @brief Pointer to the wrapped BehaviorTreeRenderer.
         * 
         * This is a non-owning reference. The adapter does not manage
         * the lifetime of the renderer; the renderer is owned by the tab system.
         */
        BehaviorTreeRenderer* m_btRenderer = nullptr;

        /**
         * @brief Cached file path for the current graph.
         * 
         * Tracks the path separate from the renderer to maintain consistency
         * with the IGraphDocument interface contract.
         */
        std::string m_filePath;
    };

} // namespace Olympe
