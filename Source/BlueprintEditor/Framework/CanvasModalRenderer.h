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

#include "../../Editor/Modals/FilePickerModal.h"
#include "../../Editor/Modals/SaveFilePickerModal.h"
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
 * Phase 44 Consolidation: Uses FilePickerModal for SubGraph selection (with folder panel)
 * instead of dedicated SubGraphFilePickerModal (which lacked folder navigation).
 * This ensures consistent UI across all graph editors with full folder browsing.
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

    // ========================================================================
    // BehaviorTree File Picker Modal
    // ========================================================================

    /**
     * @brief Open BehaviorTree file picker modal.
     * @param initialPath Directory to start browsing from (e.g., "./Gamedata").
     *
     * Used by EntityPrefabRenderer property panels for BehaviorTree selection.
     */
    void OpenBehaviorTreeFilePickerModal(const std::string& initialPath);

    /**
     * @brief Render BehaviorTree file picker modal this frame.
     */
    void RenderBehaviorTreeFilePickerModal();

    /**
     * @brief Check if BehaviorTree modal was confirmed by user.
     */
    bool IsBehaviorTreeModalConfirmed() const;

    /**
     * @brief Get selected BehaviorTree file path.
     */
    std::string GetSelectedBehaviorTreeFile() const;

    /**
     * @brief Check if BehaviorTree modal is currently open.
     */
    bool IsBehaviorTreeModalOpen() const;

    /**
     * @brief Close BehaviorTree modal without selection.
     */
    void CloseBehaviorTreeModal();

    // ========================================================================
    // SaveAs File Picker Modal
    // ========================================================================

    /**
     * @brief Open Save File picker modal.
     * @param initialPath Directory to start from.
     * @param suggestedName Default filename.
     * @param fileType Type of file being saved (BehaviorTree, Blueprint, EntityPrefab, Audio).
     */
    void OpenSaveFilePickerModal(const std::string& initialPath, 
                                  const std::string& suggestedName,
                                  SaveFileType fileType);

    /**
     * @brief Render Save File picker modal this frame.
     */
    void RenderSaveFilePickerModal();

    /**
     * @brief Check if Save modal was confirmed by user.
     */
    bool IsSaveFileModalConfirmed() const;

    /**
     * @brief Get selected save file path.
     */
    std::string GetSelectedSaveFilePath() const;

    /**
     * @brief Check if Save modal is currently open.
     */
    bool IsSaveFileModalOpen() const;

    /**
     * @brief Close Save modal without selection.
     */
    void CloseSaveFileModal();

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
     * Phase 44: Changed from SubGraphFilePickerModal to FilePickerModal (with folder tree UI).
     * This ensures consistent UI across all property panels with folder navigation enabled.
     */
    FilePickerModal m_subGraphModal{FilePickerType::SubGraph};

    /**
     * @brief Instance of BehaviorTree file picker modal.
     * Phase 44.1: Unified from PropertyEditorPanel::m_behaviorTreeModal.
     */
    FilePickerModal m_behaviorTreeModal{FilePickerType::BehaviorTree};

    /**
     * @brief Instance of Save file picker modal.
     * Phase 44.1: Unified from CanvasToolbarRenderer::m_saveModal.
     */
    SaveFilePickerModal m_saveFileModal{SaveFileType::Blueprint};

    // ========================================================================
    // State Cache
    // ========================================================================

    /**
     * @brief Last confirmed selection from SubGraph modal.
     */
    std::string m_cachedSubGraphSelection;

    /**
     * @brief Whether SubGraph modal was confirmed last frame.
     */
    bool m_subGraphConfirmed = false;

    /**
     * @brief Last confirmed selection from BehaviorTree modal.
     */
    std::string m_cachedBehaviorTreeSelection;

    /**
     * @brief Whether BehaviorTree modal was confirmed last frame.
     */
    bool m_behaviorTreeConfirmed = false;

    /**
     * @brief Last confirmed selection from Save modal.
     */
    std::string m_cachedSaveSelection;

    /**
     * @brief Whether Save modal was confirmed last frame.
     */
    bool m_saveConfirmed = false;
};

} // namespace Olympe
