/**
 * @file AIEditorMenus.h
 * @brief Menu handlers for AI Editor
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Provides menu rendering and dialog handling for AI Editor.
 */

#pragma once

#include <string>

namespace Olympe {
namespace AI {

// Forward declaration
class AIEditorGUI;

/**
 * @class AIEditorMenus
 * @brief Static menu helper functions
 */
class AIEditorMenus {
public:
    /**
     * @brief Render File menu
     * @param editor Pointer to editor instance
     */
    static void RenderFileMenu(AIEditorGUI* editor);
    
    /**
     * @brief Render Edit menu
     * @param editor Pointer to editor instance
     */
    static void RenderEditMenu(AIEditorGUI* editor);
    
    /**
     * @brief Render View menu
     * @param editor Pointer to editor instance
     */
    static void RenderViewMenu(AIEditorGUI* editor);
    
    /**
     * @brief Render Help menu
     * @param editor Pointer to editor instance
     */
    static void RenderHelpMenu(AIEditorGUI* editor);
    
    /**
     * @brief Show New BT dialog
     * @param editor Pointer to editor instance
     */
    static void ShowNewBTDialog(AIEditorGUI* editor);
    
    /**
     * @brief Show Open dialog
     * @param editor Pointer to editor instance
     */
    static void ShowOpenDialog(AIEditorGUI* editor);
    
    /**
     * @brief Show Save As dialog
     * @param editor Pointer to editor instance
     */
    static void ShowSaveAsDialog(AIEditorGUI* editor);
    
    /**
     * @brief Show About dialog
     */
    static void ShowAboutDialog();
};

} // namespace AI
} // namespace Olympe
