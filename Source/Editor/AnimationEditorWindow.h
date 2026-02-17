/**
 * @file AnimationEditorWindow.h
 * @brief Animation Editor window for creating and editing animation banks
 * @author Olympe Engine - Animation System
 * @date 2025
 *
 * @details
 * Provides a comprehensive animation editor for creating, editing, and previewing
 * animation banks with multi-spritesheet support.
 *
 * Features:
 * - Multi-spritesheet management per bank
 * - Animation sequence editor with frame ranges
 * - Real-time preview with playback controls
 * - Spritesheet viewer with grid overlay and zoom/pan
 * - JSON export using Unified Schema v2 format
 */

#pragma once

#include "../Animation/AnimationTypes.h"
#include <string>
#include <vector>

// Forward declarations
struct SDL_Texture;
struct SDL_Window;
struct SDL_Renderer;
struct ImGuiContext;

namespace Olympe
{
    /**
     * @class AnimationEditorWindow
     * @brief Main animation editor window
     *
     * Provides UI for creating and editing animation banks with multi-spritesheet support.
     * Opens with F9 hotkey. Renders in standalone SDL3 window (like BT Debugger).
     */
    class AnimationEditorWindow
    {
    public:
        AnimationEditorWindow();
        ~AnimationEditorWindow();

        /**
         * @brief Toggle window visibility
         */
        void Toggle();

        /**
         * @brief Check if window is open
         */
        bool IsOpen() const { return m_isOpen; }

        /**
         * @brief Update and render the editor window (separate window)
         * @param deltaTime Time elapsed since last frame in seconds
         */
        void Update(float deltaTime);

        /**
         * @brief Process SDL events for the separate window
         * @param event SDL event to process
         */
        void ProcessEvent(SDL_Event* event);

        /**
         * @brief Update preview animation (call every frame with deltaTime)
         * @param deltaTime Time elapsed since last frame in seconds
         */
        void UpdatePreview(float deltaTime);

        /**
         * @brief Render the editor window
         */
        void Render();

    private:
        // UI Panel Rendering
        void RenderMainMenu();
        void RenderBankListPanel();
        void RenderSpritesheetPanel();
        void RenderSequencePanel();
        void RenderPreviewPanel();
        void RenderPropertiesPanel();

        // File Operations
        void NewBank();
        void OpenBank(const std::string& filepath);
        void SaveBank();
        void SaveBankAs();
        void ImportBankJSON(const std::string& filepath);
        void ExportBankJSON(const std::string& filepath);
        std::vector<std::string> ScanBankDirectory(const std::string& dirPath);

        // Spritesheet Operations
        void AddSpritesheet();
        void RemoveSpritesheet(int index);
        void AutoDetectGrid(SpritesheetInfo& sheet);
        SDL_Texture* LoadSpritesheetTexture(const std::string& path);

        // Sequence Operations
        void AddSequence();
        void RemoveSequence(int index);

        // Preview Operations
        void StartPreview();
        void StopPreview();
        void PausePreview();
        void ResetPreview();
        void RenderPreviewFrame();

        // Helper Methods
        void MarkDirty();
        void ClearDirty();
        bool PromptUnsavedChanges();
        void UpdateWindowTitle();

        // State
        bool m_isOpen = false;
        bool m_isDirty = false;
        
        // Current Bank Data
        AnimationBank m_currentBank;
        std::string m_currentBankPath;
        bool m_hasBankLoaded = false;

        // Selection State
        int m_selectedSpritesheetIndex = -1;
        int m_selectedSequenceIndex = -1;
        
        // UI State
        int m_activeTab = 0;  // 0 = Spritesheets, 1 = Sequences
        
        // Preview State
        bool m_isPreviewPlaying = false;
        bool m_isPreviewPaused = false;
        float m_previewSpeed = 1.0f;
        int m_previewCurrentFrame = 0;
        float m_previewFrameTimer = 0.0f;
        
        // Spritesheet Viewer State
        float m_spritesheetZoom = 1.0f;
        float m_spritesheetPanX = 0.0f;
        float m_spritesheetPanY = 0.0f;
        bool m_showGrid = true;
        
        // Texture Cache (spritesheet path -> SDL_Texture*)
        // Note: We'll use DataManager for texture loading, so this is just for tracking
        
        // Dialog State
        bool m_showNewBankDialog = false;
        bool m_showOpenBankDialog = false;
        bool m_showAddSpritesheetDialog = false;
        bool m_showAddSequenceDialog = false;
        
        // Input Buffers (for dialogs)
        char m_inputBankId[256] = "";
        char m_inputDescription[1024] = "";
        char m_inputAuthor[256] = "";
        char m_inputSpritesheetId[256] = "";
        char m_inputSpritesheetPath[512] = "";
        char m_inputSequenceName[256] = "";

        // ===== Standalone Window Management =====
        SDL_Window* m_separateWindow = nullptr;
        SDL_Renderer* m_separateRenderer = nullptr;
        ImGuiContext* m_separateImGuiContext = nullptr;
        
        void CreateSeparateWindow();
        void DestroySeparateWindow();
        void RenderSeparateWindow();
    };

} // namespace Olympe
