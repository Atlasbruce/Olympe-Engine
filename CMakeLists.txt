cmake_minimum_required(VERSION 3.14)
project(OlympeEngine VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_RUNTIME_ENGINE "Build runtime engine executable" ON)
option(BUILD_BLUEPRINT_EDITOR_STANDALONE "Build Blueprint Editor Standalone executable" ON)
option(BUILD_TILEMAP_EDITOR "Build Tilemap Editor executable" ON)

# Find SDL3
find_package(SDL3 REQUIRED)

# ============================================================================
# Shared Blueprint Editor Sources
# ============================================================================
file(GLOB_RECURSE BLUEPRINT_EDITOR_SOURCES "Source/BlueprintEditor/*.cpp")
file(GLOB_RECURSE IMGUI_SOURCES 
    "Source/third_party/imgui/*.cpp" 
    "Source/third_party/imgui/backends/imgui_impl_sdl3.cpp"
    "Source/third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp"
)
file(GLOB_RECURSE IMNODES_SOURCES "Source/third_party/imnodes/*.cpp")
file(GLOB_RECURSE ENGINE_CORE_SOURCES 
    "Source/AI/*.cpp"
    "Source/system/*.cpp"
    "Source/DataManager.cpp"
    "Source/drawing.cpp"
    "Source/InputsManager.cpp"
)

# ============================================================================
# OlympeCore - Shared Library for Blueprint Editor
# ============================================================================
add_library(OlympeCore STATIC
    ${BLUEPRINT_EDITOR_SOURCES}
    ${IMGUI_SOURCES}
    ${IMNODES_SOURCES}
    Source/AI/BehaviorTree.cpp
    Source/DataManager.cpp
    Source/drawing.cpp
)

target_include_directories(OlympeCore PUBLIC
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/third_party
    ${CMAKE_SOURCE_DIR}/Source/third_party/imgui
    ${CMAKE_SOURCE_DIR}/Source/third_party/imnodes
    ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
    ${SDL3_INCLUDE_DIRS}
)

target_link_libraries(OlympeCore PUBLIC SDL3::SDL3)
target_compile_definitions(OlympeCore PUBLIC OLYMPE_BLUEPRINT_EDITOR_ENABLED)

# C++14 standard
set_target_properties(OlympeCore PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

# ============================================================================
# Exécutable 1: Runtime Engine (OlympeEngine.exe)
# ============================================================================
if(BUILD_RUNTIME_ENGINE)
    file(GLOB_RECURSE RUNTIME_SOURCES 
        "Source/OlympeEngine.cpp"
        "Source/GameEngine.cpp"
        "Source/GameState.cpp"
        "Source/World.cpp"
        "Source/VideoGame.cpp"
        "Source/PanelManager.cpp"
        "Source/InputsInspectorPanel.cpp"
        "Source/ECS_Systems.cpp"
        "Source/ECS_Systems_AI.cpp"
        "Source/ECS_Systems_Camera.cpp"
        "Source/ECS_Systems_Rendering_Camera.cpp"
        "Source/engine_utils.cpp"
        "Source/vector.cpp"
        "Source/Sprite.cpp"
    )
    
    add_executable(OlympeEngine ${RUNTIME_SOURCES})
    
    target_link_libraries(OlympeEngine PRIVATE OlympeCore)
    target_compile_definitions(OlympeEngine PRIVATE OLYMPE_RUNTIME_MODE)
    
    set_target_properties(OlympeEngine PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )
    
    # Copy SDL3 DLL to output directory (Windows only)
    if(WIN32)
        add_custom_command(TARGET OlympeEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:OlympeEngine>
        )
    endif()
endif()

# ============================================================================
# Exécutable 2: Blueprint Editor Standalone (OlympeBlueprintEditor.exe)
# ============================================================================
if(BUILD_BLUEPRINT_EDITOR_STANDALONE)
    add_executable(OlympeBlueprintEditor
        Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp
    )
    
    target_link_libraries(OlympeBlueprintEditor PRIVATE OlympeCore)
    target_compile_definitions(OlympeBlueprintEditor PRIVATE OLYMPE_BLUEPRINT_EDITOR_STANDALONE_MODE)
    
    set_target_properties(OlympeBlueprintEditor PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )
    
    # Copy SDL3 DLL to output directory (Windows only)
    if(WIN32)
        add_custom_command(TARGET OlympeBlueprintEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:OlympeBlueprintEditor>
        )
    endif()
    
    # Copy Blueprints directory to output
    add_custom_command(TARGET OlympeBlueprintEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Blueprints
        $<TARGET_FILE_DIR:OlympeBlueprintEditor>/Blueprints
    )
endif()

# ============================================================================
# Installation
# ============================================================================
if(BUILD_RUNTIME_ENGINE)
    install(TARGETS OlympeEngine RUNTIME DESTINATION bin)
endif()

if(BUILD_BLUEPRINT_EDITOR_STANDALONE)
    install(TARGETS OlympeBlueprintEditor RUNTIME DESTINATION bin)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/Blueprints DESTINATION bin)
endif()

# ============================================================================
# Exécutable 3: Tilemap Editor (OlympeTilemapEditor.exe)
# ============================================================================
if(BUILD_TILEMAP_EDITOR)
    add_subdirectory(Source/OlympeTilemapEditor)
endif()

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "============================================")
message(STATUS "  Olympe Engine Build Configuration")
message(STATUS "============================================")
message(STATUS "  C++ Standard: C++14")
message(STATUS "  Runtime Engine: ${BUILD_RUNTIME_ENGINE}")
message(STATUS "  Blueprint Editor Standalone: ${BUILD_BLUEPRINT_EDITOR_STANDALONE}")
message(STATUS "  Tilemap Editor: ${BUILD_TILEMAP_EDITOR}")
message(STATUS "============================================")
