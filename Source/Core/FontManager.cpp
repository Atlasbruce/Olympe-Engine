/**
 * @file FontManager.cpp
 * @brief Implementation of font management system
 * @author Olympe Engine
 * @date 2026
 */

#include "FontManager.h"
#include "IconsFontAwesome6.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <fstream>

namespace Olympe {

FontManager& FontManager::Get()
{
    static FontManager instance;
    return instance;
}

void FontManager::Initialize()
{
    if (m_Initialized) {
        return;
    }

    // Store default font
    m_DefaultFont = ImGui::GetFont();
    m_Initialized = true;

    SYSTEM_LOG << "[FontManager] Initialized" << std::endl;
}

bool FontManager::LoadFontAwesome(const std::string& fontPath, float fontSize)
{
    if (!m_Initialized) {
        SYSTEM_LOG << "[FontManager] ERROR: Not initialized. Call Initialize() first." << std::endl;
        return false;
    }

    // Check if file exists
    std::ifstream file(fontPath);
    if (!file.good()) {
        SYSTEM_LOG << "[FontManager] ERROR: Font file not found: " << fontPath << std::endl;
        return false;
    }
    file.close();

    // Get ImGui font atlas
    ImGuiIO& io = ImGui::GetIO();

    // Configure font with icon range
    static const ImWchar icons_ranges[] = { FONT_ICON_RANGE_FA_MIN, FONT_ICON_RANGE_FA_MAX, 0 };
    
    ImFontConfig config;
    config.MergeMode = false;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = fontSize; // Use fontSize as minimum width
    config.OversampleH = 1;
    config.OversampleV = 1;

    // Load Font Awesome as separate font
    m_FontAwesome = io.Fonts->AddFontFromFileTTF(
        fontPath.c_str(),
        fontSize,
        &config,
        icons_ranges
    );

    if (m_FontAwesome == nullptr) {
        SYSTEM_LOG << "[FontManager] ERROR: Failed to load Font Awesome from: " << fontPath << std::endl;
        return false;
    }

    // Rebuild font atlas
    io.Fonts->Build();

    SYSTEM_LOG << "[FontManager] Font Awesome loaded successfully from: " << fontPath << std::endl;
    return true;
}

ImFont* FontManager::LoadFont(const std::string& fontPath, float fontSize, const std::string& fontName)
{
    if (!m_Initialized) {
        SYSTEM_LOG << "[FontManager] ERROR: Not initialized. Call Initialize() first." << std::endl;
        return nullptr;
    }

    // Check if file exists
    std::ifstream file(fontPath);
    if (!file.good()) {
        SYSTEM_LOG << "[FontManager] ERROR: Font file not found: " << fontPath << std::endl;
        return nullptr;
    }
    file.close();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);

    if (font == nullptr) {
        SYSTEM_LOG << "[FontManager] ERROR: Failed to load font from: " << fontPath << std::endl;
        return nullptr;
    }

    // Rebuild font atlas
    io.Fonts->Build();

    std::string name = fontName.empty() ? fontPath : fontName;
    SYSTEM_LOG << "[FontManager] Font '" << name << "' loaded successfully" << std::endl;
    
    return font;
}

} // namespace Olympe
