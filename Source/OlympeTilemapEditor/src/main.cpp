/*
 * Olympe Tilemap Editor - Main Entry Point
 * 
 * Standalone tilemap editor for creating and editing game levels
 * Built with SDL3 and ImGui
 */

#include "../include/TilemapEditorApp.h"
#include <iostream>
#include <exception>

int main(int argc, char* argv[])
{
    std::cout << "=============================================" << std::endl;
    std::cout << "  Olympe Tilemap Editor" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "=============================================" << std::endl;
    
    try {
        // Create application instance
        Olympe::Editor::TilemapEditorApp app;
        
        // Configure application
        Olympe::Editor::AppConfig config;
        config.window_title = "Olympe Tilemap Editor";
        config.window_width = 1920;
        config.window_height = 1080;
        config.window_resizable = true;
        config.window_maximized = true;
        config.vsync_enabled = true;
        config.assets_directory = "Assets";
        config.levels_directory = "Levels";
        config.tilesets_directory = "Tilesets";
        
        // Initialize application
        if (!app.initialize(config)) {
            std::cerr << "[Main] Error: Failed to initialize application" << std::endl;
            return 1;
        }
        
        std::cout << "[Main] Application initialized successfully" << std::endl;
        std::cout << "[Main] Starting main loop..." << std::endl;
        
        // Run main loop
        app.run();
        
        std::cout << "[Main] Main loop ended normally" << std::endl;
        
        // Shutdown application
        app.shutdown();
        
        std::cout << "[Main] Application shutdown complete" << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "  Tilemap Editor closed successfully" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "=============================================" << std::endl;
        std::cerr << "  FATAL ERROR" << std::endl;
        std::cerr << "=============================================" << std::endl;
        std::cerr << "Exception: " << e.what() << std::endl;
        std::cerr << "=============================================" << std::endl;
        return 2;
        
    } catch (...) {
        std::cerr << "=============================================" << std::endl;
        std::cerr << "  FATAL ERROR" << std::endl;
        std::cerr << "=============================================" << std::endl;
        std::cerr << "Unknown exception occurred" << std::endl;
        std::cerr << "=============================================" << std::endl;
        return 3;
    }
}
