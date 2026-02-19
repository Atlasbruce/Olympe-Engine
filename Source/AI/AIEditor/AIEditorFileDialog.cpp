/**
 * @file AIEditorFileDialog.cpp
 * @brief Implementation of AIEditorFileDialog
 * @author Olympe Engine
 * @date 2026-02-19
 */

#include "AIEditorFileDialog.h"
#include "../../system/system_utils.h"
#include "../../third_party/nfd/nfd.hpp"
#include <vector>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Olympe {
namespace AI {

// ============================================================================
// Platform-specific string conversion
// ============================================================================

#ifdef _WIN32
std::wstring AIEditorFileDialog::ToNFDString(const std::string& str)
{
    if (str.empty()) {
        return std::wstring();
    }
    
    // Convert UTF-8 to UTF-16 (Windows wide string)
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (wideSize == 0) {
        return std::wstring();
    }
    
    std::wstring wideStr(static_cast<size_t>(wideSize), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideStr[0], wideSize);
    
    // Remove trailing null character
    if (!wideStr.empty() && wideStr[wideStr.size() - 1] == L'\0') {
        wideStr.resize(wideStr.size() - 1);
    }
    
    return wideStr;
}

std::string AIEditorFileDialog::FromNFDString(const wchar_t* wstr)
{
    if (wstr == nullptr) {
        return std::string();
    }
    
    // Convert UTF-16 to UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0) {
        return std::string();
    }
    
    std::string utf8Str(static_cast<size_t>(utf8Size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &utf8Str[0], utf8Size, nullptr, nullptr);
    
    // Remove trailing null character
    if (!utf8Str.empty() && utf8Str[utf8Str.size() - 1] == '\0') {
        utf8Str.resize(utf8Str.size() - 1);
    }
    
    return utf8Str;
}
#else
std::string AIEditorFileDialog::ToNFDString(const std::string& str)
{
    return str;
}

std::string AIEditorFileDialog::FromNFDString(const char* str)
{
    if (str == nullptr) {
        return std::string();
    }
    return std::string(str);
}
#endif

// ============================================================================
// Filter conversion
// ============================================================================

std::string AIEditorFileDialog::ConvertFiltersToNFD(const std::string& filterList)
{
    // Convert comma-separated list to semicolon-separated
    // "json,btree" -> "json;btree"
    std::string result = filterList;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == ',') {
            result[i] = ';';
        }
    }
    return result;
}

// ============================================================================
// File Dialog Methods
// ============================================================================

std::string AIEditorFileDialog::OpenFile(
    const std::string& filterList,
    const std::string& defaultPath)
{
    // Initialize NFD
    nfdresult_t initResult = NFD::Init();
    if (initResult != NFD_OKAY) {
        SYSTEM_LOG << "[AIEditorFileDialog] ERROR: Failed to initialize NFD" << std::endl;
        return std::string();
    }
    
    // Convert filter list
    std::string nfdFilters = ConvertFiltersToNFD(filterList);
    
    // Create filter items and persistent strings
    std::vector<nfdnfilteritem_t> filterItems;
#ifdef _WIN32
    std::wstring filterName;
    std::wstring filterSpec;
#else
    std::string filterSpec;
#endif
    
    if (!nfdFilters.empty()) {
        nfdnfilteritem_t item;
#ifdef _WIN32
        filterName = L"AI Graphs";
        filterSpec = ToNFDString(nfdFilters);
        item.name = filterName.c_str();
        item.spec = filterSpec.c_str();
#else
        item.name = "AI Graphs";
        filterSpec = nfdFilters;
        item.spec = filterSpec.c_str();
#endif
        filterItems.push_back(item);
    }
    
    // Convert default path
#ifdef _WIN32
    std::wstring wDefaultPath = ToNFDString(defaultPath);
    const wchar_t* defaultPathPtr = wDefaultPath.empty() ? nullptr : wDefaultPath.c_str();
#else
    const char* defaultPathPtr = defaultPath.empty() ? nullptr : defaultPath.c_str();
#endif
    
    // Open dialog
    nfdnchar_t* outPath = nullptr;
    nfdresult_t result = NFD::OpenDialog(
        outPath,
        filterItems.empty() ? nullptr : filterItems.data(),
        static_cast<nfdfiltersize_t>(filterItems.size()),
        defaultPathPtr
    );
    
    std::string selectedPath;
    
    if (result == NFD_OKAY) {
        // Convert result to std::string
        selectedPath = FromNFDString(outPath);
        NFD::FreePath(outPath);
        
        SYSTEM_LOG << "[AIEditorFileDialog] Selected file: " << selectedPath << std::endl;
    }
    else if (result == NFD_CANCEL) {
        // User canceled - this is normal, no need to log
    }
    else {
        const char* error = NFD::GetError();
        SYSTEM_LOG << "[AIEditorFileDialog] ERROR: " << (error ? error : "Unknown error") << std::endl;
    }
    
    // Cleanup NFD
    NFD::Quit();
    
    return selectedPath;
}

