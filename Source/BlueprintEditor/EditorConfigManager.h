/*
 * Olympe Blueprint Editor - Configuration Manager
 * 
 * Manages editor configuration through JSON file instead of imgui.ini
 * Handles loading and saving of:
 * - Window positions and sizes
 * - Panel visibility states
 * - Editor preferences
 * - ImGui and ImNodes settings
 */

#pragma once

#include "../third_party/nlohmann/json.hpp"
#include <string>
#include <map>

namespace Olympe
{
    struct WindowConfig
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 800.0f;
        float height = 600.0f;
        bool collapsed = false;
    };

    struct PanelVisibility
    {
        bool showAssetBrowser = true;
        bool showInspector = true;
        bool showNodeGraph = true;
        bool showTemplateBrowser = false;
        bool showHistory = false;
    };

    struct EditorPreferences
    {
        bool gridLinesEnabled = true;
        bool autoSaveEnabled = true;
        int autoSaveIntervalSeconds = 300;
        float panningSpeed = 1.0f;
        float zoomSpeed = 0.1f;
    };

    struct ImGuiSettings
    {
        // Store ImGui window settings as key-value pairs
        std::map<std::string, WindowConfig> windowConfigs;
    };

    class EditorConfigManager
    {
    public:
        static EditorConfigManager& Get();

        // Load configuration from JSON file
        bool LoadConfig(const std::string& filepath);
        
        // Save configuration to JSON file
        bool SaveConfig(const std::string& filepath);
        
        // Getters
        const PanelVisibility& GetPanelVisibility() const { return m_PanelVisibility; }
        const EditorPreferences& GetPreferences() const { return m_Preferences; }
        const ImGuiSettings& GetImGuiSettings() const { return m_ImGuiSettings; }
        
        // Setters
        void SetPanelVisibility(const PanelVisibility& visibility) { m_PanelVisibility = visibility; }
        void SetPreferences(const EditorPreferences& prefs) { m_Preferences = prefs; }
        
        // ImGui window state management
        void SaveWindowConfig(const std::string& windowName, const WindowConfig& config);
        WindowConfig GetWindowConfig(const std::string& windowName) const;
        
        // Apply settings to ImGui/ImNodes
        void ApplyToImGui();
        
        // Capture current ImGui state
        void CaptureFromImGui();

    private:
        EditorConfigManager() = default;
        ~EditorConfigManager() = default;
        
        EditorConfigManager(const EditorConfigManager&) = delete;
        EditorConfigManager& operator=(const EditorConfigManager&) = delete;

        PanelVisibility m_PanelVisibility;
        EditorPreferences m_Preferences;
        ImGuiSettings m_ImGuiSettings;

        // Default config file path
        static constexpr const char* DEFAULT_CONFIG_PATH = "./config/ATS-VS-editor-config.json";
    };
}
