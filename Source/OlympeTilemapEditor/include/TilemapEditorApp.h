/*
 * Olympe Tilemap Editor - Main Application
 * 
 * Main application class that integrates SDL3 and ImGui for the tilemap editor.
 * Handles initialization, main loop, rendering, and event processing.
 */

#ifndef OLYMPE_EDITOR_TILEMAPEDITORAPP_H
#define OLYMPE_EDITOR_TILEMAPEDITORAPP_H

#include <string>
#include <memory>
#include <SDL3/SDL.h>
#include "LevelManager.h"
#include "EditorState.h"
#include "AssetManager.h"
#include "TilesetManager.h"

namespace Olympe {
namespace Editor {

/**
 * Application configuration structure
 */
struct AppConfig {
    std::string window_title;
    int window_width;
    int window_height;
    bool window_resizable;
    bool window_maximized;
    bool vsync_enabled;
    std::string assets_directory;
    std::string levels_directory;
    std::string tilesets_directory;
};

/**
 * TilemapEditorApp class
 * Main application class for the tilemap editor
 */
class TilemapEditorApp {
public:
    TilemapEditorApp();
    ~TilemapEditorApp();

    // Application lifecycle
    bool initialize(const AppConfig& config);
    void run();
    void shutdown();
    
    // Main loop control
    bool is_running() const;
    void request_quit();
    
    // Configuration
    const AppConfig& get_config() const;
    void set_config(const AppConfig& config);
    
    // Manager access
    LevelManager& get_level_manager();
    EditorState& get_editor_state();
    AssetManager& get_asset_manager();
    TilesetManager& get_tileset_manager();
    
    // SDL access
    SDL_Window* get_window() const;
    SDL_Renderer* get_renderer() const;
    
    // Time management
    float get_delta_time() const;
    float get_total_time() const;
    int get_fps() const;
    
    // UI state
    bool is_ui_hovered() const;
    bool is_ui_focused() const;

private:
    // SDL objects
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    
    // Application state
    bool is_running_;
    bool is_initialized_;
    AppConfig config_;
    
    // Managers
    std::unique_ptr<LevelManager> level_manager_;
    std::unique_ptr<EditorState> editor_state_;
    std::unique_ptr<AssetManager> asset_manager_;
    std::unique_ptr<TilesetManager> tileset_manager_;
    
    // Timing
    Uint64 last_frame_time_;
    float delta_time_;
    float total_time_;
    int frame_count_;
    float fps_update_timer_;
    int current_fps_;
    
    // Initialization
    bool initialize_sdl();
    bool initialize_imgui();
    bool initialize_managers();
    
    // Main loop functions
    void process_events();
    void update(float delta_time);
    void render();
    
    // Event handling
    void handle_event(const SDL_Event& event);
    void handle_keyboard_event(const SDL_KeyboardEvent& event);
    void handle_mouse_event(const SDL_MouseButtonEvent& event);
    void handle_mouse_motion(const SDL_MouseMotionEvent& event);
    void handle_mouse_wheel(const SDL_MouseWheelEvent& event);
    void handle_window_event(const SDL_WindowEvent& event);
    
    // UI rendering
    void render_main_menu_bar();
    void render_toolbar();
    void render_tileset_panel();
    void render_tile_groups_panel();
    void render_tile_list_panel();
    void render_canvas();
    void render_layer_panel();
    void render_properties_panel();
    void render_status_bar();
    
    // UI dialogs
    void show_new_level_dialog();
    void show_open_level_dialog();
    void show_save_level_dialog();
    void show_settings_dialog();
    void show_about_dialog();
    
    // Helper functions
    void update_window_title();
    void handle_unsaved_changes();
    void calculate_fps();
    void apply_editor_shortcuts();
    
    // Canvas rendering
    void render_grid();
    void render_tiles();
    void render_selection();
    void render_cursor();
    
    // Tool operations
    void apply_brush_tool(int tile_x, int tile_y);
    void apply_eraser_tool(int tile_x, int tile_y);
    void apply_fill_tool(int tile_x, int tile_y);
    void apply_rectangle_tool(int start_x, int start_y, int end_x, int end_y);
    void apply_eyedropper_tool(int tile_x, int tile_y);
    
    // Coordinate conversion
    void screen_to_world(int screen_x, int screen_y, float& out_world_x, float& out_world_y) const;
    void world_to_screen(float world_x, float world_y, int& out_screen_x, int& out_screen_y) const;
    void world_to_tile(float world_x, float world_y, int& out_tile_x, int& out_tile_y) const;
    void tile_to_world(int tile_x, int tile_y, float& out_world_x, float& out_world_y) const;
};

} // namespace Editor
} // namespace Olympe

#endif // OLYMPE_EDITOR_TILEMAPEDITORAPP_H