std::string AIEditorFileDialog::SaveFile(
    const std::string& filterList,
    const std::string& defaultPath,
    const std::string& defaultName)
{
    // Initialize NFD
    nfdresult_t initResult = NFD::Init();
    if (initResult != NFD_OKAY) {
        SYSTEM_LOG << "[AIEditorFileDialog] ERROR: Failed to initialize NFD" << std::endl;
        return std::string();
    }
    
    // Convert filter list
    std::string nfdFilters = ConvertFiltersToNFD(filterList);
    
    // Create filter items and persistent strings
    std::vector<nfdnfilteritem_t> filterItems;
#ifdef _WIN32
    std::wstring filterName;
    std::wstring filterSpec;
#else
    std::string filterSpec;
#endif
    
    if (!nfdFilters.empty()) {
        nfdnfilteritem_t item;
#ifdef _WIN32
        filterName = L"AI Graphs";
        filterSpec = ToNFDString(nfdFilters);
        item.name = filterName.c_str();
        item.spec = filterSpec.c_str();
#else
        item.name = "AI Graphs";
        filterSpec = nfdFilters;
        item.spec = filterSpec.c_str();
#endif
        filterItems.push_back(item);
    }
    
    // Convert default path and name
#ifdef _WIN32
    std::wstring wDefaultPath = ToNFDString(defaultPath);
    std::wstring wDefaultName = ToNFDString(defaultName);
    const wchar_t* defaultPathPtr = wDefaultPath.empty() ? nullptr : wDefaultPath.c_str();
    const wchar_t* defaultNamePtr = wDefaultName.empty() ? nullptr : wDefaultName.c_str();
#else
    const char* defaultPathPtr = defaultPath.empty() ? nullptr : defaultPath.c_str();
    const char* defaultNamePtr = defaultName.empty() ? nullptr : defaultName.c_str();
#endif
    
    // Open dialog
    nfdnchar_t* outPath = nullptr;
    nfdresult_t result = NFD::SaveDialog(
        outPath,
        filterItems.empty() ? nullptr : filterItems.data(),
        static_cast<nfdfiltersize_t>(filterItems.size()),
        defaultPathPtr,
        defaultNamePtr
    );
    
    std::string selectedPath;
    
    if (result == NFD_OKAY) {
        // Convert result to std::string
        selectedPath = FromNFDString(outPath);
        NFD::FreePath(outPath);
        
        SYSTEM_LOG << "[AIEditorFileDialog] Save to: " << selectedPath << std::endl;
    }
    else if (result == NFD_CANCEL) {
        // User canceled - this is normal, no need to log
    }
    else {
        const char* error = NFD::GetError();
        SYSTEM_LOG << "[AIEditorFileDialog] ERROR: " << (error ? error : "Unknown error") << std::endl;
    }
    
    // Cleanup NFD
    NFD::Quit();
    
    return selectedPath;
}

std::string AIEditorFileDialog::OpenFolder(const std::string& defaultPath)
{
    // Initialize NFD
    nfdresult_t initResult = NFD::Init();
    if (initResult != NFD_OKAY) {
        SYSTEM_LOG << "[AIEditorFileDialog] ERROR: Failed to initialize NFD" << std::endl;
        return std::string();
    }
    
    // Convert default path
#ifdef _WIN32
    std::wstring wDefaultPath = ToNFDString(defaultPath);
    const wchar_t* defaultPathPtr = wDefaultPath.empty() ? nullptr : wDefaultPath.c_str();
#else
    const char* defaultPathPtr = defaultPath.empty() ? nullptr : defaultPath.c_str();
#endif
    
    // Open dialog
    nfdnchar_t* outPath = nullptr;
    nfdresult_t result = NFD::PickFolder(outPath, defaultPathPtr);
    
    std::string selectedPath;
    
    if (result == NFD_OKAY) {
        // Convert result to std::string
        selectedPath = FromNFDString(outPath);
        NFD::FreePath(outPath);
        
        SYSTEM_LOG << "[AIEditorFileDialog] Selected folder: " << selectedPath << std::endl;
    }
    else if (result == NFD_CANCEL) {
        // User canceled - this is normal, no need to log
    }
    else {
        const char* error = NFD::GetError();
        SYSTEM_LOG << "[AIEditorFileDialog] ERROR: " << (error ? error : "Unknown error") << std::endl;
    }
    
    // Cleanup NFD
    NFD::Quit();
    
    return selectedPath;
}

std::string AIEditorFileDialog::GetLastError()
{
    const char* error = NFD::GetError();
    if (error != nullptr) {
        return std::string(error);
    }
    return std::string();
}

} // namespace AI
} // namespace Olympe
