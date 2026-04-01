/*
Olympus Game Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

purpose:

Notes:

*/

#include "system_utils.h"
#include "..\gameengine.h"
#include "..\PanelManager.h"

#include <windows.h>

void LoadOlympeConfig(const char* filename)
{
    GameEngine::screenWidth = DEFAULT_WINDOW_WIDTH;
    GameEngine::screenHeight = DEFAULT_WINDOW_HEIGHT;

    std::ifstream ifs(filename);
    if (!ifs) {
        SYSTEM_LOG << "Config file '" << filename << "' not found \xe2\x80\x93 using defaults " << GameEngine::screenWidth << "x" << GameEngine::screenHeight << "\n";
        return;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

	// Extract screen width and height
    int w = GameEngine::screenWidth;
    int h = GameEngine::screenHeight;

    if (extract_json_int(content, "screen_width", w) || extract_json_int(content, "screenWidth", w) || extract_json_int(content, "width", w)) {}
    if (extract_json_int(content, "screen_height", h) || extract_json_int(content, "screenHeight", h) || extract_json_int(content, "screen_heigth", h) || extract_json_int(content, "height", h)) {}

	if (w > 0) GameEngine::screenWidth = w;
	if (h > 0) GameEngine::screenHeight = h;

	// Phase 26 — Simplified logging: output to stdout, file, and panel always
	// (no configurable output channels)

	SYSTEM_LOG << "Config loaded from '" << filename << "': " << GameEngine::screenWidth << "x" << GameEngine::screenHeight << "\n";

	// Extract Log Panel data
	if (extract_json_int(content, "log_panel_width", PanelManager::LogPanelWidth)) {}
    if (extract_json_int(content, "log_panel_height", PanelManager::LogPanelHeight)) {}
	if (extract_json_int(content, "log_panel_posx", PanelManager::LogPanelPosX)) {}
	if (extract_json_int(content, "log_panel_posy", PanelManager::LogPanelPosY)) {}

	// Extract Object Inspector Panel data
	if (extract_json_int(content, "inspector_panel_width", PanelManager::InspectorPanelWidth)) {}
    if (extract_json_int(content, "inspector_panel_height", PanelManager::InspectorPanelHeight)) {}
	if (extract_json_int(content, "inspector_panel_posx", PanelManager::InspectorPanelPosX)) {}
	if (extract_json_int(content, "inspector_panel_posy", PanelManager::InspectorPanelPosY)) {}

	// Extract Tree View Panel data
	if (extract_json_int(content, "treeview_panel_width", PanelManager::TreeViewPanelWidth)) {}
    if (extract_json_int(content, "treeview_panel_height", PanelManager::TreeViewPanelHeight)) {}
	if (extract_json_int(content, "treeview_panel_posx", PanelManager::TreeViewPanelPosX)) {}
	if (extract_json_int(content, "treeview_panel_posy", PanelManager::TreeViewPanelPosY)) {}

	  /*  "object_panel_width" : 200,
		"object_panel_height" : 400,
		"object_panel_posx" : 700,
		"object_panel_posy" : 0,

		"treeview_panel_width" : 300,
		"treeview_panel_height" : 600,
		"treeview_panel_posx" : 700,
		"treeview_panel_posy" : 400,/**/
}

//-------------------------------------------------------------
// Path resolution for resource files (Phase 24 - Condition Presets)
// Resolves relative paths to absolute paths based on executable location.
// This works in both IDE debug mode (working dir = solution subdir) and 
// built executable mode (working dir = parent dir).
//-------------------------------------------------------------
std::string ResolveResourcePath(const std::string& relativePath)
{
	// Normalize the relative path: convert forward slashes to backslashes
	std::string normPath = relativePath;
	for (size_t i = 0; i < normPath.size(); ++i)
	{
		if (normPath[i] == '/') normPath[i] = '\\';
	}

	// Get the executable path (OlympeBlueprintEditor.exe)
	char exePath[MAX_PATH] = { 0 };
	if (GetModuleFileNameA(nullptr, exePath, MAX_PATH) == 0)
	{
		// Fallback: return the normalized relative path as-is if we can't get the executable path
		return normPath;
	}

	// Get the directory containing the executable
	std::string exeDir(exePath);
	size_t lastSlash = exeDir.find_last_of("\\/");
	if (lastSlash != std::string::npos)
	{
		exeDir = exeDir.substr(0, lastSlash);
	}
	else
	{
		// Fallback: return the normalized relative path as-is
		return normPath;
	}

	// Construct the absolute path by combining executable directory with relative path
	// Try multiple locations to handle both IDE debug and built executable scenarios:
	// 1. exeDir + relative (built executable, or when running from bin/ directly)
	// 2. parent of exeDir + relative (IDE debug, executable is in OlympeBlueprintEditor/bin/)
	// 3. grandparent of exeDir + relative (IDE debug, executable is deeper)

	// Try the direct path first (built executable case)
	std::string absolutePath = exeDir + "\\" + normPath;
	if (GetFileAttributesA(absolutePath.c_str()) != INVALID_FILE_ATTRIBUTES)
	{
		return absolutePath;
	}

	// If not found, try going up one directory level (IDE debug case)
	size_t parentSlash = exeDir.find_last_of("\\/");
	if (parentSlash != std::string::npos)
	{
		std::string parentDir = exeDir.substr(0, parentSlash);
		std::string pathAtParent = parentDir + "\\" + normPath;
		if (GetFileAttributesA(pathAtParent.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			return pathAtParent;
		}

		// Try another level up (grandparent)
		size_t grandparentSlash = parentDir.find_last_of("\\/");
		if (grandparentSlash != std::string::npos)
		{
			std::string grandparentDir = parentDir.substr(0, grandparentSlash);
			std::string pathAtGrandparent = grandparentDir + "\\" + normPath;
			if (GetFileAttributesA(pathAtGrandparent.c_str()) != INVALID_FILE_ATTRIBUTES)
			{
				return pathAtGrandparent;
			}
		}
	}

	// If file not found anywhere, return the executable directory + relative path
	// This will cause a load failure later with a meaningful path for debugging
	SYSTEM_LOG << "[ResolveResourcePath] Warning: Could not find resource '" << normPath 
			   << "' relative to executable. Returning path relative to exe dir.\n";
	return exeDir + "\\" + normPath;
}

