/*
 * Olympe Blueprint Editor - Main Entry Point
 * 
 * Functional editor for entity blueprints
 * Phase 1: Component property editing
 */

#include "BlueprintEditor.h"
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << "Starting Olympe Blueprint Editor...\n\n";
    
    try
    {
        Olympe::BlueprintEditor editor;
        editor.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
