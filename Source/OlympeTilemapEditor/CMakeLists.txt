cmake_minimum_required(VERSION 3.15)
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

# Find SDL3
find_package(SDL3 REQUIRED CONFIG)
if(NOT SDL3_FOUND)
    message(FATAL_ERROR "SDL3 not found! Please install SDL3 or set SDL3_DIR")
endif()
message(STATUS "Found SDL3: ${SDL3_DIR}")

# Find ImGui (assuming it's available via find_package or add_subdirectory)
# Adjust this based on your project's ImGui integration method
if(NOT TARGET imgui)
    find_package(imgui CONFIG)
    if(NOT imgui_FOUND)
        message(WARNING "ImGui not found via find_package, attempting to use existing target")
    endif()
endif()

# Find or use custom json.hpp (nlohmann/json)
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json CONFIG)
    if(NOT nlohmann_json_FOUND)
        message(STATUS "nlohmann_json not found via find_package, assuming custom json.hpp in project")
        # Add custom json.hpp include path if needed
        set(JSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/json/include" CACHE PATH "Path to custom json.hpp")
        if(NOT EXISTS ${JSON_INCLUDE_DIR}/nlohmann/json.hpp)
            message(WARNING "Custom json.hpp not found at ${JSON_INCLUDE_DIR}")
        endif()
    endif()
endif()

# Find fmt library
find_package(fmt REQUIRED CONFIG)
if(NOT fmt_FOUND)
    message(FATAL_ERROR "fmt not found! Please install fmt or set fmt_DIR")
endif()
message(STATUS "Found fmt: ${fmt_DIR}")

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

add_executable(OlympeTilemapEditor ${SOURCE_FILES} ${HEADER_FILES})

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(OlympeTilemapEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../OlympeBlueprintEditor/include
)

# Add custom json.hpp include if not found via find_package
if(NOT nlohmann_json_FOUND AND DEFINED JSON_INCLUDE_DIR)
    target_include_directories(OlympeTilemapEditor PRIVATE ${JSON_INCLUDE_DIR})
endif()

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(OlympeTilemapEditor PRIVATE
    SDL3::SDL3
    fmt::fmt
)

# Link ImGui
if(TARGET imgui)
    target_link_libraries(OlympeTilemapEditor PRIVATE imgui)
elseif(TARGET ImGui::ImGui)
    target_link_libraries(OlympeTilemapEditor PRIVATE ImGui::ImGui)
else()
    message(WARNING "ImGui target not found, you may need to link it manually")
endif()

# Link nlohmann_json if found
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(OlympeTilemapEditor PRIVATE nlohmann_json::nlohmann_json)
endif()

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
if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET OlympeTilemapEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
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
message(STATUS "SDL3 Found: ${SDL3_FOUND}")
message(STATUS "fmt Found: ${fmt_FOUND}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "========================================")
