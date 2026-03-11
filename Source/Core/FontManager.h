/**
 * @file FontManager.h
 * @brief Font management system for ImGui
 * @author Olympe Engine
 * @date 2026
 * 
 * @details
 * Manages loading and accessing custom fonts including icon fonts like Font Awesome
 */

#pragma once

#include <string>

// Forward declare ImFont
struct ImFont;

namespace Olympe {

/**
 * @class FontManager
 * @brief Singleton manager for custom fonts
 */
class FontManager {
public:
    /**
     * @brief Get singleton instance
     */
    static FontManager& Get();

    /**
     * @brief Initialize font system
     * Must be called after ImGui context creation but before first frame
     */
    void Initialize();

    /**
     * @brief Load Font Awesome font
     * @param fontPath Path to Font Awesome .otf file
     * @param fontSize Base font size (default: 16.0f)
     * @return true if loaded successfully
     */
    bool LoadFontAwesome(const std::string& fontPath, float fontSize = 16.0f);

    /**
     * @brief Load a custom font
     * @param fontPath Path to font file
     * @param fontSize Font size
     * @param fontName Optional name identifier
     * @return Pointer to loaded font, nullptr on failure
     */
    ImFont* LoadFont(const std::string& fontPath, float fontSize, const std::string& fontName = "");

    /**
     * @brief Get Font Awesome font
     * @return Pointer to Font Awesome font, nullptr if not loaded
     */
    ImFont* GetFontAwesome() const { return m_FontAwesome; }

    /**
     * @brief Get default font
     * @return Pointer to default ImGui font
     */
    ImFont* GetDefaultFont() const { return m_DefaultFont; }

    /**
     * @brief Check if Font Awesome is loaded
     */
    bool IsFontAwesomeLoaded() const { return m_FontAwesome != nullptr; }

private:
    FontManager() : m_FontAwesome(nullptr), m_DefaultFont(nullptr), m_Initialized(false) {}
    ~FontManager() = default;
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    ImFont* m_FontAwesome;
    ImFont* m_DefaultFont;
    bool m_Initialized;
};

} // namespace Olympe
