/*
 * Olympe Tilemap Editor - Main Application Implementation
 */

#include "../include/TilemapEditorApp.h"
#include "../../../Source/third_party/imgui/imgui.h"
#include "../../../Source/third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../../../Source/third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include <iostream>

namespace Olympe {
namespace Editor {

TilemapEditorApp::TilemapEditorApp()
    : window_(nullptr)
    , renderer_(nullptr)
    , is_running_(false)
    , is_initialized_(false)
    , last_frame_time_(0)
    , delta_time_(0.0f)
    , total_time_(0.0f)
    , frame_count_(0)
    , fps_update_timer_(0.0f)
    , current_fps_(0)
{
    std::cout << "[TilemapEditorApp] Constructor called" << std::endl;
}

TilemapEditorApp::~TilemapEditorApp()
{
    if (is_initialized_) {
        shutdown();
    }
    std::cout << "[TilemapEditorApp] Destructor called" << std::endl;
}

bool TilemapEditorApp::initialize(const AppConfig& config)
{
    std::cout << "[TilemapEditorApp] Initializing..." << std::endl;
    config_ = config;
    
    if (!initialize_sdl()) {
        std::cerr << "[TilemapEditorApp] Error: SDL initialization failed" << std::endl;
        return false;
    }
    
    if (!initialize_imgui()) {
        std::cerr << "[TilemapEditorApp] Error: ImGui initialization failed" << std::endl;
        return false;
    }
    
    if (!initialize_managers()) {
        std::cerr << "[TilemapEditorApp] Error: Managers initialization failed" << std::endl;
        return false;
    }
    
    is_initialized_ = true;
    last_frame_time_ = SDL_GetTicks();
    
    std::cout << "[TilemapEditorApp] Initialization complete" << std::endl;
    return true;
}

void TilemapEditorApp::run()
{
    if (!is_initialized_) {
        std::cerr << "[TilemapEditorApp] Error: Application not initialized" << std::endl;
        return;
    }
    
    std::cout << "[TilemapEditorApp] Starting main loop..." << std::endl;
    is_running_ = true;
    
    while (is_running_) {
        Uint64 current_time = SDL_GetTicks();
        delta_time_ = static_cast<float>(current_time - last_frame_time_) / 1000.0f;
        last_frame_time_ = current_time;
        
        // Clamp delta time
        if (delta_time_ > 0.1f) {
            delta_time_ = 0.016f; // ~60 FPS
        }
        
        total_time_ += delta_time_;
        
        process_events();
        update(delta_time_);
        render();
        
        calculate_fps();
    }
    
    std::cout << "[TilemapEditorApp] Main loop ended" << std::endl;
}

void TilemapEditorApp::shutdown()
{
    std::cout << "[TilemapEditorApp] Shutting down..." << std::endl;
    
    // Shutdown managers
    level_manager_.reset();
    editor_state_.reset();
    asset_manager_.reset();
    tileset_manager_.reset();
    
    // Shutdown ImGui
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    
    // Shutdown SDL
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
    
    is_initialized_ = false;
    std::cout << "[TilemapEditorApp] Shutdown complete" << std::endl;
}

bool TilemapEditorApp::is_running() const
{
    return is_running_;
}

void TilemapEditorApp::request_quit()
{
    std::cout << "[TilemapEditorApp] Quit requested" << std::endl;
    handle_unsaved_changes();
    is_running_ = false;
}

const AppConfig& TilemapEditorApp::get_config() const
{
    return config_;
}

void TilemapEditorApp::set_config(const AppConfig& config)
{
    config_ = config;
}

LevelManager& TilemapEditorApp::get_level_manager()
{
    return *level_manager_;
}

EditorState& TilemapEditorApp::get_editor_state()
{
    return *editor_state_;
}

AssetManager& TilemapEditorApp::get_asset_manager()
{
    return *asset_manager_;
}

TilesetManager& TilemapEditorApp::get_tileset_manager()
{
    return *tileset_manager_;
}

SDL_Window* TilemapEditorApp::get_window() const
{
    return window_;
}

SDL_Renderer* TilemapEditorApp::get_renderer() const
{
    return renderer_;
}

float TilemapEditorApp::get_delta_time() const
{
    return delta_time_;
}

float TilemapEditorApp::get_total_time() const
{
    return total_time_;
}

int TilemapEditorApp::get_fps() const
{
    return current_fps_;
}

bool TilemapEditorApp::is_ui_hovered() const
{
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

bool TilemapEditorApp::is_ui_focused() const
{
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
}

bool TilemapEditorApp::initialize_sdl()
{
    std::cout << "[TilemapEditorApp] Initializing SDL3..." << std::endl;
    
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "[TilemapEditorApp] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    window_ = SDL_CreateWindow(
        config_.window_title.c_str(),
        config_.window_width,
        config_.window_height,
        SDL_WINDOW_RESIZABLE | (config_.window_maximized ? SDL_WINDOW_MAXIMIZED : 0)
    );
    
    if (!window_) {
        std::cerr << "[TilemapEditorApp] Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        std::cerr << "[TilemapEditorApp] Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    if (config_.vsync_enabled) {
        SDL_SetRenderVSync(renderer_, 1);
    }
    
    std::cout << "[TilemapEditorApp] SDL3 initialized successfully" << std::endl;
    return true;
}

bool TilemapEditorApp::initialize_imgui()
{
    std::cout << "[TilemapEditorApp] Initializing ImGui..." << std::endl;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    // Customize style
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.40f, 0.60f, 1.00f);
    
    ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer3_Init(renderer_);
    
    std::cout << "[TilemapEditorApp] ImGui initialized successfully" << std::endl;
    return true;
}

bool TilemapEditorApp::initialize_managers()
{
    std::cout << "[TilemapEditorApp] Initializing managers..." << std::endl;
    
    level_manager_ = std::make_unique<LevelManager>();
    editor_state_ = std::make_unique<EditorState>();
    asset_manager_ = std::make_unique<AssetManager>(renderer_);
    tileset_manager_ = std::make_unique<TilesetManager>();
    
    std::cout << "[TilemapEditorApp] Managers initialized successfully" << std::endl;
    return true;
}

void TilemapEditorApp::process_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        handle_event(event);
    }
}

