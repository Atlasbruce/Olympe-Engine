/**
 * @file CanvasModalRenderer.h
 * @brief Centralized modal management for canvas editors (VisualScript, BehaviorTree, EntityPrefab).
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * Phase 42 Framework Unification - Modal Layer
 *
 * Provides unified interface for all graph editor modals:
 * - SubGraph file picker (used by node properties)
 * - Single rendering pipeline per frame
 * - Consistent appearance across all 3 graph editors
 *
 * Pattern: Singleton with delegation to modal instances.
 * Follows same architecture as CanvasToolbarRenderer.
 *
 * C++14 compliant.
 */

#pragma once

#include "../../Editor/Modals/SubGraphFilePickerModal.h"
#include <string>

namespace Olympe {

/**
 * @class CanvasModalRenderer
 * @brief Centralized dispatcher for all canvas editor modals.
 *
 * Provides unified interface for modal operations across:
 * - VisualScriptEditorPanel
 * - BehaviorTreeRenderer
 * - EntityPrefabRenderer
 *
 * Fixes BehaviorTree modal duplicate UI zones by ensuring single rendering
 * path per frame (was: DataManager rendering multiple times).
 *
 * Singleton pattern - use Get() for global access.
 */
class CanvasModalRenderer {
public:
    /**
     * @brief Get singleton instance.
     * @return Reference to global CanvasModalRenderer.
     */
    static CanvasModalRenderer& Get();

    // ========================================================================
    // SubGraph File Picker Modal
    // ========================================================================

    /**
     * @brief Open SubGraph file picker modal.
     * @param initialPath Directory to start browsing from (e.g., "Blueprints/").
     *
     * Usage:
     *   CanvasModalRenderer::Get().OpenSubGraphFilePickerModal("Blueprints/");
     *   // ... in render loop:
     *   CanvasModalRenderer::Get().RenderSubGraphFilePickerModal();
     */
    void OpenSubGraphFilePickerModal(const std::string& initialPath);

    /**
     * @brief Render SubGraph file picker modal this frame.
     *
     * Must be called once per frame if modal is open.
     * Typically called from TabManager::RenderTabBar() or main render loop.
     */
    void RenderSubGraphFilePickerModal();

    /**
     * @brief Check if SubGraph modal was confirmed by user.
     * @return true if user clicked "Select" button.
     */
    bool IsSubGraphModalConfirmed() const;

    /**
     * @brief Get selected SubGraph file path.
     * @return Full path to selected .ats file, empty if cancelled/no selection.
     *
     * Valid only after IsSubGraphModalConfirmed() returns true.
     * Call this once, then check IsSubGraphModalOpen() next frame to detect
     * when modal has closed.
     */
    std::string GetSelectedSubGraphFile() const;

    /**
     * @brief Check if SubGraph modal is currently open.
     * @return true if modal is displaying this frame.
     */
    bool IsSubGraphModalOpen() const;

    /**
     * @brief Close SubGraph modal without selection.
     */
    void CloseSubGraphModal();

private:
    // Private constructor - singleton pattern
    CanvasModalRenderer();

    // Private destructor
    ~CanvasModalRenderer() = default;

    // Disable copy/move
    CanvasModalRenderer(const CanvasModalRenderer&) = delete;
    CanvasModalRenderer& operator=(const CanvasModalRenderer&) = delete;

    // ========================================================================
    // Private Members
    // ========================================================================

    /**
     * @brief Instance of SubGraph file picker modal.
     * Wrapped by public interface for centralized rendering.
     */
    SubGraphFilePickerModal m_subGraphModal;

    /**
     * @brief Last confirmed selection from SubGraph modal.
     * Cached to avoid repeated queries from SubGraphFilePickerModal.
     */
    std::string m_cachedSelection;

    /**
     * @brief Whether SubGraph modal was confirmed last frame.
     * Clears after one read by caller to prevent multiple processing.
     */
    bool m_subGraphConfirmed;
};

} // namespace Olympe
