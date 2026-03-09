/*
 * Olympe Blueprint Editor - Configuration Manager Implementation
 */

#include "EditorConfigManager.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/imgui_internal.h"
#include "../third_party/imnodes/imnodes.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace Olympe
{
    EditorConfigManager& EditorConfigManager::Get()
    {
        static EditorConfigManager instance;
        return instance;
    }

    bool EditorConfigManager::LoadConfig(const std::string& filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                std::cout << "[EditorConfigManager] Config file not found, using defaults: " << filepath << std::endl;
                return false;
            }

            json j;
            file >> j;
            file.close();

            // Load panel visibility
            if (j.contains("panelVisibility"))
            {
                auto& pv = j["panelVisibility"];
                m_PanelVisibility.showAssetBrowser = pv.value("showAssetBrowser", true);
                m_PanelVisibility.showInspector = pv.value("showInspector", true);
                m_PanelVisibility.showNodeGraph = pv.value("showNodeGraph", true);
                m_PanelVisibility.showTemplateBrowser = pv.value("showTemplateBrowser", false);
                m_PanelVisibility.showHistory = pv.value("showHistory", false);
            }

            // Load editor preferences
            if (j.contains("preferences"))
            {
                auto& prefs = j["preferences"];
                m_Preferences.gridLinesEnabled = prefs.value("gridLinesEnabled", true);
                m_Preferences.autoSaveEnabled = prefs.value("autoSaveEnabled", true);
                m_Preferences.autoSaveIntervalSeconds = prefs.value("autoSaveIntervalSeconds", 300);
                m_Preferences.panningSpeed = prefs.value("panningSpeed", 1.0f);
                m_Preferences.zoomSpeed = prefs.value("zoomSpeed", 0.1f);
            }

            // Load ImGui window configurations
            if (j.contains("imguiSettings") && j["imguiSettings"].contains("windows"))
            {
                m_ImGuiSettings.windowConfigs.clear();
                json windows = j["imguiSettings"]["windows"];

                for (json::iterator it = windows.begin(); it != windows.end(); ++it)
                {
                    std::string key = it.key();
                    json value = it.value();

                    WindowConfig config;
                    config.x = value.value("x", 0.0f);
                    config.y = value.value("y", 0.0f);
                    config.width = value.value("width", 800.0f);
                    config.height = value.value("height", 600.0f);
                    config.collapsed = value.value("collapsed", false);

                    m_ImGuiSettings.windowConfigs[key] = config;
                }
            }

            std::cout << "[EditorConfigManager] Configuration loaded from: " << filepath << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[EditorConfigManager] Failed to load config: " << e.what() << std::endl;
            return false;
        }
    }

    bool EditorConfigManager::SaveConfig(const std::string& filepath)
    {
        try
        {
            json j;

            // Save panel visibility
            j["panelVisibility"] = {
                {"showAssetBrowser", m_PanelVisibility.showAssetBrowser},
                {"showInspector", m_PanelVisibility.showInspector},
                {"showNodeGraph", m_PanelVisibility.showNodeGraph},
                {"showTemplateBrowser", m_PanelVisibility.showTemplateBrowser},
                {"showHistory", m_PanelVisibility.showHistory}
            };

            // Save editor preferences
            j["preferences"] = {
                {"gridLinesEnabled", m_Preferences.gridLinesEnabled},
                {"autoSaveEnabled", m_Preferences.autoSaveEnabled},
                {"autoSaveIntervalSeconds", m_Preferences.autoSaveIntervalSeconds},
                {"panningSpeed", m_Preferences.panningSpeed},
                {"zoomSpeed", m_Preferences.zoomSpeed}
            };

            // Save ImGui window configurations
            json windowsJson = json::object();
            for (std::map<std::string, WindowConfig>::const_iterator it = m_ImGuiSettings.windowConfigs.begin();
                 it != m_ImGuiSettings.windowConfigs.end(); ++it)
            {
                const std::string& name = it->first;
                const WindowConfig& config = it->second;

                json windowJson = json::object();
                windowJson["x"] = config.x;
                windowJson["y"] = config.y;
                windowJson["width"] = config.width;
                windowJson["height"] = config.height;
                windowJson["collapsed"] = config.collapsed;

                windowsJson[name] = windowJson;
            }
            j["imguiSettings"]["windows"] = windowsJson;

            // Write to file with formatting
            std::ofstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "[EditorConfigManager] Failed to open config file for writing: " << filepath << std::endl;
                return false;
            }

            file << j.dump(2); // Pretty print with 2 space indentation
            file.close();

            std::cout << "[EditorConfigManager] Configuration saved to: " << filepath << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[EditorConfigManager] Failed to save config: " << e.what() << std::endl;
            return false;
        }
    }

    void EditorConfigManager::SaveWindowConfig(const std::string& windowName, const WindowConfig& config)
    {
        m_ImGuiSettings.windowConfigs[windowName] = config;
    }

    WindowConfig EditorConfigManager::GetWindowConfig(const std::string& windowName) const
    {
        auto it = m_ImGuiSettings.windowConfigs.find(windowName);
        if (it != m_ImGuiSettings.windowConfigs.end())
        {
            return it->second;
        }
        return WindowConfig(); // Return default
    }

    void EditorConfigManager::ApplyToImGui()
    {
        // Apply ImNodes grid settings
        if (m_Preferences.gridLinesEnabled)
        {
            ImNodesStyle& style = ImNodes::GetStyle();
            style.Flags |= ImNodesStyleFlags_GridLines;
        }
        else
        {
            ImNodesStyle& style = ImNodes::GetStyle();
            style.Flags &= ~ImNodesStyleFlags_GridLines;
        }

        // Note: Window positions/sizes will be applied when windows are created
        // by checking GetWindowConfig() for each window
    }

    void EditorConfigManager::CaptureFromImGui()
    {
        // Capture all open ImGui window positions and sizes
        ImGuiContext* context = ImGui::GetCurrentContext();
        if (!context)
            return;

        for (int i = 0; i < context->Windows.Size; i++)
        {
            ImGuiWindow* window = context->Windows[i];
            if (!window || window->Hidden)
                continue;

            // Skip internal ImGui windows (those starting with ##)
            std::string windowName = window->Name;
            if (windowName.empty() || windowName[0] == '#')
                continue;

            WindowConfig config;
            config.x = window->Pos.x;
            config.y = window->Pos.y;
            config.width = window->Size.x;
            config.height = window->Size.y;
            config.collapsed = window->Collapsed;

            SaveWindowConfig(windowName, config);
        }
    }
}