void TilemapEditorApp::update(float delta_time)
{
    // TODO: Update logic
    apply_editor_shortcuts();
}

void TilemapEditorApp::render()
{
    // Start ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    // Render UI
    render_main_menu_bar();
    render_toolbar();
    render_tileset_panel();
    render_canvas();
    render_layer_panel();
    render_properties_panel();
    render_status_bar();
    
    // Finish ImGui frame
    ImGui::Render();
    
    // Clear screen
    SDL_SetRenderDrawColor(renderer_, 30, 30, 34, 255);
    SDL_RenderClear(renderer_);
    
    // Render ImGui
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
    
    SDL_RenderPresent(renderer_);
}

void TilemapEditorApp::handle_event(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_EVENT_QUIT:
            request_quit();
            break;
        case SDL_EVENT_KEY_DOWN:
            handle_keyboard_event(event.key);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            handle_mouse_event(event.button);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            handle_mouse_motion(event.motion);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            handle_mouse_wheel(event.wheel);
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            request_quit();
            break;
    }
}

void TilemapEditorApp::handle_keyboard_event(const SDL_KeyboardEvent& event)
{
    // TODO: Handle keyboard events
}

void TilemapEditorApp::handle_mouse_event(const SDL_MouseButtonEvent& event)
{
    // TODO: Handle mouse button events
}

void TilemapEditorApp::handle_mouse_motion(const SDL_MouseMotionEvent& event)
{
    // TODO: Handle mouse motion events
}

void TilemapEditorApp::handle_mouse_wheel(const SDL_MouseWheelEvent& event)
{
    // TODO: Handle mouse wheel events for zooming
}

void TilemapEditorApp::handle_window_event(const SDL_WindowEvent& event)
{
    // TODO: Handle window events
}

