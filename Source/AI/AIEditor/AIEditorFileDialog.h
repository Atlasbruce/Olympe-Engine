/**
 * @file AIEditorFileDialog.h
 * @brief Native file dialog wrapper for AI Editor (Phase 1.5)
 * @author Olympe Engine
 * @date 2026-02-19
 *
 * @details
 * Provides a simple C++ interface to nativefiledialog-extended library
 * for opening native OS file dialogs (Windows Explorer, Linux GTK/Qt, macOS Finder).
 */

#pragma once

#include <string>

namespace Olympe {
namespace AI {

// Default file extensions for AI graphs
const char* const DEFAULT_AI_GRAPH_FILTER = "json,btree";
const char* const DEFAULT_AI_GRAPH_NAME = "new_ai_graph.json";
const char* const DEFAULT_AI_GRAPH_EXT = ".json";

/**
 * @class AIEditorFileDialog
 * @brief Wrapper for native file dialog operations
 *
 * @details
 * Encapsulates NFD (NativeFileDialog-Extended) API calls to provide
 * a simple C++ interface for opening, saving, and folder selection dialogs.
 * All methods are static and thread-safe (NFD handles thread initialization).
 */
class AIEditorFileDialog
{
public:
    /**
     * @brief Open a native file dialog to select an existing file
     * @param filterList Comma-separated extensions (e.g., "json,btree")
     * @param defaultPath Starting directory (optional)
     * @return Selected file path or empty string if canceled
     */
    static std::string OpenFile(
        const std::string& filterList = "json,btree",
        const std::string& defaultPath = ""
    );
    
    /**
     * @brief Open a native save file dialog
     * @param filterList Comma-separated extensions
     * @param defaultPath Starting directory (optional)
     * @param defaultName Suggested filename (optional)
     * @return Selected save path or empty string if canceled
     */
    static std::string SaveFile(
        const std::string& filterList = "json,btree",
        const std::string& defaultPath = "",
        const std::string& defaultName = "new_ai_graph.json"
    );
    
    /**
     * @brief Open a native folder selection dialog
     * @param defaultPath Starting directory (optional)
     * @return Selected folder path or empty string if canceled
     */
    static std::string OpenFolder(const std::string& defaultPath = "");
    
    /**
     * @brief Get last error message from NFD
     * @return Error string or empty if no error
     */
    static std::string GetLastError();
    
private:
    /**
     * @brief Convert comma-separated filter list to NFD format
     * @param filterList "json,btree" -> "json;btree"
     * @return Converted filter string for NFD
     */
    static std::string ConvertFiltersToNFD(const std::string& filterList);
    
    /**
     * @brief Convert std::string to native NFD character type
     * @param str UTF-8 string
     * @return Platform-specific wide string (Windows) or UTF-8 (Linux/Mac)
     */
#ifdef _WIN32
    static std::wstring ToNFDString(const std::string& str);
    static std::string FromNFDString(const wchar_t* wstr);
#else
    static std::string ToNFDString(const std::string& str);
    static std::string FromNFDString(const char* str);
#endif
};

} // namespace AI
} // namespace Olympe
