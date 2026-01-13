cmake_minimum_required(VERSION 3.14)
project(OlympeTilemapEditor VERSION 1.0.0 LANGUAGES CXX)

# Set C++14 standard (matching Olympe Engine)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Find Required Packages
# ============================================================================

# SDL3 is found at the root CMakeLists.txt level
if(NOT TARGET SDL3::SDL3)
    find_package(SDL3 REQUIRED CONFIG)
    if(NOT SDL3_FOUND)
        message(FATAL_ERROR "SDL3 not found! Please install SDL3 or set SDL3_DIR")
    endif()
    message(STATUS "Found SDL3: ${SDL3_DIR}")
endif()

# ImGui sources are included in the parent OlympeCore library
# We'll compile them directly for this standalone editor
file(GLOB IMGUI_SOURCES 
    ${CMAKE_SOURCE_DIR}/Source/third_party/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/third_party/imgui/backends/imgui_impl_sdl3.cpp
    ${CMAKE_SOURCE_DIR}/Source/third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

# ============================================================================
# Source Files
# ============================================================================

set(HEADER_FILES
    include/LevelManager.h
    include/EditorState.h
    include/AssetManager.h
    include/TilesetManager.h
    include/TilemapEditorApp.h
)

set(SOURCE_FILES
    src/LevelManager.cpp
    src/EditorState.cpp
    src/AssetManager.cpp
    src/TilesetManager.cpp
    src/TilemapEditorApp.cpp
    src/main.cpp
)

# ============================================================================
# Create Executable
# ============================================================================

add_executable(OlympeTilemapEditor 
    ${SOURCE_FILES} 
    ${HEADER_FILES}
    ${IMGUI_SOURCES}
)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(OlympeTilemapEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/third_party
    ${CMAKE_SOURCE_DIR}/Source/third_party/imgui
    ${CMAKE_SOURCE_DIR}/Source/third_party/imgui/backends
    ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
)

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(OlympeTilemapEditor PRIVATE
    SDL3::SDL3
)

# ============================================================================
# Compiler Options
# ============================================================================

# Platform-specific compiler flags
if(MSVC)
    target_compile_options(OlympeTilemapEditor PRIVATE
        /W4           # Warning level 4
        /WX-          # Don't treat warnings as errors (remove - to enable)
        /permissive-  # Standards conformance
        /MP           # Multi-processor compilation
    )
    # Windows-specific definitions
    target_compile_definitions(OlympeTilemapEditor PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    target_compile_options(OlympeTilemapEditor PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
    
    # Enable additional warnings for GCC/Clang
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(OlympeTilemapEditor PRIVATE
            -Wshadow
            -Wconversion
            -Wsign-conversion
        )
    endif()
endif()

# ============================================================================
# Platform-Specific Settings
# ============================================================================

if(WIN32)
    # Windows-specific settings
    set_target_properties(OlympeTilemapEditor PROPERTIES
        WIN32_EXECUTABLE OFF  # Set to ON for no console window
    )
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(OlympeTilemapEditor PROPERTIES
        MACOSX_BUNDLE OFF  # Set to ON for app bundle
    )
elseif(UNIX)
    # Linux-specific settings
    target_link_libraries(OlympeTilemapEditor PRIVATE
        ${CMAKE_DL_LIBS}
        pthread
    )
endif()

# ============================================================================
# Post-Build Commands
# ============================================================================

# Create output directories if they don't exist
add_custom_command(TARGET OlympeTilemapEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:OlympeTilemapEditor>/Blueprints"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:OlympeTilemapEditor>/Levels"
    COMMENT "Creating Blueprints and Levels directories"
)

# Copy Blueprints directory if it exists in source
set(BLUEPRINTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Blueprints")
if(EXISTS ${BLUEPRINTS_SOURCE_DIR})
    add_custom_command(TARGET OlympeTilemapEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${BLUEPRINTS_SOURCE_DIR}
            "$<TARGET_FILE_DIR:OlympeTilemapEditor>/Blueprints"
        COMMENT "Copying Blueprints directory"
    )
endif()

# Copy Levels directory if it exists in source
set(LEVELS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Levels")
if(EXISTS ${LEVELS_SOURCE_DIR})
    add_custom_command(TARGET OlympeTilemapEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${LEVELS_SOURCE_DIR}
            "$<TARGET_FILE_DIR:OlympeTilemapEditor>/Levels"
        COMMENT "Copying Levels directory"
    )
endif()

# Copy SDL3 DLL on Windows (if using shared library)
if(WIN32)
    add_custom_command(TARGET OlympeTilemapEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3>"
            "$<TARGET_FILE_DIR:OlympeTilemapEditor>"
        COMMENT "Copying SDL3 DLL"
    )
endif()

# ============================================================================
# Installation Rules (Optional)
# ============================================================================

install(TARGETS OlympeTilemapEditor
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/Blueprints
    ${CMAKE_CURRENT_SOURCE_DIR}/Levels
    DESTINATION bin
    OPTIONAL
)

# ============================================================================
# Print Configuration Summary
# ============================================================================

message(STATUS "========================================")
message(STATUS "OlympeTilemapEditor Configuration")
message(STATUS "========================================")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "========================================")