void TilemapEditorApp::render_main_menu_bar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Level", "Ctrl+N")) {
                show_new_level_dialog();
            }
            if (ImGui::MenuItem("Open Level", "Ctrl+O")) {
                show_open_level_dialog();
            }
            if (ImGui::MenuItem("Save Level", "Ctrl+S")) {
                show_save_level_dialog();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                request_quit();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void TilemapEditorApp::render_toolbar()
{
    // TODO: Render tool selection toolbar
    ImGui::Begin("Toolbar");
    ImGui::Text("Tools: Brush, Eraser, Fill, etc.");
    ImGui::End();
}

void TilemapEditorApp::render_tileset_panel()
{
    // TODO: Render tileset panel
    ImGui::Begin("Tilesets");
    ImGui::Text("Tileset list");
    ImGui::End();
}

void TilemapEditorApp::render_tile_groups_panel()
{
    // TODO: Render tile groups panel
}

void TilemapEditorApp::render_tile_list_panel()
{
    // TODO: Render tile list panel
}

void TilemapEditorApp::render_canvas()
{
    // TODO: Render main canvas
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Text("Canvas area");
    ImGui::End();
}

void TilemapEditorApp::render_layer_panel()
{
    // TODO: Render layer panel
    ImGui::Begin("Layers");
    const auto& layers = editor_state_->get_layers();
    for (size_t i = 0; i < layers.size(); ++i) {
        ImGui::Text("%s", layers[i].name.c_str());
    }
    ImGui::End();
}

void TilemapEditorApp::render_properties_panel()
{
    // TODO: Render properties panel
    ImGui::Begin("Properties");
    ImGui::Text("Tile properties");
    ImGui::End();
}

void TilemapEditorApp::render_status_bar()
{
    // TODO: Render status bar
    ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    ImGui::Text("FPS: %d | Tool: %s", current_fps_, editor_state_->get_tool_name().c_str());
    ImGui::End();
}

void TilemapEditorApp::show_new_level_dialog()
{
    // TODO: Show new level dialog
    std::cout << "[TilemapEditorApp] New level dialog" << std::endl;
}

void TilemapEditorApp::show_open_level_dialog()
{
    // TODO: Show open level dialog
    std::cout << "[TilemapEditorApp] Open level dialog" << std::endl;
}

void TilemapEditorApp::show_save_level_dialog()
{
    // TODO: Show save level dialog
    std::cout << "[TilemapEditorApp] Save level dialog" << std::endl;
}

void TilemapEditorApp::show_settings_dialog()
{
    // TODO: Show settings dialog
}

void TilemapEditorApp::show_about_dialog()
{
    // TODO: Show about dialog
}

void TilemapEditorApp::update_window_title()
{
    std::string title = config_.window_title;
    if (level_manager_->has_unsaved_changes()) {
        title += " *";
    }
    SDL_SetWindowTitle(window_, title.c_str());
}

void TilemapEditorApp::handle_unsaved_changes()
{
    if (level_manager_->has_unsaved_changes()) {
        std::cout << "[TilemapEditorApp] Warning: Unsaved changes detected" << std::endl;
        // TODO: Show save confirmation dialog
    }
}

void TilemapEditorApp::calculate_fps()
{
    frame_count_++;
    fps_update_timer_ += delta_time_;
    
    if (fps_update_timer_ >= 1.0f) {
        current_fps_ = frame_count_;
        frame_count_ = 0;
        fps_update_timer_ = 0.0f;
    }
}

void TilemapEditorApp::apply_editor_shortcuts()
{
    // TODO: Apply keyboard shortcuts
}

void TilemapEditorApp::render_grid()
{
    // TODO: Render grid on canvas
}

void TilemapEditorApp::render_tiles()
{
    // TODO: Render tiles on canvas
}

void TilemapEditorApp::render_selection()
{
    // TODO: Render selection rectangle
}

void TilemapEditorApp::render_cursor()
{
    // TODO: Render cursor indicator
}

void TilemapEditorApp::apply_brush_tool(int tile_x, int tile_y)
{
    // TODO: Apply brush tool
}

void TilemapEditorApp::apply_eraser_tool(int tile_x, int tile_y)
{
    // TODO: Apply eraser tool
}

void TilemapEditorApp::apply_fill_tool(int tile_x, int tile_y)
{
    // TODO: Apply fill tool
}

void TilemapEditorApp::apply_rectangle_tool(int start_x, int start_y, int end_x, int end_y)
{
    // TODO: Apply rectangle tool
}

void TilemapEditorApp::apply_eyedropper_tool(int tile_x, int tile_y)
{
    // TODO: Apply eyedropper tool
}

void TilemapEditorApp::screen_to_world(int screen_x, int screen_y, float& out_world_x, float& out_world_y) const
{
    // TODO: Convert screen coordinates to world coordinates
    out_world_x = static_cast<float>(screen_x);
    out_world_y = static_cast<float>(screen_y);
}

void TilemapEditorApp::world_to_screen(float world_x, float world_y, int& out_screen_x, int& out_screen_y) const
{
    // TODO: Convert world coordinates to screen coordinates
    out_screen_x = static_cast<int>(world_x);
    out_screen_y = static_cast<int>(world_y);
}

void TilemapEditorApp::world_to_tile(float world_x, float world_y, int& out_tile_x, int& out_tile_y) const
{
    // TODO: Convert world coordinates to tile coordinates
    out_tile_x = static_cast<int>(world_x / 32);
    out_tile_y = static_cast<int>(world_y / 32);
}

void TilemapEditorApp::tile_to_world(int tile_x, int tile_y, float& out_world_x, float& out_world_y) const
{
    // TODO: Convert tile coordinates to world coordinates
    out_world_x = static_cast<float>(tile_x * 32);
    out_world_y = static_cast<float>(tile_y * 32);
}

} // namespace Editor
} // namespace Olympe
